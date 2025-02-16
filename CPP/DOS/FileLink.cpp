// 7-Zip FileLink.cpp for DOS.

#include "StdAfx.h"
#include "../../C/CpuArch.h"
#include "../Common/UTFConvert.h"
#include "../Common/StringConvert.h"

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)

namespace NDOS {
namespace NFile {


using namespace NName;

static const wchar_t * const k_LinkPrefix = L"\\??\\";
static const unsigned k_LinkPrefix_Size = 4;


static bool IsLinkPrefix(const wchar_t *s)
{
  return IsString1PrefixedByString2(s, k_LinkPrefix);
}


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

  if (Get16(p + 6) != 0)
    return false;

  unsigned len = Get16(p + 4);
  p += 8;
  size -= 8;

  if (len != size)
    return false;

  HeaderError = false;

  if (   Tag != Z7_WIN_IO_REPARSE_TAG_MOUNT_POINT
      && Tag != Z7_WIN_IO_REPARSE_TAG_SYMLINK
      && Tag != Z7_WIN_IO_REPARSE_TAG_LX_SYMLINK)
  {
    ErrorCode = (DWORD)ERROR_REPARSE_TAG_INVALID;
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

  if (Tag != Z7_WIN_IO_REPARSE_TAG_MOUNT_POINT && Tag != Z7_WIN_IO_REPARSE_TAG_SYMLINK)
    return false;

  if (Get16(p + 6) != 0)
    return false;

  p += 8;
  size -= 8;

  if (len != size)
    return false;

  if (len < 8)
    return false;

  unsigned subOffs = Get16(p);
  unsigned subLen = Get16(p + 2);
  unsigned printOffs = Get16(p + 4);
  unsigned printLen = Get16(p + 6);
  len -= 8;
  p += 8;

  if (Tag == Z7_WIN_IO_REPARSE_TAG_SYMLINK)
  {
    if (len < 4)
      return false;
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


UString CReparseAttr::GetPath() const
{
  if (IsSymLink_WSL())
  {
    UString u;
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


}}
