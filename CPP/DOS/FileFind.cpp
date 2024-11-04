// 7-Zip FileFile.cpp for DOS

#include "StdAfx.h"

// #include <stdio.h>

#include <fcntl.h>           /* Definition of AT_* constants */
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

void CFileInfoBase::ClearBase() throw()
{
  Size = 0;
  FiTime_Clear(CTime);
  FiTime_Clear(ATime);
  FiTime_Clear(MTime);
 
 #ifdef _WIN32
  Attrib = 0;
  // ReparseTag = 0;
  IsAltStream = false;
  IsDevice = false;
 #else
  dev = 0;
  ino = 0;
  mode = 0;
  nlink = 0;
  uid = 0;
  gid = 0;
  rdev = 0;
 #endif
}


bool CFileInfoBase::SetAs_StdInFile()
{
  ClearBase();
  Size = (UInt64)(Int64)-1;
  NTime::GetCurUtc_FiTime(MTime);
  CTime = ATime = MTime;


// @FIXME:  Attrib should be used instead of mode for __DOS__

#ifdef _WIN32

  /* in GUI mode : GetStdHandle(STD_INPUT_HANDLE) returns NULL,
     and it doesn't set LastError.  */
#if 1
  SetLastError(0);
  const HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
  if (!h || h == INVALID_HANDLE_VALUE)
  {
    if (GetLastError() == 0)
      SetLastError(ERROR_INVALID_HANDLE);
    return false;
  }
  BY_HANDLE_FILE_INFORMATION info;
  if (GetFileInformationByHandle(h, &info)
      && info.dwVolumeSerialNumber)
  {
    Size = (((UInt64)info.nFileSizeHigh) << 32) + info.nFileSizeLow;
    // FileID_Low = (((UInt64)info.nFileIndexHigh) << 32) + info.nFileIndexLow;
    // NumLinks = SupportHardLinks ? info.nNumberOfLinks : 1;
    Attrib = info.dwFileAttributes;
    CTime = info.ftCreationTime;
    ATime = info.ftLastAccessTime;
    MTime = info.ftLastWriteTime;
  }
#if 0
  printf(
    "\ndwFileAttributes = %8x"
    "\nftCreationTime   = %8x"
    "\nftLastAccessTime = %8x"
    "\nftLastWriteTime  = %8x"
    "\ndwVolumeSerialNumber  = %8x"
    "\nnFileSizeHigh  = %8x"
    "\nnFileSizeLow   = %8x"
    "\nnNumberOfLinks  = %8x"
    "\nnFileIndexHigh  = %8x"
    "\nnFileIndexLow   = %8x \n",
      (unsigned)info.dwFileAttributes,
      (unsigned)info.ftCreationTime.dwHighDateTime,
      (unsigned)info.ftLastAccessTime.dwHighDateTime,
      (unsigned)info.ftLastWriteTime.dwHighDateTime,
      (unsigned)info.dwVolumeSerialNumber,
      (unsigned)info.nFileSizeHigh,
      (unsigned)info.nFileSizeLow,
      (unsigned)info.nNumberOfLinks,
      (unsigned)info.nFileIndexHigh,
      (unsigned)info.nFileIndexLow);
#endif
#endif

#else // non-Wiondow

  mode = S_IFIFO | 0777; // 0755 : 0775 : 0664 : 0644 :
#if 1
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
#endif
#endif

  return true;
}

bool CFileInfo::IsDots() const throw()
{
  if (!IsDir() || Name.IsEmpty())
    return false;
  if (Name[0] != '.')
    return false;
  return Name.Len() == 1 || (Name.Len() == 2 && Name[1] == '.');
}




static int MY_lstat(CFSTR path, struct stat *st, bool followLink)
{
  memset(st, 0, sizeof(*st));
  int res;
  // #ifdef ENV_HAVE_LSTAT
  if (/* global_use_lstat && */ !followLink)
  {
    // printf("\nlstat\n");
    res = lstat(path, st);
  }
  else
  // #endif
  {
    // printf("\nstat\n");
    res = stat(path, st);
  }
#if 0
#if defined(__clang__) && __clang_major__ >= 14
  #pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
#endif

  printf("\n st_dev = %lld", (long long)(st->st_dev));
  printf("\n st_ino = %lld", (long long)(st->st_ino));
  printf("\n st_mode = %llx", (long long)(st->st_mode));
  printf("\n st_nlink = %lld", (long long)(st->st_nlink));
  printf("\n st_uid = %lld", (long long)(st->st_uid));
  printf("\n st_gid = %lld", (long long)(st->st_gid));
  printf("\n st_size = %lld", (long long)(st->st_size));
  printf("\n st_blksize = %lld", (long long)(st->st_blksize));
  printf("\n st_blocks = %lld", (long long)(st->st_blocks));
  printf("\n st_ctim = %lld", (long long)(ST_CTIME((*st)).tv_sec));
  printf("\n st_mtim = %lld", (long long)(ST_MTIME((*st)).tv_sec));
  printf("\n st_atim = %lld", (long long)(ST_ATIME((*st)).tv_sec));
     printf(S_ISFIFO(st->st_mode) ? "\n FIFO" : "\n NO FIFO");
  printf("\n");
#endif

  return res;
}


static const char *Get_Name_from_Path(CFSTR path) throw()
{
  size_t len = strlen(path);
  if (len == 0)
    return path;
  const char *p = path + len - 1;
  {
    if (p == path)
      return path;
    p--;
  }
  for (;;)
  {
    char c = *p;
    if (IS_PATH_SEPAR(c))
      return p + 1;
    if (p == path)
      return path;
    p--;
  }
}


UInt32 Get_WinAttribPosix_From_PosixMode(UInt32 mode)
{
  UInt32 attrib = S_ISDIR(mode) ?
      FILE_ATTRIBUTE_DIRECTORY :
      FILE_ATTRIBUTE_ARCHIVE;
  if ((mode & 0222) == 0) // S_IWUSR in p7zip
    attrib |= FILE_ATTRIBUTE_READONLY;
  return attrib | FILE_ATTRIBUTE_UNIX_EXTENSION | ((mode & 0xFFFF) << 16);
}

/*
UInt32 Get_WinAttrib_From_stat(const struct stat &st)
{
  UInt32 attrib = S_ISDIR(st.st_mode) ?
    FILE_ATTRIBUTE_DIRECTORY :
    FILE_ATTRIBUTE_ARCHIVE;

  if ((st.st_mode & 0222) == 0) // check it !!!
    attrib |= FILE_ATTRIBUTE_READONLY;

  attrib |= FILE_ATTRIBUTE_UNIX_EXTENSION + ((st.st_mode & 0xFFFF) << 16);
  return attrib;
}
*/

void CFileInfoBase::SetFrom_stat(const struct stat &st)
{
  // IsDevice = false;

  if (S_ISDIR(st.st_mode))
  {
    Size = 0;
  }
  else
  {
    Size = (UInt64)st.st_size; // for a symbolic link, size = size of filename
  }

  // Attrib = Get_WinAttribPosix_From_PosixMode(st.st_mode);

  // NTime::UnixTimeToFileTime(st.st_ctime, CTime);
  // NTime::UnixTimeToFileTime(st.st_mtime, MTime);
  // NTime::UnixTimeToFileTime(st.st_atime, ATime);
  // timespec_To_FILETIME(st.st_ctim, CTime, &CTime_ns100);
  // timespec_To_FILETIME(st.st_mtim, MTime, &MTime_ns100);
  // timespec_To_FILETIME(st.st_atim, ATime, &ATime_ns100);
  CTime = dos_time_to_timespec(st.st_ctime);
  MTime = dos_time_to_timespec(st.st_mtime);
  ATime = dos_time_to_timespec(st.st_atime);

  dev = st.st_dev;
  ino = st.st_ino;
  mode = st.st_mode;
  nlink = st.st_nlink;
  uid = st.st_uid;
  gid = st.st_gid;
  rdev = st.st_rdev;

  /*
  printf("\n sizeof timespec = %d", (int)sizeof(timespec));
  printf("\n sizeof st_rdev = %d", (int)sizeof(rdev));
  printf("\n sizeof st_ino = %d", (int)sizeof(ino));
  printf("\n sizeof mode_t = %d", (int)sizeof(mode_t));
  printf("\n sizeof nlink_t = %d", (int)sizeof(nlink_t));
  printf("\n sizeof uid_t = %d", (int)sizeof(uid_t));
  printf("\n");
  */
  /*
  printf("\n st_rdev = %llx", (long long)rdev);
  printf("\n st_dev  = %llx", (long long)dev);
  printf("\n dev  : major = %5x minor = %5x", (unsigned)major(dev), (unsigned)minor(dev));
  printf("\n st_ino = %lld", (long long)(ino));
  printf("\n rdev : major = %5x minor = %5x", (unsigned)major(rdev), (unsigned)minor(rdev));
  printf("\n size = %lld \n", (long long)(Size));
  printf("\n");
  */
}

/*
int Uid_To_Uname(uid_t uid, AString &name)
{
  name.Empty();
  struct passwd *passwd;

  if (uid != 0 && uid == cached_no_such_uid)
    {
      *uname = xstrdup ("");
      return;
    }

  if (!cached_uname || uid != cached_uid)
    {
      passwd = getpwuid (uid);
      if (passwd)
  {
    cached_uid = uid;
    assign_string (&cached_uname, passwd->pw_name);
  }
      else
  {
    cached_no_such_uid = uid;
    *uname = xstrdup ("");
    return;
  }
    }
  *uname = xstrdup (cached_uname);
}
*/

bool CFileInfo::Find_DontFill_Name(CFSTR path, bool followLink)
{
  struct stat st;
  if (MY_lstat(path, &st, followLink) != 0)
    return false;
  // printf("\nFind_DontFill_Name : name=%s\n", path);
  SetFrom_stat(st);
  return true;
}


bool CFileInfo::Find(CFSTR path, bool followLink)
{
  // printf("\nCEnumerator::Find() name = %s\n", path);
  if (!Find_DontFill_Name(path, followLink))
    return false;

  // printf("\nOK\n");

  Name = Get_Name_from_Path(path);
  if (!Name.IsEmpty())
  {
    char c = Name.Back();
    if (IS_PATH_SEPAR(c))
      Name.DeleteBack();
  }
  return true;
}


bool DoesFileExist_Raw(CFSTR name)
{
  // FIXME for symbolic links.
  struct stat st;
  if (MY_lstat(name, &st, false) != 0)
    return false;
  return !S_ISDIR(st.st_mode);
}

bool DoesFileExist_FollowLink(CFSTR name)
{
  // FIXME for symbolic links.
  struct stat st;
  if (MY_lstat(name, &st, true) != 0)
    return false;
  return !S_ISDIR(st.st_mode);
}

bool DoesDirExist(CFSTR name, bool followLink)
{
  struct stat st;
  if (MY_lstat(name, &st, followLink) != 0)
    return false;
  return S_ISDIR(st.st_mode);
}

bool DoesFileOrDirExist(CFSTR name)
{
  struct stat st;
  if (MY_lstat(name, &st, false) != 0)
    return false;
  return true;
}


CEnumerator::~CEnumerator()
{
  if (_dir)
    closedir(_dir);
}

void CEnumerator::SetDirPrefix(const FString &dirPrefix)
{
  _wildcard = dirPrefix;
}

bool CDirEntry::IsDots() const throw()
{
  /* some systems (like CentOS 7.x on XFS) have (Type == DT_UNKNOWN)
     we can call fstatat() for that case, but we use only (Name) check here */

  return Name.Len() != 0
      && Name.Len() <= 2
      && Name[0] == '.'
      && (Name.Len() == 1 || Name[1] == '.');
}


bool CEnumerator::NextAny(CDirEntry &fi, bool &found)
{
  found = false;

  if (!_dir)
  {
    const char *w = "./";
    if (!_wildcard.IsEmpty())
      w = _wildcard.Ptr();
    _dir = ::opendir((const char *)w);
    if (_dir == NULL)
      return false;
  }

  // To distinguish end of stream from an error, we must set errno to zero before readdir()
  errno = 0;

  struct dirent *de = readdir(_dir);
  if (!de)
  {
    if (errno == 0)
      return true; // it's end of stream, and we report it with (found = false)
    // it's real error
    return false;
  }

  fi.iNode = de->d_ino;
  
  /*
  if (de->d_type == DT_DIR)
    fi.Attrib = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_UNIX_EXTENSION | ((UInt32)(S_IFDIR) << 16);
  else if (de->d_type < 16)
    fi.Attrib = FILE_ATTRIBUTE_UNIX_EXTENSION | ((UInt32)(de->d_type) << (16 + 12));
  */
  fi.Name = de->d_name;

  /*
  printf("\nCEnumerator::NextAny; len = %d %s \n", (int)fi.Name.Len(), fi.Name.Ptr());
  for (unsigned i = 0; i < fi.Name.Len(); i++)
    printf (" %02x", (unsigned)(Byte)de->d_name[i]);
  printf("\n");
  */

  found = true;
  return true;
}


bool CEnumerator::Next(CDirEntry &fi, bool &found)
{
  // printf("\nCEnumerator::Next()\n");
  // PrintName("Next", "");
  for (;;)
  {
    if (!NextAny(fi, found))
      return false;
    if (!found)
      return true;
    if (!fi.IsDots())
    {
      /*
      if (!NeedFullStat)
        return true;
      // we silently skip error file here - it can be wrong link item
      if (fi.Find_DontFill_Name(path))
        return true;
      */
      return true;
    }
  }
}

/*
bool CEnumerator::Next(CDirEntry &fileInfo, bool &found)
{
  bool found;
  if (!Next(fi, found))
    return false;
  return found;
}
*/

bool CEnumerator::Fill_FileInfo(const CDirEntry &de, CFileInfo &fileInfo, bool followLink) const
{
  // printf("\nCEnumerator::Fill_FileInfo()\n");
  struct stat st;
  // probably it's OK to use fstatat() even if it changes file position dirfd(_dir)
  // if fstatat() is not supported, we can use stat() / lstat()

  /* @FIXME: What is +s supposed to be? */ 
  // const FString path = _wildcard + s;
  const FString path = _wildcard;
  int res = MY_lstat(path, &st, followLink);
  
  if (res != 0)
    return false;
  // printf("\nname=%s\n", de.Name.Ptr());
  fileInfo.SetFrom_stat(st);
  fileInfo.Name = de.Name;
  return true;
}


}}}
