#gpio set PL10
gpio set PA15
setenv machid 1029
setenv bootargs "console=ttyS0,115200 console=tty1 init=/init panic=10 consoleblank=0 enforcing=0 loglevel=1 hdmi.audio=EDID:0 disp.screen0_output_mode=1280x720p60"
#setenv bootargs "console=ttyS0,115200 console=tty1 init=/init panic=10 consoleblank=0 enforcing=0"

fatload mmc 0 0x43000000 script.bin
#fatload mmc 0 0x48000000 uImage
fatload mmc 0 0x45000000 uImage
#bootm 0x48000000 - 0x46000000
setenv bootm_boot_mode sec
#bootm 0x48000000
bootm 0x45000000


#fatload mmc 0 0x42000000 zImage
#bootz 0x48000000 - 0x45000000
# Recompile with:
# mkimage -C none -A arm -T script -d orangepi.cmd boot.scr
