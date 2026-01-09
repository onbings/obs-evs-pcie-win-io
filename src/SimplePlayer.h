/*++
Copyright (c) Evs Broadcast Equipment. (a) b.harmel@evs.com. All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.
--*/
#pragma once
#include "main.h"
#include <evs-pcie-win-io-drv/evs_pa10xx_shared.h>
#include <libevs-pcie-win-io-api/src/EvsPcieIoApi.h>
#include <libevs-pcie-win-io-api/src/ioBoard/BasePcieIoBoard.h>
#include <libevs-pcie-win-io-api/src/ioChannel/PA10XXPlayers.h>
#include <libevs-win-hwlgpl/src/VseList.h>
#include <libevs-win-hwlgpl/src/WaitableFifo.h>

struct AVa_PLAYER_PARAM
{
  std::string BaseLoadDir_S;
  bool UhdCapable_B;
  EVS::EvsPcieIoApi::eVideoStandard VideoStandard_E;
  uint32_t NbFrameToPreload_U32; 

  std::string HrFileName_S;  // Always enabled
  bool Hr10bit_B;

  std::string pAudioFileName_S[AUDIO_NBRCH]; 
  uint32_t AudioChannelMaskToPlay_U32; // Enable audio if not 0
  uint32_t AudioAttenuator_U32;    // SnapshotSave_B ? 0 : 256;  24bits sign extended to 32bits Full res

  std::string AncFileName_S; // Enabled if not ""

  bool SnapshotLoad_B; // false: we load data from a single big file true: each essence is loaded from a different big file

  struct AVa_PLAYER_PARAM()
  {
    Reset();
  }
  void Reset()
  {
    uint32_t i_U32;
    BaseLoadDir_S = "";
    UhdCapable_B = false;
    VideoStandard_E = EVS::EvsPcieIoApi::VideoStd_Invalid;
    NbFrameToPreload_U32 = 0;
    HrFileName_S = "";
    Hr10bit_B = false;
    for (i_U32 = 0; i_U32 < AUDIO_NBRCH; i_U32++)
    {
      pAudioFileName_S[i_U32] = "";
    }
    AudioChannelMaskToPlay_U32 = 0;
    AudioAttenuator_U32 = 0;
    AncFileName_S = "";
    SnapshotLoad_B = false;
  }
};
typedef struct 
{
  LIST_ITEM FifoRefCollection;

  /*frame*/
  int FrameStatus_i;
  EVS::EvsPcieIoApi::FRAME_EMITTED_INFO FrameData_X;

  /*Others debug infos*/
  EVS::EvsPcieIoApi::PLAY_FRAME_DEBUG FrameDebug_X;
  EVS::EvsPcieIoApi::AUDIO_FRAME_DEBUG AudioDebug_X;
  EVS::EvsPcieIoApi::ANCILLARY_FRAME_DEBUG AncDebug_X;
  uint64_t TickAppInsertion_U64;
  uint64_t TickAppInterrupt_U64;

} PlayerItem, *PPlayerItem;
struct AVa_PLAYER_CHANNEL
{
  EVS::EvsPcieIoApi::CBasePcieIoBoard *pPcieIoBoard;
  EVS::EvsPcieIoApi::IEvsIoBoard *pIIoBoard;
  EVS::EvsPcieIoApi::IPlayer *pIPlayer;
  uint32_t GlkTaskId_U8;
  uint32_t GlkTaskDelayInUs_U32;
  std::unique_ptr<AudioSequencer> puAudioSequencer;
  uint32_t VideoFileOffset_U32;
  uint32_t AudioFileOffset_U32;
  uint32_t AncFileOffset_U32;

  AVa_PLAYER_CHANNEL()
  {
    Reset();
  }
  void Reset()
  {
    pPcieIoBoard = nullptr;
    pIIoBoard = nullptr;
    pIPlayer = nullptr;
    GlkTaskId_U8 = 0;
    GlkTaskDelayInUs_U32 = 0;
    puAudioSequencer = nullptr;
    VideoFileOffset_U32 = 0;
    AudioFileOffset_U32 = 0;
    AncFileOffset_U32 = 0;
  }
};

class SimplePlayer : public EVS::EvsPcieIoApi::IPlayFrameEmitObserver, public EVS::EvsPcieIoApi::IGenlockFrameObserver, public EVS::EvsPcieIoApi::IGenlockTask
{
public:
  SimplePlayer(EVS::EvsPcieIoApi::IEvsIoBoard *_pIIoBoard, EVS::EvsPcieIoApi::IPlayer *_pIPlayer, uint8_t _GlkTaskId_U8, uint32_t _GlkTaskDelayInUs_U32);
  virtual ~SimplePlayer();

  int OpenPlayer(AVa_PLAYER_PARAM &_rPlayerParam_X);
  int ClosePlayer(bool _FullStop_B);

  EVS::EvsPcieIoApi::eVideoStandard GetVideoStandard();

  int SwitchToPatternGenerator(uint32_t _Height_U32, uint32_t _Width_U32);
  int SwitchToDMA();

  /*allocate new frames for receiving*/
  int AllocAndQueueNewFrame(uint32_t _NbFrame_U32);
  int FreeQueuedFrame();

  /*Task to push frames to m_QueuePush*/
  int StartSendNewFrameThread();
  int StopSendNewFrameThread();

  /*Task to move frames from mFifoPlayerPop to mFifoPlayerPush */
  int StartRecycleFrameSentBufferThread();
  int StopRecycleFrameSentBufferThread();
  uint32_t GetRestartCounter();
  uint32_t GetUnderflowCounter();
  private: /*Observer implementation*/
  virtual void GenlockFrameEvent(EVS::EvsPcieIoApi::PGLK_DATA _pGlkData_X, EVS::EvsPcieIoApi::PLTC_DATA _pLtcData_X) override;
  // use  genlockDefferedTask
  virtual void GenlockTask(uint32_t _TaskId_U32, EVS::EvsPcieIoApi::PGLK_DATA _pLastGlkData_X) override;
  virtual int NewFrameEmitted(EVS::EvsPcieIoApi::FRAME_STATUS _Status_E, EVS::EvsPcieIoApi::PFRAME_EMITTED_INFO _pFrameEmittedInfo_X) override;

  int Start(EVS::EvsPcieIoApi::eVideoStandard _VideoStandard_E);
  int Stop();

  void RestartPlayer();
  void SendNewFrameThread();
  void RecycleFrameSentBufferThread();
  int AllocateDefaultSendingFrame(uint32_t _FrameId_U32, EVS::EvsPcieIoApi::PPUT_FRAME_PARAMS(&_rppPutFrameParam_X)[2]);
  void FreeSendingFrame(EVS::EvsPcieIoApi::PPUT_FRAME_PARAMS _pPutFrameParam_X);

  bool StartPlayerGo();

//public:
  uint32_t mRestartCounter_U32 = 0;
  AVa_PLAYER_CHANNEL mPlayerChannel_X = {};
  EVS::EvsPcieIoApi::PLAYER_PARAMS mPlayerParam_X = {};
  AVa_PLAYER_PARAM mAVaPlayerParam_X = {};

#if 0 /// Remove ip support
  EVS::EvsPcieIoApi::IP_TX_STREAM_PARAMS m_IPvideoStream = {};
  EVS::EvsPcieIoApi::IP_TX_STREAM_PARAMS m_IPAudioStream = {};
#endif
  static EVS::EvsPcieIoApi::eVideoStandard S_mGlobalVideoStandard_E;
  bool mPlayerInitialized_B = false;
  bool mAudioValid_B = false;
  bool mPlayerGlkTaskReceived_B = false;
  bool mPlayerGlkReceived_B = false;
  bool mPlayerStarted_B = false;
  uint64_t mPlayerStartDelay_U32 = 5 * 1000;

  /*Audio Buffer parameters*/
//  uint32_t mAudioLevel_U32 = 1228544;

  /* link pop to push */
  std::thread mRecycleFrameSentBufferThread;
  bool mStopRecycleFrameSentBufferThread_B = false;
  std::thread mSendNewFrameThread;
  bool mStopSendNewFrameThread_B = false;

  PlayerItem *mpPlayerItemPool = nullptr;
  uint32_t mPlayerItemPoolSize_U32 = 0;

  /*fifo*/
  EvsHwLGPL::CWaitFifo mFifoFrameToSend;
  EvsHwLGPL::CWaitFifo mFifoFrameToRecycle;

  uint32_t mFrameCounter_U32 = 0;

  /*Counters*/
  struct
  {
    struct
    {
      uint32_t NbItemInApiFifo_U32 = 0; // current number of items in the API fifo

      /*Drive advanced info*/
      uint32_t DrvHiresWaiting_U32 = 0;
      uint32_t DrvHiresFlying_U32 = 0;
      uint32_t DrvLoresWaiting_U32 = 0;
      uint32_t DrvLoresFlying_U32 = 0;
      uint32_t DrvAudio_U32 = 0;
      uint32_t DrvAnc_U32 = 0;
    } Fifo;

    struct
    {
      uint64_t DeltaAppDriver_U64 = 0;
      uint64_t DeltaDriverWaiting_U64 = 0;
      uint64_t DeltaWaitingFlying_U64 = 0;
      uint64_t DeltaAppFlying_U64 = 0;

      uint64_t DeltaInterruptTask_U64 = 0;
      uint64_t DeltaTaskCompletion_U64 = 0;
      uint64_t DeltaCompletionApp_U64 = 0;
      uint64_t DeltaInterruptApp_U64 = 0;

      uint32_t TimeRefGo_U32 = 0;
      uint32_t TimeRefGoGenlock_U32 = 0;
      uint64_t DeltaGoFnCall_U64 = 0;

    } TimeMeasurement;

  } mPlayerCounter_X;
};
