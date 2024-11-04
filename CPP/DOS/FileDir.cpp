// 7-Zip FileDir.cpp for DOS

#include "StdAfx.h"


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../Common/StringConvert.h"
#include "../Common/C_FileIO.h"

#include "FileDir.h"
#include "FileFind.h"
#include "FileName.h"

using namespace NDOS;
using namespace NFile;
using namespace NName;

static bool FiTime_To_timespec(const CFiTime *ft, timespec &ts)
{
  if (ft)
  {
    ts = *ft;
    return true;
  }
  // else
  {
    ts.tv_sec = 0;
    ts.tv_nsec =
    #ifdef UTIME_OMIT
      UTIME_OMIT; // -2 keep old timesptamp
    #else
      // UTIME_NOW; -1 // set to the current time
      0;
    #endif
    return false;
  }
}

namespace NDOS {
namespace NFile {
namespace NDir {



static bool CreateDir2(CFSTR path);

bool CreateComplexDir(CFSTR _path)
{
  NFind::CFileInfo fi;
  if (fi.Find(_path))
  {
    if (fi.IsDir())
      return true;
  }

  FString path (_path);

  int pos = path.ReverseFind_PathSepar();
  if (pos >= 0 && (unsigned)pos == path.Len() - 1)
  {
    if (path.Len() == 1)
      return true;
    path.DeleteBack();
  }

  const FString path2 (path);
  pos = (int)path.Len();

  for (;;)
  {
    if (CreateDir2(path))
      break;
    if (::GetLastError() == ERROR_ALREADY_EXISTS)
      return false;
    pos = path.ReverseFind_PathSepar();
    if (pos < 0 || pos == 0)
      return false;    
    path.DeleteFrom((unsigned)pos);
  }

  while (pos < (int)path2.Len())
  {
    int pos2 = NName::FindSepar(path2.Ptr((unsigned)pos + 1));
    if (pos2 < 0)
      pos = (int)path2.Len();
    else
      pos += 1 + pos2;
    path.SetFrom(path2, (unsigned)pos);
    if (!CreateDir(path))
      return false;
  }
  
  return true;
}



bool MyGetFullPathName(CFSTR path, FString &resFullPath)
{
  return GetFullPath(path, resFullPath);
}


bool GetFullPathAndSplit(CFSTR path, FString &resDirPrefix, FString &resFileName)
{
  bool res = MyGetFullPathName(path, resDirPrefix);
  if (!res)
    resDirPrefix = path;
  int pos = resDirPrefix.ReverseFind_PathSepar();
  pos++;
  resFileName = resDirPrefix.Ptr((unsigned)pos);
  resDirPrefix.DeleteFrom((unsigned)pos);
  return res;
}

bool GetOnlyDirPrefix(CFSTR path, FString &resDirPrefix)
{
  FString resFileName;
  return GetFullPathAndSplit(path, resDirPrefix, resFileName);
}


bool MyGetTempPath(FString &path)
{
  // @FIXME: Improve this code. Use %TEMP%.
  path = STRING_PATH_SEPARATOR "tmp";
  const char *s;
  if (NFind::DoesDirExist_FollowLink(path))
    s = STRING_PATH_SEPARATOR "tmp" STRING_PATH_SEPARATOR;
  else
    s = "." STRING_PATH_SEPARATOR;
  path = s;
  return true;  
}

bool CreateTempFile2(CFSTR prefix, bool addRandom, AString &postfix, NIO::COutFile *outFile)
{
  UInt32 d = (UInt32)time(NULL);

  for (unsigned i = 0; i < 100; i++)
  {
    postfix.Empty();
    if (addRandom)
    {
      char s[16];
      UInt32 val = d;
      unsigned k;
      for (k = 0; k < 8; k++)
      {
        const unsigned t = (unsigned)val & 0xF;
        val >>= 4;
        s[k] = (char)((t < 10) ? ('0' + t) : ('A' + (t - 10)));
      }
      s[k] = '\0';
      if (outFile)
        postfix.Add_Dot();
      postfix += s;
      UInt32 step = GetTickCount() + 2;
      if (step == 0)
        step = 1;
      d += step;
    }
    addRandom = true;
    if (outFile)
      postfix += ".tmp";
    FString path (prefix);
    path += postfix;
    if (NFind::DoesFileOrDirExist(path))
    {
      SetLastError(ERROR_ALREADY_EXISTS);
      continue;
    }
    if (outFile)
    {
      if (outFile->Create_NEW(path))
        return true;
    }
    else
    {
      if (CreateDir(path))
        return true;
    }
    const DWORD error = GetLastError();
    if (error != ERROR_FILE_EXISTS &&
        error != ERROR_ALREADY_EXISTS)
      break;
  }
  postfix.Empty();
  return false;
}

bool CTempFile::Create(CFSTR prefix, NIO::COutFile *outFile)
{
  if (!Remove())
    return false;
  _path.Empty();
  AString postfix;
  if (!CreateTempFile2(prefix, false, postfix, outFile))
    return false;
  _path = prefix;
  _path += postfix;
  _mustBeDeleted = true;
  return true;
}

bool CTempFile::CreateRandomInTempFolder(CFSTR namePrefix, NIO::COutFile *outFile)
{
  if (!Remove())
    return false;
  _path.Empty();
  FString tempPath;
  if (!MyGetTempPath(tempPath))
    return false;
  AString postfix;
  tempPath += namePrefix;
  if (!CreateTempFile2(tempPath, true, postfix, outFile))
    return false;
  _path = tempPath;
  _path += postfix;
  _mustBeDeleted = true;
  return true;
}

bool CTempFile::Remove()
{
  if (!_mustBeDeleted)
    return true;
  _mustBeDeleted = !DeleteFileAlways(_path);
  return !_mustBeDeleted;
}

bool CTempFile::MoveTo(CFSTR name, bool deleteDestBefore)
{
  // DWORD attrib = 0;
  if (deleteDestBefore)
  {
    if (NFind::DoesFileExist_Raw(name))
    {
      // attrib = NFind::GetFileAttrib(name);
      if (!DeleteFileAlways(name))
        return false;
    }
  }
  DisableDeleting();
  return MyMoveFile(_path, name);
  
  /*
  if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_READONLY))
  {
    DWORD attrib2 = NFind::GetFileAttrib(name);
    if (attrib2 != INVALID_FILE_ATTRIBUTES)
      SetFileAttrib(name, attrib2 | FILE_ATTRIBUTE_READONLY);
  }
  */
}

bool RemoveDir(CFSTR path)
{
  return (rmdir(path) == 0);
}


static BOOL My_CopyFile(CFSTR oldFile, CFSTR newFile)
{
  NDOS::NFile::NIO::COutFile outFile;
  if (!outFile.Create_NEW(newFile))
    return FALSE;
  
  NDOS::NFile::NIO::CInFile inFile;
  if (!inFile.Open(oldFile))
    return FALSE;

  char buf[1 << 14];

  for (;;)
  {
    const ssize_t num = inFile.read_part(buf, sizeof(buf));
    if (num == 0)
      return TRUE;
    if (num < 0)
      return FALSE;
    size_t processed;
    const ssize_t num2 = outFile.write_full(buf, (size_t)num, processed);
    if (num2 != num || processed != (size_t)num)
      return FALSE;
  }
}


bool MyMoveFile(CFSTR oldFile, CFSTR newFile)
{
  int res = rename(oldFile, newFile);
  if (res == 0)
    return true;
  if (errno != EXDEV) // (oldFile and newFile are not on the same mounted filesystem)
    return false;

  if (My_CopyFile(oldFile, newFile) == FALSE)
    return false;
    
  struct stat info_file;
  res = stat(oldFile, &info_file);
  if (res != 0)
    return false;

  /*
  ret = chmod(dst,info_file.st_mode & g_umask.mask);
  */
  return (unlink(oldFile) == 0);
}


bool CreateDir(CFSTR path)
{
  return (mkdir(path) == 0);
}

static bool CreateDir2(CFSTR path)
{
  return (mkdir(path) == 0);
}


bool DeleteFileAlways(CFSTR path)
{
  return (remove(path) == 0);
}

bool SetCurrentDir(CFSTR path)
{
  return (chdir(path) == 0);
}


bool GetCurrentDir(FString &path)
{
  path.Empty();

  #define MY_PATH_MAX  PATH_MAX
  // #define MY_PATH_MAX  1024

  char s[MY_PATH_MAX + 1];
  char *res = getcwd(s, MY_PATH_MAX);
  if (res)
  {
    path = fas2fs(s);
    return true;
  }
  {
    // if (errno != ERANGE) return false;
    return false;
  }
}



// #undef UTIME_OMIT // to debug

#ifndef UTIME_OMIT
  /* we can define UTIME_OMIT for debian and another systems.
     Is it OK to define UTIME_OMIT to -2 here, if UTIME_OMIT is not defined? */
  // #define UTIME_OMIT -2
#endif





bool SetDirTime(CFSTR path, const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime)
{
  // need testing
  /*
  struct utimbuf buf;
  struct stat st;
  UNUSED_VAR(cTime)
 
  printf("\nstat = %s\n", path);
  int ret = stat(path, &st);

  if (ret == 0)
  {
    buf.actime  = st.st_atime;
    buf.modtime = st.st_mtime;
  }
  else
  {
    time_t cur_time = time(0);
    buf.actime  = cur_time;
    buf.modtime = cur_time;
  }

  if (aTime)
  {
    UInt32 ut;
    if (NTime::FileTimeToUnixTime(*aTime, ut))
      buf.actime = ut;
  }

  if (mTime)
  {
    UInt32 ut;
    if (NTime::FileTimeToUnixTime(*mTime, ut))
      buf.modtime = ut;
  }

  return utime(path, &buf) == 0;
  */

  // if (!aTime && !mTime) return true;

  struct timespec times[2];
  UNUSED_VAR(cTime)
  
  bool needChange;
  needChange  = FiTime_To_timespec(aTime, times[0]);
  needChange |= FiTime_To_timespec(mTime, times[1]);

  /*
  if (mTime)
  {
    printf("\n time = %ld.%9ld\n", mTime->tv_sec, mTime->tv_nsec);
  }
  */

  /* @FIXME: __DOS__ compatibility */
  return true;

#if 0
  if (!needChange)
    return true;
  const int flags = 0; // follow link
    // = AT_SYMLINK_NOFOLLOW; // don't follow link
  return utimensat(AT_FDCWD, path, times, flags) == 0;
#endif
}



struct C_umask
{
  mode_t mask;

  C_umask()
  {
    /* by security reasons we restrict attributes according
       with process's file mode creation mask (umask) */
    const mode_t um = umask(0); // octal :0022 is expected
    mask = 0777 & (~um);        // octal: 0755 is expected
    umask(um);  // restore the umask
    // printf("\n umask = 0%03o mask = 0%03o\n", um, mask);
    
    // mask = 0777; // debug we can disable the restriction:
  }
};

static C_umask g_umask;

// #define PRF(x) x;
#define PRF(x)

#define TRACE_SetFileAttrib(msg) \
  PRF(printf("\nSetFileAttrib(%s, %x) : %s\n", (const char *)path, attrib, msg);)

#define TRACE_chmod(s, mode) \
  PRF(printf("\n chmod(%s, %o)\n", (const char *)path, (unsigned)(mode));)

int my_chown(CFSTR path, uid_t owner, gid_t group)
{
  return 0;
}

bool SetFileAttrib_PosixHighDetect(CFSTR path, DWORD attrib)
{
  TRACE_SetFileAttrib("")

  struct stat st;

  bool use_lstat = true;
  if (use_lstat)
  {
    if (lstat(path, &st) != 0)
    {
      TRACE_SetFileAttrib("bad lstat()")
      return false;
    }
    // TRACE_chmod("lstat", st.st_mode);
  }
  else
  {
    if (stat(path, &st) != 0)
    {
      TRACE_SetFileAttrib("bad stat()")
      return false;
    }
  }
  
  if (attrib & FILE_ATTRIBUTE_UNIX_EXTENSION)
  {
    TRACE_SetFileAttrib("attrib & FILE_ATTRIBUTE_UNIX_EXTENSION")
    st.st_mode = attrib >> 16;
    if (S_ISDIR(st.st_mode))
    {
      // user/7z must be able to create files in this directory
      st.st_mode |= (S_IRUSR | S_IWUSR | S_IXUSR);
    }
    else if (!S_ISREG(st.st_mode))
      return true;
  }
  else if (S_ISLNK(st.st_mode))
  {
    /* for most systems: permissions for symlinks are fixed to rwxrwxrwx.
       so we don't need chmod() for symlinks. */
    return true;
    // SetLastError(ENOSYS);
    // return false;
  }
  else
  {
    TRACE_SetFileAttrib("Only Windows Attributes")
    // Only Windows Attributes
    if (S_ISDIR(st.st_mode)
        || (attrib & FILE_ATTRIBUTE_READONLY) == 0)
      return true;
    st.st_mode &= ~(mode_t)(S_IWUSR | S_IWGRP | S_IWOTH); // octal: ~0222; // disable write permissions
  }

  int res;
  /*
  if (S_ISLNK(st.st_mode))
  {
    printf("\nfchmodat()\n");
    TRACE_chmod(path, (st.st_mode) & g_umask.mask)
    // AT_SYMLINK_NOFOLLOW is not implemted still in Linux.
    res = fchmodat(AT_FDCWD, path, (st.st_mode) & g_umask.mask,
        S_ISLNK(st.st_mode) ? AT_SYMLINK_NOFOLLOW : 0);
  }
  else
  */
  {
    TRACE_chmod(path, (st.st_mode) & g_umask.mask)
    res = chmod(path, (st.st_mode) & g_umask.mask);
  }
  // TRACE_SetFileAttrib("End")
  return (res == 0);
}


bool MyCreateHardLink(CFSTR newFileName, CFSTR existFileName)
{
  return false;
}

// #endif

}}}
