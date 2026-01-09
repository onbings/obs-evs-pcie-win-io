/*
 * Copyright (c) 2024-2044, EVS Broadcast Equipment S.A. All rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 * PURPOSE.
 *
 * This module defines the Audio Sequencer Object
 *
 * Author:      Bernard HARMEL: b.harmel@evs.com
 *
 * History:
 * V 1.00  Apr 17 2024  BHA : Initial release
 */
#pragma once
#include <cstdint>
#include <libevs-pcie-win-io-api/src/EvsAVDefs.h>

class AudioSequencer
{
public:
  AudioSequencer(EVS::EvsPcieIoApi::eFrameRate _AudioFrameRate_E);
  ~AudioSequencer();
  void Reset();
  uint32_t Current();
  uint32_t Next();
  uint64_t TsInc();

private:
  EVS::EvsPcieIoApi::eFrameRate mAudioFrameRate_E = EVS::EvsPcieIoApi::Rate_invalid;
  uint32_t mCrtSeqIndx_U32 = 0;
  static uint32_t S_pAudioSeq_U32[16][5];
};

