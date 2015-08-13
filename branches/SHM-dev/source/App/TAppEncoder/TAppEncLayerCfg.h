
/** \file     TAppEncLayerCfg.h
    \brief    Handle encoder layer configuration parameters (header)
*/
#ifndef __TAPPENCLAYERCFG__
#define __TAPPENCLAYERCFG__

#if SVC_EXTENSION
#include "TLibCommon/CommonDef.h"
#include "TLibEncoder/TEncCfg.h"
#include <sstream>
#include <iomanip>

using namespace std;
class TAppEncCfg;
//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder layer configuration class
class TAppEncLayerCfg
{
  friend class TAppEncCfg;
  friend class TAppEncTop;
protected:
  // file I/O0
  string    m_inputFileName;                                     ///< source file name
  string    m_reconFileName;                                     ///< output reconstruction file
  Int       m_layerId;                                        ///< layer Id
  Int       m_iFrameRate;                                     ///< source frame-rates (Hz)
  Int       m_iSourceWidth;                                   ///< source width in pixel
  Int       m_iSourceHeight;                                  ///< source height in pixel (when interlaced = field height)
  Int       m_iSourceHeightOrg;                               ///< original source height in pixel (when interlaced = frame height)
  Int       m_conformanceWindowMode;
  Int       m_confWinLeft;
  Int       m_confWinRight;
  Int       m_confWinTop;
  Int       m_confWinBottom;
  Int       m_aiPad[2];                                       ///< number of padded pixels for width and height
  Int       m_iIntraPeriod;                                   ///< period of I-slice (random access period)
  Int       m_iGOPSize;                                       ///< GOP size of hierarchical structure
  Double    m_fQP;                                            ///< QP value of key-picture (floating point)
  ChromaFormat m_chromaFormatIDC;
  ChromaFormat m_InputChromaFormatIDC;
  ChromaFormat m_chromaFormatConstraint;
  UInt      m_bitDepthConstraint;
  Bool      m_intraConstraintFlag;
  Bool      m_lowerBitRateConstraintFlag;
  Bool      m_onePictureOnlyConstraintFlag;
#if AUXILIARY_PICTURES
  Int       m_auxId;
#endif

  Int       m_extraRPSs;                                     ///< extra RPSs added to handle CRA
  GOPEntry  m_GOPList[MAX_GOP];                              ///< the enhancement layer coding structure entries from the config file
  Int       m_inheritCodingStruct;                           ///< inherit coding structure from certain layer
  Int       m_maxTempLayer;                                  ///< Max temporal layer

  Int       *m_samplePredRefLayerIds;
  Int       m_numSamplePredRefLayers;
  Int       *m_motionPredRefLayerIds;
  Int       m_numMotionPredRefLayers;
  Int       *m_predLayerIds;
  Int       m_numActiveRefLayers;

  Int       m_iMaxCuDQPDepth;                                 ///< Max. depth for a minimum CuDQPSize (0:default)

  // coding unit (CU) definition
  UInt      m_uiMaxCUWidth;                                   ///< max. CU width in pixel
  UInt      m_uiMaxCUHeight;                                  ///< max. CU height in pixel
  UInt      m_uiMaxCUDepth;                                   ///< max. CU depth (as specified by command line)
  UInt      m_uiMaxTotalCUDepth;                              ///< max. total CU depth - includes depth of transform-block structure
  UInt      m_uiLog2DiffMaxMinCodingBlockSize;                ///< difference between largest and smallest CU depth
  
  // transfom unit (TU) definition
  UInt      m_uiQuadtreeTULog2MaxSize;
  UInt      m_uiQuadtreeTULog2MinSize;
  
  UInt      m_uiQuadtreeTUMaxDepthInter;
  UInt      m_uiQuadtreeTUMaxDepthIntra;

#if RC_SHVC_HARMONIZATION
  Bool      m_RCEnableRateControl;                ///< enable rate control or not
  Int       m_RCTargetBitrate;                    ///< target bitrate when rate control is enabled
  Bool      m_RCKeepHierarchicalBit;              ///< whether keeping hierarchical bit allocation structure or not
  Bool      m_RCLCULevelRC;                       ///< true: LCU level rate control; false: picture level rate control
  Bool      m_RCUseLCUSeparateModel;              ///< use separate R-lambda model at LCU level
  Int       m_RCInitialQP;                        ///< inital QP for rate control
  Bool      m_RCForceIntraQP;                     ///< force all intra picture to use initial QP or not
#if U0132_TARGET_BITS_SATURATION
  Bool      m_RCCpbSaturationEnabled;             ///< enable target bits saturation to avoid CPB overflow and underflow
  UInt      m_RCCpbSize;                          ///< CPB size
  Double    m_RCInitialCpbFullness;               ///< initial CPB fullness 
#endif
#endif

  Bool      m_bUseSAO;

  ScalingListMode m_useScalingListId;                         ///< using quantization matrix
  std::string m_scalingListFileName;                          ///< quantization matrix file name

  Int       m_maxTidIlRefPicsPlus1;
  Int       m_waveFrontSynchro;                   ///< 0: no WPP. >= 1: WPP is enabled, the "Top right" from which inheritance occurs is this LCU offset in the line above the current.
  Int       m_waveFrontFlush;                     ///< enable(1)/disable(0) the CABAC flush at the end of each line of LCUs.

  Int       m_iQP;                                            ///< QP value of key-picture (integer)
  std::string m_dQPFileName;                                  ///< QP offset for each slice (initialized from external file)
  Int*      m_aidQP;                                          ///< array of slice QP values
  TAppEncCfg* m_cAppEncCfg;                                   ///< pointer to app encoder config
  Int       m_numRefLayerLocationOffsets;
  Int       m_refLocationOffsetLayerId  [MAX_LAYERS];
  Int       m_scaledRefLayerLeftOffset  [MAX_LAYERS];
  Int       m_scaledRefLayerTopOffset   [MAX_LAYERS];
  Int       m_scaledRefLayerRightOffset [MAX_LAYERS];
  Int       m_scaledRefLayerBottomOffset[MAX_LAYERS];
  Bool      m_scaledRefLayerOffsetPresentFlag [MAX_LAYERS];
  Bool      m_refRegionOffsetPresentFlag      [MAX_LAYERS];
  Int       m_refRegionLeftOffset  [MAX_LAYERS];
  Int       m_refRegionTopOffset   [MAX_LAYERS];
  Int       m_refRegionRightOffset [MAX_LAYERS];
  Int       m_refRegionBottomOffset[MAX_LAYERS];
  Int       m_phaseHorLuma  [MAX_LAYERS];
  Int       m_phaseVerLuma  [MAX_LAYERS];
  Int       m_phaseHorChroma[MAX_LAYERS];
  Int       m_phaseVerChroma[MAX_LAYERS];
  Bool      m_resamplePhaseSetPresentFlag [MAX_LAYERS];

  Int       m_inputBitDepth[MAX_NUM_CHANNEL_TYPE];            ///< bit-depth of input file
  Int       m_outputBitDepth[MAX_NUM_CHANNEL_TYPE];           ///< bit-depth of output file
  Int       m_MSBExtendedBitDepth[MAX_NUM_CHANNEL_TYPE];      ///< bit-depth of input samples after MSB extension
  Int       m_internalBitDepth[MAX_NUM_CHANNEL_TYPE];         ///< bit-depth codec operates at (input/output files will be converted)

  Int       m_repFormatIdx;
#if Q0074_COLOUR_REMAPPING_SEI
  string    m_colourRemapSEIFileName;                         ///< Colour Remapping Information SEI message parameters file
  Int       m_colourRemapSEIId;
  Bool      m_colourRemapSEICancelFlag;
  Bool      m_colourRemapSEIPersistenceFlag;
  Bool      m_colourRemapSEIVideoSignalInfoPresentFlag;
  Bool      m_colourRemapSEIFullRangeFlag;
  Int       m_colourRemapSEIPrimaries;
  Int       m_colourRemapSEITransferFunction;
  Int       m_colourRemapSEIMatrixCoefficients;
  Int       m_colourRemapSEIInputBitDepth;
  Int       m_colourRemapSEIBitDepth;
  Int       m_colourRemapSEIPreLutNumValMinus1[3];
  Int*      m_colourRemapSEIPreLutCodedValue[3];
  Int*      m_colourRemapSEIPreLutTargetValue[3];
  Bool      m_colourRemapSEIMatrixPresentFlag;
  Int       m_colourRemapSEILog2MatrixDenom;
  Int       m_colourRemapSEICoeffs[3][3];
  Int       m_colourRemapSEIPostLutNumValMinus1[3];
  Int*      m_colourRemapSEIPostLutCodedValue[3];
  Int*      m_colourRemapSEIPostLutTargetValue[3];
#endif

  Int       m_layerSwitchOffBegin;
  Int       m_layerSwitchOffEnd;

  // profile/level
  Int       m_layerPTLIdx;

public:
  TAppEncLayerCfg();
  virtual ~TAppEncLayerCfg();

public:
  Void  create    ();                                         ///< create option handling class
  Void  destroy   ();                                         ///< destroy option handling class

  Void    setAppEncCfg(TAppEncCfg* p) {m_cAppEncCfg = p;          }

  string& getInputFileName()          {return m_inputFileName;       }
  string& getReconFileName()          {return m_reconFileName;       }
  Double  getFloatQP()                {return m_fQP;                 }
  Int     getConfWinLeft()            {return m_confWinLeft;         }
  Int     getConfWinRight()           {return m_confWinRight;        }
  Int     getConfWinTop()             {return m_confWinTop;          }
  Int     getConfWinBottom()          {return m_confWinBottom;       }

  Int     getNumSamplePredRefLayers()    {return m_numSamplePredRefLayers;   }
  Int*    getSamplePredRefLayerIds()     {return m_samplePredRefLayerIds;    }
  Int     getSamplePredRefLayerId(Int i) {return m_samplePredRefLayerIds[i]; }
  Int     getNumMotionPredRefLayers()    {return m_numMotionPredRefLayers;   }
  Int*    getMotionPredRefLayerIds()     {return m_motionPredRefLayerIds;    }
  Int     getMotionPredRefLayerId(Int i) {return m_motionPredRefLayerIds[i]; }

  Int     getNumActiveRefLayers()     {return m_numActiveRefLayers;}
  Int*    getPredLayerIds()           {return m_predLayerIds;     }
  Int     getPredLayerIdx(Int i)      {return m_predLayerIds[i];  }

  Int     getRepFormatIdx()           { return m_repFormatIdx;  }
  Void    setRepFormatIdx(Int x)      { m_repFormatIdx = x;     }
  Void    setSourceWidth(Int x)       { m_iSourceWidth = x;     }
  Void    setSourceHeight(Int x)      { m_iSourceHeight = x;    }
  Int     getMaxTidIlRefPicsPlus1()   { return m_maxTidIlRefPicsPlus1; }
#if LAYER_CTB
  UInt    getMaxCUWidth()             {return m_uiMaxCUWidth;      }
  UInt    getMaxCUHeight()            {return m_uiMaxCUHeight;     }
  UInt    getMaxCUDepth()             {return m_uiMaxCUDepth;      }
#endif
}; // END CLASS DEFINITION TAppEncLayerCfg

#endif //SVC_EXTENSION

//! \}

#endif // __TAPPENCLAYERCFG__
