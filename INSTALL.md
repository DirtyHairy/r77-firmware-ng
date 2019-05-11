# Installation guide

## Rewrite the SD image

Rewriting the SD image will completely replace the contents of your SD card. In particular, this will
replace Stella's assets with the most current version. This will give you a clean start and is the
recommended method if you are coming from the original firmware or the original (Stella 3) community image.

 1. Download `sdcard.zip` from the [releases page](https://github.com/DirtyHairy/r77-firmware-ng/releases).
 2. Extract the contained `sdcard.img` file from the zip archive.
 3. Turn the device off and remove the SD card.
 4. Write the `sdcard.img` file directly to the SD card. Note that this is **not** the same as copying the file;
    you need to write the file directly to the card. On windows, this can be done using an imaging tool such as
    [USB Image Tool](https://www.alexpage.de/usb-image-tool/download/),
    [Win32 Disk Imager](https://www.alexpage.de/usb-image-tool/download/) or
    [Etcher](https://www.balena.io/etcher/). The latter is also also available for MacOS and Linux; as alternative,
    you can write the image on a terminal with `dd` on those systems if you know what you are doing.
    
    **WARNING:** This step is dangerous. Selecting a wrong drive for writing the SD image will wipe the drive,
    destroying all data on it and potentially rendering your computer unusable. Be cautious and male sure you know what
    you are doing. Ask someone for help if you aren't.
  5. Reinsert the SD card into the device and turn it back on. Say hello to Stella 6.

## Replace the uImage

This will preserve the contents of your SD card and will only swap out the actual Stella binary
and the linux system. This is recommended if you are updating from any Stella 6 image.

 1. Download `uImage` from the [releases page](https://github.com/DirtyHairy/r77-firmware-ng/releases).
 2. Turn the device off and remove the SD card.
 3. (**optional**) Make a backup copy from the old `uImage` file on your SD card
 4. Replace the `uImage` with the downloaded file
 5. Reinsert the SD card into the device and turn it back on. Say hello to Stella 6.
