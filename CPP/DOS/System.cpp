// 7-Zip System.cpp for DOS.

#include "StdAfx.h"

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
  /*
   * https://github.com/dajhorn/retro7zip/wiki/LFN-Detection-Test-Matrix
   */

  union REGS regs;
  char buffer[PATH_MAX +1] = {0};

  memset(&regs, 0, sizeof(regs));
  regs.h.ah = 0x71;
  regs.h.al = 0x47;
  regs.w.cflag = 1;
  regs.x.esi = (unsigned)buffer;
  int386(0x21, &regs, &regs);

  /*
  printf("Function 7147h cflag=%x ah=0x%04x al=0x%04x esi=%s\n",
         regs.w.cflag, regs.h.ah, regs.h.al, regs.x.esi);
  */

  if (regs.w.cflag == 0 && regs.h.ah == 0x00 && regs.h.al == 0x00 ) {
    /*
     * This is:
     *
     * - DOXBox-X [dos lfn=on]
     * - Windows 95
     * - Windows 98
     * - Windows Me
     *
     * The esi register will be a pointer to a printable string on Win9x only.
     */
    return true;
  }

  if (regs.w.cflag == 0 && regs.h.ah == 0x71 && regs.h.al == 0x47 ) {
    /*
     * This is DOSLFN 0.41f running on any host.
     *
     * @TODO: Check whether esi ever points to a string or is always bogus.
     */
    return true;
  }

  if (regs.w.cflag == 0 && regs.h.ah == 0x71 && regs.h.al == 0x00 ) {
    /*
     * These are all pre-VFAT DOS platforms:
     *
     * - MS-DOS 6.22
     * - PC-DOS 7.0
     * - Windows NT 3.1  (NTVDM)
     * - Windows NT 3.50 (NTVDM)
     * - Windows NT 3.51 (NTVDM)
     * - Windows NT 4.0  (NTVDM)
     *
     * NTVDM was forked from MS-DOS 5 and always reports DOS version 5.5,
     * even on Windows 10 (the 32-bit edition still has it as a
     * feature-on-demand).
     */
    return false;
  }

  if (regs.w.cflag == 1 && regs.h.ah == 0x71 && regs.h.al == 0x00 ) {
    /*
     * These are all post-VFAT DOS platforms:
     *
     * - DOSBox-X    [dos lfn=off]
     * - FreeDOS 1.4 (without an LFN driver)
     * - MS-DOS 7.0  (MS-DOS Mode in Windows 95 RTM and Windows 95 OSR1)
     * - MS-DOS 7.1  (MS-DOS Mode in Windows 95 OSR2 and Windows 98)
     * - MS-DOS 8.0  (MS-DOS Mode in Windows Me if enabled by a patch)
     */
     return false;
  }

  /*
   * Make the default false because the *.* globbing pattern works
   * mostly everywhere.
   */
  return false;
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
