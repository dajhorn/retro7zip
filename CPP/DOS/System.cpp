// 7-Zip System.cpp for DOS
#include "StdAfx.h"


// @FIXME: This should go top-level.
#define __STDC_LIMIT_MACROS

#include <dos.h>
#include <i86.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <cstdint>

#include "../Common/Defs.h"

// #include "../Common/MyWindows.h"
// #include "../../C/CpuArch.h"

#include "System.h"

namespace NDOS {
namespace NSystem {

BOOL CProcessAffinity::Get()
{
  numSysThreads = GetNumberOfProcessors();
  return TRUE;
 }

UInt32 GetNumberOfProcessors()
{
  return 1;
}

bool LongFileNames()
{
  union REGS lfn;

  // Ralph Brown's Interrupt List
  // 2171 - Windows95 - LONG FILENAME FUNCTIONS
  lfn.h.ah = 0x71;
  lfn.h.al = 0x00;
  int386(0x21, &lfn, &lfn);

  if (!lfn.x.cflag) {
    return false;
  }

  return true;
}

bool GetRamSize(uint32_t &size)
{
  /* 
   * https://github.com/dajhorn/retro7zip/wiki/DOS-Extender-Test-Matrix
   */

  union REGS registers;

  struct dpmi_memory_info_t {
    unsigned long LargestBlockSize;
    unsigned long LargestUnlockableSize;
    unsigned long LargestLockableSize;
    unsigned long LinearAddressSpaceTotal;
    unsigned long VirtualPagesFree;
    unsigned long PhysicalPagesFree;
    unsigned long PhysicalPagesTotal;
    unsigned long LinearAddressSpaceFree;
    unsigned long PageFileSize;
    unsigned long Reserved[3];
  } dpmi_memory_info;

  registers.w.ax = 0x0500;
  registers.x.edi = (unsigned)&dpmi_memory_info;
  int386(0x31, &registers, &registers);

  if (registers.x.cflag) {
    // unsigned short error_code = registers.w.ax;
    size = 0;
    return false;
  }

  if (dpmi_memory_info.PhysicalPagesFree >= UINT32_MAX / 4096) {
    // DPMI should never report that more than 4GB of memory is available,
    // and the DOS page size is always 4 KiB.
    size = 0;
    return false;
  }

  size = dpmi_memory_info.PhysicalPagesFree * 4096;
  return true;
}

unsigned int MaximumDictionarySize(uint32_t &dictionary_size)
{
  /*
   * LZMA uses an amount of XMS memory that is approximately thirteen times
   * the size of the encoding dictionary, plus the size of the static
   * allocation of the 7z executable image.
   *
   * None of the upstream compression presets comfortably fit the intended
   * DOS target machine. Rounding up:
   *
   *   -mx1 requires 8 MiB
   *   -mx2 requires 16 MiB
   *   -mx3 requires 64 MiB
   *   -mx4 requires 256 MiB
   *   -mx5 requires 512 MiB (this is the upstream default)
   *
   * If a dictionary size is not passed with a -md switch, then override the
   * working preset and just use the largest dictionary that can be allocated
   * by the host.
   *
   * This means that a 64 MiB host will create 7z files with a 2 MiB
   * dictionary that can be unpacked by the 7zm mini variant running on a
   * 6 MiB host or the 7zr reduced variant running on a 8 MiB host.
   */

  unsigned int dictionary_width = 0;

  // Get the amount of alloc'able memory in bytes.
  GetRamSize(dictionary_size);

  if (dictionary_size == 0)
    return 0;

  // For extra dictionary headspace, divide the amount of free memory by
  // sixteen instead of thirteen.
  dictionary_size /= 16;

  // Compute floor(log2(size)).
  while (dictionary_size >>= 1)
    ++dictionary_width;

  // 4 KiB is the smallest LZMA dictionary size.
  if (dictionary_width < 12)
    dictionary_width = 12;

  dictionary_size = 1 << dictionary_width;
  return dictionary_width;
}

unsigned long Get_File_OPEN_MAX()
{
  return FOPEN_MAX;
}

unsigned Get_File_OPEN_MAX_Reduced_for_3_tasks()
{
  return (FOPEN_MAX - 10) / 3;
}

}}
