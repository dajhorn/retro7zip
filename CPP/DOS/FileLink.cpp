// 7-Zip FileLink.cpp for DOS

#include "StdAfx.h"

#include "../../C/CpuArch.h"

#include <unistd.h>

#ifdef Z7_DEVICE_FILE
#include "../../C/Alloc.h"
#endif

#include "../Common/UTFConvert.h"
#include "../Common/StringConvert.h"

#include "FileDir.h"
#include "FileFind.h"
#include "FileIO.h"
#include "FileName.h"

namespace NDOS {
namespace NFile {

using namespace NName;

/*
  Reparse Points (Junctions and Symbolic Links):
  struct
  {
    UInt32 Tag;
    UInt16 Size;     // not including starting 8 bytes
    UInt16 Reserved; // = 0
    
    UInt16 SubstituteOffset; // offset in bytes from  start of namesChars
    UInt16 SubstituteLen;    // size in bytes, it doesn't include tailed NUL
    UInt16 PrintOffset;      // offset in bytes from  start of namesChars
    UInt16 PrintLen;         // size in bytes, it doesn't include tailed NUL
    
    [UInt32] Flags;  // for Symbolic Links only.
    
    UInt16 namesChars[]
  }

  MOUNT_POINT (Junction point):
    1) there is NUL wchar after path
    2) Default Order in table:
         Substitute Path
         Print Path
    3) pathnames can not contain dot directory names

  SYMLINK:
    1) there is no NUL wchar after path
    2) Default Order in table:
         Print Path
         Substitute Path
*/

/*
Win10 WSL2:
admin rights + sudo: it creates normal windows symbolic link.
in another cases   : it creates IO_REPARSE_TAG_LX_SYMLINK repare point.
*/

/*
static const UInt32 kReparseFlags_Alias       = (1 << 29);
static const UInt32 kReparseFlags_HighLatency = (1 << 30);
static const UInt32 kReparseFlags_Microsoft   = ((UInt32)1 << 31);

#define Z7_WIN_IO_REPARSE_TAG_HSM          (0xC0000004L)
#define Z7_WIN_IO_REPARSE_TAG_HSM2         (0x80000006L)
#define Z7_WIN_IO_REPARSE_TAG_SIS          (0x80000007L)
#define Z7_WIN_IO_REPARSE_TAG_WIM          (0x80000008L)
#define Z7_WIN_IO_REPARSE_TAG_CSV          (0x80000009L)
#define Z7_WIN_IO_REPARSE_TAG_DFS          (0x8000000AL)
#define Z7_WIN_IO_REPARSE_TAG_DFSR         (0x80000012L)
*/

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)

static const wchar_t * const k_LinkPrefix = L"\\??\\";
static const unsigned k_LinkPrefix_Size = 4;

static bool IsLinkPrefix(const wchar_t *s)
{
  return IsString1PrefixedByString2(s, k_LinkPrefix);
}

/*
static const wchar_t * const k_VolumePrefix = L"Volume{";
static const bool IsVolumeName(const wchar_t *s)
{
  return IsString1PrefixedByString2(s, k_VolumePrefix);
}
*/

#if defined(_WIN32) && !defined(UNDER_CE) && !defined(__WATCOMC__)

#define Set16(p, v) SetUi16(p, v)
#define Set32(p, v) SetUi32(p, v)

static void WriteString(Byte *dest, const wchar_t *path)
{
  for (;;)
  {
    wchar_t c = *path++;
    if (c == 0)
      return;
    Set16(dest, (UInt16)c)
    dest += 2;
  }
}

bool FillLinkData(CByteBuffer &dest, const wchar_t *path, bool isSymLink, bool isWSL)
{
  bool isAbs = IsAbsolutePath(path);
  if (!isAbs && !isSymLink)
    return false;

  if (isWSL)
  {
    // unsupported characters probably use Replacement Character UTF-16 0xFFFD
    AString utf;
    ConvertUnicodeToUTF8(path, utf);
    const size_t size = 4 + utf.Len();
    if (size != (UInt16)size)
      return false;
    dest.Alloc(8 + size);
    Byte *p = dest;
    Set32(p, Z7_WIN_IO_REPARSE_TAG_LX_SYMLINK)
    Set16(p + 4, (UInt16)(size))
    Set16(p + 6, 0)
    Set32(p + 8, Z7_WIN_LX_SYMLINK_FLAG)
    memcpy(p + 12, utf.Ptr(), utf.Len());
    return true;
  }

  // usual symbolic LINK (NOT WSL)

  bool needPrintName = true;

  if (IsSuperPath(path))
  {
    path += kSuperPathPrefixSize;
    if (!IsDrivePath(path))
      needPrintName = false;
  }

  const unsigned add_Prefix_Len = isAbs ? k_LinkPrefix_Size : 0;
    
  size_t len2 = (size_t)MyStringLen(path) * 2;
  const size_t len1 = len2 + add_Prefix_Len * 2;
  if (!needPrintName)
    len2 = 0;

  size_t totalNamesSize = (len1 + len2);

  /* some WIM imagex software uses old scheme for symbolic links.
     so we can old scheme for byte to byte compatibility */

  bool newOrderScheme = isSymLink;
  // newOrderScheme = false;

  if (!newOrderScheme)
    totalNamesSize += 2 * 2;

  const size_t size = 8 + 8 + (isSymLink ? 4 : 0) + totalNamesSize;
  if (size != (UInt16)size)
    return false;
  dest.Alloc(size);
  memset(dest, 0, size);
  const UInt32 tag = isSymLink ?
      Z7_WIN_IO_REPARSE_TAG_SYMLINK :
      Z7_WIN_IO_REPARSE_TAG_MOUNT_POINT;
  Byte *p = dest;
  Set32(p, tag)
  Set16(p + 4, (UInt16)(size - 8))
  Set16(p + 6, 0)
  p += 8;

  unsigned subOffs = 0;
  unsigned printOffs = 0;
  if (newOrderScheme)
    subOffs = (unsigned)len2;
  else
    printOffs = (unsigned)len1 + 2;

  Set16(p + 0, (UInt16)subOffs)
  Set16(p + 2, (UInt16)len1)
  Set16(p + 4, (UInt16)printOffs)
  Set16(p + 6, (UInt16)len2)

  p += 8;
  if (isSymLink)
  {
    UInt32 flags = isAbs ? 0 : Z7_WIN_SYMLINK_FLAG_RELATIVE;
    Set32(p, flags)
    p += 4;
  }

  if (add_Prefix_Len != 0)
    WriteString(p + subOffs, k_LinkPrefix);
  WriteString(p + subOffs + add_Prefix_Len * 2, path);
  if (needPrintName)
    WriteString(p + printOffs, path);
  return true;
}

#endif // defined(_WIN32) && !defined(UNDER_CE) && !defined(__WATCOMC__)


static void GetString(const Byte *p, unsigned len, UString &res)
{
  wchar_t *s = res.GetBuf(len);
  unsigned i;
  for (i = 0; i < len; i++)
  {
    wchar_t c = Get16(p + i * 2);
    if (c == 0)
      break;
    s[i] = c;
  }
  s[i] = 0;
  res.ReleaseBuf_SetLen(i);
}

bool CReparseAttr::Parse(const Byte *p, size_t size)
{
  ErrorCode = (DWORD)ERROR_INVALID_REPARSE_DATA;
  HeaderError = true;
  TagIsUnknown = true;
  MinorError = false;

  if (size < 8)
    return false;
  Tag = Get32(p);
  if (Get16(p + 6) != 0) // padding
    return false;
  unsigned len = Get16(p + 4);
  p += 8;
  size -= 8;
  if (len != size)
  // if (len > size)
    return false;
  /*
  if ((type & kReparseFlags_Alias) == 0 ||
      (type & kReparseFlags_Microsoft) == 0 ||
      (type & 0xFFFF) != 3)
  */


  HeaderError = false;

  if (   Tag != Z7_WIN_IO_REPARSE_TAG_MOUNT_POINT
      && Tag != Z7_WIN_IO_REPARSE_TAG_SYMLINK
      && Tag != Z7_WIN_IO_REPARSE_TAG_LX_SYMLINK)
  {
    // for unsupported reparse points
    ErrorCode = (DWORD)ERROR_REPARSE_TAG_INVALID; // ERROR_REPARSE_TAG_MISMATCH
    // errorCode = ERROR_REPARSE_TAG_MISMATCH; // ERROR_REPARSE_TAG_INVALID
    return false;
  }

  TagIsUnknown = false;
 
  if (Tag == Z7_WIN_IO_REPARSE_TAG_LX_SYMLINK)
  {
    if (len < 4)
      return false;
    Flags = Get32(p); // maybe it's not Flags
    if (Flags != Z7_WIN_LX_SYMLINK_FLAG)
      return false;
    len -= 4;
    p += 4;
    char *s = WslName.GetBuf(len);
    unsigned i;
    for (i = 0; i < len; i++)
    {
      char c = (char)p[i];
      s[i] = c;
      if (c == 0)
        break;
    }
    WslName.ReleaseBuf_SetEnd(i);
    MinorError = (i != len);
    ErrorCode = 0;
    return true;
  }
  
  if (len < 8)
    return false;
  unsigned subOffs = Get16(p);
  unsigned subLen = Get16(p + 2);
  unsigned printOffs = Get16(p + 4);
  unsigned printLen = Get16(p + 6);
  len -= 8;
  p += 8;

  Flags = 0;
  if (Tag == Z7_WIN_IO_REPARSE_TAG_SYMLINK)
  {
    if (len < 4)
      return false;
    Flags = Get32(p);
    len -= 4;
    p += 4;
  }

  if ((subOffs & 1) != 0 || subOffs > len || len - subOffs < subLen)
    return false;
  if ((printOffs & 1) != 0 || printOffs > len || len - printOffs < printLen)
    return false;
  GetString(p + subOffs, subLen >> 1, SubsName);
  GetString(p + printOffs, printLen >> 1, PrintName);

  ErrorCode = 0;
  return true;
}


bool CReparseShortInfo::Parse(const Byte *p, size_t size)
{
  const Byte *start = p;
  Offset= 0;
  Size = 0;
  if (size < 8)
    return false;
  UInt32 Tag = Get32(p);
  UInt32 len = Get16(p + 4);
  if (len + 8 > size)
    return false;
  /*
  if ((type & kReparseFlags_Alias) == 0 ||
      (type & kReparseFlags_Microsoft) == 0 ||
      (type & 0xFFFF) != 3)
  */
  if (Tag != Z7_WIN_IO_REPARSE_TAG_MOUNT_POINT &&
      Tag != Z7_WIN_IO_REPARSE_TAG_SYMLINK)
    // return true;
    return false;

  if (Get16(p + 6) != 0) // padding
    return false;
  
  p += 8;
  size -= 8;
  
  if (len != size) // do we need that check?
    return false;
  
  if (len < 8)
    return false;
  unsigned subOffs = Get16(p);
  unsigned subLen = Get16(p + 2);
  unsigned printOffs = Get16(p + 4);
  unsigned printLen = Get16(p + 6);
  len -= 8;
  p += 8;

  // UInt32 Flags = 0;
  if (Tag == Z7_WIN_IO_REPARSE_TAG_SYMLINK)
  {
    if (len < 4)
      return false;
    // Flags = Get32(p);
    len -= 4;
    p += 4;
  }

  if ((subOffs & 1) != 0 || subOffs > len || len - subOffs < subLen)
    return false;
  if ((printOffs & 1) != 0 || printOffs > len || len - printOffs < printLen)
    return false;

  Offset = (unsigned)(p - start) + subOffs;
  Size = subLen;
  return true;
}

bool CReparseAttr::IsOkNamePair() const
{
  if (IsLinkPrefix(SubsName))
  {
    if (!IsDrivePath(SubsName.Ptr(k_LinkPrefix_Size)))
      return PrintName.IsEmpty();
    if (wcscmp(SubsName.Ptr(k_LinkPrefix_Size), PrintName) == 0)
      return true;
  }
  return wcscmp(SubsName, PrintName) == 0;
}

/*
bool CReparseAttr::IsVolume() const
{
  if (!IsLinkPrefix(SubsName))
    return false;
  return IsVolumeName(SubsName.Ptr(k_LinkPrefix_Size));
}
*/

UString CReparseAttr::GetPath() const
{
  if (IsSymLink_WSL())
  {
    UString u;
    // if (CheckUTF8(attr.WslName)
    if (!ConvertUTF8ToUnicode(WslName, u))
      MultiByteToUnicodeString2(u, WslName);
    return u;
  }

  UString s (SubsName);
  if (IsLinkPrefix(s))
  {
    s.ReplaceOneCharAtPos(1, '\\'); // we normalize prefix from "\??\" to "\\?\"
    if (IsDrivePath(s.Ptr(k_LinkPrefix_Size)))
      s.DeleteFrontal(k_LinkPrefix_Size);
  }
  return s;
}

#ifdef Z7_DEVICE_FILE

namespace NSystem
{
bool MyGetDiskFreeSpace(CFSTR rootPath, UInt64 &clusterSize, UInt64 &totalSize, UInt64 &freeSize);
}
#endif // Z7_DEVICE_FILE


}}
