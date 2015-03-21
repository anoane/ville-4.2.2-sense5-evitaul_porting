make clean
make mrproper
#cp arch/arm/configs/ville_defconfig ./.config
make ville_defconfig
make -j8 ARCH=arm SUBARCH=arm CROSS_COMPILE=/home/fabane/Scrivania/arm-eabi-4.6/bin/arm-eabi-