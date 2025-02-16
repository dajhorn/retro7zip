// 7-Zip FileIO.cpp for DOS.

#include "StdAfx.h"
#include <fcntl.h>


HRESULT GetLastError_noZero_HRESULT()
{
  const DWORD res = ::GetLastError();
  if (res == 0)
    return E_FAIL;
  return HRESULT_FROM_WIN32(res);
}


namespace NDOS {
namespace NFile {

namespace NDir {
bool SetDirTime(CFSTR path, const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime);
}

namespace NIO {


bool CFileBase::OpenBinary(const char *name, int flags, mode_t mode)
{
  #ifdef O_BINARY
  flags |= O_BINARY;
  #endif

  Close();
  _handle = ::open(name, flags, mode);

  if (_handle != -1) {
    /*
	 * @NOTE: In the Open Watcom v2 runtime for DOS, the <dos.h> functions
     *        are reliable, whereas the <sys/stat.h> functions are not.
	 */
    _dos_getfileattr(name, &_handle_attributes);
  }

  return _handle != -1;
}


bool CFileBase::Close()
{
  if (_handle == -1)
    return true;
  if (close(_handle) != 0)
    return false;
  _handle = -1;
  return true;
}


bool CFileBase::GetLength(UInt64 &length) const
{
  length = 0;
  const off_t curPos = seekToCur();
  if (curPos == -1)
    return false;
  const off_t lengthTemp = seek(0, SEEK_END);
  seek(curPos, SEEK_SET);
  length = (UInt64)lengthTemp;
  return (lengthTemp != -1);
}


off_t CFileBase::seek(off_t distanceToMove, int moveMethod) const
{
  return ::lseek(_handle, distanceToMove, moveMethod);
}


off_t CFileBase::seekToBegin() const throw()
{
  return seek(0, SEEK_SET);
}


off_t CFileBase::seekToCur() const throw()
{
  return seek(0, SEEK_CUR);
}


bool CInFile::Open(const char *name)
{
  return CFileBase::OpenBinary(name, O_RDONLY);
}


bool CInFile::OpenShared(const char *name, bool)
{
  return Open(name);
}


static const size_t kChunkSizeMax = ((size_t)1 << 22);


ssize_t CInFile::read_part(void *data, size_t size) throw()
{
  if (size > kChunkSizeMax)
    size = kChunkSizeMax;
  return ::read(_handle, data, size);
}


bool CInFile::ReadFull(void *data, size_t size, size_t &processed) throw()
{
  processed = 0;
  do
  {
    const ssize_t res = read_part(data, size);
    if (res < 0)
      return false;
    if (res == 0)
      break;
    data = (void *)((Byte *)data + (size_t)res);
    processed += (size_t)res;
    size -= (size_t)res;
  }
  while (size);
  return true;
}


bool COutFile::OpenBinary_forWrite_oflag(const char *name, int oflag)
{
  Path = name;
  return OpenBinary(name, oflag, mode_for_Create);
}


bool COutFile::Open_EXISTING(const char *name)
{
  return OpenBinary_forWrite_oflag(name, O_WRONLY);
}


bool COutFile::Create_ALWAYS(const char *name)
{
  return OpenBinary_forWrite_oflag(name, O_WRONLY | O_CREAT | O_TRUNC);
}


bool COutFile::Create_NEW(const char *name)
{
  return OpenBinary_forWrite_oflag(name, O_WRONLY | O_CREAT | O_EXCL);
}


bool COutFile::Create_ALWAYS_or_Open_ALWAYS(const char *name, bool createAlways)
{
  return OpenBinary_forWrite_oflag(name,
      createAlways ?
        O_WRONLY | O_CREAT | O_TRUNC :
        O_WRONLY | O_CREAT);
}


ssize_t COutFile::write_part(const void *data, size_t size) throw()
{
  if (size > kChunkSizeMax)
    size = kChunkSizeMax;
  return ::write(_handle, data, size);
}


ssize_t COutFile::write_full(const void *data, size_t size, size_t &processed) throw()
{
  processed = 0;
  do
  {
    const ssize_t res = write_part(data, size);
    if (res < 0)
      return res;
    if (res == 0)
      break;
    data = (const void *)((const Byte *)data + (size_t)res);
    processed += (size_t)res;
    size -= (size_t)res;
  }
  while (size);
  return (ssize_t)processed;
}


bool COutFile::SetLength(UInt64 length) throw()
{
  /* @FIXME: __DOS__ implement ftruncate */
  return false;

  #if 0
  const off_t len2 = (off_t)length;
  if ((Int64)length != len2)
  {
    SetLastError(EFBIG);
    return false;
  }
  // The value of the seek pointer shall not be modified by a call to ftruncate().
  const int iret = ftruncate(_handle, len2);
  return (iret == 0);
  #endif
}


bool COutFile::Close()
{
  const bool res = CFileBase::Close();
  if (!res)
    return res;
  if (CTime_defined || ATime_defined || MTime_defined)
  {
    NDOS::NFile::NDir::SetDirTime(Path,
      CTime_defined ? &CTime : NULL,
      ATime_defined ? &ATime : NULL,
      MTime_defined ? &MTime : NULL);
  }
  return res;
}


bool COutFile::SetTime(const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime) throw()
{
  /* @TODO: Determine whether the CTime and ATime logic can be strippped entirely. */
  if (cTime) { CTime = *cTime; CTime_defined = true; } else CTime_defined = false;
  if (aTime) { ATime = *aTime; ATime_defined = true; } else ATime_defined = false;
  if (mTime) { MTime = *mTime; MTime_defined = true; } else MTime_defined = false;

  if (!MTime_defined)
    return false;

  time_t owc_posix_seconds = mTime->tv_sec;
  struct tm *owc_posix_time = localtime(&owc_posix_seconds);

  unsigned owc_dos_date =                    /* DOS 16-bit packed date format masks: */
    ((owc_posix_time->tm_year - 80) << 9) |  /* 1111111000000000, years since 1980.  */
    ((owc_posix_time->tm_mon  +  1) << 5) |  /* 0000000111100000, month.             */
    ((owc_posix_time->tm_mday     ) << 0) ;  /* 0000000000011111, day.               */

  unsigned owc_dos_time =                    /* DOS 16-bit packed time format masks: */
     (owc_posix_time->tm_hour << 11) |       /* 11111100000000000, hours.            */
     (owc_posix_time->tm_min  <<  5) |       /* 00000011111100000, minutes.          */
     (owc_posix_time->tm_sec  >>  2) ;       /* 00000000000011111, duo-seconds.      */

  return _dos_setftime(_handle, owc_dos_date, owc_dos_time) == 0;
}


bool COutFile::SetMTime(const CFiTime *mTime) throw()
{
  if (mTime) { MTime = *mTime; MTime_defined = true; } else MTime_defined = false;
  return true;
}


}}}
