// 7-Zip FileFile.cpp for DOS

#include "StdAfx.h"


#include <fcntl.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#include "TimeUtils.h"
#include "FileFind.h"
#include "FileIO.h"
#include "FileName.h"

using namespace NDOS;
using namespace NFile;
using namespace NName;

namespace NDOS {
namespace NFile {
namespace NFind {

/*
#ifdef _WIN32
#define MY_CLEAR_FILETIME(ft) ft.dwLowDateTime = ft.dwHighDateTime = 0;
#else
#define MY_CLEAR_FILETIME(ft) ft.tv_sec = 0;  ft.tv_nsec = 0;
#endif
*/

void CFileInfoBase::ClearBase() throw()
{
  Size = 0;
  FiTime_Clear(CTime);
  FiTime_Clear(ATime);
  FiTime_Clear(MTime);
 
  Attrib = 0;
  IsAltStream = false;
  IsDevice = false;
}


bool CFileInfoBase::SetAs_StdInFile()
{
  return false;
  
#if 0
  // @FIXME:  stdin for dos
  ClearBase();
  Size = (UInt64)(Int64)-1;
  NTime::GetCurUtc_FiTime(MTime);
  CTime = ATime = MTime;

  mode = S_IFIFO | 0777; // 0755 : 0775 : 0664 : 0644 :

  struct stat st;
  if (fstat(0, &st) == 0)
  {
    SetFrom_stat(st);
    if (!S_ISREG(st.st_mode)
        // S_ISFIFO(st->st_mode)
        || st.st_size == 0)
    {
      Size = (UInt64)(Int64)-1;
      // mode = S_IFIFO | 0777;
    }
  }

  return true;
#endif
}

bool CFileInfo::IsDots() const throw()
{
  if (!IsDir() || Name.IsEmpty())
    return false;
  if (Name[0] != '.')
    return false;
  return Name.Len() == 1 || (Name.Len() == 2 && Name[1] == '.');
}

struct timespec timespec_from_dos(unsigned short wr_date, unsigned short wr_time) {
    struct timespec ts;
    struct tm tm_time;

    // Extract date components
    tm_time.tm_year = ((wr_date >> 9) & 0x7F) + 80;  // Years since 1900
    tm_time.tm_mon = ((wr_date >> 5) & 0x0F) - 1;    // 0-11
    tm_time.tm_mday = wr_date & 0x1F;                // 1-31

    // Extract time components
    tm_time.tm_hour = (wr_time >> 11) & 0x1F;        // 0-23
    tm_time.tm_min = (wr_time >> 5) & 0x3F;          // 0-59
    tm_time.tm_sec = (wr_time & 0x1F) * 2;           // 0-58 (in 2-second increments)

    // Let the system determine DST
    tm_time.tm_isdst = -1;

    // Set timespec
    ts.tv_sec = mktime(&tm_time);
	
	// DOS time doesn't have sub-second precision
    ts.tv_nsec = 0;  

    return ts;
}


static void Convert_DOS_FIND_DATA_to_FileInfo(struct find_t &fd, CFileInfo &fi)
{
  // @NOTE: DOS attribute bits are a proper subset of WIN32 attribute bits,
  // except that bit 0x80 explicitly indicates a normal file in WIN32.

  // Convert DOS file attributes to WIN32 file attributes.
#if 0
  if (fd.attrib == _A_NORMAL)
	  fi.Attrib = FILE_ATTRIBUTE_NORMAL;
  else
	  fi.Attrib = fd.attrib;
#endif
  fi.Attrib = fd.attrib;

  // DOS does not implement creation time nor access time.  
  // fi.CTime
  // fi.ATime

  // Convert file modification time.
  fi.MTime = timespec_from_dos(fd.wr_date, fd.wr_time);

  fi.Name = fas2fs(fd.name);

  fi.Size = fd.size;

  // DOS does not implement file streams.
  fi.IsAltStream = false;
  
  // DOS does not implement devices.
  fi.IsDevice = false;
}

////////////////////////////////
// CFindFile

bool CFindFileBase::Close() throw()
{
  if(_find_allocated && _dos_findclose(&_find_buffer) != 0)
	  return false;
  
  _find_allocated = false;
  return true;
}

bool CFindFile::FindFirst(CFSTR path, CFileInfo &fi)
{
  /* Return everything except _A_VOLID. */
  const unsigned findmask = \
    _A_NORMAL | \
    _A_RDONLY | \
    _A_HIDDEN | \
    _A_SYSTEM | \
    _A_SUBDIR | \
    _A_ARCH;

  if (_find_allocated || _dos_findfirst(fas2fs(path), findmask, &_find_buffer) != 0)
    return false;
 
  Convert_DOS_FIND_DATA_to_FileInfo(_find_buffer, fi);  
  _find_allocated = true;
  return true;
}

bool CFindFile::FindNext(CFileInfo &fi)
{
  if (_find_allocated && _dos_findnext(&_find_buffer) != 0)
    return false;

  Convert_DOS_FIND_DATA_to_FileInfo(_find_buffer, fi);
  return true;
}


/*
WinXP-64 GetFileAttributes():
  If the function fails, it returns INVALID_FILE_ATTRIBUTES and use GetLastError() to get error code

  \    - OK
  C:\  - OK, if there is such drive,
  D:\  - ERROR_PATH_NOT_FOUND, if there is no such drive,

  C:\folder     - OK
  C:\folder\    - OK
  C:\folderBad  - ERROR_FILE_NOT_FOUND

  \\Server\BadShare  - ERROR_BAD_NETPATH
  \\Server\Share     - WORKS OK, but MSDN says:
                          GetFileAttributes for a network share, the function fails, and GetLastError
                          returns ERROR_BAD_NETPATH. You must specify a path to a subfolder on that share.
*/

DWORD GetFileAttrib(CFSTR path)
{
  unsigned dos_attributes = _A_NORMAL;
  _dos_getfileattr(path, &dos_attributes);
  return dos_attributes;
}

/* if path is "c:" or "c::" then CFileInfo::Find() returns name of current folder for that disk
   so instead of absolute path we have relative path in Name. That is not good in some calls */

/* In CFileInfo::Find() we want to support same names for alt streams as in CreateFile(). */

/* CFileInfo::Find()
We alow the following paths (as FindFirstFile):
  C:\folder
  c:                      - if current dir is NOT ROOT ( c:\folder )

also we support paths that are not supported by FindFirstFile:
  \
  \\.\c:
  c:\                     - Name will be without tail slash ( c: )
  \\?\c:\                 - Name will be without tail slash ( c: )
  \\Server\Share
  \\?\UNC\Server\Share

  c:\folder:stream  - Name = folder:stream
  c:\:stream        - Name = :stream
  c::stream         - Name = c::stream
*/

bool CFileInfo::Find(CFSTR path, bool followLink)
{
  CFindFile finder;

  #if defined(_WIN32) && !defined(UNDER_CE) || defined(__DOS__)
  {
    
	// @FIXME: The FileName.cpp namespace is choking on narrow strings
    // if (NName::IsDrivePath(path + rootSize) && path[rootSize + 3] == 0)
    if (IS_LETTER_CHAR(path[0]) && path[1] == ':' && IS_PATH_SEPAR(path[2]) && path[3] == 0) 
    {
      DWORD attrib = GetFileAttrib(path);
      if (attrib & FILE_ATTRIBUTE_DIRECTORY)
      {
        ClearBase();
        Attrib = attrib;
        Name = path;
        Name.DeleteFrom(2);
        if (!Fill_From_ByHandleFileInfo(path))
        {
        }
        return true;
      }
    }
    else if (IS_PATH_SEPAR(path[0]))
    {
      if (path[1] == 0)
      {
        DWORD attrib = GetFileAttrib(path);
        if (attrib & FILE_ATTRIBUTE_DIRECTORY)
        {
          ClearBase();
          Name.Empty();
          Attrib = attrib;
          return true;
        }
      }
    }
  }
  #endif

  bool res = finder.FindFirst(path, *this);
  if (!followLink
      || !res
      || !HasReparsePoint())
    return res;

  return Fill_From_ByHandleFileInfo(path);
}

bool CFileInfoBase::Fill_From_ByHandleFileInfo(CFSTR path)
{
  unsigned dos_attributes = _A_NORMAL;
  int dos_handle = NULL;
  unsigned dos_date, dos_time;

  if (_dos_getfileattr(path, &dos_attributes) != 0)
    return false;

  if (_dos_open(path, O_RDONLY, &dos_handle) != 0)
    return false;

  if (_dos_getftime(dos_handle, &dos_date, &dos_time) != 0) {
    _dos_close(dos_handle);
    return false;
  }

  Attrib = dos_attributes;
  MTime = timespec_from_dos(dos_date, dos_time);

  _dos_close(dos_handle);
  return true;
}

bool DoesFileExist_Raw(CFSTR name)
{
  CFileInfo fi;
  return fi.Find(name) && !fi.IsDir();
}

bool DoesFileExist_FollowLink(CFSTR name)
{
  CFileInfo fi;
  return fi.Find_FollowLink(name) && !fi.IsDir();
}

bool DoesDirExist(CFSTR name, bool followLink)
{
  CFileInfo fi;
  return fi.Find(name, followLink) && fi.IsDir();
}

bool DoesFileOrDirExist(CFSTR name)
{
  CFileInfo fi;
  return fi.Find(name);
}

void CEnumerator::SetDirPrefix(const FString &dirPrefix)
{
  _wildcard = dirPrefix;
  _wildcard.Add_Char('*');
}

bool CEnumerator::NextAny(CFileInfo &fi)
{
  if (_findFile.IsHandleAllocated())
    return _findFile.FindNext(fi);
  else
    return _findFile.FindFirst(_wildcard, fi);
}

bool CEnumerator::Next(CFileInfo &fi)
{
  for (;;)
  {
    if (!NextAny(fi))
      return false;
    if (!fi.IsDots())
      return true;
  }
}

bool CEnumerator::Next(CFileInfo &fi, bool &found)
{
  if (Next(fi))
  {
    found = true;
    return true;
  }

  found = false;

  // @FIXME:  Always return true?  Check for correct __DOS__ behavior.
  return true;



#if !defined(__DOS__)
  DWORD lastError = ::GetLastError();
  
  if (_findFile.IsHandleAllocated())
    return (lastError == ERROR_NO_MORE_FILES);
  // we support the case for empty root folder: FindFirstFile("c:\\*") returns ERROR_FILE_NOT_FOUND


  if (lastError == ERROR_FILE_NOT_FOUND)
    return true;

  /* This never happens in DOS. */
  if (lastError == ERROR_ACCESS_DENIED)
  {
    // here we show inaccessible root system folder as empty folder to eliminate redundant user warnings
    const char *s = "System Volume Information" STRING_PATH_SEPARATOR "*";
    const int len = (int)strlen(s);
    const int delta = (int)_wildcard.Len() - len;
    if (delta == 0 || (delta > 0 && IS_PATH_SEPAR(_wildcard[(unsigned)delta - 1])))
      if (StringsAreEqual_Ascii(_wildcard.Ptr((unsigned)delta), s))
        return true;
  }
  
  return false;
#endif // __DOS__
}


#if !defined(__DOS__)

////////////////////////////////
// CFindChangeNotification
// FindFirstChangeNotification can return 0. MSDN doesn't tell about it.

bool CFindChangeNotification::Close() throw()
{
  if (!IsHandleAllocated())
    return true;
  if (!::FindCloseChangeNotification(_handle))
    return false;
  _handle = INVALID_HANDLE_VALUE;
  return true;
}
           
HANDLE CFindChangeNotification::FindFirst(CFSTR path, bool watchSubtree, DWORD notifyFilter)
{
#if defined(__WATCOMC__)
    _handle = ::FindFirstChangeNotification(fs2fas(path), BoolToBOOL(watchSubtree), notifyFilter);
#else
  #ifndef _UNICODE
  if (!g_IsNT)
    _handle = ::FindFirstChangeNotification(fs2fas(path), BoolToBOOL(watchSubtree), notifyFilter);
  else
  #endif
  {
    IF_USE_MAIN_PATH
    _handle = ::FindFirstChangeNotificationW(fs2us(path), BoolToBOOL(watchSubtree), notifyFilter);
    #ifdef Z7_LONG_PATH
    if (!IsHandleAllocated())
    {
      UString superPath;
      if (GetSuperPath(path, superPath, USE_MAIN_PATH))
        _handle = ::FindFirstChangeNotificationW(superPath, BoolToBOOL(watchSubtree), notifyFilter);
    }
    #endif
  }
#endif // defined(__WATCOMC__)
  return _handle;
}

#ifndef UNDER_CE

bool MyGetLogicalDriveStrings(CObjectVector<FString> &driveStrings)
{
  driveStrings.Clear();
#if defined(__WATCOMC__)
    driveStrings.Clear();
    UINT32 size = GetLogicalDriveStrings(0, NULL);
    if (size == 0)
      return false;
    CObjArray<char> buf(size);
    UINT32 newSize = GetLogicalDriveStrings(size, buf);
    if (newSize == 0 || newSize > size)
      return false;
    AString s;
    UINT32 prev = 0;
    for (UINT32 i = 0; i < newSize; i++)
    {
      if (buf[i] == 0)
      {
        s = buf + prev;
        prev = i + 1;
        driveStrings.Add(fas2fs(s));
      }
    }
    return prev == newSize;
#else
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    driveStrings.Clear();
    UINT32 size = GetLogicalDriveStrings(0, NULL);
    if (size == 0)
      return false;
    CObjArray<char> buf(size);
    UINT32 newSize = GetLogicalDriveStrings(size, buf);
    if (newSize == 0 || newSize > size)
      return false;
    AString s;
    UINT32 prev = 0;
    for (UINT32 i = 0; i < newSize; i++)
    {
      if (buf[i] == 0)
      {
        s = buf + prev;
        prev = i + 1;
        driveStrings.Add(fas2fs(s));
      }
    }
    return prev == newSize;
  }
  else
  #endif
  {
    UINT32 size = GetLogicalDriveStringsW(0, NULL);
    if (size == 0)
      return false;
    CObjArray<wchar_t> buf(size);
    UINT32 newSize = GetLogicalDriveStringsW(size, buf);
    if (newSize == 0 || newSize > size)
      return false;
    UString s;
    UINT32 prev = 0;
    for (UINT32 i = 0; i < newSize; i++)
    {
      if (buf[i] == 0)
      {
        s = buf + prev;
        prev = i + 1;
        driveStrings.Add(us2fs(s));
      }
    }
    return prev == newSize;
  }
#endif // defined(__WATCOMC__)
}

#endif // UNDER_CE

#endif // ! __DOS__


}}}
