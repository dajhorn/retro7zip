// Common.h

#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning(disable : 4464) // relative include path contains '..'
#endif

#ifndef ZIP7_INC_COMMON_H
#define ZIP7_INC_COMMON_H

#if defined(__DOS__)
#include "../DOS/Defs.h"
#include "../DOS/ErrorMsg.h"
#include "../DOS/FileDir.h"
#include "../DOS/FileFind.h"
#include "../DOS/FileIO.h"
#include "../DOS/FileName.h"
#include "../DOS/FileSystem.h"
#include "../DOS/PropVariant.h"
#include "../DOS/PropVariantConv.h"
#include "../DOS/PropVariantUtils.h"
#include "../DOS/System.h"
#include "../DOS/TimeUtils.h"
//using namespace NDOS;
#else
//using namespace NWindows;
#endif

#include "../../C/Precomp.h"
#include "Common0.h"
#include "MyWindows.h"

/*
This file is included to all cpp files in 7-Zip.
Each folder contains StdAfx.h file that includes "Common.h".
So 7-Zip includes "Common.h" in both modes:
  with precompiled StdAfx.h
and
  without precompiled StdAfx.h

include "Common.h" before other h files of 7-zip,
   if you need predefined macros.
do not include "Common.h", if you need only interfaces,
   and you don't need predefined macros.
*/

#endif
