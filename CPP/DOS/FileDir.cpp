// 7-Zip FileDir.cpp for DOS

#include "StdAfx.h"


#include <dos.h>
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
  const char *s;

  // By DOS convention, try %TEMP% first.
  s = getenv("TEMP");
  if (NFind::DoesDirExist_FollowLink(s)) {
    path = s;
	return true;
  }

  // By DOS convention, try %TMP% first.
  s = getenv("TMP");
  if (NFind::DoesDirExist_FollowLink(s)) {
    path = s;
	return true;
  }

  // Else, just use the current working directory.
  path = "." STRING_PATH_SEPARATOR;
  return true;

#if 0
  path = STRING_PATH_SEPARATOR "tmp";
  const char *s;
  if (NFind::DoesDirExist_FollowLink(path))
    s = STRING_PATH_SEPARATOR "tmp" STRING_PATH_SEPARATOR;
  else
    s = "." STRING_PATH_SEPARATOR;
  path = s;
  return true;  
#endif
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

#if 0 // unused
bool CTempFile::Create(CFSTR prefix, NIO::COutFile *outFile)
{
  if (!Remove())
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
#endif

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
  /*
   * @NOTE: In context of the DOS platform, ctime and atime are VFAT features
   * that regular FAT12 and FAT16 filesystems do not have. The Open Watcom v2
   * runtime for DOS cannot access this metadata because the system API does
   * expose it. Discard ctime and atime here; only process mtime.
   */

  if (!mTime) {
    if (cTime)
      return true;
    if (aTime)
      return true;
    // Else all three times are null.
    return false;
  }

  int owc_handle;
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

#if 0
  /*
   * The _dos_open() function always returns EACCES (errno 6) for directories
   * because the DOS platform lacks an API for changing directory metadata.
   */

  if (_dos_open(path, O_RDONLY, &owc_handle) != 0) {
    if (errno == EACCES)
      return true;
    else
      return false;
  }

  if (_dos_setftime(owc_handle, owc_dos_date, owc_dos_time) != 0) {
    _dos_close(owc_handle);
    return false;
  }

  _dos_close(owc_handle);
  return true;

#else
  /*
   * Interrupt 21h Function 57h does the same thing as _dos_setftime except
   * that is does not return an error if the argument is a directory.
   *
   * 7-Zip tries to reset the mtime on all extracted items, so this method
   * is simpler and faster because it does not have open+close overheads. 
   */
    union REGS registers;
    memset(&registers, 0, sizeof(registers));

    registers.w.ax  = 0x5701;
    registers.w.cx  = owc_dos_time;
    registers.w.di  = owc_dos_date;
    registers.x.esi = (unsigned)path;

    int386(0x21, &registers, &registers);

    if (registers.x.cflag & 1) {
        return false;
    }

    return true;
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
  // Some DOS flavors will fail or complain if this bit is not masked out.
  attrib &= ~_A_SUBDIR;

  return _dos_setfileattr(path, attrib) == 0;
}

// #endif

}}}
