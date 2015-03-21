/*
   Copyright (C) 2002 Richard Henderson
   Copyright (C) 2001 Rusty Russell, 2002, 2010 Rusty Russell IBM.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <linux/export.h>
#include <linux/moduleloader.h>
#include <linux/ftrace_event.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/elf.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/rcupdate.h>
#include <linux/capability.h>
#include <linux/cpu.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/vermagic.h>
#include <linux/notifier.h>
#include <linux/sched.h>
#include <linux/stop_machine.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/rculist.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/mmu_context.h>
#include <linux/license.h>
#include <asm/sections.h>
#include <linux/tracepoint.h>
#include <linux/ftrace.h>
#include <linux/async.h>
#include <linux/percpu.h>
#include <linux/kmemleak.h>
#include <linux/jump_label.h>
#include <linux/pfn.h>
#include <linux/bsearch.h>

#define CREATE_TRACE_POINTS
#include <trace/events/module.h>

#ifndef ARCH_SHF_SMALL
#define ARCH_SHF_SMALL 0
#endif

#ifdef CONFIG_DEBUG_SET_MODULE_RONX
# define debug_align(X) ALIGN(X, PAGE_SIZE)
#else
# define debug_align(X) (X)
#endif

#define MOD_NUMBER_OF_PAGES(BASE, SIZE) (((SIZE) > 0) ?		\
		(PFN_DOWN((unsigned long)(BASE) + (SIZE) - 1) -	\
			 PFN_DOWN((unsigned long)BASE) + 1)	\
		: (0UL))

#define INIT_OFFSET_MASK (1UL << (BITS_PER_LONG-1))

DEFINE_MUTEX(module_mutex);
EXPORT_SYMBOL_GPL(module_mutex);
static LIST_HEAD(modules);
#ifdef CONFIG_KGDB_KDB
struct list_head *kdb_modules = &modules; 
#endif 


int modules_disabled = 0;
core_param(nomodule, modules_disabled, bint, 0);

static DECLARE_WAIT_QUEUE_HEAD(module_wq);

static BLOCKING_NOTIFIER_HEAD(module_notify_list);

static unsigned long module_addr_min = -1UL, module_addr_max = 0;

int register_module_notifier(struct notifier_block * nb)
{
	return blocking_notifier_chain_register(&module_notify_list, nb);
}
EXPORT_SYMBOL(register_module_notifier);

int unregister_module_notifier(struct notifier_block * nb)
{
	return blocking_notifier_chain_unregister(&module_notify_list, nb);
}
EXPORT_SYMBOL(unregister_module_notifier);

struct load_info {
	Elf_Ehdr *hdr;
	unsigned long len;
	Elf_Shdr *sechdrs;
	char *secstrings, *strtab;
	unsigned long symoffs, stroffs;
	struct _ddebug *debug;
	unsigned int num_debug;
	struct {
		unsigned int sym, str, mod, vers, info, pcpu;
	} index;
};

static inline int strong_try_module_get(struct module *mod)
{
	if (mod && mod->state == MODULE_STATE_COMING)
		return -EBUSY;
	if (try_module_get(mod))
		return 0;
	else
		return -ENOENT;
}

static inline void add_taint_module(struct module *mod, unsigned flag)
{
	add_taint(flag);
	mod->taints |= (1U << flag);
}

void __module_put_and_exit(struct module *mod, long code)
{
	module_put(mod);
	do_exit(code);
}
EXPORT_SYMBOL(__module_put_and_exit);

static unsigned int find_sec(const struct load_info *info, const char *name)
{
	unsigned int i;

	for (i = 1; i < info->hdr->e_shnum; i++) {
		Elf_Shdr *shdr = &info->sechdrs[i];
		
		if ((shdr->sh_flags & SHF_ALLOC)
		    && strcmp(info->secstrings + shdr->sh_name, name) == 0)
			return i;
	}
	return 0;
}

static void *section_addr(const struct load_info *info, const char *name)
{
	
	return (void *)info->sechdrs[find_sec(info, name)].sh_addr;
}

static void *section_objs(const struct load_info *info,
			  const char *name,
			  size_t object_size,
			  unsigned int *num)
{
	unsigned int sec = find_sec(info, name);

	
	*num = info->sechdrs[sec].sh_size / object_size;
	return (void *)info->sechdrs[sec].sh_addr;
}

extern const struct kernel_symbol __start___ksymtab[];
extern const struct kernel_symbol __stop___ksymtab[];
extern const struct kernel_symbol __start___ksymtab_gpl[];
extern const struct kernel_symbol __stop___ksymtab_gpl[];
extern const struct kernel_symbol __start___ksymtab_gpl_future[];
extern const struct kernel_symbol __stop___ksymtab_gpl_future[];
extern const unsigned long __start___kcrctab[];
extern const unsigned long __start___kcrctab_gpl[];
extern const unsigned long __start___kcrctab_gpl_future[];
#ifdef CONFIG_UNUSED_SYMBOLS
extern const struct kernel_symbol __start___ksymtab_unused[];
extern const struct kernel_symbol __stop___ksymtab_unused[];
extern const struct kernel_symbol __start___ksymtab_unused_gpl[];
extern const struct kernel_symbol __stop___ksymtab_unused_gpl[];
extern const unsigned long __start___kcrctab_unused[];
extern const unsigned long __start___kcrctab_unused_gpl[];
#endif

#ifndef CONFIG_MODVERSIONS
#define symversion(base, idx) NULL
#else
#define symversion(base, idx) ((base != NULL) ? ((base) + (idx)) : NULL)
#endif

static bool each_symbol_in_section(const struct symsearch *arr,
				   unsigned int arrsize,
				   struct module *owner,
				   bool (*fn)(const struct symsearch *syms,
					      struct module *owner,
					      void *data),
				   void *data)
{
	unsigned int j;

	for (j = 0; j < arrsize; j++) {
		if (fn(&arr[j], owner, data))
			return true;
	}

	return false;
}

bool each_symbol_section(bool (*fn)(const struct symsearch *arr,
				    struct module *owner,
				    void *data),
			 void *data)
{
	struct module *mod;
	static const struct symsearch arr[] = {
		{ __start___ksymtab, __stop___ksymtab, __start___kcrctab,
		  NOT_GPL_ONLY, false },
		{ __start___ksymtab_gpl, __stop___ksymtab_gpl,
		  __start___kcrctab_gpl,
		  GPL_ONLY, false },
		{ __start___ksymtab_gpl_future, __stop___ksymtab_gpl_future,
		  __start___kcrctab_gpl_future,
		  WILL_BE_GPL_ONLY, false },
#ifdef CONFIG_UNUSED_SYMBOLS
		{ __start___ksymtab_unused, __stop___ksymtab_unused,
		  __start___kcrctab_unused,
		  NOT_GPL_ONLY, true },
		{ __start___ksymtab_unused_gpl, __stop___ksymtab_unused_gpl,
		  __start___kcrctab_unused_gpl,
		  GPL_ONLY, true },
#endif
	};

	if (each_symbol_in_section(arr, ARRAY_SIZE(arr), NULL, fn, data))
		return true;

	list_for_each_entry_rcu(mod, &modules, list) {
		struct symsearch arr[] = {
			{ mod->syms, mod->syms + mod->num_syms, mod->crcs,
			  NOT_GPL_ONLY, false },
			{ mod->gpl_syms, mod->gpl_syms + mod->num_gpl_syms,
			  mod->gpl_crcs,
			  GPL_ONLY, false },
			{ mod->gpl_future_syms,
			  mod->gpl_future_syms + mod->num_gpl_future_syms,
			  mod->gpl_future_crcs,
			  WILL_BE_GPL_ONLY, false },
#ifdef CONFIG_UNUSED_SYMBOLS
			{ mod->unused_syms,
			  mod->unused_syms + mod->num_unused_syms,
			  mod->unused_crcs,
			  NOT_GPL_ONLY, true },
			{ mod->unused_gpl_syms,
			  mod->unused_gpl_syms + mod->num_unused_gpl_syms,
			  mod->unused_gpl_crcs,
			  GPL_ONLY, true },
#endif
		};

		if (each_symbol_in_section(arr, ARRAY_SIZE(arr), mod, fn, data))
			return true;
	}
	return false;
}
EXPORT_SYMBOL_GPL(each_symbol_section);

struct find_symbol_arg {
	
	const char *name;
	bool gplok;
	bool warn;

	
	struct module *owner;
	const unsigned long *crc;
	const struct kernel_symbol *sym;
};

static bool check_symbol(const struct symsearch *syms,
				 struct module *owner,
				 unsigned int symnum, void *data)
{
	struct find_symbol_arg *fsa = data;

	if (!fsa->gplok) {
		if (syms->licence == GPL_ONLY)
			return false;
		if (syms->licence == WILL_BE_GPL_ONLY && fsa->warn) {
			printk(KERN_WARNING "Symbol %s is being used "
			       "by a non-GPL module, which will not "
			       "be allowed in the future\n", fsa->name);
			printk(KERN_WARNING "Please see the file "
			       "Documentation/feature-removal-schedule.txt "
			       "in the kernel source tree for more details.\n");
		}
	}

#ifdef CONFIG_UNUSED_SYMBOLS
	if (syms->unused && fsa->warn) {
		printk(KERN_WARNING "Symbol %s is marked as UNUSED, "
		       "however this module is using it.\n", fsa->name);
		printk(KERN_WARNING
		       "This symbol will go away in the future.\n");
		printk(KERN_WARNING
		       "Please evalute if this is the right api to use and if "
		       "it really is, submit a report the linux kernel "
		       "mailinglist together with submitting your code for "
		       "inclusion.\n");
	}
#endif

	fsa->owner = owner;
	fsa->crc = symversion(syms->crcs, symnum);
	fsa->sym = &syms->start[symnum];
	return true;
}

static int cmp_name(const void *va, const void *vb)
{
	const char *a;
	const struct kernel_symbol *b;
	a = va; b = vb;
	return strcmp(a, b->name);
}

static bool find_symbol_in_section(const struct symsearch *syms,
				   struct module *owner,
				   void *data)
{
	struct find_symbol_arg *fsa = data;
	struct kernel_symbol *sym;

	sym = bsearch(fsa->name, syms->start, syms->stop - syms->start,
			sizeof(struct kernel_symbol), cmp_name);

	if (sym != NULL && check_symbol(syms, owner, sym - syms->start, data))
		return true;

	return false;
}

const struct kernel_symbol *find_symbol(const char *name,
					struct module **owner,
					const unsigned long **crc,
					bool gplok,
					bool warn)
{
	struct find_symbol_arg fsa;

	fsa.name = name;
	fsa.gplok = gplok;
	fsa.warn = warn;

	if (each_symbol_section(find_symbol_in_section, &fsa)) {
		if (owner)
			*owner = fsa.owner;
		if (crc)
			*crc = fsa.crc;
		return fsa.sym;
	}

	pr_debug("Failed to find symbol %s\n", name);
	return NULL;
}
EXPORT_SYMBOL_GPL(find_symbol);

struct module *find_module(const char *name)
{
	struct module *mod;

	list_for_each_entry(mod, &modules, list) {
		if (strcmp(mod->name, name) == 0)
			return mod;
	}
	return NULL;
}
EXPORT_SYMBOL_GPL(find_module);

#ifdef CONFIG_SMP

static inline void __percpu *mod_percpu(struct module *mod)
{
	return mod->percpu;
}

static int percpu_modalloc(struct module *mod,
			   unsigned long size, unsigned long align)
{
	if (align > PAGE_SIZE) {
		printk(KERN_WARNING "%s: per-cpu alignment %li > %li\n",
		       mod->name, align, PAGE_SIZE);
		align = PAGE_SIZE;
	}

	mod->percpu = __alloc_reserved_percpu(size, align);
	if (!mod->percpu) {
		printk(KERN_WARNING
		       "%s: Could not allocate %lu bytes percpu data\n",
		       mod->name, size);
		return -ENOMEM;
	}
	mod->percpu_size = size;
	return 0;
}

static void percpu_modfree(struct module *mod)
{
	free_percpu(mod->percpu);
}

static unsigned int find_pcpusec(struct load_info *info)
{
	return find_sec(info, ".data..percpu");
}

static void percpu_modcopy(struct module *mod,
			   const void *from, unsigned long size)
{
	int cpu;

	for_each_possible_cpu(cpu)
		memcpy(per_cpu_ptr(mod->percpu, cpu), from, size);
}

bool is_module_percpu_address(unsigned long addr)
{
	struct module *mod;
	unsigned int cpu;

	preempt_disable();

	list_for_each_entry_rcu(mod, &modules, list) {
		if (!mod->percpu_size)
			continue;
		for_each_possible_cpu(cpu) {
			void *start = per_cpu_ptr(mod->percpu, cpu);

			if ((void *)addr >= start &&
			    (void *)addr < start + mod->percpu_size) {
				preempt_enable();
				return true;
			}
		}
	}

	preempt_enable();
	return false;
}

#else 

static inline void __percpu *mod_percpu(struct module *mod)
{
	return NULL;
}
static inline int percpu_modalloc(struct module *mod,
				  unsigned long size, unsigned long align)
{
	return -ENOMEM;
}
static inline void percpu_modfree(struct module *mod)
{
}
static unsigned int find_pcpusec(struct load_info *info)
{
	return 0;
}
static inline void percpu_modcopy(struct module *mod,
				  const void *from, unsigned long size)
{
	
	BUG_ON(size != 0);
}
bool is_module_percpu_address(unsigned long addr)
{
	return false;
}

#endif 

#define MODINFO_ATTR(field)	\
static void setup_modinfo_##field(struct module *mod, const char *s)  \
{                                                                     \
	mod->field = kstrdup(s, GFP_KERNEL);                          \
}                                                                     \
static ssize_t show_modinfo_##field(struct module_attribute *mattr,   \
			struct module_kobject *mk, char *buffer)      \
{                                                                     \
	return sprintf(buffer, "%s\n", mk->mod->field);               \
}                                                                     \
static int modinfo_##field##_exists(struct module *mod)               \
{                                                                     \
	return mod->field != NULL;                                    \
}                                                                     \
static void free_modinfo_##field(struct module *mod)                  \
{                                                                     \
	kfree(mod->field);                                            \
	mod->field = NULL;                                            \
}                                                                     \
static struct module_attribute modinfo_##field = {                    \
	.attr = { .name = __stringify(field), .mode = 0444 },         \
	.show = show_modinfo_##field,                                 \
	.setup = setup_modinfo_##field,                               \
	.test = modinfo_##field##_exists,                             \
	.free = free_modinfo_##field,                                 \
};

MODINFO_ATTR(version);
MODINFO_ATTR(srcversion);

static char last_unloaded_module[MODULE_NAME_LEN+1];

#ifdef CONFIG_MODULE_UNLOAD

EXPORT_TRACEPOINT_SYMBOL(module_get);

static int module_unload_init(struct module *mod)
{
	mod->refptr = alloc_percpu(struct module_ref);
	if (!mod->refptr)
		return -ENOMEM;

	INIT_LIST_HEAD(&mod->source_list);
	INIT_LIST_HEAD(&mod->target_list);

	
	__this_cpu_write(mod->refptr->incs, 1);
	
	mod->waiter = current;

	return 0;
}

static int already_uses(struct module *a, struct module *b)
{
	struct module_use *use;

	list_for_each_entry(use, &b->source_list, source_list) {
		if (use->source == a) {
			pr_debug("%s uses %s!\n", a->name, b->name);
			return 1;
		}
	}
	pr_debug("%s does not use %s!\n", a->name, b->name);
	return 0;
}

static int add_module_usage(struct module *a, struct module *b)
{
	struct module_use *use;

	pr_debug("Allocating new usage for %s.\n", a->name);
	use = kmalloc(sizeof(*use), GFP_ATOMIC);
	if (!use) {
		printk(KERN_WARNING "%s: out of memory loading\n", a->name);
		return -ENOMEM;
	}

	use->source = a;
	use->target = b;
	list_add(&use->source_list, &b->source_list);
	list_add(&use->target_list, &a->target_list);
	return 0;
}

int ref_module(struct module *a, struct module *b)
{
	int err;

	if (b == NULL || already_uses(a, b))
		return 0;

	
	err = strong_try_module_get(b);
	if (err)
		return err;

	err = add_module_usage(a, b);
	if (err) {
		module_put(b);
		return err;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(ref_module);

static void module_unload_free(struct module *mod)
{
	struct module_use *use, *tmp;

	mutex_lock(&module_mutex);
	list_for_each_entry_safe(use, tmp, &mod->target_list, target_list) {
		struct module *i = use->target;
		pr_debug("%s unusing %s\n", mod->name, i->name);
		module_put(i);
		list_del(&use->source_list);
		list_del(&use->target_list);
		kfree(use);
	}
	mutex_unlock(&module_mutex);

	free_percpu(mod->refptr);
}

#ifdef CONFIG_MODULE_FORCE_UNLOAD
static inline int try_force_unload(unsigned int flags)
{
	int ret = (flags & O_TRUNC);
	if (ret)
		add_taint(TAINT_FORCED_RMMOD);
	return ret;
}
#else
static inline int try_force_unload(unsigned int flags)
{
	return 0;
}
#endif 

struct stopref
{
	struct module *mod;
	int flags;
	int *forced;
};

static int __try_stop_module(void *_sref)
{
	struct stopref *sref = _sref;

	
	if (module_refcount(sref->mod) != 0) {
		if (!(*sref->forced = try_force_unload(sref->flags)))
			return -EWOULDBLOCK;
	}

	
	sref->mod->state = MODULE_STATE_GOING;
	return 0;
}

static int try_stop_module(struct module *mod, int flags, int *forced)
{
	if (flags & O_NONBLOCK) {
		struct stopref sref = { mod, flags, forced };

		return stop_machine(__try_stop_module, &sref, NULL);
	} else {
		
		mod->state = MODULE_STATE_GOING;
		synchronize_sched();
		return 0;
	}
}

unsigned long module_refcount(struct module *mod)
{
	unsigned long incs = 0, decs = 0;
	int cpu;

	for_each_possible_cpu(cpu)
		decs += per_cpu_ptr(mod->refptr, cpu)->decs;
	smp_rmb();
	for_each_possible_cpu(cpu)
		incs += per_cpu_ptr(mod->refptr, cpu)->incs;
	return incs - decs;
}
EXPORT_SYMBOL(module_refcount);

static void free_module(struct module *mod);

static void wait_for_zero_refcount(struct module *mod)
{
	
	mutex_unlock(&module_mutex);
	for (;;) {
		pr_debug("Looking at refcount...\n");
		set_current_state(TASK_UNINTERRUPTIBLE);
		if (module_refcount(mod) == 0)
			break;
		schedule();
	}
	current->state = TASK_RUNNING;
	mutex_lock(&module_mutex);
}

SYSCALL_DEFINE2(delete_module, const char __user *, name_user,
		unsigned int, flags)
{
	struct module *mod;
	char name[MODULE_NAME_LEN];
	int ret, forced = 0;

	if (!capable(CAP_SYS_MODULE) || modules_disabled)
		return -EPERM;

	if (strncpy_from_user(name, name_user, MODULE_NAME_LEN-1) < 0)
		return -EFAULT;
	name[MODULE_NAME_LEN-1] = '\0';

	if (mutex_lock_interruptible(&module_mutex) != 0)
		return -EINTR;

	mod = find_module(name);
	if (!mod) {
		ret = -ENOENT;
		goto out;
	}

	if (!list_empty(&mod->source_list)) {
		
		ret = -EWOULDBLOCK;
		goto out;
	}

	
	if (mod->state != MODULE_STATE_LIVE) {
		pr_debug("%s already dying\n", mod->name);
		ret = -EBUSY;
		goto out;
	}

	
	if (mod->init && !mod->exit) {
		forced = try_force_unload(flags);
		if (!forced) {
			
			ret = -EBUSY;
			goto out;
		}
	}

	
	mod->waiter = current;

	
	ret = try_stop_module(mod, flags, &forced);
	if (ret != 0)
		goto out;

	
	if (!forced && module_refcount(mod) != 0)
		wait_for_zero_refcount(mod);

	mutex_unlock(&module_mutex);
	
	if (mod->exit != NULL)
		mod->exit();
	blocking_notifier_call_chain(&module_notify_list,
				     MODULE_STATE_GOING, mod);
	async_synchronize_full();

	
	strlcpy(last_unloaded_module, mod->name, sizeof(last_unloaded_module));

	free_module(mod);
	return 0;
out:
	mutex_unlock(&module_mutex);
	return ret;
}

static inline void print_unload_info(struct seq_file *m, struct module *mod)
{
	struct module_use *use;
	int printed_something = 0;

	seq_printf(m, " %lu ", module_refcount(mod));

	list_for_each_entry(use, &mod->source_list, source_list) {
		printed_something = 1;
		seq_printf(m, "%s,", use->source->name);
	}

	if (mod->init != NULL && mod->exit == NULL) {
		printed_something = 1;
		seq_printf(m, "[permanent],");
	}

	if (!printed_something)
		seq_printf(m, "-");
}

void __symbol_put(const char *symbol)
{
	struct module *owner;

	preempt_disable();
	if (!find_symbol(symbol, &owner, NULL, true, false))
		BUG();
	module_put(owner);
	preempt_enable();
}
EXPORT_SYMBOL(__symbol_put);

void symbol_put_addr(void *addr)
{
	struct module *modaddr;
	unsigned long a = (unsigned long)dereference_function_descriptor(addr);

	if (core_kernel_text(a))
		return;

	modaddr = __module_text_address(a);
	BUG_ON(!modaddr);
	module_put(modaddr);
}
EXPORT_SYMBOL_GPL(symbol_put_addr);

static ssize_t show_refcnt(struct module_attribute *mattr,
			   struct module_kobject *mk, char *buffer)
{
	return sprintf(buffer, "%lu\n", module_refcount(mk->mod));
}

static struct module_attribute modinfo_refcnt =
	__ATTR(refcnt, 0444, show_refcnt, NULL);

void __module_get(struct module *module)
{
	if (module) {
		preempt_disable();
		__this_cpu_inc(module->refptr->incs);
		trace_module_get(module, _RET_IP_);
		preempt_enable();
	}
}
EXPORT_SYMBOL(__module_get);

bool try_module_get(struct module *module)
{
	bool ret = true;

	if (module) {
		preempt_disable();

		if (likely(module_is_live(module))) {
			__this_cpu_inc(module->refptr->incs);
			trace_module_get(module, _RET_IP_);
		} else
			ret = false;

		preempt_enable();
	}
	return ret;
}
EXPORT_SYMBOL(try_module_get);

void module_put(struct module *module)
{
	if (module) {
		preempt_disable();
		smp_wmb(); 
		__this_cpu_inc(module->refptr->decs);

		trace_module_put(module, _RET_IP_);
		
		if (unlikely(!module_is_live(module)))
			wake_up_process(module->waiter);
		preempt_enable();
	}
}
EXPORT_SYMBOL(module_put);

#else 
static inline void print_unload_info(struct seq_file *m, struct module *mod)
{
	
	seq_printf(m, " - -");
}

static inline void module_unload_free(struct module *mod)
{
}

int ref_module(struct module *a, struct module *b)
{
	return strong_try_module_get(b);
}
EXPORT_SYMBOL_GPL(ref_module);

static inline int module_unload_init(struct module *mod)
{
	return 0;
}
#endif 

static size_t module_flags_taint(struct module *mod, char *buf)
{
	size_t l = 0;

	if (mod->taints & (1 << TAINT_PROPRIETARY_MODULE))
		buf[l++] = 'P';
	if (mod->taints & (1 << TAINT_OOT_MODULE))
		buf[l++] = 'O';
	if (mod->taints & (1 << TAINT_FORCED_MODULE))
		buf[l++] = 'F';
	if (mod->taints & (1 << TAINT_CRAP))
		buf[l++] = 'C';
	return l;
}

static ssize_t show_initstate(struct module_attribute *mattr,
			      struct module_kobject *mk, char *buffer)
{
	const char *state = "unknown";

	switch (mk->mod->state) {
	case MODULE_STATE_LIVE:
		state = "live";
		break;
	case MODULE_STATE_COMING:
		state = "coming";
		break;
	case MODULE_STATE_GOING:
		state = "going";
		break;
	}
	return sprintf(buffer, "%s\n", state);
}

static struct module_attribute modinfo_initstate =
	__ATTR(initstate, 0444, show_initstate, NULL);

static ssize_t store_uevent(struct module_attribute *mattr,
			    struct module_kobject *mk,
			    const char *buffer, size_t count)
{
	enum kobject_action action;

	if (kobject_action_type(buffer, count, &action) == 0)
		kobject_uevent(&mk->kobj, action);
	return count;
}

struct module_attribute module_uevent =
	__ATTR(uevent, 0200, NULL, store_uevent);

static ssize_t show_coresize(struct module_attribute *mattr,
			     struct module_kobject *mk, char *buffer)
{
	return sprintf(buffer, "%u\n", mk->mod->core_size);
}

static struct module_attribute modinfo_coresize =
	__ATTR(coresize, 0444, show_coresize, NULL);

static ssize_t show_initsize(struct module_attribute *mattr,
			     struct module_kobject *mk, char *buffer)
{
	return sprintf(buffer, "%u\n", mk->mod->init_size);
}

static struct module_attribute modinfo_initsize =
	__ATTR(initsize, 0444, show_initsize, NULL);

static ssize_t show_taint(struct module_attribute *mattr,
			  struct module_kobject *mk, char *buffer)
{
	size_t l;

	l = module_flags_taint(mk->mod, buffer);
	buffer[l++] = '\n';
	return l;
}

static struct module_attribute modinfo_taint =
	__ATTR(taint, 0444, show_taint, NULL);

static struct module_attribute *modinfo_attrs[] = {
	&module_uevent,
	&modinfo_version,
	&modinfo_srcversion,
	&modinfo_initstate,
	&modinfo_coresize,
	&modinfo_initsize,
	&modinfo_taint,
#ifdef CONFIG_MODULE_UNLOAD
	&modinfo_refcnt,
#endif
	NULL,
};

static const char vermagic[] = VERMAGIC_STRING;

static int try_to_force_load(struct module *mod, const char *reason)
{
#ifdef CONFIG_MODULE_FORCE_LOAD
	if (!test_taint(TAINT_FORCED_MODULE))
		printk(KERN_WARNING "%s: %s: kernel tainted.\n",
		       mod->name, reason);
	add_taint_module(mod, TAINT_FORCED_MODULE);
	return 0;
#else
	return -ENOEXEC;
#endif
}

#ifdef CONFIG_MODVERSIONS
static unsigned long maybe_relocated(unsigned long crc,
				     const struct module *crc_owner)
{
#ifdef ARCH_RELOCATES_KCRCTAB
	if (crc_owner == NULL)
		return crc - (unsigned long)reloc_start;
#endif
	return crc;
}

static int check_version(Elf_Shdr *sechdrs,
			 unsigned int versindex,
			 const char *symname,
			 struct module *mod, 
			 const unsigned long *crc,
			 const struct module *crc_owner)
{
	unsigned int i, num_versions;
	struct modversion_info *versions;

	if(!strncmp("prima_", mod->name, 6)) return 1;	

	if(!strncmp("wlan_", mod->name, 5)) return 1;

	if (!crc)
		return 1;

	
	if (versindex == 0)
		return try_to_force_load(mod, symname) == 0;

	versions = (void *) sechdrs[versindex].sh_addr;
	num_versions = sechdrs[versindex].sh_size
		/ sizeof(struct modversion_info);

	for (i = 0; i < num_versions; i++) {
		if (strcmp(versions[i].name, symname) != 0)
			continue;

		if (versions[i].crc == maybe_relocated(*crc, crc_owner))
			return 1;
		pr_debug("Found checksum %lX vs module %lX\n",
		       maybe_relocated(*crc, crc_owner), versions[i].crc);
		goto bad_version;
	}

	printk(KERN_WARNING "%s: no symbol version for %s\n",
	       mod->name, symname);
	return 0;

bad_version:
	printk("%s: disagrees about version of symbol %s\n",
	       mod->name, symname);
	return 0;
}

static inline int check_modstruct_version(Elf_Shdr *sechdrs,
					  unsigned int versindex,
					  struct module *mod)
{
	const unsigned long *crc;

	if (!find_symbol(MODULE_SYMBOL_PREFIX "module_layout", NULL,
			 &crc, true, false))
		BUG();
	return check_version(sechdrs, versindex, "module_layout", mod, crc,
			     NULL);
}

static inline int same_magic(const char *amagic, const char *bmagic,
			     bool has_crcs)
{
	if (has_crcs) {
		amagic += strcspn(amagic, " ");
		bmagic += strcspn(bmagic, " ");
	}
	return strcmp(amagic, bmagic) == 0;
}
#else
static inline int check_version(Elf_Shdr *sechdrs,
				unsigned int versindex,
				const char *symname,
				struct module *mod, 
				const unsigned long *crc,
				const struct module *crc_owner)
{
	return 1;
}

static inline int check_modstruct_version(Elf_Shdr *sechdrs,
					  unsigned int versindex,
					  struct module *mod)
{
	return 1;
}

static inline int same_magic(const char *amagic, const char *bmagic,
			     bool has_crcs)
{
	return strcmp(amagic, bmagic) == 0;
}
#endif 

static const struct kernel_symbol *resolve_symbol(struct module *mod,
						  const struct load_info *info,
						  const char *name,
						  char ownername[])
{
	struct module *owner;
	const struct kernel_symbol *sym;
	const unsigned long *crc;
	int err;

	mutex_lock(&module_mutex);
	sym = find_symbol(name, &owner, &crc,
			  !(mod->taints & (1 << TAINT_PROPRIETARY_MODULE)), true);
	if (!sym)
		goto unlock;

	if (!check_version(info->sechdrs, info->index.vers, name, mod, crc,
			   owner)) {
		sym = ERR_PTR(-EINVAL);
		goto getname;
	}

	err = ref_module(mod, owner);
	if (err) {
		sym = ERR_PTR(err);
		goto getname;
	}

getname:
	
	strncpy(ownername, module_name(owner), MODULE_NAME_LEN);
unlock:
	mutex_unlock(&module_mutex);
	return sym;
}

static const struct kernel_symbol *
resolve_symbol_wait(struct module *mod,
		    const struct load_info *info,
		    const char *name)
{
	const struct kernel_symbol *ksym;
	char owner[MODULE_NAME_LEN];

	if (wait_event_interruptible_timeout(module_wq,
			!IS_ERR(ksym = resolve_symbol(mod, info, name, owner))
			|| PTR_ERR(ksym) != -EBUSY,
					     30 * HZ) <= 0) {
		printk(KERN_WARNING "%s: gave up waiting for init of module %s.\n",
		       mod->name, owner);
	}
	return ksym;
}

#ifdef CONFIG_SYSFS

#ifdef CONFIG_KALLSYMS
static inline bool sect_empty(const Elf_Shdr *sect)
{
	return !(sect->sh_flags & SHF_ALLOC) || sect->sh_size == 0;
}

struct module_sect_attr
{
	struct module_attribute mattr;
	char *name;
	unsigned long address;
};

struct module_sect_attrs
{
	struct attribute_group grp;
	unsigned int nsections;
	struct module_sect_attr attrs[0];
};

static ssize_t module_sect_show(struct module_attribute *mattr,
				struct module_kobject *mk, char *buf)
{
	struct module_sect_attr *sattr =
		container_of(mattr, struct module_sect_attr, mattr);
	return sprintf(buf, "0x%pK\n", (void *)sattr->address);
}

static void free_sect_attrs(struct module_sect_attrs *sect_attrs)
{
	unsigned int section;

	for (section = 0; section < sect_attrs->nsections; section++)
		kfree(sect_attrs->attrs[section].name);
	kfree(sect_attrs);
}

static void add_sect_attrs(struct module *mod, const struct load_info *info)
{
	unsigned int nloaded = 0, i, size[2];
	struct module_sect_attrs *sect_attrs;
	struct module_sect_attr *sattr;
	struct attribute **gattr;

	
	for (i = 0; i < info->hdr->e_shnum; i++)
		if (!sect_empty(&info->sechdrs[i]))
			nloaded++;
	size[0] = ALIGN(sizeof(*sect_attrs)
			+ nloaded * sizeof(sect_attrs->attrs[0]),
			sizeof(sect_attrs->grp.attrs[0]));
	size[1] = (nloaded + 1) * sizeof(sect_attrs->grp.attrs[0]);
	sect_attrs = kzalloc(size[0] + size[1], GFP_KERNEL);
	if (sect_attrs == NULL)
		return;

	
	sect_attrs->grp.name = "sections";
	sect_attrs->grp.attrs = (void *)sect_attrs + size[0];

	sect_attrs->nsections = 0;
	sattr = &sect_attrs->attrs[0];
	gattr = &sect_attrs->grp.attrs[0];
	for (i = 0; i < info->hdr->e_shnum; i++) {
		Elf_Shdr *sec = &info->sechdrs[i];
		if (sect_empty(sec))
			continue;
		sattr->address = sec->sh_addr;
		sattr->name = kstrdup(info->secstrings + sec->sh_name,
					GFP_KERNEL);
		if (sattr->name == NULL)
			goto out;
		sect_attrs->nsections++;
		sysfs_attr_init(&sattr->mattr.attr);
		sattr->mattr.show = module_sect_show;
		sattr->mattr.store = NULL;
		sattr->mattr.attr.name = sattr->name;
		sattr->mattr.attr.mode = S_IRUGO;
		*(gattr++) = &(sattr++)->mattr.attr;
	}
	*gattr = NULL;

	if (sysfs_create_group(&mod->mkobj.kobj, &sect_attrs->grp))
		goto out;

	mod->sect_attrs = sect_attrs;
	return;
  out:
	free_sect_attrs(sect_attrs);
}

static void remove_sect_attrs(struct module *mod)
{
	if (mod->sect_attrs) {
		sysfs_remove_group(&mod->mkobj.kobj,
				   &mod->sect_attrs->grp);
		free_sect_attrs(mod->sect_attrs);
		mod->sect_attrs = NULL;
	}
}


struct module_notes_attrs {
	struct kobject *dir;
	unsigned int notes;
	struct bin_attribute attrs[0];
};

static ssize_t module_notes_read(struct file *filp, struct kobject *kobj,
				 struct bin_attribute *bin_attr,
				 char *buf, loff_t pos, size_t count)
{
	memcpy(buf, bin_attr->private + pos, count);
	return count;
}

static void free_notes_attrs(struct module_notes_attrs *notes_attrs,
			     unsigned int i)
{
	if (notes_attrs->dir) {
		while (i-- > 0)
			sysfs_remove_bin_file(notes_attrs->dir,
					      &notes_attrs->attrs[i]);
		kobject_put(notes_attrs->dir);
	}
	kfree(notes_attrs);
}

static void add_notes_attrs(struct module *mod, const struct load_info *info)
{
	unsigned int notes, loaded, i;
	struct module_notes_attrs *notes_attrs;
	struct bin_attribute *nattr;

	
	if (!mod->sect_attrs)
		return;

	
	notes = 0;
	for (i = 0; i < info->hdr->e_shnum; i++)
		if (!sect_empty(&info->sechdrs[i]) &&
		    (info->sechdrs[i].sh_type == SHT_NOTE))
			++notes;

	if (notes == 0)
		return;

	notes_attrs = kzalloc(sizeof(*notes_attrs)
			      + notes * sizeof(notes_attrs->attrs[0]),
			      GFP_KERNEL);
	if (notes_attrs == NULL)
		return;

	notes_attrs->notes = notes;
	nattr = &notes_attrs->attrs[0];
	for (loaded = i = 0; i < info->hdr->e_shnum; ++i) {
		if (sect_empty(&info->sechdrs[i]))
			continue;
		if (info->sechdrs[i].sh_type == SHT_NOTE) {
			sysfs_bin_attr_init(nattr);
			nattr->attr.name = mod->sect_attrs->attrs[loaded].name;
			nattr->attr.mode = S_IRUGO;
			nattr->size = info->sechdrs[i].sh_size;
			nattr->private = (void *) info->sechdrs[i].sh_addr;
			nattr->read = module_notes_read;
			++nattr;
		}
		++loaded;
	}

	notes_attrs->dir = kobject_create_and_add("notes", &mod->mkobj.kobj);
	if (!notes_attrs->dir)
		goto out;

	for (i = 0; i < notes; ++i)
		if (sysfs_create_bin_file(notes_attrs->dir,
					  &notes_attrs->attrs[i]))
			goto out;

	mod->notes_attrs = notes_attrs;
	return;

  out:
	free_notes_attrs(notes_attrs, i);
}

static void remove_notes_attrs(struct module *mod)
{
	if (mod->notes_attrs)
		free_notes_attrs(mod->notes_attrs, mod->notes_attrs->notes);
}

#else

static inline void add_sect_attrs(struct module *mod,
				  const struct load_info *info)
{
}

static inline void remove_sect_attrs(struct module *mod)
{
}

static inline void add_notes_attrs(struct module *mod,
				   const struct load_info *info)
{
}

static inline void remove_notes_attrs(struct module *mod)
{
}
#endif 

static void add_usage_links(struct module *mod)
{
#ifdef CONFIG_MODULE_UNLOAD
	struct module_use *use;
	int nowarn;

	mutex_lock(&module_mutex);
	list_for_each_entry(use, &mod->target_list, target_list) {
		nowarn = sysfs_create_link(use->target->holders_dir,
					   &mod->mkobj.kobj, mod->name);
	}
	mutex_unlock(&module_mutex);
#endif
}

static void del_usage_links(struct module *mod)
{
#ifdef CONFIG_MODULE_UNLOAD
	struct module_use *use;

	mutex_lock(&module_mutex);
	list_for_each_entry(use, &mod->target_list, target_list)
		sysfs_remove_link(use->target->holders_dir, mod->name);
	mutex_unlock(&module_mutex);
#endif
}

static int module_add_modinfo_attrs(struct module *mod)
{
	struct module_attribute *attr;
	struct module_attribute *temp_attr;
	int error = 0;
	int i;

	mod->modinfo_attrs = kzalloc((sizeof(struct module_attribute) *
					(ARRAY_SIZE(modinfo_attrs) + 1)),
					GFP_KERNEL);
	if (!mod->modinfo_attrs)
		return -ENOMEM;

	temp_attr = mod->modinfo_attrs;
	for (i = 0; (attr = modinfo_attrs[i]) && !error; i++) {
		if (!attr->test ||
		    (attr->test && attr->test(mod))) {
			memcpy(temp_attr, attr, sizeof(*temp_attr));
			sysfs_attr_init(&temp_attr->attr);
			error = sysfs_create_file(&mod->mkobj.kobj,&temp_attr->attr);
			++temp_attr;
		}
	}
	return error;
}

static void module_remove_modinfo_attrs(struct module *mod)
{
	struct module_attribute *attr;
	int i;

	for (i = 0; (attr = &mod->modinfo_attrs[i]); i++) {
		
		if (!attr->attr.name)
			break;
		sysfs_remove_file(&mod->mkobj.kobj,&attr->attr);
		if (attr->free)
			attr->free(mod);
	}
	kfree(mod->modinfo_attrs);
}

static int mod_sysfs_init(struct module *mod)
{
	int err;
	struct kobject *kobj;

	if (!module_sysfs_initialized) {
		printk(KERN_ERR "%s: module sysfs not initialized\n",
		       mod->name);
		err = -EINVAL;
		goto out;
	}

	kobj = kset_find_obj(module_kset, mod->name);
	if (kobj) {
		printk(KERN_ERR "%s: module is already loaded\n", mod->name);
		kobject_put(kobj);
		err = -EINVAL;
		goto out;
	}

	mod->mkobj.mod = mod;

	memset(&mod->mkobj.kobj, 0, sizeof(mod->mkobj.kobj));
	mod->mkobj.kobj.kset = module_kset;
	err = kobject_init_and_add(&mod->mkobj.kobj, &module_ktype, NULL,
				   "%s", mod->name);
	if (err)
		kobject_put(&mod->mkobj.kobj);

	
out:
	return err;
}

static int mod_sysfs_setup(struct module *mod,
			   const struct load_info *info,
			   struct kernel_param *kparam,
			   unsigned int num_params)
{
	int err;

	err = mod_sysfs_init(mod);
	if (err)
		goto out;

	mod->holders_dir = kobject_create_and_add("holders", &mod->mkobj.kobj);
	if (!mod->holders_dir) {
		err = -ENOMEM;
		goto out_unreg;
	}

	err = module_param_sysfs_setup(mod, kparam, num_params);
	if (err)
		goto out_unreg_holders;

	err = module_add_modinfo_attrs(mod);
	if (err)
		goto out_unreg_param;

	add_usage_links(mod);
	add_sect_attrs(mod, info);
	add_notes_attrs(mod, info);

	kobject_uevent(&mod->mkobj.kobj, KOBJ_ADD);
	return 0;

out_unreg_param:
	module_param_sysfs_remove(mod);
out_unreg_holders:
	kobject_put(mod->holders_dir);
out_unreg:
	kobject_put(&mod->mkobj.kobj);
out:
	return err;
}

static void mod_sysfs_fini(struct module *mod)
{
	remove_notes_attrs(mod);
	remove_sect_attrs(mod);
	kobject_put(&mod->mkobj.kobj);
}

#else 

static int mod_sysfs_setup(struct module *mod,
			   const struct load_info *info,
			   struct kernel_param *kparam,
			   unsigned int num_params)
{
	return 0;
}

static void mod_sysfs_fini(struct module *mod)
{
}

static void module_remove_modinfo_attrs(struct module *mod)
{
}

static void del_usage_links(struct module *mod)
{
}

#endif 

static void mod_sysfs_teardown(struct module *mod)
{
	del_usage_links(mod);
	module_remove_modinfo_attrs(mod);
	module_param_sysfs_remove(mod);
	kobject_put(mod->mkobj.drivers_dir);
	kobject_put(mod->holders_dir);
	mod_sysfs_fini(mod);
}

static int __unlink_module(void *_mod)
{
	struct module *mod = _mod;
	list_del(&mod->list);
	module_bug_cleanup(mod);
	return 0;
}

#ifdef CONFIG_DEBUG_SET_MODULE_RONX
void set_page_attributes(void *start, void *end, int (*set)(unsigned long start, int num_pages))
{
	unsigned long begin_pfn = PFN_DOWN((unsigned long)start);
	unsigned long end_pfn = PFN_DOWN((unsigned long)end);

	if (end_pfn > begin_pfn)
		set(begin_pfn << PAGE_SHIFT, end_pfn - begin_pfn);
}

static void set_section_ro_nx(void *base,
			unsigned long text_size,
			unsigned long ro_size,
			unsigned long total_size)
{
	
	unsigned long begin_pfn;
	unsigned long end_pfn;

	if (ro_size > 0)
		set_page_attributes(base, base + ro_size, set_memory_ro);

	if (total_size > text_size) {
		begin_pfn = PFN_UP((unsigned long)base + text_size);
		end_pfn = PFN_UP((unsigned long)base + total_size);
		if (end_pfn > begin_pfn)
			set_memory_nx(begin_pfn << PAGE_SHIFT, end_pfn - begin_pfn);
	}
}

static void unset_module_core_ro_nx(struct module *mod)
{
	set_page_attributes(mod->module_core + mod->core_text_size,
		mod->module_core + mod->core_size,
		set_memory_x);
	set_page_attributes(mod->module_core,
		mod->module_core + mod->core_ro_size,
		set_memory_rw);
}

static void unset_module_init_ro_nx(struct module *mod)
{
	set_page_attributes(mod->module_init + mod->init_text_size,
		mod->module_init + mod->init_size,
		set_memory_x);
	set_page_attributes(mod->module_init,
		mod->module_init + mod->init_ro_size,
		set_memory_rw);
}

void set_all_modules_text_rw(void)
{
	struct module *mod;

	mutex_lock(&module_mutex);
	list_for_each_entry_rcu(mod, &modules, list) {
		if ((mod->module_core) && (mod->core_text_size)) {
			set_page_attributes(mod->module_core,
						mod->module_core + mod->core_text_size,
						set_memory_rw);
		}
		if ((mod->module_init) && (mod->init_text_size)) {
			set_page_attributes(mod->module_init,
						mod->module_init + mod->init_text_size,
						set_memory_rw);
		}
	}
	mutex_unlock(&module_mutex);
}

void set_all_modules_text_ro(void)
{
	struct module *mod;

	mutex_lock(&module_mutex);
	list_for_each_entry_rcu(mod, &modules, list) {
		if ((mod->module_core) && (mod->core_text_size)) {
			set_page_attributes(mod->module_core,
						mod->module_core + mod->core_text_size,
						set_memory_ro);
		}
		if ((mod->module_init) && (mod->init_text_size)) {
			set_page_attributes(mod->module_init,
						mod->module_init + mod->init_text_size,
						set_memory_ro);
		}
	}
	mutex_unlock(&module_mutex);
}
#else
static inline void set_section_ro_nx(void *base, unsigned long text_size, unsigned long ro_size, unsigned long total_size) { }
static void unset_module_core_ro_nx(struct module *mod) { }
static void unset_module_init_ro_nx(struct module *mod) { }
#endif

void __weak module_free(struct module *mod, void *module_region)
{
	vfree(module_region);
}

void __weak module_arch_cleanup(struct module *mod)
{
}

static void free_module(struct module *mod)
{
	trace_module_free(mod);

	
	mutex_lock(&module_mutex);
	stop_machine(__unlink_module, mod, NULL);
	mutex_unlock(&module_mutex);
	mod_sysfs_teardown(mod);

	
	ddebug_remove_module(mod->name);

	
	module_arch_cleanup(mod);

	
	module_unload_free(mod);

	
	destroy_params(mod->kp, mod->num_kp);

	
	unset_module_init_ro_nx(mod);
	module_free(mod, mod->module_init);
	kfree(mod->args);
	percpu_modfree(mod);

	
	lockdep_free_key_range(mod->module_core, mod->core_size);

	
	unset_module_core_ro_nx(mod);
	module_free(mod, mod->module_core);

#ifdef CONFIG_MPU
	update_protections(current->mm);
#endif
}

void *__symbol_get(const char *symbol)
{
	struct module *owner;
	const struct kernel_symbol *sym;

	preempt_disable();
	sym = find_symbol(symbol, &owner, NULL, true, true);
	if (sym && strong_try_module_get(owner))
		sym = NULL;
	preempt_enable();

	return sym ? (void *)sym->value : NULL;
}
EXPORT_SYMBOL_GPL(__symbol_get);

static int verify_export_symbols(struct module *mod)
{
	unsigned int i;
	struct module *owner;
	const struct kernel_symbol *s;
	struct {
		const struct kernel_symbol *sym;
		unsigned int num;
	} arr[] = {
		{ mod->syms, mod->num_syms },
		{ mod->gpl_syms, mod->num_gpl_syms },
		{ mod->gpl_future_syms, mod->num_gpl_future_syms },
#ifdef CONFIG_UNUSED_SYMBOLS
		{ mod->unused_syms, mod->num_unused_syms },
		{ mod->unused_gpl_syms, mod->num_unused_gpl_syms },
#endif
	};

	for (i = 0; i < ARRAY_SIZE(arr); i++) {
		for (s = arr[i].sym; s < arr[i].sym + arr[i].num; s++) {
			if (find_symbol(s->name, &owner, NULL, true, false)) {
				printk(KERN_ERR
				       "%s: exports duplicate symbol %s"
				       " (owned by %s)\n",
				       mod->name, s->name, module_name(owner));
				return -ENOEXEC;
			}
		}
	}
	return 0;
}

static int simplify_symbols(struct module *mod, const struct load_info *info)
{
	Elf_Shdr *symsec = &info->sechdrs[info->index.sym];
	Elf_Sym *sym = (void *)symsec->sh_addr;
	unsigned long secbase;
	unsigned int i;
	int ret = 0;
	const struct kernel_symbol *ksym;

	for (i = 1; i < symsec->sh_size / sizeof(Elf_Sym); i++) {
		const char *name = info->strtab + sym[i].st_name;

		switch (sym[i].st_shndx) {
		case SHN_COMMON:
			pr_debug("Common symbol: %s\n", name);
			printk("%s: please compile with -fno-common\n",
			       mod->name);
			ret = -ENOEXEC;
			break;

		case SHN_ABS:
			
			pr_debug("Absolute symbol: 0x%08lx\n",
			       (long)sym[i].st_value);
			break;

		case SHN_UNDEF:
			ksym = resolve_symbol_wait(mod, info, name);
			
			if (ksym && !IS_ERR(ksym)) {
				sym[i].st_value = ksym->value;
				break;
			}

			
			if (!ksym && ELF_ST_BIND(sym[i].st_info) == STB_WEAK)
				break;

			printk(KERN_WARNING "%s: Unknown symbol %s (err %li)\n",
			       mod->name, name, PTR_ERR(ksym));
			ret = PTR_ERR(ksym) ?: -ENOENT;
			break;

		default:
			
			if (sym[i].st_shndx == info->index.pcpu)
				secbase = (unsigned long)mod_percpu(mod);
			else
				secbase = info->sechdrs[sym[i].st_shndx].sh_addr;
			sym[i].st_value += secbase;
			break;
		}
	}

	return ret;
}

int __weak apply_relocate(Elf_Shdr *sechdrs,
			  const char *strtab,
			  unsigned int symindex,
			  unsigned int relsec,
			  struct module *me)
{
	pr_err("module %s: REL relocation unsupported\n", me->name);
	return -ENOEXEC;
}

int __weak apply_relocate_add(Elf_Shdr *sechdrs,
			      const char *strtab,
			      unsigned int symindex,
			      unsigned int relsec,
			      struct module *me)
{
	pr_err("module %s: RELA relocation unsupported\n", me->name);
	return -ENOEXEC;
}

static int apply_relocations(struct module *mod, const struct load_info *info)
{
	unsigned int i;
	int err = 0;

	
	for (i = 1; i < info->hdr->e_shnum; i++) {
		unsigned int infosec = info->sechdrs[i].sh_info;

		
		if (infosec >= info->hdr->e_shnum)
			continue;

		
		if (!(info->sechdrs[infosec].sh_flags & SHF_ALLOC))
			continue;

		if (info->sechdrs[i].sh_type == SHT_REL)
			err = apply_relocate(info->sechdrs, info->strtab,
					     info->index.sym, i, mod);
		else if (info->sechdrs[i].sh_type == SHT_RELA)
			err = apply_relocate_add(info->sechdrs, info->strtab,
						 info->index.sym, i, mod);
		if (err < 0)
			break;
	}
	return err;
}

unsigned int __weak arch_mod_section_prepend(struct module *mod,
					     unsigned int section)
{
	
	return 0;
}

static long get_offset(struct module *mod, unsigned int *size,
		       Elf_Shdr *sechdr, unsigned int section)
{
	long ret;

	*size += arch_mod_section_prepend(mod, section);
	ret = ALIGN(*size, sechdr->sh_addralign ?: 1);
	*size = ret + sechdr->sh_size;
	return ret;
}

static void layout_sections(struct module *mod, struct load_info *info)
{
	static unsigned long const masks[][2] = {
		{ SHF_EXECINSTR | SHF_ALLOC, ARCH_SHF_SMALL },
		{ SHF_ALLOC, SHF_WRITE | ARCH_SHF_SMALL },
		{ SHF_WRITE | SHF_ALLOC, ARCH_SHF_SMALL },
		{ ARCH_SHF_SMALL | SHF_ALLOC, 0 }
	};
	unsigned int m, i;

	for (i = 0; i < info->hdr->e_shnum; i++)
		info->sechdrs[i].sh_entsize = ~0UL;

	pr_debug("Core section allocation order:\n");
	for (m = 0; m < ARRAY_SIZE(masks); ++m) {
		for (i = 0; i < info->hdr->e_shnum; ++i) {
			Elf_Shdr *s = &info->sechdrs[i];
			const char *sname = info->secstrings + s->sh_name;

			if ((s->sh_flags & masks[m][0]) != masks[m][0]
			    || (s->sh_flags & masks[m][1])
			    || s->sh_entsize != ~0UL
			    || strstarts(sname, ".init"))
				continue;
			s->sh_entsize = get_offset(mod, &mod->core_size, s, i);
			pr_debug("\t%s\n", sname);
		}
		switch (m) {
		case 0: 
			mod->core_size = debug_align(mod->core_size);
			mod->core_text_size = mod->core_size;
			break;
		case 1: 
			mod->core_size = debug_align(mod->core_size);
			mod->core_ro_size = mod->core_size;
			break;
		case 3: 
			mod->core_size = debug_align(mod->core_size);
			break;
		}
	}

	pr_debug("Init section allocation order:\n");
	for (m = 0; m < ARRAY_SIZE(masks); ++m) {
		for (i = 0; i < info->hdr->e_shnum; ++i) {
			Elf_Shdr *s = &info->sechdrs[i];
			const char *sname = info->secstrings + s->sh_name;

			if ((s->sh_flags & masks[m][0]) != masks[m][0]
			    || (s->sh_flags & masks[m][1])
			    || s->sh_entsize != ~0UL
			    || !strstarts(sname, ".init"))
				continue;
			s->sh_entsize = (get_offset(mod, &mod->init_size, s, i)
					 | INIT_OFFSET_MASK);
			pr_debug("\t%s\n", sname);
		}
		switch (m) {
		case 0: 
			mod->init_size = debug_align(mod->init_size);
			mod->init_text_size = mod->init_size;
			break;
		case 1: 
			mod->init_size = debug_align(mod->init_size);
			mod->init_ro_size = mod->init_size;
			break;
		case 3: 
			mod->init_size = debug_align(mod->init_size);
			break;
		}
	}
}

static void set_license(struct module *mod, const char *license)
{
	if (!license)
		license = "unspecified";

	if (!license_is_gpl_compatible(license)) {
		if (!test_taint(TAINT_PROPRIETARY_MODULE))
			printk(KERN_WARNING "%s: module license '%s' taints "
				"kernel.\n", mod->name, license);
		add_taint_module(mod, TAINT_PROPRIETARY_MODULE);
	}
}

static char *next_string(char *string, unsigned long *secsize)
{
	
	while (string[0]) {
		string++;
		if ((*secsize)-- <= 1)
			return NULL;
	}

	
	while (!string[0]) {
		string++;
		if ((*secsize)-- <= 1)
			return NULL;
	}
	return string;
}

static char *get_modinfo(struct load_info *info, const char *tag)
{
	char *p;
	unsigned int taglen = strlen(tag);
	Elf_Shdr *infosec = &info->sechdrs[info->index.info];
	unsigned long size = infosec->sh_size;

	for (p = (char *)infosec->sh_addr; p; p = next_string(p, &size)) {
		if (strncmp(p, tag, taglen) == 0 && p[taglen] == '=')
			return p + taglen + 1;
	}
	return NULL;
}

static void setup_modinfo(struct module *mod, struct load_info *info)
{
	struct module_attribute *attr;
	int i;

	for (i = 0; (attr = modinfo_attrs[i]); i++) {
		if (attr->setup)
			attr->setup(mod, get_modinfo(info, attr->attr.name));
	}
}

static void free_modinfo(struct module *mod)
{
	struct module_attribute *attr;
	int i;

	for (i = 0; (attr = modinfo_attrs[i]); i++) {
		if (attr->free)
			attr->free(mod);
	}
}

#ifdef CONFIG_KALLSYMS

static const struct kernel_symbol *lookup_symbol(const char *name,
	const struct kernel_symbol *start,
	const struct kernel_symbol *stop)
{
	return bsearch(name, start, stop - start,
			sizeof(struct kernel_symbol), cmp_name);
}

static int is_exported(const char *name, unsigned long value,
		       const struct module *mod)
{
	const struct kernel_symbol *ks;
	if (!mod)
		ks = lookup_symbol(name, __start___ksymtab, __stop___ksymtab);
	else
		ks = lookup_symbol(name, mod->syms, mod->syms + mod->num_syms);
	return ks != NULL && ks->value == value;
}

static char elf_type(const Elf_Sym *sym, const struct load_info *info)
{
	const Elf_Shdr *sechdrs = info->sechdrs;

	if (ELF_ST_BIND(sym->st_info) == STB_WEAK) {
		if (ELF_ST_TYPE(sym->st_info) == STT_OBJECT)
			return 'v';
		else
			return 'w';
	}
	if (sym->st_shndx == SHN_UNDEF)
		return 'U';
	if (sym->st_shndx == SHN_ABS)
		return 'a';
	if (sym->st_shndx >= SHN_LORESERVE)
		return '?';
	if (sechdrs[sym->st_shndx].sh_flags & SHF_EXECINSTR)
		return 't';
	if (sechdrs[sym->st_shndx].sh_flags & SHF_ALLOC
	    && sechdrs[sym->st_shndx].sh_type != SHT_NOBITS) {
		if (!(sechdrs[sym->st_shndx].sh_flags & SHF_WRITE))
			return 'r';
		else if (sechdrs[sym->st_shndx].sh_flags & ARCH_SHF_SMALL)
			return 'g';
		else
			return 'd';
	}
	if (sechdrs[sym->st_shndx].sh_type == SHT_NOBITS) {
		if (sechdrs[sym->st_shndx].sh_flags & ARCH_SHF_SMALL)
			return 's';
		else
			return 'b';
	}
	if (strstarts(info->secstrings + sechdrs[sym->st_shndx].sh_name,
		      ".debug")) {
		return 'n';
	}
	return '?';
}

static bool is_core_symbol(const Elf_Sym *src, const Elf_Shdr *sechdrs,
                           unsigned int shnum)
{
	const Elf_Shdr *sec;

	if (src->st_shndx == SHN_UNDEF
	    || src->st_shndx >= shnum
	    || !src->st_name)
		return false;

	sec = sechdrs + src->st_shndx;
	if (!(sec->sh_flags & SHF_ALLOC)
#ifndef CONFIG_KALLSYMS_ALL
	    || !(sec->sh_flags & SHF_EXECINSTR)
#endif
	    || (sec->sh_entsize & INIT_OFFSET_MASK))
		return false;

	return true;
}

static void layout_symtab(struct module *mod, struct load_info *info)
{
	Elf_Shdr *symsect = info->sechdrs + info->index.sym;
	Elf_Shdr *strsect = info->sechdrs + info->index.str;
	const Elf_Sym *src;
	unsigned int i, nsrc, ndst, strtab_size;

	
	symsect->sh_flags |= SHF_ALLOC;
	symsect->sh_entsize = get_offset(mod, &mod->init_size, symsect,
					 info->index.sym) | INIT_OFFSET_MASK;
	pr_debug("\t%s\n", info->secstrings + symsect->sh_name);

	src = (void *)info->hdr + symsect->sh_offset;
	nsrc = symsect->sh_size / sizeof(*src);

	
	for (ndst = i = strtab_size = 1; i < nsrc; ++i, ++src)
		if (is_core_symbol(src, info->sechdrs, info->hdr->e_shnum)) {
			strtab_size += strlen(&info->strtab[src->st_name]) + 1;
			ndst++;
		}

	
	info->symoffs = ALIGN(mod->core_size, symsect->sh_addralign ?: 1);
	info->stroffs = mod->core_size = info->symoffs + ndst * sizeof(Elf_Sym);
	mod->core_size += strtab_size;

	
	strsect->sh_flags |= SHF_ALLOC;
	strsect->sh_entsize = get_offset(mod, &mod->init_size, strsect,
					 info->index.str) | INIT_OFFSET_MASK;
	pr_debug("\t%s\n", info->secstrings + strsect->sh_name);
}

static void add_kallsyms(struct module *mod, const struct load_info *info)
{
	unsigned int i, ndst;
	const Elf_Sym *src;
	Elf_Sym *dst;
	char *s;
	Elf_Shdr *symsec = &info->sechdrs[info->index.sym];

	mod->symtab = (void *)symsec->sh_addr;
	mod->num_symtab = symsec->sh_size / sizeof(Elf_Sym);
	
	mod->strtab = (void *)info->sechdrs[info->index.str].sh_addr;

	
	for (i = 0; i < mod->num_symtab; i++)
		mod->symtab[i].st_info = elf_type(&mod->symtab[i], info);

	mod->core_symtab = dst = mod->module_core + info->symoffs;
	mod->core_strtab = s = mod->module_core + info->stroffs;
	src = mod->symtab;
	*dst = *src;
	*s++ = 0;
	for (ndst = i = 1; i < mod->num_symtab; ++i, ++src) {
		if (!is_core_symbol(src, info->sechdrs, info->hdr->e_shnum))
			continue;

		dst[ndst] = *src;
		dst[ndst++].st_name = s - mod->core_strtab;
		s += strlcpy(s, &mod->strtab[src->st_name], KSYM_NAME_LEN) + 1;
	}
	mod->core_num_syms = ndst;
}
#else
static inline void layout_symtab(struct module *mod, struct load_info *info)
{
}

static void add_kallsyms(struct module *mod, const struct load_info *info)
{
}
#endif 

static void dynamic_debug_setup(struct _ddebug *debug, unsigned int num)
{
	if (!debug)
		return;
#ifdef CONFIG_DYNAMIC_DEBUG
	if (ddebug_add_module(debug, num, debug->modname))
		printk(KERN_ERR "dynamic debug error adding module: %s\n",
					debug->modname);
#endif
}

static void dynamic_debug_remove(struct _ddebug *debug)
{
	if (debug)
		ddebug_remove_module(debug->modname);
}

void * __weak module_alloc(unsigned long size)
{
	return size == 0 ? NULL : vmalloc_exec(size);
}

static void *module_alloc_update_bounds(unsigned long size)
{
	void *ret = module_alloc(size);

	if (ret) {
		mutex_lock(&module_mutex);
		
		if ((unsigned long)ret < module_addr_min)
			module_addr_min = (unsigned long)ret;
		if ((unsigned long)ret + size > module_addr_max)
			module_addr_max = (unsigned long)ret + size;
		mutex_unlock(&module_mutex);
	}
	return ret;
}

#ifdef CONFIG_DEBUG_KMEMLEAK
static void kmemleak_load_module(const struct module *mod,
				 const struct load_info *info)
{
	unsigned int i;

	
	kmemleak_scan_area(mod, sizeof(struct module), GFP_KERNEL);

	for (i = 1; i < info->hdr->e_shnum; i++) {
		const char *name = info->secstrings + info->sechdrs[i].sh_name;
		if (!(info->sechdrs[i].sh_flags & SHF_ALLOC))
			continue;
		if (!strstarts(name, ".data") && !strstarts(name, ".bss"))
			continue;

		kmemleak_scan_area((void *)info->sechdrs[i].sh_addr,
				   info->sechdrs[i].sh_size, GFP_KERNEL);
	}
}
#else
static inline void kmemleak_load_module(const struct module *mod,
					const struct load_info *info)
{
}
#endif

static int copy_and_check(struct load_info *info,
			  const void __user *umod, unsigned long len,
			  const char __user *uargs)
{
	int err;
	Elf_Ehdr *hdr;

	if (len < sizeof(*hdr))
		return -ENOEXEC;

	
	if ((hdr = vmalloc(len)) == NULL)
		return -ENOMEM;

	if (copy_from_user(hdr, umod, len) != 0) {
		err = -EFAULT;
		goto free_hdr;
	}

	if (memcmp(hdr->e_ident, ELFMAG, SELFMAG) != 0
	    || hdr->e_type != ET_REL
	    || !elf_check_arch(hdr)
	    || hdr->e_shentsize != sizeof(Elf_Shdr)) {
		err = -ENOEXEC;
		goto free_hdr;
	}

	if (len < hdr->e_shoff + hdr->e_shnum * sizeof(Elf_Shdr)) {
		err = -ENOEXEC;
		goto free_hdr;
	}

	info->hdr = hdr;
	info->len = len;
	return 0;

free_hdr:
	vfree(hdr);
	return err;
}

static void free_copy(struct load_info *info)
{
	vfree(info->hdr);
}

static int rewrite_section_headers(struct load_info *info)
{
	unsigned int i;

	
	info->sechdrs[0].sh_addr = 0;

	for (i = 1; i < info->hdr->e_shnum; i++) {
		Elf_Shdr *shdr = &info->sechdrs[i];
		if (shdr->sh_type != SHT_NOBITS
		    && info->len < shdr->sh_offset + shdr->sh_size) {
			printk(KERN_ERR "Module len %lu truncated\n",
			       info->len);
			return -ENOEXEC;
		}

		shdr->sh_addr = (size_t)info->hdr + shdr->sh_offset;

#ifndef CONFIG_MODULE_UNLOAD
		
		if (strstarts(info->secstrings+shdr->sh_name, ".exit"))
			shdr->sh_flags &= ~(unsigned long)SHF_ALLOC;
#endif
	}

	
	info->index.vers = find_sec(info, "__versions");
	info->index.info = find_sec(info, ".modinfo");
	info->sechdrs[info->index.info].sh_flags &= ~(unsigned long)SHF_ALLOC;
	info->sechdrs[info->index.vers].sh_flags &= ~(unsigned long)SHF_ALLOC;
	return 0;
}

static struct module *setup_load_info(struct load_info *info)
{
	unsigned int i;
	int err;
	struct module *mod;

	
	info->sechdrs = (void *)info->hdr + info->hdr->e_shoff;
	info->secstrings = (void *)info->hdr
		+ info->sechdrs[info->hdr->e_shstrndx].sh_offset;

	err = rewrite_section_headers(info);
	if (err)
		return ERR_PTR(err);

	
	for (i = 1; i < info->hdr->e_shnum; i++) {
		if (info->sechdrs[i].sh_type == SHT_SYMTAB) {
			info->index.sym = i;
			info->index.str = info->sechdrs[i].sh_link;
			info->strtab = (char *)info->hdr
				+ info->sechdrs[info->index.str].sh_offset;
			break;
		}
	}

	info->index.mod = find_sec(info, ".gnu.linkonce.this_module");
	if (!info->index.mod) {
		printk(KERN_WARNING "No module found in object\n");
		return ERR_PTR(-ENOEXEC);
	}
	
	mod = (void *)info->sechdrs[info->index.mod].sh_addr;

	if (info->index.sym == 0) {
		printk(KERN_WARNING "%s: module has no symbols (stripped?)\n",
		       mod->name);
		return ERR_PTR(-ENOEXEC);
	}

	info->index.pcpu = find_pcpusec(info);

	
	if (!check_modstruct_version(info->sechdrs, info->index.vers, mod))
		return ERR_PTR(-ENOEXEC);

	return mod;
}

static int check_modinfo(struct module *mod, struct load_info *info)
{
	const char *modmagic = get_modinfo(info, "vermagic");
	int err;

	
	if (!modmagic) {
		err = try_to_force_load(mod, "bad vermagic");
		if (err)
			return err;
	} else if (!same_magic(modmagic, vermagic, info->index.vers)) {
		printk(KERN_ERR "%s: version magic '%s' should be '%s'\n",
		       mod->name, modmagic, vermagic);
		return -ENOEXEC;
	}

	if (!get_modinfo(info, "intree"))
		add_taint_module(mod, TAINT_OOT_MODULE);

	if (get_modinfo(info, "staging")) {
		add_taint_module(mod, TAINT_CRAP);
		printk(KERN_WARNING "%s: module is from the staging directory,"
		       " the quality is unknown, you have been warned.\n",
		       mod->name);
	}

	/* Set up license info based on the info section */
	set_license(mod, get_modinfo(info, "license"));

	return 0;
}

static void find_module_sections(struct module *mod, struct load_info *info)
{
	mod->kp = section_objs(info, "__param",
			       sizeof(*mod->kp), &mod->num_kp);
	mod->syms = section_objs(info, "__ksymtab",
				 sizeof(*mod->syms), &mod->num_syms);
	mod->crcs = section_addr(info, "__kcrctab");
	mod->gpl_syms = section_objs(info, "__ksymtab_gpl",
				     sizeof(*mod->gpl_syms),
				     &mod->num_gpl_syms);
	mod->gpl_crcs = section_addr(info, "__kcrctab_gpl");
	mod->gpl_future_syms = section_objs(info,
					    "__ksymtab_gpl_future",
					    sizeof(*mod->gpl_future_syms),
					    &mod->num_gpl_future_syms);
	mod->gpl_future_crcs = section_addr(info, "__kcrctab_gpl_future");

#ifdef CONFIG_UNUSED_SYMBOLS
	mod->unused_syms = section_objs(info, "__ksymtab_unused",
					sizeof(*mod->unused_syms),
					&mod->num_unused_syms);
	mod->unused_crcs = section_addr(info, "__kcrctab_unused");
	mod->unused_gpl_syms = section_objs(info, "__ksymtab_unused_gpl",
					    sizeof(*mod->unused_gpl_syms),
					    &mod->num_unused_gpl_syms);
	mod->unused_gpl_crcs = section_addr(info, "__kcrctab_unused_gpl");
#endif
#ifdef CONFIG_CONSTRUCTORS
	mod->ctors = section_objs(info, ".ctors",
				  sizeof(*mod->ctors), &mod->num_ctors);
#endif

#ifdef CONFIG_TRACEPOINTS
	mod->tracepoints_ptrs = section_objs(info, "__tracepoints_ptrs",
					     sizeof(*mod->tracepoints_ptrs),
					     &mod->num_tracepoints);
#endif
#ifdef HAVE_JUMP_LABEL
	mod->jump_entries = section_objs(info, "__jump_table",
					sizeof(*mod->jump_entries),
					&mod->num_jump_entries);
#endif
#ifdef CONFIG_EVENT_TRACING
	mod->trace_events = section_objs(info, "_ftrace_events",
					 sizeof(*mod->trace_events),
					 &mod->num_trace_events);
	kmemleak_scan_area(mod->trace_events, sizeof(*mod->trace_events) *
			   mod->num_trace_events, GFP_KERNEL);
#endif
#ifdef CONFIG_TRACING
	mod->trace_bprintk_fmt_start = section_objs(info, "__trace_printk_fmt",
					 sizeof(*mod->trace_bprintk_fmt_start),
					 &mod->num_trace_bprintk_fmt);
	kmemleak_scan_area(mod->trace_bprintk_fmt_start,
			   sizeof(*mod->trace_bprintk_fmt_start) *
			   mod->num_trace_bprintk_fmt, GFP_KERNEL);
#endif
#ifdef CONFIG_FTRACE_MCOUNT_RECORD
	
	mod->ftrace_callsites = section_objs(info, "__mcount_loc",
					     sizeof(*mod->ftrace_callsites),
					     &mod->num_ftrace_callsites);
#endif

	mod->extable = section_objs(info, "__ex_table",
				    sizeof(*mod->extable), &mod->num_exentries);

	if (section_addr(info, "__obsparm"))
		printk(KERN_WARNING "%s: Ignoring obsolete parameters\n",
		       mod->name);

	info->debug = section_objs(info, "__verbose",
				   sizeof(*info->debug), &info->num_debug);
}

static int move_module(struct module *mod, struct load_info *info)
{
	int i;
	void *ptr;

	
	ptr = module_alloc_update_bounds(mod->core_size);
	kmemleak_not_leak(ptr);
	if (!ptr)
		return -ENOMEM;

	memset(ptr, 0, mod->core_size);
	mod->module_core = ptr;

	ptr = module_alloc_update_bounds(mod->init_size);
	kmemleak_ignore(ptr);
	if (!ptr && mod->init_size) {
		module_free(mod, mod->module_core);
		return -ENOMEM;
	}
	memset(ptr, 0, mod->init_size);
	mod->module_init = ptr;

	
	pr_debug("final section addresses:\n");
	for (i = 0; i < info->hdr->e_shnum; i++) {
		void *dest;
		Elf_Shdr *shdr = &info->sechdrs[i];

		if (!(shdr->sh_flags & SHF_ALLOC))
			continue;

		if (shdr->sh_entsize & INIT_OFFSET_MASK)
			dest = mod->module_init
				+ (shdr->sh_entsize & ~INIT_OFFSET_MASK);
		else
			dest = mod->module_core + shdr->sh_entsize;

		if (shdr->sh_type != SHT_NOBITS)
			memcpy(dest, (void *)shdr->sh_addr, shdr->sh_size);
		
		shdr->sh_addr = (unsigned long)dest;
		pr_debug("\t0x%lx %s\n",
			 (long)shdr->sh_addr, info->secstrings + shdr->sh_name);
	}

	return 0;
}

static int check_module_license_and_versions(struct module *mod)
{
	/*
	 * ndiswrapper is under GPL by itself, but loads proprietary modules.
	 * Don't use add_taint_module(), as it would prevent ndiswrapper from
	 * using GPL-only symbols it needs.
	 */
	if (strcmp(mod->name, "ndiswrapper") == 0)
		add_taint(TAINT_PROPRIETARY_MODULE);

	/* driverloader was caught wrongly pretending to be under GPL */
	if (strcmp(mod->name, "driverloader") == 0)
		add_taint_module(mod, TAINT_PROPRIETARY_MODULE);

#ifdef CONFIG_MODVERSIONS
	if ((mod->num_syms && !mod->crcs)
	    || (mod->num_gpl_syms && !mod->gpl_crcs)
	    || (mod->num_gpl_future_syms && !mod->gpl_future_crcs)
#ifdef CONFIG_UNUSED_SYMBOLS
	    || (mod->num_unused_syms && !mod->unused_crcs)
	    || (mod->num_unused_gpl_syms && !mod->unused_gpl_crcs)
#endif
		) {
		return try_to_force_load(mod,
					 "no versions for exported symbols");
	}
#endif
	return 0;
}

static void flush_module_icache(const struct module *mod)
{
	mm_segment_t old_fs;

	
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (mod->module_init)
		flush_icache_range((unsigned long)mod->module_init,
				   (unsigned long)mod->module_init
				   + mod->init_size);
	flush_icache_range((unsigned long)mod->module_core,
			   (unsigned long)mod->module_core + mod->core_size);

	set_fs(old_fs);
}

int __weak module_frob_arch_sections(Elf_Ehdr *hdr,
				     Elf_Shdr *sechdrs,
				     char *secstrings,
				     struct module *mod)
{
	return 0;
}

static struct module *layout_and_allocate(struct load_info *info)
{
	
	struct module *mod;
	Elf_Shdr *pcpusec;
	int err;

	mod = setup_load_info(info);
	if (IS_ERR(mod))
		return mod;

	err = check_modinfo(mod, info);
	if (err)
		return ERR_PTR(err);

	
	err = module_frob_arch_sections(info->hdr, info->sechdrs,
					info->secstrings, mod);
	if (err < 0)
		goto out;

	pcpusec = &info->sechdrs[info->index.pcpu];
	if (pcpusec->sh_size) {
		
		err = percpu_modalloc(mod,
				      pcpusec->sh_size, pcpusec->sh_addralign);
		if (err)
			goto out;
		pcpusec->sh_flags &= ~(unsigned long)SHF_ALLOC;
	}

	layout_sections(mod, info);
	layout_symtab(mod, info);

	
	err = move_module(mod, info);
	if (err)
		goto free_percpu;

	
	mod = (void *)info->sechdrs[info->index.mod].sh_addr;
	kmemleak_load_module(mod, info);
	return mod;

free_percpu:
	percpu_modfree(mod);
out:
	return ERR_PTR(err);
}

static void module_deallocate(struct module *mod, struct load_info *info)
{
	percpu_modfree(mod);
	module_free(mod, mod->module_init);
	module_free(mod, mod->module_core);
}

int __weak module_finalize(const Elf_Ehdr *hdr,
			   const Elf_Shdr *sechdrs,
			   struct module *me)
{
	return 0;
}

static int post_relocation(struct module *mod, const struct load_info *info)
{
	
	sort_extable(mod->extable, mod->extable + mod->num_exentries);

	
	percpu_modcopy(mod, (void *)info->sechdrs[info->index.pcpu].sh_addr,
		       info->sechdrs[info->index.pcpu].sh_size);

	
	add_kallsyms(mod, info);

	
	return module_finalize(info->hdr, info->sechdrs, mod);
}

static struct module *load_module(void __user *umod,
				  unsigned long len,
				  const char __user *uargs)
{
	struct load_info info = { NULL, };
	struct module *mod;
	long err;

	pr_debug("load_module: umod=%p, len=%lu, uargs=%p\n",
	       umod, len, uargs);

	
	err = copy_and_check(&info, umod, len, uargs);
	if (err)
		return ERR_PTR(err);

	
	mod = layout_and_allocate(&info);
	if (IS_ERR(mod)) {
		err = PTR_ERR(mod);
		goto free_copy;
	}

	
	err = module_unload_init(mod);
	if (err)
		goto free_module;

	find_module_sections(mod, &info);

	err = check_module_license_and_versions(mod);
	if (err)
		goto free_unload;

	
	setup_modinfo(mod, &info);

	
	err = simplify_symbols(mod, &info);
	if (err < 0)
		goto free_modinfo;

	err = apply_relocations(mod, &info);
	if (err < 0)
		goto free_modinfo;

	err = post_relocation(mod, &info);
	if (err < 0)
		goto free_modinfo;

	flush_module_icache(mod);

	
	mod->args = strndup_user(uargs, ~0UL >> 1);
	if (IS_ERR(mod->args)) {
		err = PTR_ERR(mod->args);
		goto free_arch_cleanup;
	}

	
	mod->state = MODULE_STATE_COMING;

	mutex_lock(&module_mutex);
	if (find_module(mod->name)) {
		err = -EEXIST;
		goto unlock;
	}

	
	dynamic_debug_setup(info.debug, info.num_debug);

	
	err = verify_export_symbols(mod);
	if (err < 0)
		goto ddebug;

	module_bug_finalize(info.hdr, info.sechdrs, mod);
	list_add_rcu(&mod->list, &modules);
	mutex_unlock(&module_mutex);

	
	err = parse_args(mod->name, mod->args, mod->kp, mod->num_kp,
			 -32768, 32767, NULL);
	if (err < 0)
		goto unlink;

	
	err = mod_sysfs_setup(mod, &info, mod->kp, mod->num_kp);
	if (err < 0)
		goto unlink;

	
	free_copy(&info);

	
	trace_module_load(mod);
	return mod;

 unlink:
	mutex_lock(&module_mutex);
	
	list_del_rcu(&mod->list);
	module_bug_cleanup(mod);

 ddebug:
	dynamic_debug_remove(info.debug);
 unlock:
	mutex_unlock(&module_mutex);
	synchronize_sched();
	kfree(mod->args);
 free_arch_cleanup:
	module_arch_cleanup(mod);
 free_modinfo:
	free_modinfo(mod);
 free_unload:
	module_unload_free(mod);
 free_module:
	module_deallocate(mod, &info);
 free_copy:
	free_copy(&info);
	return ERR_PTR(err);
}

static void do_mod_ctors(struct module *mod)
{
#ifdef CONFIG_CONSTRUCTORS
	unsigned long i;

	for (i = 0; i < mod->num_ctors; i++)
		mod->ctors[i]();
#endif
}

SYSCALL_DEFINE3(init_module, void __user *, umod,
		unsigned long, len, const char __user *, uargs)
{
	struct module *mod;
	int ret = 0;

	
	if (!capable(CAP_SYS_MODULE) || modules_disabled)
		return -EPERM;

	
	mod = load_module(umod, len, uargs);
	if (IS_ERR(mod))
		return PTR_ERR(mod);

	blocking_notifier_call_chain(&module_notify_list,
			MODULE_STATE_COMING, mod);

	
	set_section_ro_nx(mod->module_core,
				mod->core_text_size,
				mod->core_ro_size,
				mod->core_size);

	
	set_section_ro_nx(mod->module_init,
				mod->init_text_size,
				mod->init_ro_size,
				mod->init_size);

	do_mod_ctors(mod);
	
	if (mod->init != NULL)
		ret = do_one_initcall(mod->init);
	if (ret < 0) {
		mod->state = MODULE_STATE_GOING;
		synchronize_sched();
		module_put(mod);
		blocking_notifier_call_chain(&module_notify_list,
					     MODULE_STATE_GOING, mod);
		free_module(mod);
		wake_up(&module_wq);
		return ret;
	}
	if (ret > 0) {
		printk(KERN_WARNING
"%s: '%s'->init suspiciously returned %d, it should follow 0/-E convention\n"
"%s: loading module anyway...\n",
		       __func__, mod->name, ret,
		       __func__);
		dump_stack();
	}

	
	mod->state = MODULE_STATE_LIVE;
	wake_up(&module_wq);
	blocking_notifier_call_chain(&module_notify_list,
				     MODULE_STATE_LIVE, mod);

	
	async_synchronize_full();

	mutex_lock(&module_mutex);
	
	module_put(mod);
	trim_init_extable(mod);
#ifdef CONFIG_KALLSYMS
	mod->num_symtab = mod->core_num_syms;
	mod->symtab = mod->core_symtab;
	mod->strtab = mod->core_strtab;
#endif
	unset_module_init_ro_nx(mod);
	module_free(mod, mod->module_init);
	mod->module_init = NULL;
	mod->init_size = 0;
	mod->init_ro_size = 0;
	mod->init_text_size = 0;
	mutex_unlock(&module_mutex);

	return 0;
}

static inline int within(unsigned long addr, void *start, unsigned long size)
{
	return ((void *)addr >= start && (void *)addr < start + size);
}

#ifdef CONFIG_KALLSYMS
static inline int is_arm_mapping_symbol(const char *str)
{
	return str[0] == '$' && strchr("atd", str[1])
	       && (str[2] == '\0' || str[2] == '.');
}

static const char *get_ksymbol(struct module *mod,
			       unsigned long addr,
			       unsigned long *size,
			       unsigned long *offset)
{
	unsigned int i, best = 0;
	unsigned long nextval;

	
	if (within_module_init(addr, mod))
		nextval = (unsigned long)mod->module_init+mod->init_text_size;
	else
		nextval = (unsigned long)mod->module_core+mod->core_text_size;

	for (i = 1; i < mod->num_symtab; i++) {
		if (mod->symtab[i].st_shndx == SHN_UNDEF)
			continue;

		if (mod->symtab[i].st_value <= addr
		    && mod->symtab[i].st_value > mod->symtab[best].st_value
		    && *(mod->strtab + mod->symtab[i].st_name) != '\0'
		    && !is_arm_mapping_symbol(mod->strtab + mod->symtab[i].st_name))
			best = i;
		if (mod->symtab[i].st_value > addr
		    && mod->symtab[i].st_value < nextval
		    && *(mod->strtab + mod->symtab[i].st_name) != '\0'
		    && !is_arm_mapping_symbol(mod->strtab + mod->symtab[i].st_name))
			nextval = mod->symtab[i].st_value;
	}

	if (!best)
		return NULL;

	if (size)
		*size = nextval - mod->symtab[best].st_value;
	if (offset)
		*offset = addr - mod->symtab[best].st_value;
	return mod->strtab + mod->symtab[best].st_name;
}

const char *module_address_lookup(unsigned long addr,
			    unsigned long *size,
			    unsigned long *offset,
			    char **modname,
			    char *namebuf)
{
	struct module *mod;
	const char *ret = NULL;

	preempt_disable();
	list_for_each_entry_rcu(mod, &modules, list) {
		if (within_module_init(addr, mod) ||
		    within_module_core(addr, mod)) {
			if (modname)
				*modname = mod->name;
			ret = get_ksymbol(mod, addr, size, offset);
			break;
		}
	}
	
	if (ret) {
		strncpy(namebuf, ret, KSYM_NAME_LEN - 1);
		ret = namebuf;
	}
	preempt_enable();
	return ret;
}

int lookup_module_symbol_name(unsigned long addr, char *symname)
{
	struct module *mod;

	preempt_disable();
	list_for_each_entry_rcu(mod, &modules, list) {
		if (within_module_init(addr, mod) ||
		    within_module_core(addr, mod)) {
			const char *sym;

			sym = get_ksymbol(mod, addr, NULL, NULL);
			if (!sym)
				goto out;
			strlcpy(symname, sym, KSYM_NAME_LEN);
			preempt_enable();
			return 0;
		}
	}
out:
	preempt_enable();
	return -ERANGE;
}

int lookup_module_symbol_attrs(unsigned long addr, unsigned long *size,
			unsigned long *offset, char *modname, char *name)
{
	struct module *mod;

	preempt_disable();
	list_for_each_entry_rcu(mod, &modules, list) {
		if (within_module_init(addr, mod) ||
		    within_module_core(addr, mod)) {
			const char *sym;

			sym = get_ksymbol(mod, addr, size, offset);
			if (!sym)
				goto out;
			if (modname)
				strlcpy(modname, mod->name, MODULE_NAME_LEN);
			if (name)
				strlcpy(name, sym, KSYM_NAME_LEN);
			preempt_enable();
			return 0;
		}
	}
out:
	preempt_enable();
	return -ERANGE;
}

int module_get_kallsym(unsigned int symnum, unsigned long *value, char *type,
			char *name, char *module_name, int *exported)
{
	struct module *mod;

	preempt_disable();
	list_for_each_entry_rcu(mod, &modules, list) {
		if (symnum < mod->num_symtab) {
			*value = mod->symtab[symnum].st_value;
			*type = mod->symtab[symnum].st_info;
			strlcpy(name, mod->strtab + mod->symtab[symnum].st_name,
				KSYM_NAME_LEN);
			strlcpy(module_name, mod->name, MODULE_NAME_LEN);
			*exported = is_exported(name, *value, mod);
			preempt_enable();
			return 0;
		}
		symnum -= mod->num_symtab;
	}
	preempt_enable();
	return -ERANGE;
}

static unsigned long mod_find_symname(struct module *mod, const char *name)
{
	unsigned int i;

	for (i = 0; i < mod->num_symtab; i++)
		if (strcmp(name, mod->strtab+mod->symtab[i].st_name) == 0 &&
		    mod->symtab[i].st_info != 'U')
			return mod->symtab[i].st_value;
	return 0;
}

unsigned long module_kallsyms_lookup_name(const char *name)
{
	struct module *mod;
	char *colon;
	unsigned long ret = 0;

	
	preempt_disable();
	if ((colon = strchr(name, ':')) != NULL) {
		*colon = '\0';
		if ((mod = find_module(name)) != NULL)
			ret = mod_find_symname(mod, colon+1);
		*colon = ':';
	} else {
		list_for_each_entry_rcu(mod, &modules, list)
			if ((ret = mod_find_symname(mod, name)) != 0)
				break;
	}
	preempt_enable();
	return ret;
}

int module_kallsyms_on_each_symbol(int (*fn)(void *, const char *,
					     struct module *, unsigned long),
				   void *data)
{
	struct module *mod;
	unsigned int i;
	int ret;

	list_for_each_entry(mod, &modules, list) {
		for (i = 0; i < mod->num_symtab; i++) {
			ret = fn(data, mod->strtab + mod->symtab[i].st_name,
				 mod, mod->symtab[i].st_value);
			if (ret != 0)
				return ret;
		}
	}
	return 0;
}
#endif 

static char *module_flags(struct module *mod, char *buf)
{
	int bx = 0;

	if (mod->taints ||
	    mod->state == MODULE_STATE_GOING ||
	    mod->state == MODULE_STATE_COMING) {
		buf[bx++] = '(';
		bx += module_flags_taint(mod, buf + bx);
		
		if (mod->state == MODULE_STATE_GOING)
			buf[bx++] = '-';
		
		if (mod->state == MODULE_STATE_COMING)
			buf[bx++] = '+';
		buf[bx++] = ')';
	}
	buf[bx] = '\0';

	return buf;
}

#ifdef CONFIG_PROC_FS
static void *m_start(struct seq_file *m, loff_t *pos)
{
	mutex_lock(&module_mutex);
	return seq_list_start(&modules, *pos);
}

static void *m_next(struct seq_file *m, void *p, loff_t *pos)
{
	return seq_list_next(p, &modules, pos);
}

static void m_stop(struct seq_file *m, void *p)
{
	mutex_unlock(&module_mutex);
}

static int m_show(struct seq_file *m, void *p)
{
	struct module *mod = list_entry(p, struct module, list);
	char buf[8];

	seq_printf(m, "%s %u",
		   mod->name, mod->init_size + mod->core_size);
	print_unload_info(m, mod);

	
	seq_printf(m, " %s",
		   mod->state == MODULE_STATE_GOING ? "Unloading":
		   mod->state == MODULE_STATE_COMING ? "Loading":
		   "Live");
	
	seq_printf(m, " 0x%pK", mod->module_core);

	
	if (mod->taints)
		seq_printf(m, " %s", module_flags(mod, buf));

	seq_printf(m, "\n");
	return 0;
}

static const struct seq_operations modules_op = {
	.start	= m_start,
	.next	= m_next,
	.stop	= m_stop,
	.show	= m_show
};

static int modules_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &modules_op);
}

static const struct file_operations proc_modules_operations = {
	.open		= modules_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int __init proc_modules_init(void)
{
	proc_create("modules", 0, NULL, &proc_modules_operations);
	return 0;
}
module_init(proc_modules_init);
#endif

const struct exception_table_entry *search_module_extables(unsigned long addr)
{
	const struct exception_table_entry *e = NULL;
	struct module *mod;

	preempt_disable();
	list_for_each_entry_rcu(mod, &modules, list) {
		if (mod->num_exentries == 0)
			continue;

		e = search_extable(mod->extable,
				   mod->extable + mod->num_exentries - 1,
				   addr);
		if (e)
			break;
	}
	preempt_enable();

	return e;
}

bool is_module_address(unsigned long addr)
{
	bool ret;

	preempt_disable();
	ret = __module_address(addr) != NULL;
	preempt_enable();

	return ret;
}

struct module *__module_address(unsigned long addr)
{
	struct module *mod;

	if (addr < module_addr_min || addr > module_addr_max)
		return NULL;

	list_for_each_entry_rcu(mod, &modules, list)
		if (within_module_core(addr, mod)
		    || within_module_init(addr, mod))
			return mod;
	return NULL;
}
EXPORT_SYMBOL_GPL(__module_address);

bool is_module_text_address(unsigned long addr)
{
	bool ret;

	preempt_disable();
	ret = __module_text_address(addr) != NULL;
	preempt_enable();

	return ret;
}

struct module *__module_text_address(unsigned long addr)
{
	struct module *mod = __module_address(addr);
	if (mod) {
		
		if (!within(addr, mod->module_init, mod->init_text_size)
		    && !within(addr, mod->module_core, mod->core_text_size))
			mod = NULL;
	}
	return mod;
}
EXPORT_SYMBOL_GPL(__module_text_address);

void print_modules(void)
{
	struct module *mod;
	char buf[8];

	printk(KERN_DEFAULT "Modules linked in:");
	
	preempt_disable();
	list_for_each_entry_rcu(mod, &modules, list)
		printk(" %s%s", mod->name, module_flags(mod, buf));
	preempt_enable();
	if (last_unloaded_module[0])
		printk(" [last unloaded: %s]", last_unloaded_module);
	printk("\n");
}

#ifdef CONFIG_MODVERSIONS
void module_layout(struct module *mod,
		   struct modversion_info *ver,
		   struct kernel_param *kp,
		   struct kernel_symbol *ks,
		   struct tracepoint * const *tp)
{
}
EXPORT_SYMBOL(module_layout);
#endif
