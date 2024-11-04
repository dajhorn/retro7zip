// 7-Zip FileIO.cpp for DOS

#include "StdAfx.h"

#ifdef Z7_DEVICE_FILE
#include "../../C/Alloc.h"
#endif

#include <fcntl.h>
#include <unistd.h>

#include "FileIO.h"
#include "FileName.h"

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
    // @FIXME: In the Open Watcom v2 runtime for DOS, the <dos.h> functions
    // are reliable, whereas the <sys/stat.h> functions are not.
    _dos_getfileattr(name, &_handle_attributes);
  }
 
  return _handle != -1;

  /*
  if (_handle == -1)
    return false;
  if (IsString1PrefixedByString2(name, "/dev/"))
  {
    // /dev/sda
    // IsDeviceFile = true; // for debug
    // SizeDefined = false;
    // SizeDefined = (GetDeviceSize_InBytes(Size) == 0);
  }
  return true;
  */
}

bool CFileBase::Close()
{
  if (_handle == -1)
    return true;
  if (close(_handle) != 0)
    return false;
  _handle = -1;
  /*
  IsDeviceFile = false;
  SizeDefined = false;
  */
  return true;
}

bool CFileBase::GetLength(UInt64 &length) const
{
  length = 0;
  // length = (UInt64)(Int64)-1; // for debug
  const off_t curPos = seekToCur();
  if (curPos == -1)
    return false;
  const off_t lengthTemp = seek(0, SEEK_END);
  seek(curPos, SEEK_SET);
  length = (UInt64)lengthTemp;

  /*
  // 22.00:
  if (lengthTemp == 1)
  if (IsDeviceFile && SizeDefined)
  {
    length = Size;
    return true;
  }
  */

  return (lengthTemp != -1);
}

off_t CFileBase::seek(off_t distanceToMove, int moveMethod) const
{
  /*
  if (IsDeviceFile && SizeDefined && moveMethod == SEEK_END)
  {
    printf("\n seek : IsDeviceFile moveMethod = %d distanceToMove = %ld\n", moveMethod, distanceToMove);
    distanceToMove += Size;
    moveMethod = SEEK_SET;
  }
  */

  // printf("\nCFileBase::seek() moveMethod = %d, distanceToMove = %lld", moveMethod, (long long)distanceToMove);
  // off_t res = ::lseek(_handle, distanceToMove, moveMethod);
  // printf("\n lseek : moveMethod = %d distanceToMove = %ld\n", moveMethod, distanceToMove);
  return ::lseek(_handle, distanceToMove, moveMethod);
  // return res;
}

off_t CFileBase::seekToBegin() const throw()
{
  return seek(0, SEEK_SET);
}

off_t CFileBase::seekToCur() const throw()
{
  return seek(0, SEEK_CUR);
}

/*
bool CFileBase::SeekToBegin() const throw()
{
  return (::seek(0, SEEK_SET) != -1);
}
*/


/////////////////////////
// CInFile

bool CInFile::Open(const char *name)
{
  return CFileBase::OpenBinary(name, O_RDONLY);
}

bool CInFile::OpenShared(const char *name, bool)
{
  return Open(name);
}


/*
int CFileBase::my_ioctl_BLKGETSIZE64(unsigned long long *numBlocks)
{
  // we can read "/sys/block/sda/size" "/sys/block/sda/sda1/size" - partition
  // #include <linux/fs.h>
  return ioctl(_handle, BLKGETSIZE64, numBlocks);
  // in block size
}

int CFileBase::GetDeviceSize_InBytes(UInt64 &size)
{
  size = 0;
  unsigned long long numBlocks;
  int res = my_ioctl_BLKGETSIZE64(&numBlocks);
  if (res == 0)
    size = numBlocks; // another blockSize s possible?
  printf("\nGetDeviceSize_InBytes res = %d, size = %lld\n", res, (long long)size);
  return res;
}
*/

/*
On Linux (32-bit and 64-bit):
read(), write() (and similar system calls) will transfer at most
0x7ffff000 = (2GiB - 4 KiB) bytes, returning the number of bytes actually transferred.
*/

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


/////////////////////////
// COutFile

bool COutFile::OpenBinary_forWrite_oflag(const char *name, int oflag)
{
  Path = name; // change it : set it only if open is success.
  return OpenBinary(name, oflag, mode_for_Create);
}


/*
  windows           exist  non-exist  posix
  CREATE_NEW        Fail   Create     O_CREAT | O_EXCL
  CREATE_ALWAYS     Trunc  Create     O_CREAT | O_TRUNC
  OPEN_ALWAYS       Open   Create     O_CREAT
  OPEN_EXISTING     Open   Fail       0
  TRUNCATE_EXISTING Trunc  Fail       O_TRUNC ???

  // O_CREAT = If the file exists, this flag has no effect except as noted under O_EXCL below.
  // If O_CREAT and O_EXCL are set, open() shall fail if the file exists.
  // O_TRUNC : If the file exists and the file is successfully opened, its length shall be truncated to 0.
*/
bool COutFile::Open_EXISTING(const char *name)
  { return OpenBinary_forWrite_oflag(name, O_WRONLY); }
bool COutFile::Create_ALWAYS(const char *name)
  { return OpenBinary_forWrite_oflag(name, O_WRONLY | O_CREAT | O_TRUNC); }
bool COutFile::Create_NEW(const char *name)
  { return OpenBinary_forWrite_oflag(name, O_WRONLY | O_CREAT | O_EXCL);  }
bool COutFile::Create_ALWAYS_or_Open_ALWAYS(const char *name, bool createAlways)
{
  return OpenBinary_forWrite_oflag(name,
      createAlways ?
        O_WRONLY | O_CREAT | O_TRUNC :
        O_WRONLY | O_CREAT);
}
/*
bool COutFile::Create_ALWAYS_or_NEW(const char *name, bool createAlways)
{
  return OpenBinary_forWrite_oflag(name,
      createAlways ?
        O_WRONLY | O_CREAT | O_TRUNC :
        O_WRONLY | O_CREAT | O_EXCL);
}
bool COutFile::Open_Disposition(const char *name, DWORD creationDisposition)
{
  int flag;
  switch (creationDisposition)
  {
    case CREATE_NEW:        flag = O_WRONLY | O_CREAT | O_EXCL;  break;
    case CREATE_ALWAYS:     flag = O_WRONLY | O_CREAT | O_TRUNC;  break;
    case OPEN_ALWAYS:       flag = O_WRONLY | O_CREAT;  break;
    case OPEN_EXISTING:     flag = O_WRONLY;  break;
    case TRUNCATE_EXISTING: flag = O_WRONLY | O_TRUNC; break;
    default:
      SetLastError(EINVAL);
      return false;
  }
  return OpenBinary_forWrite_oflag(name, flag);
}
*/

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
    /* bool res2 = */ NDOS::NFile::NDir::SetDirTime(Path,
        CTime_defined ? &CTime : NULL,
        ATime_defined ? &ATime : NULL,
        MTime_defined ? &MTime : NULL);
  }
  return res;
}

bool COutFile::SetTime(const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime) throw()
{
  // On some OS (cygwin, MacOSX ...), you must close the file before updating times
  // return true;

  if (cTime) { CTime = *cTime; CTime_defined = true; } else CTime_defined = false;
  if (aTime) { ATime = *aTime; ATime_defined = true; } else ATime_defined = false;
  if (mTime) { MTime = *mTime; MTime_defined = true; } else MTime_defined = false;
  return true;

  /*
  struct timespec times[2];
  UNUSED_VAR(cTime)
  if (!aTime && !mTime)
    return true;
  bool needChange;
  needChange  = FiTime_To_timespec(aTime, times[0]);
  needChange |= FiTime_To_timespec(mTime, times[1]);
  if (!needChange)
    return true;
  return futimens(_handle, times) == 0;
  */
}

bool COutFile::SetMTime(const CFiTime *mTime) throw()
{
  if (mTime) { MTime = *mTime; MTime_defined = true; } else MTime_defined = false;
  return true;
}

}}}