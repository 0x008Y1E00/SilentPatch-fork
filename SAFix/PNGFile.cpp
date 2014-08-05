#include "StdAfxSA.h"
#include "PNGFile.h"

RwTexture* CPNGFile::ReadFromFile(const char* pFileName)
{
	RwTexture*		pTexture = nullptr;

	if ( RwImage* pImage = RtPNGImageRead(pFileName) )
	{
		RwInt32		dwWidth, dwHeight, dwDepth, dwFlags;
		RwImageFindRasterFormat(pImage, rwRASTERTYPETEXTURE, &dwWidth, &dwHeight, &dwDepth, &dwFlags);
		if ( RwRaster* pRaster = RwRasterCreate(dwWidth, dwHeight, dwDepth, dwFlags) )
		{
			RwRasterSetFromImage(pRaster, pImage);

			pTexture = RwTextureCreate(pRaster);
			//if ( pTexture )
			//	RwTextureSetName(pTexture, pFileName);
		}
		RwImageDestroy(pImage);
	}
	return pTexture;
}

RwTexture* CPNGFile::ReadFromMemory(const void* pMemory, unsigned int nLen)
{
	static BYTE*	pMem = AddressByVersion<BYTE*>(0x7CF9CA, 0, 0);
	RwTexture*		pTexture = nullptr;

	MemoryVP::Patch<BYTE>(pMem, rwSTREAMMEMORY);

	RwMemory	PNGMemory;
	PNGMemory.start = const_cast<RwUInt8*>(static_cast<const RwUInt8*>(pMemory));
	PNGMemory.length = nLen;

	if ( RwImage* pImage = RtPNGImageRead(reinterpret_cast<RwChar*>(&PNGMemory)) )
	{
		RwInt32		dwWidth, dwHeight, dwDepth, dwFlags;
		RwImageFindRasterFormat(pImage, rwRASTERTYPETEXTURE, &dwWidth, &dwHeight, &dwDepth, &dwFlags);
		if ( RwRaster* pRaster = RwRasterCreate(dwWidth, dwHeight, dwDepth, dwFlags) )
		{
			RwRasterSetFromImage(pRaster, pImage);

			pTexture = RwTextureCreate(pRaster);
			//if ( pTexture )
			//	RwTextureSetName(pTexture, pFileName);
		}
		RwImageDestroy(pImage);
	}

	MemoryVP::Patch<BYTE>(pMem, rwSTREAMFILENAME);

	return pTexture;
}