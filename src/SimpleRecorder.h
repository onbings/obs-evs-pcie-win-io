/*++
Copyright (c) Evs Broadcast Equipment. (a) b.harmel@evs.com. All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.
--*/
#pragma once
#include <evs-pcie-win-io-drv/evs_pa10xx_shared.h>
#include <libevs-pcie-win-io-api/src/EvsPcieIoApi.h>
#include <libevs-pcie-win-io-api/src/ioBoard/BasePcieIoBoard.h>
#include <libevs-win-hwlgpl/src/WaitableFifo.h>

struct AVa_RECORDER_PARAM
{
  bool SaveVideo_B; //Always enabled
  bool Hr10bit_B;

  bool SaveProxy_B;
  uint16_t LrWidth_U16;  //If not 0, enable
  uint16_t LrHeight_U16; //If not 0, enable
  bool Lr10bit_B;

  bool SaveAudio_B = true;
  uint32_t AudioChannelMaskToCapture_U32; // Enable audio if not 0
  uint32_t AudioAmplifier_U32;            // SnapshotSave_B ? 0 : 256;  24bits sign extended to 32bits Full res

  bool SaveAnc_B = true;
  bool EnableAnc_B;

  std::string BaseSaveDir_S;
  bool SnapshotSave_B; // false: we capture in a single big file true: each essence is in a different big file

  struct AVa_RECORDER_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    SaveVideo_B = false;
    Hr10bit_B = false;
    SaveProxy_B = false;
    LrWidth_U16 = 0;
    LrHeight_U16 = 0;
    Lr10bit_B = false;
    SaveAudio_B = false;
    AudioChannelMaskToCapture_U32 = 0;
    AudioAmplifier_U32 = 0;
    SaveAnc_B = false;
    EnableAnc_B = false;
    BaseSaveDir_S = "";
    SnapshotSave_B = false;
  }
};
class RecorderItem
{
public:
  LIST_ITEM FifoRefCollection; // is the reference for fifo

  /*Useful data*/
  int FrameStatus_i;
  EVS::EvsPcieIoApi::FRAME_RECEIVED_INFO FrameData_X;

  uint8_t ExpectedParity_U8;
};
struct AVa_RECORDER_CHANNEL
{
  EVS::EvsPcieIoApi::CBasePcieIoBoard *pPcieIoBoard;
  EVS::EvsPcieIoApi::IEvsIoBoard *pIIoBoard;
  EVS::EvsPcieIoApi::IRecorder *pIRecorder;
  EVS::EvsPcieIoApi::eVideoStandard VideoStandard_E;
  AVa_RECORDER_CHANNEL()
  {
    Reset();
  }
  void Reset()
  {
    pPcieIoBoard = nullptr;
    pIIoBoard = nullptr;
    pIRecorder = nullptr;
    VideoStandard_E = EVS::EvsPcieIoApi::eVideoStandard::VideoStd_Invalid;
  }
};
class SimpleRecorder : public EVS::EvsPcieIoApi::IInputStsChangeObserver, public EVS::EvsPcieIoApi::IRecFrameRcvObserver
{
public:
  SimpleRecorder(EVS::EvsPcieIoApi::IEvsIoBoard *_pIIoBoard, EVS::EvsPcieIoApi::IRecorder *_pIRecorder);
  virtual ~SimpleRecorder();

  int OpenRecorder(AVa_RECORDER_PARAM &_rRecorderParam_X);
  int CloseRecorder();
  EVS::EvsPcieIoApi::eVideoStandard GetVideoStandard();

protected: /* Recorder methods */
  void RequestNewRecorderItem(RecorderItem *_pRecorderItem0, RecorderItem *_pRecorderItem1);
  void RecorderBgTask();
  int AllocateRecorderItem(RecorderItem *_pRecorderItem, uint32_t _Id_U32);
  int ReleaseRecorderItem(RecorderItem *_pRecorderItem);

private: /*Observer implementation*/
  int InputStatusChange(uint32_t _Flag_U32, EVS::EvsPcieIoApi::eVideoStandard _DetectedStd_E) override;
  int NewFrameReceived(EVS::EvsPcieIoApi::FRAME_STATUS _Status_E, EVS::EvsPcieIoApi::PFRAME_RECEIVED_INFO _pReceivedInfo_X) override;

  int Start();
  int Stop();

protected:
  AVa_RECORDER_CHANNEL mRecorderChannel_X = {};
  EVS::EvsPcieIoApi::RECORDER_PARAMS mRecorderParam_X = {};
  AVa_RECORDER_PARAM mAVaRecorderParam_X = {};

  static const uint32_t mFrameArraySize_U32 = 16;
  RecorderItem mpFrame[mFrameArraySize_U32];
  bool mStopRecorderThread_B = false;
  std::thread mRecorderThread;

  bool mStdFounded_B = false;
  bool mRecorderInitialized_B = false;
  uint16_t mCurrentSessionId_U32 = 0;
  uint32_t mReceivedNbFrame_U32 = 0;
  EvsHwLGPL::CWaitFifo mReceivedFrameFifo;
};
