# What is this?

This is an attempt to update and rework the Retron 77 firmware sufficiently to
get SDL2 and Stella 6 running. It is built from the official Hyperkin
[source drop](https://www.hyperkin.com/r77) (v0.6.2) and includes the
changes made by Remowilliams that constitute the community firmware image on
[AtariAge](http://atariage.com/forums/topic/281462-retron-77-community-build-image/).

# State

The firmware includes hardware 3D acceleration and runs current mainline Stella. However,
the R77-specific changes to Stella 3.x must
be reintegrated into the mainline, so the console is not playable yet.

# Changes

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

  **IMPORTANT:** If there is a file called `sys` on your SD card, the firmware
  will delete it and create the directory in its place.

# Branches and versions

The `master` branch is where development for Stella 6 happens. If you want to
build stable firmware for Stella 3.x, use the `stella-3.x` branch.

# Setup

## Docker build environment

You can use this
[Dockerfile](https://github.com/DirtyHairy/r77-firmware-ng-build/tree/master)
to set up a preconfigured build environment.
This environment is based upon Debian stretch and contains all dependencies
required for building the firmware (including the toolchain and qemu-user).

## Git submodules

This repository references several git repositories as submodules. To fetch
those after checkout, run

```
    git submodule update --init --recursive
```

## Toolchain

You'll need the
[gcc 7.4 gnueabihf toolchain](http://releases.linaro.org/components/toolchain/binaries/7.4-2019.02/arm-linux-gnueabihf/)
from Linaro in order to build the firmware. The build
system expects the toolchain at `toolchain/toolchain` and the sysroot at
`toolchain/sysroot-glibc`. As an alternative, you can set the `TOOLCHAIN`
environment variable to point to the directory containing toolchain and sysroot.

# Build

Presuming that all dependencies (including the toolchain) have been installed,
the build is executed via `make RELEASE=1` (for a release build) or `make`
(for a development build). You'll find the result of the build in the `out`
directory (or in the directory pointed to by the `OUTDIR` environment variable).

## Release vs. development build

In order to do a release build, invoke `make RELEASE=1`. In contrast to an
"ordinary" development build, a release build will perform additional profiling
and optimization on Stella. For this, `qemu-arm` has to be installed on the
system.

# Settings

Behavior of the firmware can be customized via the optional `/sys/settings` file
on the SD card. The file is sourced during init and can affect the behavior of the
firmware by settings shell variables. Supported variables:

 * `PASSWORD`: Change root password for SSH login (see below)

# Developer mode, ethernet and SSH access

In order to enable ethernet and SSH support, place the firmware in development mode
by creating a file called `developer` at the root of the SD card.

**IMPORTANT:** Bootup in developer mode is slower than in normal mode as there is an
additional three second delay to wait for the USB bus to settle. In addition, the first
boot generates the necessary SSH host key, so be patient.

## Ethernet

In developer mode, the firmware will bring up a supported USB ethernet dongle as `eth0`
and request an IP using DHCP. The firmware uses the hostname `retron77` during the
DHCP request, so you can use this to find the assigned IP address from your router
(or use this hostname directly if your router provides DNS).

### Supported hardware

The kernel used by Hyperkin is very old (3.4 series from 2012), so not all USB ethernet adapters
supported by modern Linux work. Among others, RTL8150 and RTL8152 based devices
should are supported.

If your device is not supported by the kernel, there is a chance that a driver from
a more modern version of the kernel (or from the chipset vendor) can be added to the
kernel. This is what worked for the RTL8152 driver that is used by my own ethernet dongle.
However, how to do so exceeds the bounds of this documentation. If you add support for
other hardware to the kernel, please open a pull request.

## SSH

In developer mode, the device is accessible via SSH. Both SCP and SFTP work, so the filesystem
can be mounted remotely via sshfs. The `root` account is set up for SSH, and the default password
is `whyNotFrye`. It can be changed by adding a `PASSWORD="..."` line to `/sys/settings` on the
SD card (see above).

The SSH host key is stored by the firmware in the `sys` directory on the SD card.
If no key is found, a new key is generated on boot. This can take about half a minute,
so be patient.

# License

Please refer to the licenses of the included packages for details. The dumper
source is adapted from the Hyperkin source drop.
