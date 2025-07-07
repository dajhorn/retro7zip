// 7-Zip FileDir.h for DOS.

#ifndef ZIP7_INC_DOS_FILE_DIR_H
#define ZIP7_INC_DOS_FILE_DIR_H

#include <stdio.h>
#include <unistd.h>

#include "../Common/MyString.h"
#include "FileIO.h"

#define RemoveDirAlways_if_Empty RemoveDir 

namespace NDOS {
namespace NFile {
namespace NDir {

bool CreateComplexDir(CFSTR path);
bool CreateTempFile2(CFSTR prefix, bool addRandom, AString &postfix, NIO::COutFile *outFile);
bool GetCurrentDir(FString &resultPath);
bool GetFullPathAndSplit(CFSTR path, FString &resDirPrefix, FString &resFileName);
bool MyCreateHardLink(CFSTR newFileName, CFSTR existFileName);
bool MyGetFullPathName(CFSTR path, FString &resFullPath);
bool MyGetTempPath(FString &resultPath);
bool MyMoveFile(CFSTR existFileName, CFSTR newFileName);
// bool RemoveDirWithSubItems(const FString &path);
bool SetDirTime(CFSTR path, const CFiTime *cTime, const CFiTime *aTime, const CFiTime *mTime);
bool SetFileAttrib(CFSTR path, DWORD attrib);
bool SetFileAttrib_PosixHighDetect(CFSTR path, DWORD attrib);


inline bool CreateDir(CFSTR path)        { return (mkdir(path)  == 0); }
inline bool CreateDir2(CFSTR path)       { return (mkdir(path)  == 0); }
inline bool DeleteFileAlways(CFSTR path) { return (remove(path) == 0); }
inline bool RemoveDir(CFSTR path)        { return (rmdir(path)  == 0); }
inline bool SetCurrentDir(CFSTR path)    { return (chdir(path)  == 0); }


inline bool GetOnlyDirPrefix(CFSTR path, FString &resDirPrefix) {
	FString resFileName; /* This value is discarded. */
	return GetFullPathAndSplit(path, resDirPrefix, resFileName);
}


class CTempFile  MY_UNCOPYABLE
{
  bool _mustBeDeleted;
  FString _path;
  void DisableDeleting() { _mustBeDeleted = false; }

public:
  CTempFile(): _mustBeDeleted(false) {}
  ~CTempFile() { Remove(); }
  const FString &GetPath() const { return _path; }
  bool CreateRandomInTempFolder(CFSTR namePrefix, NIO::COutFile *outFile);
  bool Remove();
  bool MoveTo(CFSTR name, bool deleteDestBefore);
};


class CCurrentDirRestorer  MY_UNCOPYABLE
{
  FString _path;

public:
  bool NeedRestore;

  CCurrentDirRestorer(): NeedRestore(true)
  {
    GetCurrentDir(_path);
  }

  ~CCurrentDirRestorer()
  {
    if (!NeedRestore)
      return;
    FString s;
    if (GetCurrentDir(s))
      if (s != _path)
        SetCurrentDir(_path);
  }
};


}}}

#endif // ZIP7_INC_DOS_FILE_DIR_H
