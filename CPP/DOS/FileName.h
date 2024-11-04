// 7-Zip FileName.h for DOS

#ifndef ZIP7_INC_DOS_FILE_NAME_H
#define ZIP7_INC_DOS_FILE_NAME_H

#include "../Common/MyString.h"

#define IS_LETTER_CHAR(c) ((((unsigned)(int)(c) | 0x20) - (unsigned)'a' <= (unsigned)('z' - 'a')))

namespace NDOS {
namespace NFile {
namespace NName {

int FindSepar(const wchar_t *s) throw();
#ifndef USE_UNICODE_FSTRING
int FindSepar(const FChar *s) throw();
#endif

void NormalizeDirPathPrefix(FString &dirPath); // ensures that it ended with '\\', if dirPath is not epmty
void NormalizeDirPathPrefix(UString &dirPath);

// @FIXME:  Implement NormalizeDirSeparators for  __DOS__
#ifdef _WIN32
void NormalizeDirSeparators(FString &s);
#endif

bool IsDrivePath(const wchar_t *s) throw();  // first 3 chars are drive chars like "a:\\"
bool IsDrivePath2(CFSTR s) throw();

bool IsAltPathPrefix(CFSTR s) throw(); /* name: */
bool IsAbsolutePath(const wchar_t *s) throw();
unsigned GetRootPrefixSize(const wchar_t *s) throw();

#ifdef Z7_LONG_PATH

const int kSuperPathType_UseOnlyMain = 0;
const int kSuperPathType_UseOnlySuper = 1;
const int kSuperPathType_UseMainAndSuper = 2;

int GetUseSuperPathType(CFSTR s) throw();
bool GetSuperPath(CFSTR path, UString &superPath, bool onlyIfNew);
bool GetSuperPaths(CFSTR s1, CFSTR s2, UString &d1, UString &d2, bool onlyIfNew);

#define USE_MAIN_PATH (_useSuperPathType != kSuperPathType_UseOnlySuper)
#define USE_MAIN_PATH_2 (_useSuperPathType1 != kSuperPathType_UseOnlySuper && _useSuperPathType2 != kSuperPathType_UseOnlySuper)

#define USE_SUPER_PATH (_useSuperPathType != kSuperPathType_UseOnlyMain)
#define USE_SUPER_PATH_2 (_useSuperPathType1 != kSuperPathType_UseOnlyMain || _useSuperPathType2 != kSuperPathType_UseOnlyMain)

#define IF_USE_MAIN_PATH int _useSuperPathType = GetUseSuperPathType(path); if (USE_MAIN_PATH)
#define IF_USE_MAIN_PATH_2(x1, x2) \
    int _useSuperPathType1 = GetUseSuperPathType(x1); \
    int _useSuperPathType2 = GetUseSuperPathType(x2); \
    if (USE_MAIN_PATH_2)

#else

#define IF_USE_MAIN_PATH
#define IF_USE_MAIN_PATH_2(x1, x2)

#endif // Z7_LONG_PATH

/*
  if (dirPrefix != NULL && (path) is relative)
  {
    (dirPrefix) will be used
    result (fullPath) will contain prefix part of (dirPrefix).
  }
  Current_Dir path can be used in 2 cases:
    1) if (path) is relative && dirPrefix == NULL
    2) for _WIN32: if (path) is absolute starting wuth "\"
*/
bool GetFullPath(CFSTR dirPrefix, CFSTR path, FString &fullPath);
bool GetFullPath(CFSTR path, FString &fullPath);

}}}

#endif // ZIP7_INC_DOS_FILE_NAME_H