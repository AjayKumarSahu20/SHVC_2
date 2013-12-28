/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2012, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TAppDecCfg.cpp
    \brief    Decoder configuration class
*/

#include <cstdio>
#include <cstring>
#include <string>
#include "TAppDecCfg.h"
#include "TAppCommon/program_options_lite.h"
#if SVC_EXTENSION
#include <cassert>
#endif

#ifdef WIN32
#define strdup _strdup
#endif

using namespace std;
namespace po = df::program_options_lite;

//! \ingroup TAppDecoder
//! \{

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param argc number of arguments
    \param argv array of arguments
 */
Bool TAppDecCfg::parseCfg( Int argc, Char* argv[] )
{
  bool do_help = false;
  string cfg_BitstreamFile;
#if SVC_EXTENSION
  string cfg_ReconFile [MAX_LAYERS];
  Int nLayerNum;
#if AVC_BASE
  string cfg_BLReconFile;
#endif
#else
  string cfg_ReconFile;
#endif
#if AVC_SYNTAX || SYNTAX_OUTPUT
  string cfg_BLSyntaxFile;
#endif
#if TARGET_DECLAYERID_SET
  string cfg_TargetDecLayerIdSetFile;
#endif

  po::Options opts;
  opts.addOptions()
  ("help", do_help, false, "this help text")
  ("BitstreamFile,b", cfg_BitstreamFile, string(""), "bitstream input file name")
#if SVC_EXTENSION
  ("ReconFileL%d,-o%d",   cfg_ReconFile,   string(""), MAX_LAYERS, "Layer %d reconstructed YUV output file name\n"
                                                     "YUV writing is skipped if omitted")
#if AVC_BASE
  ("BLReconFile,-ibl",    cfg_BLReconFile,  string(""), "BL reconstructed YUV input file name")
  ("BLSourceWidth,-wdt",    m_iBLSourceWidth,        0, "BL source picture width")
  ("BLSourceHeight,-hgt",   m_iBLSourceHeight,       0, "BL source picture height")
#if AVC_SYNTAX
  ("BLSyntaxFile,-ibs",    cfg_BLSyntaxFile,  string(""), "BL syntax input file name")  
#endif
#endif
#else
  ("ReconFile,o",     cfg_ReconFile,     string(""), "reconstructed YUV output file name\n"
                                                     "YUV writing is skipped if omitted")
#endif
#if SYNTAX_OUTPUT
  ("BLSyntaxFile,-ibs",    cfg_BLSyntaxFile,  string(""), "BL syntax input file name")
  ("BLSourceWidth,-wdt",    m_iBLSourceWidth,        0, "BL source picture width")
  ("BLSourceHeight,-hgt",   m_iBLSourceHeight,       0, "BL source picture height")
  ("BLFrames,-fr",          m_iBLFrames,       0, "BL number of frames")
#endif
  ("SkipFrames,s", m_iSkipFrame, 0, "number of frames to skip before random access")
  ("OutputBitDepth,d", m_outputBitDepth, 0u, "bit depth of YUV output file (use 0 for native depth)")
#if SVC_EXTENSION
  ("LayerNum,-ls", nLayerNum, 1, "Number of layers to be decoded.")
#endif 
  ("MaxTemporalLayer,t", m_iMaxTemporalLayer, -1, "Maximum Temporal Layer to be decoded. -1 to decode all layers")
  ("SEIpictureDigest", m_decodedPictureHashSEIEnabled, 1, "Control handling of decoded picture hash SEI messages\n"
                                              "\t3: checksum\n"
                                              "\t2: CRC\n"
                                              "\t1: MD5\n"
                                              "\t0: ignore")
#if TARGET_DECLAYERID_SET
  ("TarDecLayerIdSetFile,l", cfg_TargetDecLayerIdSetFile, string(""), "targetDecLayerIdSet file name. The file should include white space separated LayerId values to be decoded. Omitting the option or a value of -1 in the file decodes all layers.")
#endif
  ;
  po::setDefaults(opts);
  const list<const char*>& argv_unhandled = po::scanArgv(opts, argc, (const char**) argv);

  for (list<const char*>::const_iterator it = argv_unhandled.begin(); it != argv_unhandled.end(); it++)
  {
    fprintf(stderr, "Unhandled argument ignored: `%s'\n", *it);
  }

  if (argc == 1 || do_help)
  {
    po::doHelp(cout, opts);
    return false;
  }

  /* convert std::string to c string for compatability */
  m_pchBitstreamFile = cfg_BitstreamFile.empty() ? NULL : strdup(cfg_BitstreamFile.c_str());
#if SVC_EXTENSION
  m_tgtLayerId = nLayerNum - 1;
  assert( m_tgtLayerId >= 0 );
  for(UInt layer=0; layer<= m_tgtLayerId; layer++)
  {
    m_pchReconFile[layer] = cfg_ReconFile[layer].empty() ? NULL : strdup(cfg_ReconFile[layer].c_str());
  }
#if AVC_BASE
  m_pchBLReconFile = cfg_BLReconFile.empty() ? NULL : strdup(cfg_BLReconFile.c_str());
#endif
#else
  m_pchReconFile = cfg_ReconFile.empty() ? NULL : strdup(cfg_ReconFile.c_str());
#endif
#if AVC_SYNTAX || SYNTAX_OUTPUT
  m_pchBLSyntaxFile = cfg_BLSyntaxFile.empty() ? NULL : strdup(cfg_BLSyntaxFile.c_str());
#endif

  if (!m_pchBitstreamFile)
  {
    fprintf(stderr, "No input file specifed, aborting\n");
    return false;
  }

#if TARGET_DECLAYERID_SET
  if ( !cfg_TargetDecLayerIdSetFile.empty() )
  {
    FILE* targetDecLayerIdSetFile = fopen ( cfg_TargetDecLayerIdSetFile.c_str(), "r" );
    if ( targetDecLayerIdSetFile )
    {
      Bool isLayerIdZeroIncluded = false;
      while ( !feof(targetDecLayerIdSetFile) )
      {
        Int layerIdParsed = 0;
        if ( fscanf( targetDecLayerIdSetFile, "%d ", &layerIdParsed ) != 1 )
        {
          if ( m_targetDecLayerIdSet.size() == 0 )
          {
            fprintf(stderr, "No LayerId could be parsed in file %s. Decoding all LayerIds as default.\n", cfg_TargetDecLayerIdSetFile.c_str() );
          }
          break;
        }
        if ( layerIdParsed  == -1 ) // The file includes a -1, which means all LayerIds are to be decoded.
        {
          m_targetDecLayerIdSet.clear(); // Empty set means decoding all layers.
          break;
        }
        if ( layerIdParsed < 0 || layerIdParsed >= MAX_NUM_LAYER_IDS )
        {
          fprintf(stderr, "Warning! Parsed LayerId %d is not withing allowed range [0,%d]. Ignoring this value.\n", layerIdParsed, MAX_NUM_LAYER_IDS-1 );
        }
        else
        {
          isLayerIdZeroIncluded = layerIdParsed == 0 ? true : isLayerIdZeroIncluded;
          m_targetDecLayerIdSet.push_back ( layerIdParsed );
        }
      }
      fclose (targetDecLayerIdSetFile);
      if ( m_targetDecLayerIdSet.size() > 0 && !isLayerIdZeroIncluded )
      {
        fprintf(stderr, "TargetDecLayerIdSet must contain LayerId=0, aborting" );
        return false;
      }
    }
    else
    {
      fprintf(stderr, "File %s could not be opened. Using all LayerIds as default.\n", cfg_TargetDecLayerIdSetFile.c_str() );
    }
  }
#endif

  return true;
}

//! \}