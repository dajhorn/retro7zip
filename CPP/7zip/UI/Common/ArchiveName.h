// ArchiveName.h

#ifndef ZIP7_INC_ARCHIVE_NAME_H
#define ZIP7_INC_ARCHIVE_NAME_H

#if defined(__DOS__)
  #include "../../../DOS/FileFind.h"
  using namespace NDOS;
#else
  #include "../../../Windows/FileFind.h"
  using namespace NWindows;
#endif

/* (fi != NULL) only if (paths.Size() == 1) */

UString CreateArchiveName(
    const UStringVector &paths,
    bool isHash,
    const NFile::NFind::CFileInfo *fi,
    UString &baseName);

#endif
