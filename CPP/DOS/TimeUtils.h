// TimeUtils.h for DOS

#ifndef ZIP7_INC_DOS_TIME_UTILS_H
#define ZIP7_INC_DOS_TIME_UTILS_H

#include <sys/stat.h>
#include <time.h>

#include "../Common/MyTypes.h"
#include "../Common/MyWindows.h"
#include "PropVariant.h"

inline UInt64 FILETIME_To_UInt64(const FILETIME &ft)
{
  return (((UInt64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
}

inline void FILETIME_Clear(FILETIME &ft)
{
  ft.dwLowDateTime = 0;
  ft.dwHighDateTime = 0;
}

inline bool FILETIME_IsZero(const FILETIME &ft)
{
  return (ft.dwHighDateTime == 0 && ft.dwLowDateTime == 0);
}

  #define CFiTime timespec
  int Compare_FiTime(const CFiTime *a1, const CFiTime *a2);
  bool FILETIME_To_timespec(const FILETIME &ft, CFiTime &ts);
  void FiTime_To_FILETIME(const CFiTime &ts, FILETIME &ft);
  void FiTime_To_FILETIME_ns100(const CFiTime &ts, FILETIME &ft, unsigned &ns100);
  inline void FiTime_Clear(CFiTime &ft)
  {
    ft.tv_sec = 0;
    ft.tv_nsec = 0;
  }

inline struct timespec dos_time_to_timespec(const time_t & dos_time)
{
  struct timespec dos_timespec;
  dos_timespec.tv_sec = dos_time;
  dos_timespec.tv_nsec = 0;
  return dos_timespec;
}
#define ST_MTIME(st) dos_time_to_timespec(st.st_mtime)
#define ST_ATIME(st) dos_time_to_timespec(st.st_atime)
#define ST_CTIME(st) dos_time_to_timespec(st.st_ctime)

// void FiTime_Normalize_With_Prec(CFiTime &ft, unsigned prec);

namespace NDOS {
namespace NTime {

bool DosTime_To_FileTime(UInt32 dosTime, FILETIME &fileTime) throw();
bool UtcFileTime_To_LocalDosTime(const FILETIME &utc, UInt32 &dosTime) throw();
bool FileTime_To_DosTime(const FILETIME &fileTime, UInt32 &dosTime) throw();

// UInt32 Unix Time : for dates 1970-2106
UInt64 UnixTime_To_FileTime64(UInt32 unixTime) throw();
void UnixTime_To_FileTime(UInt32 unixTime, FILETIME &fileTime) throw();

// Int64 Unix Time : negative values for dates before 1970
UInt64 UnixTime64_To_FileTime64(Int64 unixTime) throw(); // no check
bool UnixTime64_To_FileTime64(Int64 unixTime, UInt64 &fileTime) throw();
bool UnixTime64_To_FileTime(Int64 unixTime, FILETIME &fileTime) throw();

Int64 FileTime64_To_UnixTime64(UInt64 ft64) throw();
bool FileTime_To_UnixTime(const FILETIME &fileTime, UInt32 &unixTime) throw();
Int64 FileTime_To_UnixTime64(const FILETIME &ft) throw();
Int64 FileTime_To_UnixTime64_and_Quantums(const FILETIME &ft, UInt32 &quantums) throw();

bool GetSecondsSince1601(unsigned year, unsigned month, unsigned day, unsigned hour, unsigned min, unsigned sec, UInt64 &resSeconds) throw();
void GetCurUtc_FiTime(CFiTime &ft) throw();
void GetCurUtcFileTime(FILETIME &ft) throw();

}}

inline void PropVariant_SetFrom_UnixTime(NDOS::NCOM::CPropVariant &prop, UInt32 unixTime)
{
  FILETIME ft;
  NDOS::NTime::UnixTime_To_FileTime(unixTime, ft);
  prop.SetAsTimeFrom_FT_Prec(ft, k_PropVar_TimePrec_Unix);
}

inline void PropVariant_SetFrom_NtfsTime(NDOS::NCOM::CPropVariant &prop, const FILETIME &ft)
{
  prop.SetAsTimeFrom_FT_Prec(ft, k_PropVar_TimePrec_100ns);
}

inline void PropVariant_SetFrom_FiTime(NDOS::NCOM::CPropVariant &prop, const CFiTime &fts)
{
  unsigned ns100;
  FILETIME ft;
  FiTime_To_FILETIME_ns100(fts, ft, ns100);
  prop.SetAsTimeFrom_FT_Prec_Ns100(ft, k_PropVar_TimePrec_1ns, ns100);
}

inline bool PropVariant_SetFrom_DosTime(NDOS::NCOM::CPropVariant &prop, UInt32 dosTime)
{
  FILETIME localFileTime, utc;
  if (!NDOS::NTime::DosTime_To_FileTime(dosTime, localFileTime))
    return false;
  if (!LocalFileTimeToFileTime(&localFileTime, &utc))
    return false;
  prop.SetAsTimeFrom_FT_Prec(utc, k_PropVar_TimePrec_DOS);
  return true;
}

#endif // ZIP7_INC_DOS_TIME_UTILS_H