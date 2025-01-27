// Lzma2Encoder.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"

#include "../Common/CWrappers.h"
#include "../Common/StreamUtils.h"

#if defined(__DOS__)
#include "../../DOS/System.h"
using namespace NDOS;
#else
#include "../../Windows/System.h"
using namespace NWindows;
#endif

#include "Lzma2Encoder.h"

namespace NCompress {

namespace NLzma {

HRESULT SetLzmaProp(PROPID propID, const PROPVARIANT &prop, CLzmaEncProps &ep);

}

namespace NLzma2 {

CEncoder::CEncoder()
{
  _encoder = NULL;
  _encoder = Lzma2Enc_Create(&g_AlignedAlloc, &g_BigAlloc);
  if (!_encoder)
    throw 1;
}

CEncoder::~CEncoder()
{
  if (_encoder)
    Lzma2Enc_Destroy(_encoder);
}


unsigned int MaximumDictionarySize(uint32_t &dictionary_size)
{
  // Compute the largest dictionary size that can be used to create an
  // LZMA2 archive without using virtual memory.

  unsigned int dictionary_width = 0;

  // Get the amount of alloc'able memory in bytes.
  NSystem::GetRamSize(dictionary_size);

  if (dictionary_size == 0)
    return 0;

  // For extra dictionary headspace, divide the amount of free memory by
  // sixteen instead of thirteen.
  dictionary_size /= 16;

  // Compute floor(log2(size)).
  while (dictionary_size >>= 1)
    ++dictionary_width;

  // 4 KiB is the smallest LZMA dictionary size.
  if (dictionary_width < 12)
    dictionary_width = 12;

  // Round dictionary_size down to a power-of-two.
  dictionary_size = 1 << dictionary_width;

  // This is the largest -md<N> value that can be used without memory swapping.
  return dictionary_width;
}


HRESULT SetLzma2Prop(PROPID propID, const PROPVARIANT &prop, CLzma2EncProps &lzma2Props);
HRESULT SetLzma2Prop(PROPID propID, const PROPVARIANT &prop, CLzma2EncProps &lzma2Props)
{
  switch (propID)
  {
    case NCoderPropID::kBlockSize:
    {
      if (prop.vt == VT_UI4)
        lzma2Props.blockSize = prop.ulVal;
      else if (prop.vt == VT_UI8)
        lzma2Props.blockSize = prop.uhVal.QuadPart;
      else
        return E_INVALIDARG;
      break;
    }
    case NCoderPropID::kNumThreads:
      if (prop.vt != VT_UI4)
        return E_INVALIDARG;
      lzma2Props.numTotalThreads = (int)(prop.ulVal);
      break;
    default:
      RINOK(NLzma::SetLzmaProp(propID, prop, lzma2Props.lzmaProps))
  }
  return S_OK;
}


Z7_COM7F_IMF(CEncoder::SetCoderProperties(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps))
{
  CLzma2EncProps lzma2Props;
  Lzma2EncProps_Init(&lzma2Props);

  for (UInt32 i = 0; i < numProps; i++)
  {
    RINOK(SetLzma2Prop(propIDs[i], coderProps[i], lzma2Props))
  }

  #if defined(__WATCOMC__)
  /*
   * LZMA uses an amount of XMS memory that is approximately thirteen times
   * the size of the encoding dictionary, plus the size of the static
   * allocation of the 7z executable image.
   *
   * None of the upstream compression presets comfortably fit the intended
   * DOS target machine. Rounding up:
   *
   *   -mx1 requires 8 MiB
   *   -mx2 requires 16 MiB
   *   -mx3 requires 64 MiB
   *   -mx4 requires 256 MiB
   *   -mx5 requires 512 MiB (this is the upstream default)
   *
   * If a dictionary size is not passed with a -md switch, then override the
   * working preset and just use the largest dictionary that can be allocated
   * by the host.
   *
   * This means that a 64 MiB host will create 7z files with a 2 MiB
   * dictionary that can be unpacked by the 7zm mini variant running on a
   * 6 MiB host or the 7zr reduced variant running on a 8 MiB host.
   */

  if (lzma2Props.lzmaProps.dictSize == 0)
    MaximumDictionarySize(lzma2Props.lzmaProps.dictSize);
  #endif

  return SResToHRESULT(Lzma2Enc_SetProps(_encoder, &lzma2Props));
}


Z7_COM7F_IMF(CEncoder::SetCoderPropertiesOpt(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps))
{
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    const PROPID propID = propIDs[i];
    if (propID == NCoderPropID::kExpectedDataSize)
      if (prop.vt == VT_UI8)
        Lzma2Enc_SetDataSize(_encoder, prop.uhVal.QuadPart);
  }
  return S_OK;
}


Z7_COM7F_IMF(CEncoder::WriteCoderProperties(ISequentialOutStream *outStream))
{
  const Byte prop = Lzma2Enc_WriteProperties(_encoder);
  return WriteStream(outStream, &prop, 1);
}


#define RET_IF_WRAP_ERROR(wrapRes, sRes, sResErrorCode) \
  if (wrapRes != S_OK /* && (sRes == SZ_OK || sRes == sResErrorCode) */) return wrapRes;

Z7_COM7F_IMF(CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress))
{
  CSeqInStreamWrap inWrap;
  CSeqOutStreamWrap outWrap;
  CCompressProgressWrap progressWrap;

  inWrap.Init(inStream);
  outWrap.Init(outStream);
  progressWrap.Init(progress);

  SRes res = Lzma2Enc_Encode2(_encoder,
      &outWrap.vt, NULL, NULL,
      &inWrap.vt, NULL, 0,
      progress ? &progressWrap.vt : NULL);

  RET_IF_WRAP_ERROR(inWrap.Res, res, SZ_ERROR_READ)
  RET_IF_WRAP_ERROR(outWrap.Res, res, SZ_ERROR_WRITE)
  RET_IF_WRAP_ERROR(progressWrap.Res, res, SZ_ERROR_PROGRESS)

  return SResToHRESULT(res);
}
  
}}
