/*++
Copyright (c) Evs Broadcast Equipment. (a) b.harmel@evs.com. All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.
--*/
#include "BoardResourceManager.h"
#include "SimpleRecorder.h"
#include <cassert>

static const std::vector<EVS::EvsPcieIoApi::eVideoStandard> S_VideoStandardToScanCollection = {
    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080p_23_98,
    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080p_24,
    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080p_25,
    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080p_29_97,
    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080p_30,

    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080p_47_95,
    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080p_48,
    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080p_50,
    EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080p_59_94,
    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080p_60,

    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080i_47_95,
    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080i_48,
    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080i_50,
    EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080i_59_94,
    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080i_60,

    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_720p_50,
    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_720p_59_94,
    // EVS::EvsPcieIoApi::eVideoStandard::VideoStd_720p_60,
};

static const std::vector<EVS::EvsPcieIoApi::eVideoStandard> S_FastStdCollection = {
    EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080p_59_94,
    EVS::EvsPcieIoApi::eVideoStandard::VideoStd_1080i_59_94,
};

SimpleRecorder::SimpleRecorder(EVS::EvsPcieIoApi::IEvsIoBoard *_pIIoBoard, EVS::EvsPcieIoApi::IRecorder *_pIRecorder)
{
  mRecorderChannel_X.pPcieIoBoard = (EVS::EvsPcieIoApi::CBasePcieIoBoard *)_pIIoBoard;
  mRecorderChannel_X.pIIoBoard = _pIIoBoard;
  mRecorderChannel_X.pIRecorder = _pIRecorder;
  mRecorderChannel_X.VideoStandard_E = EVS::EvsPcieIoApi::eVideoStandard::VideoStd_Invalid;
}
SimpleRecorder::~SimpleRecorder()
{
  CloseRecorder();
}
int SimpleRecorder::OpenRecorder(AVa_RECORDER_PARAM &_rRecorderParam_X)
{
  int Rts_i = EVS::EvsPcieIoApi::Errno::NoError, MaxWait_i;
  EVS::EvsPcieIoApi::eVideoStandard VideoStandard_E = EVS::EvsPcieIoApi::eVideoStandard::VideoStd_Invalid;

  mAVaRecorderParam_X = _rRecorderParam_X;
  mRecorderParam_X.MaxFifoSize = 32;
  mRecorderParam_X.StartWaterMark = 4;
  mRecorderParam_X.aVideoStandards = S_VideoStandardToScanCollection;

  mRecorderParam_X.useSessionID = true;
  mRecorderParam_X.defaultSessionID = 0;

  mRecorderParam_X.VideoParams.bEnable = true;
  mRecorderParam_X.VideoParams.Fourcc = _rRecorderParam_X.Hr10bit_B ? FOURCC_V210 : FOURCC_UYVY;

  mRecorderParam_X.ProxyParams.bEnable = ((_rRecorderParam_X.LrWidth_U16 != 0) && (_rRecorderParam_X.LrHeight_U16 != 0));
  mRecorderParam_X.ProxyParams.Fourcc = _rRecorderParam_X.Lr10bit_B ? FOURCC_V210 : FOURCC_UYVY;

  mRecorderParam_X.ProxyParams.Width = _rRecorderParam_X.LrWidth_U16;
  mRecorderParam_X.ProxyParams.Height = _rRecorderParam_X.LrHeight_U16;

  mRecorderParam_X.AudioParams.bEnable = (_rRecorderParam_X.AudioChannelMaskToCapture_U32 != 0);
  mRecorderParam_X.AncParams.bEnable = _rRecorderParam_X.EnableAnc_B;

  /*Reset auto-std found*/
  mStdFounded_B = false;

  /*Close recorder if already initialized*/
  if (mRecorderInitialized_B)
  {
    mRecorderChannel_X.pIRecorder->Close();
    mRecorderInitialized_B = false;
  }

  /*Register observer*/
  mRecorderChannel_X.pIRecorder->RegisterInputStatusChangeEvent(this);
  mRecorderChannel_X.pIRecorder->RegisterFrameReceivedEvent(this);

  /*Init recorder*/
  Rts_i = mRecorderChannel_X.pIRecorder->Initialize(&mRecorderParam_X);

  if (Rts_i == EVS::EvsPcieIoApi::Errno::NoError)
  {
    mRecorderInitialized_B = true;
    /*Wait auto-std*/
    MaxWait_i = 3000;
    while (--MaxWait_i)
    {
      if (mStdFounded_B)
      {
        break;
      }
      else
      {
        usleep(10 * 1000);
      }
    }

    if (MaxWait_i == 0)
    {
      ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "fail to found std\n");
      Rts_i = EVS::EvsPcieIoApi::Errno::InvalidStandard;
    }
    else
    {
      VideoStandard_E = mRecorderChannel_X.pIRecorder->GetDetectedStandard();
      // Done in SimpleRecorder::InputStatusChange Rts_i = mRecorderChannel_X.pIRecorder->Start(VideoStandard_E);
      Rts_i = EVS::EvsPcieIoApi::Errno::NoError;
      if (Rts_i == EVS::EvsPcieIoApi::Errno::NoError)
      {
        Rts_i = Start();
      }
    }
  }
  if (Rts_i == EVS::EvsPcieIoApi::Errno::NoError)
  {
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "Recorder %u started with std %s\n", 0, EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::GetStandardName(VideoStandard_E));
  }
  else
  {
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "Cannot start recorder %u with std %s\n", 0, EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::GetStandardName(VideoStandard_E));
  }
  return Rts_i;
}

int SimpleRecorder::Start()
{
  mStopRecorderThread_B = false;
  mRecorderThread = std::thread(&SimpleRecorder::RecorderBgTask, this);

  return EVS::EvsPcieIoApi::Errno::NoError;
}

int SimpleRecorder::CloseRecorder()
{
  int Rts_i;

  ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "Stop thread1\n");

  mStopRecorderThread_B = true;
  ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "Stop thread2\n");
  if (mRecorderThread.joinable())
  {
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "Stop thread3\n");
    mRecorderThread.join();
  }
  ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "Stop thread4\n");
  Rts_i = Stop();
  return Rts_i;
}

int SimpleRecorder::Stop()
{
  int Rts_i;
  uint32_t i_U32;

  /*Close recorder*/
  Rts_i = mRecorderChannel_X.pIRecorder->Close();

  /*Release frames*/
  for (i_U32 = 0; i_U32 < mFrameArraySize_U32; i_U32++)
  {
    ReleaseRecorderItem(&mpFrame[i_U32]);
  }
  return Rts_i;
}

EVS::EvsPcieIoApi::eVideoStandard SimpleRecorder::GetVideoStandard()
{
  return mRecorderChannel_X.VideoStandard_E;
}

int SimpleRecorder::InputStatusChange(uint32_t _Flag_U32, EVS::EvsPcieIoApi::eVideoStandard _DetectedStd_E)
{
  int Rts_i = 0;

  ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "[NOTIF] InputStatusChange Flg %08X VideoStandard %s->%s\n", _Flag_U32, EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::GetStandardName(mRecorderChannel_X.VideoStandard_E), EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::GetStandardName(_DetectedStd_E));
  mRecorderChannel_X.VideoStandard_E = _DetectedStd_E;
  if (_Flag_U32 & EVS::EvsPcieIoApi::REC_INPUT_STATUS::INPUT_STATUS_VLOCK) // means the input is locked on this standard
  {
    mStdFounded_B = true;
    Rts_i = mRecorderChannel_X.pIRecorder->Start(_DetectedStd_E);
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "Standard %s locked\n", EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::GetStandardName(mRecorderChannel_X.VideoStandard_E));
    /*
    if (mStdFounded_B)
    {
      Rts_i = mRecorderChannel_X.pIRecorder->Start(mRecorderChannel_X.VideoStandard_E);
    }
    mStdFounded_B = true;
    */
  }
  else if (mStdFounded_B)
  {
    // means std is not locked !!
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "Standard %s NOT LOCKED\n", EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::GetStandardName(mRecorderChannel_X.VideoStandard_E));
    mStdFounded_B = false;
    Rts_i = mRecorderChannel_X.pIRecorder->Stop();
  }

  if (_Flag_U32 & EVS::EvsPcieIoApi::REC_INPUT_STATUS::INPUT_STATUS_PVID_ERR)
  {
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "INPUT_STATUS_PVID_ERR\n");
  }

  if (_Flag_U32 & EVS::EvsPcieIoApi::REC_INPUT_STATUS::INPUT_STATUS_2SI_MISMATCH)
  {
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "INPUT_STATUS_2SI_MISMATCH\n");
  }

  if (_Flag_U32 & EVS::EvsPcieIoApi::REC_INPUT_STATUS::INPUT_STATUS_NO_VPI)
  {
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "INPUT_STATUS_NO_VPI\n");
  }

  return Rts_i;
}

int SimpleRecorder::NewFrameReceived(EVS::EvsPcieIoApi::FRAME_STATUS _Status_E, EVS::EvsPcieIoApi::PFRAME_RECEIVED_INFO _pReceivedInfo_X)
{
  int Rts_i = EVS::EvsPcieIoApi::RuntimeError;
  uint32_t FrameIndex_U32;
  RecorderItem *pRecorderItem;

  if (!_pReceivedInfo_X)
  {
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "[NOTIF] NewFrameReceived Sts %x\n", _Status_E);
  }
  else
  {
    Rts_i = EVS::EvsPcieIoApi::NoError;
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "[NOTIF] NewFrameReceived Sts %x %x %x %x Sz %x %x %x %x\n", _Status_E, _pReceivedInfo_X->ProxyBufferStatus, _pReceivedInfo_X->AudioBufferStatus, _pReceivedInfo_X->AncilBufferStatus, _pReceivedInfo_X->VideoBufferSz, _pReceivedInfo_X->VideoProxyBufferSz, _pReceivedInfo_X->AudioCh[0].AudioBufferSz, _pReceivedInfo_X->AncBufferSz);

    /*Get frame index which is the */
    FrameIndex_U32 = _pReceivedInfo_X->sOriginalReq.Id;
    if (FrameIndex_U32 >= mFrameArraySize_U32)
    {
      Rts_i = EVS::EvsPcieIoApi::Errno::RuntimeError;
    }
    else
    {
      /*Get structure associated*/
      pRecorderItem = &mpFrame[FrameIndex_U32];

      /*Copy parameters*/
      pRecorderItem->FrameData_X = *_pReceivedInfo_X;

      /*Increase number of frame received*/
      mReceivedNbFrame_U32++;

      if ((mReceivedNbFrame_U32 % 100) == 0)
      {
        ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "mReceivedNbFrame_U32 %d\n", mReceivedNbFrame_U32);
      }
      /*
              static uint32_t S_LastRefTiming_U32 = 0;
              ADD_MESSAGE(EvsHwLGPL::SEV_INFO,"> New!: Frame %d %p Sts %d par %d Sz H %x L %x A %x a %x tim %u delta %d\n", mReceivedNbFrame_U32, pRecorderItem, pRecorderItem->FrameStatus_i, pRecorderItem->ExpectedParity_U8, (uint32_t)pRecorderItem->FrameData_X.VideoBufferSz,
                     (uint32_t)pRecorderItem->FrameData_X.VideoProxyBufferSz, (uint32_t)pRecorderItem->FrameData_X.AudioCh[0].AudioBufferSz, (uint32_t)pRecorderItem->FrameData_X.AncBufferSz,
                     pRecorderItem->FrameData_X.RefTiming, pRecorderItem->FrameData_X.RefTiming - S_LastRefTiming_U32);
              S_LastRefTiming_U32 = pRecorderItem->FrameData_X.RefTiming;
      */
      mReceivedFrameFifo.Insert(&pRecorderItem->FifoRefCollection, true);
    }
  }
  return Rts_i;
}
int SimpleRecorder::AllocateRecorderItem(RecorderItem *_pRecorderItem, uint32_t _Id_U32)
{
  int Rts_i;
  uint32_t i_U32, BufferAlignment_U32 = 4096;
  uint32_t HiResBufferSize_U32 = 24 * 1024 * 1024; // 23MO for 4K
  uint32_t LoResBufferSize_U32 = 2 * 1024 * 1024;
  uint32_t AudioBufferSize_U32 = AUDIO_BUFFER_SIZE;
  uint32_t AncBufferSize_U32 = ANCILLARY_BUFFER_SIZE;
  EVS::EvsPcieIoApi::PAV_USER_BUFFER_PROPERTIES pAVBufferProperty;

  _pRecorderItem->FrameData_X.sOriginalReq.Id = _Id_U32;

  pAVBufferProperty = (EVS::EvsPcieIoApi::PAV_USER_BUFFER_PROPERTIES)&_pRecorderItem->FrameData_X.sOriginalReq.Video.Buffer;
  pAVBufferProperty->BufferSize = HiResBufferSize_U32;
  Rts_i = GL_puBoardResourceManager->AllocateBuffer(&pAVBufferProperty->pBuffer, pAVBufferProperty->BufferSize, BufferAlignment_U32);
  if (!Rts_i)
  {
    //    memset(pBuffer->pBuffer, 0x50, pBuffer->BufferSize);
    pAVBufferProperty = (EVS::EvsPcieIoApi::PAV_USER_BUFFER_PROPERTIES)&_pRecorderItem->FrameData_X.sOriginalReq.VideoProxy.Buffer;
    pAVBufferProperty->BufferSize = LoResBufferSize_U32;
    Rts_i = GL_puBoardResourceManager->AllocateBuffer(&pAVBufferProperty->pBuffer, pAVBufferProperty->BufferSize, BufferAlignment_U32);
    if (!Rts_i)
    {
      for (i_U32 = 0; i_U32 < AUDIO_NBRCH; i_U32++)
      {
        pAVBufferProperty = (EVS::EvsPcieIoApi::PAV_USER_BUFFER_PROPERTIES)&_pRecorderItem->FrameData_X.sOriginalReq.Audio[i_U32].Buffer;
        pAVBufferProperty->BufferSize = AudioBufferSize_U32;
        Rts_i = GL_puBoardResourceManager->AllocateBuffer(&pAVBufferProperty->pBuffer, pAVBufferProperty->BufferSize, BufferAlignment_U32);
        if (Rts_i)
        {
          break;
        }
      }

      if (!Rts_i)
      {
        pAVBufferProperty = (EVS::EvsPcieIoApi::PAV_USER_BUFFER_PROPERTIES)&_pRecorderItem->FrameData_X.sOriginalReq.Anc.Buffer;
        pAVBufferProperty->BufferSize = AncBufferSize_U32;
        Rts_i = GL_puBoardResourceManager->AllocateBuffer(&pAVBufferProperty->pBuffer, pAVBufferProperty->BufferSize, BufferAlignment_U32);
      }
    }
  }
  return Rts_i;
}

int SimpleRecorder::ReleaseRecorderItem(RecorderItem *_pRecorderItem)
{
  int Rts_i = 0;
  uint32_t i_U32;

  GL_puBoardResourceManager->FreeBuffer(&_pRecorderItem->FrameData_X.sOriginalReq.Video.Buffer.pBuffer);
  GL_puBoardResourceManager->FreeBuffer(&_pRecorderItem->FrameData_X.sOriginalReq.VideoProxy.Buffer.pBuffer);
  for (i_U32 = 0; i_U32 < AUDIO_NBRCH; i_U32++)
  {
    GL_puBoardResourceManager->FreeBuffer(&_pRecorderItem->FrameData_X.sOriginalReq.Audio[i_U32].Buffer.pBuffer);
  }
  GL_puBoardResourceManager->FreeBuffer(&_pRecorderItem->FrameData_X.sOriginalReq.Anc.Buffer.pBuffer);
  return Rts_i;
}
void SimpleRecorder::RequestNewRecorderItem(RecorderItem *_pRecorderItem0, RecorderItem *_pRecorderItem1)
{
  int Sts_i, MaxLoop_i = 5;
  while (--MaxLoop_i > 0)
  {
    /*Request Frame 0*/
    Sts_i = EVS::EvsPcieIoApi::Errno::InvalidState;
    if (_pRecorderItem0)
    {
      _pRecorderItem0->ExpectedParity_U8 = 0;
      _pRecorderItem0->FrameData_X.sOriginalReq.FrameSessionID = mCurrentSessionId_U32;

      Sts_i = mRecorderChannel_X.pIRecorder->RequestNewFrame(&_pRecorderItem0->FrameData_X.sOriginalReq);
      if (Sts_i != EVS::EvsPcieIoApi::Errno::NoError)
      {
        if (Sts_i == EVS::EvsPcieIoApi::Errno::NoMoreSpace)
        {
          usleep(5000);
          continue;
        }
        else if (Sts_i == EVS::EvsPcieIoApi::Errno::InvalidSession)
        {
          mCurrentSessionId_U32 = _pRecorderItem0->FrameData_X.sOriginalReq.CurrentSessionID;
          continue;
        }
        else
        {
          mReceivedFrameFifo.Insert(&_pRecorderItem0->FifoRefCollection, true);
          break; // means critical error => not recoverable
        }
      }
      break;
    }
  }
  /*Request Frame 1*/
  if (_pRecorderItem1)
  {
    if (Sts_i != EVS::EvsPcieIoApi::Errno::NoError) // if frame0 is not inserted
    {
      mReceivedFrameFifo.Insert(&_pRecorderItem1->FifoRefCollection, true);
    }
    else
    {
      MaxLoop_i = 5;
      while (--MaxLoop_i > 0)
      {
        _pRecorderItem1->ExpectedParity_U8 = 1;
        _pRecorderItem1->FrameData_X.sOriginalReq.FrameSessionID = mCurrentSessionId_U32;

        Sts_i = mRecorderChannel_X.pIRecorder->RequestNewFrame(&_pRecorderItem1->FrameData_X.sOriginalReq);
        if (Sts_i != EVS::EvsPcieIoApi::Errno::NoError)
        {
          if (Sts_i == EVS::EvsPcieIoApi::Errno::NoMoreSpace)
          {
            usleep(5000); // try again when space is recovered
            continue;
          }
          else if (Sts_i == EVS::EvsPcieIoApi::Errno::InvalidSession)
          {
            // push frame as if it was received and update current session iD
            mCurrentSessionId_U32 = _pRecorderItem1->FrameData_X.sOriginalReq.CurrentSessionID;
            mReceivedFrameFifo.Insert(&_pRecorderItem1->FifoRefCollection, true);
          }
        }
        break;
      }
    }
  }
}

void SimpleRecorder::RecorderBgTask()
{
  AVa_RECORDER_PARAM RecorderParam_X;
  PLIST_ITEM pListItem;
  RecorderItem *pFrame[2];
  //bool SaveVideo_B, SaveProxy_B, SaveAudio_B, SaveAnc_B;
  AVa_BUFFER pVideoBuffer_X[2], pProxyBuffer_X[2], ppAudioBuffer_X[AUDIO_NBRCH][2], pAncBuffer_X[2];
  int32_t *pAudioData_S32;
  uint32_t f_U32, i_U32, j_U32, k_U32, VideoNbLine_U32, VideoLineWidthInPixel_U32, VideoImageSize_U32, VideoLineSizeInByte_U32, ProxyImageSize_U32, ProxyLineSizeInByte_U32;
  uint32_t NbFrameRcv_U32, NbSize0Rcv_U32, NbBadStatusRcv_U32, NbTimeOutRcv_U32, NbAudioSample_U32, AudioLevel_U32;
  uint32_t AudioChannelMaskToCapture_U32, AudioChannelMask_U32, NbFrameOk_U32, BufferAlignment_U32 = 4096;
  uint16_t ProxyNbLine_U16, ProxyLineWidthInPixel_U16;
  uint8_t *pMergedVideoField_U8;
  int Sts_i, Len_i;
  bool IncomingData_B, ReadyToSave_B, ResetStat_B, AcqRunning_B, IsProgressive_B;
  char pFn_c[512], pAncFlag_c[16], pAncData_c[0x4000]; // max ANCILLARY_BUFFER_SIZE byte * 3 for ascii dump in file
  constexpr char pBaseSaveDir_c[] = "C:/tmp/";
  EVS::EvsPcieIoApi::FOURCC VideoFourCc, ProxyFourCc;
  float VideoFrameRate_f, ProxyFrameRate_f;
  std::vector<EVS::EvsPcieIoApi::ANC_ITEM> AncCollection;
  EVS::EvsPcieIoApi::eVideoStandard VideoStandard_E;
  EVS::EvsPcieIoApi::eFrameRate InterleavedFrameRate_E;

  mReceivedFrameFifo.Init();

  /*Allocate new frames*/
  for (i_U32 = 0; i_U32 < mFrameArraySize_U32; i_U32++)
  {
    AllocateRecorderItem(&mpFrame[i_U32], i_U32);

    /*Insert item into list*/
    mReceivedFrameFifo.Insert(&mpFrame[i_U32].FifoRefCollection, false);
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "Alloc: Frame %d/%d %p sts %d par %d Sz %x tim %u\n", i_U32, mFrameArraySize_U32, &mpFrame[i_U32], mpFrame[i_U32].FrameStatus_i, mpFrame[i_U32].ExpectedParity_U8, (uint32_t)mpFrame[i_U32].FrameData_X.VideoBufferSz, mpFrame[i_U32].FrameData_X.RefTiming);
  }

  /*Register observer*/
  // mRecorderChannel_X.pIRecorder->RegisterInputStatusChangeEvent(this);
  // mRecorderChannel_X.pIRecorder->RegisterFrameReceivedEvent(this);

  NbFrameRcv_U32 = 0;
  NbSize0Rcv_U32 = 0;
  NbBadStatusRcv_U32 = 0;
  NbTimeOutRcv_U32 = 0;
  VideoStandard_E = mRecorderChannel_X.pIRecorder->GetDetectedStandard();
  assert(VideoStandard_E != EVS::EvsPcieIoApi::VideoStd_Invalid);
  VideoNbLine_U32 = EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::GetNbLines(VideoStandard_E);
  VideoLineWidthInPixel_U32 = EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::GetLineWidth(VideoStandard_E);
  IsProgressive_B = EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::IsProgressive(VideoStandard_E);
  VideoFrameRate_f = GL_puBoardResourceManager->VideoStandardToFrameRate(VideoStandard_E, InterleavedFrameRate_E);
  mRecorderChannel_X.pIRecorder->GetFourCc(VideoFourCc, ProxyFourCc);
  mRecorderChannel_X.pIRecorder->GetProxyResolution(ProxyLineWidthInPixel_U16, ProxyNbLine_U16);
  VideoImageSize_U32 = (uint32_t)EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::ComputeImageSize(VideoStandard_E, VideoFourCc, VideoLineWidthInPixel_U32, VideoNbLine_U32, false);
  // VideoLineSizeInByte_U32 = VideoImageSize_U32 / VideoNbLine_U32;
  VideoLineSizeInByte_U32 = (uint32_t)EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::ComputeImageSize(VideoStandard_E, VideoFourCc, VideoLineWidthInPixel_U32, 1, true);
  if (!IsProgressive_B)
  {
    VideoImageSize_U32 = VideoImageSize_U32 * 2;
    ProxyFrameRate_f = VideoFrameRate_f * 2;
  }
  else
  {
    ProxyFrameRate_f = VideoFrameRate_f;
  }
  Sts_i = GL_puBoardResourceManager->AllocateBuffer((void **)&pMergedVideoField_U8, VideoImageSize_U32, BufferAlignment_U32);
  if (Sts_i == EVS::EvsPcieIoApi::NoError)
  {
    ProxyImageSize_U32 = (uint32_t)EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::ComputeImageSize(VideoStandard_E, ProxyFourCc, ProxyLineWidthInPixel_U16, ProxyNbLine_U16, true);
    // ProxyLineSizeInByte_U32 = ProxyImageSize_U32 / ProxyNbLine_U16;
    ProxyLineSizeInByte_U32 = (uint32_t)EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::ComputeImageSize(VideoStandard_E, ProxyFourCc, ProxyLineWidthInPixel_U16, 1, true);
    ResetStat_B = true;
    IncomingData_B = false;
    AcqRunning_B = false;
    AudioChannelMaskToCapture_U32 = 0x000000C0; // Channel 6 and 7
    NbFrameOk_U32 = 0;
    if (!mAVaRecorderParam_X.SnapshotSave_B)
    {
      if (mAVaRecorderParam_X.SaveVideo_B)
      {
        sprintf(pFn_c, "%s%dx%d@%.2f%c_clp_0.yuv%d", pBaseSaveDir_c, VideoLineWidthInPixel_U32, VideoNbLine_U32, VideoFrameRate_f, IsProgressive_B ? 'p' : 'i', (VideoFourCc == FOURCC_V210) ? 10 : 8);
        pVideoBuffer_X[0].pIo_X = fopen(pFn_c, "wb");
        pVideoBuffer_X[1].pIo_X = pVideoBuffer_X[0].pIo_X;
      }
      if (mAVaRecorderParam_X.SaveProxy_B)
      {
        // LR is progressive
        sprintf(pFn_c, "%s%dx%d@%.2f%c_clp_0.yuv%d", pBaseSaveDir_c, ProxyLineWidthInPixel_U16, ProxyNbLine_U16, VideoFrameRate_f, true ? 'p' : 'i', (ProxyFourCc == FOURCC_V210) ? 10 : 8);
        pProxyBuffer_X[0].pIo_X = fopen(pFn_c, "wb");
        pProxyBuffer_X[1].pIo_X = pProxyBuffer_X[0].pIo_X;
      }
      if (mAVaRecorderParam_X.SaveAudio_B)
      {
        for (AudioChannelMask_U32 = 1, k_U32 = 0; k_U32 < AUDIO_NBRCH; k_U32++, AudioChannelMask_U32 <<= 1)
        {
          if (AudioChannelMaskToCapture_U32 & AudioChannelMask_U32)
          {
            sprintf(pFn_c, "%s16xS24L32@48000_%d_0.pcm", pBaseSaveDir_c, k_U32);
            ppAudioBuffer_X[k_U32][0].pIo_X = fopen(pFn_c, "wb");
            ppAudioBuffer_X[k_U32][1].pIo_X = ppAudioBuffer_X[k_U32][0].pIo_X;
          }
        }
      }
      if (mAVaRecorderParam_X.SaveAnc_B)
      {
        sprintf(pFn_c, "%sRawAnc_0.bin", pBaseSaveDir_c);
        pAncBuffer_X[0].pIo_X = fopen(pFn_c, "wb");
        sprintf(pFn_c, "%sRawAnc_0.txt", pBaseSaveDir_c);
        pAncBuffer_X[1].pIo_X = fopen(pFn_c, "wb");
      }
    }
    while (!mStopRecorderThread_B)
    {
      for (f_U32 = 0; f_U32 < 2; f_U32++)
      {
        //        ADD_MESSAGE(EvsHwLGPL::SEV_INFO,">> Wait data %d\n",  i);
        mReceivedFrameFifo.WaitItem(500, &pListItem, &mStopRecorderThread_B);
        if (pListItem)
        {
          pFrame[f_U32] = container_of(pListItem, RecorderItem, FifoRefCollection);
          // Delta based on 13.5 MHz clock: 135000000 ticks gives 1 sec so delta 225030 /13500000 = 0.016666 sec
          NbFrameRcv_U32++;
          if (pFrame[f_U32]->FrameData_X.VideoBufferSz == 0)
          {
            NbSize0Rcv_U32++;
          }
          if (pFrame[f_U32]->FrameStatus_i)
          {
            NbBadStatusRcv_U32++;
          }
          if (!IncomingData_B)
          {
            ADD_MESSAGE(EvsHwLGPL::SEV_INFO, ">> Waiting for valid data Sz %d Sts %d NbOk %d\n", pFrame[f_U32]->FrameData_X.VideoBufferSz, pFrame[f_U32]->FrameStatus_i, NbFrameOk_U32);
            if ((pFrame[f_U32]->FrameData_X.VideoBufferSz) && (pFrame[f_U32]->FrameStatus_i == 0))
            {
              NbFrameOk_U32++;
              if (NbFrameOk_U32 >= 8)
              {
                IncomingData_B = true;
              }
            }
            else
            {
              NbFrameOk_U32 = 0;
            }
          }
          if (IncomingData_B)
          {
            static uint32_t S_LastRefTiming_U32 = 0;
            ADD_MESSAGE(EvsHwLGPL::SEV_INFO, ">> Recv: Frm %d Sz0 %d Bad %d To %d i %d %p Sts %d par %d Sz H %x L %x A %x a %x tim %u delta %d\n", NbFrameRcv_U32, NbSize0Rcv_U32, NbBadStatusRcv_U32, NbTimeOutRcv_U32, f_U32,
                        pFrame[f_U32], pFrame[f_U32]->FrameStatus_i, pFrame[f_U32]->ExpectedParity_U8, (uint32_t)pFrame[f_U32]->FrameData_X.VideoBufferSz, (uint32_t)pFrame[f_U32]->FrameData_X.VideoProxyBufferSz, (uint32_t)pFrame[f_U32]->FrameData_X.AudioCh[0].AudioBufferSz, (uint32_t)pFrame[f_U32]->FrameData_X.AncBufferSz, pFrame[f_U32]->FrameData_X.RefTiming, pFrame[f_U32]->FrameData_X.RefTiming - S_LastRefTiming_U32);
            S_LastRefTiming_U32 = pFrame[f_U32]->FrameData_X.RefTiming;

            ReadyToSave_B = false;
            if (IsProgressive_B)
            {
              ReadyToSave_B = (pFrame[f_U32]->FrameData_X.VideoBufferSz != 0);
              if (ReadyToSave_B)
              {
                pVideoBuffer_X[0].Size_U32 = (uint32_t)pFrame[f_U32]->FrameData_X.VideoBufferSz;
                pVideoBuffer_X[0].pBuffer_U8 = (uint8_t *)pFrame[f_U32]->FrameData_X.sOriginalReq.Video.Buffer.pBuffer;
                pVideoBuffer_X[1].Size_U32 = 0;
                pVideoBuffer_X[1].pBuffer_U8 = nullptr;

                if (pFrame[f_U32]->FrameData_X.ProxyBufferStatus == EVS::EvsPcieIoApi::BufferStatus_Valid)
                {
                  pProxyBuffer_X[0].Size_U32 = (uint32_t)pFrame[f_U32]->FrameData_X.VideoProxyBufferSz;
                  pProxyBuffer_X[0].pBuffer_U8 = (uint8_t *)pFrame[f_U32]->FrameData_X.sOriginalReq.VideoProxy.Buffer.pBuffer;
                  pProxyBuffer_X[1].Size_U32 = 0;
                  pProxyBuffer_X[1].pBuffer_U8 = nullptr;
                }
                else
                {
                  pProxyBuffer_X[0].Size_U32 = 0;
                  pProxyBuffer_X[0].pBuffer_U8 = nullptr;
                  pProxyBuffer_X[1].Size_U32 = 0;
                  pProxyBuffer_X[1].pBuffer_U8 = nullptr;
                }

                if (pFrame[f_U32]->FrameData_X.AudioBufferStatus == EVS::EvsPcieIoApi::BufferStatus_Valid)
                {
                  for (AudioChannelMask_U32 = 1, k_U32 = 0; k_U32 < AUDIO_NBRCH; k_U32++, AudioChannelMask_U32 <<= 1)
                  {
                    if (AudioChannelMaskToCapture_U32 & AudioChannelMask_U32)
                    {
                      ppAudioBuffer_X[k_U32][0].Size_U32 = (uint32_t)pFrame[f_U32]->FrameData_X.AudioCh[k_U32].AudioBufferSz;
                      ppAudioBuffer_X[k_U32][0].pBuffer_U8 = (uint8_t *)pFrame[f_U32]->FrameData_X.sOriginalReq.Audio[k_U32].Buffer.pBuffer;
                      ppAudioBuffer_X[k_U32][1].Size_U32 = 0;
                      ppAudioBuffer_X[k_U32][1].pBuffer_U8 = nullptr;
                      if (mAVaRecorderParam_X.AudioAmplifier_U32)
                      {
                        NbAudioSample_U32 = pFrame[f_U32]->FrameData_X.AudioCh[k_U32].NbrSamples;
                        AudioLevel_U32 = pFrame[f_U32]->FrameData_X.AudioCh[k_U32].Level;
                        // ADD_MESSAGE(EvsHwLGPL::SEV_INFO,"Channel[%d] Sz %d -> Amplify %d sample by %d (level is %08X)\n", k_U32, ppAudioBuffer_X[k_U32][0].Size_U32, NbAudioSample_U32, AudioAmplifier_U32, AudioLevel_U32);
                        pAudioData_S32 = (int32_t *)ppAudioBuffer_X[k_U32][0].pBuffer_U8;
                        for (j_U32 = 0; j_U32 < NbAudioSample_U32; j_U32++, pAudioData_S32++)
                        {
                          *pAudioData_S32 = ((*pAudioData_S32) * mAVaRecorderParam_X.AudioAmplifier_U32);
                        }
                      }
                    }
                  }
                }
                else
                {
                  for (AudioChannelMask_U32 = 1, k_U32 = 0; k_U32 < AUDIO_NBRCH; k_U32++, AudioChannelMask_U32 <<= 1)
                  {
                    if (AudioChannelMaskToCapture_U32 & AudioChannelMask_U32)
                    {
                      ppAudioBuffer_X[k_U32][0].Size_U32 = 0;
                      ppAudioBuffer_X[k_U32][0].pBuffer_U8 = nullptr;
                      ppAudioBuffer_X[k_U32][1].Size_U32 = 0;
                      ppAudioBuffer_X[k_U32][1].pBuffer_U8 = nullptr;
                    }
                  }
                }

                if (pFrame[f_U32]->FrameData_X.AncilBufferStatus == EVS::EvsPcieIoApi::BufferStatus_Valid)
                {
                  pAncBuffer_X[0].Size_U32 = (uint32_t)pFrame[f_U32]->FrameData_X.AncBufferSz;
                  pAncBuffer_X[0].pBuffer_U8 = (uint8_t *)pFrame[f_U32]->FrameData_X.sOriginalReq.Anc.Buffer.pBuffer;
                  pAncBuffer_X[1].Size_U32 = 0;
                  pAncBuffer_X[1].pBuffer_U8 = nullptr;
                }
                else
                {
                  pAncBuffer_X[0].Size_U32 = 0;
                  pAncBuffer_X[0].pBuffer_U8 = nullptr;
                  pAncBuffer_X[1].Size_U32 = 0;
                  pAncBuffer_X[1].pBuffer_U8 = nullptr;
                }
              }
            }
            else
            {
              ReadyToSave_B = ((f_U32 == 1) && (pFrame[0]) && (pFrame[0]->FrameData_X.VideoBufferSz != 0) && (pFrame[1]) && (pFrame[1]->FrameData_X.VideoBufferSz != 0));
              if (ReadyToSave_B)
              {
                pVideoBuffer_X[0].Size_U32 = (uint32_t)pFrame[0]->FrameData_X.VideoBufferSz;
                pVideoBuffer_X[0].pBuffer_U8 = (uint8_t *)pFrame[0]->FrameData_X.sOriginalReq.Video.Buffer.pBuffer;
                pVideoBuffer_X[1].Size_U32 = (uint32_t)pFrame[1]->FrameData_X.VideoBufferSz;
                pVideoBuffer_X[1].pBuffer_U8 = (uint8_t *)pFrame[1]->FrameData_X.sOriginalReq.Video.Buffer.pBuffer;
                assert((pVideoBuffer_X[0].Size_U32 + pVideoBuffer_X[1].Size_U32) == VideoImageSize_U32);
                GL_puBoardResourceManager->InterleaveEvenOddField(VideoNbLine_U32 / 2, VideoLineSizeInByte_U32, &pVideoBuffer_X[0].pBuffer_U8[0], &pVideoBuffer_X[1].pBuffer_U8[0], pMergedVideoField_U8);
                pVideoBuffer_X[0].pBuffer_U8 = pMergedVideoField_U8;
                pVideoBuffer_X[1].pBuffer_U8 = pMergedVideoField_U8 + (VideoImageSize_U32 / 2);

                for (i_U32 = 0; i_U32 < 2; i_U32++)
                {
                  if (pFrame[i_U32]->FrameData_X.ProxyBufferStatus == EVS::EvsPcieIoApi::BufferStatus_Valid)
                  {
                    pProxyBuffer_X[i_U32].Size_U32 = (uint32_t)pFrame[i_U32]->FrameData_X.VideoProxyBufferSz;
                    pProxyBuffer_X[i_U32].pBuffer_U8 = (uint8_t *)pFrame[i_U32]->FrameData_X.sOriginalReq.VideoProxy.Buffer.pBuffer;
                  }
                  else
                  {
                    pProxyBuffer_X[i_U32].Size_U32 = 0;
                    pProxyBuffer_X[i_U32].pBuffer_U8 = nullptr;
                  }
                }
                assert((pProxyBuffer_X[0].Size_U32 + pProxyBuffer_X[1].Size_U32) == (2 * ProxyImageSize_U32));

                for (i_U32 = 0; i_U32 < 2; i_U32++)
                {
                  if (pFrame[i_U32]->FrameData_X.AudioBufferStatus == EVS::EvsPcieIoApi::BufferStatus_Valid)
                  {
                    for (AudioChannelMask_U32 = 1, k_U32 = 0; k_U32 < AUDIO_NBRCH; k_U32++, AudioChannelMask_U32 <<= 1)
                    {
                      if (AudioChannelMaskToCapture_U32 & AudioChannelMask_U32)
                      {
                        ppAudioBuffer_X[k_U32][i_U32].Size_U32 = (uint32_t)pFrame[i_U32]->FrameData_X.AudioCh[k_U32].AudioBufferSz;
                        ppAudioBuffer_X[k_U32][i_U32].pBuffer_U8 = (uint8_t *)pFrame[i_U32]->FrameData_X.sOriginalReq.Audio[k_U32].Buffer.pBuffer;
                        if (mAVaRecorderParam_X.AudioAmplifier_U32)
                        {
                          NbAudioSample_U32 = pFrame[i_U32]->FrameData_X.AudioCh[k_U32].NbrSamples;
                          AudioLevel_U32 = pFrame[i_U32]->FrameData_X.AudioCh[k_U32].Level;
                          // ADD_MESSAGE(EvsHwLGPL::SEV_INFO,"Channel[%d] Sz %d -> Amplify %d sample by %d (level is %08X)\n", k_U32, ppAudioBuffer_X[k_U32][i_U32].Size_U32, NbAudioSample_U32, AudioAmplifier_U32, AudioLevel_U32);
                          pAudioData_S32 = (int32_t *)ppAudioBuffer_X[k_U32][i_U32].pBuffer_U8;
                          for (j_U32 = 0; j_U32 < NbAudioSample_U32; j_U32++, pAudioData_S32++)
                          {
                            *pAudioData_S32 = ((*pAudioData_S32) * mAVaRecorderParam_X.AudioAmplifier_U32);
                          }
                        }
                      }
                    }
                  }
                  else
                  {
                    for (AudioChannelMask_U32 = 1, k_U32 = 0; k_U32 < AUDIO_NBRCH; k_U32++, AudioChannelMask_U32 <<= 1)
                    {
                      if (AudioChannelMaskToCapture_U32 & AudioChannelMask_U32)
                      {
                        ppAudioBuffer_X[k_U32][i_U32].Size_U32 = 0;
                        ppAudioBuffer_X[k_U32][i_U32].pBuffer_U8 = nullptr;
                      }
                    }
                  }
                }
                for (i_U32 = 0; i_U32 < 2; i_U32++)
                {
                  AncCollection.clear();
                  if (pFrame[i_U32]->FrameData_X.AncilBufferStatus == EVS::EvsPcieIoApi::BufferStatus_Valid)
                  {
                    pAncBuffer_X[i_U32].Size_U32 = (uint32_t)pFrame[i_U32]->FrameData_X.AncBufferSz;
                    pAncBuffer_X[i_U32].pBuffer_U8 = (uint8_t *)pFrame[i_U32]->FrameData_X.sOriginalReq.Anc.Buffer.pBuffer;
                  }
                  else
                  {
                    pAncBuffer_X[i_U32].Size_U32 = 0;
                    pAncBuffer_X[i_U32].pBuffer_U8 = nullptr;
                  }
                }
              }
            }
            if (ReadyToSave_B)
            {
              if (ResetStat_B)
              {
                ADD_MESSAGE(EvsHwLGPL::SEV_INFO, ">> ReadyToSave: Reset statistic TIMEOUT Frm %d Sz0 %d Bad %d To %d\n", NbFrameRcv_U32, NbSize0Rcv_U32, NbBadStatusRcv_U32, NbTimeOutRcv_U32);
                NbFrameRcv_U32 = 0;
                NbSize0Rcv_U32 = 0;
                NbBadStatusRcv_U32 = 0;
                NbTimeOutRcv_U32 = 0;
                ResetStat_B = false;
                AcqRunning_B = true;
              }
              if (mAVaRecorderParam_X.SnapshotSave_B)
              {
                if (mAVaRecorderParam_X.SaveVideo_B)
                {
                  sprintf(pFn_c, "%s%dx%d@%.2f%c_pic_%d.yuv%d", pBaseSaveDir_c, VideoLineWidthInPixel_U32, VideoNbLine_U32, VideoFrameRate_f, IsProgressive_B ? 'p' : 'i', NbFrameRcv_U32, (VideoFourCc == FOURCC_V210) ? 10 : 8);
                  pVideoBuffer_X[0].pIo_X = fopen(pFn_c, "wb");
                  pVideoBuffer_X[1].pIo_X = pVideoBuffer_X[0].pIo_X;
                }
                if (mAVaRecorderParam_X.SaveProxy_B)
                {
                  // LR is progressive
                  sprintf(pFn_c, "%s%dx%d@%.2f%c_pic_%d.yuv%d", pBaseSaveDir_c, ProxyLineWidthInPixel_U16, ProxyNbLine_U16, VideoFrameRate_f, true ? 'p' : 'i', NbFrameRcv_U32, (ProxyFourCc == FOURCC_V210) ? 10 : 8);
                  pProxyBuffer_X[0].pIo_X = fopen(pFn_c, "wb");
                  pProxyBuffer_X[1].pIo_X = pProxyBuffer_X[0].pIo_X;
                }
                if (mAVaRecorderParam_X.SaveAudio_B)
                {
                  for (AudioChannelMask_U32 = 1, k_U32 = 0; k_U32 < AUDIO_NBRCH; k_U32++, AudioChannelMask_U32 <<= 1)
                  {
                    if (AudioChannelMaskToCapture_U32 & AudioChannelMask_U32)
                    {
                      sprintf(pFn_c, "%s16xS24L32@48000_%d_%d.pcm", pBaseSaveDir_c, k_U32, NbFrameRcv_U32);
                      ppAudioBuffer_X[k_U32][0].pIo_X = fopen(pFn_c, "wb");
                      ppAudioBuffer_X[k_U32][1].pIo_X = ppAudioBuffer_X[k_U32][0].pIo_X;
                    }
                  }
                }
                if (mAVaRecorderParam_X.SaveAnc_B)
                {
                  sprintf(pFn_c, "%sRawAnc_%d.bin", pBaseSaveDir_c, NbFrameRcv_U32);
                  pAncBuffer_X[0].pIo_X = fopen(pFn_c, "wb");
                  sprintf(pFn_c, "%sRawAnc_%d.txt", pBaseSaveDir_c, NbFrameRcv_U32);
                  pAncBuffer_X[1].pIo_X = fopen(pFn_c, "wb");
                }
              }
              // ADD_MESSAGE(EvsHwLGPL::SEV_INFO,">> Save data in %s (%p)\n", pFn_c, pVideoBuffer_X[0].pIo_X);
              //  DWORD d = GetLastError();

              if ((pVideoBuffer_X[0].pBuffer_U8) && (pVideoBuffer_X[0].Size_U32))
              {
                if (pVideoBuffer_X[0].pIo_X)
                {
                  fwrite(pVideoBuffer_X[0].pBuffer_U8, 1, pVideoBuffer_X[0].Size_U32, pVideoBuffer_X[0].pIo_X);
                  if ((pVideoBuffer_X[1].pBuffer_U8) && (pVideoBuffer_X[1].Size_U32))
                  {
                    fwrite(pVideoBuffer_X[1].pBuffer_U8, 1, pVideoBuffer_X[1].Size_U32, pVideoBuffer_X[0].pIo_X);
                  }
                }
              }
              if ((pProxyBuffer_X[0].pBuffer_U8) && (pProxyBuffer_X[0].Size_U32))
              {
                if (pProxyBuffer_X[0].pIo_X)
                {
                  fwrite(pProxyBuffer_X[0].pBuffer_U8, 1, pProxyBuffer_X[0].Size_U32, pProxyBuffer_X[0].pIo_X);
                  if ((pProxyBuffer_X[1].pBuffer_U8) && (pProxyBuffer_X[1].Size_U32))
                  {
                    fwrite(pProxyBuffer_X[1].pBuffer_U8, 1, pProxyBuffer_X[1].Size_U32, pProxyBuffer_X[0].pIo_X);
                  }
                }
              }
              for (AudioChannelMask_U32 = 1, k_U32 = 0; k_U32 < AUDIO_NBRCH; k_U32++, AudioChannelMask_U32 <<= 1)
              {
                if (AudioChannelMaskToCapture_U32 & AudioChannelMask_U32)
                {
                  if ((ppAudioBuffer_X[k_U32][0].pBuffer_U8) && (ppAudioBuffer_X[k_U32][0].Size_U32))
                  {
                    if (ppAudioBuffer_X[k_U32][0].pIo_X)
                    {
                      fwrite(ppAudioBuffer_X[k_U32][0].pBuffer_U8, 1, ppAudioBuffer_X[k_U32][0].Size_U32, ppAudioBuffer_X[k_U32][0].pIo_X);
                      if ((ppAudioBuffer_X[k_U32][1].pBuffer_U8) && (ppAudioBuffer_X[k_U32][1].Size_U32))
                      {
                        fwrite(ppAudioBuffer_X[k_U32][1].pBuffer_U8, 1, ppAudioBuffer_X[k_U32][1].Size_U32, ppAudioBuffer_X[k_U32][0].pIo_X);
                      }
                    }
                  }
                }
              }
              if ((pAncBuffer_X[1].pIo_X) && (pAncBuffer_X[0].pBuffer_U8) && (pAncBuffer_X[0].Size_U32))
              {
                if (pAncBuffer_X[0].pIo_X)
                {
                  fwrite(pAncBuffer_X[0].pBuffer_U8, 1, pAncBuffer_X[0].Size_U32, pAncBuffer_X[0].pIo_X);
                  if ((pAncBuffer_X[1].pBuffer_U8) && (pAncBuffer_X[1].Size_U32))
                  {
                    fwrite(pAncBuffer_X[1].pBuffer_U8, 1, pAncBuffer_X[1].Size_U32, pAncBuffer_X[0].pIo_X);
                  }
                  for (i_U32 = 0; i_U32 < 2; i_U32++)
                  {
                    if ((pAncBuffer_X[i_U32].pBuffer_U8) && (pAncBuffer_X[i_U32].Size_U32))
                    {
                      Len_i = snprintf(pAncData_c, sizeof(pAncData_c), "Anc Sz %d Data: ", pAncBuffer_X[i_U32].Size_U32);
                      fwrite(pAncData_c, 1, Len_i, pAncBuffer_X[1].pIo_X);
                      for (j_U32 = 0; j_U32 < pAncBuffer_X[i_U32].Size_U32; j_U32++)
                      {
                        Len_i += snprintf(&pAncData_c[Len_i], sizeof(pAncData_c) - Len_i, "%02X ", pAncBuffer_X[i_U32].pBuffer_U8[j_U32]);
                      }
                      Len_i += snprintf(&pAncData_c[Len_i], sizeof(pAncData_c) - Len_i, "\n");
                      fwrite(pAncData_c, 1, Len_i, pAncBuffer_X[1].pIo_X);

                      AncCollection = EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::ParseBufferToListOfAncItems(pAncBuffer_X[i_U32].pBuffer_U8, pAncBuffer_X[i_U32].Size_U32);
                      if (AncCollection.size())
                      {
                        Len_i = snprintf(pAncData_c, sizeof(pAncData_c), "Anc Line Offs Strm Flg   Did Sdid Cnt  Data\n");
                        fwrite(pAncData_c, 1, Len_i, pAncBuffer_X[1].pIo_X);
                        for (j_U32 = 0; j_U32 < AncCollection.size(); j_U32++)
                        {
                          pAncFlag_c[0] = AncCollection[j_U32].flags.F ? 'F' : '.';
                          pAncFlag_c[1] = AncCollection[j_U32].flags.M ? 'M' : '.';
                          pAncFlag_c[2] = AncCollection[j_U32].flags.I ? 'I' : '.';
                          pAncFlag_c[3] = AncCollection[j_U32].flags.C ? 'C' : '.';
                          pAncFlag_c[4] = AncCollection[j_U32].flags.S ? 'S' : '.';
                          pAncFlag_c[5] = 0;

                          Len_i = snprintf(pAncData_c, sizeof(pAncData_c), "%03d %04d %04d %04d %s %03d %03d  %03d: ", j_U32, AncCollection[j_U32].lineNumber, AncCollection[j_U32].horizontalOffset, AncCollection[j_U32].streamNumber, pAncFlag_c, AncCollection[j_U32].did, AncCollection[j_U32].sdid, AncCollection[j_U32].dataCount);
                          for (k_U32 = 0; k_U32 < AncCollection[j_U32].dataCount; k_U32++)
                          {
                            Len_i += snprintf(&pAncData_c[Len_i], sizeof(pAncData_c) - Len_i, "%02X ", AncCollection[j_U32].data[k_U32]);
                          }
                          Len_i += snprintf(&pAncData_c[Len_i], sizeof(pAncData_c) - Len_i, "\n\n");
                          fwrite(pAncData_c, 1, Len_i, pAncBuffer_X[1].pIo_X);
                        }
                      }
                    }
                  }
                }
              }
              if (mAVaRecorderParam_X.SnapshotSave_B)
              {
                if (pVideoBuffer_X[0].pIo_X)
                {
                  fclose(pVideoBuffer_X[0].pIo_X);
                  pVideoBuffer_X[0].pIo_X = nullptr;
                }
                if (pProxyBuffer_X[0].pIo_X)
                {
                  fclose(pProxyBuffer_X[0].pIo_X);
                  pProxyBuffer_X[0].pIo_X = nullptr;
                }
                for (AudioChannelMask_U32 = 1, k_U32 = 0; k_U32 < AUDIO_NBRCH; k_U32++, AudioChannelMask_U32 <<= 1)
                {
                  if (AudioChannelMaskToCapture_U32 & AudioChannelMask_U32)
                  {
                    if (ppAudioBuffer_X[k_U32][0].pIo_X)
                    {
                      fclose(ppAudioBuffer_X[k_U32][0].pIo_X);
                      ppAudioBuffer_X[k_U32][0].pIo_X = nullptr;
                    }
                  }
                }
                if (pAncBuffer_X[0].pIo_X)
                {
                  fclose(pAncBuffer_X[0].pIo_X);
                  pAncBuffer_X[0].pIo_X = nullptr;
                }
                if (pAncBuffer_X[1].pIo_X)
                {
                  fclose(pAncBuffer_X[1].pIo_X);
                  pAncBuffer_X[1].pIo_X = nullptr;
                }
              }
            }
          }
        }
        else
        {
          pFrame[f_U32] = nullptr;
          NbTimeOutRcv_U32++;
          ADD_MESSAGE(EvsHwLGPL::SEV_INFO, ">> Recv: TIMEOUT Frm %d Sz0 %d Bad %d To %d\n", NbFrameRcv_U32, NbSize0Rcv_U32, NbBadStatusRcv_U32, NbTimeOutRcv_U32);
        }
      }
      RequestNewRecorderItem(pFrame[0], pFrame[1]);
    }
    ADD_MESSAGE(EvsHwLGPL::SEV_INFO, ">> Recv: END Frm %d Sz0 %d Bad %d To %d\n", NbFrameRcv_U32, NbSize0Rcv_U32, NbBadStatusRcv_U32, NbTimeOutRcv_U32);
    GL_puBoardResourceManager->FreeBuffer((void **)&pMergedVideoField_U8);
    if (!mAVaRecorderParam_X.SnapshotSave_B)
    {
      if (pVideoBuffer_X[0].pIo_X)
      {
        fclose(pVideoBuffer_X[0].pIo_X);
        pVideoBuffer_X[0].pIo_X = nullptr;
      }
      if (pProxyBuffer_X[0].pIo_X)
      {
        fclose(pProxyBuffer_X[0].pIo_X);
        pProxyBuffer_X[0].pIo_X = nullptr;
      }
      for (AudioChannelMask_U32 = 1, k_U32 = 0; k_U32 < AUDIO_NBRCH; k_U32++, AudioChannelMask_U32 <<= 1)
      {
        if (AudioChannelMaskToCapture_U32 & AudioChannelMask_U32)
        {
          if (ppAudioBuffer_X[k_U32][0].pIo_X)
          {
            fclose(ppAudioBuffer_X[k_U32][0].pIo_X);
            ppAudioBuffer_X[k_U32][0].pIo_X = nullptr;
          }
        }
      }
      if (pAncBuffer_X[0].pIo_X)
      {
        fclose(pAncBuffer_X[0].pIo_X);
        pAncBuffer_X[0].pIo_X = nullptr;
      }
      if (pAncBuffer_X[1].pIo_X)
      {
        fclose(pAncBuffer_X[1].pIo_X);
        pAncBuffer_X[1].pIo_X = nullptr;
      }
    }
  }
  mRecorderChannel_X.pIRecorder->UnregisterInputStatusChangeEvent(this);
  mRecorderChannel_X.pIRecorder->UnregisterFrameReceivedEvent(this);
  // CloseRecorder();
}
