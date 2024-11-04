// 7-Zip System.cpp for DOS
#include "StdAfx.h"

#include <dos.h>
#include <i86.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>

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

bool GetRamSize(UInt64 &size)
{
  /* @FIXME:  Check whether this actually works. */
  union REGS regs;
  regs.h.ah = 0x88;
  int386(0x15, &regs, &regs);
  size = regs.w.ax;
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