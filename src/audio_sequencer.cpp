/*
 * Copyright (c) 2024-2044, EVS Broadcast Equipment S.A. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * This module implements the Audio Sequencer Object
 *
 * Author:      Bernard HARMEL: b.harmel@evs.com
 *
 * History:
 * V 1.00  Apr 17 2024  BHA : Initial release
 */
#include "audio_sequencer.h"
#include <libevs-pcie-win-io-api/src/EvsPcieIoHelpers.h> 

// MUST folow the definition of enum eFrameRate : uint8_t
uint32_t AudioSequencer::S_pAudioSeq_U32[16][5] = {
    {0, 0, 0, 0, 0},                // Dummy
    {0, 0, 0, 0, 0},                // Dummy
    {2002, 2002, 2002, 2002, 2002}, // 23.98f 24*1000/1001
    {2000, 2000, 2000, 2000, 2000}, // 24.00f
    {1001, 1001, 1001, 1001, 1000}, // 47.95f 48*1000/1001
    {1920, 1920, 1920, 1920, 1920}, // 25.00f
    {1602, 1602, 1602, 1602, 1600}, // 29.97f 30*1000/1001
    {1600, 1600, 1600, 1600, 1600}, // 30.00f
    {1000, 1000, 1000, 1000, 1000}, // 48.00f
    {960, 960, 960, 960, 960},      // 50.00f
    {801, 801, 801, 801, 800},      // 59.94f 60*1000/1001
    {800, 800, 800, 800, 800},      // 60.00f
    {0, 0, 0, 0, 0},                // Dummy
    {0, 0, 0, 0, 0},                // Dummy
    {0, 0, 0, 0, 0},                // Dummy
    {0, 0, 0, 0, 0},                // Dummy
};

AudioSequencer::AudioSequencer(EVS::EvsPcieIoApi::eFrameRate _AudioFrameRate_E)
{
  mAudioFrameRate_E = _AudioFrameRate_E;
  
}
AudioSequencer::~AudioSequencer()
{
}
void AudioSequencer::Reset()
{
  mCrtSeqIndx_U32 = 0;
}
uint32_t AudioSequencer::Current()
{
  return (mAudioFrameRate_E != EVS::EvsPcieIoApi::eFrameRate::Rate_invalid) ? S_pAudioSeq_U32[mAudioFrameRate_E][mCrtSeqIndx_U32] : S_pAudioSeq_U32[0][mCrtSeqIndx_U32];
}
uint32_t AudioSequencer::Next()
{
  uint32_t Rts_U32 = Current();
  
  mCrtSeqIndx_U32++;
  if (mCrtSeqIndx_U32 >= sizeof(S_pAudioSeq_U32[0]) / sizeof(S_pAudioSeq_U32[0][0]))
  {
    mCrtSeqIndx_U32 = 0;
  }
  return Rts_U32;
}
// MUST folow the definition of enum eFrameRate : uint8_t
uint64_t AudioSequencer::TsInc()
{
  constexpr float TS_SCALE = (1000.0f * 1000.0f); // To nano
  static uint64_t S_pTsInc_U64[] = {
      0,
      0,
      static_cast<uint64_t>((1000.0f / (24.0f * 1000.0f / 1001.0f)) * TS_SCALE),
      static_cast<uint64_t>((1000.0f / (24.0f * 1000.0f / 1000.0f)) * TS_SCALE),
      static_cast<uint64_t>((1000.0f / (48.0f * 1000.0f / 1001.0f)) * TS_SCALE),
      static_cast<uint64_t>((1000.0f / (25.0f * 1000.0f / 1000.0f)) * TS_SCALE),
      static_cast<uint64_t>((1000.0f / (30.0f * 1000.0f / 1001.0f)) * TS_SCALE),
      static_cast<uint64_t>((1000.0f / (30.0f * 1000.0f / 1000.0f)) * TS_SCALE),
      static_cast<uint64_t>((1000.0f / (48.0f * 1000.0f / 1000.0f)) * TS_SCALE),
      static_cast<uint64_t>((1000.0f / (50.0f * 1000.0f / 1000.0f)) * TS_SCALE),
      static_cast<uint64_t>((1000.0f / (60.0f * 1000.0f / 1001.0f)) * TS_SCALE),
      static_cast<uint64_t>((1000.0f / (60.0f * 1000.0f / 1000.0f)) * TS_SCALE),
      static_cast<uint64_t>(0 * TS_SCALE),
      0,
      0,
      0,
      0};
  return (mAudioFrameRate_E != EVS::EvsPcieIoApi::eFrameRate::Rate_invalid) ? S_pTsInc_U64[mAudioFrameRate_E] : S_pTsInc_U64[0];
}
