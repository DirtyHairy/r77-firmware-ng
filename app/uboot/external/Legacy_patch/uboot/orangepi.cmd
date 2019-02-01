#gpio set PL10
gpio set PA15
setenv machid 1029
setenv bootm_boot_mode sec
setenv bootargs "console=ttyS0,115200 console=tty1 root=/dev/mmcblk0p2 init=/sbin/init rootwait rootfstype=ext4 panic=10 consoleblank=0 enforcing=0 loglevel=7"


fatload mmc 0 0x46000000 sun8i-h3-orangepi-pc-plus.dtb
fatload mmc 0 0x48000000 uImage
bootm 0x48000000 - 0x46000000


#fatload mmc 0 0x42000000 zImage
#bootz 0x48000000 - 0x45000000
# Recompile with:
# mkimage -C none -A arm -T script -d orangepi.cmd boot.scr
