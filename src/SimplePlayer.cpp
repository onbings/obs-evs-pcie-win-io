/*++
Copyright (c) Evs Broadcast Equipment. (a) b.harmel@evs.com. All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.
--*/
#include "SimplePlayer.h"
#include <cassert>
#include <libevs-pcie-win-io-api/src/ioChannel/PA1022Channels.h>
#include <libevs-pcie-win-io-api/src/private/AncillaryParser.h>
#include <libevs-win-hwlgpl/src/VseLogger_exp.h>

extern uint64_t GL_LastGenlockTime_U64;
EVS::EvsPcieIoApi::eVideoStandard SimplePlayer::S_mGlobalVideoStandard_E = EVS::EvsPcieIoApi::eVideoStandard::VideoStd_Invalid;

SimplePlayer::SimplePlayer(EVS::EvsPcieIoApi::IEvsIoBoard *_pIIoBoard, EVS::EvsPcieIoApi::IPlayer *_pIPlayer, uint8_t _GlkTaskId_U8, uint32_t _GlkTaskDelayInUs_U32)
{
  mPlayerChannel_X.pPcieIoBoard = (EVS::EvsPcieIoApi::CBasePcieIoBoard *)_pIIoBoard;
  mPlayerChannel_X.pIIoBoard = _pIIoBoard;
  mPlayerChannel_X.pIPlayer = _pIPlayer;
  mPlayerChannel_X.GlkTaskId_U8 = _GlkTaskId_U8;
  mPlayerChannel_X.GlkTaskDelayInUs_U32 = _GlkTaskDelayInUs_U32;
  mFifoFrameToSend.Init();
  mFifoFrameToRecycle.Init();
}

SimplePlayer::~SimplePlayer()
{
  ClosePlayer(true);
}

int SimplePlayer::OpenPlayer(AVa_PLAYER_PARAM &_rPlayerParam_X)
{
  int Rts_i = EVS::EvsPcieIoApi::NoError;
  float VideoFrameRate_f;
  EVS::EvsPcieIoApi::eFrameRate InterleavedFrameRate_E;

  mAVaPlayerParam_X = _rPlayerParam_X;
  if (S_mGlobalVideoStandard_E == EVS::EvsPcieIoApi::eVideoStandard::VideoStd_Invalid)
  {
    // ATTENTION the board can have only one standard for all its outputs. This standard is programmed in the board
    Rts_i = mPlayerChannel_X.pIIoBoard->ConfigureGlobalStd(_rPlayerParam_X.VideoStandard_E, false);
    if (Rts_i == EVS::EvsPcieIoApi::NoError)
    {
      S_mGlobalVideoStandard_E = _rPlayerParam_X.VideoStandard_E;
    }
  }
  if (Rts_i == EVS::EvsPcieIoApi::NoError)
  {
    if (_rPlayerParam_X.VideoStandard_E != S_mGlobalVideoStandard_E)
    {
      Rts_i = EVS::EvsPcieIoApi::InvalidStandard;
    }
    else
    {
      mPlayerParam_X.VideoStandard = _rPlayerParam_X.VideoStandard_E;
      mPlayerParam_X.Hdr = EVS::EvsPcieIoApi::SDR;
      mPlayerParam_X.Color = EVS::EvsPcieIoApi::Rec709;
      mPlayerParam_X.MaxFifoSize = 15;                                           // is the maximum !!! cannot be more or will erase the previous buffer
      mPlayerParam_X.StartingType = EVS::EvsPcieIoApi::PlayerStartingTypeWaitGo; // PlayerStartingTypeWaterMark; // PlayerStartingTypeWaitGo;
      mPlayerParam_X.StartWaterMark = 3;
      mPlayerParam_X.useSessionID = true;
      mPlayerParam_X.defaultSessionID = 0;

      mPlayerParam_X.VideoParams.Fourcc = _rPlayerParam_X.Hr10bit_B ? FOURCC_V210 : FOURCC_UYVY;
      mPlayerParam_X.VideoParams.FirstField = EVS::EvsPcieIoApi::FieldTypeEven;
      mPlayerParam_X.VideoParams.bEnable = true;

      mPlayerParam_X.AudioParams.maxDrifting = 10;
      mPlayerParam_X.AudioParams.bEnable = (_rPlayerParam_X.AudioChannelMaskToPlay_U32);

      mPlayerParam_X.AncParams.bEnable = (_rPlayerParam_X.AncFileName_S != "");

      Rts_i = mPlayerChannel_X.pPcieIoBoard->RegisterGenlockFrameEvent(this);
      if (Rts_i == EVS::EvsPcieIoApi::NoError)
      {
        Rts_i = mPlayerChannel_X.pPcieIoBoard->CreateNewGenlockTask(mPlayerChannel_X.GlkTaskId_U8, mPlayerChannel_X.GlkTaskDelayInUs_U32);
        if (Rts_i == EVS::EvsPcieIoApi::NoError)
        {
          VideoFrameRate_f = GL_puBoardResourceManager->VideoStandardToFrameRate(mAVaPlayerParam_X.VideoStandard_E, InterleavedFrameRate_E);
          mPlayerChannel_X.puAudioSequencer = std::make_unique<AudioSequencer>(InterleavedFrameRate_E);
          Rts_i = Start(mPlayerParam_X.VideoStandard);
        }
      }
    }
  }
  return Rts_i;
}
int SimplePlayer::ClosePlayer(bool _FullStop_B)
{
  int Rts_i = EVS::EvsPcieIoApi::NoError;

  if (_FullStop_B)
  {
    Rts_i = Stop();
    mPlayerChannel_X.pIIoBoard->DeleteGenlockTask(mPlayerChannel_X.GlkTaskId_U8);
  }
  else
  {
    if (mPlayerChannel_X.pIPlayer)
    {
      Rts_i = mPlayerChannel_X.pIPlayer->Stop();
    }
    else
    {
      Rts_i = EVS::EvsPcieIoApi::RuntimeError;
    }
  }
  return Rts_i;
}

int SimplePlayer::Start(EVS::EvsPcieIoApi::eVideoStandard _VideoStandard_E)
{
  int Rts_i = EVS::EvsPcieIoApi::NotConfigured;

  if (mPlayerChannel_X.pIPlayer)
  {
    // Check Task is not already running
    if (mSendNewFrameThread.joinable())
    {
      Rts_i = EVS::EvsPcieIoApi::AlreadyInUse;
    }
    else
    {
      // Stop channel
      Rts_i = Stop();
      if (Rts_i == EVS::EvsPcieIoApi::NoError)
      {
        Rts_i = mPlayerChannel_X.pIPlayer->Initialize(&mPlayerParam_X);
        if (Rts_i == EVS::EvsPcieIoApi::NoError)
        {
#if 0 /// Remove ip support
  if (mPlayerChannel_X.pIIoBoard->m_boardType == BOARD_PA1021_V1)
  {
    Rts_i = mPlayerChannel_X.pIPlayer->ConfigureIpStream(&m_IPvideoStream);
    if (Rts_i != EVS::EvsPcieIoApi::NoError)
    {
      return Rts_i;
    }

    if (mPlayerParam_X.AudioParams.bEnable)
    {
      if (!m_AudioValid)
      {
        ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "Audio stream not initialize\n");
        return -1;
      }

      Rts_i = mPlayerChannel_X.pIPlayer->ConfigureIpStream(&m_IPAudioStream);
      if (Rts_i != EVS::EvsPcieIoApi::NoError)
      {
        return Rts_i;
      }
    }
  }
#endif
          mRestartCounter_U32 = 0;
          // Register Frame Emmitter observer
          Rts_i = mPlayerChannel_X.pIPlayer->RegisterFrameEmittedEvent(this);
          if (Rts_i == EVS::EvsPcieIoApi::NoError)
          {
            Rts_i = AllocAndQueueNewFrame(mAVaPlayerParam_X.NbFrameToPreload_U32);
            if (Rts_i == EVS::EvsPcieIoApi::NoError)
            {
              /*
              ADD_MESSAGE(EvsHwLGPL::SEV_INFO, ">>> --Player--------------wait 10 sec\n");
              ADD_MESSAGE(EvsHwLGPL::SEV_INFO, ">>> --Player--------------wait for long long time\n");
              Sleep(10 * 1000);
              Sleep(10000 * 1000);
              ADD_MESSAGE(EvsHwLGPL::SEV_INFO, ">>> --Player--------------end of wait 10 sec\n");
              */
              // Start engines
              Rts_i = mPlayerChannel_X.pIPlayer->Start();
              if (Rts_i == EVS::EvsPcieIoApi::NoError)
              {
                Rts_i = mPlayerChannel_X.pPcieIoBoard->RegisterGenlockTask(mPlayerChannel_X.GlkTaskId_U8, this);
                if (Rts_i == EVS::EvsPcieIoApi::NoError)
                {
                  Rts_i = StartRecycleFrameSentBufferThread();
                  if (Rts_i == EVS::EvsPcieIoApi::NoError)
                  {
                    Rts_i = StartSendNewFrameThread();
                    if (Rts_i == EVS::EvsPcieIoApi::NoError)
                    {
                      mPlayerInitialized_B = true;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return Rts_i;
}

void SimplePlayer::RestartPlayer()
{
  ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "## Restart: Start process\n");
  ClosePlayer(true);
  OpenPlayer(mAVaPlayerParam_X);
  #if 0
  mPlayerChannel_X.pIPlayer->Stop();
  mPlayerStarted_B = false; // Used in GenlockTask to evaluate if StartPlayerGo() needs to be called

  mPlayerChannel_X.pIIoBoard->UnregisterGenlockFrameEvent(this);
  mPlayerChannel_X.pIIoBoard->UnregisterGenlockTask(mPlayerChannel_X.GlkTaskId_U8, this);
  mPlayerChannel_X.pIPlayer->UnregisterFrameEmittedEvent(this);
  mPlayerChannel_X.pIPlayer->Close();

  const bool skip_wait = false;
  //initialize_player(skip_wait); // check_standards, call player->Initialize(...) and register events and tasks again

  mPlayerChannel_X.pIPlayer->Start();

  //preload_frames(); // Fills the app side buffer queue,
                    // while ensuring that all frames that where sent but not emitted, are re-sent.
                    // Once the queue has data, frames start to be sent again 
  #endif
}

int SimplePlayer::Stop()
{
  int Rts_i = EVS::EvsPcieIoApi::NoError;

  mPlayerInitialized_B = false;
  mPlayerStarted_B = false;
  mPlayerGlkTaskReceived_B = false;
  mPlayerGlkReceived_B = false;

  //  unregister observer
  if (mPlayerChannel_X.pIIoBoard)
  {
    mPlayerChannel_X.pIIoBoard->UnregisterGenlockFrameEvent(this);
    mPlayerChannel_X.pIIoBoard->UnregisterGenlockTask(mPlayerChannel_X.GlkTaskId_U8, this);
  }

  // Stop engines
  if (mPlayerChannel_X.pIPlayer)
  {
    Rts_i |= mPlayerChannel_X.pIPlayer->Stop();
  }

  Rts_i |= StopSendNewFrameThread();
  Rts_i |= StopRecycleFrameSentBufferThread();

  //  Wait all is deleted
  usleep(30000);
  Rts_i |= FreeQueuedFrame();

  // unregister observer
  if (mPlayerChannel_X.pIPlayer)
  {
    Rts_i |= mPlayerChannel_X.pIPlayer->UnregisterFrameEmittedEvent(this);
  }

  // Close player
  if (mPlayerChannel_X.pIPlayer)
  {
    mPlayerChannel_X.pIPlayer->Close();
  }

  return Rts_i;
}

EVS::EvsPcieIoApi::eVideoStandard SimplePlayer::GetVideoStandard()
{
  return mPlayerParam_X.VideoStandard;
}

int SimplePlayer::SwitchToPatternGenerator(uint32_t _Height_U32, uint32_t _Width_U32)
{
  uint32_t Reg_U32 = (_Width_U32 << 16) + (_Height_U32);

  mPlayerChannel_X.pPcieIoBoard->Write32(0x4100c, Reg_U32); // write width x height
  mPlayerChannel_X.pPcieIoBoard->Write32(0x41008, 0x12);    // for patgen BIT(4) = devalid

  return EVS::EvsPcieIoApi::NoError;
}
int SimplePlayer::SwitchToDMA()
{
  mPlayerChannel_X.pPcieIoBoard->Write32(0x41008, 0x11);
  return EVS::EvsPcieIoApi::NoError;
}

int SimplePlayer::AllocAndQueueNewFrame(uint32_t _NbFrameToPreload_U32)
{
  int Rts_i = EVS::EvsPcieIoApi::InvalidArgument;
  uint32_t i_U32;
  EVS::EvsPcieIoApi::PPUT_FRAME_PARAMS ppPutFrameParam_X[2];
  bool IsProgressive_B;

  if (((_NbFrameToPreload_U32 & 1) == 0)      // Even number for odd/even field in not progressive mode
      && ((_NbFrameToPreload_U32 % 10) == 0)) // multiple of 10 for audio sequence periodicity in not progressive mode (ex: 59.94: 801 801 801 801 800: 5 size * 2 fields)
  {
    IsProgressive_B = EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::IsProgressive(mAVaPlayerParam_X.VideoStandard_E);
    if (!IsProgressive_B)
    {
      mAVaPlayerParam_X.NbFrameToPreload_U32 *= 2;
    }

    Rts_i = EVS::EvsPcieIoApi::NoMoreSpace;
    // Allocate buffer pool
    if ((mPlayerItemPoolSize_U32 != _NbFrameToPreload_U32) || (!mpPlayerItemPool))
    {
      if (mpPlayerItemPool)
      {
        delete[] mpPlayerItemPool;
        mpPlayerItemPool = nullptr;
      }
      mPlayerItemPoolSize_U32 = _NbFrameToPreload_U32;
      mpPlayerItemPool = new PlayerItem[mPlayerItemPoolSize_U32];
    }

    // Allocate all the buffer
    if (mpPlayerItemPool)
    {
      Rts_i = EVS::EvsPcieIoApi::NoError;
      mPlayerChannel_X.VideoFileOffset_U32 = 0;
      mPlayerChannel_X.AudioFileOffset_U32 = 0;
      mPlayerChannel_X.AncFileOffset_U32 = 0;
      for (i_U32 = 0; i_U32 < mPlayerItemPoolSize_U32; i_U32 += 2)
      {
        ppPutFrameParam_X[0] = &mpPlayerItemPool[i_U32].FrameData_X.sOriginalReq;
        ppPutFrameParam_X[1] = &mpPlayerItemPool[i_U32 + 1].FrameData_X.sOriginalReq;
        Rts_i = AllocateDefaultSendingFrame(i_U32, ppPutFrameParam_X);
        if (Rts_i != EVS::EvsPcieIoApi::NoError)
        {
          break;
        }

        // init debug structures
        mpPlayerItemPool[i_U32].FrameDebug_X = {};
        mpPlayerItemPool[i_U32].AudioDebug_X = {};
        mpPlayerItemPool[i_U32].AncDebug_X = {};

        // push it int waitable fifo

        mFifoFrameToRecycle.Insert(&mpPlayerItemPool[i_U32].FifoRefCollection, false);
        mFifoFrameToRecycle.Insert(&mpPlayerItemPool[i_U32 + 1].FifoRefCollection, false);
      }
    }
  }
  return Rts_i;
}
int SimplePlayer::FreeQueuedFrame()
{
  PLIST_ITEM pList_X;
  uint32_t i_U32;

  while (mFifoFrameToRecycle.CheckIfItemAvail())
  {
    if (mFifoFrameToRecycle.WaitItem(1, &pList_X) != 0)
    {
      break;
    }
  }

  while (mFifoFrameToSend.CheckIfItemAvail())
  {
    if (mFifoFrameToSend.WaitItem(1, &pList_X) != 0)
    {
      break;
    }
  }

  if (mpPlayerItemPool)
  {
    for (i_U32 = 0; i_U32 < mPlayerItemPoolSize_U32; i_U32++)
    {
      FreeSendingFrame(&mpPlayerItemPool[i_U32].FrameData_X.sOriginalReq);
    }
    delete[] mpPlayerItemPool;
    mpPlayerItemPool = nullptr;
    mPlayerItemPoolSize_U32 = 0;
  }

  return EVS::EvsPcieIoApi::NoError;
}


  void SimplePlayer::SendNewFrameThread()
{
  PPlayerItem pPlayerItem;
  PLIST_ITEM pListItem_X;
  uint32_t FifoLevel_U32, LastFifoLevel_U32;

  int Sts_i = SetThreadRtParam("SndNewFrm", PRIO_HIGH, 0);
  assert(Sts_i == 0);

  LastFifoLevel_U32 = 0xFFFFFFFF;
  while (!mStopSendNewFrameThread_B)
  {
    if (mPlayerChannel_X.pIPlayer->GetPlayerState() != EVS::EvsPcieIoApi::CHANNEL_STATE::CHANNEL_RUNNING)
    {
//      ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "## Restart Player: State %d\n", mPlayerChannel_X.pIPlayer->GetPlayerState());
//      RestartPlayer();
      usleep(500 * 1000);
      continue;
    }

    FifoLevel_U32 = mPlayerChannel_X.pIPlayer->GetCurrentFifoLevel();
    if (FifoLevel_U32 != LastFifoLevel_U32)
    {
      ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "New Fifo level is %d/%d\n", FifoLevel_U32, (uint32_t)mPlayerParam_X.MaxFifoSize);
      LastFifoLevel_U32 = FifoLevel_U32;
    }
    if (FifoLevel_U32 >= (uint32_t)mPlayerParam_X.MaxFifoSize)
    {
//      ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "HIGH Fifo level is %d/%d\n", FifoLevel_U32, (uint32_t)mPlayerParam_X.MaxFifoSize);
      //      usleep(500 * 1000);
      continue;
    }
    if (mFifoFrameToRecycle.WaitItem(100, &pListItem_X, &mStopSendNewFrameThread_B) != 0)
    {
//      ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "WaitItem\n");
      continue;
    }

    if (!pListItem_X)
    {
      ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "%s : pointer null\n", __FUNCTION__);
    }

    pPlayerItem = container_of(pListItem_X, PlayerItem, FifoRefCollection);
    pPlayerItem->TickAppInsertion_U64 = EvsHwLGPL::GetTick();
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "## Pop %p from push fifo (level %d)\n", pListItem_X, mPlayerChannel_X.pIPlayer->GetCurrentFifoLevel());

    Sts_i = ((EVS::EvsPcieIoApi::CPA1022Player *)mPlayerChannel_X.pIPlayer)->SendNewFrame(&pPlayerItem->FrameData_X.sOriginalReq, &pPlayerItem->FrameDebug_X, &pPlayerItem->AudioDebug_X, &pPlayerItem->AncDebug_X);
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "TaskSendNewFrame: Sts %d Id %d Ses %d/%d Ctx %p V %ld:%p A %d %ld:%p a %ld:%p\n", Sts_i, pPlayerItem->FrameData_X.sOriginalReq.Id, pPlayerItem->FrameData_X.sOriginalReq.CurrentSessionID, pPlayerItem->FrameData_X.sOriginalReq.FrameSessionID,
                pPlayerItem->FrameData_X.sOriginalReq.Context, pPlayerItem->FrameData_X.sOriginalReq.Video.Buffer.BufferSize, pPlayerItem->FrameData_X.sOriginalReq.Video.Buffer.pBuffer,
                pPlayerItem->FrameData_X.sOriginalReq.Audio[6].NbrSamples,
                pPlayerItem->FrameData_X.sOriginalReq.Audio[6].Buffer.BufferSize, pPlayerItem->FrameData_X.sOriginalReq.Audio[6].Buffer.pBuffer,
                pPlayerItem->FrameData_X.sOriginalReq.Anc.Buffer.BufferSize, pPlayerItem->FrameData_X.sOriginalReq.Anc.Buffer.pBuffer);
    if (Sts_i == EVS::EvsPcieIoApi::InvalidSession)
    {
      ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "TaskSendNewFrame: Session id error !!\n");
    }
    if (Sts_i < 0)
    {
      mFifoFrameToRecycle.Insert(pListItem_X, true);
    }
  }
}

int SimplePlayer::StartSendNewFrameThread()
{
  int Rts_i = EVS::EvsPcieIoApi::NotConfigured;
  // Check Inputs
  if (mPlayerChannel_X.pIPlayer)
  {
    // Stop channel
    Rts_i = StopSendNewFrameThread();
    if (Rts_i == EVS::EvsPcieIoApi::NoError)
    {
      // Start pushing task
      mStopSendNewFrameThread_B = false;
      mFrameCounter_U32 = 0;
      try
      {
        mSendNewFrameThread = std::thread(&SimplePlayer::SendNewFrameThread, this);
      }
      catch (std::exception & /*e*/)
      {
        Rts_i = EVS::EvsPcieIoApi::RuntimeError;
      }
    }
  }
  return Rts_i;
}
int SimplePlayer::StopSendNewFrameThread()
{
  int Rts_i = EVS::EvsPcieIoApi::NoError;

  // Stop Thread
  mStopSendNewFrameThread_B = true;
  if (mSendNewFrameThread.joinable())
  {
    mSendNewFrameThread.join();
  }
  return Rts_i;
}
void SimplePlayer::RecycleFrameSentBufferThread()
{
  int Sts_i = SetThreadRtParam("RclFrmSnt", PRIO_HIGH, 0);
  assert(Sts_i == 0);

  PLIST_ITEM pListItem_X;

  while (!mStopRecycleFrameSentBufferThread_B)
  {
    if (mFifoFrameToSend.WaitItem(100, &pListItem_X, &mStopRecycleFrameSentBufferThread_B) != 0)
    {
      continue;
    }

    if (!pListItem_X)
    {
      ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "%s : pointer null", __FUNCTION__);
    }

    // reinsert in next fifo
    mFifoFrameToRecycle.Insert(pListItem_X, true);
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "## RePush %p in push fifo (level %d)\n", pListItem_X, mPlayerChannel_X.pIPlayer->GetCurrentFifoLevel());
  }
}

int SimplePlayer::StartRecycleFrameSentBufferThread()
{
  int Rts_i = EVS::EvsPcieIoApi::AlreadyInUse;
  if (!mRecycleFrameSentBufferThread.joinable())
  {
    /*Init variables */
    mStopRecycleFrameSentBufferThread_B = false;
    Rts_i = EVS::EvsPcieIoApi::NoError;
    /*Start thread */
    try
    {
      mRecycleFrameSentBufferThread = std::thread(&SimplePlayer::RecycleFrameSentBufferThread, this);
    }
    catch (std::exception &e)
    {
      ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "%s : cannot start task :%s\n", __FUNCTION__, e.what());
      Rts_i = EVS::EvsPcieIoApi::RuntimeError;
    }
  }
  return Rts_i;
}
int SimplePlayer::StopRecycleFrameSentBufferThread()
{
  mStopRecycleFrameSentBufferThread_B = true;

  if (mRecycleFrameSentBufferThread.joinable())
  {
    mRecycleFrameSentBufferThread.join();
  }

  return EVS::EvsPcieIoApi::NoError;
}
uint32_t SimplePlayer::GetRestartCounter()
{
  return mRestartCounter_U32;
}
uint32_t SimplePlayer::GetUnderflowCounter()
{
  return mPlayerChannel_X.pIPlayer->GetUnderflowCounter();
}
//  Observers
int SimplePlayer::NewFrameEmitted(EVS::EvsPcieIoApi::FRAME_STATUS _Status_E, EVS::EvsPcieIoApi::PFRAME_EMITTED_INFO _pFrameEmittedInfo_X)
{
  int Rts_i = EVS::EvsPcieIoApi::RuntimeError;
  PPlayerItem pPlayerItem;

  // Increment counter
  mFrameCounter_U32++;
  if (!_pFrameEmittedInfo_X)
  {
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "[NOTIF] NewFrameEmitted Status %u\n", _Status_E);
  }
  else
  {
    Rts_i = EVS::EvsPcieIoApi::NoError;
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "[NOTIF] NewFrameEmitted %u Status %u Id %x %x %x Size %x %x %x\n", mFrameCounter_U32, _Status_E, _pFrameEmittedInfo_X->sOriginalReq.Id, _pFrameEmittedInfo_X->sOriginalReq.FrameSessionID, _pFrameEmittedInfo_X->sOriginalReq.CurrentSessionID,
                _pFrameEmittedInfo_X->sOriginalReq.Video.Buffer.BufferSize,
                _pFrameEmittedInfo_X->sOriginalReq.Audio[6].Buffer.BufferSize,
                _pFrameEmittedInfo_X->sOriginalReq.Anc.Buffer.BufferSize);

    if ((_Status_E != EVS::EvsPcieIoApi::FRAME_STATUS::FRAME_STATUS_OK) && (_Status_E != EVS::EvsPcieIoApi::FRAME_STATUS::FRAME_STATUS_CANCELLED))
    {
      ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "Frame %u has invalid status (%u)\n", mFrameCounter_U32, _Status_E);
    }

    if (_pFrameEmittedInfo_X->sOriginalReq.Id >= mPlayerItemPoolSize_U32)
    {
      // error
      ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "frame %u has invalid id=%u, max=%u\n", mFrameCounter_U32, _pFrameEmittedInfo_X->sOriginalReq.Id, mPlayerItemPoolSize_U32);
      Rts_i = 0;
    }
    else
    {
      pPlayerItem = &mpPlayerItemPool[_pFrameEmittedInfo_X->sOriginalReq.Id];
      pPlayerItem->TickAppInterrupt_U64 = EvsHwLGPL::GetTick();

      // update info
      pPlayerItem->FrameStatus_i = _Status_E;
      pPlayerItem->FrameData_X = *_pFrameEmittedInfo_X;

      // Save fifo levels
      mPlayerCounter_X.Fifo.NbItemInApiFifo_U32 = pPlayerItem->FrameDebug_X.ApiFifoNbrItems;
      mPlayerCounter_X.Fifo.DrvHiresFlying_U32 = pPlayerItem->FrameDebug_X.HiresDrvFifoFlying;
      mPlayerCounter_X.Fifo.DrvHiresWaiting_U32 = pPlayerItem->FrameDebug_X.HiresDrvFifoWaiting;
      mPlayerCounter_X.Fifo.DrvAudio_U32 = pPlayerItem->AudioDebug_X.DrvFifo;
      mPlayerCounter_X.Fifo.DrvAnc_U32 = pPlayerItem->AncDebug_X.DrvFifo;

      // Compute delta
      mPlayerCounter_X.TimeMeasurement.DeltaAppDriver_U64 = pPlayerItem->FrameDebug_X.HiResCounters[0] - pPlayerItem->TickAppInsertion_U64;
      mPlayerCounter_X.TimeMeasurement.DeltaDriverWaiting_U64 = pPlayerItem->FrameDebug_X.HiResCounters[2] - pPlayerItem->FrameDebug_X.HiResCounters[0];
      mPlayerCounter_X.TimeMeasurement.DeltaWaitingFlying_U64 = pPlayerItem->FrameDebug_X.HiResCounters[3] - pPlayerItem->FrameDebug_X.HiResCounters[2];
      mPlayerCounter_X.TimeMeasurement.DeltaAppFlying_U64 = pPlayerItem->FrameDebug_X.HiResCounters[3] - pPlayerItem->TickAppInsertion_U64;

      mPlayerCounter_X.TimeMeasurement.DeltaInterruptTask_U64 = pPlayerItem->FrameDebug_X.HiResCounters[5] - pPlayerItem->FrameDebug_X.HiResCounters[4];
      mPlayerCounter_X.TimeMeasurement.DeltaTaskCompletion_U64 = pPlayerItem->FrameDebug_X.HiResCounters[1] - pPlayerItem->FrameDebug_X.HiResCounters[5];
      mPlayerCounter_X.TimeMeasurement.DeltaCompletionApp_U64 = pPlayerItem->TickAppInterrupt_U64 - pPlayerItem->FrameDebug_X.HiResCounters[1];
      mPlayerCounter_X.TimeMeasurement.DeltaInterruptApp_U64 = pPlayerItem->TickAppInterrupt_U64 - pPlayerItem->FrameDebug_X.HiResCounters[4];

      ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "ApiFifo %lu DrvFly %lu DrvVWait %d DrvA %lu Drva %lu\n", mPlayerCounter_X.Fifo.NbItemInApiFifo_U32, mPlayerCounter_X.Fifo.DrvHiresFlying_U32,
                  mPlayerCounter_X.Fifo.DrvHiresWaiting_U32, mPlayerCounter_X.Fifo.DrvAudio_U32, mPlayerCounter_X.Fifo.DrvAnc_U32);
      ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "Delta AppDrv %lu WaitFly %lu DrvWait %lu AppFly %lu IntTsk %lu TskComp %lu CompApp %lu IntApp %lu\n", 
                  (uint32_t)mPlayerCounter_X.TimeMeasurement.DeltaAppDriver_U64,
                  (uint32_t)mPlayerCounter_X.TimeMeasurement.DeltaWaitingFlying_U64,
                  (uint32_t)mPlayerCounter_X.TimeMeasurement.DeltaDriverWaiting_U64,
                  (uint32_t)mPlayerCounter_X.TimeMeasurement.DeltaAppFlying_U64,
                  (uint32_t)mPlayerCounter_X.TimeMeasurement.DeltaInterruptTask_U64,
                  (uint32_t)mPlayerCounter_X.TimeMeasurement.DeltaTaskCompletion_U64,
                  (uint32_t)mPlayerCounter_X.TimeMeasurement.DeltaCompletionApp_U64,
                  (uint32_t)mPlayerCounter_X.TimeMeasurement.DeltaInterruptApp_U64);

      // put poped frame into queue
      mFifoFrameToSend.Insert(&pPlayerItem->FifoRefCollection, true);
    }
  }
  return Rts_i;
}

void SimplePlayer::GenlockFrameEvent(EVS::EvsPcieIoApi::PGLK_DATA pglkData, EVS::EvsPcieIoApi::PLTC_DATA ltcData)
{
  mPlayerGlkReceived_B = true;
  //   StartPlayerGo();
  // if (!mPlayerStarted_B)
  {
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "GenlockFrameEvent Frm %lld:%d->%d Gnlk %u\n", pglkData->FrameNb, pglkData->currField, pglkData->nextField, pglkData->genlockTimeRef);
  }
}

void SimplePlayer::GenlockTask(uint32_t taskID, EVS::EvsPcieIoApi::PGLK_DATA plastGlkData)
{
  bool Sts_B = false;
  //uint32_t FifoLevel_U32;

  mPlayerGlkTaskReceived_B = true;
#if defined(DEBUG_DEFFEDED_HANDLER)
  uint64_t Delta_U64, DeltaTaskTime_U64;
  static uint64_t S_LastGenlockTaskTime_U64 = 0;
  DeltaTaskTime_U64 = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count() - S_LastGenlockTaskTime_U64;
  Delta_U64 = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count() - GL_LastGenlockTime_U64;
  printf("%u %u GNLK:6 Callback delta %lld uS DeltaCallback %lld uS\n", GetTickCount(), GetCurrentThreadId(), Delta_U64, DeltaTaskTime_U64);
  S_LastGenlockTaskTime_U64 = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
#endif
  ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "GenlockTask: PlayerStarted %d NxtFld %d/%d Fifo %d\n", mPlayerStarted_B, plastGlkData->nextField, (uint8_t)mPlayerParam_X.VideoParams.FirstField, mPlayerChannel_X.pIPlayer->GetCurrentFifoLevel());

  if (plastGlkData->nextField != (uint8_t)mPlayerParam_X.VideoParams.FirstField) // sync play with the right field. next field must be the same as first field
  {
  }
  else if (!mPlayerStarted_B)
  {
    Sts_B = StartPlayerGo();
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "GenlockTask: PlayerStarted %d Sts %d Field %d Fifo %d\n", mPlayerStarted_B, Sts_B, plastGlkData->nextField, mPlayerChannel_X.pIPlayer->GetCurrentFifoLevel());
  }
  /*
  else if ((!mPlayerStarted_B) && (mPlayerChannel_X.pIPlayer->GetPlayerState() == EVS::EvsPcieIoApi::CHANNEL_STATE::CHANNEL_RUNNING))
  {
    FifoLevel_U32 = mPlayerChannel_X.pIPlayer->GetCurrentFifoLevel();
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "## Restart: GenlockTask: State %d/%d Fifo %d\n", mPlayerChannel_X.pIPlayer->GetPlayerState(), EVS::EvsPcieIoApi::CHANNEL_STATE::CHANNEL_RUNNING, FifoLevel_U32);
    if (FifoLevel_U32 > 3) // start_water_mark))
    {
      StartPlayerGo(); // If successful, sets  player_started = true;
    }
  }
  */
#if 0
  else if (!StartPlayerGo())
  {
  }
  else if (mPlayerChannel_X.pIIoBoard)
  {
//Remove genlock task if ok
    mPlayerChannel_X.pPcieIoBoard->UnregisterGenlockTask(mPlayerChannel_X.GlkTaskId_U8, this);
    //    mPlayerChannel_X.pPcieIoBoard->DeleteGenlockTask(mPlayerChannel_X.GlkTaskId_U8);
  }
#endif
}

bool SimplePlayer::StartPlayerGo()
{
  bool Rts_B = false;
  uint64_t TimeStartFn_U64, TimeEndFn_U64;
  int Sts_i;

  if (mPlayerChannel_X.pIPlayer)
  {
    TimeStartFn_U64 = EvsHwLGPL::GetTick();

    /*Check conditions*/
    if (mPlayerStarted_B || (mPlayerChannel_X.pIPlayer->GetCurrentFifoLevel() < 2))
    {
    }
    else
    {
      Sts_i = mPlayerChannel_X.pIPlayer->GoPlayer(&mPlayerCounter_X.TimeMeasurement.TimeRefGo_U32);
      if (Sts_i != EVS::EvsPcieIoApi::Errno::NoError)
      {
        ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "%s : fail to start player (Rts_i =%i)\n", __FUNCTION__, Sts_i);
      }
      else
      {
        if (mPlayerChannel_X.pIIoBoard)
        {
          mPlayerCounter_X.TimeMeasurement.TimeRefGoGenlock_U32 = mPlayerChannel_X.pPcieIoBoard->GetLastGenlockTimeRef();
        }

        TimeEndFn_U64 = EvsHwLGPL::GetTick();

        mPlayerCounter_X.TimeMeasurement.DeltaGoFnCall_U64 = TimeEndFn_U64 - TimeStartFn_U64;

        // Notify player is stared
        mPlayerStarted_B = true;
        Rts_B = true;
      }
    }
  }

  return Rts_B;
}

int SimplePlayer::AllocateDefaultSendingFrame(uint32_t _FrameId_U32, EVS::EvsPcieIoApi::PPUT_FRAME_PARAMS (&_rppPutFrameParam_X)[2])
{
  int Rts_i;
  ssize_t BufferSize = 0;
  int32_t *pAudioData_S32;
  uint32_t i_U32, j_U32, k_U32, Mask_U32, BufferAlignment_U32 = 4096, NbAudioChannel_U32, NbAudioSample_U32;
  uint32_t VideoImageSize_U32, VideoNbLine_U32, VideoLineWidthInPixel_U32, VideoLineSizeInByte_U32;
  uint32_t LoResBufferSize_U32 = 2 * 1024 * 1024;
  uint32_t AudioBufferSize_U32; // = AUDIO_BUFFER_SIZE;
  uint32_t AncBufferSize_U32;   // ANCILLARY_BUFFER_SIZE;
  EVS::EvsPcieIoApi::PAV_USER_BUFFER_PROPERTIES pAvUserBufferProperty_X;
  bool IsProgressive_B, Eof_B;
  uint8_t *pMergedVideoField_U8, pAncBuffer_U8[ANCILLARY_BUFFER_SIZE];
  uint8_t NbAncItem_U8;
  size_t AncSize, FileSize;

  if ((!_rppPutFrameParam_X[0]) || (!_rppPutFrameParam_X[1]))
  {
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "%s : invalid arguments, pointers nulls\n", __FUNCTION__);
    Rts_i = EVS::EvsPcieIoApi::InvalidArgument;
  }
  else
  {
    Rts_i = EVS::EvsPcieIoApi::NoError;
    VideoNbLine_U32 = EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::GetNbLines(mAVaPlayerParam_X.VideoStandard_E);
    VideoLineWidthInPixel_U32 = EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::GetLineWidth(mAVaPlayerParam_X.VideoStandard_E);
    VideoImageSize_U32 = (uint32_t)EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::ComputeImageSize(mAVaPlayerParam_X.VideoStandard_E, mPlayerParam_X.VideoParams.Fourcc, VideoLineWidthInPixel_U32, VideoNbLine_U32, false);
    VideoLineSizeInByte_U32 = (uint32_t)EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::ComputeImageSize(mAVaPlayerParam_X.VideoStandard_E, mPlayerParam_X.VideoParams.Fourcc, VideoLineWidthInPixel_U32, 1, true);

    IsProgressive_B = EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::IsProgressive(mAVaPlayerParam_X.VideoStandard_E);
    if (!IsProgressive_B)
    {
      VideoImageSize_U32 = VideoImageSize_U32 * 2;
    }
    Rts_i = GL_puBoardResourceManager->AllocateBuffer((void **)&pMergedVideoField_U8, VideoImageSize_U32, BufferAlignment_U32);
    if (Rts_i == EVS::EvsPcieIoApi::NoError)
    {
      NbAudioChannel_U32 = 0;
      for (Mask_U32 = 1, i_U32 = 0; i_U32 < AUDIO_NBRCH; i_U32++, Mask_U32 <<= 1)
      {
        if (mAVaPlayerParam_X.AudioChannelMaskToPlay_U32 & Mask_U32)
        {
          NbAudioChannel_U32++;
        }
      }

      for (i_U32 = 0; i_U32 < 2; i_U32++)
      {
        /* global transaction parameters */
        _rppPutFrameParam_X[i_U32]->Id = _FrameId_U32 + i_U32;
        _rppPutFrameParam_X[i_U32]->Context = this;
        _rppPutFrameParam_X[i_U32]->FrameSessionID = 0;
        _rppPutFrameParam_X[i_U32]->CurrentSessionID = 0;

        pAvUserBufferProperty_X = &_rppPutFrameParam_X[i_U32]->Video.Buffer;
        pAvUserBufferProperty_X->_keepSize = 0;
        pAvUserBufferProperty_X->_skipSize = 0;
        pAvUserBufferProperty_X->_startOffset = 0;
        if (mPlayerParam_X.VideoParams.bEnable)
        {
          pAvUserBufferProperty_X->BufferSize = IsProgressive_B ? VideoImageSize_U32 : (VideoImageSize_U32 / 2);
          Rts_i = GL_puBoardResourceManager->AllocateBuffer(&pAvUserBufferProperty_X->pBuffer, pAvUserBufferProperty_X->BufferSize, BufferAlignment_U32);
          if (Rts_i != EVS::EvsPcieIoApi::NoError)
          {
            ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "%s : cannot allocate video buffers\n", __FUNCTION__);
          }
          else
          {
            if (IsProgressive_B)
            {
              if (mAVaPlayerParam_X.HrFileName_S.compare("") != 0)
              {
                if (GL_puBoardResourceManager->LoadFile(pAvUserBufferProperty_X->BufferSize, pAvUserBufferProperty_X->pBuffer, mPlayerChannel_X.VideoFileOffset_U32, (mAVaPlayerParam_X.BaseLoadDir_S + mAVaPlayerParam_X.HrFileName_S).c_str(), FileSize, Eof_B) <= 0)
                {
                  Rts_i = EVS::EvsPcieIoApi::RuntimeError;
                }
                mPlayerChannel_X.VideoFileOffset_U32 += VideoImageSize_U32;
                if ((Eof_B) || (mPlayerChannel_X.VideoFileOffset_U32 >= FileSize))
                {
                  mPlayerChannel_X.VideoFileOffset_U32 = 0;
                }
              }
            }
            else
            {
              if (i_U32 == 0)
              {
                if (mAVaPlayerParam_X.HrFileName_S.compare("") != 0)
                {
                  if (GL_puBoardResourceManager->LoadFile(VideoImageSize_U32, pMergedVideoField_U8, mPlayerChannel_X.VideoFileOffset_U32, (mAVaPlayerParam_X.BaseLoadDir_S + mAVaPlayerParam_X.HrFileName_S).c_str(), FileSize, Eof_B) <= 0)
                  {
                    Rts_i = EVS::EvsPcieIoApi::RuntimeError;
                  }
                  mPlayerChannel_X.VideoFileOffset_U32 += VideoImageSize_U32;
                  if ((Eof_B) || (mPlayerChannel_X.VideoFileOffset_U32 >= FileSize))
                  {
                    mPlayerChannel_X.VideoFileOffset_U32 = 0;
                  }
                }
              }
              if (Rts_i == EVS::EvsPcieIoApi::NoError)
              {
                GL_puBoardResourceManager->DeInterleaveMergedField(VideoNbLine_U32 / 2, VideoLineSizeInByte_U32, pMergedVideoField_U8, (i_U32 == 0) ? true : false, pAvUserBufferProperty_X->pBuffer);
              }
            }
          }
        }
        else
        {
          pAvUserBufferProperty_X->pBuffer = nullptr;
          pAvUserBufferProperty_X->BufferSize = 0;
          Rts_i = EVS::EvsPcieIoApi::InvalidState;
        }

        if (Rts_i == EVS::EvsPcieIoApi::NoError)
        {
          NbAudioSample_U32 = mPlayerChannel_X.puAudioSequencer->Next();
          AudioBufferSize_U32 = NbAudioSample_U32 * sizeof(uint32_t);
          for (Mask_U32 = 1, j_U32 = 0; j_U32 < AUDIO_NBRCH; j_U32++, Mask_U32 <<= 1)
          {
            pAvUserBufferProperty_X = &_rppPutFrameParam_X[i_U32]->Audio[j_U32].Buffer;
            pAvUserBufferProperty_X->_keepSize = 0;
            pAvUserBufferProperty_X->_skipSize = 0;
            pAvUserBufferProperty_X->_startOffset = 0;
            if (mPlayerParam_X.AudioParams.bEnable)
            {
              //  Allocate Audio Buffer
              if (mAVaPlayerParam_X.AudioChannelMaskToPlay_U32 & Mask_U32)
              {
                pAvUserBufferProperty_X->BufferSize = AudioBufferSize_U32;
                Rts_i = GL_puBoardResourceManager->AllocateBuffer(&pAvUserBufferProperty_X->pBuffer, pAvUserBufferProperty_X->BufferSize, BufferAlignment_U32);
                if (Rts_i != EVS::EvsPcieIoApi::NoError)
                {
                  ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "%s : cannot allocate audio buffers\n", __FUNCTION__);
                  break;
                }
                else
                {
                  if (mAVaPlayerParam_X.pAudioFileName_S[j_U32].compare("") != 0)
                  {
                    if (GL_puBoardResourceManager->LoadFile(pAvUserBufferProperty_X->BufferSize, pAvUserBufferProperty_X->pBuffer, mPlayerChannel_X.AudioFileOffset_U32, (mAVaPlayerParam_X.BaseLoadDir_S + mAVaPlayerParam_X.pAudioFileName_S[j_U32]).c_str(), FileSize, Eof_B) <= 0)
                    {
                      Rts_i = EVS::EvsPcieIoApi::RuntimeError;
                      break;
                    }
                    mPlayerChannel_X.AudioFileOffset_U32 += AudioBufferSize_U32;
                    if ((Eof_B) || (mPlayerChannel_X.AudioFileOffset_U32 >= FileSize))
                    {
                      mPlayerChannel_X.AudioFileOffset_U32 = 0;
                    }
                    if (mAVaPlayerParam_X.AudioAttenuator_U32)
                    {
                      pAudioData_S32 = (int32_t *)pAvUserBufferProperty_X->pBuffer;
                      for (k_U32 = 0; k_U32 < NbAudioSample_U32; k_U32++, pAudioData_S32++)
                      {
                        *pAudioData_S32 = ((*pAudioData_S32) / mAVaPlayerParam_X.AudioAttenuator_U32);
                      }
                    }
                    _rppPutFrameParam_X[i_U32]->Audio[j_U32].NbrSamples = NbAudioSample_U32;
                    _rppPutFrameParam_X[i_U32]->Audio[j_U32].Level = 0;
                  }
                  else
                  {
                    memset(&_rppPutFrameParam_X[i_U32]->Audio[j_U32].Buffer.pBuffer, (int)pAvUserBufferProperty_X->BufferSize, 0);
                    _rppPutFrameParam_X[i_U32]->Audio[j_U32].NbrSamples = 0;
                    _rppPutFrameParam_X[i_U32]->Audio[j_U32].Level = 0;
                  }
                }
              }
              else
              {
                pAvUserBufferProperty_X->BufferSize = 0;
                pAvUserBufferProperty_X->pBuffer = nullptr;
                _rppPutFrameParam_X[i_U32]->Audio[j_U32].NbrSamples = 0;
                _rppPutFrameParam_X[i_U32]->Audio[j_U32].Level = 0;
              }
            }
            else
            {
              pAvUserBufferProperty_X->BufferSize = 0;
              pAvUserBufferProperty_X->pBuffer = nullptr;
              _rppPutFrameParam_X[i_U32]->Audio[j_U32].NbrSamples = 0;
              _rppPutFrameParam_X[i_U32]->Audio[j_U32].Level = 0;
            }
          }
        }

        if (Rts_i == EVS::EvsPcieIoApi::NoError)
        {
          // Allocate Ancillary Buffer
          pAvUserBufferProperty_X = &_rppPutFrameParam_X[i_U32]->Anc.Buffer;
          pAvUserBufferProperty_X->_keepSize = 0;
          pAvUserBufferProperty_X->_skipSize = 0;
          pAvUserBufferProperty_X->_startOffset = 0;

          if (mPlayerParam_X.AncParams.bEnable)
          {
            AncBufferSize_U32 = ANCILLARY_BUFFER_SIZE;
            if (mAVaPlayerParam_X.AncFileName_S.compare("") != 0)
            {
              BufferSize = GL_puBoardResourceManager->LoadFile(AncBufferSize_U32, pAncBuffer_U8, mPlayerChannel_X.AncFileOffset_U32, (mAVaPlayerParam_X.BaseLoadDir_S + mAVaPlayerParam_X.AncFileName_S).c_str(), FileSize, Eof_B);
              if (BufferSize > 0) // if buffer size is not an error code
              {
                Rts_i = EVS::EvsPcieIoApi::CAncillaryParser::GetSizeOfAnc(pAncBuffer_U8, BufferSize, &NbAncItem_U8, &AncSize);
                if (Rts_i == EVS::EvsPcieIoApi::NoError)
                {
                  AncBufferSize_U32 = (uint32_t)AncSize;
                }
              }
              else
              {
                Rts_i = EVS::EvsPcieIoApi::RuntimeError;
                break;
              }
            }
            if (Rts_i == EVS::EvsPcieIoApi::NoError)
            {
              pAvUserBufferProperty_X->BufferSize = AncBufferSize_U32;
              Rts_i = GL_puBoardResourceManager->AllocateBuffer(&pAvUserBufferProperty_X->pBuffer, pAvUserBufferProperty_X->BufferSize, BufferAlignment_U32);
              if (Rts_i != EVS::EvsPcieIoApi::NoError)
              {
                ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "%s : cannot allocate ancillary buffers\n", __FUNCTION__);
              }
              else
              {
                if (mAVaPlayerParam_X.AncFileName_S.compare("") != 0)
                {
                  mPlayerChannel_X.AncFileOffset_U32 += AncBufferSize_U32;
                  if ((Eof_B) || (mPlayerChannel_X.AncFileOffset_U32 >= FileSize))
                  {
                    mPlayerChannel_X.AncFileOffset_U32 = 0;
                  }
                  memcpy(pAvUserBufferProperty_X->pBuffer, pAncBuffer_U8, AncBufferSize_U32);
                  _rppPutFrameParam_X[i_U32]->Anc.BufferSz = AncBufferSize_U32;
                }
              }
            }
          }
          else
          {
            pAvUserBufferProperty_X->BufferSize = 0;
            pAvUserBufferProperty_X->pBuffer = nullptr;
          }
        }
        if (Rts_i != EVS::EvsPcieIoApi::NoError)
        {
          break;
        }
      }

      GL_puBoardResourceManager->FreeBuffer((void **)&pMergedVideoField_U8);
    }
  }

  return Rts_i;
}
void SimplePlayer::FreeSendingFrame(EVS::EvsPcieIoApi::PPUT_FRAME_PARAMS _pPutFrameParam_X)
{
  uint32_t i_U32;

  if (!_pPutFrameParam_X)
  {
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "%s : invalid args\n", __FUNCTION__);
  }
  else
  {
    GL_puBoardResourceManager->FreeBuffer(&_pPutFrameParam_X->Video.Buffer.pBuffer);
    for (i_U32 = 0; i_U32 < AUDIO_NBRCH; i_U32++)
    {
      GL_puBoardResourceManager->FreeBuffer(&_pPutFrameParam_X->Audio[i_U32].Buffer.pBuffer);
    }
    GL_puBoardResourceManager->FreeBuffer(&_pPutFrameParam_X->Anc.Buffer.pBuffer);
  }
}
