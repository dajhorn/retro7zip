// 7-Zip Defs.h for DOS

#ifndef ZIP7_INC_DOS_DEFS_H
#define ZIP7_INC_DOS_DEFS_H

#include "../Common/MyWindows.h"

inline bool BOOLToBool(BOOL v) { return (v != FALSE); }

inline VARIANT_BOOL BoolToVARIANT_BOOL(bool v) { return (v ? VARIANT_TRUE: VARIANT_FALSE); }
inline bool VARIANT_BOOLToBool(VARIANT_BOOL v) { return (v != VARIANT_FALSE); }

#endif // ZIP7_INC_DOS_DEFS_H