// FilePathAutoRename.cpp

#include "StdAfx.h"
#include "FilePathAutoRename.h"

#if defined(__DOS__)
#include "../../DOS/FileFind.h"
using namespace NDOS;
#else
#include "../../Windows/FileFind.h"
using namespace NWindows;
#endif // defined(__DOS__)

static bool MakeAutoName(const FString &name,
    const FString &extension, UInt32 value, FString &path)
{
  path = name;
  path.Add_UInt32(value);
  path += extension;
  return NFile::NFind::DoesFileOrDirExist(path);
}

bool AutoRenamePath(FString &path)
{
  const int dotPos = path.ReverseFind_Dot();
  const int slashPos = path.ReverseFind_PathSepar();

  FString name = path;
  FString extension;
  if (dotPos > slashPos + 1)
  {
    name.DeleteFrom((unsigned)dotPos);
    extension = path.Ptr((unsigned)dotPos);
  }
  name.Add_Char('_');
  
  FString temp;

  UInt32 left = 1, right = (UInt32)1 << 30;
  while (left != right)
  {
    const UInt32 mid = (left + right) / 2;
    if (MakeAutoName(name, extension, mid, temp))
      left = mid + 1;
    else
      right = mid;
  }
  return !MakeAutoName(name, extension, right, path);
}
