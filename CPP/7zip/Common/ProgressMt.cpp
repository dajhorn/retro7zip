// ProgressMt.h

#include "StdAfx.h"

#include "ProgressMt.h"

#if defined(__DOS__)
using namespace NDOS;
#else
using namespace NWindows;
#endif

void CMtCompressProgressMixer::Init(unsigned numItems, ICompressProgressInfo *progress)
{
  NSynchronization::CCriticalSectionLock lock(CriticalSection);
  InSizes.Clear();
  OutSizes.Clear();
  for (unsigned i = 0; i < numItems; i++)
  {
    InSizes.Add(0);
    OutSizes.Add(0);
  }
  TotalInSize = 0;
  TotalOutSize = 0;
  _progress = progress;
}

void CMtCompressProgressMixer::Reinit(unsigned index)
{
  NSynchronization::CCriticalSectionLock lock(CriticalSection);
  InSizes[index] = 0;
  OutSizes[index] = 0;
}

HRESULT CMtCompressProgressMixer::SetRatioInfo(unsigned index, const UInt64 *inSize, const UInt64 *outSize)
{
  NSynchronization::CCriticalSectionLock lock(CriticalSection);
  if (inSize)
  {
    const UInt64 diff = *inSize - InSizes[index];
    InSizes[index] = *inSize;
    TotalInSize += diff;
  }
  if (outSize)
  {
    const UInt64 diff = *outSize - OutSizes[index];
    OutSizes[index] = *outSize;
    TotalOutSize += diff;
  }
  if (_progress)
    return _progress->SetRatioInfo(&TotalInSize, &TotalOutSize);
  return S_OK;
}


Z7_COM7F_IMF(CMtCompressProgress::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize))
{
  return _progress->SetRatioInfo(_index, inSize, outSize);
}
