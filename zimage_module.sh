rm -f /home/fabane/Scrivania/ville_build/*

cp -f arch/arm/boot/zImage /home/fabane/Scrivania/ville_build/zImage

find . -name "*.ko" -exec cp {} /home/fabane/Scrivania/ville_build/ \;