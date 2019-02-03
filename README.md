# What is this?

This a an attempt to update and rework the Retron 77 firmware sufficiently to
get SDL2 and Stella 6 running. It is built from the official Hyperkin
[source drop](https://www.hyperkin.com/r77) (v0.6.2) and includes the
changes made my Remowilliams to build the community firmware image on
[AtariAge](http://atariage.com/forums/topic/281462-retron-77-community-build-image/).

# Changes so far

* The kernel has been patched to include the kernel side Mali drivers, taken from
  the official OrangePi H3
  [kernel](https://github.com/orangepi-xunlong/OrangePiH3_kernel)
  (version r3p0 as there is a matching blob with an usable license).
* The build system and kernel have been patched to build with the current
  Linaro gcc 7.4 gnueabihf toolchain.
* The build system has been cleaned up, the Hyperkin gui removed, and libpng
  and libz are now integrated into the main build.

# Setup

## Toolchain

The toolchain is huge and can be readily downloaded from Linaro. You'll
need the
[gcc 7.4 gnueabihf toolchain](http://releases.linaro.org/components/toolchain/binaries/7.4-2019.02/arm-linux-gnueabihf/)
to build this version of the firmware. The build
system expects the toolchain at `toolchain/toolchain` and the sysroot at
`toolchain/sysroot-glibc`. As an alternative, you can set the `TOOLCHAIN`
environment variable to point to the directory containing toolchain and sysroot.

## Stella

Stella has been integrated as a git submodule. Follow the build instructions below
to make sure that all submodules are properly checked out.

# Building

After checkout, make sure that all submodules are checked out by running
`git submodule update --init`. After that, the build process is initiated
by running `make`. If all goes well, you will find the output in the `out`
subdirectory. You can specify an alternate output directory by setting the
`OUTDIR` environment variable.

## Docker build environment

You can find a Dockerfile for setting up a Debian-based build environment
[here](https://github.com/DirtyHairy/r77-firmware-ng-build/tree/master).

# License

Please refer to the licenses of the included packages for details. The dumper
source is adapted from the Hyperkin source drop.
