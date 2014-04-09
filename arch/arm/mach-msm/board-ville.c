/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <linux/i2c/isl9519.h>
#include <linux/gpio.h>
#include <linux/slimbus/slimbus.h>
#include <linux/mfd/wcd9xxx/core.h>
#include <linux/mfd/wcd9xxx/pdata.h>
#include <linux/mfd/pm8xxx/misc.h>
#include <linux/msm_ssbi.h>
#include <linux/regulator/msm-gpio-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/slimbus/slimbus.h>
#include <linux/msm_kgsl.h>
#include <linux/cyttsp-qc.h>
#include <linux/dma-mapping.h>
#include <linux/platform_data/qcom_crypto_device.h>
#include <linux/ion.h>
#include <linux/memory.h>
#include <linux/memblock.h>
#include <linux/msm_thermal.h>
#include <linux/ks8851.h>
#include <linux/proc_fs.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/hardware/gic.h>
#include <asm/mach/mmc.h>
#include <linux/platform_data/qcom_wcnss_device.h>
#include <linux/atmel_qt602240.h>
#include <linux/input/cy8c_cs.h>
#include <linux/htc_flashlight.h>
#include <mach/board.h>
#include <mach/restart.h>
#include <mach/msm_iomap.h>
#include <mach/ion.h>
#include <linux/usb/msm_hsusb.h>
#include <mach/htc_usb.h>
#include <linux/usb/android_composite.h>
#include <mach/usbdiag.h>
#include <mach/socinfo.h>
#include <mach/msm_spi.h>
#include "timer.h"
#include "devices.h"
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/rpm.h>
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif
#include <mach/msm_memtypes.h>
#include <linux/bootmem.h>
#include <asm/setup.h>
#include <mach/dma.h>
#include <mach/msm_dsps.h>
#include <mach/msm_bus_board.h>
#include <mach/cpuidle.h>
#include <mach/mdm2.h>
#include <linux/msm_tsens.h>
#include <mach/msm_xo.h>
#include <mach/msm_rtb.h>
#include <linux/fmem.h>
#include <mach/htc_headset_mgr.h>
#include <mach/htc_headset_pmic.h>
#include <mach/htc_headset_one_wire.h>
#include <linux/mfd/pm8xxx/pm8xxx-vibrator-pwm.h>
#include <mach/htc_ramdump.h>

#ifdef CONFIG_PERFLOCK
#include <mach/perflock.h>
#endif


#ifdef CONFIG_BT
#include <mach/htc_bdaddress.h>
#endif


#include <mach/msm_cache_dump.h>
#include <mach/scm.h>
#include <mach/kgsl.h>


#include <linux/akm8975.h>
#include <linux/bma250.h>
#include <linux/ewtzmu2.h>

#include <mach/msm_watchdog.h>
#include "board-ville.h"
#include "spm.h"
#include <mach/mpm.h>
#include "rpm_log.h"
#include "smd_private.h"
#include "rpm_resources.h"
#include "pm.h"
#include "pm-boot.h"
#include "board-8960.h"
#include <mach/board_htc.h>
#include <mach/htc_util.h>
#include <mach/cable_detect.h>
#include "devices-msm8x60.h"
#include <linux/cm3629.h>
#include <linux/pn544.h>
#include <mach/panel_id.h>
#include <linux/leds.h>
#include <linux/leds-pm8921.h>
#ifdef CONFIG_HTC_BATT_8960
#include "mach/htc_battery_8960.h"
#include "mach/htc_battery_cell.h"
#include "linux/mfd/pm8xxx/pm8921-charger.h"
#endif

#include "../../../../drivers/video/msm/mdp.h"

#ifdef CONFIG_FB_MSM_HDMI_MHL
#include <mach/mhl.h>
#endif

#define MSM_PMEM_ADSP_SIZE         0x8600000
#define MSM_PMEM_AUDIO_SIZE        0x4CF000
#define MSM_PMEM_SIZE              0x0 

#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
#define HOLE_SIZE		0x20000
#ifdef CONFIG_MSM_IOMMU
#define MSM_PMEM_KERNEL_EBI1_SIZE  0x280000
#else
#define MSM_PMEM_KERNEL_EBI1_SIZE  0x6400000
#endif

#define MSM_ION_KGSL_SIZE	0x0
#define MSM_ION_SF_SIZE		(MSM_PMEM_SIZE + MSM_ION_KGSL_SIZE)
#define MSM_ION_MM_FW_SIZE	(0x200000 - HOLE_SIZE) 
#define MSM_ION_MM_SIZE		MSM_PMEM_ADSP_SIZE
#define MSM_ION_QSECOM_SIZE	0x600000 
#define MSM_ION_MFC_SIZE	SZ_8K
#define MSM_ION_AUDIO_SIZE	MSM_PMEM_AUDIO_SIZE
#define MSM_ION_HEAP_NUM	8

#else
#define MSM_PMEM_KERNEL_EBI1_SIZE  0x110C000
#define MSM_ION_HEAP_NUM	1
#endif

#define APQ8064_FIXED_AREA_START (0xa0000000 - (MSM_ION_MM_FW_SIZE + HOLE_SIZE))
#define MAX_FIXED_AREA_SIZE	0x10000000
#define MSM_MM_FW_SIZE		(0x200000 - HOLE_SIZE)
#define APQ8064_FW_START	APQ8064_FIXED_AREA_START

#ifdef CONFIG_FB_MSM_HDMI_MHL
static int hdmi_enable_5v(int on);
static int hdmi_core_power(int on, int show);
extern void hdmi_hpd_feature(int enable);
#endif

unsigned skuid;
#define HW_VER_ID_VIRT	(MSM_TLMM_BASE + 0x00002054)

extern int panel_type;
static unsigned int engineerid;

static struct platform_device msm_fm_platform_init = {
	.name = "iris_fm",
	.id   = -1,
};

struct pm8xxx_gpio_init {
	unsigned			gpio;
	struct pm_gpio			config;
};

struct pm8xxx_mpp_init {
	unsigned			mpp;
	struct pm8xxx_mpp_config_data	config;
};

#define PM8XXX_GPIO_INIT(_gpio, _dir, _buf, _val, _pull, _vin, _out_strength, \
			_func, _inv, _disable) \
{ \
	.gpio	= PM8921_GPIO_PM_TO_SYS(_gpio), \
	.config	= { \
		.direction	= _dir, \
		.output_buffer	= _buf, \
		.output_value	= _val, \
		.pull		= _pull, \
		.vin_sel	= _vin, \
		.out_strength	= _out_strength, \
		.function	= _func, \
		.inv_int_pol	= _inv, \
		.disable_pin	= _disable, \
	} \
}

#define PM8XXX_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8921_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8XXX_GPIO_DISABLE(_gpio) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, 0, 0, 0, PM_GPIO_VIN_S4, \
			 0, 0, 0, 1)

#define PM8XXX_GPIO_OUTPUT(_gpio, _val) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_INPUT(_gpio, _pull) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0, \
			_pull, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_NO, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_OUTPUT_FUNC(_gpio, _val, _func) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			_func, 0, 0)

#define PM8XXX_GPIO_OUTPUT_VIN_BB_FUNC(_gpio, _val, _func) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_BB, \
			PM_GPIO_STRENGTH_HIGH, \
			_func, 0, 0)

#define PM8XXX_GPIO_OUTPUT_VIN_L17_FUNC(_gpio, _val) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_L17, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_OUTPUT_VIN_S4_FUNC_XC(_gpio, _val) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_OUTPUT_VIN_L17_LED_FUNC(_gpio, _val, _func) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_L17, \
			PM_GPIO_STRENGTH_HIGH, \
			_func, 0, 0)

static struct pm8xxx_gpio_init pm8921_gpios[] __initdata = {
	PM8XXX_GPIO_INIT(VILLE_PMGPIO_EARPHONE_DETz, PM_GPIO_DIR_IN,
			 PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_UP_1P5,
			 PM_GPIO_VIN_S4, PM_GPIO_STRENGTH_LOW,
			 PM_GPIO_FUNC_NORMAL, 0, 0),
	PM8XXX_GPIO_INIT(VILLE_PMGPIO_AUD_REMO_PRESz, PM_GPIO_DIR_IN,
			 PM_GPIO_OUT_BUF_CMOS, 0, PM_GPIO_PULL_NO,
			 PM_GPIO_VIN_L17, PM_GPIO_STRENGTH_LOW,
			 PM_GPIO_FUNC_NORMAL, 0, 0),
	PM8XXX_GPIO_OUTPUT_VIN_L17_FUNC(VILLE_PMGPIO_CAP_RST, 0),
};

static struct  pm8xxx_gpio_init pm8921_gpios_cap_rst[] __initdata = {
	PM8XXX_GPIO_OUTPUT_VIN_S4_FUNC_XC(VILLE_PMGPIO_CAP_RST, 0),
};

static struct pm8xxx_mpp_init pm8921_mpps[] __initdata = {
	
	PM8XXX_MPP_INIT(7, D_INPUT, PM8921_MPP_DIG_LEVEL_VPH, DIN_TO_INT),
	PM8XXX_MPP_INIT(PM8XXX_AMUX_MPP_3, D_BI_DIR, PM8921_MPP_DIG_LEVEL_S4, BI_PULLUP_10KOHM),
	PM8XXX_MPP_INIT(PM8XXX_AMUX_MPP_4, D_BI_DIR, PM8921_MPP_DIG_LEVEL_L17, BI_PULLUP_10KOHM),
	PM8XXX_MPP_INIT(PM8XXX_AMUX_MPP_11, D_BI_DIR, PM8921_MPP_DIG_LEVEL_S4, BI_PULLUP_10KOHM),
	PM8XXX_MPP_INIT(PM8XXX_AMUX_MPP_12, D_BI_DIR, PM8921_MPP_DIG_LEVEL_L17, BI_PULLUP_10KOHM),
	PM8XXX_MPP_INIT(PM8XXX_AMUX_MPP_8, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH8,
								DOUT_CTRL_LOW),
};

static void __init pm8921_gpio_mpp_init(void)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(pm8921_gpios); i++) {
		rc = pm8xxx_gpio_config(pm8921_gpios[i].gpio,
					&pm8921_gpios[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_gpio_config: rc=%d\n", __func__, rc);
			break;
		}
	}

	if (system_rev > 1) {	
		rc = pm8xxx_gpio_config(pm8921_gpios_cap_rst[0].gpio,
					&pm8921_gpios_cap_rst[0].config);
		if (rc)
			pr_err("%s: pm8xxx_gpio_config: rc=%d\n", __func__, rc);
	}

	for (i = 0; i < ARRAY_SIZE(pm8921_mpps); i++) {
		rc = pm8xxx_mpp_config(pm8921_mpps[i].mpp,
					&pm8921_mpps[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_mpp_config: rc=%d\n", __func__, rc);
			break;
		}
	}
}

#ifdef CONFIG_I2C

#define MSM_8960_GSBI4_QUP_I2C_BUS_ID 4
#define MSM_8960_GSBI3_QUP_I2C_BUS_ID 3
#define MSM_8960_GSBI2_QUP_I2C_BUS_ID 2
#define MSM_8960_GSBI8_QUP_I2C_BUS_ID 8
#define MSM_8960_GSBI12_QUP_I2C_BUS_ID 12
#define MSM_8960_GSBI5_QUP_I2C_BUS_ID 5

#endif

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_2_PHASE
int set_two_phase_freq(int cpufreq);
#endif

int set_input_event_min_freq_by_cpu(int cpu_nr, int cpufreq);

#ifdef CONFIG_KERNEL_PMEM_EBI_REGION
static unsigned pmem_kernel_ebi1_size = MSM_PMEM_KERNEL_EBI1_SIZE;
static int __init pmem_kernel_ebi1_size_setup(char *p)
{
	pmem_kernel_ebi1_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_kernel_ebi1_size", pmem_kernel_ebi1_size_setup);
#endif

#ifdef CONFIG_ANDROID_PMEM
static unsigned pmem_size = MSM_PMEM_SIZE;
static int __init pmem_size_setup(char *p)
{
	pmem_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_size", pmem_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;

static int __init pmem_adsp_size_setup(char *p)
{
	pmem_adsp_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_adsp_size", pmem_adsp_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;

static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);
#endif

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device m7_android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = {.platform_data = &android_pmem_pdata},
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};
static struct platform_device m7_android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

static struct android_pmem_platform_data m7_android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};
#endif 
#endif 

struct fmem_platform_data apq8064_fmem_pdata = {
};

static struct memtype_reserve apq8064_reserve_table[] __initdata = {
	[MEMTYPE_SMI] = {
	},
	[MEMTYPE_EBI0] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
	[MEMTYPE_EBI1] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
};

#if defined(CONFIG_MSM_RTB)
static struct msm_rtb_platform_data m7_rtb_pdata = {
		//TODO:FIX
		//.buffer_start_addr = MSM_RTB_PHYS,
		//.size = MSM_RTB_BUFFER_SIZE,
  		.buffer_start_addr = 0x8FD00000,
		.size = SZ_1M,
};

static int __init msm_rtb_set_buffer_size(char *p)
{
       int s;

       s = memparse(p, NULL);
       m7_rtb_pdata.size = ALIGN(s, SZ_4K);
       return 0;
}
early_param("msm_rtb_size", msm_rtb_set_buffer_size);


static struct platform_device m7_rtb_device = {
       .name           = "msm_rtb",
       .id             = -1,
       .dev            = {
               .platform_data = &m7_rtb_pdata,
       },
};
#endif

static void __init size_pmem_devices(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	android_pmem_adsp_pdata.size = pmem_adsp_size;
	android_pmem_pdata.size = pmem_size;
	android_pmem_audio_pdata.size = MSM_PMEM_AUDIO_SIZE;
#endif 
#endif 
}

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static void __init reserve_memory_for(struct android_pmem_platform_data *p)
{
	apq8064_reserve_table[p->memory_type].size += p->size;
}
#endif 
#endif 
static void __init reserve_pmem_memory(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	reserve_memory_for(&android_pmem_adsp_pdata);
	reserve_memory_for(&android_pmem_pdata);
	reserve_memory_for(&android_pmem_audio_pdata);
#endif 
	apq8064_reserve_table[MEMTYPE_EBI1].size += pmem_kernel_ebi1_size;
#endif 
}

static int m7_paddr_to_memtype(unsigned int paddr)
{
	return MEMTYPE_EBI1;
}

#define FMEM_ENABLED 0

#ifdef CONFIG_ION_MSM
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct ion_cp_heap_pdata cp_mm_m7_ion_pdata = {
	.permission_type = IPT_TYPE_MM_CARVEOUT,
	.align = PAGE_SIZE,
	.reusable = FMEM_ENABLED,
	.mem_is_fmem = FMEM_ENABLED,
	.fixed_position = FIXED_MIDDLE,
};

static struct ion_cp_heap_pdata cp_mfc_m7_ion_pdata = {
	.permission_type = IPT_TYPE_MFC_SHAREDMEM,
	.align = PAGE_SIZE,
	.reusable = 0,
	.mem_is_fmem = FMEM_ENABLED,
	.fixed_position = FIXED_HIGH,
};

static struct ion_co_heap_pdata co_m7_ion_pdata = {
	.adjacent_mem_id = INVALID_HEAP_ID,
	.align = PAGE_SIZE,
	.mem_is_fmem = 0,
};

static struct ion_co_heap_pdata fw_co_m7_ion_pdata = {
	.adjacent_mem_id = ION_CP_MM_HEAP_ID,
	.align = SZ_128K,
	.mem_is_fmem = FMEM_ENABLED,
	.fixed_position = FIXED_LOW,
};
#endif

static struct ion_platform_data ion_pdata = {
	.nr = MSM_ION_HEAP_NUM,
	.heaps = {
		{
			.id	= ION_SYSTEM_HEAP_ID,
			.type	= ION_HEAP_TYPE_SYSTEM,
			.name	= ION_VMALLOC_HEAP_NAME,
		},
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
		{
			.id	= ION_CP_MM_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_MM_HEAP_NAME,
			.size	= MSM_ION_MM_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &cp_mm_m7_ion_pdata,
		},
		{
			.id	= ION_MM_FIRMWARE_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_MM_FIRMWARE_HEAP_NAME,
			.size	= MSM_ION_MM_FW_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &fw_co_m7_ion_pdata,
		},
		{
			.id	= ION_CP_MFC_HEAP_ID,
			.type	= ION_HEAP_TYPE_CP,
			.name	= ION_MFC_HEAP_NAME,
			.size	= MSM_ION_MFC_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &cp_mfc_m7_ion_pdata,
		},
		{
			.id	= ION_SF_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_SF_HEAP_NAME,
			.size	= MSM_ION_SF_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_m7_ion_pdata,
		},
		{
			.id	= ION_IOMMU_HEAP_ID,
			.type	= ION_HEAP_TYPE_IOMMU,
			.name	= ION_IOMMU_HEAP_NAME,
		},
		{
			.id	= ION_QSECOM_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_QSECOM_HEAP_NAME,
			.size	= MSM_ION_QSECOM_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_m7_ion_pdata,
		},
		{
			.id	= ION_AUDIO_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_AUDIO_HEAP_NAME,
			.size	= MSM_ION_AUDIO_SIZE,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *) &co_m7_ion_pdata,
		},
#endif
	}
};

static struct platform_device m7_ion_dev = {
	.name = "ion-msm",
	.id = 1,
	.dev = { .platform_data = &ion_pdata },
};
#endif

static struct platform_device apq8064_fmem_device = {
	.name = "fmem",
	.id = 1,
	.dev = { .platform_data = &apq8064_fmem_pdata },
};

static void __init reserve_mem_for_ion(enum ion_memory_types mem_type,
				      unsigned long size)
{
	apq8064_reserve_table[mem_type].size += size;
}

static void __init apq8064_reserve_fixed_area(unsigned long fixed_area_size)
{
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	int ret;

	if (fixed_area_size > MAX_FIXED_AREA_SIZE)
		panic("fixed area size is larger than %dM\n",
			MAX_FIXED_AREA_SIZE >> 20);

	reserve_info->fixed_area_size = fixed_area_size;
	reserve_info->fixed_area_start = APQ8064_FW_START;

	ret = memblock_remove(reserve_info->fixed_area_start,
		reserve_info->fixed_area_size);
	BUG_ON(ret);
#endif
}

static void __init reserve_ion_memory(void)
{
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	unsigned int i;
	unsigned int reusable_count = 0;
	unsigned int fixed_size = 0;
	unsigned int fixed_low_size, fixed_middle_size, fixed_high_size;
	unsigned long fixed_low_start, fixed_middle_start, fixed_high_start;

	apq8064_fmem_pdata.size = 0;
	apq8064_fmem_pdata.reserved_size_low = 0;
	apq8064_fmem_pdata.reserved_size_high = 0;
	apq8064_fmem_pdata.align = PAGE_SIZE;
	fixed_low_size = 0;
	fixed_middle_size = 0;
	fixed_high_size = 0;

	for (i = 0; i < ion_pdata.nr; ++i) {
		const struct ion_platform_heap *heap =
			&(ion_pdata.heaps[i]);

		if (heap->type == ION_HEAP_TYPE_CP && heap->extra_data) {
			struct ion_cp_heap_pdata *data = heap->extra_data;

			reusable_count += (data->reusable) ? 1 : 0;

			if (data->reusable && reusable_count > 1) {
				pr_err("%s: Too many heaps specified as "
					"reusable. Heap %s was not configured "
					"as reusable.\n", __func__, heap->name);
				data->reusable = 0;
			}
		}
	}

	for (i = 0; i < ion_pdata.nr; ++i) {
		const struct ion_platform_heap *heap =
			&(ion_pdata.heaps[i]);

		if (heap->extra_data) {
			int fixed_position = NOT_FIXED;
			int mem_is_fmem = 0;

			switch (heap->type) {
			case ION_HEAP_TYPE_CP:
				mem_is_fmem = ((struct ion_cp_heap_pdata *)
					heap->extra_data)->mem_is_fmem;
				fixed_position = ((struct ion_cp_heap_pdata *)
					heap->extra_data)->fixed_position;
				break;
			case ION_HEAP_TYPE_CARVEOUT:
				mem_is_fmem = ((struct ion_co_heap_pdata *)
					heap->extra_data)->mem_is_fmem;
				fixed_position = ((struct ion_co_heap_pdata *)
					heap->extra_data)->fixed_position;
				break;
			default:
				break;
			}

			if (fixed_position != NOT_FIXED)
				fixed_size += heap->size;
			else
				reserve_mem_for_ion(MEMTYPE_EBI1, heap->size);

			if (fixed_position == FIXED_LOW)
				fixed_low_size += heap->size;
			else if (fixed_position == FIXED_MIDDLE)
				fixed_middle_size += heap->size;
			else if (fixed_position == FIXED_HIGH)
				fixed_high_size += heap->size;

			if (mem_is_fmem)
				apq8064_fmem_pdata.size += heap->size;
		}
	}

	if (!fixed_size)
		return;

	if (apq8064_fmem_pdata.size) {
		apq8064_fmem_pdata.reserved_size_low = fixed_low_size +
								HOLE_SIZE;
		apq8064_fmem_pdata.reserved_size_high = fixed_high_size;
	}

	fixed_size = (fixed_size + HOLE_SIZE + SECTION_SIZE - 1)
		& SECTION_MASK;
	apq8064_reserve_fixed_area(fixed_size);

	fixed_low_start = APQ8064_FIXED_AREA_START;
	fixed_middle_start = fixed_low_start + fixed_low_size + HOLE_SIZE;
	fixed_high_start = fixed_middle_start + fixed_middle_size;

	for (i = 0; i < ion_pdata.nr; ++i) {
		struct ion_platform_heap *heap = &(ion_pdata.heaps[i]);

		if (heap->extra_data) {
			int fixed_position = NOT_FIXED;
			struct ion_cp_heap_pdata *pdata = NULL;

			switch (heap->type) {
			case ION_HEAP_TYPE_CP:
				pdata =
				(struct ion_cp_heap_pdata *)heap->extra_data;
				fixed_position = pdata->fixed_position;
				break;
			case ION_HEAP_TYPE_CARVEOUT:
				fixed_position = ((struct ion_co_heap_pdata *)
					heap->extra_data)->fixed_position;
				break;
			default:
				break;
			}

			switch (fixed_position) {
			case FIXED_LOW:
				heap->base = fixed_low_start;
				break;
			case FIXED_MIDDLE:
				heap->base = fixed_middle_start;
				pdata->secure_base = fixed_middle_start
								- HOLE_SIZE;
				pdata->secure_size = HOLE_SIZE + heap->size;
				break;
			case FIXED_HIGH:
				heap->base = fixed_high_start;
				break;
			default:
				break;
			}
		}
	}
#endif
}

static void __init reserve_mdp_memory(void)
{
	msm8960_mdp_writeback(apq8064_reserve_table);
}

static void __init m7_calculate_reserve_sizes(void)
{
	size_pmem_devices();
	reserve_pmem_memory();
	reserve_ion_memory();
	reserve_mdp_memory();
}

static struct reserve_info m7_reserve_info __initdata = {
	.memtype_reserve_table = apq8064_reserve_table,
	.calculate_reserve_sizes = m7_calculate_reserve_sizes,
	.reserve_fixed_area = apq8064_reserve_fixed_area,
	.paddr_to_memtype = m7_paddr_to_memtype,
};

static int m7_memory_bank_size(void)
{
	return 1<<29;
}

static void __init locate_unstable_memory(void)
{
	struct membank *mb = &meminfo.bank[meminfo.nr_banks - 1];
	unsigned long bank_size;
	unsigned long low, high;

	bank_size = m7_memory_bank_size();
	low = meminfo.bank[0].start;
	high = mb->start + mb->size;

	
	if (high < mb->start)
		high = -PAGE_SIZE;

	low &= ~(bank_size - 1);

	if (high - low <= bank_size)
		goto no_dmm;

#ifdef CONFIG_ENABLE_DMM
	m7_reserve_info.low_unstable_address = mb->start -
					MIN_MEMORY_BLOCK_SIZE + mb->size;
	m7_reserve_info.max_unstable_size = MIN_MEMORY_BLOCK_SIZE;

	m7_reserve_info.bank_size = bank_size;
	pr_info("low unstable address %lx max size %lx bank size %lx\n",
		m7_reserve_info.low_unstable_address,
		m7_reserve_info.max_unstable_size,
		m7_reserve_info.bank_size);
	return;
#endif
no_dmm:
	m7_reserve_info.low_unstable_address = high;
	m7_reserve_info.max_unstable_size = 0;
}

int __init parse_tag_memsize(const struct tag *tags);
static unsigned int mem_size_mb;

static void __init m7_reserve(void)
{
	if (mem_size_mb == 64)
		return;
	
	msm_reserve();
	if (apq8064_fmem_pdata.size) {
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
		if (reserve_info->fixed_area_size) {
			apq8064_fmem_pdata.phys =
				reserve_info->fixed_area_start + MSM_MM_FW_SIZE;
			pr_info("mm fw at %lx (fixed) size %x\n",
				reserve_info->fixed_area_start, MSM_MM_FW_SIZE);
			pr_info("fmem start %lx (fixed) size %lx\n",
				apq8064_fmem_pdata.phys,
				apq8064_fmem_pdata.size);
		}
#endif
	}
}

static void __init place_movable_zone(void)
{
#ifdef CONFIG_ENABLE_DMM
	movable_reserved_start = monarudo_reserve_info.low_unstable_address;
	movable_reserved_size = monarudo_reserve_info.max_unstable_size;
	pr_info("movable zone start %lx size %lx\n",
		movable_reserved_start, movable_reserved_size);
#endif
}

static void __init m7_early_reserve(void)
{
	reserve_info = &m7_reserve_info;
	locate_unstable_memory();
	place_movable_zone();
}

static int msm8960_change_memory_power(u64 start, u64 size,
	int change_type)
{
	return soc_change_memory_power(start, size, change_type);
}

#ifdef CONFIG_RAWCHIP
static struct spi_board_info rawchip_spi_board_info[] __initdata = {
	{
		.modalias               = "spi_rawchip",
		.max_speed_hz           = 27000000,
		.bus_num                = 1,
		.chip_select            = 0,
		.mode                   = SPI_MODE_0,
	},
};
#endif

#ifdef CONFIG_HTC_BATT_8960
#ifdef CONFIG_HTC_PNPMGR
extern int pnpmgr_battery_charging_enabled(int charging_enabled);
#endif 
static int critical_alarm_voltage_mv[] = {3000, 3100, 3200, 3400};

static struct htc_battery_platform_data htc_battery_pdev_data = {
	.guage_driver = 0,
	.chg_limit_active_mask = HTC_BATT_CHG_LIMIT_BIT_TALK |
								HTC_BATT_CHG_LIMIT_BIT_NAVI |
								HTC_BATT_CHG_LIMIT_BIT_THRML,
#ifdef CONFIG_DUTY_CYCLE_LIMIT
	.chg_limit_timer_sub_mask = HTC_BATT_CHG_LIMIT_BIT_THRML,
#endif
	.critical_low_voltage_mv = 3200,
	.critical_alarm_vol_ptr = critical_alarm_voltage_mv,
	.critical_alarm_vol_cols = sizeof(critical_alarm_voltage_mv) / sizeof(int),
	.overload_vol_thr_mv = 4000,
	.overload_curr_thr_ma = 0,
	.smooth_chg_full_delay_min = 1,
	
	.icharger.name = "pm8921",
	.icharger.set_limit_charge_enable = pm8921_limit_charge_enable,
	.icharger.get_attr_text = pm8921_charger_get_attr_text,
	.icharger.max_input_current = pm8921_set_hsml_target_ma,
	.icharger.enable_5v_output = NULL,
	.icharger.get_charging_source = pm8921_get_charging_source,
	.icharger.get_charging_enabled = pm8921_get_charging_enabled,
	.icharger.set_charger_enable = pm8921_charger_enable,
	.icharger.set_pwrsrc_enable = pm8921_pwrsrc_enable,
	.icharger.set_pwrsrc_and_charger_enable =
						pm8921_set_pwrsrc_and_charger_enable,
	.icharger.is_ovp = pm8921_is_charger_ovp,
	.icharger.is_batt_temp_fault_disable_chg =
						pm8921_is_batt_temp_fault_disable_chg,
	.icharger.charger_change_notifier_register =
						cable_detect_register_notifier,
	.icharger.dump_all = pm8921_dump_all,
	.icharger.is_safty_timer_timeout = pm8921_is_chg_safety_timer_timeout,
	.icharger.is_battery_full_eoc_stop = pm8921_is_batt_full_eoc_stop,

	
	.igauge.name = "pm8921",
	.igauge.get_battery_voltage = pm8921_get_batt_voltage,
	.igauge.get_battery_current = pm8921_bms_get_batt_current,
	.igauge.get_battery_temperature = pm8921_get_batt_temperature,
	.igauge.get_battery_id = pm8921_get_batt_id,
	.igauge.get_battery_soc = pm8921_bms_get_batt_soc,
	.igauge.get_battery_cc = pm8921_bms_get_batt_cc,
	.igauge.is_battery_temp_fault = pm8921_is_batt_temperature_fault,
	.igauge.is_battery_full = pm8921_is_batt_full,
	.igauge.get_attr_text = pm8921_gauge_get_attr_text,
	.igauge.register_lower_voltage_alarm_notifier =
						pm8xxx_batt_lower_alarm_register_notifier,
	.igauge.enable_lower_voltage_alarm = pm8xxx_batt_lower_alarm_enable,
	.igauge.set_lower_voltage_alarm_threshold =
						pm8xxx_batt_lower_alarm_threshold_set,
	
#ifdef CONFIG_THERMAL_TSENS8960
	.get_thermal_sensor_temp = tsens_get_sensor_temp,
#endif
	
#ifdef CONFIG_HTC_PNPMGR
	.notify_pnpmgr_charging_enabled = pnpmgr_battery_charging_enabled,
#endif 
};
static struct platform_device htc_battery_pdev = {
	.name = "htc_battery",
	.id = -1,
	.dev    = {
		.platform_data = &htc_battery_pdev_data,
	},
};

static struct pm8921_charger_batt_param chg_batt_params[] = {
	
	[0] = {
		.max_voltage = 4200,
		.cool_bat_voltage = 4200,
		.warm_bat_voltage = 4000,
	},
	
	[1] = {
		.max_voltage = 4340,
		.cool_bat_voltage = 4340,
		.warm_bat_voltage = 4000,
	},
	
	[2] = {
		.max_voltage = 4300,
		.cool_bat_voltage = 4300,
		.warm_bat_voltage = 4000,
	},
	
	[3] = {
		.max_voltage = 4350,
		.cool_bat_voltage = 4350,
		.warm_bat_voltage = 4000,
	},
};

static struct single_row_lut fcc_temp_id_1 = {
	.x		= {-20, 0, 25, 40, 65},
	.y		= {1620, 1630, 1640, 1641, 1640},
	.cols	= 5
};

static struct single_row_lut fcc_sf_id_1 = {
	.x		= {0},
	.y		= {100},
	.cols	= 1
};

static struct sf_lut pc_sf_id_1 = {
	.rows		= 10,
	.cols		= 5,
	
	.row_entries	= {100, 200, 300, 400, 500},
	.percent	= {100, 90, 80, 70, 60, 50, 40, 30, 20, 10},
	.sf		= {
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100}
	},
};

static struct pc_temp_ocv_lut  pc_temp_ocv_id_1 = {
	.rows		= 29,
	.cols		= 5,
	.temp		= {-20, 0, 25, 40, 65},
	.percent	= {100, 95, 90, 85, 80, 75, 70, 65, 60, 55,
				50, 45, 40, 35, 30, 25, 20, 15, 10, 9,
				8, 7, 6, 5, 4, 3, 2, 1, 0},
	.ocv		= {
				{4157, 4174, 4172, 4169, 4165},
				{4104, 4131, 4133, 4131, 4125},
				{4041, 4086, 4093, 4092, 4087},
				{3985, 4050, 4059, 4058, 4054},
				{3939, 4002, 4016, 4015, 4010},
				{3899, 3962, 3980, 3979, 3974},
				{3869, 3925, 3948, 3947, 3942},
				{3845, 3892, 3919, 3919, 3914},
				{3825, 3862, 3884, 3886, 3883},
				{3809, 3839, 3852, 3855, 3854},
				{3792, 3818, 3826, 3827, 3827},
				{3776, 3797, 3803, 3803, 3802},
				{3762, 3786, 3792, 3792, 3791},
				{3747, 3754, 3770, 3767, 3764},
				{3730, 3735, 3762, 3758, 3741},
				{3713, 3722, 3750, 3744, 3723},
				{3692, 3715, 3726, 3720, 3704},
				{3666, 3701, 3689, 3683, 3670},
				{3626, 3671, 3664, 3659, 3646},
				{3620, 3667, 3661, 3658, 3645},
				{3612, 3663, 3659, 3655, 3642},
				{3604, 3659, 3657, 3653, 3640},
				{3595, 3651, 3653, 3648, 3633},
				{3582, 3637, 3638, 3632, 3614},
				{3566, 3616, 3611, 3602, 3587},
				{3544, 3584, 3575, 3567, 3551},
				{3514, 3538, 3527, 3521, 3508},
				{3466, 3477, 3468, 3466, 3453},
				{3357, 3370, 3369, 3367, 3362}
	}
};

struct pm8921_bms_battery_data  bms_battery_data_id_1 = {
	.fcc			= 1650,
	.fcc_temp_lut		= &fcc_temp_id_1,
	.fcc_sf_lut		= &fcc_sf_id_1,
	.pc_temp_ocv_lut	= &pc_temp_ocv_id_1,
	.pc_sf_lut		= &pc_sf_id_1,
};

static struct single_row_lut fcc_temp_id_2 = {
	.x		= {-20, 0, 25, 40, 65},
	.y		= {1631, 1642, 1643, 1641, 1642},
	.cols	= 5
};

static struct single_row_lut fcc_sf_id_2 = {
	.x		= {0},
	.y		= {100},
	.cols	= 1
};

static struct sf_lut pc_sf_id_2 = {
	.rows		= 10,
	.cols		= 5,
	
	.row_entries	= {100, 200, 300, 400, 500},
	.percent	= {100, 90, 80, 70, 60, 50, 40, 30, 20, 10},
	.sf		= {
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100},
			{100, 100, 100, 100, 100}
	},
};

static struct pc_temp_ocv_lut  pc_temp_ocv_id_2 = {
	.rows		= 29,
	.cols		= 5,
	.temp		= {-20, 0, 25, 40, 65},
	.percent	= {100, 95, 90, 85, 80, 75, 70, 65, 60, 55,
				50, 45, 40, 35, 30, 25, 20, 15, 10, 9,
				8, 7, 6, 5, 4, 3, 2, 1, 0},
	.ocv		= {
				{4156 ,4154 ,4151 ,4147 ,4150},
				{4112 ,4123 ,4123 ,4120 ,4125},
				{4062 ,4074 ,4079 ,4076 ,4081},
				{4007 ,4027 ,4038 ,4035 ,4039},
				{3956 ,3981 ,4000 ,3998 ,4001},
				{3921 ,3945 ,3966 ,3964 ,3965},
				{3887 ,3913 ,3936 ,3935 ,3934},
				{3855 ,3883 ,3905 ,3907 ,3906},
				{3828 ,3857 ,3863 ,3876 ,3877},
				{3807 ,3833 ,3837 ,3836 ,3841},
				{3789 ,3813 ,3816 ,3815 ,3814},
				{3775 ,3792 ,3801 ,3799 ,3799},
				{3764 ,3774 ,3788 ,3785 ,3785},
				{3752 ,3762 ,3778 ,3775 ,3774},
				{3739 ,3750 ,3770 ,3767 ,3760},
				{3724 ,3739 ,3762 ,3757 ,3741},
				{3706 ,3727 ,3742 ,3735 ,3720},
				{3681 ,3714 ,3711 ,3703 ,3687},
				{3641 ,3685 ,3676 ,3668 ,3654},
				{3635 ,3681 ,3675 ,3667 ,3653},
				{3628 ,3677 ,3673 ,3666 ,3651},
				{3620 ,3672 ,3672 ,3665 ,3650},
				{3610 ,3666 ,3670 ,3662 ,3646},
				{3599 ,3656 ,3662 ,3653 ,3633},
				{3585 ,3639 ,3641 ,3632 ,3608},
				{3566 ,3612 ,3608 ,3599 ,3574},
				{3537 ,3571 ,3564 ,3556 ,3530},
				{3487 ,3506 ,3502 ,3494 ,3468},
				{3360 ,3371 ,3371 ,3368 ,3366}
	}
};

struct pm8921_bms_battery_data  bms_battery_data_id_2 = {
	.fcc			= 1650,
	.fcc_temp_lut		= &fcc_temp_id_2,
	.fcc_sf_lut		= &fcc_sf_id_2,
	.pc_temp_ocv_lut	= &pc_temp_ocv_id_2,
	.pc_sf_lut		= &pc_sf_id_2,
};

static struct htc_battery_cell htc_battery_cells[] = {
	[0] = {
		.model_name = "BJ40100",
		.capacity = 1650,
		.id = 1,
		.id_raw_min = 0, 
		.id_raw_max = 330,
		.type = HTC_BATTERY_CELL_TYPE_NORMAL,
		.voltage_max = 4200,
		.voltage_min = 3200,
		.chg_param = &chg_batt_params[0],
		.gauge_param = &bms_battery_data_id_1,
	},
	[1] = {
		.model_name = "BJ40100",
		.capacity = 1650,
		.id = 2,
		.id_raw_min = 331, 
		.id_raw_max = 540,
		.type = HTC_BATTERY_CELL_TYPE_NORMAL,
		.voltage_max = 4200,
		.voltage_min = 3200,
		.chg_param = &chg_batt_params[0],
		.gauge_param = &bms_battery_data_id_2,
	},
	[2] = {
		.model_name = "UNKNOWN",
		.capacity = 1650,
		.id = 255,
		.id_raw_min = INT_MIN,
		.id_raw_max = INT_MAX,
		.type = HTC_BATTERY_CELL_TYPE_NORMAL,
		.voltage_max = 4200,
		.voltage_min = 3200,
		.chg_param = &chg_batt_params[0],
		.gauge_param = NULL,
	},
};
#endif 

static uint32_t headset_gpio[] = {
	GPIO_CFG(VILLE_GPIO_AUD_1WIRE_DO, 1, GPIO_CFG_OUTPUT,
		 GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(VILLE_GPIO_AUD_1WIRE_DI, 1, GPIO_CFG_INPUT,
		 GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(VILLE_GPIO_AUD_LEVEL_SHIFTER_ENz, 0, GPIO_CFG_OUTPUT,
		 GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t headset_cpu_gpio[] = {
 	GPIO_CFG(VILLE_GPIO_AUD_1WIRE_DI, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(VILLE_GPIO_AUD_1WIRE_DO, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(VILLE_GPIO_AUD_1WIRE_DI, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(VILLE_GPIO_AUD_1WIRE_DO, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static void headset_init(void)
{

	pr_info("[HS_BOARD] (%s) Headset initiation\n", __func__);

	gpio_tlmm_config(headset_gpio[0], GPIO_CFG_ENABLE);
	gpio_tlmm_config(headset_gpio[1], GPIO_CFG_ENABLE);
	gpio_tlmm_config(headset_gpio[2], GPIO_CFG_ENABLE);
	gpio_set_value(VILLE_GPIO_AUD_LEVEL_SHIFTER_ENz, 1);
}

static void uart_tx_gpo(int mode)
{
	switch (mode) {
		case 0:
			gpio_tlmm_config(headset_cpu_gpio[1], GPIO_CFG_ENABLE);
			gpio_set_value_cansleep(VILLE_GPIO_AUD_1WIRE_DO, 0);
			break;
		case 1:
			gpio_tlmm_config(headset_cpu_gpio[1], GPIO_CFG_ENABLE);
			gpio_set_value_cansleep(VILLE_GPIO_AUD_1WIRE_DO, 1);
			break;
		case 2:
			gpio_tlmm_config(headset_cpu_gpio[3], GPIO_CFG_ENABLE);
			break;
	}
}

static void uart_lv_shift_en(int enable)
{
	gpio_set_value_cansleep(VILLE_GPIO_AUD_LEVEL_SHIFTER_ENz, enable);
}


static struct htc_headset_pmic_platform_data htc_headset_pmic_data = {
	.driver_flag		= DRIVER_HS_PMIC_ADC,
	.hpin_gpio		= PM8921_GPIO_PM_TO_SYS(
					VILLE_PMGPIO_EARPHONE_DETz),
	.hpin_irq		= 0,
	.key_gpio		= PM8921_GPIO_PM_TO_SYS(
					VILLE_PMGPIO_AUD_REMO_PRESz),
	.key_irq		= 0,
	.key_enable_gpio	= 0,
	.adc_mic		= 0,
	.adc_remote		= {0, 57, 58, 147, 148, 339},
	.adc_mpp		= PM8XXX_AMUX_MPP_10,
	.adc_amux		= ADC_MPP_1_AMUX6,
	.hs_controller		= 0,
	.hs_switch		= 0,
};

static struct htc_headset_pmic_platform_data htc_headset_pmic_data_xc = {
	.driver_flag		= DRIVER_HS_PMIC_ADC,
	.hpin_gpio		= PM8921_GPIO_PM_TO_SYS(
					VILLE_PMGPIO_EARPHONE_DETz),
	.hpin_irq		= 0,
	.key_gpio		= PM8921_GPIO_PM_TO_SYS(
					VILLE_PMGPIO_AUD_REMO_PRESz),
	.key_irq		= 0,
	.key_enable_gpio	= 0,
	.adc_mic		= 0,
	.adc_remote		= {0, 149, 150, 349, 350, 630},
	.adc_mpp		= PM8XXX_AMUX_MPP_10,
	.adc_amux		= ADC_MPP_1_AMUX6,
	.hs_controller		= 0,
	.hs_switch		= 0,
};

static struct platform_device htc_headset_pmic = {
	.name	= "HTC_HEADSET_PMIC",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_pmic_data,
	},
};

static struct htc_headset_1wire_platform_data htc_headset_1wire_data = {
	.tx_level_shift_en	= 91,
	.uart_sw		= 0,
	.one_wire_remote	={0x7E, 0x7F, 0x7D, 0x7F, 0x7B, 0x7F},
	.remote_press		= 0,
	.onewire_tty_dev	= "/dev/ttyHSL1",
};

static struct platform_device htc_headset_one_wire = {
	.name	= "HTC_HEADSET_1WIRE",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_1wire_data,
	},
};

static struct platform_device *headset_devices[] = {
	&htc_headset_pmic,
	&htc_headset_one_wire,
	
};

static struct headset_adc_config htc_headset_mgr_config[] = {
	{
		.type = HEADSET_MIC,
		.adc_max = 1530,
		.adc_min = 1223,
	},
	{
		.type = HEADSET_BEATS,
		.adc_max = 1222,
		.adc_min = 916,
	},
	{
		.type = HEADSET_BEATS_SOLO,
		.adc_max = 915,
		.adc_min = 566,
	},
	{
		.type = HEADSET_METRICO, 
		.adc_max = 565,
		.adc_min = 255,
	},
	{
		.type = HEADSET_NO_MIC,
		.adc_max = 254,
		.adc_min = 0,
	},
};

static struct headset_adc_config htc_headset_mgr_config_xc[] = {
	{
		.type = HEADSET_MIC,
		.adc_max = 2700,
		.adc_min = 1201,
	},
	{
		.type = HEADSET_METRICO,
		.adc_max = 1200,
		.adc_min = 501,
	},
	{
		.type = HEADSET_NO_MIC,
		.adc_max = 500,
		.adc_min = 0,
	},
};

static struct htc_headset_mgr_platform_data htc_headset_mgr_data = {
	.driver_flag		= DRIVER_HS_MGR_FLOAT_DET,
	.headset_devices_num	= ARRAY_SIZE(headset_devices),
	.headset_devices	= headset_devices,
	.headset_config_num	= ARRAY_SIZE(htc_headset_mgr_config),
	.headset_config		= htc_headset_mgr_config,
	.headset_init		= headset_init,
	.uart_tx_gpo		= uart_tx_gpo,
	.uart_lv_shift_en	= uart_lv_shift_en,
};

static struct platform_device htc_headset_mgr = {
	.name	= "HTC_HEADSET_MGR",
	.id	= -1,
	.dev	= {
		.platform_data	= &htc_headset_mgr_data,
	},
};

static void headset_device_register(void)
{
	pr_info("[HS_BOARD] (%s) Headset device register\n", __func__);

	pr_info("[HS_BOARD] (%s) system_rev = %d\n", __func__, system_rev);
	if (system_rev == 2) {
		htc_headset_pmic.dev.platform_data = &htc_headset_pmic_data_xc;
		htc_headset_mgr_data.headset_config_num =
					ARRAY_SIZE(htc_headset_mgr_config_xc);
		htc_headset_mgr_data.headset_config = htc_headset_mgr_config_xc;
	}

	platform_device_register(&htc_headset_mgr);
}

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static struct resource hdmi_msm_resources[] = {
	{
		.name  = "hdmi_msm_qfprom_addr",
		.start = 0x00700000,
		.end   = 0x007060FF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_hdmi_addr",
		.start = 0x04A00000,
		.end   = 0x04A00FFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_irq",
		.start = HDMI_IRQ,
		.end   = HDMI_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static mhl_driving_params ville_driving_params[] = {
	{.format = HDMI_VFRMT_640x480p60_4_3,	.reg_a3=0xEC, .reg_a6=0x0C},
	{.format = HDMI_VFRMT_720x480p60_16_9,	.reg_a3=0xEC, .reg_a6=0x0C},
	{.format = HDMI_VFRMT_1280x720p60_16_9,	.reg_a3=0xEC, .reg_a6=0x0C},
	{.format = HDMI_VFRMT_720x576p50_16_9,	.reg_a3=0xEC, .reg_a6=0x0C},
	{.format = HDMI_VFRMT_1920x1080p24_16_9, .reg_a3=0xEC, .reg_a6=0x0C},
	{.format = HDMI_VFRMT_1920x1080p30_16_9, .reg_a3=0xEC, .reg_a6=0x0C},
};

static struct msm_hdmi_platform_data hdmi_msm_data = {

	.irq = HDMI_IRQ,
	.enable_5v = hdmi_enable_5v,
	.core_power = hdmi_core_power,
	
	.driving_params =  ville_driving_params,
	.dirving_params_count = ARRAY_SIZE(ville_driving_params),
};

static struct platform_device hdmi_msm_device = {
	.name = "hdmi_msm",
	.id = 0,
	.num_resources = ARRAY_SIZE(hdmi_msm_resources),
	.resource = hdmi_msm_resources,
	.dev.platform_data = &hdmi_msm_data,
};
#endif 

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static int hdmi_enable_5v(int on)
{
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (on) {
		rc = gpio_request(VILLE_GPIO_V_BOOST_5V_EN, "HDMI_BOOST_5V");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_BOOST_5V", VILLE_GPIO_V_BOOST_5V_EN, rc);
			goto error;
		}
		gpio_set_value(VILLE_GPIO_V_BOOST_5V_EN, 1);
		pr_info("%s(on): success\n", __func__);
	} else {
		gpio_set_value(VILLE_GPIO_V_BOOST_5V_EN, 0);
		gpio_free(VILLE_GPIO_V_BOOST_5V_EN);
		pr_info("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
error:
	return rc;
}

static int hdmi_core_power(int on, int show)
{
	static struct regulator *reg_8921_l23;
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (!reg_8921_l23) {
		reg_8921_l23 = regulator_get(&hdmi_msm_device.dev, "hdmi_avdd");
		if (IS_ERR(reg_8921_l23)) {
			pr_err("could not get reg_8921_l23, rc = %ld\n",
				PTR_ERR(reg_8921_l23));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_l23, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_l23, rc=%d\n", rc);
			return -EINVAL;
		}
	}
	if (on) {
		rc = regulator_set_optimum_mode(reg_8921_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_8921_l23);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_avdd", rc);
			return rc;
		}

		pr_info("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_8921_l23);
		if (rc) {
			pr_err("disable reg_8921_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}

		rc = regulator_set_optimum_mode(reg_8921_l23, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		pr_info("%s(off): success\n", __func__);
	}
	prev_on = on;
	return rc;
}
#endif 


#define _GET_REGULATOR(var, name) do {				\
	var = regulator_get(NULL, name);			\
	if (IS_ERR(var)) {					\
		pr_err("'%s' regulator not found, rc=%ld\n",	\
			name, IS_ERR(var));			\
		var = NULL;					\
		return -ENODEV;					\
	}							\
} while (0)

static uint32_t mhl_usb_switch_ouput_table[] = {
	GPIO_CFG(VILLE_GPIO_MHL_USB_SELz, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_4MA),
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

#ifdef CONFIG_FB_MSM_HDMI_MHL
static void ville_usb_dpdn_switch(int path)
{
	switch (path) {
	case PATH_USB:
	case PATH_MHL:
	{
		int polarity = 1; 
		int mhl = (path == PATH_MHL);

		config_gpio_table(mhl_usb_switch_ouput_table,
				ARRAY_SIZE(mhl_usb_switch_ouput_table));

		pr_info("[CABLE] %s: Set %s path\n", __func__, mhl ? "MHL" : "USB");
		gpio_set_value(VILLE_GPIO_MHL_USB_SELz, (mhl ^ !polarity) ? 1 : 0);
		break;
	}
	}

	#ifdef CONFIG_FB_MSM_HDMI_MHL
	sii9234_change_usb_owner((path == PATH_MHL) ? 1 : 0);
	#endif 
}
#endif

#ifdef CONFIG_FB_MSM_HDMI_MHL
static struct regulator *reg_8921_l12;
static struct regulator *reg_8921_s4;
static struct regulator *reg_8921_l16;
static struct regulator *reg_8921_l10;
static struct regulator *reg_8921_s2;
uint32_t msm_hdmi_off_gpio[] = {
	GPIO_CFG(VILLE_GPIO_HDMI_DDC_CLK,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(VILLE_GPIO_HDMI_DDC_DATA,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(VILLE_GPIO_HDMI_HPD,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};
uint32_t msm_hdmi_on_gpio[] = {
	GPIO_CFG(VILLE_GPIO_HDMI_DDC_CLK,  1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(VILLE_GPIO_HDMI_DDC_DATA,  1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(VILLE_GPIO_HDMI_HPD,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

static void mhl_sii9234_1v2_power(bool enable)
{
	static bool prev_on = false;

	if (enable == prev_on)
		return;

	if (enable) {
		config_gpio_table(msm_hdmi_on_gpio, ARRAY_SIZE(msm_hdmi_on_gpio));
		hdmi_hpd_feature(1);
		pr_info("%s(on): success\n", __func__);
	} else {
		config_gpio_table(msm_hdmi_off_gpio, ARRAY_SIZE(msm_hdmi_off_gpio));
		hdmi_hpd_feature(0);
		pr_info("%s(off): success\n", __func__);
	}

	prev_on = enable;
}

static int mhl_sii9234_all_power(bool enable)
{
	static bool prev_on = false;
	int rc;
	if (enable == prev_on)
		return 0;

	if (!reg_8921_s4)
		_GET_REGULATOR(reg_8921_s4, "8921_s4");
	if (!reg_8921_l16)
		_GET_REGULATOR(reg_8921_l16, "8921_l16");
	if (!reg_8921_l12)
		_GET_REGULATOR(reg_8921_l12, "8921_l12");

	if (enable) {
		rc = regulator_set_voltage(reg_8921_s4, 1800000, 1800000);
		if (rc) {
			pr_err("%s: regulator_set_voltage reg_8921_s4 failed rc=%d\n",
				__func__, rc);
			return rc;
		}
		rc = regulator_set_voltage(reg_8921_l16, 3300000, 3300000);
		if (rc) {
			pr_err("%s: regulator_set_voltage reg_8921_l16 failed rc=%d\n",
				__func__, rc);
			return rc;
		}
		rc = regulator_set_voltage(reg_8921_l12, 1200000, 1200000);
		if (rc) {
			pr_err("%s: regulator_set_voltage reg_8921_l12 failed rc=%d\n",
				__func__, rc);
			return rc;
		}
		rc = regulator_enable(reg_8921_s4);

		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"reg_8921_s4", rc);
			return rc;
		}
		rc = regulator_enable(reg_8921_l16);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"reg_8921_l16", rc);
			return rc;
		}
		rc = regulator_enable(reg_8921_l12);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"reg_8921_l12", rc);
			return rc;
		}
		pr_info("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_8921_s4);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"reg_8921_s4", rc);
		rc = regulator_disable(reg_8921_l16);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"reg_8921_l16", rc);
		rc = regulator_disable(reg_8921_l12);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"reg_8921_l12", rc);
		pr_info("%s(off): success\n", __func__);
	}

	prev_on = enable;

	return 0;
}

#ifdef CONFIG_FB_MSM_HDMI_MHL_SII9234
static uint32_t mhl_gpio_table[] = {
	GPIO_CFG(VILLE_GPIO_MHL_RSTz, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(VILLE_GPIO_MHL_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

static int mhl_sii9234_power_vote(bool enable)
{
	int rc;

	if (!reg_8921_l10) {
		_GET_REGULATOR(reg_8921_l10, "8921_l10");
		rc = regulator_set_voltage(reg_8921_l10, 3000000, 3000000);
                if (rc) {
                        pr_err("%s: regulator_set_voltage reg_8921_l10 failed rc=%d\n",
                                __func__, rc);
                        return rc;
                }
	}
	if (!reg_8921_s2) {
		_GET_REGULATOR(reg_8921_s2, "8921_s2");
                rc = regulator_set_voltage(reg_8921_s2, 1300000, 1300000);
                if (rc) {
                        pr_err("%s: regulator_set_voltage reg_8921_s2 failed rc=%d\n",
                                __func__, rc);
                        return rc;
                }

	}

	if (enable) {
                if (reg_8921_l10) {
                        rc = regulator_enable(reg_8921_l10);
                        if (rc)
                                pr_warning("'%s' regulator enable failed, rc=%d\n",
                                        "reg_8921_l10", rc);
                }
                if (reg_8921_s2) {
                        rc = regulator_enable(reg_8921_s2);
                        if (rc)
                                pr_warning("'%s' regulator enable failed, rc=%d\n",
                                        "reg_8921_s2", rc);
                }
		pr_info("%s(on): success\n", __func__);
	} else {
		if (reg_8921_l10) {
			rc = regulator_disable(reg_8921_l10);
			if (rc)
				pr_warning("'%s' regulator disable failed, rc=%d\n",
					"reg_8921_l10", rc);
		}
		if (reg_8921_s2) {
			rc = regulator_disable(reg_8921_s2);
			if (rc)
				pr_warning("'%s' regulator disable failed, rc=%d\n",
					"reg_8921_s2", rc);
		}
		pr_info("%s(off): success\n", __func__);
	}
	return 0;
}

static int mhl_sii9234_power(int on)
{
	int rc = 0;

	switch (on) {
	case 0:
		mhl_sii9234_1v2_power(false);
		break;
	case 1:
		mhl_sii9234_all_power(true);
		config_gpio_table(mhl_gpio_table, ARRAY_SIZE(mhl_gpio_table));
		break;
	default:
		pr_warning("%s(%d) got unsupport parameter!!!\n", __func__, on);
		break;
	}
	return rc;
}

static T_MHL_PLATFORM_DATA mhl_sii9234_device_data = {
	.gpio_intr = VILLE_GPIO_MHL_INT,
	.gpio_reset = VILLE_GPIO_MHL_RSTz,
	.ci2ca = 0,
#ifdef CONFIG_FB_MSM_HDMI_MHL
	.mhl_usb_switch		= ville_usb_dpdn_switch,
	.mhl_1v2_power = mhl_sii9234_1v2_power,
	.enable_5v = hdmi_enable_5v,
	.mhl_power_vote = mhl_sii9234_power_vote,
#ifdef CONFIG_FB_MSM_HDMI_MHL_SUPERDEMO
	.abs_x_min = 941,
	.abs_x_max = 31664,
	.abs_y_min = 417,
	.abs_y_max = 32053,
	.abs_pressure_min = 0,
	.abs_pressure_max = 255,
	.abs_width_min = 0,
	.abs_width_max = 20,
#endif
#endif
	.power = mhl_sii9234_power,
};

static struct i2c_board_info msm_i2c_gsbi8_mhl_sii9234_info[] =
{
	{
		I2C_BOARD_INFO(MHL_SII9234_I2C_NAME, 0x72 >> 1),
		.platform_data = &mhl_sii9234_device_data,
		.irq = VILLE_GPIO_MHL_INT
	},
};

#endif

#endif

#ifdef CONFIG_USB_EHCI_MSM_HSIC

static struct msm_bus_vectors hsic_init_vectors[] = {
       {
               .src = MSM_BUS_MASTER_SPS,
               .dst = MSM_BUS_SLAVE_EBI_CH0,
               .ab = 0,
               .ib = 0,
       },
       {
               .src = MSM_BUS_MASTER_SPS,
               .dst = MSM_BUS_SLAVE_SPS,
               .ab = 0,
               .ib = 0,
       },
};

static struct msm_bus_vectors hsic_max_vectors[] = {
       {
               .src = MSM_BUS_MASTER_SPS,
               .dst = MSM_BUS_SLAVE_EBI_CH0,
               .ab = 60000000,         
               .ib = 960000000,        
       },
       {
               .src = MSM_BUS_MASTER_SPS,
               .dst = MSM_BUS_SLAVE_SPS,
               .ab = 0,
               .ib = 512000000, 
       },
};

static struct msm_bus_paths hsic_bus_scale_usecases[] = {
       {
               ARRAY_SIZE(hsic_init_vectors),
               hsic_init_vectors,
       },
       {
               ARRAY_SIZE(hsic_max_vectors),
               hsic_max_vectors,
       },
};

static struct msm_bus_scale_pdata hsic_bus_scale_pdata = {
       hsic_bus_scale_usecases,
       ARRAY_SIZE(hsic_bus_scale_usecases),
       .name = "hsic",
};

 static struct msm_hsic_host_platform_data msm_hsic_pdata = {
       .strobe                 = 88,
       .data                   = 89,
       .bus_scale_table        = &hsic_bus_scale_pdata,
 };
#else
//TODO:fix warning "unused" or enable HSIC
//static struct msm_hsic_host_platform_data msm_hsic_pdata;
#endif

#define PID_MAGIC_ID		0x71432909
#define SERIAL_NUM_MAGIC_ID	0x61945374
#define SERIAL_NUMBER_LENGTH	127
#define DLOAD_USB_BASE_ADD	0x2A03F0C8

struct magic_num_struct {
	uint32_t pid;
	uint32_t serial_num;
};

struct dload_struct {
	uint32_t	reserved1;
	uint32_t	reserved2;
	uint32_t	reserved3;
	uint16_t	reserved4;
	uint16_t	pid;
	char		serial_number[SERIAL_NUMBER_LENGTH];
	uint16_t	reserved5;
	struct magic_num_struct magic_struct;
};

static int usb_diag_update_pid_and_serial_num(uint32_t pid, const char *snum)
{
	struct dload_struct __iomem *dload = 0;

	dload = ioremap(DLOAD_USB_BASE_ADD, sizeof(*dload));
	if (!dload) {
		pr_err("%s: cannot remap I/O memory region: %08x\n",
					__func__, DLOAD_USB_BASE_ADD);
		return -ENXIO;
	}

	pr_debug("%s: dload:%p pid:%x serial_num:%s\n",
				__func__, dload, pid, snum);
	
	dload->magic_struct.pid = PID_MAGIC_ID;
	dload->pid = pid;

	
	dload->magic_struct.serial_num = 0;
	if (!snum) {
		memset(dload->serial_number, 0, SERIAL_NUMBER_LENGTH);
		goto out;
	}

	dload->magic_struct.serial_num = SERIAL_NUM_MAGIC_ID;
	strlcpy(dload->serial_number, snum, SERIAL_NUMBER_LENGTH);
out:
	iounmap(dload);
	return 0;
}

static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x0BB4,
	.product_id	= 0x0df9,
	.version	= 0x0100,
	.product_name		= "Android Phone",
	.manufacturer_name	= "HTC",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
	.update_pid_and_serial_num = usb_diag_update_pid_and_serial_num,
	.usb_id_pin_gpio = VILLE_GPIO_USB_ID1,
	.usb_rmnet_interface = "HSIC:HSIC",
	.usb_diag_interface = "diag,diag_mdm",
	.fserial_init_string = "HSIC:modem,tty,tty:autobot,tty:serial,tty:autobot,tty:acm",
	.serial_number = "000000000000",
	.nluns		= 1,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};

static struct msm_bus_vectors usb_init_vectors[] = {
       {
               .src = MSM_BUS_MASTER_SPS,
               .dst = MSM_BUS_SLAVE_EBI_CH0,
               .ab = 0,
               .ib = 0,
       },
};

static struct msm_bus_vectors usb_max_vectors[] = {
       {
               .src = MSM_BUS_MASTER_SPS,
               .dst = MSM_BUS_SLAVE_EBI_CH0,
               .ab = 60000000,         
               .ib = 960000000,        
       },
};

static struct msm_bus_paths usb_bus_scale_usecases[] = {
       {
               ARRAY_SIZE(usb_init_vectors),
               usb_init_vectors,
       },
       {
               ARRAY_SIZE(usb_max_vectors),
               usb_max_vectors,
       },
};

static struct msm_bus_scale_pdata usb_bus_scale_pdata = {
       usb_bus_scale_usecases,
       ARRAY_SIZE(usb_bus_scale_usecases),
       .name = "usb",
};

static int phy_init_seq_v3[] = { 0x7f, 0x81, 0x3c, 0x82, -1};
static int phy_init_seq_v3_2_1[] = { 0x5f, 0x81, 0x3c, 0x82, -1};

#ifdef CONFIG_SUPPORT_USB_SPEAKER
struct pm_qos_request pm_qos_req_dma;
void msm_hsusb_setup_gpio(enum usb_otg_state state)
{
	switch (state) {
	case OTG_STATE_UNDEFINED:
		headset_ext_detect(USB_NO_HEADSET);
		pm_qos_update_request(&pm_qos_req_dma, PM_QOS_DEFAULT_VALUE);
		break;
	case OTG_STATE_A_HOST:
		pm_qos_update_request(&pm_qos_req_dma, 3);
		break;
	default:
		break;
	}
}
#endif

static int msm_hsusb_vbus_power(bool on);
static struct msm_otg_platform_data msm_otg_pdata = {
	.phy_init_seq		= phy_init_seq_v3,
	.mode			= USB_OTG,
	.otg_control		= OTG_PMIC_CONTROL,
	.phy_type		= SNPS_28NM_INTEGRATED_PHY,
	.vbus_power		= msm_hsusb_vbus_power,
	.power_budget		= 750,
	.bus_scale_table	= &usb_bus_scale_pdata,
	.ldo_power_collapse	= POWER_COLLAPSE_LDO3V3,
};

int64_t ville_get_usbid_adc(void)
{
       struct pm8xxx_adc_chan_result result;
       int err = 0, adc =0;
	err = pm8xxx_adc_mpp_config_read(PM8XXX_AMUX_MPP_7, ADC_MPP_1_AMUX6, &result);
	
       if (err) {
               pr_info("[CABLE] %s: get adc fail, err %d\n", __func__, err);
               return err;
       }
	pr_info("[CABLE] chan=%d, adc_code=%d, measurement=%lld, \
			physical=%lld\n", result.chan, result.adc_code,
			result.measurement, result.physical);
	adc = result.physical;
	return adc/1000;
}

#define TABLA_INTERRUPT_BASE (NR_MSM_IRQS + NR_GPIO_IRQS + NR_PM8921_IRQS)

static struct wcd9xxx_pdata tabla_platform_data = {
	.slimbus_slave_device = {
		.name = "tabla-slave",
		.e_addr = {0, 0, 0x10, 0, 0x17, 2},
	},
	.irq = MSM_GPIO_TO_INT(62),
	.irq_base = TABLA_INTERRUPT_BASE,
	.num_irqs = NR_WCD9XXX_IRQS,
	.reset_gpio = PM8921_GPIO_PM_TO_SYS(34),
	.micbias = {
		.ldoh_v = TABLA_LDOH_2P85_V,
		.cfilt1_mv = 1800,
		.cfilt2_mv = 1800,
		.cfilt3_mv = 1800,
		.bias1_cfilt_sel = TABLA_CFILT1_SEL,
		.bias2_cfilt_sel = TABLA_CFILT2_SEL,
		.bias3_cfilt_sel = TABLA_CFILT3_SEL,
		.bias4_cfilt_sel = TABLA_CFILT3_SEL,
	},
	.regulator = {
	{
		.name = "CDC_VDD_CP",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_CP_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_RX",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_RX_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_TX",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_TX_CUR_MAX,
	},
	{
		.name = "VDDIO_CDC",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_VDDIO_CDC_CUR_MAX,
	},
	{
		.name = "VDDD_CDC_D",
		.min_uV = 1225000,
		.max_uV = 1250000,
		.optimum_uA = WCD9XXX_VDDD_CDC_D_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_A_1P2V",
		.min_uV = 1225000,
		.max_uV = 1250000,
		.optimum_uA = WCD9XXX_VDDD_CDC_A_CUR_MAX,
	},
	},
};

static struct slim_device msm_slim_tabla = {
	.name = "tabla-slim",
	.e_addr = {0, 1, 0x10, 0, 0x17, 2},
	.dev = {
		.platform_data = &tabla_platform_data,
	},
};

static struct wcd9xxx_pdata tabla20_platform_data = {
	.slimbus_slave_device = {
		.name = "tabla-slave",
		.e_addr = {0, 0, 0x60, 0, 0x17, 2},
	},
	.irq = MSM_GPIO_TO_INT(62),
	.irq_base = TABLA_INTERRUPT_BASE,
	.num_irqs = NR_WCD9XXX_IRQS,
	.reset_gpio = PM8921_GPIO_PM_TO_SYS(34),
	.micbias = {
		.ldoh_v = TABLA_LDOH_2P85_V,
		.cfilt1_mv = 1800,
		.cfilt2_mv = 1800,
		.cfilt3_mv = 1800,
		.bias1_cfilt_sel = TABLA_CFILT1_SEL,
		.bias2_cfilt_sel = TABLA_CFILT2_SEL,
		.bias3_cfilt_sel = TABLA_CFILT3_SEL,
		.bias4_cfilt_sel = TABLA_CFILT3_SEL,
	},
	.amic_settings = {
		.legacy_mode = 0x7F,
		.use_pdata = 0x7F,
	},
	.regulator = {
	{
		.name = "CDC_VDD_CP",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_CP_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_RX",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_RX_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_TX",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_CDC_VDDA_TX_CUR_MAX,
	},
	{
		.name = "VDDIO_CDC",
		.min_uV = 1800000,
		.max_uV = 1800000,
		.optimum_uA = WCD9XXX_VDDIO_CDC_CUR_MAX,
	},
	{
		.name = "VDDD_CDC_D",
		.min_uV = 1225000,
		.max_uV = 1250000,
		.optimum_uA = WCD9XXX_VDDD_CDC_D_CUR_MAX,
	},
	{
		.name = "CDC_VDDA_A_1P2V",
		.min_uV = 1225000,
		.max_uV = 1250000,
		.optimum_uA = WCD9XXX_VDDD_CDC_A_CUR_MAX,
	},
	},
};

static struct slim_device msm_slim_tabla20 = {
	.name = "tabla2x-slim",
	.e_addr = {0, 1, 0x60, 0, 0x17, 2},
	.dev = {
		.platform_data = &tabla20_platform_data,
	},
};


static struct slim_boardinfo msm_slim_devices[] = {
#ifdef CONFIG_WCD9310_CODEC
	{
		.bus_num = 1,
		.slim_slave = &msm_slim_tabla,
	},
	{
		.bus_num = 1,
		.slim_slave = &msm_slim_tabla20,
	},
#endif
	
};

#define MSM_WCNSS_PHYS	0x03000000
#define MSM_WCNSS_SIZE	0x280000

static struct resource resources_wcnss_wlan[] = {
	{
		.start	= RIVA_APPS_WLAN_RX_DATA_AVAIL_IRQ,
		.end	= RIVA_APPS_WLAN_RX_DATA_AVAIL_IRQ,
		.name	= "wcnss_wlanrx_irq",
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= RIVA_APPS_WLAN_DATA_XFER_DONE_IRQ,
		.end	= RIVA_APPS_WLAN_DATA_XFER_DONE_IRQ,
		.name	= "wcnss_wlantx_irq",
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= MSM_WCNSS_PHYS,
		.end	= MSM_WCNSS_PHYS + MSM_WCNSS_SIZE - 1,
		.name	= "wcnss_mmio",
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= 84,
		.end	= 88,
		.name	= "wcnss_gpios_5wire",
		.flags	= IORESOURCE_IO,
	},
};

static struct qcom_wcnss_opts qcom_wcnss_pdata = {
	.has_48mhz_xo	= 1,
};

static struct platform_device msm_device_wcnss_wlan = {
	.name		= "wcnss_wlan",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(resources_wcnss_wlan),
	.resource	= resources_wcnss_wlan,
	.dev		= {.platform_data = &qcom_wcnss_pdata},
};

#ifdef CONFIG_QSEECOM
static struct msm_bus_vectors qseecom_clks_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ib = 0,
		.ab = 0,
	},
	{
		.src = MSM_BUS_MASTER_SPDM,
		.dst = MSM_BUS_SLAVE_SPDM,
		.ib = 0,
		.ab = 0,
	},
};

static struct msm_bus_vectors qseecom_enable_dfab_vectors[] = {
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ib = (492 * 8) * 1000000UL,
		.ab = (492 * 8) *  100000UL,
	},
	{
		.src = MSM_BUS_MASTER_SPDM,
		.dst = MSM_BUS_SLAVE_SPDM,
		.ib = 0,
		.ab = 0,
	},
};

static struct msm_bus_vectors qseecom_enable_sfpb_vectors[] = {
	{
		.src = MSM_BUS_MASTER_SPS,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ib = 0,
		.ab = 0,
	},
	{
		.src = MSM_BUS_MASTER_SPDM,
		.dst = MSM_BUS_SLAVE_SPDM,
		.ib = (64 * 8) * 1000000UL,
		.ab = (64 * 8) *  100000UL,
	},
};

static struct msm_bus_paths qseecom_hw_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(qseecom_clks_init_vectors),
		qseecom_clks_init_vectors,
	},
	{
		ARRAY_SIZE(qseecom_enable_dfab_vectors),
		qseecom_enable_sfpb_vectors,
	},
	{
		ARRAY_SIZE(qseecom_enable_sfpb_vectors),
		qseecom_enable_sfpb_vectors,
	},
};

static struct msm_bus_scale_pdata qseecom_bus_pdata = {
	qseecom_hw_bus_scale_usecases,
	ARRAY_SIZE(qseecom_hw_bus_scale_usecases),
	.name = "qsee",
};

static struct platform_device qseecom_device = {
	.name		= "qseecom",
	.id		= 0,
	.dev		= {
		.platform_data = &qseecom_bus_pdata,
	},
};
#endif

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

#define QCE_SIZE		0x10000
#define QCE_0_BASE		0x18500000

#define QCE_HW_KEY_SUPPORT	0
#define QCE_SHA_HMAC_SUPPORT	1
#define QCE_SHARE_CE_RESOURCE	1
#define QCE_CE_SHARED		0

static struct msm_bus_vectors crypto_hw_init_vectors[] = {
        {
                .src = MSM_BUS_MASTER_ADM_PORT0,
                .dst = MSM_BUS_SLAVE_EBI_CH0,
                .ab = 0,
                .ib = 0,
        },
        {
                .src = MSM_BUS_MASTER_ADM_PORT1,
                .dst = MSM_BUS_SLAVE_GSBI1_UART,
                .ab = 0,
                .ib = 0,
        },
};

static struct msm_bus_vectors crypto_hw_active_vectors[] = {
        {
                .src = MSM_BUS_MASTER_ADM_PORT0,
                .dst = MSM_BUS_SLAVE_EBI_CH0,
                .ab = 70000000UL,
                .ib = 70000000UL,
        },
        {
                .src = MSM_BUS_MASTER_ADM_PORT1,
                .dst = MSM_BUS_SLAVE_GSBI1_UART,
                .ab = 2480000000UL,
                .ib = 2480000000UL,
        },
};

static struct msm_bus_paths crypto_hw_bus_scale_usecases[] = {
        {
                ARRAY_SIZE(crypto_hw_init_vectors),
                crypto_hw_init_vectors,
        },
        {
                ARRAY_SIZE(crypto_hw_active_vectors),
                crypto_hw_active_vectors,
        },
};

static struct msm_bus_scale_pdata crypto_hw_bus_scale_pdata = {
                crypto_hw_bus_scale_usecases,
                ARRAY_SIZE(crypto_hw_bus_scale_usecases),
                .name = "cryptohw",
};

static struct resource qcrypto_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

static struct resource qcedev_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

#endif

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)

static struct msm_ce_hw_support qcrypto_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
	.bus_scale_table = &crypto_hw_bus_scale_pdata,
};

static struct platform_device qcrypto_device = {
	.name		= "qcrypto",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcrypto_resources),
	.resource	= qcrypto_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcrypto_ce_hw_suppport,
	},
};
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

static struct msm_ce_hw_support qcedev_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
	.sha_hmac = QCE_SHA_HMAC_SUPPORT,
	.bus_scale_table = &crypto_hw_bus_scale_pdata,
};

static struct platform_device qcedev_device = {
	.name		= "qce",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcedev_resources),
	.resource	= qcedev_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcedev_ce_hw_suppport,
	},
};
#endif

static uint32_t usb_ID_PIN_input_table[] = {
	GPIO_CFG(VILLE_GPIO_USB_ID1, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t usb_ID_PIN_ouput_table[] = {
	GPIO_CFG(VILLE_GPIO_USB_ID1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

void config_ville_usb_id_gpios(bool output)
{
	if (output) {
		gpio_tlmm_config(usb_ID_PIN_ouput_table[0], GPIO_CFG_ENABLE);
		gpio_set_value(VILLE_GPIO_USB_ID1, 1);
		pr_info("[CABLE] %s: %d output high\n",  __func__, VILLE_GPIO_USB_ID1);
	} else {
		gpio_tlmm_config(usb_ID_PIN_input_table[0], GPIO_CFG_ENABLE);
		pr_info("[CABLE] %s: %d input none pull\n",  __func__, VILLE_GPIO_USB_ID1);
	}
}



static struct cable_detect_platform_data cable_detect_pdata = {
	.detect_type		= CABLE_TYPE_PMIC_ADC,
	.usb_id_pin_gpio	= VILLE_GPIO_USB_ID1,
	.get_adc_cb		= ville_get_usbid_adc,
	.config_usb_id_gpios	= config_ville_usb_id_gpios,
	.mhl_reset_gpio = VILLE_GPIO_MHL_RSTz,
#ifdef CONFIG_FB_MSM_HDMI_MHL
	.mhl_1v2_power = mhl_sii9234_1v2_power,
	.usb_dpdn_switch	= ville_usb_dpdn_switch,
#endif
};

static struct platform_device cable_detect_device = {
	.name   = "cable_detect",
	.id     = -1,
	.dev    = {
		.platform_data = &cable_detect_pdata,
	},
};

static void ville_cable_detect_register(void)
{
	pr_info("%s\n", __func__);
	platform_device_register(&cable_detect_device);
}

static void pm8xxx_adc_device_register(void)
{
	pr_info("%s: Register PM8XXX ADC device\n", __func__);
	headset_device_register();
}


static int pm8921_therm_mitigation[] = {
	1100,
	700,
	600,
	225,
};

static struct pm8921_charger_platform_data pm8921_chg_pdata __devinitdata = {
	.safety_time		= 510,
	.update_time		= 60000,
	.max_voltage		= 4200,
	.min_voltage		= 3200,
	.resume_voltage_delta	= 50,
	.term_current		= 50,
	.cool_temp		= 0,
	.warm_temp		= 48,
	.temp_check_period	= 1,
	.max_bat_chg_current	= 1025,
	.cool_bat_chg_current	= 1025,
	.warm_bat_chg_current	= 1025,
	.cool_bat_voltage	= 4200,
	.warm_bat_voltage	= 4000,
	.mbat_in_gpio		= VILLE_GPIO_MBAT_IN,
	.is_embeded_batt	= 1,
	.thermal_mitigation	= pm8921_therm_mitigation,
	.thermal_levels		= ARRAY_SIZE(pm8921_therm_mitigation),
	.cold_thr = PM_SMBC_BATT_TEMP_COLD_THR__HIGH,
	.hot_thr = PM_SMBC_BATT_TEMP_HOT_THR__LOW,
};

static struct pm8921_bms_platform_data pm8921_bms_pdata __devinitdata = {
	.r_sense		= 10,
	.i_test			= 0,
	.v_failure		= 3000,
	.max_voltage_uv		= 4200 * 1000,
};

static int __init check_dq_setup(char *str)
{
	int i = 0;
	int size = 0;

	size = sizeof(chg_batt_params)/sizeof(chg_batt_params[0]);

	if (!strcmp(str, "PASS")) {
		pr_info("[BATT] overwrite HV battery config\n");
		pm8921_chg_pdata.max_voltage = 4340;
		pm8921_chg_pdata.cool_bat_voltage = 4340;
		pm8921_bms_pdata.max_voltage_uv = 4340 * 1000;
	} else {
		pr_info("[BATT] use default battery config\n");
		pm8921_chg_pdata.max_voltage = 4200;
		pm8921_chg_pdata.cool_bat_voltage = 4200;
		pm8921_bms_pdata.max_voltage_uv = 4200 * 1000;

		for(i=0; i < size; i++)
		{
			chg_batt_params[i].max_voltage = 4200;
			chg_batt_params[i].cool_bat_voltage = 4200;
		}
	}
	return 1;
}
__setup("androidboot.dq=", check_dq_setup);

static struct pm8xxx_adc_amux pm8xxx_adc_channels_data[] = {
	{"vcoin", CHANNEL_VCOIN, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"vbat", CHANNEL_VBAT, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"dcin", CHANNEL_DCIN, CHAN_PATH_SCALING4, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"ichg", CHANNEL_ICHG, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"vph_pwr", CHANNEL_VPH_PWR, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"ibat", CHANNEL_IBAT, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"batt_therm", CHANNEL_BATT_THERM, CHAN_PATH_SCALING1, AMUX_RSV2,
		ADC_DECIMATION_TYPE2, ADC_SCALE_BATT_THERM},
	{"batt_id", CHANNEL_BATT_ID, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"usbin", CHANNEL_USBIN, CHAN_PATH_SCALING3, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"pmic_therm", CHANNEL_DIE_TEMP, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PMIC_THERM},
	{"625mv", CHANNEL_625MV, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"125v", CHANNEL_125V, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"chg_temp", CHANNEL_CHG_TEMP, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"pa_therm1", ADC_MPP_1_AMUX8, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PA_THERM},
	{"xo_therm", CHANNEL_MUXOFF, CHAN_PATH_SCALING1, AMUX_RSV0,
		ADC_DECIMATION_TYPE2, ADC_SCALE_XOTHERM},
	{"pa_therm0", ADC_MPP_1_AMUX3, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PA_THERM},
	{"mpp_amux6", ADC_MPP_1_AMUX6, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
};

static struct pm8xxx_adc_properties pm8xxx_adc_data = {
	.adc_vdd_reference	= 1800, 
	.bitresolution		= 15,
	.bipolar                = 0,
};

static const struct pm8xxx_adc_map_pt ville_adcmap_btm_table[] = {
	{-200,	1671},
	{-190,	1663},
	{-180,	1654},
	{-170,	1646},
	{-160,	1636},
	{-150,	1627},
	{-140,	1617},
	{-130,	1606},
	{-120,	1595},
	{-110,	1584},
	{-100,	1572},
	{-90,	1560},
	{-80,	1548},
	{-70,	1534},
	{-60,	1521},
	{-50,	1507},
	{-40,	1492},
	{-30,	1477},
	{-20,	1462},
	{-10,	1446},
	{-0,	1430},
	{10,	1413},
	{20,	1396},
	{30,	1379},
	{40,	1361},
	{50,	1343},
	{60,	1325},
	{70,	1306},
	{80,	1287},
	{90,	1267},
	{100,	1248},
	{110,	1228},
	{120,	1208},
	{130,	1188},
	{140,	1168},
	{150,	1147},
	{160,	1127},
	{170,	1106},
	{180,	1086},
	{190,	1065},
	{200,	1044},
	{210,	1024},
	{220,	1004},
	{230,	983},
	{240,	963},
	{250,	943},
	{260,	923},
	{270,	903},
	{280,	884},
	{290,	864},
	{300,	845},
	{310,	827},
	{320,	808},
	{330,	790},
	{340,	772},
	{350,	755},
	{360,	738},
	{370,	721},
	{380,	704},
	{390,	688},
	{400,	672},
	{410,	657},
	{420,	642},
	{430,	627},
	{440,	613},
	{450,	599},
	{460,	585},
	{470,	572},
	{480,	559},
	{490,	547},
	{500,	535},
	{510,	523},
	{520,	511},
	{530,	500},
	{540,	489},
	{550,	479},
	{560,	469},
	{570,	459},
	{580,	449},
	{590,	440},
	{600,	431},
	{610,	423},
	{620,	414},
	{630,	406},
	{640,	398},
	{650,	390},
	{660,	383},
	{670,	376},
	{680,	369},
	{690,	363},
	{700,	356},
	{710,	350},
	{720,	344},
	{730,	338},
	{740,	333},
	{750,	327},
	{760,	322},
	{770,	317},
	{780,	312},
	{790,	308}
};

static struct pm8xxx_adc_map_table pm8xxx_adcmap_btm_table = {
	.table = ville_adcmap_btm_table,
	.size = ARRAY_SIZE(ville_adcmap_btm_table),
};

static struct pm8xxx_adc_platform_data pm8xxx_adc_pdata = {
	.adc_channel		= pm8xxx_adc_channels_data,
	.adc_num_board_channel	= ARRAY_SIZE(pm8xxx_adc_channels_data),
	.adc_prop		= &pm8xxx_adc_data,
	.adc_mpp_base		= PM8921_MPP_PM_TO_SYS(1),
	.adc_map_btm_table	= &pm8xxx_adcmap_btm_table,
	.pm8xxx_adc_device_register	= pm8xxx_adc_device_register,
};

#define MSM_SHARED_RAM_PHYS 0x80000000

static void __init m7_map_io(void)
{
	msm_shared_ram_phys = MSM_SHARED_RAM_PHYS;
	msm_map_msm8960_io();
	if (socinfo_init() < 0)
		pr_err("socinfo_init() failed!\n");
}

static void __init ville_init_irq(void)
{
	struct msm_mpm_device_data *data = NULL;

#ifdef CONFIG_MSM_MPM
	data = &msm8960_mpm_dev_data;
#endif

	msm_mpm_irq_extn_init(data);
	gic_init(0, GIC_PPI_START, MSM_QGIC_DIST_BASE,
					(void *)MSM_QGIC_CPU_BASE);
}

static struct platform_device msm_device_saw_core0 = {
	.name          = "saw-regulator",
	.id            = 0,
	.dev	= {
		.platform_data = &msm_saw_regulator_pdata_s5,
	},
};

static struct platform_device msm_device_saw_core1 = {
	.name          = "saw-regulator",
	.id            = 1,
	.dev	= {
		.platform_data = &msm_saw_regulator_pdata_s6,
	},
};

static struct msm_rpmrs_level msm_rpmrs_levels[] = {
	{
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1, 784, 180000, 100,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE,
		MSM_RPMRS_LIMITS(ON, ACTIVE, MAX, ACTIVE),
		true,
		1300, 228, 1200000, 3212,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, GDHS, MAX, ACTIVE),
		false,
		2000, 138, 1208400, 9152,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(ON, HSFS_OPEN, ACTIVE, RET_HIGH),
		false,
		6000, 119, 1850300, 10212,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, GDHS, MAX, ACTIVE),
		false,
		9200, 68, 2839200, 17612,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, MAX, ACTIVE),
		false,
		10300, 63, 3128000, 19412,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, ACTIVE, RET_HIGH),
		false,
		18000, 10, 4602600, 28212,
	},

	{
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE,
		MSM_RPMRS_LIMITS(OFF, HSFS_OPEN, RET_HIGH, RET_LOW),
		false,
		20000, 2, 5752000, 33212,
	},
};

static struct msm_pm_boot_platform_data msm_pm_boot_pdata __initdata = {
	.mode = MSM_PM_BOOT_CONFIG_TZ,
};

static struct msm_rpmrs_platform_data msm_rpmrs_data __initdata = {
	.levels = &msm_rpmrs_levels[0],
	.num_levels = ARRAY_SIZE(msm_rpmrs_levels),
	.vdd_mem_levels  = {
		[MSM_RPMRS_VDD_MEM_RET_LOW]	= 750000,
		[MSM_RPMRS_VDD_MEM_RET_HIGH]	= 750000,
		[MSM_RPMRS_VDD_MEM_ACTIVE]	= 1050000,
		[MSM_RPMRS_VDD_MEM_MAX]		= 1150000,
	},
	.vdd_dig_levels = {
		[MSM_RPMRS_VDD_DIG_RET_LOW]	= 500000,
		[MSM_RPMRS_VDD_DIG_RET_HIGH]	= 750000,
		[MSM_RPMRS_VDD_DIG_ACTIVE]	= 950000,
		[MSM_RPMRS_VDD_DIG_MAX]		= 1150000,
	},
	.vdd_mask = 0x7FFFFF,
	.rpmrs_target_id = {
		[MSM_RPMRS_ID_PXO_CLK]		= MSM_RPM_ID_PXO_CLK,
		[MSM_RPMRS_ID_L2_CACHE_CTL]	= MSM_RPM_ID_LAST,
		[MSM_RPMRS_ID_VDD_DIG_0]	= MSM_RPM_ID_PM8921_S3_0,
		[MSM_RPMRS_ID_VDD_DIG_1]	= MSM_RPM_ID_PM8921_S3_1,
		[MSM_RPMRS_ID_VDD_MEM_0]	= MSM_RPM_ID_PM8921_L24_0,
		[MSM_RPMRS_ID_VDD_MEM_1]	= MSM_RPM_ID_PM8921_L24_1,
		[MSM_RPMRS_ID_RPM_CTL]		= MSM_RPM_ID_RPM_CTL,
	},
};



static struct msm_spi_platform_data msm8960_qup_spi_gsbi10_pdata = {
	.max_clock_speed = 27000000,
};

#define USB_5V_EN		42
static int msm_hsusb_vbus_power(bool on)
{
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (on) {
		rc = gpio_request(VILLE_GPIO_V_BOOST_5V_EN, "USB_BOOST_5V");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_BOOST_5V", VILLE_GPIO_V_BOOST_5V_EN, rc);
			return rc;
		}
		gpio_set_value(VILLE_GPIO_V_BOOST_5V_EN, 1);
		pr_info("%s(on): success\n", __func__);
	} else {
		gpio_set_value(VILLE_GPIO_V_BOOST_5V_EN, 0);
		gpio_free(VILLE_GPIO_V_BOOST_5V_EN);
		pr_info("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
}



#define VERSION_ID (readl(HW_VER_ID_VIRT) & 0xf0000000) >> 28
#define HW_8960_V3_2_1   0x07
void ville_add_usb_devices(void)
{
	if (VERSION_ID >= HW_8960_V3_2_1) {
		printk(KERN_INFO "%s rev: %d v3.2.1\n", __func__, system_rev);
		msm_otg_pdata.phy_init_seq = phy_init_seq_v3_2_1;
	} else {
		printk(KERN_INFO "%s rev: %d\n", __func__, system_rev);
		msm_otg_pdata.phy_init_seq = phy_init_seq_v3;
	}
	android_usb_pdata.products[0].product_id =
			android_usb_pdata.product_id;

	
	if (get_radio_flag() & 0x20000) {
		android_usb_pdata.diag_init = 1;
		android_usb_pdata.modem_init = 1;
		android_usb_pdata.rmnet_init = 1;
	}

	
	if (board_mfg_mode() == 0) {
		android_usb_pdata.nluns = 1;
		android_usb_pdata.cdrom_lun = 0x1;
	}
	android_usb_pdata.serial_number = board_serialno();

	platform_device_register(&msm8960_device_gadget_peripheral);
	platform_device_register(&android_usb_device);

	printk(KERN_INFO "%s: OTG_PMIC_CONTROL in rev: %d\n",
			__func__, system_rev);
}


static uint8_t spm_wfi_cmd_sequence[] __initdata = {
	0x03, 0x0f,
};

static uint8_t spm_power_collapse_without_rpm[] __initdata = {
	0x00, 0x24, 0x54, 0x10,
	0x09, 0x03, 0x01,
	0x10, 0x54, 0x30, 0x0C,
	0x24, 0x30, 0x0f,
};

static uint8_t spm_power_collapse_with_rpm[] __initdata = {
	0x00, 0x24, 0x54, 0x10,
	0x09, 0x07, 0x01, 0x0B,
	0x10, 0x54, 0x30, 0x0C,
	0x24, 0x30, 0x0f,
};

static struct msm_spm_seq_entry msm_spm_seq_list[] __initdata = {
	[0] = {
		.mode = MSM_SPM_MODE_CLOCK_GATING,
		.notify_rpm = false,
		.cmd = spm_wfi_cmd_sequence,
	},
	[1] = {
		.mode = MSM_SPM_MODE_POWER_COLLAPSE,
		.notify_rpm = false,
		.cmd = spm_power_collapse_without_rpm,
	},
	[2] = {
		.mode = MSM_SPM_MODE_POWER_COLLAPSE,
		.notify_rpm = true,
		.cmd = spm_power_collapse_with_rpm,
	},
};

static uint8_t l2_spm_wfi_cmd_sequence[] __initdata = {
	0x00, 0x20, 0x03, 0x20,
	0x00, 0x0f,
};

static uint8_t l2_spm_gdhs_cmd_sequence[] __initdata = {
	0x00, 0x20, 0x34, 0x64,
	0x48, 0x07, 0x48, 0x20,
	0x50, 0x64, 0x04, 0x34,
	0x50, 0x0f,
};
static uint8_t l2_spm_power_off_cmd_sequence[] __initdata = {
	0x00, 0x10, 0x34, 0x64,
	0x48, 0x07, 0x48, 0x10,
	0x50, 0x64, 0x04, 0x34,
	0x50, 0x0F,
};

static struct msm_spm_seq_entry msm_spm_l2_seq_list[] __initdata = {
	[0] = {
		.mode = MSM_SPM_L2_MODE_RETENTION,
		.notify_rpm = false,
		.cmd = l2_spm_wfi_cmd_sequence,
	},
	[1] = {
		.mode = MSM_SPM_L2_MODE_GDHS,
		.notify_rpm = true,
		.cmd = l2_spm_gdhs_cmd_sequence,
	},
	[2] = {
		.mode = MSM_SPM_L2_MODE_POWER_COLLAPSE,
		.notify_rpm = true,
		.cmd = l2_spm_power_off_cmd_sequence,
	},
};


static struct msm_spm_platform_data msm_spm_l2_data[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW_L2_BASE,
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x02020204,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x00A000AE,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x00A00020,
		.modes = msm_spm_l2_seq_list,
		.num_modes = ARRAY_SIZE(msm_spm_l2_seq_list),
	},
};
static struct msm_spm_platform_data msm_spm_data[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW0_BASE,
		.reg_init_values[MSM_SPM_REG_SAW2_CFG] = 0x1F,
#if defined(CONFIG_MSM_AVS_HW)
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_CTL] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_HYSTERESIS] = 0x00,
#endif
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x02020204,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x0060009C,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x0000001C,
		.vctl_timeout_us = 50,
		.num_modes = ARRAY_SIZE(msm_spm_seq_list),
		.modes = msm_spm_seq_list,
	},
	[1] = {
		.reg_base_addr = MSM_SAW1_BASE,
		.reg_init_values[MSM_SPM_REG_SAW2_CFG] = 0x1F,
#if defined(CONFIG_MSM_AVS_HW)
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_CTL] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW2_AVS_HYSTERESIS] = 0x00,
#endif
		.reg_init_values[MSM_SPM_REG_SAW2_SPM_CTL] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DLY] = 0x02020204,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_0] = 0x0060009C,
		.reg_init_values[MSM_SPM_REG_SAW2_PMIC_DATA_1] = 0x0000001C,
		.vctl_timeout_us = 50,
		.num_modes = ARRAY_SIZE(msm_spm_seq_list),
		.modes = msm_spm_seq_list,
	},
};

#ifdef CONFIG_FLASHLIGHT_TPS61310
static void config_flashlight_gpios(void)
{
	static uint32_t flashlight_gpio_table[] = {
		GPIO_CFG(VILLE_GPIO_TORCH_FLASHz, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		GPIO_CFG(VILLE_GPIO_DRIVER_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	};

	gpio_tlmm_config(flashlight_gpio_table[0], GPIO_CFG_ENABLE);
	gpio_tlmm_config(flashlight_gpio_table[1], GPIO_CFG_ENABLE);
}

static struct TPS61310_flashlight_platform_data ville_flashlight_data = {
	.gpio_init = config_flashlight_gpios,
	.tps61310_strb0 = VILLE_GPIO_DRIVER_EN,
	.tps61310_strb1 = VILLE_GPIO_TORCH_FLASHz,
	.flash_duration_ms = 600,
	.mode_pin_suspend_state_low = 1,
};

static struct i2c_board_info i2c_tps61310_flashlight[] = {
	{
		I2C_BOARD_INFO("TPS61310_FLASHLIGHT", 0x66 >> 1),
		.platform_data = &ville_flashlight_data,
	},
};
#endif

static struct msm_pm_sleep_status_data msm_pm_slp_sts_data = {
	.base_addr = MSM_ACC0_BASE + 0x08,
	.cpu_offset = MSM_ACC1_BASE - MSM_ACC0_BASE,
	.mask = 1UL << 13,
};

static struct atmel_cfg ts_atmel_mferr_cfg_data[] = {
	{.objid = GEN_POWERCONFIG_T7,		.byte = 1,     .value = 255,    .orival = 12},
	{.objid = TOUCH_MULTITOUCHSCREEN_T9,    .byte = 7,     .value = 105,	.orival = 50},
	{.objid = TOUCH_MULTITOUCHSCREEN_T9,    .byte = 13,    .value = 1,	.orival = 15},
	{.objid = TOUCH_MULTITOUCHSCREEN_T9,    .byte = 31,    .value = 25,	.orival = 10},
	{.objid = TOUCH_MULTITOUCHSCREEN_T9,    .byte = 34,    .value = 3,	.orival = 2},
	{.objid = EXTRA_NOISE_SUPPRESSION_T58,  .byte = 0,     .value = 0,	.orival = 100},
	{.objid = EXTRA_NOISE_SUPPRESSION_T58,  .byte = 1,     .value = 0, 	.orival = 20},
	{.objid = EXTRA_NOISE_SUPPRESSION_T58,  .byte = 2,     .value = 0,	.orival = 3},
	{.objid = EXTRA_NOISE_SUPPRESSION_T58,  .byte = 3,     .value = 0,	.orival = 1},
	{.objid = EXTRA_NOISE_SUPPRESSION_T58,  .byte = 4,     .value = 0,	.orival = 50},
	{.objid = SPT_CTECONFIG_T46,            .byte = 3,     .value = 63,	.orival = 16},
	{.objid = PROCG_NOISESUPPRESSION_T48,   .byte = 8,     .value = 18,	.orival = 0},
	{.objid = PROCG_NOISESUPPRESSION_T48,   .byte = 9,     .value = 30,	.orival = 0},
	{.objid = PROCG_NOISESUPPRESSION_T48,   .byte = 11,    .value = 45,	.orival = 20},
};

static void __init msm8960_init_buses(void)
{
	msm_bus_8960_apps_fabric_pdata.rpm_enabled = 1;
	msm_bus_8960_sys_fabric_pdata.rpm_enabled = 1;
	msm_bus_8960_mm_fabric_pdata.rpm_enabled = 1;
	msm_bus_apps_fabric.dev.platform_data =
		&msm_bus_8960_apps_fabric_pdata;
	msm_bus_sys_fabric.dev.platform_data = &msm_bus_8960_sys_fabric_pdata;
	msm_bus_mm_fabric.dev.platform_data = &msm_bus_8960_mm_fabric_pdata;
	msm_bus_sys_fpb.dev.platform_data = &msm_bus_8960_sys_fpb_pdata;
	msm_bus_cpss_fpb.dev.platform_data = &msm_bus_8960_cpss_fpb_pdata;
	msm_bus_rpm_set_mt_mask();
}

static struct atmel_cfg ts_atmel_cfm_calb_data[] = {
	{.objid = GEN_ACQUISITIONCONFIG_T8,    .byte = 6,    .value = 255,	.orival = 5},
	{.objid = GEN_ACQUISITIONCONFIG_T8,    .byte = 7,    .value = 1,	.orival = 25},
	{.objid = GEN_ACQUISITIONCONFIG_T8,    .byte = 8,    .value = 0,	.orival = 5},
	{.objid = GEN_ACQUISITIONCONFIG_T8,    .byte = 9,    .value = 0,	.orival = 192},
};

static struct atmel_cfg ts_atmel_cable_cfg_data[] = {
	{.objid = TOUCH_MULTITOUCHSCREEN_T9,    .byte = 8,    .value = 3,	.orival = 2},
};

static struct atmel_i2c_platform_data ts_atmel_data[] = {
	{
		.version          = 0x0011,
		.build            = 0xAA,
		.source 	  = 0,
		.abs_x_min        = 0,
		.abs_x_max        = 539,
		.abs_y_min        = 0,
		.abs_y_max        = 959,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min    = 0,
		.abs_width_max    = 20,
		.gpio_irq         = VILLE_GPIO_TP_ATTz,
		.unlock_attr      = 1,
		.config_T6        = {0, 0, 0, 0, 0, 0},
		.config_T7        = {50, 10, 25},
		.config_T8        = {30, 0, 5, 5, 0, 0, 5, 25, 5, 192},
		.config_T9        = {131, 0, 0, 19, 11, 0, 32, 50, 2, 1,
				     0, 3, 1, 15, 5, 10, 10, 10, 191, 3,
				     27, 2, 6, 252, 30, 25, 184, 35, 180, 55,
				     20, 10, 0, 0, 2},
		.config_T15       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T18       = {0, 0},
		.config_T19       = {0, 0, 0, 60, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0},
		.config_T23       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0, 0},
		.config_T25       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0},
		.config_T40       = {0, 0, 0, 0, 0},
		.config_T42       = {0, 0, 0, 0, 0, 0, 0, 0},
		.config_T46       = {0, 3, 16, 16, 0, 0, 0, 0, 0},
		.config_T47       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T48       = {3, 0, 66, 0, 0, 0, 0, 0, 0, 0,
				     16, 20, 0, 6, 6, 0, 0, 100, 4, 64,
				     10, 0, 20, 5, 0, 38, 0, 5, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0},
		.config_T55       = {0, 0, 0, 0},
		.config_T58       = {100, 20, 3, 1, 50, 0, 0, 0, 0, 0,
				     0},
		.object_crc       = {0xE4, 0x29, 0x74},
		.workaround 	  = TW_SHIFT,
		.cable_config	  = {	.cnt = ARRAY_SIZE(ts_atmel_cable_cfg_data),
					.cfg = ts_atmel_cable_cfg_data, },
		.mferr_config 	  = { 	.cnt = ARRAY_SIZE(ts_atmel_mferr_cfg_data),
					.cfg = ts_atmel_mferr_cfg_data, },
		.cfm_calb	  = {	.cnt = ARRAY_SIZE(ts_atmel_cfm_calb_data),
					.cfg = ts_atmel_cfm_calb_data, },
#if defined(CONFIG_TOUCH_KEY_FILTER)
		.flt_th	  = 945,
#endif
	},
	{
		.version          = 0x0011,
		.build            = 0xAA,
		.source 	  = 1,
		.abs_x_min        = 0,
		.abs_x_max        = 539,
		.abs_y_min        = 0,
		.abs_y_max        = 959,
		.abs_pressure_min = 0,
		.abs_pressure_max = 255,
		.abs_width_min    = 0,
		.abs_width_max    = 20,
		.gpio_irq         = VILLE_GPIO_TP_ATTz,
		.unlock_attr      = 1,
		.config_T6        = {0, 0, 0, 0, 0, 0},
		.config_T7        = {50, 8, 25},
		.config_T8        = {30, 0, 5, 5, 0, 0, 5, 25, 5, 192},
		.config_T9        = {131, 0, 0, 19, 11, 0, 32, 50, 2, 5,
				     0, 3, 1, 15, 5, 10, 10, 10, 191, 3,
				     27, 2, 6, 0, 30, 25, 184, 35, 180, 55,
				     20, 10, 0, 0, 2},
		.config_T15       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T18       = {0, 0},
		.config_T19       = {0, 0, 0, 60, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0},
		.config_T23       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0, 0},
		.config_T25       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0},
		.config_T40       = {0, 0, 0, 0, 0},
		.config_T42       = {0, 0, 0, 0, 0, 0, 0, 0},
		.config_T46       = {0, 3, 16, 16, 0, 0, 0, 0, 0},
		.config_T47       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.config_T48       = {3, 0, 66, 0, 0, 0, 0, 0, 0, 0,
				     16, 20, 0, 6, 6, 0, 0, 100, 4, 64,
				     10, 0, 20, 5, 0, 38, 0, 5, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				     0, 0, 0, 0},
		.config_T55       = {0, 0, 0, 0},
		.config_T58       = {100, 20, 3, 1, 50, 0, 0, 0, 0, 0,
				     0},
		.workaround 	  = TW_SHIFT,
		.cable_config	  = {	.cnt = ARRAY_SIZE(ts_atmel_cable_cfg_data),
					.cfg = ts_atmel_cable_cfg_data, },
		.mferr_config 	  = { 	.cnt = ARRAY_SIZE(ts_atmel_mferr_cfg_data),
					.cfg = ts_atmel_mferr_cfg_data, },
		.cfm_calb	  = {	.cnt = ARRAY_SIZE(ts_atmel_cfm_calb_data),
					.cfg = ts_atmel_cfm_calb_data, },
#if defined(CONFIG_TOUCH_KEY_FILTER)
		.flt_th	  = 945,
#endif
	},
        {
                .version = 0x0010,
                .abs_x_min = 0,
                .abs_x_max = 2047,
                .abs_y_min = 0,
                .abs_y_max = 2047,
                .abs_pressure_min = 0,
                .abs_pressure_max = 255,
                .abs_width_min = 0,
                .abs_width_max = 20,
                .gpio_irq = VILLE_GPIO_TP_ATTz,
                .config_T6 = {0, 0, 0, 0, 0, 0},
                .config_T7 = {15, 8, 50},
                .config_T8 = {30, 0, 5, 5, 0, 64, 5, 1, 0, 0},
                .config_T9 = {139, 0, 0, 19, 11, 0, 16, 40, 0, 1, 0, 5, 2, 0, 5, 20, 20, 20, 255, 7, 255, 7, 5, 0, 30, 30, 141, 60, 153, 75, 18, 15, 54, 54, 0},
                .config_T15 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                .config_T18 = {0, 0},
                .config_T19 = {0, 0, 0, 60, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                .config_T23 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                .config_T25 = {3, 0, 48, 117, 156, 99, 0, 0, 0, 0, 0, 0, 0, 0},
                .config_T40 = {0, 0, 0, 0, 0},
                .config_T42 = {0, 50, 23, 23, 250, 3, 0, 5},
                .config_T46 = {0, 3, 8, 8, 0, 0, 0, 0, 0},
                .config_T47 = {0, 20, 50, 5, 2, 50, 40, 0, 0, 63},
                .config_T48 = {0, 196, 96, 20, 0, 0, 0, 0, 12, 12, 16, 22, 0, 6, 6, 0, 0, 100, 4, 64, 10, 0, 20, 5, 0, 38, 0, 20, 0, 0, 0, 0, 0, 0, 0, 35, 3, 5, 2, 0, 5, 10, 10, 239, 238, 35, 35, 143, 75, 138, 50, 18, 17, 4},

                
                .GCAF_level = {20, 24, 28, 40, 63},
        },
};

static struct i2c_board_info msm_i2c_gsbi3_info[] = {
	{
		I2C_BOARD_INFO(ATMEL_MXT224E_NAME, 0x94 >> 1),
		.platform_data = &ts_atmel_data,
		.irq = MSM_GPIO_TO_INT(VILLE_GPIO_TP_ATTz)
	},
};

int cy8c_cs_reset(void)
{
	pr_info("[cap]%s Enter\n", __func__);

	gpio_set_value_cansleep(PM8921_GPIO_PM_TO_SYS(VILLE_PMGPIO_CAP_RST), 1);
	msleep(5);
	gpio_set_value_cansleep(PM8921_GPIO_PM_TO_SYS(VILLE_PMGPIO_CAP_RST), 0);

	return 0;
}

struct cy8c_i2c_cs_platform_data cs_cy8c_data[] = {
	{
		.gpio_irq = VILLE_GPIO_CAP_SENSOR_INTz,
		.gpio_rst = PM8921_GPIO_PM_TO_SYS(VILLE_PMGPIO_CAP_RST),
		.reset    = cy8c_cs_reset,
		.keycode  = {KEY_BACK, KEY_HOME, KEY_APP_SWITCH},
		.func_support = CS_FUNC_PRINTRAW,
		.id       = {
			.config = CS_KEY_3,
		},
	},
	{
		.gpio_irq = VILLE_GPIO_CAP_SENSOR_INTz,
		.gpio_rst = PM8921_GPIO_PM_TO_SYS(VILLE_PMGPIO_CAP_RST),
		.reset    = cy8c_cs_reset,
		.keycode  = {KEY_BACK, KEY_HOME, KEY_APP_SWITCH, KEY_WEIBO},
		.func_support = CS_FUNC_PRINTRAW,
		.id       = {
			.config = CS_KEY_4,
		},
	},
};

static struct i2c_board_info msm_i2c_gsbi5_info[] = {
	{
		I2C_BOARD_INFO(CYPRESS_CS_NAME, 0x40 >> 1),
		.platform_data = &cs_cy8c_data,
		.irq = MSM_GPIO_TO_INT(VILLE_GPIO_CAP_SENSOR_INTz),
	},
};

static uint32_t gsbi3_gpio_table[] = {
	GPIO_CFG(VILLE_GPIO_TP_I2C_DAT, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(VILLE_GPIO_TP_I2C_CLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi3_gpio_table_gpio[] = {
	GPIO_CFG(VILLE_GPIO_TP_I2C_DAT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(VILLE_GPIO_TP_I2C_CLK, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi4_gpio_table[] = {
	GPIO_CFG(VILLE_GPIO_CAM_I2C_DAT, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(VILLE_GPIO_CAM_I2C_CLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi4_gpio_table_gpio[] = {
	GPIO_CFG(VILLE_GPIO_CAM_I2C_DAT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(VILLE_GPIO_CAM_I2C_CLK, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi5_gpio_table[] = {
	GPIO_CFG(VILLE_GPIO_CAP_I2C_DAT, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(VILLE_GPIO_CAP_I2C_CLK, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi5_gpio_table_gpio[] = {
	GPIO_CFG(VILLE_GPIO_CAP_I2C_DAT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(VILLE_GPIO_CAP_I2C_CLK, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi8_gpio_table[] = {
	GPIO_CFG(VILLE_GPIO_MC_I2C_DAT, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(VILLE_GPIO_MC_I2C_CLK, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi8_gpio_table_gpio[] = {
	GPIO_CFG(VILLE_GPIO_MC_I2C_DAT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(VILLE_GPIO_MC_I2C_CLK, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi12_gpio_table[] = {
	GPIO_CFG(VILLE_GPIO_SR_I2C_DAT, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(VILLE_GPIO_SR_I2C_CLK, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static uint32_t gsbi12_gpio_table_gpio[] = {
	GPIO_CFG(VILLE_GPIO_SR_I2C_DAT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(VILLE_GPIO_SR_I2C_CLK, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
};

static void gsbi_qup_i2c_gpio_config(int adap_id, int config_type)
{
	printk(KERN_INFO "%s(): adap_id = %d, config_type = %d \n", __func__, adap_id, config_type);

	if ((adap_id == MSM_8960_GSBI3_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi3_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi3_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI3_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi3_gpio_table_gpio[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi3_gpio_table_gpio[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI4_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi4_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi4_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI4_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi4_gpio_table_gpio[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi4_gpio_table_gpio[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI5_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi5_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi5_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI5_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi5_gpio_table_gpio[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi5_gpio_table_gpio[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI8_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi8_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi8_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI8_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi8_gpio_table_gpio[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi8_gpio_table_gpio[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI12_QUP_I2C_BUS_ID) && (config_type == 1)) {
		gpio_tlmm_config(gsbi12_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi12_gpio_table[1], GPIO_CFG_ENABLE);
	}

	if ((adap_id == MSM_8960_GSBI12_QUP_I2C_BUS_ID) && (config_type == 0)) {
		gpio_tlmm_config(gsbi12_gpio_table_gpio[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi12_gpio_table_gpio[1], GPIO_CFG_ENABLE);
	}
}

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi4_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi3_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi8_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
	.share_uart_flag = 1,	
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi12_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm8960_i2c_qup_gsbi5_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static uint32_t gyro_DIAG_PIN_pull_down[] = {
	GPIO_CFG(VILLE_GPIO_GYRO_DIAG, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};
static uint32_t gyro_DIAG_PIN_no_pull[] = {
	GPIO_CFG(VILLE_GPIO_GYRO_DIAG, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};
void config_VILLE_gyro_diag_gpios(bool pulldown)
{
	if (pulldown) {
		config_gpio_table(gyro_DIAG_PIN_pull_down, ARRAY_SIZE(gyro_DIAG_PIN_pull_down));
		printk(KERN_INFO "%s %d pull down\n",  __func__, VILLE_GPIO_GYRO_DIAG);
	} else {
		config_gpio_table(gyro_DIAG_PIN_no_pull, ARRAY_SIZE(gyro_DIAG_PIN_no_pull));
		printk(KERN_INFO "%s %d input none pull\n",  __func__, VILLE_GPIO_GYRO_DIAG);
	}
}

static struct pana_gyro_platform_data pana_gyro_pdata = {
	.acc_dir = 0x06,
	.acc_polarity = 0x07,
	.gyro_dir = 0x12,
	.gyro_polarity = 0x05,
	.mag_dir = 0x06,
	.mag_polarity = 0x07,
	.sleep_pin = VILLE_GPIO_PANA_GYRO_SLEEP,
	.config_gyro_diag_gpios = config_VILLE_gyro_diag_gpios,
	 
};

static struct bma250_platform_data gsensor_bma250_platform_data = {
	.intr = VILLE_GPIO_GSENSOR_INT,
	.chip_layout = 1,
};

static struct akm8975_platform_data compass_platform_data = {
	.layouts = VILLE_LAYOUTS,
	.use_pana_gyro = 1,
};

static struct i2c_board_info __initdata msm_i2c_sensor_gsbi12_info[] = {
	{
		I2C_BOARD_INFO(BMA250_I2C_NAME, 0x30 >> 1),
		.platform_data = &gsensor_bma250_platform_data,
		.irq = MSM_GPIO_TO_INT(VILLE_GPIO_GSENSOR_INT),
	},
	{
		I2C_BOARD_INFO(AKM8975_I2C_NAME, 0x1A >> 1),
		.platform_data = &compass_platform_data,
		.irq = MSM_GPIO_TO_INT(VILLE_GPIO_COMPASS_INT),
	},
	{
		I2C_BOARD_INFO("ewtzmu2", 0xD2 >> 1),
		.irq = MSM_GPIO_TO_INT(VILLE_GPIO_GYRO_INT),
		.platform_data = &pana_gyro_pdata,
	},
};





static struct tsens_platform_data msm_tsens_pdata  = {
		.slope			= {910, 910, 910, 910, 910},
		.tsens_factor		= 1000,
		.hw_type		= MSM_8960,
		.patherm0		= ADC_MPP_1_AMUX3,
		.patherm1		= -1,
		.tsens_num_sensor	= 5,
};

static struct platform_device msm_tsens_device = {
	.name   = "tsens8960-tm",
	.id = -1,
};

static struct msm_thermal_data msm_thermal_pdata = {
        .sensor_id = 0,
        .poll_ms = 1000,
        .limit_temp = 60,
        .temp_hysteresis = 10,
        .limit_freq = 918000,
};

#ifdef CONFIG_MSM_FAKE_BATTERY
static struct platform_device fish_battery_device = {
	.name = "fish_battery",
};
#endif

static struct platform_device scm_memchk_device = {
	.name		= "scm-memchk",
	.id		= -1,
};

#define MSM_RAM_CONSOLE_BASE   MSM_HTC_RAM_CONSOLE_PHYS
#define MSM_RAM_CONSOLE_SIZE   MSM_HTC_RAM_CONSOLE_SIZE

static struct resource ram_console_resources[] = {
	{
		.start	= MSM_RAM_CONSOLE_BASE,
		.end	= MSM_RAM_CONSOLE_BASE + MSM_RAM_CONSOLE_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device ram_console_device = {
	.name		= "ram_console",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(ram_console_resources),
	.resource	= ram_console_resources,
};

static struct pm8xxx_vibrator_platform_data pm8xxx_vib_pdata = {
	.initial_vibrate_ms = 0,
	.max_timeout_ms = 15000,
	.level_mV = 2500,
};

static struct ramdump_platform_data ville_ramdump_data = {
	.count = 0,
	.region = {
		{
			.start  = 0,
			.size   = 0,
		},
	}
};

struct platform_device device_htc_ramdump = {
	.name           = "htc_ramdump",
	.id             = 0,
	.dev = {.platform_data = &ville_ramdump_data},
};

static struct platform_device *common_devices[] __initdata = {
	&ram_console_device,
	&msm8960_device_acpuclk,
	&msm8960_device_dmov,
	&msm_device_smd,
	&msm8960_device_uart_gsbi8,
	&msm_device_uart_dm6,
	&msm_device_saw_core0,
	&msm_device_saw_core1,
	&msm8960_device_ext_5v_vreg,
	&msm8960_device_ssbi_pmic,
	&msm8960_device_qup_spi_gsbi10,
	&msm8960_device_qup_i2c_gsbi3,
	&msm8960_device_uart_gsbi3,
	&msm8960_device_qup_i2c_gsbi4,
	&msm8960_device_qup_i2c_gsbi8,
	&msm8960_device_qup_i2c_gsbi5,
#ifndef CONFIG_MSM_DSPS
	&msm8960_device_qup_i2c_gsbi12,
#endif
	&msm_slim_ctrl,
	&msm_device_wcnss_wlan,
#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)
	&qcrypto_device,
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)
	&qcedev_device,
#endif
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
	&msm_device_sps,
#ifdef CONFIG_MSM_FAKE_BATTERY
	&fish_battery_device,
#endif
	&apq8064_fmem_device,
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
#endif
#endif
	&msm_device_vidc,
	&msm_device_bam_dmux,
	&msm_fm_platform_init,

#ifdef CONFIG_HW_RANDOM_MSM
	&msm_device_rng,
#endif
	&msm8960_rpm_device,
#ifdef CONFIG_ION_MSM
	&m7_ion_dev,
#endif
#ifdef CONFIG_QSEECOM
	&qseecom_device,
#endif
#ifdef CONFIG_MSM_IOMMU
	&msm8960_iommu_domain_device,
#endif
	&msm8960_rpm_log_device,
	&msm8960_rpm_stat_device,
#ifdef CONFIG_MSM_QDSS
	&msm_etb_device,
	&msm_tpiu_device,
	&msm_funnel_device,
	&msm_etm_device,
#endif
	&msm8960_device_watchdog,
#ifdef CONFIG_MSM_RTB
	&m7_rtb_device,
#endif
#ifdef CONFIG_MSM_CACHE_ERP
	&msm8960_device_cache_erp,
#endif
#ifdef CONFIG_MSM_CACHE_DUMP
	&msm_cache_dump_device,
#endif
#ifdef CONFIG_HTC_BATT_8960
	&htc_battery_pdev,
#endif
	&msm_tsens_device,
#ifdef CONFIG_PERFLOCK
	&msm8960_device_perf_lock,
#endif
};

static struct platform_device *ville_devices[] __initdata = {
	&msm_8960_q6_lpass,
	&msm_8960_q6_mss_fw,
	&msm_8960_q6_mss_sw,
	&msm_8960_riva,
	&msm_pil_tzapps,
	&msm8960_device_otg,
	&msm_device_hsusb_host,
	&msm_pcm,
	&msm_multi_ch_pcm,
	&msm_pcm_routing,
	&msm_cpudai0,
	&msm_cpudai1,
	&msm8960_cpudai_slimbus_2_rx,
	&msm8960_cpudai_slimbus_2_tx,
	&msm_cpudai_hdmi_rx,
	&msm_cpudai_bt_rx,
	&msm_cpudai_bt_tx,
	&msm_cpudai_fm_rx,
	&msm_cpudai_fm_tx,
	&msm_cpudai_auxpcm_rx,
	&msm_cpudai_auxpcm_tx,
	&msm_cpu_fe,
	&msm_stub_codec,
#ifdef CONFIG_MSM_GEMINI
	&msm8960_gemini_device,
#endif
	&msm_voice,
	&msm_voip,
	&msm_lpa_pcm,
	&msm_cpudai_afe_01_rx,
	&msm_cpudai_afe_01_tx,
	&msm_cpudai_afe_02_rx,
	&msm_cpudai_afe_02_tx,
	&msm_pcm_afe,
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
	&hdmi_msm_device,
#endif
	&msm_compr_dsp,
	&msm_cpudai_incall_music_rx,
	&msm_cpudai_incall_record_rx,
	&msm_cpudai_incall_record_tx,
	&msm_pcm_hostless,
	&msm_bus_apps_fabric,
	&msm_bus_sys_fabric,
	&msm_bus_mm_fabric,
	&msm_bus_sys_fpb,
	&msm_bus_cpss_fpb,
	&msm_device_tz_log,
	&scm_memchk_device,
};

static void __init msm8960_i2c_init(void)
{
	msm8960_device_qup_i2c_gsbi4.dev.platform_data =
					&msm8960_i2c_qup_gsbi4_pdata;

	msm8960_device_qup_i2c_gsbi3.dev.platform_data =
					&msm8960_i2c_qup_gsbi3_pdata;

	msm8960_device_qup_i2c_gsbi8.dev.platform_data =
					&msm8960_i2c_qup_gsbi8_pdata;

	msm8960_device_qup_i2c_gsbi12.dev.platform_data =
					&msm8960_i2c_qup_gsbi12_pdata;

	msm8960_device_qup_i2c_gsbi5.dev.platform_data =
					&msm8960_i2c_qup_gsbi5_pdata;
}

static void __init msm8960_gfx_init(void)
{
	struct kgsl_device_platform_data *kgsl_3d0_pdata =
		msm_kgsl_3d0.dev.platform_data;
	uint32_t soc_platform_version = socinfo_get_version();

	
	if (cpu_is_msm8960ab()) {
		kgsl_3d0_pdata->chipid = ADRENO_CHIPID(3, 2, 1, 0);
		
		kgsl_3d0_pdata->pwrlevel[1].gpu_freq = 320000000;
	} else {
		kgsl_3d0_pdata->iommu_count = 1;
		if (SOCINFO_VERSION_MAJOR(soc_platform_version) == 1) {
			kgsl_3d0_pdata->pwrlevel[0].gpu_freq = 320000000;
			kgsl_3d0_pdata->pwrlevel[1].gpu_freq = 266667000;
		}
		if (SOCINFO_VERSION_MAJOR(soc_platform_version) >= 3) {
			kgsl_3d0_pdata->chipid = ADRENO_CHIPID(2, 2, 0, 6);
		}
	}

	
	platform_device_register(&msm_kgsl_3d0);

	
	if (!cpu_is_msm8960ab()) {
		platform_device_register(&msm_kgsl_2d0);
		platform_device_register(&msm_kgsl_2d1);
	}
}



static struct pm8xxx_irq_platform_data pm8xxx_irq_pdata __devinitdata = {
	.irq_base		= PM8921_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(104),
	.irq_trigger_flag	= IRQF_TRIGGER_LOW,
};

static struct pm8xxx_gpio_platform_data pm8xxx_gpio_pdata __devinitdata = {
	.gpio_base	= PM8921_GPIO_PM_TO_SYS(1),
};

static struct pm8xxx_mpp_platform_data pm8xxx_mpp_pdata __devinitdata = {
	.mpp_base	= PM8921_MPP_PM_TO_SYS(1),
};

static struct pm8xxx_rtc_platform_data pm8xxx_rtc_pdata __devinitdata = {
	.rtc_write_enable       = true,
#ifdef CONFIG_HTC_OFFMODE_ALARM
	.rtc_alarm_powerup      = true,
#else
	.rtc_alarm_powerup      = false,
#endif
};

static struct pm8xxx_pwrkey_platform_data pm8xxx_pwrkey_pdata = {
	.pull_up		= 1,
	.kpd_trigger_delay_us	= 970,
	.wakeup			= 1,
};

static struct pm8xxx_misc_platform_data pm8xxx_misc_pdata = {
	.priority		= 0,
};



static struct pm8xxx_gpio_init green_gpios[] = {
	PM8XXX_GPIO_OUTPUT_VIN_L17_LED_FUNC(VILLE_PMGPIO_GREEN_LED, 1, PM_GPIO_FUNC_2),
	PM8XXX_GPIO_OUTPUT_VIN_L17_LED_FUNC(VILLE_PMGPIO_GREEN_LED, 1, PM_GPIO_FUNC_NORMAL),
};

static struct pm8xxx_gpio_init amber_gpios[] = {
	PM8XXX_GPIO_OUTPUT_VIN_L17_LED_FUNC(VILLE_PMGPIO_AMBER_LED, 1, PM_GPIO_FUNC_2),
	PM8XXX_GPIO_OUTPUT_VIN_L17_LED_FUNC(VILLE_PMGPIO_AMBER_LED, 1, PM_GPIO_FUNC_NORMAL),
};

static void green_gpio_config(bool enable)
{
	if (enable)
		pm8xxx_gpio_config(green_gpios[0].gpio, &green_gpios[0].config);
	else
		pm8xxx_gpio_config(green_gpios[1].gpio, &green_gpios[1].config);
}

static void amber_gpio_config(bool enable)
{
	if (enable)
		pm8xxx_gpio_config(amber_gpios[0].gpio, &amber_gpios[0].config);
	else
		pm8xxx_gpio_config(amber_gpios[1].gpio, &amber_gpios[1].config);
}

static struct pm8xxx_led_configure pm8921_led_info[] = {
	[0] = {
		.name		= "button-backlight",
		.flags		= PM8XXX_ID_LED_0,
		.function_flags = LED_PWM_FUNCTION | LED_BRETH_FUNCTION,
		.period_us 	= USEC_PER_SEC / 1000,
		.start_index 	= 0,
		.duites_size 	= 8,
		.duty_time_ms 	= 64,
		.lut_flag 	= PM_PWM_LUT_RAMP_UP | PM_PWM_LUT_PAUSE_HI_EN,
		.out_current    = 40,
		.duties		= {0, 9, 18, 27, 36, 45, 54, 60,
				60, 54, 45, 36, 27, 18, 9, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0},
	},
	[1] = {
		.name           = "green",
		.flags		= PM8XXX_ID_GPIO24,
		.function_flags = LED_PWM_FUNCTION | LED_BLINK_FUNCTION,
		.gpio_status_switch = green_gpio_config,
		.pwm_coefficient 	= 40,
	},
	[2] = {
		.name           = "amber",
		.flags		= PM8XXX_ID_GPIO25,
		.function_flags = LED_PWM_FUNCTION | LED_BLINK_FUNCTION,
		.gpio_status_switch = amber_gpio_config,
	},
};

static struct pm8xxx_led_platform_data pm8xxx_leds_pdata = {
	.num_leds = ARRAY_SIZE(pm8921_led_info),
	.leds = pm8921_led_info,
};

static struct pm8xxx_ccadc_platform_data pm8xxx_ccadc_pdata = {
	.r_sense		= 10,
	.calib_delay_ms	= 600000,
};

static struct pm8921_platform_data pm8921_platform_data __devinitdata = {
	.irq_pdata		= &pm8xxx_irq_pdata,
	.gpio_pdata		= &pm8xxx_gpio_pdata,
	.mpp_pdata		= &pm8xxx_mpp_pdata,
	.rtc_pdata              = &pm8xxx_rtc_pdata,
	.pwrkey_pdata		= &pm8xxx_pwrkey_pdata,
	.misc_pdata		= &pm8xxx_misc_pdata,
	.regulator_pdatas	= msm_pm8921_regulator_pdata,
	.charger_pdata		= &pm8921_chg_pdata,
	.bms_pdata		= &pm8921_bms_pdata,
	.adc_pdata		= &pm8xxx_adc_pdata,
	.leds_pdata		= &pm8xxx_leds_pdata,
	.ccadc_pdata		= &pm8xxx_ccadc_pdata,
	.vibrator_pdata         = &pm8xxx_vib_pdata,
};

static struct msm_ssbi_platform_data msm8960_ssbi_pm8921_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name			= "pm8921-core",
		.platform_data		= &pm8921_platform_data,
	},
};





static uint32_t msm_rpm_get_swfi_latency(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(msm_rpmrs_levels); i++) {
		if (msm_rpmrs_levels[i].sleep_mode ==
			MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)
			return msm_rpmrs_levels[i].latency_us;
	}

	return 0;
}

static struct cm3629_platform_data cm36282_TMO_EN1_pdata = {
	.model = CAPELLA_CM36282,
	.ps_select = CM3629_PS1_ONLY,
	.intr = PM8921_GPIO_PM_TO_SYS(VILLE_PMGPIO_PROXIMITY_INTz),
	.levels = { 8, 10, 12, 19, 283, 3094, 5313, 7847, 10383, 65535},
	.golden_adc = 3857,
	.power = NULL,
	.cm3629_slave_address = 0xC0>>1,
	.ps1_thd_set = 0x05,
	.ps1_thd_no_cal = 0xF1,
	.ps1_thd_with_cal = 0x05,
	.ps_calibration_rule = 1,
	.ps_conf1_val = CM3629_PS_DR_1_320 | CM3629_PS_IT_1_6T |
			CM3629_PS1_PERS_1,
	.ps_conf2_val = CM3629_PS_ITB_1 | CM3629_PS_ITR_1 |
			CM3629_PS2_INT_DIS | CM3629_PS1_INT_DIS,
	.ps_conf3_val = CM3629_PS2_PROL_32,
	.dark_level = 3,
	.no_need_change_setting = 1,
};
static struct i2c_board_info i2c_CM36282_TMO_EN1_devices[] = {
	{
		I2C_BOARD_INFO(CM3629_I2C_NAME, 0xC0 >> 1),
		.platform_data = &cm36282_TMO_EN1_pdata,
		.irq =  PM8921_GPIO_IRQ(PM8921_IRQ_BASE, VILLE_PMGPIO_PROXIMITY_INTz),
	},
};

static struct cm3629_platform_data cm36282_TMO_pdata = {
	.model = CAPELLA_CM36282,
	.ps_select = CM3629_PS1_ONLY,
	.intr = PM8921_GPIO_PM_TO_SYS(VILLE_PMGPIO_PROXIMITY_INTz),
	.levels = { 8, 10, 12, 19, 283, 3094, 5313, 7847, 10383, 65535},
	.golden_adc = 3857,
	.power = NULL,
	.cm3629_slave_address = 0xC0>>1,
	.ps1_thd_set = 0x05,
	.ps1_thd_no_cal = 0xF1,
	.ps1_thd_with_cal = 0x05,
	.ps_calibration_rule = 1,
	.ps_conf1_val = CM3629_PS_DR_1_80 | CM3629_PS_IT_2T |
			CM3629_PS1_PERS_1,
	.ps_conf2_val = CM3629_PS_ITB_2 | CM3629_PS_ITR_1 |
			CM3629_PS2_INT_DIS | CM3629_PS1_INT_DIS,
	.ps_conf3_val = CM3629_PS2_PROL_32,
};

static struct i2c_board_info i2c_CM36282_TMO_devices[] = {
	{
		I2C_BOARD_INFO(CM3629_I2C_NAME, 0xC0 >> 1),
		.platform_data = &cm36282_TMO_pdata,
		.irq =  PM8921_GPIO_IRQ(PM8921_IRQ_BASE, VILLE_PMGPIO_PROXIMITY_INTz),
	},
};
static struct cm3629_platform_data cm36282_XD_EN1_pdata = {
	.model = CAPELLA_CM36282,
	.ps_select = CM3629_PS1_ONLY,
	.intr = PM8921_GPIO_PM_TO_SYS(VILLE_PMGPIO_PROXIMITY_INTz),
	.levels = { 8, 10, 17, 134, 257, 2827, 4779, 6989, 9198, 65535},
	.golden_adc = 3490,
	.power = NULL,
	.cm3629_slave_address = 0xC0>>1,
	.ps1_thd_set = 0x05,
	.ps1_thd_no_cal = 0xF1,
	.ps1_thd_with_cal = 0x05,
	.ps_calibration_rule = 1,
	.ps_conf1_val = CM3629_PS_DR_1_320 | CM3629_PS_IT_1_6T |
			CM3629_PS1_PERS_1,
	.ps_conf2_val = CM3629_PS_ITB_1 | CM3629_PS_ITR_1 |
			CM3629_PS2_INT_DIS | CM3629_PS1_INT_DIS,
	.ps_conf3_val = CM3629_PS2_PROL_32,
	.no_need_change_setting = 1,
};

static struct i2c_board_info i2c_CM36282_XD_EN1_devices[] = {
	{
		I2C_BOARD_INFO(CM3629_I2C_NAME, 0xC0 >> 1),
		.platform_data = &cm36282_XD_EN1_pdata,
		.irq =  PM8921_GPIO_IRQ(PM8921_IRQ_BASE, VILLE_PMGPIO_PROXIMITY_INTz),
	},
};

static struct cm3629_platform_data cm36282_XD_pdata = {
	.model = CAPELLA_CM36282,
	.ps_select = CM3629_PS1_ONLY,
	.intr = PM8921_GPIO_PM_TO_SYS(VILLE_PMGPIO_PROXIMITY_INTz),
	.levels = { 8, 10, 17, 134, 257, 2827, 4779, 6989, 9198, 65535},
	.golden_adc = 3490,
	.power = NULL,
	.cm3629_slave_address = 0xC0>>1,
	.ps1_thd_set = 0x05,
	.ps1_thd_no_cal = 0xF1,
	.ps1_thd_with_cal = 0x05,
	.ps_calibration_rule = 1,
	.ps_conf1_val = CM3629_PS_DR_1_80 | CM3629_PS_IT_2T |
			CM3629_PS1_PERS_1,
	.ps_conf2_val = CM3629_PS_ITB_2 | CM3629_PS_ITR_1 |
			CM3629_PS2_INT_DIS | CM3629_PS1_INT_DIS,
	.ps_conf3_val = CM3629_PS2_PROL_32,
};

static struct i2c_board_info i2c_CM36282_XD_devices[] = {
	{
		I2C_BOARD_INFO(CM3629_I2C_NAME, 0xC0 >> 1),
		.platform_data = &cm36282_XD_pdata,
		.irq =  PM8921_GPIO_IRQ(PM8921_IRQ_BASE, VILLE_PMGPIO_PROXIMITY_INTz),
	},
};

static struct cm3629_platform_data cm36282_pdata = {
	.model = CAPELLA_CM36282,
	.ps_select = CM3629_PS1_ONLY,
	.intr = PM8921_GPIO_PM_TO_SYS(VILLE_PMGPIO_PROXIMITY_INTz),
	.levels = { 8, 10, 33, 259, 516, 4881, 8411, 13023, 23251, 65535},
	.golden_adc = 5573,
	.power = NULL,
	.cm3629_slave_address = 0xC0>>1,
	.ps1_thd_set = 0x05,
	.ps1_thd_no_cal = 0xF1,
	.ps1_thd_with_cal = 0x05,
	.ps_calibration_rule = 1,
	.ps_conf1_val = CM3629_PS_DR_1_80 | CM3629_PS_IT_2T |
			CM3629_PS1_PERS_1,
	.ps_conf2_val = CM3629_PS_ITB_2 | CM3629_PS_ITR_1 |
			CM3629_PS2_INT_DIS | CM3629_PS1_INT_DIS,
	.ps_conf3_val = CM3629_PS2_PROL_32,
};

static struct i2c_board_info i2c_CM36282_devices[] = {
	{
		I2C_BOARD_INFO(CM3629_I2C_NAME, 0xC0 >> 1),
		.platform_data = &cm36282_pdata,
		.irq =  PM8921_GPIO_IRQ(PM8921_IRQ_BASE, VILLE_PMGPIO_PROXIMITY_INTz),
	},
};

#ifdef CONFIG_I2C
#define I2C_SURF 1
#define I2C_FFA  (1 << 1)
#define I2C_RUMI (1 << 2)
#define I2C_SIM  (1 << 3)
#define I2C_FLUID (1 << 4)
#define I2C_LIQUID (1 << 5)

struct i2c_registry {
	u8                     machs;
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};


static struct i2c_registry msm8960_i2c_devices[] __initdata = {
	{
		I2C_SURF | I2C_FFA,
		MSM_8960_GSBI12_QUP_I2C_BUS_ID,
		msm_i2c_sensor_gsbi12_info,
		ARRAY_SIZE(msm_i2c_sensor_gsbi12_info),
	},
#ifdef CONFIG_FB_MSM_HDMI_MHL
#ifdef CONFIG_FB_MSM_HDMI_MHL_SII9234
	{
		I2C_SURF | I2C_FFA,
		MSM_8960_GSBI8_QUP_I2C_BUS_ID,
		msm_i2c_gsbi8_mhl_sii9234_info,
		ARRAY_SIZE(msm_i2c_gsbi8_mhl_sii9234_info),
	},
#endif
#endif
#ifdef CONFIG_FLASHLIGHT_TPS61310
	{
		I2C_SURF | I2C_FFA,
		MSM_8960_GSBI12_QUP_I2C_BUS_ID,
		i2c_tps61310_flashlight,
		ARRAY_SIZE(i2c_tps61310_flashlight),
	},
#endif
	{
		I2C_SURF | I2C_FFA,
		MSM_8960_GSBI3_QUP_I2C_BUS_ID,
		msm_i2c_gsbi3_info,
		ARRAY_SIZE(msm_i2c_gsbi3_info),
	},
	{
		I2C_SURF | I2C_FFA,
		MSM_8960_GSBI5_QUP_I2C_BUS_ID,
		msm_i2c_gsbi5_info,
		ARRAY_SIZE(msm_i2c_gsbi5_info),
	},
};
#endif 

static void __init register_i2c_devices(void)
{
#ifdef CONFIG_I2C
	u8 mach_mask = 0;
	int i;

#ifdef CONFIG_MSM_CAMERA
	struct i2c_registry msm_camera_i2c_devices = {
		I2C_SURF | I2C_FFA | I2C_LIQUID | I2C_RUMI,
		MSM_8960_GSBI4_QUP_I2C_BUS_ID,
		ville_camera_board_info.board_info,
		ville_camera_board_info.num_i2c_board_info,
	};
#endif 

	mach_mask = I2C_SURF;

	if (1 == board_mfg_mode())
		if (cs_cy8c_data[1].id.config == CS_KEY_4)
			cs_cy8c_data[1].keycode[3] = KEY_MENU;

	if (system_rev < 2) {
		if (cs_cy8c_data[1].id.config == CS_KEY_4) {
			pr_info("[cap] Setting as old printing\n");
			cs_cy8c_data[1].keycode[0] = KEY_HOME;
			cs_cy8c_data[1].keycode[1] = KEY_MENU;
			cs_cy8c_data[1].keycode[2] = KEY_BACK;
			cs_cy8c_data[1].keycode[3] = KEY_SEARCH;
		}
	}
	
	for (i = 0; i < ARRAY_SIZE(msm8960_i2c_devices); ++i) {
		if (msm8960_i2c_devices[i].machs & mach_mask) {
			i2c_register_board_info(msm8960_i2c_devices[i].bus,
						msm8960_i2c_devices[i].info,
						msm8960_i2c_devices[i].len);
		}
	}

	if (system_rev < 3) {
		i2c_register_board_info(MSM_8960_GSBI12_QUP_I2C_BUS_ID,
			i2c_CM36282_devices, ARRAY_SIZE(i2c_CM36282_devices));
		pr_info("%s: cm36282 PL-sensor for XA,XB,XC, system_rev %d ",
				 __func__, system_rev);
	} else {
		if (skuid & 0x1) {
			if (engineerid  & 0x1) {
				i2c_register_board_info(MSM_8960_GSBI12_QUP_I2C_BUS_ID,
				i2c_CM36282_TMO_EN1_devices, ARRAY_SIZE(i2c_CM36282_TMO_EN1_devices));
				pr_info("%s: cm36282 PL-sensor TMO EN1 system_rev %d, sku %x ",
					 __func__, system_rev, skuid);
			} else {
				i2c_register_board_info(MSM_8960_GSBI12_QUP_I2C_BUS_ID,
					i2c_CM36282_TMO_devices, ARRAY_SIZE(i2c_CM36282_TMO_devices));
				pr_info("%s: cm36282 PL-sensor TMO system_rev %d, sku %x ",
					 __func__, system_rev, skuid);
			}
		} else {
			if (engineerid  & 0x1) {
				i2c_register_board_info(MSM_8960_GSBI12_QUP_I2C_BUS_ID,
					i2c_CM36282_XD_EN1_devices, ARRAY_SIZE(i2c_CM36282_XD_EN1_devices));
				pr_info("%s: cm36282 PL-sensor for XD and newer HW version EN1, system_rev %d ",
					__func__, system_rev);
			} else {
				i2c_register_board_info(MSM_8960_GSBI12_QUP_I2C_BUS_ID,
					i2c_CM36282_XD_devices,	ARRAY_SIZE(i2c_CM36282_XD_devices));
				pr_info("%s: cm36282 PL-sensor for XD and newer HW version, system_rev %d ",
					__func__, system_rev);
			}
		}
	}
#ifdef CONFIG_MSM_CAMERA
	if (msm_camera_i2c_devices.machs & mach_mask)
		i2c_register_board_info(msm_camera_i2c_devices.bus,
			msm_camera_i2c_devices.info,
			msm_camera_i2c_devices.len);
#endif 
#endif
}

static uint32_t msm_uart_gpio[] = {
	GPIO_CFG(34, 1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
	GPIO_CFG(35, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA),
};

static void msm_uart_gsbi_gpio_init(void)
{
	gpio_tlmm_config(msm_uart_gpio[0], GPIO_CFG_ENABLE);
	gpio_tlmm_config(msm_uart_gpio[1], GPIO_CFG_ENABLE);
}

static uint32_t msm_region_gpio[] = {
	GPIO_CFG(VILLE_GPIO_REGION_ID, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, 0),
};
static void msm_region_id_gpio_init(void)
{
	gpio_tlmm_config(msm_region_gpio[0], GPIO_CFG_ENABLE);
}

static void __init m7_allocate_memory_regions(void)
{
	ville_allocate_fb_regions();
}

static void __init ville_init(void)
{
	if (meminfo_init(SYS_MEMORY, SZ_256M) < 0)
		pr_err("meminfo_init() failed!\n");

	msm_rotator_set_split_iommu_domain();
	msm_rotator_update_bus_vectors(1920, 1080);
	msm_tsens_early_init(&msm_tsens_pdata);
        msm_thermal_init(&msm_thermal_pdata);
	BUG_ON(msm_rpm_init(&msm8960_rpm_data));
	BUG_ON(msm_rpmrs_levels_init(&msm_rpmrs_data));
	msm_rpmrs_lpm_init(1, 1, 1, 1);

	pmic_reset_irq = PM8921_IRQ_BASE + PM8921_RESOUT_IRQ;
	regulator_suppress_info_printing();
	if (msm_xo_init())
		pr_err("Failed to initialize XO votes\n");
	platform_device_register(&msm8960_device_rpm_regulator);
	clk_ignor_list_add("msm_serial_hsl.0", "core_clk", &msm8960_clock_init_data);
	msm_clock_init(&msm8960_clock_init_data);

	android_usb_pdata.swfi_latency = msm_rpm_get_swfi_latency();

	msm8960_device_otg.dev.platform_data = &msm_otg_pdata;
	ville_gpiomux_init();
	msm8960_device_qup_spi_gsbi10.dev.platform_data =
				&msm8960_qup_spi_gsbi10_pdata;
#ifdef CONFIG_RAWCHIP
	spi_register_board_info(rawchip_spi_board_info,
				ARRAY_SIZE(rawchip_spi_board_info));
#endif
	msm8960_device_ssbi_pmic.dev.platform_data =
				&msm8960_ssbi_pm8921_pdata;
	pm8921_platform_data.num_regulators = msm_pm8921_regulator_pdata_len;
	msm8960_i2c_init();
	msm8960_gfx_init();

	ville_cable_detect_register();

	msm_spm_init(msm_spm_data, ARRAY_SIZE(msm_spm_data));
	msm_spm_l2_init(msm_spm_l2_data);
	msm8960_init_buses();
#ifdef CONFIG_BT
	bt_export_bd_address();
#endif
#ifdef CONFIG_HTC_BATT_8960
	htc_battery_cell_init(htc_battery_cells, ARRAY_SIZE(htc_battery_cells));
#endif 

	platform_add_devices(msm8960_footswitch,
		msm8960_num_footswitch);

	if(board_mfg_mode() == MFG_MODE_MINI) {
		platform_device_register(&device_htc_ramdump);
	}

	platform_device_register(&msm8960_device_ext_l2_vreg);
	platform_add_devices(common_devices, ARRAY_SIZE(common_devices));
	msm_uart_gsbi_gpio_init();
	pm8921_gpio_mpp_init();
	msm_region_id_gpio_init();
	platform_add_devices(ville_devices, ARRAY_SIZE(ville_devices));
#ifdef CONFIG_MSM_CAMERA
	msm8960_init_cam();
#endif
	msm8960_init_mmc();

	register_i2c_devices();
	
	if(board_mfg_mode() != MFG_MODE_MINI) {
		msm8960_init_fb();
	}

	slim_register_board_info(msm_slim_devices,
		ARRAY_SIZE(msm_slim_devices));

	change_memory_power = &msm8960_change_memory_power;

#ifdef CONFIG_CPU_FREQ_GOV_ONDEMAND_2_PHASE
	if(!cpu_is_krait_v1())
		set_two_phase_freq(1134000);
#endif
	set_input_event_min_freq_by_cpu(1, 1134000);
	set_input_event_min_freq_by_cpu(2, 1026000);
	
	if (!(board_mfg_mode() == 6 || board_mfg_mode() == 7))
		ville_add_usb_devices();

	ville_init_keypad();

	BUG_ON(msm_pm_boot_init(&msm_pm_boot_pdata));
	if ((get_kernel_flag() & KERNEL_FLAG_PM_MONITOR) ||
		(!(get_kernel_flag() & KERNEL_FLAG_TEST_PWR_SUPPLY) && (!get_tamper_sf()))) {
		htc_monitor_init();
		htc_pm_monitor_init();
	}

	msm_pm_init_sleep_status_data(&msm_pm_slp_sts_data);
	msm_pm_radio_info_init(MSM_SHARED_RAM_BASE + 0x1F0000);
}

#define PHY_BASE_ADDR1  0x80400000
#define SIZE_ADDR1      (132 * 1024 * 1024)

#define PHY_BASE_ADDR2  0x90000000
#define SIZE_ADDR2      (768 * 1024 * 1024)

#define EBICS0_RAMDUMP_START  0x80000000

static void __init ville_fixup(struct tag *tags, char **cmdline, struct meminfo *mi)
{
	mem_size_mb = parse_tag_memsize((const struct tag *)tags);
	printk(KERN_DEBUG "%s: mem_size_mb=%u\n, mfg_mode = %d", __func__, mem_size_mb, board_mfg_mode());

	mi->nr_banks = 2;
	mi->bank[0].start = PHY_BASE_ADDR1;
	mi->bank[0].size = SIZE_ADDR1;
	mi->bank[1].start = PHY_BASE_ADDR2;
	mi->bank[1].size = SIZE_ADDR2;


	if (mem_size_mb == 64) {
		printk(KERN_INFO "%s: setup memory(%uMB) for minikernel dump\n", __func__, mem_size_mb);
		mi->nr_banks = 1;
		mi->bank[0].start = PHY_BASE_ADDR1;
		mi->bank[0].size = (64 * 1024 * 1024);

		ville_ramdump_data.count = 1;
		ville_ramdump_data.region[0].start = mi->bank[0].start + mi->bank[0].size;
		ville_ramdump_data.region[0].size = (SZ_1G
			- (ville_ramdump_data.region[0].start - EBICS0_RAMDUMP_START));
		printk(KERN_INFO "%s: setup ramdump_data start = %lx ,size = %lx count=%d\n"
				, __func__
				, ville_ramdump_data.region[0].start, ville_ramdump_data.region[0].size
				, ville_ramdump_data.count);
	}


	skuid = parse_tag_skuid((const struct tag *)tags);
	printk(KERN_INFO "Ville_fixup:skuid=0x%x\n", skuid);
}


MACHINE_START(VILLE, "ville")
	.fixup = ville_fixup,
	.map_io = m7_map_io,
	.reserve = m7_reserve,
	.init_irq = ville_init_irq,
	.handle_irq = gic_handle_irq,
	.timer = &msm_timer,
	.init_machine = ville_init,
	.init_early = m7_allocate_memory_regions,
	.init_very_early = m7_early_reserve,
	.restart = msm_restart,
MACHINE_END
