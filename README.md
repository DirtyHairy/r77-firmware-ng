# What is this?

This is a reworked firmware for the R77 which runs Stella 6.
It is built from the official Hyperkin [source drop](https://www.hyperkin.com/r77) (v0.6.2)
and includes the changes made by Remowilliams that constitute the community firmware image on
[AtariAge](http://atariage.com/forums/topic/281462-retron-77-community-build-image/).

# Installation

Grab a release from the [releases page](https://github.com/DirtyHairy/r77-firmware-ng/releases)
and follow the [installation instructions](./INSTALL.md).

# Features

Stella 6 is a huge improvement over the original version of Stella 3 packaged with the
R77. Among many other things you will get:

 * Near-perfect TIA emulation
 * Cycle exact audio that sounds just like the real thing
 * TV emulation, including scanlines and phosphor effect
 * Smooth video without tearing
 * Support for all cartridges out there, including all ARM-based games

 The firmware contains many other enhancements over the original, many of which are inherited
 from the original community image by Remowilliams (see above). Among other things:

 * ROM dumps can be saved to the SD card
 * The Hyperkin GUI is replaced with Stella's own launcher
 * There is a dedicated "mini" settings UI for easy acces to the most important settings
 * A command palette that can be opened by pressing 4:3/16:9 (16:9 mode is availabe from
   the command palette).
 * UI navigation with the buttons on the console (check out the
   [documentation](https://github.com/stella-emu/stella/blob/master/docs/R77_readme.txt)
   for details)
 * Paddle jitter is much better than with the original firmware
 * Press "Fry" to return to the launcher from a running game.
   Frying is available from the command palette
 * Support for USB controllers (gamepads, joysticks, keyboards). You'll need to use a USB Y
   cable to connect them while powering the device at the same time. Refer to the Stella manual
   for more instructions.
 * Support for the Atarivox connected to a USB-to-serial dongle. Most dongles out there should
   be supported. You need a USB Y cable to connect the dongle, and the dongle needs to be present
   when the console is powered on. Refer to the Stella manual for more instruction.
 * A "development mode" that allows you to connect to the device through a supported USB network
   dongle and access a shell via SSH (see below).

**IMPORTANT:** If there is a file called `sys` or `games` on your SD card, the firmware
will delete it and create a directory in its place.

# Dumping ROMs to SD

The cartridge slot on the R77 is, in fact, a ROM dumper. When you insert a cartridge into the slot,
the R77 dumps the ROM on the cartridge and then runs it in Stella. In order to keep the dumped ROM,
this firmware can be configured to store the dumped ROM files on the SD card.

Dumping the cartridge ROMs to the SD card is enabled by creating a file called `sys/settings` on
the SD card with the content

```
DUMP_TO_SD=y
```

After inserting a cartridge, wait until Stella starts up and plays the game. Now switch off
the console and put the SD card into a card reader. The dumped ROM files are called
`rom-<md5>.bin` where `<md5>` is the MD5 hash of the dumped ROM.

# Known issues

Using USB devices (2600-daptor, USB-to-serial dongles) may cause slowdowns in games that require ARM
emulation (DPC+ / CDF / CDFJ cartridges).

Please be sure to report any issues not mentioned here on AtariAge and / or
open an [issue](https://github.com/DirtyHairy/r77-firmware-ng/issues) on github.

# Disclaimer

**You are using this firmware at your own risk**. None of the developers are liable for any damages to your
R77, your computer or your mental health.

In particular, note that this firmware raises the CPU clock of the R77 from 1GHz to 1.2GHz (which is still
within the specs). No excessive temperature has been observed during testing, but be sure to keep the R77
in a well-ventillated area (not under a stack of books). If the temperature gets to hot, the device should
just throttle the clock and get slower but, again, **you are using this at your own risk**.

You can disable overclocking by setting `DONT_OVERCLOCK` (see below). This may stop consoles with buggy
hardware from crashing, but you will get audio jitter and frame drops in demanding games.

# Hacking the firmware

What follows are technical details and instructions for hacking and developing the firmware.
You do not have to read them if you just want to use the firmware for playing.

## Changes

* The kernel has been patched to include the kernel side Mali drivers, taken from
  the official OrangePi H3
  [kernel](https://github.com/orangepi-xunlong/OrangePiH3_kernel)
  (version r3p0 as there is a matching blob with an usable license).
* User space has working EGL via the Mali blob
* The build system and kernel have been patched to build with the current
  Linaro gcc 7.4 gnueabihf toolchain.
* SDL1 has been replaced with a version of SDL2 that has been patched to support
  hardware acceleration on Mali / framebuffer
* Stella 3.x has been replaced with the current Stella 6.x mainline
* The build system has been cleaned up, the Hyperkin gui removed, and libpng
  and libz are now integrated into the main build.
* Most dependencies now build out-of-tree to avoid cluttering the source tree
  (U-Boot and ALSA still clutter).
* Smaller firmware image due to various optimizations
* SSH access via USB ethernet dongles in developer mode (see below)
* The firmware creates and uses a directory called `sys` in the root of the
  SD card.

## Branches and versions

The `master` branch is where development for Stella 6 happens. If you want to
build stable firmware for Stella 3.x, use the `stella-3.x` branch.

## Docker build environment

The easiest way to build the firmware is the docker container at
https://hub.docker.com/repository/docker/cnspeckn/r77-firmware-ng-build .

In order to do a release build, start the container and run

```
$ git pull
§ git submodule update
$ make RELEASE=1
```

Including `RELEASE=1` will build Stella with PGO (profile guided optimisation).
This gives a considerable speed boost at the expense of increased build time.
The last build step (creating the SD card image) will fail unless the container
is started as `--privileged`, but the `uImage` will be created and can be used
in any case.

The container is available for both `x86_64` and `arm64`. The ARM container runs
on Apple Silicon Macs. If you want to build on ARM Linux you need `binfmt_misc`
and `qemu-user` set up for userspace `x86_64` emulation (as the build relies on
a propietary binary-only tool from Allwinner). The container is built from the
`Dockerfile`  at https://github.com/DirtyHairy/r77-firmware-ng-build .

## Manual build environment

### Git submodules

This repository references several git repositories as submodules. To fetch
those after checkout, run

```
    git submodule update --init --recursive
```

### Toolchain

You'll need the [gcc 10 A-profile
toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads)
from ARM in order to build the firmware. The build system expects the toolchain
at `toolchain/toolchain`. The sysroot is located in the `libc` subdirectory of
the toolchain and needs to be symlinked to `toolchain/sysroot-libc`. As an
alternative, you can set the `TOOLCHAIN` environment variable to point to the
directory containing toolchain and sysroot.

### Build

Presuming that all dependencies (including the toolchain) have been installed,
the build is executed via `make RELEASE=1` (for a release build) or `make`
(for a development build). You'll find the result of the build in the `out`
directory (or in the directory pointed to by the `OUTDIR` environment variable).

### Release vs. development build

In order to do a release build, invoke `make RELEASE=1`. In contrast to an
"ordinary" development build, a release build will perform additional profiling
and optimization on Stella. For this, `qemu-arm` has to be installed on the
system.

## Developer mode, ethernet and SSH access

In order to enable ethernet and SSH support, place the firmware in development mode
by creating a file called `developer` at the root of the SD card.

**IMPORTANT:** Bootup in developer mode is slower than in normal mode as there is an
additional three second delay to wait for the USB bus to settle. In addition, the first
boot generates the necessary SSH host key, so be patient.

### Ethernet

In developer mode, the firmware will bring up a supported USB ethernet dongle as `eth0`
and request an IP using DHCP. The firmware uses the hostname `retron77` during the
DHCP request, so you can use this to find the assigned IP address from your router
(or use this hostname directly if your router provides DNS).

#### Supported hardware

The kernel used by Hyperkin is very old (3.4 series from 2012), so not all USB ethernet adapters
supported by modern Linux work. Among others, RTL8150 and RTL8152 based devices
should are supported. An AX88772 based device has been reported to work as well, which
implies that other AX88xxx based devices should work as well.

If your device is not supported by the kernel, there is a chance that a driver from
a more modern version of the kernel (or from the chipset vendor) can be added to the
kernel. This is what worked for the RTL8152 driver that is used by my own ethernet dongle.
However, how to do so exceeds the bounds of this documentation. If you add support for
other hardware to the kernel, please open a pull request.

#### Troubleshooting

Depending on the dongle, it may take a few seconds until the bus settles and the network interface
becomes available. By default, the firmware will wait five seconds before attempting to bring
up the network. This can be customized in the settings file described below.

### SSH

In developer mode, the device is accessible via SSH. Both SCP and SFTP work, so the filesystem
can be mounted remotely via sshfs. The `root` account is set up for SSH, and the default password
is `whyNotFrye`. It can be changed by adding a `PASSWORD="..."` line to `/sys/settings` on the
SD card (see above).

The SSH host key is stored by the firmware in the `sys` directory on the SD card.
If no key is found, a new key is generated on boot. This can take about half a minute,
so be patient.

### Settings

The behavior of development mode can be customized via the optional `/sys/settings` file
on the SD card. The file is sourced during init and can affect the behavior of the
firmware by settings shell variables. Supported variables:

 * `PASSWORD`: Change root password for SSH login (see below)
 * `LOGFILE`: During boot, the firmware will write diagnostics data to a file on the SD.
   The default is `/sys/diagnostics.log`.
 * `IFUP_DELAY`: Delay before attempting to bring up `eth0`. The default
   is 5 seconds.
 * `WAIT_FOR_ETH0`: Don't just delay, but wait indefinitely until `eth0` is available
   before continuing boot (and bringing up the network).
 * `DONT_OVERCLOCK`: This keeps the CPU frequency at 1GHz instead of overclocking it to 1.2GHz.
   This may stop consoles with buggy hardware from crashing, but you will get audio jitter
   and frame drops in demanding games.

# License

Please refer to the licenses of the included packages for details. The dumper
source is adapted from the Hyperkin source drop.
