/*
Plugin Name
Copyright (C) <Year> <Developer> <Email Address>

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
#include <obs-module.h>
#include <plugin-support.h>
#include <BoardResourceManager.h>
#include <fstream>
#include "SimpleRecorder.h"
extern struct obs_source_info raw_10bit_info;
OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")
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
class CObsLogger : public EvsHwLGPL::CLoggerSink
{
public:
  CObsLogger() {}
  ~CObsLogger()  {}

private:
  int Message(uint32_t _Severity_U32, const char *_pMsg_c, uint32_t _Size_U32)
  {
    return CObsLogger::Message(_Severity_U32, 0, _pMsg_c, _Size_U32);
  }

  int Message(uint32_t _Severity_U32, uint32_t _Module_U32, const char *_pMsg_c, uint32_t _Size_U32)
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

    //Rts_i |= fwrite(LogMessage_S.c_str(), LogMessage_S.size(), sizeof(char), stdout);
    obs_log(ObsLogLevel_i, LogMessage_S.c_str());
    return Rts_i;
  }
};
// SMPTE color bars in YUV (BT.601) for 8 bars (White, Yellow, Cyan, Green, Magenta, Red, Blue, Black)
// Y, U, V values for each bar (8-bit, full range)
static const uint8_t BAR_Y[8] = {235, 210, 170, 145, 106, 81, 41, 16};
static const uint8_t BAR_U[8] = {128, 16, 166, 54, 202, 90, 240, 128};
static const uint8_t BAR_V[8] = {128, 146, 16, 34, 202, 218, 112, 128};

void fill_uyvy_color_bars(uint8_t *buffer, int width, int height)
{
  int bar_width = width / 8;
  for (int bar = 0; bar < 8; ++bar)
  {
    int x_start = bar * bar_width;
    int x_end = (bar == 7) ? width : (bar + 1) * bar_width; // last bar fills to end
    uint8_t y_val = BAR_Y[bar];
    uint8_t u_val = BAR_U[bar];
    uint8_t v_val = BAR_V[bar];
    for (int x = x_start; x < x_end; x += 2)
    {
      // For each column pair, fill all rows
      for (int y = 0; y < height; ++y)
      {
        uint8_t *row = buffer + y * width * 2;
        int offset = x * 2;
        // UYVY: U0 Y0 V0 Y1 (U and V shared for the pair)
        row[offset + 0] = u_val;
        row[offset + 1] = y_val;
        row[offset + 2] = v_val;
        row[offset + 3] = y_val;
      }
    }
  }
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
    puRecorder = std::make_unique<SimpleRecorder>(GL_puBoardResourceManager->GetBoard(), GL_puBoardResourceManager->GetRecorder(GL_CliArg_X.RecorderNumber_U32));
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
CObsLogger *GL_pObsLogger = nullptr;
EvsHwLGPL::CMsgLogger *GL_pEvsPcieWinIoLogger = nullptr;
std::unique_ptr<BoardResourceManager> GL_puBoardResourceManager = nullptr;

//home dir is C:\pro\obs-studio\bin\64bit
extern "C" bool obs_module_load(void)
{
  bool Rts_B = false;
  obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
  
  GL_pObsLogger = new CObsLogger();
  GL_pEvsPcieWinIoLogger = new EvsHwLGPL::CMsgLogger(GL_pObsLogger, 512);
  GL_pEvsPcieWinIoLogger->ChangeMsgSev(EvsHwLGPL::SEV_DEBUG);
  EvsHwLGPL::SetGlobalLoggerPointer(GL_pEvsPcieWinIoLogger);

  //char p[512];
  //DWORD l=GetCurrentDirectoryA(sizeof(p),p);
  //obs_log(LOG_INFO, "GetCurrentDirectoryA %d %s", l, p);

  Rts_B=LoadCliArgFromJson("../../obs-plugins/64bit/obs-evs-pcie-win-io-config.json", GL_CliArg_X);
  obs_log(LOG_INFO, "LoadCliArgFromJson returns %s", Rts_B ? "true" : "false");
  if (Rts_B)
  {
    Rts_B = false;
    if (!GL_CliArg_X.Verbose_i)
    {
      GL_pEvsPcieWinIoLogger->ChangeMsgSev(EvsHwLGPL::SEV_WARNING);
    }
    GL_puBoardResourceManager = std::make_unique<BoardResourceManager>(GL_CliArg_X.BoardNumber_U32);
    obs_log(LOG_INFO, "create BoardResourceManager for board %d: %p", GL_CliArg_X.BoardNumber_U32, GL_puBoardResourceManager.get());
    if (GL_puBoardResourceManager)
    {
      Rts_B = true;
    }
  }
  obs_register_source(&raw_10bit_info);
  obs_log(LOG_INFO, "obs_module_load returns %s", Rts_B ? "true" : "false");
  //ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "hello world\n");
  return Rts_B;
}

extern "C" void obs_module_unload(void)
{
  obs_log(LOG_INFO, "plugin unloaded");
  if (GL_pEvsPcieWinIoLogger)
  {
    EvsHwLGPL::SetGlobalLoggerPointer(nullptr);
    delete GL_pEvsPcieWinIoLogger;
    GL_pEvsPcieWinIoLogger = nullptr;
  }
  if (GL_pObsLogger)
  {
    delete GL_pObsLogger;
    GL_pObsLogger = nullptr;
  }
  if (GL_puBoardResourceManager)
  {
    GL_puBoardResourceManager.reset();
  }
}



#include <obs-module.h>
#include <cstdio>
#include <chrono>
#include <thread>

struct raw_10bit_source
{
  obs_source_t *source;
  FILE *file;
  uint32_t width;
  uint32_t height;
  uint32_t fps;
  size_t frame_size;
  uint64_t frame_count;

  // Buffer for one frame
  uint8_t *buffer;
};
constexpr const char *PLUGIN_INPUT_NAME = "obs-evs-pcie-win-io source";

// 1. Get Plugin Name
    static const char *raw_get_name(void *unused)
{
  obs_log(LOG_INFO, ">>>raw_get_name %p", unused);
  return PLUGIN_INPUT_NAME;   //"Raw 10-bit YUV File";
}

// 2. Create the Source
static void *raw_create(obs_data_t *settings, obs_source_t *source)
{
  auto *context = (raw_10bit_source *)bzalloc(sizeof(raw_10bit_source));
  context->source = source;

  // Hardcoded for your specific use case, or pull from settings
  //context->width = 1920;
  //context->height = 1080;
  context->width = 640;
  context->height = 480;
  context->fps = 60;

  // P010 format: Y plane (2 bytes/pixel) + UV plane (1 byte/pixel avg) = 3 bytes total per pixel
//  context->frame_size = context->width * context->height * 3;
  context->frame_size = context->width * context->height * 2;
  context->buffer = (uint8_t *)bmalloc(context->frame_size);
  fill_uyvy_color_bars(context->buffer, context->width, context->height);

  const char *p = obs_data_get_string(settings, "file_path");
  obs_log(LOG_INFO, ">>>raw_create obs_data_get_string '%s'", p);

  char path[512];
//  sprintf(path, "%s1920x1080@29.97i_clp_0.yuv10", GL_CliArg_X.BaseDirectory_S.c_str());
  sprintf(path, "%s640x480@59.94p_clp_0.yuv8", GL_CliArg_X.BaseDirectory_S.c_str());
  
  context->file = fopen(path, "rb");
  obs_log(LOG_INFO, ">>>raw_create '%s' %p", path, context->file);

  return context;
}

// 3. Destroy the Source
static void raw_destroy(void *data)
{
  auto *context = (raw_10bit_source *)data;
  obs_log(LOG_INFO, ">>>raw_destroy %p", context->file);
  if (context->file)
  {
    fclose(context->file);
  }
  bfree(context->buffer);
  bfree(context);
}

// 4. Main Video Loop (Tick)
static void raw_video_tick(void *data, float seconds)
{
  auto *context = (raw_10bit_source *)data;
  if (!context->file)
  {
    return;
  }

  // Read one frame
#if 0
  size_t read = fread(context->buffer, 1, context->frame_size, context->file);
  // Loop file if EOF
  if (read < context->frame_size)
  {
    fseek(context->file, 0, SEEK_SET);
    obs_log(LOG_INFO, ">>>raw_video_tick %p sz %d not enought", context->file, read);

    return;
  }
#endif
  struct obs_source_frame frame = {};
  frame.width = context->width;
  frame.height = context->height;
  frame.format = VIDEO_FORMAT_UYVY;   //VIDEO_FORMAT_P010;

  // P010 is semi-planar: Y in plane 0, interleaved UV in plane 1
//  frame.data[0] = context->buffer;
//  frame.data[1] = context->buffer + (context->width * context->height * 2);
  frame.data[0] = context->buffer;

  //frame.linesize[0] = context->width * 2;
  //frame.linesize[1] = context->width * 2;
  frame.linesize[0] = context->width * 2;

  // Timing using std::chrono
  auto duration = std::chrono::nanoseconds(1000000000 / context->fps);
  frame.timestamp = context->frame_count * duration.count();

  obs_source_output_video(context->source, &frame);
  context->frame_count++;
 // obs_log(LOG_INFO, ">>>raw_video_tick %p sz %d cnt %d ts %ld", context->file, read, context->frame_count, frame.timestamp);
  obs_log(LOG_INFO, ">>>raw_video_tick %p cnt %d ts %ld", context->file, context->frame_count, frame.timestamp);
}
static obs_properties_t *raw_get_properties(void *data)
{
  obs_log(LOG_INFO, ">>>raw_get_properties");
  obs_properties_t *props = obs_properties_create();

  // The first string "file_path" is the KEY used by obs_data_get_string
  obs_properties_add_path(props, "file_path", "Select Raw YUV File", OBS_PATH_FILE, "Raw Files (*.yuv8 *.yuv8)", nullptr);

  return props;
}
// 5. Register the Source Info
struct obs_source_info raw_10bit_info = {
    .id = PLUGIN_INPUT_NAME, //"obs-evs-pcie-win-io source",
    .type = OBS_SOURCE_TYPE_INPUT,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = raw_get_name,
    .create = raw_create,
    .destroy = raw_destroy,
    .get_width = [](void *data) { return ((raw_10bit_source *)data)->width; },
    .get_height = [](void *data) { return ((raw_10bit_source *)data)->height; },
    .video_tick = raw_video_tick,
};


