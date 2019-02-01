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
  (version r3p0 as there is a matching blob with an usable license)).
* The build system and kernel have been patched to build with the current
  Linaro gcc 7.4 gnueabihf toolchain.
* The build system has been cleaned up, the Hyperkin gui removed, and libpng
  and libz are now integrated into the main build.

# Omissions

## Licensing issues

The following parts have been ommitted from this repository because of unclear
licensing terms. Please copy them from the original Hyperkin source drop:

 * `app/dumper`
 * `tools/allwinner-pack-tools`

## Toolchain

The toolchain is huge and can be readily downloaded from Linaro. You'll
need the GCC 7.4 gnueabihf toolchain to build this version of the firmware. The build
system expects the toolchain at `toolchain/toolchain` and the sysroot at
`toolchain/sysroot-glibc`.

## Stella

Stella has been integrated as a git submodule. Please do a `git submodule init`
to fetch it.

# Building

The firmware builds fine in a Debian docker container (you'll have to install
a bunch of packages first, though). 

Provided all tools are installed, a `make install` will build the firmware.

# License

Please refer to the licenses of the included packages for details.
