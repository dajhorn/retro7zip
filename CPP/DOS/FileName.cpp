// 7-Zip FileName.cpp for DOS.

#include "StdAfx.h"

#define IS_SEPAR(c) IS_PATH_SEPAR(c)


namespace NDOS {
namespace NFile {
namespace NName {


int FindSepar(const FChar *s) throw()
{
  for (const FChar *p = s;; p++)
  {
    const FChar c = *p;
    if (c == 0)
      return -1;
    if (IS_SEPAR(c))
      return (int)(p - s);
  }
}


void NormalizeDirPathPrefix(FString &dirPath)
{
  if (dirPath.IsEmpty())
    return;
  if (!IsPathSepar(dirPath.Back()))
    dirPath.Add_PathSepar();
}


void NormalizeDirPathPrefix(UString &dirPath)
{
  if (dirPath.IsEmpty())
    return;
  if (!IsPathSepar(dirPath.Back()))
    dirPath.Add_PathSepar();
}


bool IsDrivePath(const wchar_t *s) throw() {
  return IS_LETTER_CHAR(s[0]) && s[1] == ':' && IS_SEPAR(s[2]);
}


bool IsAltPathPrefix(CFSTR s) throw()
{
  unsigned len = MyStringLen(s);

  if (len == 0)
    return false;

  if (s[len - 1] != ':')
    return false;

  return true;
}

bool IsAbsolutePath(const wchar_t *s) throw() { return IS_SEPAR(s[0]); }

unsigned GetRootPrefixSize(CFSTR s) throw();
unsigned GetRootPrefixSize(CFSTR s) throw() { return IS_SEPAR(s[0]) ? 1 : 0; }
unsigned GetRootPrefixSize(const wchar_t *s) throw() { return IS_SEPAR(s[0]) ? 1 : 0; }


static bool GetCurDir(UString &path)
{
  path.Empty();
  FString s;
  if (!NDir::GetCurrentDir(s))
    return false;
  path = fs2us(s);
  return true;
}


static bool ResolveDotsFolders(UString &s)
{
  for (unsigned i = 0;;)
  {
    const wchar_t c = s[i];
    if (c == 0)
      return true;
    if (c == '.' && (i == 0 || IS_SEPAR(s[i - 1])))
    {
      const wchar_t c1 = s[i + 1];
      if (c1 == '.')
      {
        const wchar_t c2 = s[i + 2];
        if (IS_SEPAR(c2) || c2 == 0)
        {
          if (i == 0)
            return false;
          int k = (int)i - 2;
          i += 2;
          
          for (;; k--)
          {
            if (k < 0)
              return false;
            if (!IS_SEPAR(s[(unsigned)k]))
              break;
          }

          do
            k--;
          while (k >= 0 && !IS_SEPAR(s[(unsigned)k]));
          
          unsigned num;
          
          if (k >= 0)
          {
            num = i - (unsigned)k;
            i = (unsigned)k;
          }
          else
          {
            num = (c2 == 0 ? i : (i + 1));
            i = 0;
          }
          
          s.Delete(i, num);
          continue;
        }
      }
      else if (IS_SEPAR(c1) || c1 == 0)
      {
        unsigned num = 2;
        if (i != 0)
          i--;
        else if (c1 == 0)
          num = 1;
        s.Delete(i, num);
        continue;
      }
    }

    i++;
  }
}


static bool AreThereDotsFolders(CFSTR s)
{
  for (unsigned i = 0;; i++)
  {
    FChar c = s[i];
    if (c == 0)
      return false;
    if (c == '.' && (i == 0 || IS_SEPAR(s[i - 1])))
    {
      FChar c1 = s[i + 1];
      if (c1 == 0 || IS_SEPAR(c1) ||
          (c1 == '.' && (s[i + 2] == 0 || IS_SEPAR(s[i + 2]))))
        return true;
    }
  }
}


bool GetFullPath(CFSTR dirPrefix, CFSTR s, FString &res)
{
  res = s;

  const unsigned prefixSize = GetRootPrefixSize(s);
  if (prefixSize != 0)
#ifdef _WIN32
  if (prefixSize != 1)
#endif
  {
    if (!AreThereDotsFolders(s + prefixSize))
      return true;

    UString rem = fs2us(s + prefixSize);
    if (!ResolveDotsFolders(rem))
      return true; // maybe false;
    res.DeleteFrom(prefixSize);
    res += us2fs(rem);
    return true;
  }

  UString curDir;
  if (dirPrefix && prefixSize == 0)
    curDir = fs2us(dirPrefix);  // we use (dirPrefix), only if (s) path is relative
  else
  {
    if (!GetCurDir(curDir))
      return false;
  }
  NormalizeDirPathPrefix(curDir);

  unsigned fixedSize = GetRootPrefixSize(curDir);

  UString temp;
#ifdef _WIN32
  if (prefixSize != 0)
  {
    /* (s) is absolute path, but only (prefixSize == 1) is possible here.
       So for full resolving we need root of current folder and
       relative part of (s). */
    s += prefixSize;
    // (s) is relative part now
    if (fixedSize == 0)
    {
      // (curDir) is not absolute.
      // That case is unexpected, but we support it too.
      curDir.Empty();
      curDir.Add_PathSepar();
      fixedSize = 1;
      // (curDir) now is just Separ character.
      // So final (res) path later also will have Separ prefix.
    }
  }
  else
#endif // _WIN32
  {
    // (s) is relative path
    temp = curDir.Ptr(fixedSize);
    // (temp) is relative_part_of(curDir)
  }
  temp += fs2us(s);
  if (!ResolveDotsFolders(temp))
    return false;
  curDir.DeleteFrom(fixedSize);
  // (curDir) now contains only absolute prefix part
  res = us2fs(curDir);
  res += us2fs(temp);

  return true;
}


bool GetFullPath(CFSTR path, FString &fullPath)
{
  return GetFullPath(NULL, path, fullPath);
}


}}}
