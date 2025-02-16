// 7-Zip FileFile.cpp for DOS.

#include "StdAfx.h"
#include <fcntl.h>

using namespace NDOS;
using namespace NFile;
using namespace NName;

namespace NDOS {
namespace NFile {
namespace NFind {


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


struct timespec timespec_from_dos(unsigned short wr_date, unsigned short wr_time)
{
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
  /*
   * @NOTE: DOS attribute bits are a proper subset of WIN32 attribute bits,
   *        except that bit 0x80 explicitly indicates a normal file in WIN32.
   */
  fi.Attrib = fd.attrib;

  /* DOS does not implement creation time nor access time. */
  // fi.CTime
  // fi.ATime

  fi.MTime = timespec_from_dos(fd.wr_date, fd.wr_time);
  fi.Name = fas2fs(fd.name);
  fi.Size = fd.size;

  /* DOS does not implement file streams. */
  fi.IsAltStream = false;

  /* DOS does not implement devices. */
  fi.IsDevice = false;
}


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


DWORD GetFileAttrib(CFSTR path)
{
  unsigned dos_attributes = _A_NORMAL;
  _dos_getfileattr(path, &dos_attributes);
  return dos_attributes;
}


bool CFileInfo::Find(CFSTR path, bool followLink)
{
  CFindFile finder;

  /* @FIXME: The FileName.cpp namespace is choking on narrow strings. */
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
  /*
   * If the DOS host lacks LFN support or is in SFN mode, then a single
   * asterisk globs only file names that lack a file extension, and a
   * double asterisk globs all file names.
   *
   * If the DOS host is in LFN mode, then a single asterisk globs all
   * file names like it does on posix and win32 hosts.
   */

  _wildcard = dirPrefix;

  if (NDOS::NSystem::LongFileNames()) {
    _wildcard += "*";
  } else {
    _wildcard += "*.*";
  }
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
  Next(fi) ? found = true : found = false;
  return true;
}


}}}
