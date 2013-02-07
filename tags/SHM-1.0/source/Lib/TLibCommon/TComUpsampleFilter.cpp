
#include "TComUpsampleFilter.h"
#include "TypeDef.h"

#if SVC_UPSAMPLING
// ====================================================================================================================
// Tables:
// 1. PHASE_DERIVATION_IN_INTEGER = 0 is the implementation using 2 multi-phase (12 phase and 8 phase) filter sets 
//    by following K0378.
// 2. PHASE_DERIVATION_IN_INTEGER = 1 is the implemetation using a 16-phase filter set just for speed up. Some phases 
//    is approximated to x/16 (e.g. 1/3 -> 5/16). 
// 3. It was confirmed that two implementations provides the identical result. 
// 4. By default, PHASE_DERIVATION_IN_INTEGER is set to 1. 
// ====================================================================================================================
#define CNU -1 ///< Coefficients Not Used

#if PHASE_DERIVATION_IN_INTEGER
const Int TComUpsampleFilter::m_lumaFixedFilter[16][NTAPS_US_LUMA] =
{
  {  0,  0,  0, 64,  0,  0,  0,  0}, //
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU}, //
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU}, //
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU}, // 
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU}, //
  { -1, 4, -11, 52, 26,  -8, 3, -1}, // <-> actual phase shift 1/3, used for spatial scalability x1.5      
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU}, //       
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU}, // 
  { -1, 4, -11, 40, 40, -11, 4, -1}, // <-> actual phase shift 1/2, equal to HEVC MC, used for spatial scalability x2
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU}, // 
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU}, // 
  { -1, 3,  -8, 26, 52, -11, 4, -1}, // <-> actual phase shift 2/3, used for spatial scalability x1.5
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU}, // 
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU}, // 
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU}, // 
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU}  // 
};

const Int TComUpsampleFilter::m_chromaFixedFilter[16][NTAPS_US_CHROMA] =
{
#if CHROMA_UPSAMPLING
  {  0, 64,  0,  0},//
  {CNU,CNU,CNU,CNU},//
  {CNU,CNU,CNU,CNU},//
  {CNU,CNU,CNU,CNU},// 
  { -4, 54, 16, -2},// <-> actual phase shift 1/4,equal to HEVC MC, used for spatial scalability x1.5 (only for accurate Chroma alignement)
  { -6, 52, 20, -2},// <-> actual phase shift 1/3, used for spatial scalability x1.5   
  { -6, 46, 28, -4},// <-> actual phase shift 3/8,equal to HEVC MC, used for spatial scalability x2 (only for accurate Chroma alignement)      
  {CNU,CNU,CNU,CNU},// 
  { -4, 36, 36, -4},// <-> actual phase shift 1/2,equal to HEVC MC, used for spatial scalability x2
  { -4, 30, 42, -4},// <-> actual phase shift 7/12, used for spatial scalability x1.5 (only for accurate Chroma alignement)
  {CNU,CNU,CNU,CNU},// 
  { -2, 20, 52, -6},// <-> actual phase shift 2/3, used for spatial scalability x1.5
  {CNU,CNU,CNU,CNU},// 
  {CNU,CNU,CNU,CNU},// 
  { -2, 10, 58, -2},// <-> actual phase shift 7/8,equal to HEVC MC, used for spatial scalability x2 (only for accurate Chroma alignement)  
  {  0,  4, 62, -2} // <-> actual phase shift 11/12, used for spatial scalability x1.5 (only for accurate Chroma alignement)
#else
  {  0, 64,  0,  0},//
  {CNU,CNU,CNU,CNU},//
  {CNU,CNU,CNU,CNU},//
  {CNU,CNU,CNU,CNU},// 
  { -4, 54, 16, -2},// <-> actual phase shift 1/4,equal to HEVC MC, used for spatial scalability x1.5 (only for accurate Chroma alignement)
  { -5, 50, 22, -3},// <-> actual phase shift 1/3, used for spatial scalability x1.5   
  { -6, 46, 28, -4},// <-> actual phase shift 3/8,equal to HEVC MC, used for spatial scalability x2 (only for accurate Chroma alignement)      
  {CNU,CNU,CNU,CNU},// 
  { -4, 36, 36, -4},// <-> actual phase shift 1/2,equal to HEVC MC, used for spatial scalability x2
  { -4, 30, 43, -5},// <-> actual phase shift 7/12, used for spatial scalability x1.5 (only for accurate Chroma alignement)
  {CNU,CNU,CNU,CNU},// 
  { -3, 22, 50, -5},// <-> actual phase shift 2/3, used for spatial scalability x1.5
  {CNU,CNU,CNU,CNU},// 
  {CNU,CNU,CNU,CNU},// 
  { -2, 10, 58, -2},// <-> actual phase shift 7/8,equal to HEVC MC, used for spatial scalability x2 (only for accurate Chroma alignement)  
  { -1,  5, 62, -2} // <-> actual phase shift 11/12, used for spatial scalability x1.5 (only for accurate Chroma alignement)
#endif
};
#else
const Int TComUpsampleFilter::m_lumaFixedFilter[12][NTAPS_US_LUMA] =
{
  { 0,  0,   0, 64,  0,   0, 0,  0},
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU},//1/12 
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU},//2/12 
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU},//3/12 
  { -1, 4, -11, 52, 26,  -8, 3, -1},//4/12 
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU},//5/12 
  { -1, 4, -11, 40, 40, -11, 4, -1},//6/12       
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU},//7/12 
  { -1, 3,  -8, 26, 52, -11, 4, -1},//8/12 
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU},//9/12 
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU},//10/12
  {CNU,CNU,CNU,CNU,CNU,CNU,CNU,CNU},//11/12
};

const Int TComUpsampleFilter::m_chromaFixedFilter15[12][NTAPS_US_CHROMA] =
{
  {  0, 64,  0,  0},
  {CNU,CNU,CNU,CNU},//1/12 
  {CNU,CNU,CNU,CNU},//2/12 
  { -4, 54, 16, -2},//3/12 
  { -5, 50, 22, -3},//4/12 
  {CNU,CNU,CNU,CNU},//5/12 
  {CNU,CNU,CNU,CNU},//6/12 
  { -4, 30, 43, -5},//7/12 
  { -3, 22, 50, -5},//8/12 
  {CNU,CNU,CNU,CNU},//9/12 
  {CNU,CNU,CNU,CNU},//10/12
  { -1,  5, 62, -2} //11/12
};

const Int TComUpsampleFilter::m_chromaFixedFilter20[8][NTAPS_US_CHROMA] =
{
  {  0, 64,  0,  0},
  {CNU,CNU,CNU,CNU},//1/8 
  {CNU,CNU,CNU,CNU},//2/8 
  { -6, 46, 28, -4},//3/8 
  { -4, 36, 36, -4},//4/8 
  {CNU,CNU,CNU,CNU},//5/8 
  {CNU,CNU,CNU,CNU},//6/8 
  { -2, 10, 58, -2},//7/8 
};
#endif

TComUpsampleFilter::TComUpsampleFilter(void)
{
}

TComUpsampleFilter::~TComUpsampleFilter(void)
{
}

Void TComUpsampleFilter::upsampleBasePic( TComPicYuv* pcUsPic, TComPicYuv* pcBasePic, TComPicYuv* pcTempPic )
{
  assert ( NTAPS_US_LUMA == 8 );
  assert ( NTAPS_US_CHROMA == 4 );

  Int i, j;

  //========== Y component upsampling ===========
  Int iBWidth   = pcBasePic->getWidth () - pcBasePic->getPicCropLeftOffset() - pcBasePic->getPicCropRightOffset();
  Int iBHeight  = pcBasePic->getHeight() - pcBasePic->getPicCropTopOffset() - pcBasePic->getPicCropBottomOffset();
  Int iBStride  = pcBasePic->getStride();

  Int iEWidth   = pcUsPic->getWidth () - pcUsPic->getPicCropLeftOffset() - pcUsPic->getPicCropRightOffset();
  Int iEHeight  = pcUsPic->getHeight() - pcUsPic->getPicCropTopOffset() - pcUsPic->getPicCropBottomOffset();
  Int iEStride  = pcUsPic->getStride();
  
  Pel* piTempBufY = pcTempPic->getLumaAddr();
  Pel* piSrcBufY  = pcBasePic->getLumaAddr();
  Pel* piDstBufY  = pcUsPic->getLumaAddr();
  
  Pel* piSrcY;
  Pel* piDstY;
  
  Pel* piTempBufU = pcTempPic->getCbAddr();
  Pel* piSrcBufU  = pcBasePic->getCbAddr();
  Pel* piDstBufU  = pcUsPic->getCbAddr();
  
  Pel* piTempBufV = pcTempPic->getCrAddr();
  Pel* piSrcBufV  = pcBasePic->getCrAddr();
  Pel* piDstBufV  = pcUsPic->getCrAddr();
  
  Pel* piSrcU;
  Pel* piDstU;
  Pel* piSrcV;
  Pel* piDstV;
  
#if JCTVC_L0178
  Pel *tempBufRight = NULL, *tempBufBottom = NULL;
  Int tempBufSizeRight = 0, tempBufSizeBottom = 0;
  
  if ( pcBasePic->getPicCropRightOffset() )
  {
    tempBufSizeRight = pcBasePic->getPicCropRightOffset() * pcBasePic->getHeight();
  }
  
  if( pcBasePic->getPicCropBottomOffset() )
  {
    tempBufSizeBottom = pcBasePic->getPicCropBottomOffset() * pcBasePic->getWidth ();
  }
  
  if( tempBufSizeRight )
  {
    tempBufRight = (Pel *) xMalloc(Pel, tempBufSizeRight + (tempBufSizeRight>>1) );
    assert( tempBufRight );
  }
  
  if( tempBufSizeBottom )
  {
    tempBufBottom = (Pel *) xMalloc(Pel, tempBufSizeBottom + (tempBufSizeBottom>>1) );
    assert( tempBufBottom );
  }
 
#endif
  
#if PHASE_DERIVATION_IN_INTEGER
  Int iRefPos16 = 0;
  Int phase    = 0;
  Int refPos   = 0; 
  Int* coeff = m_chromaFilter[phase];
  for ( i = 0; i < 16; i++)
  {
    memcpy(   m_lumaFilter[i],   m_lumaFixedFilter[i], sizeof(Int) * NTAPS_US_LUMA   );
    memcpy( m_chromaFilter[i], m_chromaFixedFilter[i], sizeof(Int) * NTAPS_US_CHROMA );
  }
#else
  for ( i = 0; i < 12; i++)
  {
    memcpy( m_lumaFilter[i], m_lumaFixedFilter[i], sizeof(Int) * NTAPS_US_LUMA );
  }

  Int chromaPhaseDenominator;
  if (iEWidth == 2*iBWidth) // 2x scalability
  {
    for ( i = 0; i < 8; i++)
    {
      memcpy( m_chromaFilter[i], m_chromaFixedFilter20[i], sizeof(Int) * NTAPS_US_CHROMA );
    }
    chromaPhaseDenominator = 8;
  }
  else
  {
    for ( i = 0; i < 12; i++) // 1.5x scalability
    {
      memcpy( m_chromaFilter[i], m_chromaFixedFilter15[i], sizeof(Int) * NTAPS_US_CHROMA );
    }
    chromaPhaseDenominator = 12;
  }
#endif



  assert ( iEWidth == 2*iBWidth || 2*iEWidth == 3*iBWidth );
  assert ( iEHeight == 2*iBHeight || 2*iEHeight == 3*iBHeight );

#if JCTVC_L0178
  // save the cropped region to copy back to the base picture since the base picture might be used as a reference picture
  if( tempBufSizeRight )
  {
    piSrcY = piSrcBufY + iBWidth;
    piDstY = tempBufRight;
    for( i = 0; i < pcBasePic->getHeight(); i++ )
    {
      memcpy(piDstY, piSrcY, sizeof(Pel) * pcBasePic->getPicCropRightOffset());
      piSrcY += iBStride;
      piDstY += pcBasePic->getPicCropRightOffset();
    }
    
    if(pcBasePic->getPicCropRightOffset()>>1)
    {
      Int iBStrideChroma = (iBStride>>1);
      piSrcU = piSrcBufU + (iBWidth>>1);
      piDstU = tempBufRight + pcBasePic->getPicCropRightOffset() * pcBasePic->getHeight();
      piSrcV = piSrcBufV + (iBWidth>>1);
      piDstV = piDstU + (pcBasePic->getPicCropRightOffset()>>1) * (pcBasePic->getHeight()>>1);
    
      for( i = 0; i < pcBasePic->getHeight()>>1; i++ )
      {
        memcpy(piDstU, piSrcU, sizeof(Pel) * (pcBasePic->getPicCropRightOffset()>>1));
        piSrcU += iBStrideChroma;
        piDstU += (pcBasePic->getPicCropRightOffset()>>1);
        
        memcpy(piDstV, piSrcV, sizeof(Pel) * (pcBasePic->getPicCropRightOffset()>>1));
        piSrcV += iBStrideChroma;
        piDstV += (pcBasePic->getPicCropRightOffset()>>1);
      }
    }
    
    pcBasePic->setWidth(iBWidth);
  }
  
  if( tempBufSizeBottom )
  {
    piSrcY = piSrcBufY + iBHeight * iBStride;
    piDstY = tempBufBottom;
    for( i = 0; i < pcBasePic->getPicCropBottomOffset(); i++ )
    {
      memcpy(piDstY, piSrcY, sizeof(Pel) * pcBasePic->getWidth());
      piSrcY += iBStride;
      piDstY += pcBasePic->getWidth();
    }
    
    if(pcBasePic->getPicCropBottomOffset()>>1)
    {
      Int iBStrideChroma = (iBStride>>1);
      piSrcU = piSrcBufU + (iBHeight>>1) * iBStrideChroma;
      piDstU = tempBufBottom + pcBasePic->getPicCropBottomOffset() * pcBasePic->getWidth();
      piSrcV = piSrcBufV + (iBHeight>>1) * iBStrideChroma;
      piDstV = piDstU + (pcBasePic->getPicCropBottomOffset()>>1) * (pcBasePic->getWidth()>>1);
      
      for( i = 0; i < pcBasePic->getPicCropBottomOffset()>>1; i++ )
      {
        memcpy(piDstU, piSrcU, sizeof(Pel) * (pcBasePic->getWidth()>>1));
        piSrcU += iBStrideChroma;
        piDstU += (pcBasePic->getWidth()>>1);
        
        memcpy(piDstV, piSrcV, sizeof(Pel) * (pcBasePic->getWidth()>>1));
        piSrcV += iBStrideChroma;
        piDstV += (pcBasePic->getWidth()>>1);
      }
    }

    pcBasePic->setHeight(iBHeight);
  }
#endif
  
  pcBasePic->setBorderExtension(false);
  pcBasePic->extendPicBorder   (); // extend the border.

#if PHASE_DERIVATION_IN_INTEGER
  Int   iShiftX = 16;
  Int   iShiftY = 16;

  Int   iPhaseX = 0;
  Int   iPhaseY = 0;

  Int   iAddX       = ( ( ( iBWidth * iPhaseX ) << ( iShiftX - 2 ) ) + ( iEWidth >> 1 ) ) / iEWidth + ( 1 << ( iShiftX - 5 ) );
  Int   iAddY       = ( ( ( iBHeight * iPhaseY ) << ( iShiftY - 2 ) ) + ( iEHeight >> 1 ) ) / iEHeight+ ( 1 << ( iShiftY - 5 ) );

  Int   iDeltaX     = 4 * iPhaseX;
  Int   iDeltaY     = 4 * iPhaseY;  


  Int iShiftXM4 = iShiftX - 4;
  Int iShiftYM4 = iShiftY - 4;

  Int   iScaleX     = ( ( iBWidth << iShiftX ) + ( iEWidth >> 1 ) ) / iEWidth;
  Int   iScaleY     = ( ( iBHeight << iShiftY ) + ( iEHeight >> 1 ) ) / iEHeight;
#else
  const Double sFactor = 1.0 * iBWidth / iEWidth;
  const Double sFactor12 = sFactor * 12;
#endif

  //========== horizontal upsampling ===========
  for( i = 0; i < iEWidth; i++ )
  {
#if PHASE_DERIVATION_IN_INTEGER
    iRefPos16 = ((i*iScaleX + iAddX) >> iShiftXM4) - iDeltaX;
    phase    = iRefPos16 & 15;
    refPos   = iRefPos16 >> 4;
    coeff = m_lumaFilter[phase];
#else
    Int refPos12 = (Int) ( i * sFactor12 );
    Int refPos = (Int)( i * sFactor );
    Int phase = (refPos12 + 12) % 12; 
    Int* coeff = m_lumaFilter[phase];
#endif

    piSrcY = piSrcBufY + refPos -((NTAPS_US_LUMA>>1) - 1);
    piDstY = piTempBufY + i;

    for( j = 0; j < iBHeight ; j++ )
    {
      *piDstY = sumLumaHor(piSrcY, coeff);
      piSrcY += iBStride;
      piDstY += iEStride;
    }
  }


  //========== vertical upsampling ===========
  pcTempPic->setBorderExtension(false);
  pcTempPic->setHeight(iBHeight);
  pcTempPic->extendPicBorder   (); // extend the border.
  pcTempPic->setHeight(iEHeight);

  const Int nShift = US_FILTER_PREC*2;
  Int iOffset = 1 << (nShift - 1); 

  for( j = 0; j < iEHeight; j++ )
  {
#if PHASE_DERIVATION_IN_INTEGER
    iRefPos16 = ((j*iScaleY + iAddY) >> iShiftYM4) - iDeltaY;
    phase    = iRefPos16 & 15;
    refPos   = iRefPos16 >> 4;
    coeff = m_lumaFilter[phase];
#else
    Int refPos12 = (Int) (j * sFactor12 );
    Int refPos = (Int)( j * sFactor );
    Int phase = (refPos12 + 12) % 12;
    Int* coeff = m_lumaFilter[phase];
#endif

    piSrcY = piTempBufY + (refPos -((NTAPS_US_LUMA>>1) - 1))*iEStride;
    piDstY = piDstBufY + j * iEStride;

    for( i = 0; i < iEWidth; i++ )
    {
      *piDstY = Clip( (sumLumaVer(piSrcY, coeff, iEStride) + iOffset) >> (nShift));
      piSrcY++;
      piDstY++;
    }
  }

  //========== UV component upsampling ===========

  iEWidth  >>= 1;
  iEHeight >>= 1;

  iBWidth  >>= 1;
  iBHeight >>= 1;

  iBStride  = pcBasePic->getCStride();
  iEStride  = pcUsPic->getCStride();

#if PHASE_DERIVATION_IN_INTEGER
  iShiftX = 16;
  iShiftY = 16;

  iPhaseX = 0;
  iPhaseY = 1;

  iAddX       = ( ( ( iBWidth * iPhaseX ) << ( iShiftX - 2 ) ) + ( iEWidth >> 1 ) ) / iEWidth + ( 1 << ( iShiftX - 5 ) );
  iAddY       = ( ( ( iBHeight * iPhaseY ) << ( iShiftY - 2 ) ) + ( iEHeight >> 1 ) ) / iEHeight+ ( 1 << ( iShiftY - 5 ) );

  iDeltaX     = 4 * iPhaseX;
  iDeltaY     = 4 * iPhaseY;

  iShiftXM4 = iShiftX - 4;
  iShiftYM4 = iShiftY - 4;

  iScaleX     = ( ( iBWidth << iShiftX ) + ( iEWidth >> 1 ) ) / iEWidth;
  iScaleY     = ( ( iBHeight << iShiftY ) + ( iEHeight >> 1 ) ) / iEHeight;
#endif

  //========== horizontal upsampling ===========
  for( i = 0; i < iEWidth; i++ )
  {
#if PHASE_DERIVATION_IN_INTEGER
    iRefPos16 = ((i*iScaleX + iAddX) >> iShiftXM4) - iDeltaX;
    phase    = iRefPos16 & 15;
    refPos   = iRefPos16 >> 4;
    coeff = m_chromaFilter[phase];
#else
    Int refPosM = (Int) ( i * chromaPhaseDenominator * sFactor );
    Int refPos = (Int)( i * sFactor );
    Int phase = (refPosM + chromaPhaseDenominator) % chromaPhaseDenominator;
    Int* coeff = m_chromaFilter[phase];
#endif

    piSrcU = piSrcBufU + refPos -((NTAPS_US_CHROMA>>1) - 1);
    piSrcV = piSrcBufV + refPos -((NTAPS_US_CHROMA>>1) - 1);
    piDstU = piTempBufU + i;
    piDstV = piTempBufV + i;

    for( j = 0; j < iBHeight ; j++ )
    {
      *piDstU = sumChromaHor(piSrcU, coeff);
      *piDstV = sumChromaHor(piSrcV, coeff);

      piSrcU += iBStride;
      piSrcV += iBStride;
      piDstU += iEStride;
      piDstV += iEStride;
    }
  }

  //========== vertical upsampling ===========
  pcTempPic->setBorderExtension(false);
  pcTempPic->setHeight(iBHeight << 1);
  pcTempPic->extendPicBorder   (); // extend the border.
  pcTempPic->setHeight(iEHeight << 1);

  for( j = 0; j < iEHeight; j++ )
  {
#if PHASE_DERIVATION_IN_INTEGER
    iRefPos16 = ((j*iScaleY + iAddY) >> iShiftYM4) - iDeltaY;
    phase    = iRefPos16 & 15;
    refPos   = iRefPos16 >> 4; 
    coeff = m_chromaFilter[phase];
#else
    Int refPosM = (Int) (j * chromaPhaseDenominator * sFactor) - 1;
    Int refPos; 
    if ( refPosM < 0 )
    {
      refPos = (Int)( j * sFactor ) - 1;
    }
    else
    {
      refPos = refPosM / chromaPhaseDenominator;
    }
    Int phase = (refPosM + chromaPhaseDenominator) % chromaPhaseDenominator;
    Int* coeff = m_chromaFilter[phase];
#endif

    piSrcU = piTempBufU  + (refPos -((NTAPS_US_CHROMA>>1) - 1))*iEStride;
    piSrcV = piTempBufV  + (refPos -((NTAPS_US_CHROMA>>1) - 1))*iEStride;

    piDstU = piDstBufU + j*iEStride;
    piDstV = piDstBufV + j*iEStride;

    for( i = 0; i < iEWidth; i++ )
    {
      *piDstU = Clip( (sumChromaVer(piSrcU, coeff, iEStride) + iOffset) >> (nShift));
      *piDstV = Clip( (sumChromaVer(piSrcV, coeff, iEStride) + iOffset) >> (nShift));

      piSrcU++;
      piSrcV++;
      piDstU++;
      piDstV++;
    }
  }

  pcUsPic->setBorderExtension(false);
  pcUsPic->extendPicBorder   (); // extend the border.

  //Reset the Border extension flag
  pcUsPic->setBorderExtension(false);
  pcTempPic->setBorderExtension(false);
  pcBasePic->setBorderExtension(false);
  
#if JCTVC_L0178
  // copy back the saved cropped region
  if( tempBufSizeRight )
  {
    // put the correct width back
    pcBasePic->setWidth(pcBasePic->getWidth()+pcBasePic->getPicCropRightOffset());
  }
  if( tempBufSizeBottom )
  {
    pcBasePic->setHeight(pcBasePic->getHeight()+pcBasePic->getPicCropBottomOffset());
  }
  
  iBWidth   = pcBasePic->getWidth () - pcBasePic->getPicCropLeftOffset() - pcBasePic->getPicCropRightOffset();
  iBHeight  = pcBasePic->getHeight() - pcBasePic->getPicCropTopOffset() - pcBasePic->getPicCropBottomOffset();
  
  iBStride  = pcBasePic->getStride();
  
  if( tempBufSizeRight )
  {
    piSrcY = tempBufRight;
    piDstY = piSrcBufY + iBWidth;
    
    for( i = 0; i < pcBasePic->getHeight(); i++ )
    {
      memcpy(piDstY, piSrcY, sizeof(Pel) * pcBasePic->getPicCropRightOffset());
      piSrcY += pcBasePic->getPicCropRightOffset();
      piDstY += iBStride;
    }
    
    if(pcBasePic->getPicCropRightOffset()>>1)
    {
      Int iBStrideChroma = (iBStride>>1);
      piSrcU = tempBufRight + pcBasePic->getPicCropRightOffset() * pcBasePic->getHeight();
      piDstU = piSrcBufU + (iBWidth>>1);
      piSrcV = piSrcU + (pcBasePic->getPicCropRightOffset()>>1) * (pcBasePic->getHeight()>>1);
      piDstV = piSrcBufV + (iBWidth>>1);

      for( i = 0; i < pcBasePic->getHeight()>>1; i++ )
      {
        memcpy(piDstU, piSrcU, sizeof(Pel) * (pcBasePic->getPicCropRightOffset()>>1));
        piSrcU += (pcBasePic->getPicCropRightOffset()>>1);
        piDstU += iBStrideChroma;
        
        memcpy(piDstV, piSrcV, sizeof(Pel) * (pcBasePic->getPicCropRightOffset()>>1));
        piSrcV += (pcBasePic->getPicCropRightOffset()>>1);
        piDstV += iBStrideChroma;
      }
    }
  }
  
  if( tempBufSizeBottom )
  {
    piDstY = piSrcBufY + iBHeight * iBStride;
    piSrcY = tempBufBottom;
    for( i = 0; i < pcBasePic->getPicCropBottomOffset(); i++ )
    {
      memcpy(piDstY, piSrcY, sizeof(Pel) * pcBasePic->getWidth());
      piDstY += iBStride;
      piSrcY += pcBasePic->getWidth();
    }
    
    if(pcBasePic->getPicCropBottomOffset()>>1)
    {
      Int iBStrideChroma = (iBStride>>1);
      piSrcU = tempBufBottom + pcBasePic->getPicCropBottomOffset() * pcBasePic->getWidth();
      piDstU = piSrcBufU + (iBHeight>>1) * iBStrideChroma;
      piSrcV = piSrcU + (pcBasePic->getPicCropBottomOffset()>>1) * (pcBasePic->getWidth()>>1);
      piDstV = piSrcBufV + (iBHeight>>1) * iBStrideChroma;
            
      for( i = 0; i < pcBasePic->getPicCropBottomOffset()>>1; i++ )
      {
        memcpy(piDstU, piSrcU, sizeof(Pel) * (pcBasePic->getWidth()>>1));
        piSrcU += (pcBasePic->getWidth()>>1);
        piDstU += iBStrideChroma;
        
        memcpy(piDstV, piSrcV, sizeof(Pel) * (pcBasePic->getWidth()>>1));
        piSrcV += (pcBasePic->getWidth()>>1);
        piDstV += iBStrideChroma;
      }
    }
  
  }

  if( tempBufSizeRight )
  {
    xFree( tempBufRight );
  }
  if( tempBufSizeBottom )
  {
    xFree( tempBufBottom );
  }
#endif
}
#endif //SVC_EXTENSION
