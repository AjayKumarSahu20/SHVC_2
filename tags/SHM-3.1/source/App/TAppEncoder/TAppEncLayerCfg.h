
/** \file     TAppEncLayerCfg.h
    \brief    Handle encoder layer configuration parameters (header)
*/
#ifndef __TAPPENCLAYERCFG__
#define __TAPPENCLAYERCFG__

#include "TLibCommon/CommonDef.h"
#include "TLibEncoder/TEncCfg.h"
#include <sstream>

using namespace std;
#if SVC_EXTENSION
class TAppEncCfg;
#endif
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
  string    m_cInputFile;                                     ///< source file name
  string    m_cReconFile;                                     ///< output reconstruction file

  Int       m_iFrameRate;                                     ///< source frame-rates (Hz)
  Int       m_iSourceWidth;                                   ///< source width in pixel
  Int       m_iSourceHeight;                                  ///< source height in pixel (when interlaced = field height)
  Int       m_iSourceHeightOrg;                               ///< original source height in pixel (when interlaced = frame height)
  Int       m_conformanceMode;
  Int       m_confLeft;
  Int       m_confRight;
  Int       m_confTop;
  Int       m_confBottom;
  Int       m_aiPad[2];                                       ///< number of padded pixels for width and height
  Int       m_iIntraPeriod;                                   ///< period of I-slice (random access period)
  Double    m_fQP;                                            ///< QP value of key-picture (floating point)
#if VPS_EXTN_DIRECT_REF_LAYERS
#if M0457_PREDICTION_INDICATIONS
  Int       *m_samplePredRefLayerIds;
  Int       m_numSamplePredRefLayers;
  Int       *m_motionPredRefLayerIds;
  Int       m_numMotionPredRefLayers;
#else
  Int       *m_refLayerIds;
  Int       m_numDirectRefLayers;
#endif
  Int       *m_predLayerIds;
  Int       m_numActiveRefLayers;
#endif

#if RC_SHVC_HARMONIZATION
  Bool      m_RCEnableRateControl;                ///< enable rate control or not
  Int       m_RCTargetBitrate;                    ///< target bitrate when rate control is enabled
  Bool      m_RCKeepHierarchicalBit;              ///< whether keeping hierarchical bit allocation structure or not
  Bool      m_RCLCULevelRC;                       ///< true: LCU level rate control; false: picture level rate control
  Bool      m_RCUseLCUSeparateModel;              ///< use separate R-lambda model at LCU level
  Int       m_RCInitialQP;                        ///< inital QP for rate control
  Bool      m_RCForceIntraQP;                     ///< force all intra picture to use initial QP or not
#endif

#if N0120_MAX_TID_REF_CFG
  Int       m_maxTidIlRefPicsPlus1;
#endif 
#if SVC_EXTENSION
  Int       m_iWaveFrontSubstreams; //< If iWaveFrontSynchro, this is the number of substreams per frame (dependent tiles) or per tile (independent tiles).
#endif

  Int       m_iQP;                                            ///< QP value of key-picture (integer)
  char*     m_pchdQPFile;                                     ///< QP offset for each slice (initialized from external file)
  Int*      m_aidQP;                                          ///< array of slice QP values
  TAppEncCfg* m_cAppEncCfg;                                   ///< pointer to app encoder config
#if SCALED_REF_LAYER_OFFSETS
  Int       m_numScaledRefLayerOffsets  ;
  Int       m_scaledRefLayerLeftOffset  [MAX_LAYERS];
  Int       m_scaledRefLayerTopOffset   [MAX_LAYERS];
  Int       m_scaledRefLayerRightOffset [MAX_LAYERS];
  Int       m_scaledRefLayerBottomOffset[MAX_LAYERS];
#endif  
#if FINAL_RPL_CHANGE_N0082
  GOPEntry  m_GOPListLayer[MAX_GOP];                            ///< for layer
#endif
#if REPN_FORMAT_IN_VPS
  Int       m_repFormatIdx;
#endif
public:
  TAppEncLayerCfg();
  virtual ~TAppEncLayerCfg();

public:
  Void  create    ();                                         ///< create option handling class
  Void  destroy   ();                                         ///< destroy option handling class
  bool  parseCfg  ( const string& cfgFileName );              ///< parse layer configuration file to fill member variables

#if AVC_SYNTAX
  Void  xPrintParameter( UInt layerId );
#else
  Void  xPrintParameter();
#endif
  Bool  xCheckParameter( Bool isField );

  Void    setAppEncCfg(TAppEncCfg* p) {m_cAppEncCfg = p;          }

  string  getInputFile()              {return m_cInputFile;       }
  string  getReconFile()              {return m_cReconFile;       }
  Int     getFrameRate()              {return m_iFrameRate;       }
  Int     getSourceWidth()            {return m_iSourceWidth;     }
  Int     getSourceHeight()           {return m_iSourceHeight;    }
  Int     getSourceHeightOrg()        {return m_iSourceHeightOrg; }
  Int     getConformanceMode()        { return m_conformanceMode; }
  Int*    getPad()                    {return m_aiPad;            }
  Double  getFloatQP()                {return m_fQP;              }
  Int     getConfLeft()               {return m_confLeft;         }
  Int     getConfRight()              {return m_confRight;        }
  Int     getConfTop()                {return m_confTop;          }
  Int     getConfBottom()             {return m_confBottom;       }

  Int     getIntQP()                  {return m_iQP;              } 
  Int*    getdQPs()                   {return m_aidQP;            }
#if VPS_EXTN_DIRECT_REF_LAYERS
#if M0457_PREDICTION_INDICATIONS
  Int     getNumSamplePredRefLayers()    {return m_numSamplePredRefLayers;   }
  Int*    getSamplePredRefLayerIds()     {return m_samplePredRefLayerIds;    }
  Int     getSamplePredRefLayerId(Int i) {return m_samplePredRefLayerIds[i]; }
  Int     getNumMotionPredRefLayers()    {return m_numMotionPredRefLayers;   }
  Int*    getMotionPredRefLayerIds()     {return m_motionPredRefLayerIds;    }
  Int     getMotionPredRefLayerId(Int i) {return m_motionPredRefLayerIds[i]; }
#else
  Int     getNumDirectRefLayers()     {return m_numDirectRefLayers;}
  Int*    getRefLayerIds()            {return m_refLayerIds;      }
  Int     getRefLayerId(Int i)        {return m_refLayerIds[i];   }
#endif

  Int     getNumActiveRefLayers()     {return m_numActiveRefLayers;}
  Int*    getPredLayerIds()           {return m_predLayerIds;     }
  Int     getPredLayerId(Int i)       {return m_predLayerIds[i];  }
#endif
#if RC_SHVC_HARMONIZATION
  Bool    getRCEnableRateControl()    {return m_RCEnableRateControl;   }
  Int     getRCTargetBitrate()        {return m_RCTargetBitrate;       }
  Bool    getRCKeepHierarchicalBit()  {return m_RCKeepHierarchicalBit; }
  Bool    getRCLCULevelRC()           {return m_RCLCULevelRC;          }
  Bool    getRCUseLCUSeparateModel()  {return m_RCUseLCUSeparateModel; }
  Int     getRCInitialQP()            {return m_RCInitialQP;           }
  Bool    getRCForceIntraQP()         {return m_RCForceIntraQP;        }
#endif
#if FINAL_RPL_CHANGE_N0082
  GOPEntry getGOPEntry(Int i )        {return m_GOPListLayer[i];  }
#endif
#if REPN_FORMAT_IN_VPS
  Int     getRepFormatIdx()           { return m_repFormatIdx;  }
  Void    setRepFormatIdx(Int x)      { m_repFormatIdx = x;     }
  Void    setSourceWidth(Int x)       { m_iSourceWidth = x;     }
  Void    setSourceHeight(Int x)      { m_iSourceHeight = x;    }
#endif
#if N0120_MAX_TID_REF_CFG
  Int     getMaxTidIlRefPicsPlus1()   { return m_maxTidIlRefPicsPlus1; }
#endif 
}; // END CLASS DEFINITION TAppEncLayerCfg

//! \}

#endif // __TAPPENCLAYERCFG__