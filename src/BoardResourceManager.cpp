#include <fcntl.h>
#include <io.h>

#include "BoardResourceManager.h"
#include <libevs-pcie-win-io-api/src/ioBoard/BasePcieIoBoard.h>

CLIARG GL_CliArg_X;

BoardResourceManager::BoardResourceManager(uint8_t _BoardNumber_U8)
{
	if (OpenApi() == EVS::EvsPcieIoApi::Errno::NoError) {
		if (OpenBoard(_BoardNumber_U8) == EVS::EvsPcieIoApi::Errno::NoError) {
		}
	}
}
BoardResourceManager::~BoardResourceManager()
{
	Release();
}
int BoardResourceManager::OpenApi()
{
	int Rts_i = EVS::EvsPcieIoApi::Errno::NoError;
	char *pLibName_c, *pDate_c, pApiVersion_c[128];
	uint32_t VersionMajor_U32, VersionMinor_U32, VersionBuild_U32;

	EVS::EvsPcieIoApi::EVS_PCIE_WIN_IO_PARAM ApiParam_X;
	ApiParam_X.PcieBufferingSize = GL_CliArg_X.PcieBufferSize_U32;
  ApiParam_X.UhdPcieBufferingSize = GL_CliArg_X.UhdPcieBufferingSize_U32;
	ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "Set pci buffering size to 0x%X for hd and to 0x%X for uhd\n",
		    ApiParam_X.PcieBufferingSize, ApiParam_X.UhdPcieBufferingSize);

	Rts_i = EVS::EvsPcieIoApi::InitApi(ApiParam_X);
	if (Rts_i == EVS::EvsPcieIoApi::Errno::NoError) {
		EVS::EvsPcieIoApi::GetVersion((const char **)&pLibName_c, &VersionMajor_U32, &VersionMinor_U32,
					      &VersionBuild_U32, (const char **)&pDate_c);
		EVS::EvsPcieIoApi::GetVersionStr(pApiVersion_c, sizeof(pApiVersion_c));
	}
	return Rts_i;
}

int BoardResourceManager::OpenBoard(uint8_t _BoardNumber_U8)
{
	int Rts_i = EVS::EvsPcieIoApi::Errno::InvalidArgument;
	EVS::EvsPcieIoApi::BOARD_TYPE BoardType_E;

	if ((_BoardNumber_U8 < EVS_PA1022_NB_BOARD) && (mpBoard == nullptr)) {
		BoardType_E = EVS::EvsPcieIoApi::DetermineBoardType(_BoardNumber_U8);
		if (BoardType_E == EVS::EvsPcieIoApi::BOARD_TYPE::BOARD_UNKNOWN) {
			Rts_i = EVS::EvsPcieIoApi::Errno::RuntimeError;
		} else {
			mpBoard = EVS::EvsPcieIoApi::CreateBoard(_BoardNumber_U8, BoardType_E, nullptr,
								 GL_CliArg_X.HwInitAlreadyDone_i);
			if (mpBoard == nullptr) {
				Rts_i = EVS::EvsPcieIoApi::Errno::RuntimeError;
			} else {
				Rts_i = mpBoard->Init();
				if (Rts_i == EVS::EvsPcieIoApi::Errno::NoError) {
					mpsBar0 = ((EVS::EvsPcieIoApi::CBasePcieIoBoard *)(mpBoard))
							  ->GetSpecificBaseAccess(0, 0);
					ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "[board %u] mpsBar0 %p\n", _BoardNumber_U8,
						    mpsBar0.get());
					if (mpsBar0) {
						mBoardNumber_i = _BoardNumber_U8;
						//!!!Locate where is the board 0 in case of multi board config ....!!!
						if (mBoardNumber_i == 0) {
							Rts_i = mpBoard->ConfigureRefType(
								EVS::EvsPcieIoApi::eRefType::
									RefTypeAnalogMaster); // Light ref led in green
							ADD_MESSAGE(
								EvsHwLGPL::SEV_INFO,
								"[board %u] ConfigureRefType(EVS::EvsPcieIoApi::eRefType::RefTypeAnalogMaster)->%d\n",
								_BoardNumber_U8, Rts_i);
						} else {
							Rts_i = mpBoard->ConfigureRefType(
								EVS::EvsPcieIoApi::eRefType::
									RefTypeSlave); // Light ref led in orange
							ADD_MESSAGE(
								EvsHwLGPL::SEV_INFO,
								"[board %u] ConfigureRefType(EVS::EvsPcieIoApi::eRefType::RefTypeSlave)->%d\n",
								_BoardNumber_U8, Rts_i);
						}
					}
				}
			}
		}
	}
	return Rts_i;
}
EVS::EvsPcieIoApi::IEvsIoBoard *BoardResourceManager::GetBoard()
{
	return mpBoard;
}
EVS::EvsPcieIoApi::IRecorder *BoardResourceManager::GetRecorder(uint8_t _RecorderNumber_U8)
{
	return (_RecorderNumber_U8 < EVS_PA1022_NB_IN) ? mppRecorder[_RecorderNumber_U8] : nullptr;
}

EVS::EvsPcieIoApi::IPlayer *BoardResourceManager::GetPlayer(uint8_t _PlayerNumber_U8)
{
	return (_PlayerNumber_U8 < EVS_PA1022_NB_IN) ? mppPlayer[_PlayerNumber_U8] : nullptr;
}
void BoardResourceManager::Release()
{
	uint8_t i_U8;

	if (mBoardNumber_i >= 0) {
		for (i_U8 = 0; i_U8 < EVS_PA1022_NB_IN; i_U8++) {
			if (mppRecorder[i_U8] != nullptr) {
				mppRecorder[i_U8]->Close();
				mpBoard->ReleaseRecorder(mppRecorder[i_U8]);
				mppRecorder[i_U8] = nullptr;
			}
		}
		for (i_U8 = 0; i_U8 < EVS_PA1022_NB_OUT; i_U8++) {
			if (mppPlayer[i_U8] != nullptr) {
				mppPlayer[i_U8]->Close();
				mpBoard->ReleasePlayer(mppPlayer[i_U8]);
				mppPlayer[i_U8] = nullptr;
			}
		}
		if (mpBoard != nullptr) {
			// First call exit
			mpBoard->Exit();

			// then unallocate board
			delete mpBoard;
			mpBoard = nullptr;
		}

		mBoardNumber_i = -1;
	}
}

void BoardResourceManager::PrintCounter()
{
	/* ================================================
   *                 Read register
   * ================================================*/
	union {
		struct {
			uint32_t pcieWidth : 4;
			uint32_t pcieGen : 4;
			uint32_t retraining : 8;
			uint32_t empty : 8;
			uint32_t recDmaStop : 7;
			uint32_t recDmaStopWrap : 1;
		};

		uint32_t val;
	} reg;

	reg.val = mpsBar0->read32(0x4101c);

	ADD_MESSAGE(EvsHwLGPL::SEV_INFO,
		    "[board %u] pci width %02u gen %02u retraining=%03u, dma stop=%03u, cnt overflow=%3s\n",
		    mBoardNumber_i, reg.pcieWidth, reg.pcieGen, reg.retraining, reg.recDmaStop,
		    (reg.recDmaStopWrap ? "yes" : "no"));
}
void BoardResourceManager::ResetCounters()
{
	mpsBar0->write32(0x4101c, 1);
	ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "[board %u] DMA Stop counter reseted\n", mBoardNumber_i);
}
int BoardResourceManager::OpenRecorder(uint8_t _RecorderNumber_U8)
{
	int Rts_i = EVS::EvsPcieIoApi::Errno::NotConfigured;
	uint32_t TotalNbRecorder_U32;

	if (mBoardNumber_i >= 0) {
		if (mpBoard == nullptr) {
			Rts_i = EVS::EvsPcieIoApi::Errno::NotConfigured;
		} else {
			Rts_i = EVS::EvsPcieIoApi::Errno::NoError;
			TotalNbRecorder_U32 = mpBoard->GetNbInputChannels();
			if (_RecorderNumber_U8 > TotalNbRecorder_U32) {
				Rts_i = EVS::EvsPcieIoApi::Errno::InvalidArgument;
			} else {
				mppRecorder[_RecorderNumber_U8] = mpBoard->RequestNewRecorder(
					_RecorderNumber_U8, false); // only ask for non-UHD recorder
				if (mppRecorder[_RecorderNumber_U8] == nullptr) {
					Rts_i = EVS::EvsPcieIoApi::Errno::RuntimeError;
				}
			}
		}
	}
	return Rts_i;
}
int BoardResourceManager::OpenPlayer(uint8_t _PlayerNumber_U8)
{
	int Rts_i = EVS::EvsPcieIoApi::Errno::NotConfigured;
	uint32_t TotalNbPlayer_U32;

	if (mBoardNumber_i >= 0) {
		if (mpBoard == nullptr) {
			Rts_i = EVS::EvsPcieIoApi::Errno::NotConfigured;
		} else {
			Rts_i = EVS::EvsPcieIoApi::Errno::NoError;
			TotalNbPlayer_U32 = mpBoard->GetNbOutputChannels();
			if (_PlayerNumber_U8 > TotalNbPlayer_U32) {
				Rts_i = EVS::EvsPcieIoApi::Errno::InvalidArgument;
			} else {
				mppPlayer[_PlayerNumber_U8] = mpBoard->RequestNewPlayer(_PlayerNumber_U8);
				if (mppPlayer[_PlayerNumber_U8] == nullptr) {
					Rts_i = EVS::EvsPcieIoApi::Errno::RuntimeError;
				}
			}
		}
	}
	return Rts_i;
}

int BoardResourceManager::AllocateBuffer(void **_ppBuffer, size_t _Size, uint32_t _Alignment_U32)
{
	int Rts_i = EVS::EvsPcieIoApi::Errno::NoError;

	if ((!_ppBuffer) || (_Size <= 0) || ((_Alignment_U32 % 256) != 0)) {
		Rts_i = EVS::EvsPcieIoApi::Errno::InvalidArgument;
	} else {
		*_ppBuffer = _aligned_malloc(_Size, _Alignment_U32);
		if (*_ppBuffer == nullptr) {
			Rts_i = EVS::EvsPcieIoApi::Errno::RuntimeError;
		}
	}
	return Rts_i;
}
int BoardResourceManager::FreeBuffer(void **_ppBuffer)
{
	int Rts_i = EVS::EvsPcieIoApi::Errno::NoError;

	if ((!_ppBuffer) || (*_ppBuffer == nullptr)) {
		Rts_i = EVS::EvsPcieIoApi::Errno::InvalidArgument;
	} else {
		_aligned_free(*_ppBuffer);
		*_ppBuffer = nullptr;
	}
	return Rts_i;
}
int64_t BoardResourceManager::LoadFile(size_t _DataSize, void *_pBuffer, size_t _Offset, const char *_pFilePath_c,
				       size_t &_rFileSize, bool &_rEof_B)
{
	int64_t Rts;
	int Io_i;
	size_t ReadSize;

	_rFileSize = 0;
	_rEof_B = true;
	if ((!_pBuffer) || (!_pFilePath_c)) {
		ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "%s : pdata=%p, _DataSize=%llu, path=%s\n", __FUNCTION__, _pBuffer,
			    _DataSize, _pFilePath_c);
		Rts = -2;
	} else {
		Io_i = _open(_pFilePath_c, O_RDONLY | _O_BINARY);
		if (Io_i < 0) {
			ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "%s : cannot open file %s\n", __FUNCTION__, _pFilePath_c);
			Rts = -3;
		} else {
			_rFileSize = _lseek(Io_i, 0, SEEK_END);
			_lseek(Io_i, 0, SEEK_SET);

			if ((_Offset < 0) || (_Offset > _rFileSize)) {
				_Offset = _rFileSize;
			}
			if (_DataSize > 0) {
				if ((_DataSize + _Offset) < _rFileSize) {
					ReadSize = _DataSize;
					_rEof_B = false;
				} else {
					ReadSize = _rFileSize - _Offset;
				}
			} else {
				ReadSize = _rFileSize - _Offset;
			}
			Rts = -4;
			if (_lseek(Io_i, (long)_Offset, SEEK_SET) == _Offset) {
				Rts = _read(Io_i, _pBuffer, (uint32_t)ReadSize);
			}
			_close(Io_i);
		}
	}
	return Rts;
}
int BoardResourceManager::InterleaveEvenOddField(uint32_t _NbLinePerField_U32, uint32_t _LineWidthInPixel_U32,
						 void *_pSrcEvenFieldBuffer, void *_pSrcOddFieldBuffer,
						 void *_pDstMergedFieldBuffer)
{
	int Rts_i = EVS::EvsPcieIoApi::InvalidArgument;
	uint32_t i_U32;
	uint8_t *pSrcEvenField_U8, *pSrcOddField_U8, *pDstMergedField_U8;

	if ((_NbLinePerField_U32) && (_LineWidthInPixel_U32) && (_pSrcEvenFieldBuffer) && (_pSrcOddFieldBuffer) &&
	    (_pDstMergedFieldBuffer)) {
		Rts_i = EVS::EvsPcieIoApi::NoError;
		pSrcEvenField_U8 = (uint8_t *)_pSrcEvenFieldBuffer;
		pSrcOddField_U8 = (uint8_t *)_pSrcOddFieldBuffer;
		pDstMergedField_U8 = (uint8_t *)_pDstMergedFieldBuffer;
		for (i_U32 = 0; i_U32 < _NbLinePerField_U32; i_U32++) {
			memcpy(pDstMergedField_U8, pSrcEvenField_U8, _LineWidthInPixel_U32);
			pSrcEvenField_U8 += _LineWidthInPixel_U32;
			pDstMergedField_U8 += _LineWidthInPixel_U32;
			memcpy(pDstMergedField_U8, pSrcOddField_U8, _LineWidthInPixel_U32);
			pSrcOddField_U8 += _LineWidthInPixel_U32;
			pDstMergedField_U8 += _LineWidthInPixel_U32;
		}
	}
	return Rts_i;
}
int BoardResourceManager::DeInterleaveMergedField(uint32_t _NbLinePerField_U32, uint32_t _LineWidthInPixel_U32,
						  void *_pSrcMergedFieldBuffer, void *_pDstEvenFieldBuffer,
						  void *_pDstOddFieldBuffer)
{
	int Rts_i = EVS::EvsPcieIoApi::InvalidArgument;
	uint32_t i_U32;
	uint8_t *pSrcMergedField_U8, *pDstEvenField_U8, *pDstOddField_U8;

	if ((_NbLinePerField_U32) && (_LineWidthInPixel_U32) && (_pSrcMergedFieldBuffer) && (_pDstEvenFieldBuffer) &&
	    (_pDstOddFieldBuffer)) {
		Rts_i = EVS::EvsPcieIoApi::NoError;
		pSrcMergedField_U8 = (uint8_t *)_pSrcMergedFieldBuffer;
		pDstEvenField_U8 = (uint8_t *)_pDstEvenFieldBuffer;
		pDstOddField_U8 = (uint8_t *)_pDstOddFieldBuffer;
		for (i_U32 = 0; i_U32 < _NbLinePerField_U32; i_U32++) {
			memcpy(pDstEvenField_U8, pSrcMergedField_U8, _LineWidthInPixel_U32);
			pSrcMergedField_U8 += _LineWidthInPixel_U32;
			pDstEvenField_U8 += _LineWidthInPixel_U32;
			memcpy(pDstOddField_U8, pSrcMergedField_U8, _LineWidthInPixel_U32);
			pSrcMergedField_U8 += _LineWidthInPixel_U32;
			pDstOddField_U8 += _LineWidthInPixel_U32;
		}
	}
	return Rts_i;
}
int BoardResourceManager::DeInterleaveMergedField(uint32_t _NbLinePerField_U32, uint32_t _LineWidthInPixel_U32,
						  void *_pSrcMergedFieldBuffer, bool _EvenField_B,
						  void *_pDstFieldBuffer)
{
	int Rts_i = EVS::EvsPcieIoApi::InvalidArgument;
	uint32_t i_U32;
	uint8_t *pSrcMergedField_U8, *pDstField_U8;

	if ((_NbLinePerField_U32) && (_LineWidthInPixel_U32) && (_pSrcMergedFieldBuffer) && (_pDstFieldBuffer)) {
		Rts_i = EVS::EvsPcieIoApi::NoError;
		pSrcMergedField_U8 = _EvenField_B ? (uint8_t *)_pSrcMergedFieldBuffer
						  : &((uint8_t *)_pSrcMergedFieldBuffer)[_LineWidthInPixel_U32];
		pDstField_U8 = (uint8_t *)_pDstFieldBuffer;
		for (i_U32 = 0; i_U32 < _NbLinePerField_U32; i_U32++) {
			memcpy(pDstField_U8, pSrcMergedField_U8, _LineWidthInPixel_U32);
			pSrcMergedField_U8 += _LineWidthInPixel_U32;
			pSrcMergedField_U8 += _LineWidthInPixel_U32;
			pDstField_U8 += _LineWidthInPixel_U32;
		}
	}
	return Rts_i;
}

float BoardResourceManager::VideoStandardToFrameRate(EVS::EvsPcieIoApi::eVideoStandard _VideoStandard_E,
						     EVS::EvsPcieIoApi::eFrameRate &_rInterleavedFrameRate_E)
{
	float Rts_f;
	EVS::EvsPcieIoApi::eFrameRate VideoFrameRate_E;
	bool IsProgressive_B = EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::IsProgressive(_VideoStandard_E);
	VideoFrameRate_E = EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::GetFrameRate(_VideoStandard_E);
	_rInterleavedFrameRate_E = VideoFrameRate_E;
	switch (VideoFrameRate_E) {
	case EVS::EvsPcieIoApi::Rate_23_98:
		Rts_f = 23.98f;
		if (!IsProgressive_B) {
			_rInterleavedFrameRate_E = EVS::EvsPcieIoApi::Rate_47_95;
		}
		break;
	case EVS::EvsPcieIoApi::Rate_24:
		Rts_f = 24.0f;
		if (!IsProgressive_B) {
			_rInterleavedFrameRate_E = EVS::EvsPcieIoApi::Rate_47_95;
		}
		break;
	case EVS::EvsPcieIoApi::Rate_25:
		Rts_f = 25.0f;
		if (!IsProgressive_B) {
			_rInterleavedFrameRate_E = EVS::EvsPcieIoApi::Rate_50;
		}
		break;
	case EVS::EvsPcieIoApi::Rate_29_97:
		Rts_f = 29.97f;
		if (!IsProgressive_B) {
			_rInterleavedFrameRate_E = EVS::EvsPcieIoApi::Rate_59_94;
		}
		break;
	case EVS::EvsPcieIoApi::Rate_30:
		Rts_f = 30.0f;
		if (!IsProgressive_B) {
			_rInterleavedFrameRate_E = EVS::EvsPcieIoApi::Rate_60;
		}
		break;
	case EVS::EvsPcieIoApi::Rate_47_95:
		Rts_f = 47.95f;
		if (!IsProgressive_B) {
			_rInterleavedFrameRate_E = EVS::EvsPcieIoApi::Rate_47_95;
		}
		break;
	case EVS::EvsPcieIoApi::Rate_48:
		Rts_f = 48.0f;
		if (!IsProgressive_B) {
			_rInterleavedFrameRate_E = EVS::EvsPcieIoApi::Rate_48;
		}
		break;
	case EVS::EvsPcieIoApi::Rate_50:
		Rts_f = 50.0f;
		if (!IsProgressive_B) {
			_rInterleavedFrameRate_E = EVS::EvsPcieIoApi::Rate_50;
		}
		break;
	case EVS::EvsPcieIoApi::Rate_59_94:
		Rts_f = 59.94f;
		if (!IsProgressive_B) {
			_rInterleavedFrameRate_E = EVS::EvsPcieIoApi::Rate_59_94;
		}
		break;
	case EVS::EvsPcieIoApi::Rate_60:
		Rts_f = 60.0f;
		if (!IsProgressive_B) {
			_rInterleavedFrameRate_E = EVS::EvsPcieIoApi::Rate_invalid;
		}
		break;
	default:
		Rts_f = 0.0f;
		if (!IsProgressive_B) {
			_rInterleavedFrameRate_E = EVS::EvsPcieIoApi::Rate_47_95;
		}
		break;
	}
	return Rts_f;
}