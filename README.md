![Retro7zip Icon](https://github.com/user-attachments/assets/22abdb30-fe6e-456d-9e34-acd7fc9c1e92)

## Retro7zip

Retro7zip is a native backport of 7-Zip for DOS and Win32c that runs on
FreeDOS, MS-DOS, SvarDOS, OS/2, Windows 3, Windows 4 (95/98/Me),
Windows NT, and most other DOS-compatible platforms.

### System Requirements

* DOS 3.3
* 386 CPU
* 3MB RAM (for the mini build)

### FreeDOS Package Installation

Rename the versioned `7zip-XX.YY+ZZ_dos.zip` release file to `7ZIP.ZIP`
and do this:

  A:\> `FDNPKG INSTALL 7ZIP.ZIP`

### 7-Zip Program Variants

The release package for DOS contains three executable files:

1.  `7zm.exe` Mini for DOS

Runs on computers that have less than 8 megabytes of memory.  Implements only
the BCJ2 filter, LZMA2 codec, and 7Z container that are used in regular 7z
files.

2.  `7zr.exe` Reduced for DOS

Runs best on computers that have at least 8 megabytes of memory.  Implements
all filters, codecs, and containers for the archive formats that 7-Zip can
read and write.  (7z, bz2, gz, tar, xz, zip, etc.)

3.  `7za.exe` Aggregated for DOS

Contains all upstream features, including support for filesystem images and
exotic archive formats.  Requires more than 8 megabytes of memory to unpack
some complex files.  (Run `7za i` for a complete list of supported types.)

### Development Notes

7zr and 7za correspond to Z7_PROG_VARIANT_R and Z7_PROG_VARIANT_A in the
upstream 7-Zip release.  7zm is a unique build for DOS that is defined by
a downstream Z7_PROG_VARIANT_M macro.

Retro7zip is built with the Open Watcom v2 toolchain and uses the
Causeway DOS extender.

### Licensing

7-Zip is copyright Igor Pavlov and released into the public domain.

Retro7zip is a derived work, copyright Darik Horn, that is similarly
released into the public domain.

SPDX-License-Identifier: CC0-1.0
https://creativecommons.org/publicdomain/zero/1.0/

Some builds of 7-Zip and/or Retro7zip may contain non-free components that
are subject to additional constraints and restrictions.  Review the
`DOC/Licensing.txt` file in the source tree for specific details.
