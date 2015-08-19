/** \file     TAppEncLayerCfg.cpp
\brief    Handle encoder configuration parameters
*/

#include <stdlib.h>
#include <cassert>
#include <cstring>
#include <string>
#include "TLibCommon/TComRom.h"
#include "TAppEncCfg.h"
#include "TAppEncLayerCfg.h"
#include "TAppCommon/program_options_lite.h"

#ifdef WIN32
#define strdup _strdup
#endif

using namespace std;
namespace po = df::program_options_lite;

//! \ingroup TAppEncoder
//! \{


#if AUXILIARY_PICTURES
static inline ChromaFormat numberToChromaFormat(const Int val)
{
  switch (val)
  {
    case 400: return CHROMA_400; break;
    case 420: return CHROMA_420; break;
    case 422: return CHROMA_422; break;
    case 444: return CHROMA_444; break;
    default:  return NUM_CHROMA_FORMAT;
  }
}
#endif

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================
#if SVC_EXTENSION
TAppEncLayerCfg::TAppEncLayerCfg()
: m_cInputFile(string(""))
, m_cReconFile(string(""))
, m_conformanceMode( 0 )
, m_scalingListFile(NULL)
, m_aidQP(NULL)
, m_repFormatIdx(-1)
#if Q0074_COLOUR_REMAPPING_SEI
, m_colourRemapSEIFileRoot(string(""))
#endif
{
#if Q0074_COLOUR_REMAPPING_SEI
  for( Int c=0 ; c<3 ; c++)
  {
    m_colourRemapSEIPreLutCodedValue[c]   = NULL;
    m_colourRemapSEIPreLutTargetValue[c]  = NULL;
    m_colourRemapSEIPostLutCodedValue[c]  = NULL;
    m_colourRemapSEIPostLutTargetValue[c] = NULL;
  }
#endif
  m_confWinLeft = m_confWinRight = m_confWinTop = m_confWinBottom = 0;
  m_aiPad[1] = m_aiPad[0] = 0;
  m_numRefLayerLocationOffsets = 0;
  ::memset(m_refLocationOffsetLayerId,   0, sizeof(m_refLocationOffsetLayerId));
  ::memset(m_scaledRefLayerLeftOffset,   0, sizeof(m_scaledRefLayerLeftOffset));
  ::memset(m_scaledRefLayerTopOffset,    0, sizeof(m_scaledRefLayerTopOffset));
  ::memset(m_scaledRefLayerRightOffset,  0, sizeof(m_scaledRefLayerRightOffset));
  ::memset(m_scaledRefLayerBottomOffset, 0, sizeof(m_scaledRefLayerBottomOffset));
  ::memset(m_scaledRefLayerOffsetPresentFlag, 0, sizeof(m_scaledRefLayerOffsetPresentFlag));
  ::memset(m_refRegionOffsetPresentFlag, 0, sizeof(m_refRegionOffsetPresentFlag));
  ::memset(m_refRegionLeftOffset,   0, sizeof(m_refRegionLeftOffset));
  ::memset(m_refRegionTopOffset,    0, sizeof(m_refRegionTopOffset));
  ::memset(m_refRegionRightOffset,  0, sizeof(m_refRegionRightOffset));
  ::memset(m_refRegionBottomOffset, 0, sizeof(m_refRegionBottomOffset));
  ::memset(m_resamplePhaseSetPresentFlag, 0, sizeof(m_resamplePhaseSetPresentFlag));
  ::memset(m_phaseHorLuma,   0, sizeof(m_phaseHorLuma));
  ::memset(m_phaseVerLuma,   0, sizeof(m_phaseVerLuma));
  ::memset(m_phaseHorChroma, 0, sizeof(m_phaseHorChroma));
  ::memset(m_phaseVerChroma, 0, sizeof(m_phaseVerChroma));
}

TAppEncLayerCfg::~TAppEncLayerCfg()
{
  if ( m_aidQP )
  {
    delete[] m_aidQP;
  }
#if Q0074_COLOUR_REMAPPING_SEI
  for( Int c=0 ; c<3 ; c++)
  {
    if ( m_colourRemapSEIPreLutCodedValue[c] )
    {
      delete[] m_colourRemapSEIPreLutCodedValue[c];
    }
    if ( m_colourRemapSEIPreLutTargetValue[c] )
    {
      delete[] m_colourRemapSEIPreLutTargetValue[c];
    }
    if ( m_colourRemapSEIPostLutCodedValue[c] )
    {
      delete[] m_colourRemapSEIPostLutCodedValue[c];
    }
    if ( m_colourRemapSEIPostLutTargetValue[c] )
    {
      delete[] m_colourRemapSEIPostLutTargetValue[c];
    }
  }
#endif
  free(m_scalingListFile);
}

Void TAppEncLayerCfg::create()
{
}

Void TAppEncLayerCfg::destroy()
{
}


// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param  argc        number of arguments
\param  argv        array of arguments
\retval             true when success
*/
bool TAppEncLayerCfg::parseCfg( const string& cfgFileName  )
{
  string cfg_InputFile;
  string cfg_ReconFile;
  string cfg_dQPFile;
#if AUXILIARY_PICTURES
  Int tmpInputChromaFormat;
  Int tmpChromaFormat;
#endif
#if Q0074_COLOUR_REMAPPING_SEI
  string cfg_colourRemapSEIFileRoot;
#endif

  po::Options opts;
  opts.addOptions()
    ("InputFile,i",           cfg_InputFile,  string(""), "original YUV input file name")
#if AVC_BASE
    ("InputBLFile,-ibl",      cfg_InputFile,  string(""), "original YUV input file name")
#endif
    ("ReconFile,o",           cfg_ReconFile,  string(""), "reconstructed YUV output file name")
    ("SourceWidth,-wdt",      m_iSourceWidth,  0, "Source picture width")
    ("SourceHeight,-hgt",     m_iSourceHeight, 0, "Source picture height")
    ("CroppingMode",          m_conformanceMode,  0, "Cropping mode (0: no cropping, 1:automatic padding, 2: padding, 3:cropping")
#if AUXILIARY_PICTURES
    ("InputChromaFormat",     tmpInputChromaFormat,  420, "InputChromaFormatIDC")
    ("ChromaFormatIDC",       tmpChromaFormat,    420, "ChromaFormatIDC (400|420|422|444 or set 0 (default) for same as InputChromaFormat)")
#endif
    ("ConfLeft",              m_confWinLeft,            0, "Deprecated alias of ConfWinLeft")
    ("ConfRight",             m_confWinRight,           0, "Deprecated alias of ConfWinRight")
    ("ConfTop",               m_confWinTop,             0, "Deprecated alias of ConfWinTop")
    ("ConfBottom",            m_confWinBottom,          0, "Deprecated alias of ConfWinBottom")
    ("ConfWinLeft",           m_confWinLeft,            0, "Left offset for window conformance mode 3")
    ("ConfWinRight",          m_confWinRight,           0, "Right offset for window conformance mode 3")
    ("ConfWinTop",            m_confWinTop,             0, "Top offset for window conformance mode 3")
    ("ConfWinBottom",         m_confWinBottom,          0, "Bottom offset for window conformance mode 3")
    ("HorizontalPadding,-pdx",m_aiPad[0],      0, "horizontal source padding for cropping mode 2")
    ("VerticalPadding,-pdy",  m_aiPad[1],      0, "vertical source padding for cropping mode 2")
    ("IntraPeriod,-ip",       m_iIntraPeriod,  -1, "intra period in frames, (-1: only first frame)")
    ("FrameRate,-fr",         m_iFrameRate,    0, "Frame rate")
    ("dQPFile,m",             cfg_dQPFile, string(""), "dQP file name")
    ("QP,q",                  m_fQP,          30.0, "Qp value, if value is float, QP is switched once during encoding")
#if Q0074_COLOUR_REMAPPING_SEI
    ("SEIColourRemappingInfoFileRoot", cfg_colourRemapSEIFileRoot, string(""), "Colour Remapping Information SEI parameters file name")
#endif
  ;

  po::setDefaults(opts);
  po::parseConfigFile(opts, cfgFileName);

  m_cInputFile = cfg_InputFile;
  m_cReconFile = cfg_ReconFile;
  m_pchdQPFile = cfg_dQPFile.empty() ? NULL : strdup(cfg_dQPFile.c_str());
#if AUXILIARY_PICTURES
  m_InputChromaFormatIDC = numberToChromaFormat(tmpInputChromaFormat);
  m_chromaFormatIDC   = ((tmpChromaFormat == 0) ? (m_InputChromaFormatIDC) : (numberToChromaFormat(tmpChromaFormat)));
#endif
#if Q0074_COLOUR_REMAPPING_SEI
  if( !cfg_colourRemapSEIFileRoot.empty() )
  {
    m_colourRemapSEIFileRoot = strdup(cfg_colourRemapSEIFileRoot.c_str());
  }
#endif

  // reading external dQP description from file
  if ( m_pchdQPFile )
  {
    FILE* fpt=fopen( m_pchdQPFile, "r" );
    if ( fpt )
    {
      Int iValue;
      Int iPOC = 0;
      while ( iPOC < m_cAppEncCfg->getNumFrameToBeEncoded() )
      {
        if ( fscanf(fpt, "%d", &iValue ) == EOF ) break;
        m_aidQP[ iPOC ] = iValue;
        iPOC++;
      }
      fclose(fpt);
    }
  }

  return true;
}

Void TAppEncLayerCfg::xPrintParameter()
{
  printf("Input File                        : %s\n", m_cInputFile.c_str()  );
  printf("Reconstruction File               : %s\n", m_cReconFile.c_str()  );
  printf("Real     Format                   : %dx%d %dHz\n", m_iSourceWidth - ( m_confWinLeft + m_confWinRight ) * TComSPS::getWinUnitX( m_chromaFormatIDC ), m_iSourceHeight - ( m_confWinTop + m_confWinBottom ) * TComSPS::getWinUnitY( m_chromaFormatIDC ), m_iFrameRate );
  printf("Internal Format                   : %dx%d %dHz\n", m_iSourceWidth, m_iSourceHeight, m_iFrameRate );
  printf("PTL index                         : %d\n", m_layerPTLIdx );
  printf("Input bit depth                   : (Y:%d, C:%d)\n", m_inputBitDepth[CHANNEL_TYPE_LUMA], m_inputBitDepth[CHANNEL_TYPE_CHROMA] );
  printf("Internal bit depth                : (Y:%d, C:%d)\n", m_internalBitDepth[CHANNEL_TYPE_LUMA], m_internalBitDepth[CHANNEL_TYPE_CHROMA] );
  printf("PCM sample bit depth              : (Y:%d, C:%d)\n", m_cAppEncCfg->getPCMInputBitDepthFlag() ? m_inputBitDepth[CHANNEL_TYPE_LUMA] : m_internalBitDepth[CHANNEL_TYPE_LUMA], m_cAppEncCfg->getPCMInputBitDepthFlag() ? m_inputBitDepth[CHANNEL_TYPE_CHROMA] : m_internalBitDepth[CHANNEL_TYPE_CHROMA] );
  std::cout << "Input ChromaFormatIDC             :";

  switch (m_InputChromaFormatIDC)
  {
  case CHROMA_400:  std::cout << " 4:0:0"; break;
  case CHROMA_420:  std::cout << " 4:2:0"; break;
  case CHROMA_422:  std::cout << " 4:2:2"; break;
  case CHROMA_444:  std::cout << " 4:4:4"; break;
  default:
    std::cerr << "Invalid";
    exit(1);
  }
  std::cout << std::endl;

  std::cout << "Output (internal) ChromaFormatIDC :";
  switch (m_chromaFormatIDC)
  {
  case CHROMA_400:  std::cout << " 4:0:0"; break;
  case CHROMA_420:  std::cout << " 4:2:0"; break;
  case CHROMA_422:  std::cout << " 4:2:2"; break;
  case CHROMA_444:  std::cout << " 4:4:4"; break;
  default:
    std::cerr << "Invalid";
    exit(1);
  }
  printf("\n");
  printf("CU size / depth / total-depth     : %d / %d / %d\n", m_uiMaxCUWidth, m_uiMaxCUDepth, m_uiMaxTotalCUDepth );
  printf("RQT trans. size (min / max)       : %d / %d\n", 1 << m_uiQuadtreeTULog2MinSize, 1 << m_uiQuadtreeTULog2MaxSize );
  printf("Max RQT depth inter               : %d\n", m_uiQuadtreeTUMaxDepthInter);
  printf("Max RQT depth intra               : %d\n", m_uiQuadtreeTUMaxDepthIntra);
  printf("QP                                : %5.2f\n", m_fQP );
  printf("Max dQP signaling depth           : %d\n", m_iMaxCuDQPDepth);
  printf("Intra period                      : %d\n", m_iIntraPeriod );
#if RC_SHVC_HARMONIZATION                    
  printf("RateControl                       : %d\n", m_RCEnableRateControl );
  if(m_RCEnableRateControl)
  {
    printf("TargetBitrate                     : %d\n", m_RCTargetBitrate );
    printf("KeepHierarchicalBit               : %d\n", m_RCKeepHierarchicalBit );
    printf("LCULevelRC                        : %d\n", m_RCLCULevelRC );
    printf("UseLCUSeparateModel               : %d\n", m_RCUseLCUSeparateModel );
    printf("InitialQP                         : %d\n", m_RCInitialQP );
    printf("ForceIntraQP                      : %d\n", m_RCForceIntraQP );
  }
#endif
  printf("WaveFrontSynchro                  : %d\n", m_waveFrontSynchro);

  const Int iWaveFrontSubstreams = m_waveFrontSynchro ? (m_iSourceHeight + m_uiMaxCUHeight - 1) / m_uiMaxCUHeight : 1;
  printf("WaveFrontSubstreams               : %d\n", iWaveFrontSubstreams);
  printf("ScalingList                       : %d\n", m_useScalingListId );
  printf("PCM                               : %d\n", (m_cAppEncCfg->getUsePCM() && (1<<m_cAppEncCfg->getPCMLog2MinSize()) <= m_uiMaxCUWidth)? 1 : 0);
}

Bool confirmPara(Bool bflag, const char* message);

Bool TAppEncLayerCfg::xCheckParameter( Bool isField )
{
  switch (m_conformanceMode)
  {
  case 0:
    {
      // no cropping or padding
      m_confWinLeft = m_confWinRight = m_confWinTop = m_confWinBottom = 0;
      m_aiPad[1] = m_aiPad[0] = 0;
      break;
    }
  case 1:
    {
      // conformance
      if ((m_confWinLeft != 0) || (m_confWinRight != 0) || (m_confWinTop != 0) || (m_confWinBottom != 0))
      {
        fprintf(stderr, "Warning: Automatic padding enabled, but cropping parameters are set. Undesired size possible.\n");
      }
      if ((m_aiPad[1] != 0) || (m_aiPad[0] != 0))
      {
        fprintf(stderr, "Warning: Automatic padding enabled, but padding parameters are also set\n");
      }

      // automatic padding to minimum CU size
      Int minCuSize = m_uiMaxCUHeight >> (m_uiMaxCUDepth - 1);

      if (m_iSourceWidth % minCuSize)
      {
        m_aiPad[0] = m_confWinRight  = ((m_iSourceWidth / minCuSize) + 1) * minCuSize - m_iSourceWidth;
        m_iSourceWidth  += m_confWinRight;
        m_confWinRight /= TComSPS::getWinUnitX( m_chromaFormatIDC );
      }
      if (m_iSourceHeight % minCuSize)
      {
        m_aiPad[1] = m_confWinBottom = ((m_iSourceHeight / minCuSize) + 1) * minCuSize - m_iSourceHeight;
        m_iSourceHeight += m_confWinBottom;
        if ( isField )
        {
          m_iSourceHeightOrg += m_confWinBottom << 1;
          m_aiPad[1] = m_confWinBottom << 1;
        }
        m_confWinBottom /= TComSPS::getWinUnitY( m_chromaFormatIDC );
      }
      break;
    }
  case 2:
    {
      // conformance
      if ((m_confWinLeft != 0) || (m_confWinRight != 0) || (m_confWinTop != 0) || (m_confWinBottom != 0))
      {
        fprintf(stderr, "Warning: Automatic padding enabled, but cropping parameters are set. Undesired size possible.\n");
      }

      //padding
      m_iSourceWidth  += m_aiPad[0];
      m_iSourceHeight += m_aiPad[1];
      m_confWinRight  = m_aiPad[0];
      m_confWinBottom = m_aiPad[1];
      m_confWinRight /= TComSPS::getWinUnitX( m_chromaFormatIDC );
      m_confWinBottom /= TComSPS::getWinUnitY( m_chromaFormatIDC );
      break;
    }
  case 3:
    {
      // conformance
      if ((m_confWinLeft == 0) && (m_confWinRight == 0) && (m_confWinTop == 0) && (m_confWinBottom == 0))
      {
        fprintf(stderr, "Warning: Cropping enabled, but all cropping parameters set to zero\n");
      }
      if ((m_aiPad[1] != 0) || (m_aiPad[0]!=0))
      {
        fprintf(stderr, "Warning: Cropping enabled, padding parameters will be ignored\n");
      }
      m_aiPad[1] = m_aiPad[0] = 0;
      break;
    }
  }

  // allocate slice-based dQP values
  Int iFrameToBeEncoded = m_cAppEncCfg->getNumFrameToBeEncoded();
  Int iGOPSize = m_cAppEncCfg->getGOPSize();
  if( m_aidQP == NULL )
    m_aidQP = new Int[iFrameToBeEncoded + iGOPSize + 1 ];
  ::memset( m_aidQP, 0, sizeof(Int)*( iFrameToBeEncoded + iGOPSize + 1 ) );

  // handling of floating-point QP values
  // if QP is not integer, sequence is split into two sections having QP and QP+1
  m_iQP = (Int)( m_fQP );
  if ( m_iQP < m_fQP )
  {
    Int iSwitchPOC = (Int)( iFrameToBeEncoded - (m_fQP - m_iQP)*iFrameToBeEncoded + 0.5 );


    iSwitchPOC = (Int)( (Double)iSwitchPOC / iGOPSize + 0.5 )*iGOPSize;
    for ( Int i=iSwitchPOC; i<iFrameToBeEncoded + iGOPSize + 1; i++ )
    {
      m_aidQP[i] = 1;
    }
  }

  UInt maxCUWidth = m_uiMaxCUWidth;
  UInt maxCUHeight = m_uiMaxCUHeight;
  UInt maxCUDepth = m_uiMaxCUDepth;
  bool check_failed = false; /* abort if there is a fatal configuration problem */
#define xConfirmPara(a,b) check_failed |= confirmPara(a,b)
  // check range of parameters
  xConfirmPara( m_iFrameRate <= 0,                                                          "Frame rate must be more than 1" );
  xConfirmPara( (m_iSourceWidth  % (maxCUWidth  >> (maxCUDepth-1)))!=0,             "Resulting coded frame width must be a multiple of the minimum CU size");
  xConfirmPara( (m_iSourceHeight % (maxCUHeight >> (maxCUDepth-1)))!=0,             "Resulting coded frame height must be a multiple of the minimum CU size");
  xConfirmPara( (m_iIntraPeriod > 0 && m_iIntraPeriod < iGOPSize) || m_iIntraPeriod == 0, "Intra period must be more than GOP size, or -1 , not 0" );
  if (m_cAppEncCfg->getDecodingRefreshType() == 2)
  {
    xConfirmPara( m_iIntraPeriod > 0 && m_iIntraPeriod <= iGOPSize ,                      "Intra period must be larger than GOP size for periodic IDR pictures");
  }

  xConfirmPara( m_iQP <  -6 * (m_internalBitDepth[CHANNEL_TYPE_LUMA] - 8) || m_iQP > 51,                "QP exceeds supported range (-QpBDOffsety to 51)" );

  xConfirmPara( m_waveFrontSynchro < 0, "WaveFrontSynchro cannot be negative" );

  //chekc parameters
  xConfirmPara( m_iSourceWidth  % TComSPS::getWinUnitX(CHROMA_420) != 0, "Picture width must be an integer multiple of the specified chroma subsampling");
  xConfirmPara( m_iSourceHeight % TComSPS::getWinUnitY(CHROMA_420) != 0, "Picture height must be an integer multiple of the specified chroma subsampling");

  xConfirmPara( m_aiPad[0] % TComSPS::getWinUnitX(CHROMA_420) != 0, "Horizontal padding must be an integer multiple of the specified chroma subsampling");
  xConfirmPara( m_aiPad[1] % TComSPS::getWinUnitY(CHROMA_420) != 0, "Vertical padding must be an integer multiple of the specified chroma subsampling");

  xConfirmPara( m_iMaxCuDQPDepth > m_uiMaxCUDepth - 1,                                          "Absolute depth for a minimum CuDQP exceeds maximum coding unit depth" );

  xConfirmPara( (m_uiMaxCUWidth  >> m_uiMaxCUDepth) < 4,                                    "Minimum partition width size should be larger than or equal to 8");
  xConfirmPara( (m_uiMaxCUHeight >> m_uiMaxCUDepth) < 4,                                    "Minimum partition height size should be larger than or equal to 8");
  xConfirmPara( m_uiMaxCUWidth < 16,                                                        "Maximum partition width size should be larger than or equal to 16");
  xConfirmPara( m_uiMaxCUHeight < 16,                                                       "Maximum partition height size should be larger than or equal to 16");
  xConfirmPara( m_uiQuadtreeTULog2MinSize < 2,                                        "QuadtreeTULog2MinSize must be 2 or greater.");
  xConfirmPara( m_uiQuadtreeTULog2MaxSize > 5,                                        "QuadtreeTULog2MaxSize must be 5 or smaller.");
  xConfirmPara( (1<<m_uiQuadtreeTULog2MaxSize) > m_uiMaxCUWidth,                                        "QuadtreeTULog2MaxSize must be log2(maxCUSize) or smaller.");
  xConfirmPara( m_uiQuadtreeTULog2MaxSize < m_uiQuadtreeTULog2MinSize,                "QuadtreeTULog2MaxSize must be greater than or equal to m_uiQuadtreeTULog2MinSize.");
  xConfirmPara( (1<<m_uiQuadtreeTULog2MinSize)>(m_uiMaxCUWidth >>(m_uiMaxCUDepth-1)), "QuadtreeTULog2MinSize must not be greater than minimum CU size" ); // HS
  xConfirmPara( (1<<m_uiQuadtreeTULog2MinSize)>(m_uiMaxCUHeight>>(m_uiMaxCUDepth-1)), "QuadtreeTULog2MinSize must not be greater than minimum CU size" ); // HS
  xConfirmPara( ( 1 << m_uiQuadtreeTULog2MinSize ) > ( m_uiMaxCUWidth  >> m_uiMaxCUDepth ), "Minimum CU width must be greater than minimum transform size." );
  xConfirmPara( ( 1 << m_uiQuadtreeTULog2MinSize ) > ( m_uiMaxCUHeight >> m_uiMaxCUDepth ), "Minimum CU height must be greater than minimum transform size." );
  xConfirmPara( m_uiQuadtreeTUMaxDepthInter < 1,                                                         "QuadtreeTUMaxDepthInter must be greater than or equal to 1" );
  xConfirmPara( m_uiMaxCUWidth < ( 1 << (m_uiQuadtreeTULog2MinSize + m_uiQuadtreeTUMaxDepthInter - 1) ), "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1" );
  xConfirmPara( m_uiQuadtreeTUMaxDepthIntra < 1,                                                         "QuadtreeTUMaxDepthIntra must be greater than or equal to 1" );
  xConfirmPara( m_uiMaxCUWidth < ( 1 << (m_uiQuadtreeTULog2MinSize + m_uiQuadtreeTUMaxDepthIntra - 1) ), "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1" );

  // max CU width and height should be power of 2
  UInt ui = m_uiMaxCUWidth;
  while(ui)
  {
    ui >>= 1;
    if( (ui & 1) == 1)
      xConfirmPara( ui != 1 , "Width should be 2^n");
  }
  ui = m_uiMaxCUHeight;
  while(ui)
  {
    ui >>= 1;
    if( (ui & 1) == 1)
      xConfirmPara( ui != 1 , "Height should be 2^n");
  }

#undef xConfirmPara
  return check_failed;
}

#endif //SVC_EXTENSION


//! \}