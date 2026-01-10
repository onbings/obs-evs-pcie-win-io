#pragma once
#include <memory>

#include <libevs-pcie-win-io-api/src/EvsPcieIoApi.h>
#include <libevs-pcie-win-io-api/src/private/BaseAccess.h>
#include <libevs-pcie-win-io-api/src/EvsPcieIoHelpers.h>

#define container_of(ptr, type, member) (reinterpret_cast<type *>(reinterpret_cast<char *>(ptr) - offsetof(type, member)))

constexpr uint32_t EVS_PA1022_NB_BOARD = 4;
constexpr uint32_t EVS_PA1022_NB_IN = 8;
constexpr uint32_t EVS_PA1022_NB_OUT = 4;
constexpr uint32_t POLL_TIME_IN_MS = 1000;

struct AVa_BUFFER
{
  uint32_t Size_U32;
  uint8_t *pBuffer_U8;
  FILE *pIo_X;

  AVa_BUFFER()
  {
    Reset();
  }
  void Reset()
  {
    Size_U32 = 0;
    pBuffer_U8 = nullptr;
    pIo_X = nullptr;
  }
};

struct CLIARG
{
  // Flag
  int Verbose_i;
  int SnapshotMode_i;
  int AncStreamPresent_i;
  int NoSave_i;
  int HwInitAlreadyDone_i;
  // Var
  uint32_t BoardNumber_U32;
  uint32_t PcieBufferSize_U32; // see CPA10XXPcieBoard::SetMemoryBufferingSize for unit (0 keep default)
  uint32_t UhdPcieBufferingSize_U32; // see CPA10XXPcieBoard::SetMemoryBufferingSize for unit (0 keep default)
  uint32_t PlayerNumber_U32;
  uint32_t RecorderNumber_U32;
  uint32_t TimeToRunInSec_U32;
  uint32_t NbToPreload_U32;
  std::string VideoStandard_S;
  EVS::EvsPcieIoApi::eVideoStandard VideoStandard_E;
  uint32_t AudioChannelMask_U32;
  std::string BaseDirectory_S;

  CLIARG()
  {
    Reset();
  }
  void Reset()
  {
    Verbose_i = 0;
    SnapshotMode_i = 0;
    AncStreamPresent_i = 0;
    NoSave_i = 0;
    HwInitAlreadyDone_i = 0;

    BoardNumber_U32 = 0;
    PcieBufferSize_U32 = 0;
    UhdPcieBufferingSize_U32 = 0;
    PlayerNumber_U32 = 0;
    RecorderNumber_U32 = 0;
    TimeToRunInSec_U32 = 0;
    NbToPreload_U32 = 0;
    VideoStandard_S = "";
    VideoStandard_E = EVS::EvsPcieIoApi::VideoStd_Invalid;
    AudioChannelMask_U32 = 0;
    BaseDirectory_S = "";
  }
};
extern CLIARG GL_CliArg_X;

class BoardResourceManager
{
public:
  BoardResourceManager(uint8_t _BoardNumber_U8);
  ~BoardResourceManager();

  EVS::EvsPcieIoApi::IEvsIoBoard *GetBoard();

  int OpenRecorder(uint8_t _RecorderNumber_U8);
  EVS::EvsPcieIoApi::IRecorder *GetRecorder(uint8_t _RecorderNumber_U8);

  int OpenPlayer(uint8_t _PlayerNumber_U8);
  EVS::EvsPcieIoApi::IPlayer *GetPlayer(uint8_t _PlayerNumber_U8);

  int AllocateBuffer(void **_ppBuffer, size_t _Size, uint32_t _Alignment_U32);
  int FreeBuffer(void **_ppBuffer);
  int64_t LoadFile(size_t _DataSize, void *_pBuffer, size_t _Offset, const char *_pFilePath_c, size_t &_rFileSize, bool &_rEof_B);
  int InterleaveEvenOddField(uint32_t _NbLinePerField_U32, uint32_t _LineWidthInPixel_U32, void *_pSrcEvenFieldBuffer, void *_pSrcOddFieldBuffer,
                             void *_pDstMergedFieldBuffer);
  int DeInterleaveMergedField(uint32_t _NbLinePerField_U32, uint32_t _LineWidthInPixel_U32, void *_pSrcMergedFieldBuffer, void *_pDstEvenFieldBuffer,
                              void *_pDstOddFieldBuffer);
  int DeInterleaveMergedField(uint32_t _NbLinePerField_U32, uint32_t _LineWidthInPixel_U32, void *_pSrcMergedFieldBuffer, bool _EvenField_B,
                              void *_pDstFieldBuffer);
  float VideoStandardToFrameRate(EVS::EvsPcieIoApi::eVideoStandard _VideoStandard_E, EVS::EvsPcieIoApi::eFrameRate &_rInterleavedFrameRate_E);
  void PrintCounter();
  void ResetCounters();

private:
  int OpenApi();
  int OpenBoard(uint8_t _BoardNumber_U8);
  void Release();

  int mBoardNumber_i = -1;
  EVS::EvsPcieIoApi::IEvsIoBoard *mpBoard = nullptr;
  EVS::EvsPcieIoApi::IPlayer *mppPlayer[EVS_PA1022_NB_OUT] = {};
  EVS::EvsPcieIoApi::IRecorder *mppRecorder[EVS_PA1022_NB_IN] = {};
  std::shared_ptr<EVS::EvsPcieIoApi::CBaseAccess> mpsBar0 = nullptr;
};

extern std::unique_ptr<BoardResourceManager> GL_puBoardResourceManager;