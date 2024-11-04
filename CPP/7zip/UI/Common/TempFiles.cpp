// TempFiles.cpp

#include "StdAfx.h"

#if defined(__DOS__)
#include "../../../DOS/FileDir.h"
using namespace NDOS;
#else
#include "../../../Windows/FileDir.h"
using namespace NWindows;
#endif // defined(__DOS__)

#include "TempFiles.h"

using namespace NFile;

void CTempFiles::Clear()
{
  while (!Paths.IsEmpty())
  {
    if (NeedDeleteFiles)
      NDir::DeleteFileAlways(Paths.Back());
    Paths.DeleteBack();
  }
}
