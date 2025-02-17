# ![Retro7zip Icon 36x24](https://github.com/user-attachments/assets/e3474408-c4e2-41ea-bb8c-5d2d609124fe) Retro7zip

Retro7zip is a native backport of 7-Zip for DOS and old
Microsoft Windows operating systems.

## System Requirements

https://github.com/dajhorn/retro7zip/wiki/System-Requirements

### DOS System Requirements

* 386 CPU
* 4 MB RAM
* DOS 3.3 Compatibility

FreeDOS, MS-DOS, PC-DOC, SvarDOS, IBM OS/2 and others are compatible.

Long File Names are supported through the DOSLFN TSR on plain DOS,
or natively on Windows 95, Windows 98, and Windows Me.


### Windows System Requirements

* 386 CPU
* 12 MB RAM
* Win32c Compatibility

Windows 9x, Windows NT 3.50, Windows NT 3.51, and Windows NT 4.0 are
compatible.


## Installation

On FreeDOS, download the versioned release file and do this:

    RENAME 7zip-24.09+3_dos.zip 7zip.zip
    FDNPKG INSTALL 7zip.zip

On any other platform, unzip the release file and put the `EXE` files
anywhere in the `%PATH%`.


## 7-Zip Variants for DOS

The release package for DOS contains three executable files:

1.  `7zm.exe` Miniature for DOS

Runs on computers that have less than 8 megabytes of memory.  Implements only
the BCJ2 filter, LZMA2 codec, and 7Z container that are used in regular 7-Zip
archives.

2.  `7zr.exe` Reduced for DOS

Runs best on computers that have at least 8 megabytes of memory.  Implements
all filters, codecs, and containers for the archive formats that 7-Zip can
read and write.  (7z, bz2, gz, tar, xz, zip.)

3.  `7za.exe` Aggregated for DOS

Contains all upstream features, including support for filesystem images and
exotic archive formats.  Requires more than 8 megabytes of memory to unpack
some complex files.  (Run `7za i` for a complete list of supported types.)


## 7-Zip Variants for Win32c

The release package for Windows contains one executable file:

4.  `7za.exe` Aggregated for Win32c

Use this program if you need:

* Better performance.
* File sizes larger than the 2GB DOS limit.
* Long file names on Windows NT, because the NTVDM lacks the LFN API that
  DOS applications use.

The DOS variants are compatible with
[ntlfn-0.81.71](https://sta.c64.org/lfnemu.html),
but the Win32c variant is better for Windows NT.


## Licensing

The LZMA SDK is copyright Igor Pavlov and released into the public domain.

7-Zip is an aggregate work, copyright Igor Pavlov,  that is subject to
multiple licenses that are described in the [COPYRIGHT.txt](COPYRIGHT.txt)
file and the [License.txt](DOC/License.txt) file.

Retro7zip is a derived work, copyright Darik Horn, that is released into
the public domain.
