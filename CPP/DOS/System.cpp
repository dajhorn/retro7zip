// Windows/System.cpp

#include "StdAfx.h"

#include <dos.h>
#include <i86.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>

#include "../Common/Defs.h"
// #include "../Common/MyWindows.h"dir 

// #include "../../C/CpuArch.h"

#include "System.h"

namespace NDOS {
namespace NSystem {

#ifdef _WIN32

UInt32 CountAffinity(DWORD_PTR mask)
{
  UInt32 num = 0;
  for (unsigned i = 0; i < sizeof(mask) * 8; i++)
  {
    num += (UInt32)(mask & 1);
    mask >>= 1;
  }
  return num;
}

BOOL CProcessAffinity::Get()
{
  #if !defined(UNDER_CE) && !defined(__WATCOMC__)
  return GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);
  #else
  return FALSE;
  #endif // !defined(UNDER_CE) && !defined(__WATCOMC__)
}


UInt32 GetNumberOfProcessors()
{
#if defined(__WATCOMC__)
  return 1;
#else
  // We need to know how many threads we can use.
  // By default the process is assigned to one group.
  // So we get the number of logical processors (threads)
  // assigned to current process in the current group.
  // Group size can be smaller than total number logical processors, for exammple, 2x36

  CProcessAffinity pa;

  if (pa.Get() && pa.processAffinityMask != 0)
    return pa.GetNumProcessThreads();

  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);
  // the number of logical processors in the current group
  return (UInt32)systemInfo.dwNumberOfProcessors;
#endif // defined(__WATCOMC__)
}

#else


BOOL CProcessAffinity::Get()
{
  numSysThreads = GetNumberOfProcessors();

  /*
  numSysThreads = 8;
  for (unsigned i = 0; i < numSysThreads; i++)
    CpuSet_Set(&cpu_set, i);
  return TRUE;
  */
  
  #ifdef Z7_AFFINITY_SUPPORTED
  
  // numSysThreads = sysconf(_SC_NPROCESSORS_ONLN); // The number of processors currently online
  if (sched_getaffinity(0, sizeof(cpu_set), &cpu_set) != 0)
    return FALSE;
  return TRUE;
  
  #else
  
  // cpu_set = ((CCpuSet)1 << (numSysThreads)) - 1;
  return TRUE;
  // errno = ENOSYS;
  // return FALSE;
  
  #endif
}

UInt32 GetNumberOfProcessors()
{
  #ifndef Z7_ST
  long n = sysconf(_SC_NPROCESSORS_CONF);  // The number of processors configured
  if (n < 1)
    n = 1;
  return (UInt32)n;
  #else
  return 1;
  #endif
}

#endif


#ifdef _WIN32

#ifndef UNDER_CE

#if !defined(_WIN64) && \
  (defined(__MINGW32_VERSION) || defined(Z7_OLD_WIN_SDK))

typedef struct {
  DWORD dwLength;
  DWORD dwMemoryLoad;
  DWORDLONG ullTotalPhys;
  DWORDLONG ullAvailPhys;
  DWORDLONG ullTotalPageFile;
  DWORDLONG ullAvailPageFile;
  DWORDLONG ullTotalVirtual;
  DWORDLONG ullAvailVirtual;
  DWORDLONG ullAvailExtendedVirtual;
} MY_MEMORYSTATUSEX, *MY_LPMEMORYSTATUSEX;

#else

#define MY_MEMORYSTATUSEX MEMORYSTATUSEX
#define MY_LPMEMORYSTATUSEX LPMEMORYSTATUSEX

#endif

typedef BOOL (WINAPI *Func_GlobalMemoryStatusEx)(MY_LPMEMORYSTATUSEX lpBuffer);

#endif // !UNDER_CE

  
bool GetRamSize(UInt64 &size)
{
  size = (UInt64)(sizeof(size_t)) << 29;

  #ifndef UNDER_CE
    MY_MEMORYSTATUSEX stat;
    stat.dwLength = sizeof(stat);
  #endif
  
  #ifdef _WIN64
    
    if (!::GlobalMemoryStatusEx(&stat))
      return false;
    size = MyMin(stat.ullTotalVirtual, stat.ullTotalPhys);
    return true;

  #else
    
    #ifndef UNDER_CE
      const
      Func_GlobalMemoryStatusEx fn = Z7_GET_PROC_ADDRESS(
      Func_GlobalMemoryStatusEx, ::GetModuleHandleA("kernel32.dll"),
          "GlobalMemoryStatusEx");
      if (fn && fn(&stat))
      {
        size = MyMin(stat.ullTotalVirtual, stat.ullTotalPhys);
        return true;
      }
    #endif
  
    {
      MEMORYSTATUS stat2;
      stat2.dwLength = sizeof(stat2);
      ::GlobalMemoryStatus(&stat2);
      size = MyMin(stat2.dwTotalVirtual, stat2.dwTotalPhys);
      return true;
    }
  #endif
}
  
#else

// POSIX
// #include <stdio.h>

bool GetRamSize(UInt64 &size)
{
  /* @FIXME:  Check whether this actually works. */
  union REGS regs;
  regs.h.ah = 0x88;
  int386(0x15, &regs, &regs);
  size = regs.w.ax;
  return true;
}

#endif


unsigned long Get_File_OPEN_MAX()
{
#if defined(__WATCOMC__)
  return FOPEN_MAX;
#elif defined(_WIN32)
  return (1 << 24) - (1 << 16); // ~16M handles
#else
  // some linux versions have default open file limit for user process of 1024 files.
  long n = sysconf(_SC_OPEN_MAX);
  // n = -1; // for debug
  // n = 9; // for debug
  if (n < 1)
  {
    // n = OPEN_MAX;  // ???
    // n = FOPEN_MAX; // = 16 : <stdio.h>
   #ifdef _POSIX_OPEN_MAX
    n = _POSIX_OPEN_MAX; // = 20 : <limits.h>
   #else
    n = 30; // our limit
   #endif
  }
  return (unsigned long)n;
#endif // defined(__WATCOMC__)
}

unsigned Get_File_OPEN_MAX_Reduced_for_3_tasks()
{
  unsigned long numFiles_OPEN_MAX = NSystem::Get_File_OPEN_MAX();
  const unsigned delta = 10; // the reserve for another internal needs of process
  if (numFiles_OPEN_MAX > delta)
    numFiles_OPEN_MAX -= delta;
  else
    numFiles_OPEN_MAX = 1;
  numFiles_OPEN_MAX /= 3; // we suppose that we have up to 3 tasks in total for multiple file processing
  numFiles_OPEN_MAX = MyMax(numFiles_OPEN_MAX, (unsigned long)3);
  unsigned n = (unsigned)(int)-1;
  if (n > numFiles_OPEN_MAX)
    n = (unsigned)numFiles_OPEN_MAX;
  return n;
}

}}
