// 7-Zip FileIO.h for DOS.

#ifndef ZIP7_INC_DOS_FILE_IO_H
#define ZIP7_INC_DOS_FILE_IO_H

#define Z7_WIN_IO_REPARSE_TAG_MOUNT_POINT  (0xA0000003L)
#define Z7_WIN_IO_REPARSE_TAG_SYMLINK      (0xA000000CL)
#define Z7_WIN_IO_REPARSE_TAG_LX_SYMLINK   (0xA000001DL)
#define Z7_WIN_SYMLINK_FLAG_RELATIVE       1
#define Z7_WIN_LX_SYMLINK_FLAG             2

#include "../Common/MyBuffer.h"
#include "../DOS/TimeUtils.h"


HRESULT GetLastError_noZero_HRESULT();

namespace NDOS {
namespace NFile {


struct CReparseShortInfo
{
  unsigned Offset;
  unsigned Size;
  bool Parse(const Byte *p, size_t size);
};


struct CReparseAttr
{
  UInt32 Tag;
  UInt32 Flags;
  UString SubsName;
  UString PrintName;
  AString WslName;
  bool HeaderError;
  bool TagIsUnknown;
  bool MinorError;
  DWORD ErrorCode;

  CReparseAttr(): Tag(0), Flags(0) {}

  bool Parse(const Byte *p, size_t size);

  bool IsMountPoint()   const { return Tag   == Z7_WIN_IO_REPARSE_TAG_MOUNT_POINT; }
  bool IsSymLink_Win()  const { return Tag   == Z7_WIN_IO_REPARSE_TAG_SYMLINK;     }
  bool IsSymLink_WSL()  const { return Tag   == Z7_WIN_IO_REPARSE_TAG_LX_SYMLINK;  }
  bool IsRelative_Win() const { return Flags == Z7_WIN_SYMLINK_FLAG_RELATIVE;      }

  bool IsRelative_WSL() const
  {
    if (WslName.IsEmpty())
      return true;
    char c = WslName[0];
    return !IS_PATH_SEPAR(c);
  }

  bool IsOkNamePair() const;
  UString GetPath() const;
};

#define CFiInfo stat

namespace NIO {

bool GetReparseData(CFSTR path, CByteBuffer &reparseData);

// parameters are in reverse order of symlink() function !!!
bool SetSymLink(CFSTR from, CFSTR to);
bool SetSymLink_UString(CFSTR from, const UString &to);


class CFileBase
{
protected:
  int _handle;
  unsigned _handle_attributes;
  bool OpenBinary(const char *name, int flags, mode_t mode = 0666);

public:
  bool PreserveATime;

  CFileBase(): _handle(-1), PreserveATime(false) {}
  ~CFileBase() { Close(); }
  bool Close();
  bool GetLength(UInt64 &length) const;
  off_t seek(off_t distanceToMove, int moveMethod) const;
  off_t seekToBegin() const throw();
  off_t seekToCur() const throw();

  int my_fstat(struct stat *st) const
  {
    /*
     * @FIXME: Open Watcom v2 kludge.
     *
     * The stat() and fstat() in the DOS runtime sometimes return bogus
     * values for everything except the st_size and st_mtime fields.
     */
    int owc_shim = fstat(_handle, st);
    st->st_attr = _handle_attributes;
    return owc_shim;
  }
};


class CInFile: public CFileBase
{
public:
  bool Open(const char *name);
  bool OpenShared(const char *name, bool shareForWrite);
  #if 0
  bool AttachStdIn()
  {
    _handle = GetStdHandle(STD_INPUT_HANDLE);
    if (_handle == INVALID_HANDLE_VALUE || !_handle)
      return false;
    IsStdStream = true;
  }
  #endif
  ssize_t read_part(void *data, size_t size) throw();
  bool ReadFull(void *data, size_t size, size_t &processedSize) throw();
};


class COutFile: public CFileBase
{
  bool CTime_defined;
  bool ATime_defined;
  bool MTime_defined;
  CFiTime CTime;
  CFiTime ATime;
  CFiTime MTime;
  AString Path;

  ssize_t write_part(const void *data, size_t size) throw();
  bool OpenBinary_forWrite_oflag(const char *name, int oflag);

public:

  mode_t mode_for_Create;

  COutFile():
      CTime_defined(false),
      ATime_defined(false),
      MTime_defined(false),
      mode_for_Create(0666)
      {}

  bool Close();
  bool Open_EXISTING(CFSTR fileName);
  bool Create_ALWAYS_or_Open_ALWAYS(CFSTR fileName, bool createAlways);
  bool Create_ALWAYS(CFSTR fileName);
  bool Create_NEW(CFSTR fileName);

  ssize_t write_full(const void *data, size_t size, size_t &processed) throw();

  bool WriteFull(const void *data, size_t size) throw()
  {
    size_t processed;
    ssize_t res = write_full(data, size, processed);
    if (res == -1)
      return false;
    return processed == size;
  }

  bool SetLength(UInt64 length) throw();
  bool SetLength_KeepPosition(UInt64 length) throw()
  {
    return SetLength(length);
  }
  bool SetTime(const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime) throw();
  bool SetMTime(const CFiTime *mTime) throw();
};


}}}

#endif // ZIP7_INC_DOS_FILE_IO_H
