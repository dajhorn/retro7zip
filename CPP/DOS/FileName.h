// 7-Zip FileName.h for DOS.

#ifndef ZIP7_INC_DOS_FILE_NAME_H
#define ZIP7_INC_DOS_FILE_NAME_H

#include "../Common/MyString.h"

#define IS_LETTER_CHAR(c) ((((unsigned)(int)(c) | 0x20) - (unsigned)'a' <= (unsigned)('z' - 'a')))

namespace NDOS {
namespace NFile {
namespace NName {

int FindSepar(const wchar_t *s) throw();
int FindSepar(const FChar *s) throw();

void NormalizeDirPathPrefix(FString &dirPath);
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

#define IF_USE_MAIN_PATH
#define IF_USE_MAIN_PATH_2(x1, x2)

bool GetFullPath(CFSTR dirPrefix, CFSTR path, FString &fullPath);
bool GetFullPath(CFSTR path, FString &fullPath);

}}}

#endif // ZIP7_INC_DOS_FILE_NAME_H
