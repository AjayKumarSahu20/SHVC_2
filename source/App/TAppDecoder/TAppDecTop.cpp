/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2014, ITU/ISO/IEC
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

/** \file     TAppDecTop.cpp
    \brief    Decoder application class
*/

#include <list>
#include <vector>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "TAppDecTop.h"
#include "TLibDecoder/AnnexBread.h"
#include "TLibDecoder/NALread.h"
#if RExt__DECODER_DEBUG_BIT_STATISTICS
#include "TLibCommon/TComCodingStatistics.h"
#endif
#if CONFORMANCE_BITSTREAM_MODE
#include "TLibCommon/TComPicYuv.h"
#include "libmd5/MD5.h"
#endif

//! \ingroup TAppDecoder
//! \{

#if Q0074_COLOUR_REMAPPING_SEI
static Void applyColourRemapping(TComPicYuv& pic, const SEIColourRemappingInfo* colourRemappingInfoSEI, UInt layerId=0 );
static std::vector<SEIColourRemappingInfo> storeCriSEI; //Persistent Colour Remapping Information SEI
static SEIColourRemappingInfo *seiColourRemappingInfoPrevious=NULL ;
#endif

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

#if SVC_EXTENSION
TAppDecTop::TAppDecTop()
{
  for(UInt layer=0; layer < MAX_LAYERS; layer++)
  {
    m_aiPOCLastDisplay[layer]  = -MAX_INT;
    m_apcTDecTop[layer] = &m_acTDecTop[layer];
  }
}
#else
TAppDecTop::TAppDecTop()
: m_iPOCLastDisplay(-MAX_INT)
{
}
#endif

Void TAppDecTop::create()
{
}

Void TAppDecTop::destroy()
{
  if (m_pchBitstreamFile)
  {
    free (m_pchBitstreamFile);
    m_pchBitstreamFile = NULL;
  }
#if SVC_EXTENSION
#if CONFORMANCE_BITSTREAM_MODE
  for(Int i = 0; i < MAX_VPS_LAYER_IDX_PLUS1; i++ )
#else
  for( Int i = 0; i <= m_tgtLayerId; i++ )
#endif
  {
    if( m_pchReconFile[i] )
    {
      free ( m_pchReconFile[i] );
      m_pchReconFile[i] = NULL;
    }
  }
#if AVC_BASE
  if( m_pchBLReconFile )
  {
    free ( m_pchBLReconFile );
    m_pchBLReconFile = NULL;
  }
#endif
#else
  if (m_pchReconFile)
  {
    free (m_pchReconFile);
    m_pchReconFile = NULL;
  }
#endif
#if Q0074_COLOUR_REMAPPING_SEI
  if (seiColourRemappingInfoPrevious != NULL)
  {
    delete seiColourRemappingInfoPrevious;
  }
#endif
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 - create internal class
 - initialize internal class
 - until the end of the bitstream, call decoding function in TDecTop class
 - delete allocated buffers
 - destroy internal class
 .
 */
#if SVC_EXTENSION
Void TAppDecTop::decode()
{
  Int                poc;
  TComList<TComPic*>* pcListPic = NULL;

  ifstream bitstreamFile(m_pchBitstreamFile, ifstream::in | ifstream::binary);
  if (!bitstreamFile)
  {
    fprintf(stderr, "\nfailed to open bitstream file `%s' for reading\n", m_pchBitstreamFile);
    exit(EXIT_FAILURE);
  }

  InputByteStream bytestream(bitstreamFile);

  if (!m_outputDecodedSEIMessagesFilename.empty() && m_outputDecodedSEIMessagesFilename!="-")
  {
    m_seiMessageFileStream.open(m_outputDecodedSEIMessagesFilename.c_str(), std::ios::out);
    if (!m_seiMessageFileStream.is_open() || !m_seiMessageFileStream.good())
    {
      fprintf(stderr, "\nUnable to open file `%s' for writing decoded SEI messages\n", m_outputDecodedSEIMessagesFilename.c_str());
      exit(EXIT_FAILURE);
    }
  }

  // create & initialize internal classes
  xCreateDecLib();
  xInitDecLib  ();  

  // main decoder loop
  Bool openedReconFile[MAX_LAYERS]; // reconstruction file not yet opened. (must be performed after SPS is seen)
  Bool loopFiltered[MAX_LAYERS];
  memset( loopFiltered, false, sizeof( loopFiltered ) );

#if CONFORMANCE_BITSTREAM_MODE
  for(UInt layer = 0; layer < MAX_VPS_LAYER_IDX_PLUS1; layer++)
#else
  for(UInt layer=0; layer<=m_tgtLayerId; layer++)
#endif
  {
    openedReconFile[layer] = false;
    m_aiPOCLastDisplay[layer] += m_iSkipFrame;      // set the last displayed POC correctly for skip forward.
  }

  UInt curLayerId = 0;     // current layer to be reconstructed

#if AVC_BASE
  TComPic pcBLPic;
  fstream streamYUV;
  if( m_pchBLReconFile )
  {
    streamYUV.open( m_pchBLReconFile, fstream::in | fstream::binary );
  }
  TComList<TComPic*> *cListPic = m_acTDecTop[0].getListPic();
  m_acTDecTop[0].setBLReconFile( &streamYUV );
  pcBLPic.setLayerId( 0 );
  cListPic->pushBack( &pcBLPic );
#endif

  while (!!bitstreamFile)
  {
    /* location serves to work around a design fault in the decoder, whereby
     * the process of reading a new slice that is the first slice of a new frame
     * requires the TDecTop::decode() method to be called again with the same
     * nal unit. */
#if RExt__DECODER_DEBUG_BIT_STATISTICS
    TComCodingStatistics::TComCodingStatisticsData backupStats(TComCodingStatistics::GetStatistics());
    streampos location = bitstreamFile.tellg() - streampos(bytestream.GetNumBufferedBytes());
#else
    streampos location = bitstreamFile.tellg();
#endif
    AnnexBStats stats = AnnexBStats();

    vector<uint8_t> nalUnit;
    InputNALUnit nalu;
    byteStreamNALUnit(bytestream, nalUnit, stats);

    // call actual decoding function
    Bool bNewPicture = false;
    Bool bNewPOC = false;

    if (nalUnit.empty())
    {
      /* this can happen if the following occur:
       *  - empty input file
       *  - two back-to-back start_code_prefixes
       *  - start_code_prefix immediately followed by EOF
       */
      fprintf(stderr, "Warning: Attempt to decode an empty NAL unit\n");
    }
    else
    {
      read(nalu, nalUnit);
      if( (m_iMaxTemporalLayer >= 0 && nalu.m_temporalId > m_iMaxTemporalLayer) || !isNaluWithinTargetDecLayerIdSet(&nalu)  ||
#if CONFORMANCE_BITSTREAM_MODE
        (nalu.m_layerId > m_commonDecoderParams.getTargetLayerId()) )
#else
        (nalu.m_layerId > m_tgtLayerId) )
#endif
      {
        bNewPicture = false;
      }
      else
      {
        bNewPicture = m_acTDecTop[nalu.m_layerId].decode(nalu, m_iSkipFrame, m_aiPOCLastDisplay[nalu.m_layerId], curLayerId, bNewPOC);
#if POC_RESET_IDC_DECODER
        if ( (bNewPicture && m_acTDecTop[nalu.m_layerId].getParseIdc() == 3) || (m_acTDecTop[nalu.m_layerId].getParseIdc() == 0) )
#else
        if (bNewPicture)
#endif
        {
          bitstreamFile.clear();
          /* location points to the current nalunit payload[1] due to the
           * need for the annexB parser to read three extra bytes.
           * [1] except for the first NAL unit in the file
           *     (but bNewPicture doesn't happen then) */
#if RExt__DECODER_DEBUG_BIT_STATISTICS
          bitstreamFile.seekg(location);
          bytestream.reset();
          TComCodingStatistics::SetStatistics(backupStats);
#else
          bitstreamFile.seekg(location-streamoff(3));
          bytestream.reset();
#endif
        }
#if POC_RESET_IDC_DECODER
        else if(m_acTDecTop[nalu.m_layerId].getParseIdc() == 1) 
        {
          bitstreamFile.clear();
          // This is before third parse of the NAL unit, and 
          // location points to correct beginning of the NALU
          bitstreamFile.seekg(location);
          bytestream.reset();
#if RExt__DECODER_DEBUG_BIT_STATISTICS
          TComCodingStatistics::SetStatistics(backupStats);
#endif
        }
#endif
      }
    }

#if POC_RESET_IDC_DECODER
    if ( ( (bNewPicture && m_acTDecTop[nalu.m_layerId].getParseIdc() == 3) || m_acTDecTop[nalu.m_layerId].getParseIdc() == 0 || !bitstreamFile || nalu.m_nalUnitType == NAL_UNIT_EOS ) && 
        !m_acTDecTop[nalu.m_layerId].getFirstSliceInSequence() )
#else
    if ( (bNewPicture || !bitstreamFile || nalu.m_nalUnitType == NAL_UNIT_EOS) &&
        !m_acTDecTop[nalu.m_layerId].getFirstSliceInSequence() )
#endif
    {
#if O0194_DIFFERENT_BITDEPTH_EL_BL
      //Bug fix: The bit depth was not set correctly for each layer when doing DBF
      g_bitDepth[CHANNEL_TYPE_LUMA]   = g_bitDepthLayer[CHANNEL_TYPE_LUMA][curLayerId];
      g_bitDepth[CHANNEL_TYPE_CHROMA] = g_bitDepthLayer[CHANNEL_TYPE_CHROMA][curLayerId];
#endif
      if (!loopFiltered[curLayerId] || bitstreamFile)
      {
        m_acTDecTop[curLayerId].executeLoopFilters(poc, pcListPic);
      }
      loopFiltered[curLayerId] = (nalu.m_nalUnitType == NAL_UNIT_EOS);

      if (nalu.m_nalUnitType == NAL_UNIT_EOS)
      {
        m_acTDecTop[nalu.m_layerId].setFirstSliceInSequence(true);
      }
    }
    else if ( (bNewPicture || !bitstreamFile || nalu.m_nalUnitType == NAL_UNIT_EOS ) &&
              m_acTDecTop[nalu.m_layerId].getFirstSliceInSequence () ) 
    {
      m_acTDecTop[nalu.m_layerId].setFirstSliceInPicture (true);
    }

#if POC_RESET_IDC_DECODER
    if( bNewPicture && m_acTDecTop[nalu.m_layerId].getParseIdc() == 0 )
    {
      outputAllPictures( nalu.m_layerId, true );
    }
#endif

    if( pcListPic )
    {
      if ( m_pchReconFile[curLayerId] && !openedReconFile[curLayerId] )
      {
        for (UInt channelType = 0; channelType < MAX_NUM_CHANNEL_TYPE; channelType++)
        {
          if (m_outputBitDepth[channelType] == 0) m_outputBitDepth[channelType] = g_bitDepth[channelType];
        }
        m_acTVideoIOYuvReconFile[curLayerId].open( m_pchReconFile[curLayerId], true, m_outputBitDepth, m_outputBitDepth, g_bitDepth ); // write mode

        openedReconFile[curLayerId] = true;
      }
#if ALIGNED_BUMPING
      Bool outputPicturesFlag = true;  

      if( m_acTDecTop[nalu.m_layerId].getNoOutputPriorPicsFlag() )
      {
        outputPicturesFlag = false;
      }

      if (nalu.m_nalUnitType == NAL_UNIT_EOS) // End of sequence
      {
        flushAllPictures( nalu.m_layerId, outputPicturesFlag );       
      }

#if POC_RESET_IDC_DECODER
      if( bNewPicture && m_acTDecTop[nalu.m_layerId].getParseIdc() != 0 )
      // New picture, slice header parsed but picture not decoded
#else
      if( bNewPicture ) // New picture, slice header parsed but picture not decoded
#endif
      {
         if(   nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_RADL
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_LP   )
        {
          flushAllPictures( nalu.m_layerId, outputPicturesFlag );
        }
        else
        {
          this->checkOutputBeforeDecoding( nalu.m_layerId );
        }
      }

      /* The following code has to be executed when the last DU of the picture is decoded
         TODO: Need code to identify end of decoding a picture
      {
        this->checkOutputAfterDecoding( );
      } */
#else
      if ( bNewPicture && bNewPOC &&
           (   nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_RADL
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_LP ) )
      {
        xFlushOutput( pcListPic, curLayerId );
      }
      if (nalu.m_nalUnitType == NAL_UNIT_EOS)
      {
        xFlushOutput( pcListPic, curLayerId );        
      }
      // write reconstruction to file
      if(bNewPicture)
      {
        xWriteOutput( pcListPic, curLayerId, nalu.m_temporalId );
      }
#endif
    }
  }
#if ALIGNED_BUMPING
   flushAllPictures( true );   
#else
  for(UInt layer = 0; layer <= m_tgtLayerId; layer++)
  {
    xFlushOutput( m_acTDecTop[layer].getListPic(), layer );
  }
#endif
  // delete buffers
#if AVC_BASE
  UInt layerIdxmin = m_acTDecTop[0].getBLReconFile()->is_open() ? 1 : 0;

  if( streamYUV.is_open() )
  {
    streamYUV.close();
  }
  pcBLPic.destroy();

#if CONFORMANCE_BITSTREAM_MODE
  for(UInt layer = layerIdxmin; layer < MAX_VPS_LAYER_IDX_PLUS1; layer++)
#else
  for(UInt layer = layerIdxmin; layer <= m_tgtLayerId; layer++)
#endif
#else
  for(UInt layer = 0; layer <= m_tgtLayerId; layer++)
#endif
  {
    m_acTDecTop[layer].deletePicBuffer();
  }

  // destroy internal classes
  xDestroyDecLib();
}
#else
Void TAppDecTop::decode()
{
  Int                 poc;
  TComList<TComPic*>* pcListPic = NULL;

  ifstream bitstreamFile(m_pchBitstreamFile, ifstream::in | ifstream::binary);
  if (!bitstreamFile)
  {
    fprintf(stderr, "\nfailed to open bitstream file `%s' for reading\n", m_pchBitstreamFile);
    exit(EXIT_FAILURE);
  }

  InputByteStream bytestream(bitstreamFile);

  if (!m_outputDecodedSEIMessagesFilename.empty() && m_outputDecodedSEIMessagesFilename!="-")
  {
    m_seiMessageFileStream.open(m_outputDecodedSEIMessagesFilename.c_str(), std::ios::out);
    if (!m_seiMessageFileStream.is_open() || !m_seiMessageFileStream.good())
    {
      fprintf(stderr, "\nUnable to open file `%s' for writing decoded SEI messages\n", m_outputDecodedSEIMessagesFilename.c_str());
      exit(EXIT_FAILURE);
    }
  }

  // create & initialize internal classes
  xCreateDecLib();
  xInitDecLib  ();
  m_iPOCLastDisplay += m_iSkipFrame;      // set the last displayed POC correctly for skip forward.

  // main decoder loop
  Bool openedReconFile = false; // reconstruction file not yet opened. (must be performed after SPS is seen)
  Bool loopFiltered = false;

  while (!!bitstreamFile)
  {
    /* location serves to work around a design fault in the decoder, whereby
     * the process of reading a new slice that is the first slice of a new frame
     * requires the TDecTop::decode() method to be called again with the same
     * nal unit. */
#if RExt__DECODER_DEBUG_BIT_STATISTICS
    TComCodingStatistics::TComCodingStatisticsData backupStats(TComCodingStatistics::GetStatistics());
    streampos location = bitstreamFile.tellg() - streampos(bytestream.GetNumBufferedBytes());
#else
    streampos location = bitstreamFile.tellg();
#endif
    AnnexBStats stats = AnnexBStats();

    vector<uint8_t> nalUnit;
    InputNALUnit nalu;
    byteStreamNALUnit(bytestream, nalUnit, stats);

    // call actual decoding function
    Bool bNewPicture = false;
    if (nalUnit.empty())
    {
      /* this can happen if the following occur:
       *  - empty input file
       *  - two back-to-back start_code_prefixes
       *  - start_code_prefix immediately followed by EOF
       */
      fprintf(stderr, "Warning: Attempt to decode an empty NAL unit\n");
    }
    else
    {
      read(nalu, nalUnit);
      if( (m_iMaxTemporalLayer >= 0 && nalu.m_temporalId > m_iMaxTemporalLayer) || !isNaluWithinTargetDecLayerIdSet(&nalu)  )
      {
        bNewPicture = false;
      }
      else
      {
        bNewPicture = m_cTDecTop.decode(nalu, m_iSkipFrame, m_iPOCLastDisplay);
        if (bNewPicture)
        {
          bitstreamFile.clear();
          /* location points to the current nalunit payload[1] due to the
           * need for the annexB parser to read three extra bytes.
           * [1] except for the first NAL unit in the file
           *     (but bNewPicture doesn't happen then) */
#if RExt__DECODER_DEBUG_BIT_STATISTICS
          bitstreamFile.seekg(location);
          bytestream.reset();
          TComCodingStatistics::SetStatistics(backupStats);
#else
          bitstreamFile.seekg(location-streamoff(3));
          bytestream.reset();
#endif
        }
      }
    }

    if ( (bNewPicture || !bitstreamFile || nalu.m_nalUnitType == NAL_UNIT_EOS) &&
        !m_cTDecTop.getFirstSliceInSequence () )
    {
      if (!loopFiltered || bitstreamFile)
      {
        m_cTDecTop.executeLoopFilters(poc, pcListPic);
      }
      loopFiltered = (nalu.m_nalUnitType == NAL_UNIT_EOS);
      if (nalu.m_nalUnitType == NAL_UNIT_EOS)
      {
        m_cTDecTop.setFirstSliceInSequence(true);
      }
    }
    else if ( (bNewPicture || !bitstreamFile || nalu.m_nalUnitType == NAL_UNIT_EOS ) &&
              m_cTDecTop.getFirstSliceInSequence () ) 
    {
      m_cTDecTop.setFirstSliceInPicture (true);
    }

    if( pcListPic )
    {
      if ( m_pchReconFile && !openedReconFile )
      {
        for (UInt channelType = 0; channelType < MAX_NUM_CHANNEL_TYPE; channelType++)
        {
          if (m_outputBitDepth[channelType] == 0) m_outputBitDepth[channelType] = g_bitDepth[channelType];
        }

        m_cTVideoIOYuvReconFile.open( m_pchReconFile, true, m_outputBitDepth, m_outputBitDepth, g_bitDepth ); // write mode
        openedReconFile = true;
      }
      // write reconstruction to file
      if( bNewPicture )
      {
        xWriteOutput( pcListPic, nalu.m_temporalId );
      }
      if ( (bNewPicture || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_CRA) && m_cTDecTop.getNoOutputPriorPicsFlag() )
      {
        m_cTDecTop.checkNoOutputPriorPics( pcListPic );
        m_cTDecTop.setNoOutputPriorPicsFlag (false);
      }
      if ( bNewPicture &&
           (   nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_RADL
            || nalu.m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_LP ) )
      {
        xFlushOutput( pcListPic );
      }
      if (nalu.m_nalUnitType == NAL_UNIT_EOS)
      {
        xWriteOutput( pcListPic, nalu.m_temporalId );
        m_cTDecTop.setFirstSliceInPicture (false);
      }
      // write reconstruction to file -- for additional bumping as defined in C.5.2.3
      if(!bNewPicture && nalu.m_nalUnitType >= NAL_UNIT_CODED_SLICE_TRAIL_N && nalu.m_nalUnitType <= NAL_UNIT_RESERVED_VCL31)
      {
        xWriteOutput( pcListPic, nalu.m_temporalId );
      }
    }
  }

  xFlushOutput( pcListPic );
  // delete buffers
  m_cTDecTop.deletePicBuffer();

  // destroy internal classes
  xDestroyDecLib();
}
#endif

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

Void TAppDecTop::xCreateDecLib()
{
#if SVC_EXTENSION
  // initialize global variables
  initROM();

#if CONFORMANCE_BITSTREAM_MODE
  for(UInt layer = 0; layer < MAX_VPS_LAYER_IDX_PLUS1; layer++)
#else
  for(UInt layer = 0; layer <= m_tgtLayerId; layer++)
#endif
  {
    // set layer ID
    m_acTDecTop[layer].setLayerId                      ( layer );

    // create decoder class
    m_acTDecTop[layer].create();

    m_acTDecTop[layer].setLayerDec(m_apcTDecTop);
  }
#else
  // create decoder class
  m_cTDecTop.create();
#endif
}

Void TAppDecTop::xDestroyDecLib()
{
#if SVC_EXTENSION
  // destroy ROM
  destroyROM();

#if CONFORMANCE_BITSTREAM_MODE
  for(UInt layer = 0; layer < MAX_VPS_LAYER_IDX_PLUS1; layer++)
#else
  for(UInt layer = 0; layer <= m_tgtLayerId; layer++)
#endif
  {
    if ( m_pchReconFile[layer] )
    {
      m_acTVideoIOYuvReconFile[layer].close();
    }

    // destroy decoder class
    m_acTDecTop[layer].destroy();
  }
#else
  if ( m_pchReconFile )
  {
    m_cTVideoIOYuvReconFile. close();
  }

  // destroy decoder class
  m_cTDecTop.destroy();
#endif
}

Void TAppDecTop::xInitDecLib()
{
  // initialize decoder class
#if SVC_EXTENSION
#if CONFORMANCE_BITSTREAM_MODE
  for(UInt layer = 0; layer < MAX_VPS_LAYER_IDX_PLUS1; layer++)
#else
  for(UInt layer = 0; layer <= m_tgtLayerId; layer++)
#endif
  {
    m_acTDecTop[layer].init();
    m_acTDecTop[layer].setDecodedPictureHashSEIEnabled(m_decodedPictureHashSEIEnabled);
#if CONFORMANCE_BITSTREAM_MODE
    m_acTDecTop[layer].setNumLayer( MAX_LAYERS );
#else
    m_acTDecTop[layer].setNumLayer( m_tgtLayerId + 1 );
#endif
#if OUTPUT_LAYER_SET_INDEX
    m_acTDecTop[layer].setCommonDecoderParams( &m_commonDecoderParams );
#endif
  }
#if CONFORMANCE_BITSTREAM_MODE
  for(UInt layer = 0; layer < MAX_VPS_LAYER_IDX_PLUS1; layer++)
  {
    m_acTDecTop[layer].setConfModeFlag( m_confModeFlag );
  }
#endif
#else
  m_cTDecTop.init();
  m_cTDecTop.setDecodedPictureHashSEIEnabled(m_decodedPictureHashSEIEnabled);
#if O0043_BEST_EFFORT_DECODING
  m_cTDecTop.setForceDecodeBitDepth(m_forceDecodeBitDepth);
#endif
  if (!m_outputDecodedSEIMessagesFilename.empty())
  {
    std::ostream &os=m_seiMessageFileStream.is_open() ? m_seiMessageFileStream : std::cout;
    m_cTDecTop.setDecodedSEIMessageOutputStream(&os);
  }
#endif
}

/** \param pcListPic list of pictures to be written to file
    \todo            DYN_REF_FREE should be revised
 */
#if SVC_EXTENSION
Void TAppDecTop::xWriteOutput( TComList<TComPic*>* pcListPic, UInt layerId, UInt tId )
#else
Void TAppDecTop::xWriteOutput( TComList<TComPic*>* pcListPic, UInt tId )
#endif
{
  if (pcListPic->empty())
  {
    return;
  }

  TComList<TComPic*>::iterator iterPic   = pcListPic->begin();
  Int numPicsNotYetDisplayed = 0;
  Int dpbFullness = 0;
#if SVC_EXTENSION
TComSPS* activeSPS = m_acTDecTop[layerId].getActiveSPS();
#else
  TComSPS* activeSPS = m_cTDecTop.getActiveSPS();
#endif
  UInt numReorderPicsHighestTid;
  UInt maxDecPicBufferingHighestTid;
  UInt maxNrSublayers = activeSPS->getMaxTLayers();

  if(m_iMaxTemporalLayer == -1 || m_iMaxTemporalLayer >= maxNrSublayers)
  {
    numReorderPicsHighestTid = activeSPS->getNumReorderPics(maxNrSublayers-1);
    maxDecPicBufferingHighestTid =  activeSPS->getMaxDecPicBuffering(maxNrSublayers-1); 
  }
  else
  {
    numReorderPicsHighestTid = activeSPS->getNumReorderPics(m_iMaxTemporalLayer);
    maxDecPicBufferingHighestTid = activeSPS->getMaxDecPicBuffering(m_iMaxTemporalLayer); 
  }

  while (iterPic != pcListPic->end())
  {
    TComPic* pcPic = *(iterPic);
#if SVC_EXTENSION
    if(pcPic->getOutputMark() && pcPic->getPOC() > m_aiPOCLastDisplay[layerId])
#else
    if(pcPic->getOutputMark() && pcPic->getPOC() > m_iPOCLastDisplay)
#endif
    {
       numPicsNotYetDisplayed++;
      dpbFullness++;
    }
    else if(pcPic->getSlice( 0 )->isReferenced())
    {
      dpbFullness++;
    }
    iterPic++;
  }

  iterPic = pcListPic->begin();

  if (numPicsNotYetDisplayed>2)
  {
    iterPic++;
  }

  TComPic* pcPic = *(iterPic);
  if (numPicsNotYetDisplayed>2 && pcPic->isField()) //Field Decoding
  {
    TComList<TComPic*>::iterator endPic   = pcListPic->end();
    endPic--;
    iterPic   = pcListPic->begin();
    while (iterPic != endPic)
    {
      TComPic* pcPicTop = *(iterPic);
      iterPic++;
      TComPic* pcPicBottom = *(iterPic);

#if SVC_EXTENSION
      if( pcPicTop->getOutputMark() && pcPicBottom->getOutputMark() &&
        (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid) &&
        (!(pcPicTop->getPOC()%2) && pcPicBottom->getPOC() == pcPicTop->getPOC()+1) &&        
        (pcPicTop->getPOC() == m_aiPOCLastDisplay[layerId]+1 || m_aiPOCLastDisplay[layerId]<0) )
#else
      if ( pcPicTop->getOutputMark() && pcPicBottom->getOutputMark() &&
          (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid) &&
          (!(pcPicTop->getPOC()%2) && pcPicBottom->getPOC() == pcPicTop->getPOC()+1) &&
          (pcPicTop->getPOC() == m_iPOCLastDisplay+1 || m_iPOCLastDisplay < 0))
#endif
      {
        // write to file
        numPicsNotYetDisplayed = numPicsNotYetDisplayed-2;
#if SVC_EXTENSION
        if ( m_pchReconFile[layerId] )
        {
          const Window &conf = pcPicTop->getConformanceWindow();
          const Window &defDisp = m_respectDefDispWindow ? pcPicTop->getDefDisplayWindow() : Window();
          const Bool isTff = pcPicTop->isTopField();

          Bool display = true;
          if( m_decodedNoDisplaySEIEnabled )
          {
            SEIMessages noDisplay = getSeisByType(pcPic->getSEIs(), SEI::NO_DISPLAY );
            const SEINoDisplay *nd = ( noDisplay.size() > 0 ) ? (SEINoDisplay*) *(noDisplay.begin()) : NULL;
            if( (nd != NULL) && nd->m_noDisplay )
            {
              display = false;
            }
          }

          if (display)
          {            
#if REPN_FORMAT_IN_VPS
            UInt chromaFormatIdc = pcPic->getSlice(0)->getChromaFormatIdc();
            Int xScal =  TComSPS::getWinUnitX( chromaFormatIdc ), yScal = TComSPS::getWinUnitY( chromaFormatIdc );

            m_acTVideoIOYuvReconFile[layerId].write( pcPicTop->getPicYuvRec(), pcPicBottom->getPicYuvRec(),
              m_outputColourSpaceConvert,
              conf.getWindowLeftOffset()  * xScal + defDisp.getWindowLeftOffset(),
              conf.getWindowRightOffset() * xScal + defDisp.getWindowRightOffset(),
              conf.getWindowTopOffset()   * yScal + defDisp.getWindowTopOffset(),
              conf.getWindowBottomOffset()* yScal + defDisp.getWindowBottomOffset(), NUM_CHROMA_FORMAT, isTff );
#else
            m_acTVideoIOYuvReconFile[layerId].write( pcPicTop->getPicYuvRec(), pcPicBottom->getPicYuvRec(),
              m_outputColourSpaceConvert,
              conf.getWindowLeftOffset()   + defDisp.getWindowLeftOffset(),
              conf.getWindowRightOffset()  + defDisp.getWindowRightOffset(),
              conf.getWindowTopOffset()    + defDisp.getWindowTopOffset(),
              conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(), NUM_CHROMA_FORMAT, isTff );
#endif
          }
        }

        // update POC of display order
        m_aiPOCLastDisplay[layerId] = pcPicBottom->getPOC();
#else
        if ( m_pchReconFile )
        {
          const Window &conf = pcPicTop->getConformanceWindow();
          const Window &defDisp = m_respectDefDispWindow ? pcPicTop->getDefDisplayWindow() : Window();
          const Bool isTff = pcPicTop->isTopField();

          Bool display = true;
          if( m_decodedNoDisplaySEIEnabled )
          {
            SEIMessages noDisplay = getSeisByType(pcPic->getSEIs(), SEI::NO_DISPLAY );
            const SEINoDisplay *nd = ( noDisplay.size() > 0 ) ? (SEINoDisplay*) *(noDisplay.begin()) : NULL;
            if( (nd != NULL) && nd->m_noDisplay )
            {
              display = false;
            }
          }

          if (display)
          {
            m_cTVideoIOYuvReconFile.write( pcPicTop->getPicYuvRec(), pcPicBottom->getPicYuvRec(),
                                           m_outputColourSpaceConvert,
                                           conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                           conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                           conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
                                           conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(), NUM_CHROMA_FORMAT, isTff );
          }
        }

        // update POC of display order
        m_iPOCLastDisplay = pcPicBottom->getPOC();
#endif

        // erase non-referenced picture in the reference picture list after display
        if ( !pcPicTop->getSlice(0)->isReferenced() && pcPicTop->getReconMark() == true )
        {
#if !DYN_REF_FREE
          pcPicTop->setReconMark(false);

          // mark it should be extended later
          pcPicTop->getPicYuvRec()->setBorderExtension( false );

#else
          pcPicTop->destroy();
          pcListPic->erase( iterPic );
          iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
          continue;
#endif
        }
        if ( !pcPicBottom->getSlice(0)->isReferenced() && pcPicBottom->getReconMark() == true )
        {
#if !DYN_REF_FREE
          pcPicBottom->setReconMark(false);

          // mark it should be extended later
          pcPicBottom->getPicYuvRec()->setBorderExtension( false );

#else
          pcPicBottom->destroy();
          pcListPic->erase( iterPic );
          iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
          continue;
#endif
        }
        pcPicTop->setOutputMark(false);
        pcPicBottom->setOutputMark(false);
      }
    }
  }
  else if (!pcPic->isField()) //Frame Decoding
  {
    iterPic = pcListPic->begin();

    while (iterPic != pcListPic->end())
    {
      pcPic = *(iterPic);

#if SVC_EXTENSION
      if( pcPic->getOutputMark() && pcPic->getPOC() > m_aiPOCLastDisplay[layerId] &&
        (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid) )
#else
      if(pcPic->getOutputMark() && pcPic->getPOC() > m_iPOCLastDisplay &&
        (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid))
#endif
      {
        // write to file
         numPicsNotYetDisplayed--;
        if(pcPic->getSlice(0)->isReferenced() == false)
        {
          dpbFullness--;
        }
#if SVC_EXTENSION
        if( m_pchReconFile[layerId] )
        {
          const Window &conf = pcPic->getConformanceWindow();
          const Window &defDisp = m_respectDefDispWindow ? pcPic->getDefDisplayWindow() : Window();          
#if REPN_FORMAT_IN_VPS
          UInt chromaFormatIdc = pcPic->getSlice(0)->getChromaFormatIdc();
          Int xScal =  TComSPS::getWinUnitX( chromaFormatIdc ), yScal = TComSPS::getWinUnitY( chromaFormatIdc );

          m_acTVideoIOYuvReconFile[layerId].write( pcPic->getPicYuvRec(), m_outputColourSpaceConvert,
            conf.getWindowLeftOffset()  * xScal + defDisp.getWindowLeftOffset(),
            conf.getWindowRightOffset() * xScal + defDisp.getWindowRightOffset(),
            conf.getWindowTopOffset()   * yScal + defDisp.getWindowTopOffset(),
            conf.getWindowBottomOffset()* yScal + defDisp.getWindowBottomOffset() );
#else
          m_acTVideoIOYuvReconFile[layerId].write( pcPic->getPicYuvRec(), m_outputColourSpaceConvert,
            conf.getWindowLeftOffset()   + defDisp.getWindowLeftOffset(),
            conf.getWindowRightOffset()  + defDisp.getWindowRightOffset(),
            conf.getWindowTopOffset()    + defDisp.getWindowTopOffset(),
            conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset() );
#endif
        }

        // update POC of display order
        m_aiPOCLastDisplay[layerId] = pcPic->getPOC();
#else
        if ( m_pchReconFile )
        {
          const Window &conf    = pcPic->getConformanceWindow();
          const Window &defDisp = m_respectDefDispWindow ? pcPic->getDefDisplayWindow() : Window();

          m_cTVideoIOYuvReconFile.write( pcPic->getPicYuvRec(),
                                         m_outputColourSpaceConvert,
                                         conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                         conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                         conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
                                         conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset() );
        }

#if Q0074_COLOUR_REMAPPING_SEI
        if (m_colourRemapSEIEnabled)
        {
          SEIMessages colourRemappingInfo = getSeisByType(pcPic->getSEIs(), SEI::COLOUR_REMAPPING_INFO );
          const SEIColourRemappingInfo *seiColourRemappingInfo = ( colourRemappingInfo.size() > 0 ) ? (SEIColourRemappingInfo*) *(colourRemappingInfo.begin()) : NULL;
          if (colourRemappingInfo.size() > 1)
          {
            printf ("Warning: Got multiple Colour Remapping Information SEI messages. Using first.");
          }
          if (seiColourRemappingInfo)
          {
            applyColourRemapping(*pcPic->getPicYuvRec(), seiColourRemappingInfo);
          }
          else  // using the last CRI SEI received
          {
            const SEIColourRemappingInfo *seiColourRemappingInfoCopy;
            seiColourRemappingInfoCopy = seiColourRemappingInfoPrevious;
            applyColourRemapping(*pcPic->getPicYuvRec(), seiColourRemappingInfoCopy);
          }

          // save the last CRI SEI received
          if( seiColourRemappingInfo != NULL){
            if (seiColourRemappingInfoPrevious != NULL)
            {
              delete seiColourRemappingInfoPrevious;
              seiColourRemappingInfoPrevious = NULL;
            }
            if (seiColourRemappingInfo->m_colourRemapPersistenceFlag)
            {
              seiColourRemappingInfoPrevious = new SEIColourRemappingInfo();
              seiColourRemappingInfoPrevious->copyFrom(seiColourRemappingInfo);
            }
          }
        }
#endif

        // update POC of display order
        m_iPOCLastDisplay = pcPic->getPOC();
#endif

        // erase non-referenced picture in the reference picture list after display
        if ( !pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true )
        {
#if !DYN_REF_FREE
          pcPic->setReconMark(false);

          // mark it should be extended later
          pcPic->getPicYuvRec()->setBorderExtension( false );

#else
          pcPic->destroy();
          pcListPic->erase( iterPic );
          iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
          continue;
#endif
        }
        pcPic->setOutputMark(false);
      }

      iterPic++;
    }
  }
}

/** \param pcListPic list of pictures to be written to file
    \todo            DYN_REF_FREE should be revised
 */
#if SVC_EXTENSION
Void TAppDecTop::xFlushOutput( TComList<TComPic*>* pcListPic, UInt layerId )
#else
Void TAppDecTop::xFlushOutput( TComList<TComPic*>* pcListPic )
#endif
{
  if(!pcListPic || pcListPic->empty())
  {
    return;
  }
  TComList<TComPic*>::iterator iterPic   = pcListPic->begin();

  iterPic   = pcListPic->begin();
  TComPic* pcPic = *(iterPic);

  if (pcPic->isField()) //Field Decoding
  {
    TComList<TComPic*>::iterator endPic   = pcListPic->end();
    endPic--;
    TComPic *pcPicTop, *pcPicBottom = NULL;
    while (iterPic != endPic)
    {
      pcPicTop = *(iterPic);
      iterPic++;
      pcPicBottom = *(iterPic);

      if ( pcPicTop->getOutputMark() && pcPicBottom->getOutputMark() && !(pcPicTop->getPOC()%2) && (pcPicBottom->getPOC() == pcPicTop->getPOC()+1) )
      {
        // write to file
#if SVC_EXTENSION
        if ( m_pchReconFile[layerId] )
        {
          const Window &conf = pcPicTop->getConformanceWindow();
          const Window &defDisp = m_respectDefDispWindow ? pcPicTop->getDefDisplayWindow() : Window();
          const Bool isTff = pcPicTop->isTopField();          
#if REPN_FORMAT_IN_VPS
          UInt chromaFormatIdc = pcPic->getSlice(0)->getChromaFormatIdc();
          Int xScal =  TComSPS::getWinUnitX( chromaFormatIdc ), yScal = TComSPS::getWinUnitY( chromaFormatIdc );

          m_acTVideoIOYuvReconFile[layerId].write( pcPicTop->getPicYuvRec(), pcPicBottom->getPicYuvRec(), m_outputColourSpaceConvert,
            conf.getWindowLeftOffset()  *xScal + defDisp.getWindowLeftOffset(),
            conf.getWindowRightOffset() *xScal + defDisp.getWindowRightOffset(),
            conf.getWindowTopOffset()   *yScal + defDisp.getWindowTopOffset(),
            conf.getWindowBottomOffset()*yScal + defDisp.getWindowBottomOffset(), NUM_CHROMA_FORMAT, isTff );
#else
          m_acTVideoIOYuvReconFile[layerId].write( pcPicTop->getPicYuvRec(), pcPicBottom->getPicYuvRec(), m_outputColourSpaceConvert,
            conf.getWindowLeftOffset()   + defDisp.getWindowLeftOffset(),
            conf.getWindowRightOffset()  + defDisp.getWindowRightOffset(),
            conf.getWindowTopOffset()    + defDisp.getWindowTopOffset(),
            conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(), NUM_CHROMA_FORMAT, isTff );
#endif
        }

        // update POC of display order
        m_aiPOCLastDisplay[layerId] = pcPicBottom->getPOC();
#else
        if ( m_pchReconFile )
        {
          const Window &conf = pcPicTop->getConformanceWindow();
          const Window &defDisp = m_respectDefDispWindow ? pcPicTop->getDefDisplayWindow() : Window();
          const Bool isTff = pcPicTop->isTopField();
          m_cTVideoIOYuvReconFile.write( pcPicTop->getPicYuvRec(), pcPicBottom->getPicYuvRec(),
                                         m_outputColourSpaceConvert,
                                         conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                         conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                         conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
                                         conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset(), NUM_CHROMA_FORMAT, isTff );
        }

        // update POC of display order
        m_iPOCLastDisplay = pcPicBottom->getPOC();
#endif

        // erase non-referenced picture in the reference picture list after display
        if ( !pcPicTop->getSlice(0)->isReferenced() && pcPicTop->getReconMark() == true )
        {
#if !DYN_REF_FREE
          pcPicTop->setReconMark(false);

          // mark it should be extended later
          pcPicTop->getPicYuvRec()->setBorderExtension( false );

#else
          pcPicTop->destroy();
          pcListPic->erase( iterPic );
          iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
          continue;
#endif
        }
        if ( !pcPicBottom->getSlice(0)->isReferenced() && pcPicBottom->getReconMark() == true )
        {
#if !DYN_REF_FREE
          pcPicBottom->setReconMark(false);

          // mark it should be extended later
          pcPicBottom->getPicYuvRec()->setBorderExtension( false );

#else
          pcPicBottom->destroy();
          pcListPic->erase( iterPic );
          iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
          continue;
#endif
        }
        pcPicTop->setOutputMark(false);
        pcPicBottom->setOutputMark(false);

#if !DYN_REF_FREE
        if(pcPicTop)
        {
          pcPicTop->destroy();
          delete pcPicTop;
          pcPicTop = NULL;
        }
#endif
      }
    }
    if(pcPicBottom)
    {
      pcPicBottom->destroy();
      delete pcPicBottom;
      pcPicBottom = NULL;
    }
  }
  else //Frame decoding
  {
    while (iterPic != pcListPic->end())
    {
      pcPic = *(iterPic);

      if ( pcPic->getOutputMark() )
      {
        // write to file
#if SVC_EXTENSION
        if ( m_pchReconFile[layerId] )
        {
          const Window &conf = pcPic->getConformanceWindow();
          const Window &defDisp = m_respectDefDispWindow ? pcPic->getDefDisplayWindow() : Window();          
#if REPN_FORMAT_IN_VPS
          UInt chromaFormatIdc = pcPic->getSlice(0)->getChromaFormatIdc();
          Int xScal =  TComSPS::getWinUnitX( chromaFormatIdc ), yScal = TComSPS::getWinUnitY( chromaFormatIdc );

          m_acTVideoIOYuvReconFile[layerId].write( pcPic->getPicYuvRec(), m_outputColourSpaceConvert,
            conf.getWindowLeftOffset()  *xScal + defDisp.getWindowLeftOffset(),
            conf.getWindowRightOffset() *xScal + defDisp.getWindowRightOffset(),
            conf.getWindowTopOffset()   *yScal + defDisp.getWindowTopOffset(),
            conf.getWindowBottomOffset()*yScal + defDisp.getWindowBottomOffset() );
#else
          m_acTVideoIOYuvReconFile[layerId].write( pcPic->getPicYuvRec(), m_outputColourSpaceConvert,
            conf.getWindowLeftOffset()   + defDisp.getWindowLeftOffset(),
            conf.getWindowRightOffset()  + defDisp.getWindowRightOffset(),
            conf.getWindowTopOffset()    + defDisp.getWindowTopOffset(),
            conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset() );
#endif
        }

        // update POC of display order
        m_aiPOCLastDisplay[layerId] = pcPic->getPOC();
#else
        if ( m_pchReconFile )
        {
          const Window &conf    = pcPic->getConformanceWindow();
          const Window &defDisp = m_respectDefDispWindow ? pcPic->getDefDisplayWindow() : Window();

          m_cTVideoIOYuvReconFile.write( pcPic->getPicYuvRec(),
                                         m_outputColourSpaceConvert,
                                         conf.getWindowLeftOffset() + defDisp.getWindowLeftOffset(),
                                         conf.getWindowRightOffset() + defDisp.getWindowRightOffset(),
                                         conf.getWindowTopOffset() + defDisp.getWindowTopOffset(),
                                         conf.getWindowBottomOffset() + defDisp.getWindowBottomOffset() );
        }

#if Q0074_COLOUR_REMAPPING_SEI
        if (m_colourRemapSEIEnabled)
        {
          SEIMessages colourRemappingInfo = getSeisByType(pcPic->getSEIs(), SEI::COLOUR_REMAPPING_INFO );
          const SEIColourRemappingInfo *seiColourRemappingInfo = ( colourRemappingInfo.size() > 0 ) ? (SEIColourRemappingInfo*) *(colourRemappingInfo.begin()) : NULL;
          if (colourRemappingInfo.size() > 1)
          {
            printf ("Warning: Got multiple Colour Remapping Information SEI messages. Using first.");
          }
          if (seiColourRemappingInfo)
          {
            applyColourRemapping(*pcPic->getPicYuvRec(), seiColourRemappingInfo);
          }
          else  // using the last CRI SEI received
          {
            const SEIColourRemappingInfo *seiColourRemappingInfoCopy;
            seiColourRemappingInfoCopy = seiColourRemappingInfoPrevious;
            applyColourRemapping(*pcPic->getPicYuvRec(), seiColourRemappingInfoCopy);
          }

          // save the last CRI SEI received
          if( seiColourRemappingInfo != NULL){
            if (seiColourRemappingInfoPrevious != NULL)
            {
              delete seiColourRemappingInfoPrevious;
              seiColourRemappingInfoPrevious = NULL;
            }
            if (seiColourRemappingInfo->m_colourRemapPersistenceFlag)
            {
              seiColourRemappingInfoPrevious = new SEIColourRemappingInfo();
              seiColourRemappingInfoPrevious->copyFrom(seiColourRemappingInfo);
            }
          }
        }
#endif

        // update POC of display order
        m_iPOCLastDisplay = pcPic->getPOC();
#endif

        // erase non-referenced picture in the reference picture list after display
        if ( !pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true )
        {
#if !DYN_REF_FREE
          pcPic->setReconMark(false);

          // mark it should be extended later
          pcPic->getPicYuvRec()->setBorderExtension( false );

#else
          pcPic->destroy();
          pcListPic->erase( iterPic );
          iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
          continue;
#endif
        }
        pcPic->setOutputMark(false);
      }
#if !SVC_EXTENSION
#if !DYN_REF_FREE
      if(pcPic)
      {
        pcPic->destroy();
        delete pcPic;
        pcPic = NULL;
      }
#endif
#endif
      iterPic++;
    }
  }
#if SVC_EXTENSION
  m_aiPOCLastDisplay[layerId] = -MAX_INT;
#else
  pcListPic->clear();
  m_iPOCLastDisplay = -MAX_INT;
#endif
}

/** \param nalu Input nalu to check whether its LayerId is within targetDecLayerIdSet
 */
Bool TAppDecTop::isNaluWithinTargetDecLayerIdSet( InputNALUnit* nalu )
{
  if ( m_targetDecLayerIdSet.size() == 0 ) // By default, the set is empty, meaning all LayerIds are allowed
  {
    return true;
  }
#if SVC_EXTENSION
  if (nalu->m_layerId == 0 && (nalu->m_nalUnitType == NAL_UNIT_VPS || nalu->m_nalUnitType == NAL_UNIT_SPS || nalu->m_nalUnitType == NAL_UNIT_PPS || nalu->m_nalUnitType == NAL_UNIT_EOS))
  {
    return true;
  }
#endif
  for (std::vector<Int>::iterator it = m_targetDecLayerIdSet.begin(); it != m_targetDecLayerIdSet.end(); it++)
  {
    if ( nalu->m_reservedZero6Bits == (*it) )
    {
      return true;
    }
  }
  return false;
}

#if ALIGNED_BUMPING
// Function outputs a picture, and marks it as not needed for output.
Void TAppDecTop::xOutputAndMarkPic( TComPic *pic, const Char *reconFile, const Int layerId, Int &pocLastDisplay, DpbStatus &dpbStatus )
{
  if ( reconFile )
  {
    const Window &conf = pic->getConformanceWindow();
    const Window &defDisp = m_respectDefDispWindow ? pic->getDefDisplayWindow() : Window();
    Int xScal =  1, yScal = 1;
#if REPN_FORMAT_IN_VPS
    UInt chromaFormatIdc = pic->getSlice(0)->getChromaFormatIdc();
    xScal = TComSPS::getWinUnitX( chromaFormatIdc );
    yScal = TComSPS::getWinUnitY( chromaFormatIdc );
#endif
    TComPicYuv* pPicCYuvRec = pic->getPicYuvRec();
    m_acTVideoIOYuvReconFile[layerId].write( pPicCYuvRec, m_outputColourSpaceConvert,
      conf.getWindowLeftOffset()  * xScal + defDisp.getWindowLeftOffset(),
      conf.getWindowRightOffset() * xScal + defDisp.getWindowRightOffset(),
      conf.getWindowTopOffset()   * yScal + defDisp.getWindowTopOffset(),
      conf.getWindowBottomOffset()* yScal + defDisp.getWindowBottomOffset() );

#if Q0074_COLOUR_REMAPPING_SEI
    if (m_colourRemapSEIEnabled)
    {
      SEIMessages colourRemappingInfo = getSeisByType(pic->getSEIs(), SEI::COLOUR_REMAPPING_INFO );
      const SEIColourRemappingInfo *seiColourRemappingInfo = ( colourRemappingInfo.size() > 0 ) ? (SEIColourRemappingInfo*) *(colourRemappingInfo.begin()) : NULL;
      if (colourRemappingInfo.size() > 1)
      {
        printf ("Warning: Got multiple Colour Remapping Information SEI messages. Using first.");
      }

      if (seiColourRemappingInfo)
      {
        //printf ("\n\nColour Remapping is applied to POC : %d and LayerId : %d ",pic->getPOC(), pic->getLayerId());
        applyColourRemapping(*pic->getPicYuvRec(), seiColourRemappingInfo
#if SVC_EXTENSION
       , pic->getLayerId()
#endif
        );
      }
      else  // using the last CRI SEI received
      {
        const SEIColourRemappingInfo *seiColourRemappingInfoCopy;
        seiColourRemappingInfoCopy = seiColourRemappingInfoPrevious;
        applyColourRemapping(*pic->getPicYuvRec(), seiColourRemappingInfoCopy
#if SVC_EXTENSION
        , pic->getLayerId()
#endif
        );
      }

      // save the last CRI SEI received
      if( seiColourRemappingInfo != NULL){
        if (seiColourRemappingInfoPrevious != NULL)
        {
          delete seiColourRemappingInfoPrevious;
          seiColourRemappingInfoPrevious = NULL;
        }
        if (seiColourRemappingInfo->m_colourRemapPersistenceFlag)
        {
          seiColourRemappingInfoPrevious = new SEIColourRemappingInfo();
          seiColourRemappingInfoPrevious->copyFrom(seiColourRemappingInfo);
        }
      }
    }

#endif
  }
  // update POC of display order
  pocLastDisplay = pic->getPOC();

  // Mark as not needed for output
  pic->setOutputMark(false);

  // "erase" non-referenced picture in the reference picture list after display
  if ( !pic->getSlice(0)->isReferenced() && pic->getReconMark() == true )
  {
    pic->setReconMark(false);

    // mark it should be extended later
    pic->getPicYuvRec()->setBorderExtension( false );

#if FIX_ALIGN_BUMPING
    dpbStatus.m_numPicsInSubDpb[dpbStatus.m_layerIdToSubDpbIdMap[layerId]]--;
#else
    dpbStatus.m_numPicsInSubDpb[layerIdx]--;
#endif
  }
}

Void TAppDecTop::flushAllPictures(Int layerId, Bool outputPictures)
{
  // First "empty" all pictures that are not used for reference and not needed for output
  emptyUnusedPicturesNotNeededForOutput();

  if( outputPictures )  // All pictures in the DPB in that layer are to be output; this means other pictures would also be output
  {
    std::vector<Int>  listOfPocs;
#if FIX_ALIGN_BUMPING
    std::vector<Int>  listOfPocsInEachLayer[MAX_VPS_LAYER_IDX_PLUS1];
    std::vector<Int>  listOfPocsPositionInEachLayer[MAX_VPS_LAYER_IDX_PLUS1];
#else
    std::vector<Int>  listOfPocsInEachLayer[MAX_LAYERS];
    std::vector<Int>  listOfPocsPositionInEachLayer[MAX_LAYERS];
#endif
    DpbStatus dpbStatus;

    // Find the status of the DPB
    xFindDPBStatus(listOfPocs, listOfPocsInEachLayer, listOfPocsPositionInEachLayer, dpbStatus);

    if( listOfPocs.size() )
    {
      while( listOfPocsInEachLayer[layerId].size() )    // As long as there picture in the layer to be output
      {
        bumpingProcess( listOfPocs, listOfPocsInEachLayer, listOfPocsPositionInEachLayer, dpbStatus );
      }
    }
  }

  // Now remove all pictures from the layer DPB?
  markAllPicturesAsErased(layerId);
}
Void TAppDecTop::flushAllPictures(Bool outputPictures)
{
  // First "empty" all pictures that are not used for reference and not needed for output
  emptyUnusedPicturesNotNeededForOutput();

  if( outputPictures )  // All pictures in the DPB are to be output
  {
    std::vector<Int>  listOfPocs;
#if FIX_ALIGN_BUMPING
    std::vector<Int>  listOfPocsInEachLayer[MAX_VPS_LAYER_IDX_PLUS1];
    std::vector<Int>  listOfPocsPositionInEachLayer[MAX_VPS_LAYER_IDX_PLUS1];
#else
    std::vector<Int>  listOfPocsInEachLayer[MAX_LAYERS];
    std::vector<Int>  listOfPocsPositionInEachLayer[MAX_LAYERS];
#endif
    DpbStatus dpbStatus;

    // Find the status of the DPB
#if POC_RESET_IDC_DECODER
    xFindDPBStatus(listOfPocs, listOfPocsInEachLayer, listOfPocsPositionInEachLayer, dpbStatus, false);
#else
    xFindDPBStatus(listOfPocs, listOfPocsInEachLayer, listOfPocsPositionInEachLayer, dpbStatus);
#endif

    while( dpbStatus.m_numAUsNotDisplayed )
    {
      bumpingProcess( listOfPocs, listOfPocsInEachLayer, listOfPocsPositionInEachLayer, dpbStatus );
    }
  }

  // Now remove all pictures from the DPB?
  markAllPicturesAsErased();
}

Void TAppDecTop::markAllPicturesAsErased()
{
#if FIX_ALIGN_BUMPING
  for(Int i = 0; i < MAX_VPS_LAYER_IDX_PLUS1; i++)
#else
  for(Int i = 0; i < MAX_LAYERS; i++)
#endif
  {
    markAllPicturesAsErased(i);
  }
}

Void TAppDecTop::markAllPicturesAsErased(Int layerIdx)
{
  TComList<TComPic*>::iterator  iterPic = m_acTDecTop[layerIdx].getListPic()->begin();
  Int iSize = Int( m_acTDecTop[layerIdx].getListPic()->size() );
  
  for (Int i = 0; i < iSize; i++ )
  {
    TComPic* pcPic = *(iterPic++);

    if( pcPic )
    {
      pcPic->destroy();

      // pcPic is statically created for the external (AVC) base layer, no need to delete it
      if( !m_acTDecTop[layerIdx].getParameterSetManager()->getActiveVPS()->getNonHEVCBaseLayerFlag() || layerIdx )
      {
        delete pcPic;
        pcPic = NULL;
      }
    }
  }

  m_acTDecTop[layerIdx].getListPic()->clear();
}

Void TAppDecTop::checkOutputBeforeDecoding(Int layerIdx)
{
    
  std::vector<Int>  listOfPocs;
#if FIX_ALIGN_BUMPING
  std::vector<Int>  listOfPocsInEachLayer[MAX_VPS_LAYER_IDX_PLUS1];
  std::vector<Int>  listOfPocsPositionInEachLayer[MAX_VPS_LAYER_IDX_PLUS1];
#else
  std::vector<Int>  listOfPocsInEachLayer[MAX_LAYERS];
  std::vector<Int>  listOfPocsPositionInEachLayer[MAX_LAYERS];
#endif
  DpbStatus dpbStatus;

  // First "empty" all pictures that are not used for reference and not needed for output
  emptyUnusedPicturesNotNeededForOutput();

  // Find the status of the DPB
  xFindDPBStatus(listOfPocs, listOfPocsInEachLayer, listOfPocsPositionInEachLayer, dpbStatus);

  // If not picture to be output, return
  if( listOfPocs.size() == 0 )
  {
    return;
  }

  // Find DPB-information from the VPS
  DpbStatus maxDpbLimit;
#if FIX_ALIGN_BUMPING
  Int subDpbIdx = getCommonDecoderParams()->getTargetOutputLayerSetIdx() == 0 
                  ? dpbStatus.m_layerIdToSubDpbIdMap[0] 
                  : dpbStatus.m_layerIdToSubDpbIdMap[layerIdx];
#else
  Int subDpbIdx = getCommonDecoderParams()->getTargetOutputLayerSetIdx() == 0 ? 0 : layerIdx;
#endif
  findDpbParametersFromVps(listOfPocs, listOfPocsInEachLayer, listOfPocsPositionInEachLayer, maxDpbLimit);
  // Assume that listOfPocs is sorted in increasing order - if not have to sort it.
  while( ifInvokeBumpingBeforeDecoding(dpbStatus, maxDpbLimit, layerIdx, subDpbIdx) )
  {
    bumpingProcess( listOfPocs, listOfPocsInEachLayer, listOfPocsPositionInEachLayer, dpbStatus );
  }  
}

Void TAppDecTop::checkOutputAfterDecoding()
{    
  std::vector<Int>  listOfPocs;
#if FIX_ALIGN_BUMPING
  std::vector<Int>  listOfPocsInEachLayer[MAX_VPS_LAYER_IDX_PLUS1];
  std::vector<Int>  listOfPocsPositionInEachLayer[MAX_VPS_LAYER_IDX_PLUS1];
#else
  std::vector<Int>  listOfPocsInEachLayer[MAX_LAYERS];
  std::vector<Int>  listOfPocsPositionInEachLayer[MAX_LAYERS];
#endif
  DpbStatus dpbStatus;

  // First "empty" all pictures that are not used for reference and not needed for output
  emptyUnusedPicturesNotNeededForOutput();

  // Find the status of the DPB
  xFindDPBStatus(listOfPocs, listOfPocsInEachLayer, listOfPocsPositionInEachLayer, dpbStatus);

  // If not picture to be output, return
  if( listOfPocs.size() == 0 )
  {
    return;
  }

  // Find DPB-information from the VPS
  DpbStatus maxDpbLimit;
  findDpbParametersFromVps(listOfPocs, listOfPocsInEachLayer, listOfPocsPositionInEachLayer, maxDpbLimit);

  // Assume that listOfPocs is sorted in increasing order - if not have to sort it.
  while( ifInvokeBumpingAfterDecoding(dpbStatus, maxDpbLimit) )
  {
    bumpingProcess( listOfPocs, listOfPocsInEachLayer, listOfPocsPositionInEachLayer, dpbStatus );
  }  
}

Void TAppDecTop::bumpingProcess(std::vector<Int> &listOfPocs, std::vector<Int> *listOfPocsInEachLayer, std::vector<Int> *listOfPocsPositionInEachLayer, DpbStatus &dpbStatus)
{
  // Choose the smallest POC value
  Int pocValue = *(listOfPocs.begin());
  std::vector<int>::iterator it;
  TComList<TComPic*>::iterator iterPic;
#if FIX_ALIGN_BUMPING
  for( Int dpbLayerCtr = 0; dpbLayerCtr < dpbStatus.m_numLayers; dpbLayerCtr++)
  {
    Int layerId  = dpbStatus.m_targetDecLayerIdList[dpbLayerCtr];
#else
  for( Int layerIdx = 0; layerIdx < dpbStatus.m_numLayers; layerIdx++)
  {
#endif
    // Check if picture with pocValue is present.
    it = find( listOfPocsInEachLayer[layerId].begin(), listOfPocsInEachLayer[layerId].end(), pocValue );
    if( it != listOfPocsInEachLayer[layerId].end() )  // picture found.
    {
      Int picPosition = (Int)std::distance( listOfPocsInEachLayer[layerId].begin(), it );
      Int j;
      for(j = 0, iterPic = m_acTDecTop[layerId].getListPic()->begin(); j < listOfPocsPositionInEachLayer[layerId][picPosition]; j++) // Picture to be output
      {
        iterPic++;
      }
      TComPic *pic = *iterPic;

      xOutputAndMarkPic( pic, m_pchReconFile[layerId], layerId, m_aiPOCLastDisplay[layerId], dpbStatus );

#if CONFORMANCE_BITSTREAM_MODE
      FILE *fptr;
      if( m_confModeFlag )
      {
        if( m_metadataFileRefresh )
        {
          fptr = fopen( this->getMetadataFileName().c_str(), "w" );
          fprintf(fptr, " LayerId      POC    MD5\n");
          fprintf(fptr, "------------------------\n");
        }
        else
        {
          fptr = fopen( this->getMetadataFileName().c_str(), "a+" );
        }
        this->setMetadataFileRefresh(false);

        TComDigest recon_digest;
        Int numChar = calcMD5(*pic->getPicYuvRec(), recon_digest);
        fprintf(fptr, "%8d%9d    MD5:%s\n", pic->getLayerId(), pic->getSlice(0)->getPOC(), digestToString(recon_digest, numChar).c_str());
        fclose(fptr);
      }
#endif

      listOfPocsInEachLayer[layerId].erase( it );
      listOfPocsPositionInEachLayer[layerId].erase( listOfPocsPositionInEachLayer[layerId].begin() + picPosition );
#if FIX_ALIGN_BUMPING
      dpbStatus.m_numPicsInSubDpb[dpbStatus.m_layerIdToSubDpbIdMap[layerId]]--;
#endif
    }
  }
#if !FIX_ALIGN_BUMPING
  // Update sub-DPB status
  for( Int subDpbIdx = 0; subDpbIdx < dpbStatus.m_numSubDpbs; subDpbIdx++)
  {
    dpbStatus.m_numPicsInSubDpb[subDpbIdx]--;
  }
#endif
  dpbStatus.m_numAUsNotDisplayed--;

#if CONFORMANCE_BITSTREAM_MODE
  if( m_confModeFlag )
  {
    for( Int dpbLayerCtr = 0; dpbLayerCtr < dpbStatus.m_numLayers; dpbLayerCtr++)
    {
      Int layerId = dpbStatus.m_targetDecLayerIdList[dpbLayerCtr];
      // Output all picutres "decoded" in that layer that have POC less than the current picture
      std::vector<TComPic> *layerBuffer = (m_acTDecTop->getLayerDec(layerId))->getConfListPic();
      // Write all pictures to the file.
      if( this->getDecodedYuvLayerRefresh(layerId) )
      {
        m_outputBitDepth[CHANNEL_TYPE_LUMA]   = g_bitDepth[CHANNEL_TYPE_LUMA]   = g_bitDepthLayer[CHANNEL_TYPE_LUMA][layerId];
        m_outputBitDepth[CHANNEL_TYPE_CHROMA] = g_bitDepth[CHANNEL_TYPE_CHROMA] = g_bitDepthLayer[CHANNEL_TYPE_CHROMA][layerId];

        char tempFileName[256];
        strcpy(tempFileName, this->getDecodedYuvLayerFileName( layerId ).c_str());
        m_confReconFile[layerId].open(tempFileName, true, m_outputBitDepth, m_outputBitDepth, g_bitDepth ); // write mode
        this->setDecodedYuvLayerRefresh( layerId, false );
      }

      std::vector<TComPic>::iterator itPic;
      for(itPic = layerBuffer->begin(); itPic != layerBuffer->end(); itPic++)
      {
        TComPic checkPic = *itPic;
        const Window &conf = checkPic.getConformanceWindow();
        const Window &defDisp = m_respectDefDispWindow ? checkPic.getDefDisplayWindow() : Window();
        Int xScal = 1, yScal = 1;
  #if REPN_FORMAT_IN_VPS
        UInt chromaFormatIdc = checkPic.getSlice(0)->getChromaFormatIdc();
        xScal = TComSPS::getWinUnitX( chromaFormatIdc );
        yScal = TComSPS::getWinUnitY( chromaFormatIdc );
  #endif
        if( checkPic.getPOC() <= pocValue )
        {
          TComPicYuv* pPicCYuvRec = checkPic.getPicYuvRec();
          m_confReconFile[layerId].write( pPicCYuvRec, m_outputColourSpaceConvert,
            conf.getWindowLeftOffset()  * xScal + defDisp.getWindowLeftOffset(),
            conf.getWindowRightOffset() * xScal + defDisp.getWindowRightOffset(),
            conf.getWindowTopOffset()   * yScal + defDisp.getWindowTopOffset(),
            conf.getWindowBottomOffset()* yScal + defDisp.getWindowBottomOffset() );
          layerBuffer->erase(itPic);
          itPic = layerBuffer->begin();  // Ensure doesn't go to infinite loop
          if(layerBuffer->size() == 0)
          {
            break;
          }
        }
      }
    }
  }
#endif

  // Remove the picture from the listOfPocs
  listOfPocs.erase( listOfPocs.begin() );
}

TComVPS *TAppDecTop::findDpbParametersFromVps(std::vector<Int> const &listOfPocs, std::vector<Int> const *listOfPocsInEachLayer, std::vector<Int> const *listOfPocsPositionInEachLayer, DpbStatus &maxDpbLimit)
{
  Int targetOutputLsIdx = getCommonDecoderParams()->getTargetOutputLayerSetIdx();
  TComVPS *vps = NULL;

  if( targetOutputLsIdx == 0 )   // Only base layer is output
  {
    TComSPS *sps = NULL;
    assert( listOfPocsInEachLayer[0].size() != 0 );
    TComList<TComPic*>::iterator iterPic;
    Int j;
    for(j = 0, iterPic = m_acTDecTop[0].getListPic()->begin(); j < listOfPocsPositionInEachLayer[0][0]; j++) // Picture to be output
    {
      iterPic++;
    }
    TComPic *pic = *iterPic;
    sps = pic->getSlice(0)->getSPS();   assert( sps->getLayerId() == 0 );
    vps = pic->getSlice(0)->getVPS();
    Int highestTId = sps->getMaxTLayers() - 1;

    maxDpbLimit.m_numAUsNotDisplayed = sps->getNumReorderPics( highestTId ); // m_numAUsNotDisplayed is only variable name - stores reorderpics
    maxDpbLimit.m_maxLatencyIncrease = sps->getMaxLatencyIncrease( highestTId ) > 0;
    if( maxDpbLimit.m_maxLatencyIncrease )
    {
      maxDpbLimit.m_maxLatencyPictures = sps->getMaxLatencyIncrease( highestTId ) + sps->getNumReorderPics( highestTId ) - 1;
    }
    maxDpbLimit.m_numPicsInSubDpb[0] = sps->getMaxDecPicBuffering( highestTId );
  }
  else
  {
    // -------------------------------------
    // Find the VPS used for the pictures
    // -------------------------------------
#if FIX_ALIGN_BUMPING
    for(Int i = 0; i < MAX_VPS_LAYER_IDX_PLUS1; i++)
#else
    for(Int i = 0; i < MAX_LAYERS; i++)
#endif
    {
      if( m_acTDecTop[i].getListPic()->empty() )
      {
        assert( listOfPocsInEachLayer[i].size() == 0 );
        continue;
      }
      std::vector<Int>::const_iterator it;
      it = find( listOfPocsInEachLayer[i].begin(), listOfPocsInEachLayer[i].end(), listOfPocs[0] );
      TComList<TComPic*>::iterator iterPic;
      if( it != listOfPocsInEachLayer[i].end() )
      {
        Int picPosition = (Int)std::distance( listOfPocsInEachLayer[i].begin(), it );
        Int j;
        for(j = 0, iterPic = m_acTDecTop[i].getListPic()->begin(); j < listOfPocsPositionInEachLayer[i][picPosition]; j++) // Picture to be output
        {
          iterPic++;
        }
        TComPic *pic = *iterPic;
        vps = pic->getSlice(0)->getVPS();
        break;
      }
    }

    Int targetLsIdx       = vps->getOutputLayerSetIdx( getCommonDecoderParams()->getTargetOutputLayerSetIdx() );
    Int highestTId = vps->getMaxTLayers() - 1;

    maxDpbLimit.m_numAUsNotDisplayed = vps->getMaxVpsNumReorderPics( targetOutputLsIdx, highestTId ); // m_numAUsNotDisplayed is only variable name - stores reorderpics
    maxDpbLimit.m_maxLatencyIncrease  = vps->getMaxVpsLatencyIncreasePlus1(targetOutputLsIdx, highestTId ) > 0;
    if( maxDpbLimit.m_maxLatencyIncrease )
    {
      maxDpbLimit.m_maxLatencyPictures = vps->getMaxVpsNumReorderPics( targetOutputLsIdx, highestTId ) + vps->getMaxVpsLatencyIncreasePlus1(targetOutputLsIdx, highestTId ) - 1;
    }
    for(Int i = 0; i < vps->getNumLayersInIdList( targetLsIdx ); i++)
    {
#if RESOUTION_BASED_DPB
      maxDpbLimit.m_numPicsInLayer[i] = vps->getMaxVpsLayerDecPicBuffMinus1( targetOutputLsIdx, i, highestTId ) + 1;
      maxDpbLimit.m_numPicsInSubDpb[vps->getSubDpbAssigned( targetLsIdx, i )] = vps->getMaxVpsDecPicBufferingMinus1( targetOutputLsIdx, vps->getSubDpbAssigned( targetLsIdx, i ), highestTId) + 1;
#else
      maxDpbLimit.m_numPicsInSubDpb[i] = vps->getMaxVpsDecPicBufferingMinus1( targetOutputLsIdx, i, highestTId) + 1;
#endif
    }
    // -------------------------------------
  }
  return vps;
}
Void TAppDecTop::emptyUnusedPicturesNotNeededForOutput()
{
#if FIX_ALIGN_BUMPING
  for(Int layerIdx = 0; layerIdx < MAX_VPS_LAYER_IDX_PLUS1; layerIdx++)
#else
  for(Int layerIdx = 0; layerIdx < MAX_LAYERS; layerIdx++)
#endif
  {
    TComList <TComPic*> *pcListPic = m_acTDecTop[layerIdx].getListPic();
    TComList<TComPic*>::iterator iterPic = pcListPic->begin();
    while ( iterPic != pcListPic->end() )
    {
      TComPic *pic = *iterPic;
      if( !pic->getSlice(0)->isReferenced() && !pic->getOutputMark() )
      {
        // Emtpy the picture buffer
        pic->setReconMark( false );
      }
      iterPic++;
    }
  }
}

Bool TAppDecTop::ifInvokeBumpingBeforeDecoding( const DpbStatus &dpbStatus, const DpbStatus &dpbLimit, const Int layerIdx, const Int subDpbIdx )
{
  Bool retVal = false;
  // Number of reorder picutres
  retVal |= ( dpbStatus.m_numAUsNotDisplayed > dpbLimit.m_numAUsNotDisplayed );

  // Number of pictures in each sub-DPB
  retVal |= ( dpbStatus.m_numPicsInSubDpb[subDpbIdx] >= dpbLimit.m_numPicsInSubDpb[subDpbIdx] );

  return retVal;
}

Bool TAppDecTop::ifInvokeBumpingAfterDecoding( const DpbStatus &dpbStatus, const DpbStatus &dpbLimit )
{
  Bool retVal = false;

  // Number of reorder picutres
  retVal |= ( dpbStatus.m_numAUsNotDisplayed > dpbLimit.m_numAUsNotDisplayed );

  return retVal;
}

Void TAppDecTop::xFindDPBStatus( std::vector<Int> &listOfPocs
                            , std::vector<Int> *listOfPocsInEachLayer
                            , std::vector<Int> *listOfPocsPositionInEachLayer
                            , DpbStatus &dpbStatus
#if POC_RESET_IDC_DECODER
                            , Bool notOutputCurrAu
#endif
                            )
{
  TComVPS *vps = NULL;
  dpbStatus.init();
#if FIX_ALIGN_BUMPING
  for( Int i = 0; i < MAX_VPS_LAYER_IDX_PLUS1; i++ )
#else
  for( Int i = 0; i < MAX_LAYERS; i++ )
#endif
  {
    if( m_acTDecTop[i].getListPic()->empty() )
    {
      continue;
    }
    
    // To check # AUs that have at least one picture not output,
    // For each layer, populate listOfPOcs if not already present
    TComList<TComPic*>::iterator iterPic = m_acTDecTop[i].getListPic()->begin();
    Int picPositionInList = 0;
    while (iterPic != m_acTDecTop[i].getListPic()->end())
    {
      TComPic* pic = *(iterPic);
      if( pic->getReconMark() )
      {
        if( vps == NULL )
        {
          vps = pic->getSlice(0)->getVPS();
        }
#if POC_RESET_IDC_DECODER
        if( !(pic->isCurrAu() && notOutputCurrAu ) )
        {
#endif
          std::vector<Int>::iterator it;
          if( pic->getOutputMark() ) // && pic->getPOC() > m_aiPOCLastDisplay[i])
          {
            it = find( listOfPocs.begin(), listOfPocs.end(), pic->getPOC() ); // Check if already included
            if( it == listOfPocs.end() )  // New POC value - i.e. new AU - add to the list
            {
              listOfPocs.push_back( pic->getPOC() );
            }
            listOfPocsInEachLayer         [i].push_back( pic->getPOC()    );    // POC to be output in each layer
            listOfPocsPositionInEachLayer [i].push_back( picPositionInList  );  // For ease of access
          }
          if( pic->getSlice(0)->isReferenced() || pic->getOutputMark() )
          {
            dpbStatus.m_numPicsInSubDpb[i]++;  // Count pictures that are "used for reference" or "needed for output"
          }
#if POC_RESET_IDC_DECODER
        }
#endif
      }
      iterPic++;
      picPositionInList++;
    }
  }

  assert( vps != NULL );    // No picture in any DPB?
  std::sort( listOfPocs.begin(), listOfPocs.end() );    // Sort in increasing order of POC
  Int targetLsIdx = vps->getOutputLayerSetIdx( getCommonDecoderParams()->getTargetOutputLayerSetIdx() );
  // Update status
  dpbStatus.m_numAUsNotDisplayed = (Int)listOfPocs.size();   // Number of AUs not displayed
  dpbStatus.m_numLayers = vps->getNumLayersInIdList( targetLsIdx );
#if FIX_ALIGN_BUMPING
  for(Int i = 0; i < dpbStatus.m_numLayers; i++)
  {
    dpbStatus.m_layerIdToSubDpbIdMap[vps->getLayerSetLayerIdList(targetLsIdx, i)] = i;
    dpbStatus.m_targetDecLayerIdList[i] = vps->getLayerSetLayerIdList(targetLsIdx, i);  // Layer Id stored in a particular sub-DPB
  }
  dpbStatus.m_numSubDpbs = vps->getNumSubDpbs( targetLsIdx ); 
#else
  dpbStatus.m_numSubDpbs = vps->getNumSubDpbs( vps->getOutputLayerSetIdx(
                                                      this->getCommonDecoderParams()->getTargetOutputLayerSetIdx() ) );
#endif

#if FIX_ALIGN_BUMPING
  for(Int i = 0; i < MAX_VPS_LAYER_IDX_PLUS1; i++)
#else
  for(Int i = 0; i < dpbStatus.m_numLayers; i++)
#endif
  {
    dpbStatus.m_numPicsNotDisplayedInLayer[i] = (Int)listOfPocsInEachLayer[i].size();
  }
  assert( dpbStatus.m_numAUsNotDisplayed != -1 );
}  

#if POC_RESET_IDC_DECODER
Void TAppDecTop::outputAllPictures(Int layerId, Bool notOutputCurrPic)
{
  { // All pictures in the DPB in that layer are to be output; this means other pictures would also be output
    std::vector<Int>  listOfPocs;
#if FIX_ALIGN_BUMPING
    std::vector<Int>  listOfPocsInEachLayer[MAX_VPS_LAYER_IDX_PLUS1];
    std::vector<Int>  listOfPocsPositionInEachLayer[MAX_VPS_LAYER_IDX_PLUS1];
#else
    std::vector<Int>  listOfPocsInEachLayer[MAX_LAYERS];
    std::vector<Int>  listOfPocsPositionInEachLayer[MAX_LAYERS];
#endif
    DpbStatus dpbStatus;

    // Find the status of the DPB
    xFindDPBStatus(listOfPocs, listOfPocsInEachLayer, listOfPocsPositionInEachLayer, dpbStatus, notOutputCurrPic);

    if( listOfPocs.size() )
    {
      while( listOfPocsInEachLayer[layerId].size() )    // As long as there picture in the layer to be output
      {
        bumpingProcess( listOfPocs, listOfPocsInEachLayer, listOfPocsPositionInEachLayer, dpbStatus );
      }
    }
  }
}
#endif
#endif

#if Q0074_COLOUR_REMAPPING_SEI
Void xInitColourRemappingLut( const Int bitDepthY, const Int bitDepthC, std::vector<Int>(&preLut)[3], std::vector<Int>(&postLut)[3], const SEIColourRemappingInfo* const pCriSEI )
{
  for ( Int c=0 ; c<3 ; c++ )
  {  
    Int bitDepth = c ? bitDepthC : bitDepthY ;
    preLut[c].resize(1 << bitDepth);
    postLut[c].resize(1 << pCriSEI->m_colourRemapBitDepth);
    
    Int bitDepthDiff = pCriSEI->m_colourRemapBitDepth - bitDepth;
    Int iShift1 = (bitDepthDiff>0) ? bitDepthDiff : 0; //bit scale from bitdepth to ColourRemapBitdepth (manage only case colourRemapBitDepth>= bitdepth)
    if( bitDepthDiff<0 )
      printf ("Warning: CRI SEI - colourRemapBitDepth (%d) <bitDepth (%d) - case not handled\n", pCriSEI->m_colourRemapBitDepth, bitDepth);
    bitDepthDiff = pCriSEI->m_colourRemapBitDepth - pCriSEI->m_colourRemapInputBitDepth;
    Int iShift2 = (bitDepthDiff>0) ? bitDepthDiff : 0; //bit scale from ColourRemapInputBitdepth to ColourRemapBitdepth (manage only case colourRemapBitDepth>= colourRemapInputBitDepth)
    if( bitDepthDiff<0 )
      printf ("Warning: CRI SEI - colourRemapBitDepth (%d) <colourRemapInputBitDepth (%d) - case not handled\n", pCriSEI->m_colourRemapBitDepth, pCriSEI->m_colourRemapInputBitDepth);

    //Fill preLut
    for ( Int k=0 ; k<(1<<bitDepth) ; k++ )
    {
      Int iSample = k << iShift1 ;
      for ( Int iPivot=0 ; iPivot<=pCriSEI->m_preLutNumValMinus1[c] ; iPivot++ )
      {
        Int iCodedPrev  = pCriSEI->m_preLutCodedValue[c][iPivot]    << iShift2; //Coded in CRInputBitdepth
        Int iCodedNext  = pCriSEI->m_preLutCodedValue[c][iPivot+1]  << iShift2; //Coded in CRInputBitdepth
        Int iTargetPrev = pCriSEI->m_preLutTargetValue[c][iPivot];              //Coded in CRBitdepth
        Int iTargetNext = pCriSEI->m_preLutTargetValue[c][iPivot+1];            //Coded in CRBitdepth
        if ( iCodedPrev <= iSample && iSample <= iCodedNext )
        {
          Float fInterpol = (Float)( (iCodedNext - iSample)*iTargetPrev + (iSample - iCodedPrev)*iTargetNext ) * 1.f / (Float)(iCodedNext - iCodedPrev);
          preLut[c][k]  = (Int)( 0.5f + fInterpol );
          iPivot = pCriSEI->m_preLutNumValMinus1[c] + 1;
        }
      }
    }
    
    //Fill postLut
    for ( Int k=0 ; k<(1<<pCriSEI->m_colourRemapBitDepth) ; k++ )
    {
      Int iSample = k;
      for ( Int iPivot=0 ; iPivot<=pCriSEI->m_postLutNumValMinus1[c] ; iPivot++ )
      {
        Int iCodedPrev  = pCriSEI->m_postLutCodedValue[c][iPivot];    //Coded in CRBitdepth
        Int iCodedNext  = pCriSEI->m_postLutCodedValue[c][iPivot+1];  //Coded in CRBitdepth
        Int iTargetPrev = pCriSEI->m_postLutTargetValue[c][iPivot];   //Coded in CRBitdepth
        Int iTargetNext = pCriSEI->m_postLutTargetValue[c][iPivot+1]; //Coded in CRBitdepth
        if ( iCodedPrev <= iSample && iSample <= iCodedNext )
        {
          Float fInterpol =  (Float)( (iCodedNext - iSample)*iTargetPrev + (iSample - iCodedPrev)*iTargetNext ) * 1.f / (Float)(iCodedNext - iCodedPrev) ;
          postLut[c][k]  = (Int)( 0.5f + fInterpol );
          iPivot = pCriSEI->m_postLutNumValMinus1[c] + 1;
        }
      }
    }
  }
}

static void applyColourRemapping(TComPicYuv& pic, const SEIColourRemappingInfo* pCriSEI, UInt layerId )
{  
  if( !storeCriSEI.size() )
#if SVC_EXTENSION
    storeCriSEI.resize(MAX_LAYERS);
#else
    storeCriSEI.resize(1);
#endif

  if ( pCriSEI ) //if a CRI SEI has just been retrieved, keep it in memory (persistence management)
    storeCriSEI[layerId] = *pCriSEI;

  if( !storeCriSEI[layerId].m_colourRemapCancelFlag && pCriSEI) 
  {
    Int iHeight  = pic.getHeight(COMPONENT_Y);
    Int iWidth   = pic.getWidth(COMPONENT_Y);
    Int iStride  = pic.getStride(COMPONENT_Y);
    Int iCStride = pic.getStride(COMPONENT_Cb);

    Pel *YUVIn[3], *YUVOut[3];
    YUVIn[0] = pic.getAddr(COMPONENT_Y);
    YUVIn[1] = pic.getAddr(COMPONENT_Cb);
    YUVIn[2] = pic.getAddr(COMPONENT_Cr);
    
    TComPicYuv picColourRemapped;
#if SVC_EXTENSION
#if AUXILIARY_PICTURES
    picColourRemapped.create( pic.getWidth(COMPONENT_Y), pic.getHeight(COMPONENT_Y), pic.getChromaFormat(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, NULL );
#else
    picColourRemapped.create( pic.getWidth(), pic.getHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, NULL );
#endif
#else
    picColourRemapped.create( pic.getWidth(COMPONENT_Y), pic.getHeight(COMPONENT_Y), pic.getChromaFormat(), g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth );
#endif 
    YUVOut[0] = picColourRemapped.getAddr(COMPONENT_Y);
    YUVOut[1] = picColourRemapped.getAddr(COMPONENT_Cb);
    YUVOut[2] = picColourRemapped.getAddr(COMPONENT_Cr);

#if SVC_EXTENSION
    Int bitDepthY = g_bitDepthLayer[CHANNEL_TYPE_LUMA][layerId];
    Int bitDepthC = g_bitDepthLayer[CHANNEL_TYPE_CHROMA][layerId];

#else
    Int bitDepthY = g_bitDepth[CHANNEL_TYPE_LUMA];
    Int bitDepthC = g_bitDepth[CHANNEL_TYPE_CHROMA];
#endif

    std::vector<Int> preLut[3];
    std::vector<Int> postLut[3];
    xInitColourRemappingLut( bitDepthY, bitDepthC, preLut, postLut, &storeCriSEI[layerId] );
    
    Int roundingOffset = (storeCriSEI[layerId].m_log2MatrixDenom==0) ? 0 : (1 << (storeCriSEI[layerId].m_log2MatrixDenom - 1));

    for( Int y = 0; y < iHeight ; y++ )
    {
      for( Int x = 0; x < iWidth ; x++ )
      {
        Int YUVPre[3], YUVMat[3];
        YUVPre[0] = preLut[0][ YUVIn[0][x]   ];
        YUVPre[1] = preLut[1][ YUVIn[1][x>>1] ];
        YUVPre[2] = preLut[2][ YUVIn[2][x>>1] ];

        YUVMat[0] = ( storeCriSEI[layerId].m_colourRemapCoeffs[0][0]*YUVPre[0]
                    + storeCriSEI[layerId].m_colourRemapCoeffs[0][1]*YUVPre[1] 
                    + storeCriSEI[layerId].m_colourRemapCoeffs[0][2]*YUVPre[2] 
                    + roundingOffset ) >> ( storeCriSEI[layerId].m_log2MatrixDenom );
        YUVMat[0] = Clip3( 0, (1<<storeCriSEI[layerId].m_colourRemapBitDepth)-1, YUVMat[0] );
        YUVOut[0][x] = postLut[0][ YUVMat[0] ];

        if( (y&1) && (x&1) )
        {
          for(Int c=1 ; c<3 ; c++)
          {
            YUVMat[c] = ( storeCriSEI[layerId].m_colourRemapCoeffs[c][0]*YUVPre[0] 
                        + storeCriSEI[layerId].m_colourRemapCoeffs[c][1]*YUVPre[1] 
                        + storeCriSEI[layerId].m_colourRemapCoeffs[c][2]*YUVPre[2] 
                        + roundingOffset ) >> ( storeCriSEI[layerId].m_log2MatrixDenom );
            YUVMat[c] = Clip3( 0, (1<<storeCriSEI[layerId].m_colourRemapBitDepth)-1, YUVMat[c] );
            YUVOut[c][x>>1] = postLut[c][ YUVMat[c] ];   
          }
        }
      }
      YUVIn[0]  += iStride;
      YUVOut[0] += iStride;
      if( y&1 )
      {
        YUVIn[1]  += iCStride;
        YUVIn[2]  += iCStride;
        YUVOut[1] += iCStride;
        YUVOut[2] += iCStride;
      }
    }

    //Write remapped picture in decoding order
    Char  cTemp[255];
    sprintf(cTemp, "seiColourRemappedPic_L%d_%dx%d_%dbits.yuv", layerId, iWidth, iHeight, storeCriSEI[layerId].m_colourRemapBitDepth );
    picColourRemapped.dump( cTemp, true, storeCriSEI[layerId].m_colourRemapBitDepth );

    picColourRemapped.destroy();

    storeCriSEI[layerId].m_colourRemapCancelFlag = !storeCriSEI[layerId].m_colourRemapPersistenceFlag; //Handling persistence
  }
  else
  {
    //Write no remapped picture in decoding order
    if (storeCriSEI[layerId].m_colourRemapBitDepth == 8 || storeCriSEI[layerId].m_colourRemapBitDepth == 10)
    {
      Char  cTemp[255];
      sprintf(cTemp, "seiColourRemappedPic_L%d_%dx%d_%dbits.yuv", layerId, pic.getWidth(COMPONENT_Y), pic.getHeight(COMPONENT_Y), storeCriSEI[layerId].m_colourRemapBitDepth );
      pic.dump( cTemp, true, storeCriSEI[layerId].m_colourRemapBitDepth );
    }
  }
}
#endif
//! \}
