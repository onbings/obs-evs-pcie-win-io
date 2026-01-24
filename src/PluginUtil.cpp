/*
obs-evs-pcie-win-io plugin
Copyright (C) 2026 Bernard HARMEL b.harmel@gmail.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/
#include <nlohmann/json.hpp>
#include <PluginUtil.h>
#include <map>
#include <fstream>
#include <obs-module.h>
#include <plugin-support.h>
#include "SimpleRecorder.h"

#include <libevs-pcie-win-io-api/src/EvsPcieIoHelpers.h>
static std::map<std::string, EVS::EvsPcieIoApi::eVideoStandard> S_VideoStandardCollection = {
    {"625i", EVS::EvsPcieIoApi::VideoStd_625i},
    {"525i,", EVS::EvsPcieIoApi::VideoStd_525i},
    {"720p_50,", EVS::EvsPcieIoApi::VideoStd_720p_50},
    {"720p_59_94", EVS::EvsPcieIoApi::VideoStd_720p_59_94},
    {"720p_60", EVS::EvsPcieIoApi::VideoStd_720p_60},
    {"1080i_47_95", EVS::EvsPcieIoApi::VideoStd_1080i_47_95},
    {"1080i_48", EVS::EvsPcieIoApi::VideoStd_1080i_48},
    {"1080i_50", EVS::EvsPcieIoApi::VideoStd_1080i_50},
    {"1080i_59_94", EVS::EvsPcieIoApi::VideoStd_1080i_59_94},
    {"1080i_60", EVS::EvsPcieIoApi::VideoStd_1080i_60},
    {"1080p_23_98", EVS::EvsPcieIoApi::VideoStd_1080p_23_98},
    {"1080p_24", EVS::EvsPcieIoApi::VideoStd_1080p_24},
    {"1080p_25", EVS::EvsPcieIoApi::VideoStd_1080p_25},
    {"1080p_29_97", EVS::EvsPcieIoApi::VideoStd_1080p_29_97},
    {"1080p_30", EVS::EvsPcieIoApi::VideoStd_1080p_30},
    {"1080p_47_95", EVS::EvsPcieIoApi::VideoStd_1080p_47_95},
    {"1080p_48", EVS::EvsPcieIoApi::VideoStd_1080p_48},
    {"1080p_59_94", EVS::EvsPcieIoApi::VideoStd_1080p_59_94},
    {"1080p_60", EVS::EvsPcieIoApi::VideoStd_1080p_60},
    {"1080p_50", EVS::EvsPcieIoApi::VideoStd_1080p_50},
    {"1080psf_23_98", EVS::EvsPcieIoApi::VideoStd_1080psf_23_98},
    {"1080psf_24", EVS::EvsPcieIoApi::VideoStd_1080psf_24},
    {"1080psf_25", EVS::EvsPcieIoApi::VideoStd_1080psf_25},
    {"1080psf_29_97", EVS::EvsPcieIoApi::VideoStd_1080psf_29_97},
    {"1080psf_30", EVS::EvsPcieIoApi::VideoStd_1080psf_30},
    {"2160p_23_98", EVS::EvsPcieIoApi::VideoStd_2160p_23_98},
    {"2160p_24", EVS::EvsPcieIoApi::VideoStd_2160p_24},
    {"2160p_25", EVS::EvsPcieIoApi::VideoStd_2160p_25},
    {"2160p_29_97", EVS::EvsPcieIoApi::VideoStd_2160p_29_97},
    {"2160p_30", EVS::EvsPcieIoApi::VideoStd_2160p_30},
    {"2160p_47_95", EVS::EvsPcieIoApi::VideoStd_2160p_47_95},
    {"2160p_48", EVS::EvsPcieIoApi::VideoStd_2160p_48},
    {"2160p_50", EVS::EvsPcieIoApi::VideoStd_2160p_50},
    {"2160p_59_94", EVS::EvsPcieIoApi::VideoStd_2160p_59_94},
    {"2160p_60", EVS::EvsPcieIoApi::VideoStd_2160p_60},
    {"2160p_23_98_4c", EVS::EvsPcieIoApi::VideoStd_2160p_23_98_4c}, //!< Unofficial std which can be confused with 1080p23.98
    {"2160p_24_4c", EVS::EvsPcieIoApi::VideoStd_2160p_24_4c},       //!< Unofficial std which can be confused with 1080p24
    {"2160p_25_4c", EVS::EvsPcieIoApi::VideoStd_2160p_25_4c},       //!< Unofficial std which can be confused with 1080p25
    {"2160p_29_97_4c", EVS::EvsPcieIoApi::VideoStd_2160p_29_97_4c}, //!< Unofficial std which can be confused with 1080p29.97
    {"2160p_30_4c", EVS::EvsPcieIoApi::VideoStd_2160p_30_4c},       //!< Unofficial std which can be confused with 1080p30
    {"2160p_47_95_4c", EVS::EvsPcieIoApi::VideoStd_2160p_47_95_4c},
    {"2160p_48_4c", EVS::EvsPcieIoApi::VideoStd_2160p_48_4c},
    {"2160p_59_94_4c", EVS::EvsPcieIoApi::VideoStd_2160p_59_94_4c},
    {"2160p_60_4c", EVS::EvsPcieIoApi::VideoStd_2160p_60_4c},
    {"2160p_50_4c", EVS::EvsPcieIoApi::VideoStd_2160p_50_4c}};

int64_t ConvertStringToInt(const char *_pInputString_c)
{
  int64_t Rts_S64 = 0;
  if (_pInputString_c == nullptr)
  {
    throw std::invalid_argument("Input string is null");
  }
  else
  {
    // Check for hexadecimal prefix "0x" or "0X"
    if (std::strncmp(_pInputString_c, "0x", 2) == 0 || std::strncmp(_pInputString_c, "0X", 2) == 0)
    {
      Rts_S64 = std::strtoll(_pInputString_c, nullptr, 16);
    }
    // Check for binary prefix "0b" or "0B"
    else if (std::strncmp(_pInputString_c, "0b", 2) == 0 || std::strncmp(_pInputString_c, "0B", 2) == 0)
    {
      Rts_S64 = std::strtoll(_pInputString_c + 2, nullptr, 2);
    }
    // Otherwise, assume decimal
    else
    {
      Rts_S64 = std::strtoll(_pInputString_c, nullptr, 10);
    }
  }
  return Rts_S64;
}
bool LoadCliArgFromJson(const std::string &_rFilePath_S, CLIARG &_rCliArg_X)
{
  bool Rts_B = false;
  std::ifstream InFile(_rFilePath_S);
  nlohmann::json json;

  if (InFile)
  {
    try
    {
      InFile >> json;
      Rts_B = true;
    }
    catch (const std::exception &e)
    {
      obs_log(LOG_ERROR, "LoadCliArgFromJson: JSON parse error: %s", e.what());
    }
    if (Rts_B)
    {
      Rts_B = false;
      _rCliArg_X.Verbose_i = json.value("Verbose", 0);
      _rCliArg_X.SnapshotMode_i = json.value("SnapshotMode", 0);
      _rCliArg_X.AncStreamPresent_i = json.value("AncStreamPresent", 0);
      _rCliArg_X.NoSave_i = json.value("NoSave", 1);
      _rCliArg_X.HwInitAlreadyDone_i = json.value("HwInitAlreadyDone", 0);

      _rCliArg_X.BoardNumber_U32 = json.value("BoardNumber", 0);
      _rCliArg_X.PcieBufferSize_U32 = json.value("PcieBufferSize", 0);
      _rCliArg_X.UhdPcieBufferingSize_U32 = json.value("UhdPcieBufferingSize", 0);
      _rCliArg_X.PlayerNumber_U32 = json.value("PlayerNumber", 0);
      _rCliArg_X.RecorderNumber_U32 = json.value("RecorderNumber", 0);
      _rCliArg_X.TimeToRunInSec_U32 = json.value("TimeToRunInSec", 0);
      _rCliArg_X.NbToPreload_U32 = json.value("NbToPreload", 0);
      _rCliArg_X.VideoStandard_S = json.value("VideoStandard", "");
      _rCliArg_X.AudioChannelMask_U32 = json.value("AudioChannelMask", 0);
      _rCliArg_X.BaseDirectory_S = json.value("BaseDirectory", "./");
      auto It = S_VideoStandardCollection.find(_rCliArg_X.VideoStandard_S);
      if (It != S_VideoStandardCollection.end())
      {
        GL_CliArg_X.VideoStandard_E = It->second;
        Rts_B = true;
      }
    }
  }
  obs_log(LOG_INFO, "Verbose .............. %d\n", _rCliArg_X.Verbose_i);
  obs_log(LOG_INFO, "SnapshotMode ......... %d\n", _rCliArg_X.SnapshotMode_i);
  obs_log(LOG_INFO, "AncStreamPresent ..... %d\n", _rCliArg_X.AncStreamPresent_i);
  obs_log(LOG_INFO, "NoSave ............... %d\n", _rCliArg_X.NoSave_i);
  obs_log(LOG_INFO, "HwInitAlreadyDone .... %d\n", _rCliArg_X.HwInitAlreadyDone_i);

  obs_log(LOG_INFO, "BoardNumber .......... %d\n", _rCliArg_X.BoardNumber_U32);
  obs_log(LOG_INFO, "Pcie buffer size ..... %d\n", _rCliArg_X.PcieBufferSize_U32);
  obs_log(LOG_INFO, "Uhd Pcie buffer size . %d\n", _rCliArg_X.UhdPcieBufferingSize_U32);
  obs_log(LOG_INFO, "PlayerNumber ......... %d\n", _rCliArg_X.PlayerNumber_U32);
  obs_log(LOG_INFO, "RecorderNumber ....... %d\n", _rCliArg_X.RecorderNumber_U32);
  obs_log(LOG_INFO, "TimeToRunInSec ....... %d\n", _rCliArg_X.TimeToRunInSec_U32);
  obs_log(LOG_INFO, "NbToPreload .......... %d\n", _rCliArg_X.NbToPreload_U32);
  obs_log(LOG_INFO, "VideoStandard ........ '%s' (%d)\n", _rCliArg_X.VideoStandard_S.c_str(), _rCliArg_X.VideoStandard_E);
  obs_log(LOG_INFO, "AudioChannelMask ..... %d\n", _rCliArg_X.AudioChannelMask_U32);
  obs_log(LOG_INFO, "BaseDirectory ........ '%s'\n", _rCliArg_X.BaseDirectory_S.c_str());

  obs_log(LOG_INFO, "Json status .......... %d\n", Rts_B);

  return Rts_B;
}
ObsLogger::ObsLogger() : EvsHwLGPL::CLoggerSink()
{
}
ObsLogger::~ObsLogger()
{
}

int ObsLogger::Message(uint32_t _Severity_U32, const char *_pMsg_c, uint32_t _Size_U32)
{
  return ObsLogger::Message(_Severity_U32, 0, _pMsg_c, _Size_U32);
}

int ObsLogger::Message(uint32_t _Severity_U32, uint32_t _Module_U32, const char *_pMsg_c, uint32_t _Size_U32)
{
  (void)_Module_U32;
  int Rts_i = 0, ObsLogLevel_i;
  std::string LogMessage_S;

  switch (_Severity_U32)
  {
    case EvsHwLGPL::SEV_ERROR:
      LogMessage_S = "[error]   ";
      ObsLogLevel_i = LOG_ERROR;
      break;
    case EvsHwLGPL::SEV_WARNING:
      LogMessage_S = "[warning] ";
      ObsLogLevel_i = LOG_WARNING;
      break;
    case EvsHwLGPL::SEV_INFO:
      LogMessage_S = "[info]    ";
      ObsLogLevel_i = LOG_INFO;
      break;
    case EvsHwLGPL::SEV_VERBOSE:
      LogMessage_S = "[verbose] ";
      ObsLogLevel_i = LOG_DEBUG;
      break;
    case EvsHwLGPL::SEV_DEBUG:
      LogMessage_S = "[debug]   ";
      ObsLogLevel_i = LOG_DEBUG;
      break;
    default:
      LogMessage_S = "[?????]   ";
      ObsLogLevel_i = LOG_DEBUG;
      break;
  }
  EvsHwLGPL::CLoggerSink::FormatLogMessage(LogMessage_S, _pMsg_c);

  // Rts_i |= fwrite(LogMessage_S.c_str(), LogMessage_S.size(), sizeof(char), stdout);
  obs_log(ObsLogLevel_i, LogMessage_S.c_str());
  return Rts_i;
}

int TestRecorder()
{
  int Rts_i;
  uint32_t i_U32;
  std::unique_ptr<SimpleRecorder> puRecorder;
  AVa_RECORDER_PARAM RecorderParam_X;

  Rts_i = GL_puBoardResourceManager->OpenRecorder(GL_CliArg_X.RecorderNumber_U32);
  if (Rts_i == EVS::EvsPcieIoApi::NoError)
  {
    Rts_i = EVS::EvsPcieIoApi::RuntimeError;
    puRecorder =
        std::make_unique<SimpleRecorder>(GL_puBoardResourceManager->GetBoard(), GL_puBoardResourceManager->GetRecorder(GL_CliArg_X.RecorderNumber_U32));
    if (puRecorder)
    {
      RecorderParam_X.SaveVideo_B = (GL_CliArg_X.NoSave_i == 0);
      RecorderParam_X.Hr10bit_B = true;

      RecorderParam_X.SaveProxy_B = (GL_CliArg_X.NoSave_i == 0);
      RecorderParam_X.LrWidth_U16 = 640;
      RecorderParam_X.LrHeight_U16 = 480;
      RecorderParam_X.Lr10bit_B = false;

      RecorderParam_X.SaveAudio_B = (GL_CliArg_X.NoSave_i == 0);
      RecorderParam_X.AudioChannelMaskToCapture_U32 = GL_CliArg_X.AudioChannelMask_U32; // 0x000000C0; // Channel 6 & 7
      RecorderParam_X.SnapshotSave_B = GL_CliArg_X.SnapshotMode_i;
      RecorderParam_X.AudioAmplifier_U32 = RecorderParam_X.SnapshotSave_B ? 0 : 256; // 24bits sign extended to 32bits Full res

      RecorderParam_X.SaveAnc_B = (GL_CliArg_X.NoSave_i == 0) ? GL_CliArg_X.AncStreamPresent_i : false;
      RecorderParam_X.EnableAnc_B = GL_CliArg_X.AncStreamPresent_i;

      RecorderParam_X.BaseSaveDir_S = GL_CliArg_X.BaseDirectory_S; //"C:/tmp/";

      Rts_i = puRecorder->OpenRecorder(RecorderParam_X);
      if (Rts_i == EVS::EvsPcieIoApi::NoError)
      {
        // DWORD processID = GetCurrentProcessId(); // Replace with the target process ID
        // EvsHwLGPL::ListThreadProperties(processID);
        ADD_MESSAGE(EvsHwLGPL::SEV_INFO, ">>> --Recorder--------------run\n");
        for (i_U32 = 0; i_U32 < ((GL_CliArg_X.TimeToRunInSec_U32 * 1000) / POLL_TIME_IN_MS); i_U32++)
        {
          ADD_MESSAGE(EvsHwLGPL::SEV_INFO, ">>> TheRecorderLoop %d/%d\n", i_U32, ((GL_CliArg_X.TimeToRunInSec_U32 * 1000) / POLL_TIME_IN_MS));
          Sleep(POLL_TIME_IN_MS);
        }
        ADD_MESSAGE(EvsHwLGPL::SEV_INFO, ">>> --Recorder--------------stop\n");
        Rts_i = puRecorder->CloseRecorder();
      }
    }
  }

  return Rts_i;
}
ObsLogger *GL_pObsLogger = nullptr;
EvsHwLGPL::CMsgLogger *GL_pEvsPcieWinIoLogger = nullptr;
std::unique_ptr<BoardResourceManager> GL_puBoardResourceManager = nullptr;


// Global pointer to keep track of our output instance
obs_output_t *global_dumper = nullptr;
void on_menu_click(void *private_data)
{
  // 1. Create the instance of your plugin if it doesn't exist
  if (!global_dumper)
  {
    // "raw_dump_output" is the ID you defined in obs_output_info
    global_dumper = obs_output_create("raw_dump_output", "MyDumperInstance", nullptr, nullptr);
  }

  // 2. Toggle Start/Stop
  if (!obs_output_active(global_dumper))
  {
    if (obs_output_start(global_dumper))
    {
      blog(LOG_INFO, "Raw Dump Started!");
    }
  }
  else
  {
    obs_output_stop(global_dumper);
    blog(LOG_INFO, "Raw Dump Stopped!");
  }
}