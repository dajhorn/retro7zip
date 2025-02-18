// 7-Zip FileName.h for DOS.

#ifndef ZIP7_INC_DOS_FILE_NAME_H
#define ZIP7_INC_DOS_FILE_NAME_H

#include "../Common/MyString.h"

namespace NDOS {
namespace NFile {
namespace NName {

int FindSepar(const wchar_t *s) throw();
int FindSepar(const FChar *s) throw();

void NormalizeDirPathPrefix(FString &dirPath);
void NormalizeDirPathPrefix(UString &dirPath);
void NormalizeDirSeparators(FString &s);

bool IsDrivePath(const wchar_t *s) throw();
bool IsDrivePath(CFSTR s) throw();

bool IsDrivePath2(const wchar_t *s) throw();
bool IsDrivePath2(CFSTR s) throw();

bool IsAbsolutePath(const wchar_t *s) throw();

unsigned GetRootPrefixSize(const wchar_t *s) throw();

#define IF_USE_MAIN_PATH
#define IF_USE_MAIN_PATH_2(x1, x2)

bool GetFullPath(CFSTR dirPrefix, CFSTR path, FString &fullPath);
bool GetFullPath(CFSTR path, FString &fullPath);

}}}

#endif // ZIP7_INC_DOS_FILE_NAME_H
