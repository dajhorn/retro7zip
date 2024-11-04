// 7-Zip FileFind.h for DOS

#ifndef ZIP7_INC_DOS_FILE_FIND_H
#define ZIP7_INC_DOS_FILE_FIND_H

#include <sys/stat.h>
#include <sys/types.h>
#include <direct.h>
#include <dos.h>

#include "../Common/MyLinux.h"
#include "../Common/MyString.h"
#include "../Common/MyWindows.h"

#include "Defs.h"
#include "FileIO.h"

namespace NDOS {
namespace NFile {
namespace NFind {

// bool DoesFileExist(CFSTR name, bool followLink);
bool DoesFileExist_Raw(CFSTR name);
bool DoesFileExist_FollowLink(CFSTR name);
bool DoesDirExist(CFSTR name, bool followLink);

inline bool DoesDirExist(CFSTR name)
  { return DoesDirExist(name, false); }

inline bool DoesDirExist_FollowLink(CFSTR name)
  { return DoesDirExist(name, true); }

// it's always _Raw
bool DoesFileOrDirExist(CFSTR name);

DWORD GetFileAttrib(CFSTR path);



namespace NAttributes
{
  inline bool IsReadOnly(DWORD attrib)   { return (attrib & _A_RDONLY) != 0; }
  inline bool IsHidden(DWORD attrib)     { return (attrib & _A_HIDDEN) != 0; }
  inline bool IsSystem(DWORD attrib)     { return (attrib & _A_SYSTEM) != 0; }
  inline bool IsDir(DWORD attrib)        { return (attrib & _A_SUBDIR) != 0; }
  inline bool IsArchived(DWORD attrib)   { return (attrib & _A_ARCH)   != 0; }
  inline bool IsCompressed(DWORD attrib) { return false; }
  inline bool IsEncrypted(DWORD attrib)  { return false; }

  inline UInt32 Get_PosixMode_From_WinAttrib(DWORD attrib)
  {
    UInt32 v = IsDir(attrib) ? MY_LIN_S_IFDIR : MY_LIN_S_IFREG;
    /* 21.06: as WSL we allow write permissions (0222) for directories even for (FILE_ATTRIBUTE_READONLY).
    So extracting at Linux will be allowed to write files inside (0777) directories. */
    v |= ((IsReadOnly(attrib) && !IsDir(attrib)) ? 0555 : 0777);
    return v;
  }
}

#if defined(_WIN32) || defined(__DOS__)
// @FIXME:  Prefer attrib over mode in __DOS__
#else
UInt32 Get_WinAttribPosix_From_PosixMode(UInt32 mode);
#endif

class CFileInfoBase
{
  bool MatchesMask(UINT32 mask) const { return ((Attrib & mask) != 0); }

public:

  UInt64 Size;
  CFiTime CTime;
  CFiTime ATime;
  CFiTime MTime;
  DWORD Attrib;
  bool IsAltStream;
  bool IsDevice;

  CFileInfoBase() { ClearBase(); }
  void ClearBase() throw();
  bool SetAs_StdInFile();

 
  bool Fill_From_ByHandleFileInfo(CFSTR path);
  void SetAsDir()  { Attrib = FILE_ATTRIBUTE_DIRECTORY; } // |= (FILE_ATTRIBUTE_UNIX_EXTENSION + (S_IFDIR << 16));

  bool IsArchived() const { return MatchesMask(FILE_ATTRIBUTE_ARCHIVE); }
  bool IsCompressed() const { return MatchesMask(FILE_ATTRIBUTE_COMPRESSED); }
  bool IsDir() const { return MatchesMask(FILE_ATTRIBUTE_DIRECTORY); }
  bool IsEncrypted() const { return MatchesMask(FILE_ATTRIBUTE_ENCRYPTED); }
  bool IsHidden() const { return MatchesMask(FILE_ATTRIBUTE_HIDDEN); }
  bool IsNormal() const { return MatchesMask(FILE_ATTRIBUTE_NORMAL); }
  bool IsOffline() const { return MatchesMask(FILE_ATTRIBUTE_OFFLINE); }
  bool IsReadOnly() const { return MatchesMask(FILE_ATTRIBUTE_READONLY); }
  bool HasReparsePoint() const { return MatchesMask(FILE_ATTRIBUTE_REPARSE_POINT); }
  bool IsSparse() const { return MatchesMask(FILE_ATTRIBUTE_SPARSE_FILE); }
  bool IsSystem() const { return MatchesMask(FILE_ATTRIBUTE_SYSTEM); }
  bool IsTemporary() const { return MatchesMask(FILE_ATTRIBUTE_TEMPORARY); }

  UInt32 GetWinAttrib() const { return Attrib; }
  UInt32 GetPosixAttrib() const
  {
    return NAttributes::Get_PosixMode_From_WinAttrib(Attrib);
  }
  bool Has_Attrib_ReparsePoint() const { return (Attrib & FILE_ATTRIBUTE_REPARSE_POINT) != 0; }
 

  bool IsOsSymLink() const
  {
      return HasReparsePoint();
  }
};

struct CFileInfo: public CFileInfoBase
{
  FString Name;

  bool IsDots() const throw();
  bool Find(CFSTR path, bool followLink = false);
  bool Find_FollowLink(CFSTR path) { return Find(path, true); }
};


class CFindFileBase  MY_UNCOPYABLE
{
public:
  struct find_t _find_buffer;
  bool          _find_allocated;

  bool IsHandleAllocated() const { return _find_allocated; }
  CFindFileBase(): _find_allocated(false) {}
  ~CFindFileBase() { Close(); }
  bool Close() throw();
};

class CFindFile: public CFindFileBase
{
public:
  bool FindFirst(CFSTR wildcard, CFileInfo &fileInfo);
  bool FindNext(CFileInfo &fileInfo);
};

// @FIXME:  Bad syntax in upstream code?
class CEnumerator
{
  CFindFile _findFile;
  FString _wildcard;
  bool NextAny(CFileInfo &fileInfo);

public:
  void SetDirPrefix(const FString &dirPrefix);
  bool Next(CFileInfo &fileInfo);
  bool Next(CFileInfo &fileInfo, bool &found);
};

#if !defined(__DOS__)
class CFindChangeNotification  MY_UNCOPYABLE
{
  HANDLE _handle;
public:
  operator HANDLE () { return _handle; }
  bool IsHandleAllocated() const
  {
    /* at least on win2000/XP (undocumented):
       if pathName is "" or NULL,
       FindFirstChangeNotification() could return NULL.
       So we check for INVALID_HANDLE_VALUE and NULL.
    */
    return _handle != INVALID_HANDLE_VALUE && _handle != NULL;
  }
  CFindChangeNotification(): _handle(INVALID_HANDLE_VALUE) {}
  ~CFindChangeNotification() { Close(); }
  bool Close() throw();
  HANDLE FindFirst(CFSTR pathName, bool watchSubtree, DWORD notifyFilter);
  bool FindNext() { return BOOLToBool(::FindNextChangeNotification(_handle)); }
};

bool MyGetLogicalDriveStrings(CObjectVector<FString> &driveStrings);
#endif // ! __DOS__

typedef CFileInfo CDirEntry;

}}}

#endif // ZIP7_INC_DOS_FILE_FIND_H