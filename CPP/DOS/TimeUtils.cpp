// 7-Zip TimeUtils.cpp for DOS.

#include "StdAfx.h"

namespace NDOS {
namespace NTime {

static const UInt32 kHighDosTime = 0xFF9FBF7D;
static const UInt32 kLowDosTime = 0x210000;
static const UInt32 kNumTimeQuantumsInSecond = 10000000;
static const unsigned kFileTimeStartYear = 1601;
static const unsigned kDosTimeStartYear = 1980;
static const unsigned kUnixTimeStartYear = 1970;
static const UInt64 kUnixTimeOffset = (UInt64)60 * 60 * 24 * (89 + 365 * (UInt32)(kUnixTimeStartYear - kFileTimeStartYear));
static const UInt64 kNumSecondsInFileTime = (UInt64)(Int64)-1 / kNumTimeQuantumsInSecond;


bool DosTime_To_FileTime(UInt32 dosTime, FILETIME &ft) throw()
{
  ft.dwLowDateTime = 0;
  ft.dwHighDateTime = 0;
  UInt64 res;
  if (!GetSecondsSince1601(
      kDosTimeStartYear + (unsigned)(dosTime >> 25),
      (unsigned)((dosTime >> 21) & 0xF),
      (unsigned)((dosTime >> 16) & 0x1F),
      (unsigned)((dosTime >> 11) & 0x1F),
      (unsigned)((dosTime >>  5) & 0x3F),
      (unsigned)((dosTime & 0x1F)) * 2,
      res))
    return false;
  res *= kNumTimeQuantumsInSecond;
  ft.dwLowDateTime = (UInt32)res;
  ft.dwHighDateTime = (UInt32)(res >> 32);
  return true;
}


bool FileTime_To_DosTime(const FILETIME &ft, UInt32 &dosTime) throw()
{
  #define PERIOD_4 (4 * 365 + 1)
  #define PERIOD_100 (PERIOD_4 * 25 - 1)
  #define PERIOD_400 (PERIOD_100 * 4 + 1)

  unsigned year, mon, day, hour, min, sec;
  UInt64 v64 = ft.dwLowDateTime | ((UInt64)ft.dwHighDateTime << 32);
  Byte ms[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  unsigned temp;
  UInt32 v;

  v64 += (kNumTimeQuantumsInSecond * 2 - 1);
  v64 /= kNumTimeQuantumsInSecond;
  sec = (unsigned)(v64 % 60);
  v64 /= 60;
  min = (unsigned)(v64 % 60);
  v64 /= 60;
  hour = (unsigned)(v64 % 24);
  v64 /= 24;

  v = (UInt32)v64;

  year = kFileTimeStartYear + (unsigned)(v / PERIOD_400 * 400);
  v %= PERIOD_400;

  temp = (unsigned)(v / PERIOD_100);
  if (temp == 4)
    temp = 3;
  year += temp * 100;
  v -= temp * PERIOD_100;

  temp = v / PERIOD_4;
  if (temp == 25)
    temp = 24;
  year += temp * 4;
  v -= temp * PERIOD_4;

  temp = v / 365;
  if (temp == 4)
    temp = 3;
  year += temp;
  v -= temp * 365;

  if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
    ms[1] = 29;

  for (mon = 1; mon <= 12; mon++)
  {
    unsigned s = ms[mon - 1];
    if (v < s)
      break;
    v -= s;
  }
  day = (unsigned)v + 1;

  dosTime = kLowDosTime;
  if (year < kDosTimeStartYear)
    return false;
  year -= kDosTimeStartYear;
  dosTime = kHighDosTime;
  if (year >= 128)
    return false;
  dosTime =
      ((UInt32)year << 25)
    | ((UInt32)mon  << 21)
    | ((UInt32)day  << 16)
    | ((UInt32)hour << 11)
    | ((UInt32)min  << 5)
    | ((UInt32)sec  >> 1);

  return true;
}


bool UtcFileTime_To_LocalDosTime(const FILETIME &utc, UInt32 &dosTime) throw()
{
  FILETIME loc = { 0, 0 };
  const UInt64 u1 = FILETIME_To_UInt64(utc);
  const UInt64 kDelta = ((UInt64)1 << 41); // it's larger than quantums in 1 sec.
  if (u1 >= kDelta)
  {
    if (!FileTimeToLocalFileTime(&utc, &loc))
      loc = utc;
    else
    {
      const UInt64 u2 = FILETIME_To_UInt64(loc);
      const UInt64 delta = u1 < u2 ? (u2 - u1) : (u1 - u2);
      if (delta > kDelta) // if FileTimeToLocalFileTime() overflow, we use UTC time
        loc = utc;
    }
  }
  return FileTime_To_DosTime(loc, dosTime);
}


UInt64 UnixTime_To_FileTime64(UInt32 unixTime) throw()
{
  return (kUnixTimeOffset + (UInt64)unixTime) * kNumTimeQuantumsInSecond;
}


void UnixTime_To_FileTime(UInt32 unixTime, FILETIME &ft) throw()
{
  const UInt64 v = UnixTime_To_FileTime64(unixTime);
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
}


UInt64 UnixTime64_To_FileTime64(Int64 unixTime) throw()
{
  return (UInt64)((Int64)kUnixTimeOffset + unixTime) * kNumTimeQuantumsInSecond;
}


bool UnixTime64_To_FileTime64(Int64 unixTime, UInt64 &fileTime) throw()
{
  if (unixTime > (Int64)(kNumSecondsInFileTime - kUnixTimeOffset))
  {
    fileTime = (UInt64)(Int64)-1;
    return false;
  }
  if (unixTime < -(Int64)kUnixTimeOffset)
  {
    fileTime = 0;
    return false;
  }
  fileTime = UnixTime64_To_FileTime64(unixTime);
  return true;
}


bool UnixTime64_To_FileTime(Int64 unixTime, FILETIME &ft) throw()
{
  UInt64 v;
  const bool res = UnixTime64_To_FileTime64(unixTime, v);
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
  return res;
}


Int64 FileTime_To_UnixTime64(const FILETIME &ft) throw()
{
  const UInt64 winTime = (((UInt64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
  return (Int64)(winTime / kNumTimeQuantumsInSecond) - (Int64)kUnixTimeOffset;
}


Int64 FileTime_To_UnixTime64_and_Quantums(const FILETIME &ft, UInt32 &quantums) throw()
{
  const UInt64 winTime = (((UInt64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
  quantums = (UInt32)(winTime % kNumTimeQuantumsInSecond);
  return (Int64)(winTime / kNumTimeQuantumsInSecond) - (Int64)kUnixTimeOffset;
}


bool FileTime_To_UnixTime(const FILETIME &ft, UInt32 &unixTime) throw()
{
  UInt64 winTime = (((UInt64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
  winTime /= kNumTimeQuantumsInSecond;
  if (winTime < kUnixTimeOffset)
  {
    unixTime = 0;
    return false;
  }
  winTime -= kUnixTimeOffset;
  if (winTime > (UInt32)0xFFFFFFFF)
  {
    unixTime = (UInt32)0xFFFFFFFF;
    return false;
  }
  unixTime = (UInt32)winTime;
  return true;
}


bool GetSecondsSince1601(unsigned year, unsigned month, unsigned day,
  unsigned hour, unsigned min, unsigned sec, UInt64 &resSeconds) throw()
{
  resSeconds = 0;
  if (year < kFileTimeStartYear || year >= 10000 || month < 1 || month > 12 ||
      day < 1 || day > 31 || hour > 23 || min > 59 || sec > 59)
    return false;
  const unsigned numYears = year - kFileTimeStartYear;
  UInt32 numDays = (UInt32)((UInt32)numYears * 365 + numYears / 4 - numYears / 100 + numYears / 400);
  Byte ms[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
    ms[1] = 29;
  month--;
  for (unsigned i = 0; i < month; i++)
    numDays += ms[i];
  numDays += (UInt32)(day - 1);
  resSeconds = ((UInt64)(numDays * 24 + hour) * 60 + min) * 60 + sec;
  return true;
}


void GetCurUtc_FiTime(CFiTime &ft) throw()
{
  ft = dos_time_to_timespec(time(NULL));
}


void GetCurUtcFileTime(FILETIME &ft) throw()
{
  UInt64 v = 0;
  timespec ts;

  ts = dos_time_to_timespec(time(NULL));
  {
    v = ((UInt64)ts.tv_sec + kUnixTimeOffset) *
      kNumTimeQuantumsInSecond + (UInt64)ts.tv_nsec / 100;
  }

  ft.dwLowDateTime  = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
}


}}


int Compare_FiTime(const CFiTime *a1, const CFiTime *a2)
{
  if (a1->tv_sec < a2->tv_sec) return -1;
  if (a1->tv_sec > a2->tv_sec) return 1;
  if (a1->tv_nsec < a2->tv_nsec) return -1;
  if (a1->tv_nsec > a2->tv_nsec) return 1;
  return 0;
}


bool FILETIME_To_timespec(const FILETIME &ft, CFiTime &ts)
{
  UInt32 quantums;
  const Int64 sec = NDOS::NTime::FileTime_To_UnixTime64_and_Quantums(ft, quantums);
  // time_t is long
  const time_t sec2 = (time_t)sec;
  if (sec2 == sec)
  {
    ts.tv_sec = sec2;
    ts.tv_nsec = (Int32)(quantums * 100);
    return true;
  }
  return false;
}


void FiTime_To_FILETIME_ns100(const CFiTime &ts, FILETIME &ft, unsigned &ns100)
{
  const UInt64 v = NDOS::NTime::UnixTime64_To_FileTime64(ts.tv_sec) + ((UInt64)ts.tv_nsec / 100);
  ns100 = (unsigned)((UInt64)ts.tv_nsec % 100);
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
}


void FiTime_To_FILETIME(const CFiTime &ts, FILETIME &ft)
{
  const UInt64 v = NDOS::NTime::UnixTime64_To_FileTime64(ts.tv_sec) + ((UInt64)ts.tv_nsec / 100);
  ft.dwLowDateTime = (DWORD)v;
  ft.dwHighDateTime = (DWORD)(v >> 32);
}
