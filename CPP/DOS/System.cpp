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

unsigned long Get_File_OPEN_MAX()
{
  return FOPEN_MAX;
}

unsigned Get_File_OPEN_MAX_Reduced_for_3_tasks()
{
  return (FOPEN_MAX - 10) / 3;
}

}}
