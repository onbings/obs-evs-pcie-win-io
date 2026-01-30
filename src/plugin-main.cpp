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
/*
srt://127.0.0.1:9001?mode=listener&latency=50000
*/
#include <winsock2.h>
#include <ws2tcpip.h>
#include <obs-module.h>
#include <plugin-support.h>
#include "PluginUtil.h"
#include "AudioVideoHelper.h"
#include "Shader.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")
static const char *PLUGIN_INPUT_NAME = "obs-evs-pcie-win-io video/audio pSource_X";
constexpr uint32_t PLUGIN_MAX_AUDIO_CHANNELS = 16;

// Property string constants
constexpr const char *PROP_VIDEO_SOURCE_TYPE = "VideoSourceType_E";
constexpr const char *PROP_VIDEO_SOURCE_LABEL = "Video Source";
constexpr const char *PROP_VIDEO_FILE_PATH = "video_file_path";
constexpr const char *PROP_VIDEO_FILE_LABEL = "Video YUV File";
constexpr const char *PROP_USE_10BIT_VIDEO = "IsVideoIn10bit_B";
constexpr const char *PROP_USE_10BIT_VIDEO_LABEL = "Use 10-bit Video (unchecked = 8-bit)";
constexpr const char *PROP_COLOR_BAR_MOVING = "IsLineInColorBarMoving_B";
constexpr const char *PROP_COLOR_BAR_MOVING_LABEL = "Moving Color Bar";
constexpr const char *PROP_AUDIO_SOURCE_TYPE = "AudioSourceType_E";
constexpr const char *PROP_AUDIO_SOURCE_LABEL = "Audio Source";
constexpr const char *PROP_AUDIO_FILE_PATH = "audio_file_path";
constexpr const char *PROP_AUDIO_FILE_LABEL = "Audio PCM File";
constexpr const char *PROP_SPLIT_AUDIO_MODE = "IsAudioSplitMode_B";
constexpr const char *PROP_SPLIT_AUDIO_MODE_LABEL = "Split Audio Mode (checked = 16 separate sources, unchecked = single 16-channel source)";
constexpr const char *PROP_SINE_ENABLED_FMT = "sine_enabled_%d";
constexpr const char *PROP_SINE_ENABLED_LABEL_FMT = "Channel %d Enabled";
constexpr const char *PROP_SINE_FREQUENCY_FMT = "sine_frequency_%d";
constexpr const char *PROP_SINE_FREQUENCY_LABEL_FMT = "Channel %d Frequency (Hz)";
constexpr const char *PROP_SINE_AMPLITUDE_FMT = "sine_amplitude_%d";
constexpr const char *PROP_SINE_AMPLITUDE_LABEL_FMT = "Channel %d Amplitude";
constexpr const char *PROP_ENABLED = "enabled";
constexpr const char *PROP_ENABLED_LABEL = "Enabled";
constexpr const char *PROP_FREQUENCY = "frequency";
constexpr const char *PROP_FREQUENCY_LABEL = "Frequency (Hz)";
constexpr const char *PROP_AMPLITUDE = "amplitude";
constexpr const char *PROP_AMPLITUDE_LABEL = "Amplitude";

// Source type labels
constexpr const char *SOURCE_BOARD_LABEL = "Board";
constexpr const char *SOURCE_FILE_LABEL = "File";
constexpr const char *SOURCE_COLOR_BAR_LABEL = "Color Bar";
constexpr const char *SOURCE_SINE_GENERATOR_LABEL = "Sine Generator";
enum VIDEO_SOURCE_TYPE
{
  VIDEO_SOURCE_BOARD = 0,
  VIDEO_SOURCE_FILE = 1,
  VIDEO_SOURCE_COLOR_BAR = 2
};
enum AUDIO_SOURCE_TYPE
{
  AUDIO_SOURCE_BOARD = 0,
  AUDIO_SOURCE_FILE = 1,
  AUDIO_SOURCE_SINE_GENERATOR = 2
};
// Shared Audio Engine structure to eliminate duplication
struct AudioEngine
{
  obs_source_t *pAudioSource_X;   // OBS source pointer
  uint32_t AudioSampleRate_U32;   // Audio sampling rate 48000 Hz
  size_t AudioFrameSize;          // Number of bytes in pAudioBuffer_U8
  uint8_t *pAudioBuffer_U8;       // Pointer to audio buffer
  double AudioRemainderSecond_lf; // Accumulator for audio samples producer
  double AudioPhase_lf;           // Audio phase for Sin generator

  // Single channel sine parameters (for AudioChannelSource)
  double AudioSinFrequency_lf; // Frequency for single channel
  double AudioSinAmplitude_lf; // Amplitude for single channel
  bool AudioChannelEnabled_B;  // Enable/disable for single channel

  // Multi-channel sine parameters (for MultiMediaSource)
  double pSineFrequency_lf[PLUGIN_MAX_AUDIO_CHANNELS];        // Frequency for each channel
  double pSineAmplitude_lf[PLUGIN_MAX_AUDIO_CHANNELS];        // Amplitude for each channel
  bool pSineEnabled_B[PLUGIN_MAX_AUDIO_CHANNELS];             // Enable/disable each channel
  double pSinePhase_lf[PLUGIN_MAX_AUDIO_CHANNELS];            // Phase tracking for each channel
  uint8_t *pAudioChannelBuffer_U8[PLUGIN_MAX_AUDIO_CHANNELS]; // Independent buffer for each channel
  uint32_t NbAudioChannel_U32;                                // Number of audio channels
  bool IsMultichannel_B;                                      // true=16-channel mode, false=single channel mode
};

// Structure for individual sine wave audio channels (Option 2)
struct AudioChannelSource
{
  AudioEngine PluginAudioEngine; // Shared audio engine
  int channel_number;      // Channel number (1-16)
};

struct MultiMediaSource
{
  obs_source_t *pMultiMediaSource_X; // Obs source pointer
  FILE *pVideoFile_X;                // Video file handle
  // Graphics resources
  gs_effect_t *pEffect_X;   // Shader effect
  gs_texture_t *pTexture_X; // Texture for video frame

  // Shader Parameters
  gs_eparam_t *pShaderParamImage_X; // Texture parameter
  gs_eparam_t *pShaderParamWidth_X; // Width parameter
  gs_eparam_t *pShaderParamAlpha_X; // Alpha parameter

  // Data handling
  uint8_t *pVideoBuffer_U8;   // Pointer to your video data buffer
  uint32_t VideoWidth_U32;    // Video Width (e.g. 1920)
  uint32_t VideoHeight_U32;   // Video Height (e.g. 1080)
  size_t VideoFrameSize;      //  Size of a single video frame in bytes
  float Alpha_f;              // The alpha value (default 255.0)
  uint32_t MovingLinePos_U32; // Current position of the line in the color bar

  // Audio members
  FILE *pAudioFile_X;      // Audio file handle
  AudioEngine PluginAudioEngine; // Shared audio engine
  bool IsAudioSplitMode_B;   // Runtime control: true = separate sources, false = multi-channel

  // Runtime pSource_X selection variables
  int VideoSourceType_E; // VIDEO_SOURCE_TYPE enum
  int AudioSourceType_E; // AUDIO_SOURCE_TYPE enum
  bool IsLineInColorBarMoving_B; // Moving color bar option
  bool IsVideoIn10bit_B;  // 10-bit vs 8-bit video

  // Perf
  std::chrono::microseconds::rep DeltaTickInUs;
  std::chrono::high_resolution_clock::time_point TickIn;
  std::chrono::high_resolution_clock::time_point TickOut;
  std::chrono::microseconds::rep TickElapsedTimeInUs;
  std::chrono::microseconds::rep DeltaRendererInUs;
  std::chrono::high_resolution_clock::time_point RendererIn;
  std::chrono::high_resolution_clock::time_point RendererOut;
  std::chrono::microseconds::rep RendererElapsedTimeInUs;
};

// Audio Engine Helper Functions
static void AudioEngineInit(AudioEngine *_pAudioEngine_X, obs_source_t *_pSource_X, bool _IsMultichannel_B)
{
  uint32_t i_U32;

  _pAudioEngine_X->pAudioSource_X = _pSource_X;
  _pAudioEngine_X->AudioSampleRate_U32 = 48000;
  _pAudioEngine_X->AudioFrameSize = sizeof(int32_t) * PLUGIN_MAX_AUDIO_CHANNELS * 16 * 1024;
  _pAudioEngine_X->pAudioBuffer_U8 = (uint8_t *)bmalloc(_pAudioEngine_X->AudioFrameSize);
  _pAudioEngine_X->AudioRemainderSecond_lf = 0.0;
  _pAudioEngine_X->AudioPhase_lf = 0.0;
  _pAudioEngine_X->IsMultichannel_B = _IsMultichannel_B;

  if (_IsMultichannel_B)
  {
    _pAudioEngine_X->NbAudioChannel_U32 = 16;
    for (i_U32 = 0; i_U32 < PLUGIN_MAX_AUDIO_CHANNELS; i_U32++)
    {
      _pAudioEngine_X->pSineFrequency_lf[i_U32] = 440.0 + (i_U32 * 110.0);
      _pAudioEngine_X->pSineAmplitude_lf[i_U32] = 0.3;
      _pAudioEngine_X->pSineEnabled_B[i_U32] = (i_U32 < 2);
      _pAudioEngine_X->pSinePhase_lf[i_U32] = 0.0;
      _pAudioEngine_X->pAudioChannelBuffer_U8[i_U32] = (uint8_t *)bmalloc(_pAudioEngine_X->AudioFrameSize);
    }
  }
  else
  {
    _pAudioEngine_X->NbAudioChannel_U32 = 1;
    _pAudioEngine_X->AudioSinFrequency_lf = 440.0;
    _pAudioEngine_X->AudioSinAmplitude_lf = 0.3;
    _pAudioEngine_X->AudioChannelEnabled_B = true;
    for (i_U32 = 0; i_U32 < PLUGIN_MAX_AUDIO_CHANNELS; i_U32++)
    {
      _pAudioEngine_X->pAudioChannelBuffer_U8[i_U32] = nullptr;
    }
  }
}

static void AudioEngineDestroy(AudioEngine *_pAudioEngine_X)
{
  uint32_t i_U32;

  if (_pAudioEngine_X->pAudioBuffer_U8)
  {
    bfree(_pAudioEngine_X->pAudioBuffer_U8);
    _pAudioEngine_X->pAudioBuffer_U8 = nullptr;
  }

    for (i_U32 = 0; i_U32 < PLUGIN_MAX_AUDIO_CHANNELS; i_U32++)
  {
      if (_pAudioEngine_X->pAudioChannelBuffer_U8[i_U32])
    {
        bfree(_pAudioEngine_X->pAudioChannelBuffer_U8[i_U32]);
      _pAudioEngine_X->pAudioChannelBuffer_U8[i_U32] = nullptr;
    }
  }
}

static void AudioEngineGenerateAudio(AudioEngine *_pAudioEngine_X, uint32_t _NbSampleToGenerate_U32)
{
  int32_t *pSample_S32 = (int32_t *)_pAudioEngine_X->pAudioBuffer_U8;

  if (_pAudioEngine_X->IsMultichannel_B)
  {
    // Multi-channel mode: generate interleaved 16-channel audio
    memset(pSample_S32, 0, _NbSampleToGenerate_U32 * 16 * sizeof(int32_t));

    for (int channel = 0; channel < 16; channel++)
    {
      if (_pAudioEngine_X->pSineEnabled_B[channel])
      {
        double amplitude = _pAudioEngine_X->pSineAmplitude_lf[channel] * 0x7FFFFFFF;

        for (uint32_t sample = 0; sample < _NbSampleToGenerate_U32; sample++)
        {
          int32_t sample_value = (int32_t)(amplitude * sin(_pAudioEngine_X->pSinePhase_lf[channel]));
          pSample_S32[sample * 16 + channel] = sample_value;

          _pAudioEngine_X->pSinePhase_lf[channel] += (2.0 * M_PI * _pAudioEngine_X->pSineFrequency_lf[channel]) / _pAudioEngine_X->AudioSampleRate_U32;
          if (_pAudioEngine_X->pSinePhase_lf[channel] > 2.0 * M_PI)
          {
            _pAudioEngine_X->pSinePhase_lf[channel] -= 2.0 * M_PI;
          }
        }
      }
    }
  }
  else
  {
    // Single channel mode
    if (_pAudioEngine_X->AudioChannelEnabled_B)
    {
      double amplitude = _pAudioEngine_X->AudioSinAmplitude_lf * 0x7FFFFFFF;
      GenerateAudioSinusData(_pAudioEngine_X->AudioSinFrequency_lf, amplitude, (double)_pAudioEngine_X->AudioSampleRate_U32, _NbSampleToGenerate_U32, pSample_S32,
                             _pAudioEngine_X->AudioPhase_lf);
    }
    else
    {
      memset(pSample_S32, 0, _NbSampleToGenerate_U32 * sizeof(int32_t));
    }
  }
}

static void AudioEngine_output_audio(AudioEngine *_pAudioEngine_X, uint32_t _NbSampleToGenerate_U32)
{
  struct obs_source_audio audio_frame = {0};
  audio_frame.data[0] = _pAudioEngine_X->pAudioBuffer_U8;
  audio_frame.frames = _NbSampleToGenerate_U32;

  if (_pAudioEngine_X->IsMultichannel_B)
  {
    audio_frame.speakers = SPEAKERS_UNKNOWN; // 16-channel layout
  }
  else
  {
    audio_frame.speakers = SPEAKERS_MONO;
  }

  audio_frame.samples_per_sec = _pAudioEngine_X->AudioSampleRate_U32;
  audio_frame.format = AUDIO_FORMAT_32BIT;
  audio_frame.timestamp = obs_get_video_frame_time() - util_mul_div64(_NbSampleToGenerate_U32, 1000000000ULL, _pAudioEngine_X->AudioSampleRate_U32);

  obs_source_output_audio(_pAudioEngine_X->pAudioSource_X, &audio_frame);
}

static const char *MultiMedia_get_name(void *type_data)
{
  obs_log(LOG_INFO, ">>>MultiMedia_get_name %p->%s", type_data, PLUGIN_INPUT_NAME);
  return PLUGIN_INPUT_NAME;
}

static void *MultiMedia_create(obs_data_t *settings, obs_source_t *pSource_X)
{
  MultiMediaSource *pContext_X = (MultiMediaSource *)bzalloc(sizeof(MultiMediaSource));
  obs_log(LOG_INFO, ">>>MultiMedia_create %p %p", settings, pSource_X);
  pContext_X->pMultiMediaSource_X = pSource_X;

  // Hardcoded for your specific use case, or pull from settings
  if (pContext_X->IsVideoIn10bit_B)
  {
    pContext_X->VideoWidth_U32 = 1920;
    pContext_X->VideoHeight_U32 = 1080;
    pContext_X->VideoFrameSize = EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::ComputeImageSize(EVS::EvsPcieIoApi::VideoStd_1080p_59_94, FOURCC_V210,
                                                                                          pContext_X->VideoWidth_U32, pContext_X->VideoHeight_U32, false);
    //  EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::ComputeImageSize(EVS::EvsPcieIoApi::VideoStd_1080i_59_94, FOURCC_V210, pContext_X->VideoWidth_U32,
    //  pContext_X->VideoHeight_U32, false);
    pContext_X->pVideoBuffer_U8 = (uint8_t *)bmalloc(pContext_X->VideoFrameSize);
    fill_uyvy_color_bars(pContext_X->pVideoBuffer_U8, pContext_X->VideoWidth_U32, pContext_X->VideoHeight_U32, true);
  }
  else
  {
    pContext_X->VideoWidth_U32 = 640;
    pContext_X->VideoHeight_U32 = 480;
    pContext_X->VideoFrameSize = pContext_X->VideoWidth_U32 * pContext_X->VideoHeight_U32 * 2;
    pContext_X->pVideoBuffer_U8 = (uint8_t *)bmalloc(pContext_X->VideoFrameSize);
    fill_uyvy_color_bars(pContext_X->pVideoBuffer_U8, pContext_X->VideoWidth_U32, pContext_X->VideoHeight_U32, false);
  }

  // Create texture
  obs_enter_graphics();
  pContext_X->pTexture_X = gs_texture_create(pContext_X->VideoWidth_U32, pContext_X->VideoHeight_U32, GS_BGRA, 1, NULL, GS_DYNAMIC);
  obs_leave_graphics();

  // Initialize audio engine for multi-channel mode
  AudioEngineInit(&pContext_X->PluginAudioEngine, pSource_X, true);

  // Initialize source selection variables
  pContext_X->IsAudioSplitMode_B = false;                        // Default to multi-channel mode
  pContext_X->VideoSourceType_E = VIDEO_SOURCE_COLOR_BAR;      // Default to Color Bar
  pContext_X->AudioSourceType_E = AUDIO_SOURCE_SINE_GENERATOR; // Default to Sine Generator
  pContext_X->IsLineInColorBarMoving_B = true;                         // Default moving bar
  pContext_X->IsVideoIn10bit_B = true;                          // Default 10-bit

  const char *p = obs_data_get_string(settings, "file_path");
  obs_log(LOG_INFO, ">>>MultiMedia_create obs_data_get_string '%s'", p);

  char path[512];
  if (pContext_X->IsVideoIn10bit_B)
  {
    sprintf(path, "%s1920x1080@29.97i_clp_0.yuv10", GL_CliArg_X.BaseDirectory_S.c_str());
  }
  else
  {
    sprintf(path, "%s640x480@59.94p_clp_0.yuv8", GL_CliArg_X.BaseDirectory_S.c_str());
  }
  pContext_X->pVideoFile_X = fopen(path, "rb");
  obs_log(LOG_INFO, ">>>MultiMedia_create video '%s' %p", path, pContext_X->pVideoFile_X);

  sprintf(path, "%s16xS24L32@48000_6_0.pcm", GL_CliArg_X.BaseDirectory_S.c_str());
  pContext_X->pAudioFile_X = fopen(path, "rb");
  obs_log(LOG_INFO, ">>>MultiMedia_create audio '%s' %p", path, pContext_X->pAudioFile_X);

  pContext_X->Alpha_f = 255.0f; // 128.0f; // 255.0f; // Default 0xFF

  // Compile the Shader
  obs_enter_graphics();
  char *errors = NULL;
  pContext_X->pEffect_X = gs_effect_create(GL_v210_unpacker_effect_code, "v210_unpacker_effect_embedded", &errors);

  if (errors)
  {
    obs_log(LOG_ERROR, ">>>MultiMedia_create Shader compile error: %s", errors);
    bfree(errors);
  }

  if (pContext_X->pEffect_X)
  {
    pContext_X->pShaderParamImage_X = gs_effect_get_param_by_name(pContext_X->pEffect_X, "image");
    pContext_X->pShaderParamWidth_X = gs_effect_get_param_by_name(pContext_X->pEffect_X, "width_pixels");
    pContext_X->pShaderParamAlpha_X = gs_effect_get_param_by_name(pContext_X->pEffect_X, "alpha_val");
  }
  obs_leave_graphics();
  obs_log(LOG_INFO, ">>>MultiMedia_create ->%p", pContext_X);
  return pContext_X;
}

static void MultiMedia_destroy(void *_pData)
{
  MultiMediaSource *pContext_X = (MultiMediaSource *)_pData;

  obs_log(LOG_INFO, ">>>MultiMedia_destroy %p", _pData);
  if (pContext_X->pVideoFile_X)
  {
    fclose(pContext_X->pVideoFile_X);
    pContext_X->pVideoFile_X = nullptr;
  }
  if (pContext_X->pAudioFile_X)
  {
    fclose(pContext_X->pAudioFile_X);
    pContext_X->pAudioFile_X = nullptr;
  }
  bfree(pContext_X->pVideoBuffer_U8);
  pContext_X->pVideoBuffer_U8 = nullptr;

  // Clean up audio engine
  AudioEngineDestroy(&pContext_X->PluginAudioEngine);

  obs_enter_graphics();
  if (pContext_X->pTexture_X)
  {
    gs_texture_destroy(pContext_X->pTexture_X);
    pContext_X->pTexture_X = nullptr;
  }
  if (pContext_X->pEffect_X)
  {
    gs_effect_destroy(pContext_X->pEffect_X);
    pContext_X->pEffect_X = nullptr;
  }
  obs_leave_graphics();

  bfree(pContext_X);
  pContext_X = nullptr;
}

static uint32_t MultiMedia_get_width(void *_pData)
{
  MultiMediaSource *pContext_X = (MultiMediaSource *)_pData;
  // obs_log(LOG_INFO, ">>>MultiMedia_get_width %p->%d", _pData, pContext_X->VideoWidth_U32);
  return pContext_X->VideoWidth_U32;
}

static uint32_t MultiMedia_get_height(void *_pData)
{
  MultiMediaSource *pContext_X = (MultiMediaSource *)_pData;
  // obs_log(LOG_INFO, ">>>MultiMedia_get_height %p->%d", _pData, pContext_X->VideoHeight_U32);
  return pContext_X->VideoHeight_U32;
}

static obs_properties_t *MultiMedia_get_properties(void *_pData)
{
  uint32_t i_U32;

  // _pData is your MultiMediaSource* pContext_X
  obs_properties_t *pProperty_X = obs_properties_create();
  obs_log(LOG_INFO, ">>>MultiMedia_get_properties %p->%p", _pData, pProperty_X);

  // Video Source Selection
  obs_property_t *video_source =
      obs_properties_add_list(pProperty_X, PROP_VIDEO_SOURCE_TYPE, PROP_VIDEO_SOURCE_LABEL, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
  obs_property_list_add_int(video_source, SOURCE_BOARD_LABEL, 0);
  obs_property_list_add_int(video_source, SOURCE_FILE_LABEL, 1);
  obs_property_list_add_int(video_source, SOURCE_COLOR_BAR_LABEL, 2);

  // Video options
  obs_properties_add_path(pProperty_X, PROP_VIDEO_FILE_PATH, PROP_VIDEO_FILE_LABEL, OBS_PATH_FILE, "Raw Files (*.yuv8 *.yuv10)", nullptr);
  obs_properties_add_bool(pProperty_X, PROP_USE_10BIT_VIDEO, PROP_USE_10BIT_VIDEO_LABEL);
  obs_properties_add_bool(pProperty_X, PROP_COLOR_BAR_MOVING, PROP_COLOR_BAR_MOVING_LABEL);

  // Audio Source Selection
  obs_property_t *audio_source =
      obs_properties_add_list(pProperty_X, PROP_AUDIO_SOURCE_TYPE, PROP_AUDIO_SOURCE_LABEL, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
  obs_property_list_add_int(audio_source, SOURCE_BOARD_LABEL, 0);
  obs_property_list_add_int(audio_source, SOURCE_FILE_LABEL, 1);
  obs_property_list_add_int(audio_source, SOURCE_SINE_GENERATOR_LABEL, 2);

  // Audio options
  obs_properties_add_path(pProperty_X, PROP_AUDIO_FILE_PATH, PROP_AUDIO_FILE_LABEL, OBS_PATH_FILE, "PCM Files (*.pcm)", nullptr);
  obs_properties_add_bool(pProperty_X, PROP_SPLIT_AUDIO_MODE, PROP_SPLIT_AUDIO_MODE_LABEL);

  // Multi-channel sine wave controls
  for (i_U32 = 0; i_U32 < PLUGIN_MAX_AUDIO_CHANNELS; i_U32++)
  {
    char prop_name[64];
    char prop_desc[64];

    sprintf(prop_name, PROP_SINE_ENABLED_FMT, i_U32 + 1);
    sprintf(prop_desc, PROP_SINE_ENABLED_LABEL_FMT, i_U32 + 1);
    obs_properties_add_bool(pProperty_X, prop_name, prop_desc);

    sprintf(prop_name, PROP_SINE_FREQUENCY_FMT, i_U32 + 1);
    sprintf(prop_desc, PROP_SINE_FREQUENCY_LABEL_FMT, i_U32 + 1);
    obs_properties_add_float_slider(pProperty_X, prop_name, prop_desc, 20.0, 20000.0, 10.0);

    sprintf(prop_name, PROP_SINE_AMPLITUDE_FMT, i_U32 + 1);
    sprintf(prop_desc, PROP_SINE_AMPLITUDE_LABEL_FMT, i_U32 + 1);
    obs_properties_add_float_slider(pProperty_X, prop_name, prop_desc, 0.0, 1.0, 0.01);
  }

  return pProperty_X;
}
static void MultiMedia_get_defaults(obs_data_t *settings)
{
  uint32_t i_U32;

  obs_log(LOG_INFO, ">>>MultiMedia_get_defaults %p", settings);

  // Video pSource_X defaults
  obs_data_set_default_int(settings, PROP_VIDEO_SOURCE_TYPE, 2); // Default to Color Bar
  obs_data_set_default_bool(settings, PROP_USE_10BIT_VIDEO, true);
  obs_data_set_default_bool(settings, PROP_COLOR_BAR_MOVING, true);

  // Audio pSource_X defaults
  obs_data_set_default_int(settings, PROP_AUDIO_SOURCE_TYPE, 2);     // Default to Sine Generator
  obs_data_set_default_bool(settings, PROP_SPLIT_AUDIO_MODE, false); // Default to multi-channel mode

  // Set defaults for multi-channel sine waves
  for (i_U32 = 0; i_U32 < PLUGIN_MAX_AUDIO_CHANNELS; i_U32++)
  {
    char prop_name[64];

    sprintf(prop_name, PROP_SINE_ENABLED_FMT, i_U32 + 1);
    obs_data_set_default_bool(settings, prop_name, (i_U32 < 2)); // Enable first 2 channels

    sprintf(prop_name, PROP_SINE_FREQUENCY_FMT, i_U32 + 1);
    obs_data_set_default_double(settings, prop_name, 440.0 + (i_U32 * 110.0)); // Different frequencies

    sprintf(prop_name, PROP_SINE_AMPLITUDE_FMT, i_U32 + 1);
    obs_data_set_default_double(settings, prop_name, 0.3);
  }
}
static void MultiMedia_update(void *_pData, obs_data_t *settings)
{
  auto *pContext_X = (MultiMediaSource *)_pData;
  uint32_t i_U32;
  obs_log(LOG_INFO, ">>>MultiMedia_update %p %p", _pData, settings);

  // Update pSource_X selection settings
  pContext_X->VideoSourceType_E = (int)obs_data_get_int(settings, PROP_VIDEO_SOURCE_TYPE);
  pContext_X->AudioSourceType_E = (int)obs_data_get_int(settings, PROP_AUDIO_SOURCE_TYPE);
  pContext_X->IsVideoIn10bit_B = obs_data_get_bool(settings, PROP_USE_10BIT_VIDEO);
  pContext_X->IsLineInColorBarMoving_B = obs_data_get_bool(settings, PROP_COLOR_BAR_MOVING);
  pContext_X->IsAudioSplitMode_B = obs_data_get_bool(settings, PROP_SPLIT_AUDIO_MODE);

  // Update multi-channel sine wave settings
  for (i_U32 = 0; i_U32 < PLUGIN_MAX_AUDIO_CHANNELS; i_U32++)
  {
    char prop_name[64];

    sprintf(prop_name, PROP_SINE_ENABLED_FMT, i_U32 + 1);
    pContext_X->PluginAudioEngine.pSineEnabled_B[i_U32] = obs_data_get_bool(settings, prop_name);

    sprintf(prop_name, PROP_SINE_FREQUENCY_FMT, i_U32 + 1);
    pContext_X->PluginAudioEngine.pSineFrequency_lf[i_U32] = obs_data_get_double(settings, prop_name);

    sprintf(prop_name, PROP_SINE_AMPLITUDE_FMT, i_U32 + 1);
    pContext_X->PluginAudioEngine.pSineAmplitude_lf[i_U32] = obs_data_get_double(settings, prop_name);
  }

  // 1. Handle Video File Update
  const char *video_path = obs_data_get_string(settings, PROP_VIDEO_FILE_PATH);
  if (video_path && *video_path)
  {
    if (pContext_X->pVideoFile_X)
    {
      fclose(pContext_X->pVideoFile_X);
      pContext_X->pVideoFile_X = nullptr;
    }
    pContext_X->pVideoFile_X = fopen(video_path, "rb");
    obs_log(LOG_INFO, "Video pSource_X updated to: %s", video_path);
  }

  // 2. Handle Audio File Update
  const char *audio_path = obs_data_get_string(settings, PROP_AUDIO_FILE_PATH);
  if (audio_path && *audio_path)
  {
    if (pContext_X->pAudioFile_X)
    {
      fclose(pContext_X->pAudioFile_X);
      pContext_X->pAudioFile_X = nullptr;
    }
    pContext_X->pAudioFile_X = fopen(audio_path, "rb");
    obs_log(LOG_INFO, "Audio pSource_X updated to: %s", audio_path);
  }
}
static void MultiMedia_video_tick(void *_pData, float seconds)
{
  auto *pContext_X = (MultiMediaSource *)_pData;
  auto Now = std::chrono::high_resolution_clock::now();
  pContext_X->DeltaTickInUs = std::chrono::duration_cast<std::chrono::microseconds>(Now - pContext_X->TickIn).count();
  pContext_X->TickIn = std::chrono::high_resolution_clock::now();
  if (!pContext_X->pMultiMediaSource_X)
  {
    return;
  }

  // 1. Add current frame time to our accumulator
  pContext_X->PluginAudioEngine.AudioRemainderSecond_lf += (double)seconds;

  // 2. Calculate exactly how many whole samples fit into our accumulated time
  uint32_t samples_to_generate = (uint32_t)(pContext_X->PluginAudioEngine.AudioSampleRate_U32 * pContext_X->PluginAudioEngine.AudioRemainderSecond_lf);

  // 3. Subtract the time we are actually using from the accumulator
  // This preserves the fractional "leftover" time for the next tick
  pContext_X->PluginAudioEngine.AudioRemainderSecond_lf -= (double)samples_to_generate / pContext_X->PluginAudioEngine.AudioSampleRate_U32;
  int32_t *pSample_S32 = (int32_t *)pContext_X->PluginAudioEngine.pAudioBuffer_U8;

  if (pContext_X->AudioSourceType_E == AUDIO_SOURCE_BOARD && samples_to_generate > 0) // Board
  {
    // TODO: Implement board audio capture
    // For now, output silence
    memset(pSample_S32, 0, samples_to_generate * pContext_X->PluginAudioEngine.NbAudioChannel_U32 * sizeof(int32_t));
  }
  else if (pContext_X->AudioSourceType_E == AUDIO_SOURCE_SINE_GENERATOR && samples_to_generate > 0) // Sine Generator
  {
    if (pContext_X->IsAudioSplitMode_B)
    {
      // Option 2: Individual channels are handled by separate sources
      // This main source only generates a simple test tone
      GenerateAudioSinusData(440.0, 0.3 * 0x7FFFFFFF, (double)pContext_X->PluginAudioEngine.AudioSampleRate_U32, samples_to_generate, pSample_S32,
                             pContext_X->PluginAudioEngine.AudioPhase_lf);

      // Output single test audio
      struct obs_source_audio audio_frame = {0};
      audio_frame.data[0] = pContext_X->PluginAudioEngine.pAudioBuffer_U8;
      audio_frame.frames = samples_to_generate;
      audio_frame.speakers = SPEAKERS_MONO;
      audio_frame.samples_per_sec = pContext_X->PluginAudioEngine.AudioSampleRate_U32;
      audio_frame.format = AUDIO_FORMAT_32BIT;
      audio_frame.timestamp = obs_get_video_frame_time() - util_mul_div64(samples_to_generate, 1000000000ULL, pContext_X->PluginAudioEngine.AudioSampleRate_U32);
      obs_source_output_audio(pContext_X->pMultiMediaSource_X, &audio_frame);
    }
    else
    {
      // Option 3: Generate proper 16-channel interleaved audio using AudioEngine
      AudioEngineGenerateAudio(&pContext_X->PluginAudioEngine, samples_to_generate);
      AudioEngine_output_audio(&pContext_X->PluginAudioEngine, samples_to_generate);
    }
  }
  else if (pContext_X->AudioSourceType_E == AUDIO_SOURCE_FILE) // File
  {
    // --- FILE READING ---
    if (pContext_X->pAudioFile_X)
    {
      // 1. Read the mono samples
      size_t audio_read = fread(pSample_S32, sizeof(int32_t), samples_to_generate, pContext_X->pAudioFile_X);

      // Handle Looping
      if (audio_read < samples_to_generate)
      {
        fseek(pContext_X->pAudioFile_X, 0, SEEK_SET);
        fread(pSample_S32 + audio_read, sizeof(int32_t), samples_to_generate - audio_read, pContext_X->pAudioFile_X);
      }

      // Output file-based audio
      struct obs_source_audio audio_frame = {0};
      audio_frame.data[0] = pContext_X->PluginAudioEngine.pAudioBuffer_U8;
      audio_frame.frames = samples_to_generate;
      audio_frame.speakers = SPEAKERS_MONO;
      audio_frame.samples_per_sec = pContext_X->PluginAudioEngine.AudioSampleRate_U32;
      audio_frame.format = AUDIO_FORMAT_32BIT;
      audio_frame.timestamp = obs_get_video_frame_time() - util_mul_div64(samples_to_generate, 1000000000ULL, pContext_X->PluginAudioEngine.AudioSampleRate_U32);
      obs_source_output_audio(pContext_X->pMultiMediaSource_X, &audio_frame);
    }
    else
    {
      // Fallback: If no file, output silence so the mixer doesn't "freeze"
      memset(pSample_S32, 0, samples_to_generate * sizeof(int32_t));
    }
  }
  pContext_X->TickOut = std::chrono::high_resolution_clock::now();
  pContext_X->TickElapsedTimeInUs = std::chrono::duration_cast<std::chrono::microseconds>(pContext_X->TickOut - pContext_X->TickIn).count();
  obs_log(LOG_INFO, ">>>MultiMedia_video_tick Delta %lld Elapsed %lld uS", pContext_X->DeltaTickInUs, pContext_X->TickElapsedTimeInUs);
}
// This is where we draw
static void MultiMedia_video_render(void *_pData, gs_effect_t *effect)
{
  MultiMediaSource *pContext_X = (MultiMediaSource *)_pData;
  auto Now = std::chrono::high_resolution_clock::now();
  pContext_X->DeltaRendererInUs = std::chrono::duration_cast<std::chrono::microseconds>(Now - pContext_X->RendererIn).count();
  pContext_X->RendererIn = std::chrono::high_resolution_clock::now();
  if (!pContext_X->pVideoBuffer_U8 || !pContext_X->pEffect_X)
  {
    return;
  }

  // 1. Calculate MultiMedia Stride
  // Standard MultiMedia stride is usually 128-byte aligned (48 pixels)
  // Formula: (Width + 47) / 48 * 128
  uint32_t stride_bytes = ((pContext_X->VideoWidth_U32 + 47) / 48) * 128;

  // Texture VideoWidth_U32 in "integers" (pixels for the GPU texture)
  // Since 1 integer = 4 bytes, we divide stride by 4.
  uint32_t tex_width = stride_bytes / 4;

  // 2. Manage Texture
  if (!pContext_X->pTexture_X || gs_texture_get_width(pContext_X->pTexture_X) != tex_width ||
      gs_texture_get_height(pContext_X->pTexture_X) != pContext_X->VideoHeight_U32)
  {
    if (pContext_X->pTexture_X)
    {
      gs_texture_destroy(pContext_X->pTexture_X);
      pContext_X->pTexture_X = nullptr;
    }

    // Use GS_RGBA to store 32-bit words
    pContext_X->pTexture_X = gs_texture_create(tex_width, pContext_X->VideoHeight_U32, GS_RGBA, 1, NULL, GS_DYNAMIC);
  }

  // 3. Upload Data
  if (pContext_X->pTexture_X)
  {
    if (pContext_X->VideoSourceType_E == VIDEO_SOURCE_BOARD) // Board
    {
      // TODO: Implement board video capture
      return;
    }
    else if (pContext_X->VideoSourceType_E == VIDEO_SOURCE_FILE) // File
    {
      if (!pContext_X->pVideoFile_X)
      {
        return;
      }
      // Read one frame
      size_t read = fread(pContext_X->pVideoBuffer_U8, 1, pContext_X->VideoFrameSize, pContext_X->pVideoFile_X);
      // Loop file if EOF
      if (read < pContext_X->VideoFrameSize)
      {
        fseek(pContext_X->pVideoFile_X, 0, SEEK_SET);
        // obs_log(LOG_INFO, ">>>raw_video_tick %p sz %d not enought", pContext_X->video_file, read);
        return;
      }
    }
    else if (pContext_X->VideoSourceType_E == VIDEO_SOURCE_COLOR_BAR) // Color Bar
    {
      fill_uyvy_color_bars(pContext_X->pVideoBuffer_U8, pContext_X->VideoWidth_U32, pContext_X->VideoHeight_U32, pContext_X->IsVideoIn10bit_B);

      if (pContext_X->IsLineInColorBarMoving_B)
      {
        draw_line_avx2(pContext_X->pVideoBuffer_U8, pContext_X->VideoWidth_U32, pContext_X->VideoHeight_U32, 255, 0, 0, pContext_X->MovingLinePos_U32, 2, true,
                       pContext_X->IsVideoIn10bit_B);
        pContext_X->MovingLinePos_U32 += 2;
        if (pContext_X->MovingLinePos_U32 > pContext_X->VideoHeight_U32)
        {
          pContext_X->MovingLinePos_U32 = 0;
        }
      }
    }
    gs_texture_set_image(pContext_X->pTexture_X, pContext_X->pVideoBuffer_U8, stride_bytes, false);
  }

  // 4. Draw with Custom Shader
  // We override the default 'effect' passed in by OBS with our custom one
  gs_effect_t *cur_effect = pContext_X->pEffect_X;

  gs_effect_set_texture(pContext_X->pShaderParamImage_X, pContext_X->pTexture_X);
  gs_effect_set_float(pContext_X->pShaderParamWidth_X, (float)pContext_X->VideoWidth_U32);
  gs_effect_set_float(pContext_X->pShaderParamAlpha_X, pContext_X->Alpha_f); // 255.0

  while (gs_effect_loop(cur_effect, "Draw"))
  {
    // Draw sprite to fill the canvas output, not the texture size
    gs_draw_sprite(pContext_X->pTexture_X, 0, pContext_X->VideoWidth_U32, pContext_X->VideoHeight_U32);
  }
  pContext_X->RendererOut = std::chrono::high_resolution_clock::now();
  pContext_X->RendererElapsedTimeInUs = std::chrono::duration_cast<std::chrono::microseconds>(pContext_X->RendererOut - pContext_X->TickIn).count();
  obs_log(LOG_INFO, ">>>MultiMedia_video_render Delta %lld Elapsed %lld uS", pContext_X->DeltaRendererInUs, pContext_X->RendererElapsedTimeInUs);
}

/*
OBS_SOURCE_CUSTOM_DRAW

Used for sources that render directly using graphics API calls (OpenGL/Direct3D)
You control the rendering in video_render() callback
You draw using textures, shaders, sprites, etc.
Rendering happens synchronously with OBS's render loop
Example: Your original red background drawing with gs_draw_sprite()

OBS_SOURCE_ASYNC

Used for sources that provide video frames as data
You push frames using obs_source_output_video()
OBS handles the frame -> texture -> rendering pipeline internally
Frames are timestamped and processed asynchronously
Example: Webcams, capture cards, media sources with video files
*/
struct obs_source_info MultiMedia_source_info = {
    .id = PLUGIN_INPUT_NAME,
    .type = OBS_SOURCE_TYPE_INPUT,
    .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_AUDIO,
    .get_name = MultiMedia_get_name,
    .create = MultiMedia_create,
    .destroy = MultiMedia_destroy,
    .get_width = MultiMedia_get_width,
    .get_height = MultiMedia_get_height,
    .get_defaults = MultiMedia_get_defaults,
    .get_properties = MultiMedia_get_properties,
    .update = MultiMedia_update,
    .video_tick = MultiMedia_video_tick,
    .video_render = MultiMedia_video_render,
};

// Individual Sine Channel Source Functions (Option 2)
static const char *AudioChannel_get_name(void *type_data)
{
  // Return a generic name, specific channel number will be set during registration
  return "Sine Channel";
}

static void *AudioChannel_create(obs_data_t *settings, obs_source_t *pSource_X)
{
  AudioChannelSource *pContext_X = (AudioChannelSource *)bzalloc(sizeof(AudioChannelSource));

  // Initialize audio engine for single channel mode
  AudioEngineInit(&pContext_X->PluginAudioEngine, pSource_X, false);
  pContext_X->channel_number = 1; // Will be set during registration

  return pContext_X;
}

static void AudioChannel_destroy(void *_pData)
{
  AudioChannelSource *pContext_X = (AudioChannelSource *)_pData;

  // Clean up audio engine
  AudioEngineDestroy(&pContext_X->PluginAudioEngine);

  bfree(pContext_X);
  pContext_X = nullptr;
}

static obs_properties_t *AudioChannel_get_properties(void *_pData)
{
  obs_properties_t *pProperty_X = obs_properties_create();

  obs_properties_add_bool(pProperty_X, PROP_ENABLED, PROP_ENABLED_LABEL);
  obs_properties_add_float_slider(pProperty_X, PROP_FREQUENCY, PROP_FREQUENCY_LABEL, 20.0, 20000.0, 10.0);
  obs_properties_add_float_slider(pProperty_X, PROP_AMPLITUDE, PROP_AMPLITUDE_LABEL, 0.0, 1.0, 0.01);

  return pProperty_X;
}

static void AudioChannel_get_defaults(obs_data_t *_pSetting_X)
{
  obs_data_set_default_bool(_pSetting_X, PROP_ENABLED, true);
  obs_data_set_default_double(_pSetting_X, PROP_FREQUENCY, 440.0);
  obs_data_set_default_double(_pSetting_X, PROP_AMPLITUDE, 0.3);
}

static void AudioChannel_update(void *_pData, obs_data_t *_pSetting_X)
{
  AudioChannelSource *pContext_X = (AudioChannelSource *)_pData;

  pContext_X->PluginAudioEngine.AudioChannelEnabled_B = obs_data_get_bool(_pSetting_X, PROP_ENABLED);
  pContext_X->PluginAudioEngine.AudioSinFrequency_lf = obs_data_get_double(_pSetting_X, PROP_FREQUENCY);
  pContext_X->PluginAudioEngine.AudioSinAmplitude_lf = obs_data_get_double(_pSetting_X, PROP_AMPLITUDE);
}

static void AudioChannel_tick(void *_pData, float seconds)
{
  AudioChannelSource *pContext_X = (AudioChannelSource *)_pData;
  AudioEngine *engine = &pContext_X->PluginAudioEngine;

  if (!engine->pAudioSource_X || !engine->AudioChannelEnabled_B)
  {
    return;
  }

  // Add current frame time to our accumulator
  engine->AudioRemainderSecond_lf += (double)seconds;

  // Calculate exactly how many whole samples fit into our accumulated time
  uint32_t samples_to_generate = (uint32_t)(engine->AudioSampleRate_U32 * engine->AudioRemainderSecond_lf);

  // Subtract the time we are actually using from the accumulator
  engine->AudioRemainderSecond_lf -= (double)samples_to_generate / engine->AudioSampleRate_U32;

  if (samples_to_generate > 0)
  {
    AudioEngineGenerateAudio(engine, samples_to_generate);
    AudioEngine_output_audio(engine, samples_to_generate);
  }
}

// Create 16 separate pSource_X info structures
struct obs_source_info AudioChannel_source_infos[PLUGIN_MAX_AUDIO_CHANNELS];

// Static strings for pSource_X IDs and names
static char sine_source_ids[PLUGIN_MAX_AUDIO_CHANNELS][64];
static char sine_source_names[PLUGIN_MAX_AUDIO_CHANNELS][64];

// Static name functions for each channel
static const char *AudioChannel_get_name_0(void *)
{
  return sine_source_names[0];
}
static const char *AudioChannel_get_name_1(void *)
{
  return sine_source_names[1];
}
static const char *AudioChannel_get_name_2(void *)
{
  return sine_source_names[2];
}
static const char *AudioChannel_get_name_3(void *)
{
  return sine_source_names[3];
}
static const char *AudioChannel_get_name_4(void *)
{
  return sine_source_names[4];
}
static const char *AudioChannel_get_name_5(void *)
{
  return sine_source_names[5];
}
static const char *AudioChannel_get_name_6(void *)
{
  return sine_source_names[6];
}
static const char *AudioChannel_get_name_7(void *)
{
  return sine_source_names[7];
}
static const char *AudioChannel_get_name_8(void *)
{
  return sine_source_names[8];
}
static const char *AudioChannel_get_name_9(void *)
{
  return sine_source_names[9];
}
static const char *AudioChannel_get_name_10(void *)
{
  return sine_source_names[10];
}
static const char *AudioChannel_get_name_11(void *)
{
  return sine_source_names[11];
}
static const char *AudioChannel_get_name_12(void *)
{
  return sine_source_names[12];
}
static const char *AudioChannel_get_name_13(void *)
{
  return sine_source_names[13];
}
static const char *AudioChannel_get_name_14(void *)
{
  return sine_source_names[14];
}
static const char *AudioChannel_get_name_15(void *)
{
  return sine_source_names[15];
}

// Static defaults functions for each channel
static void AudioChannel_get_defaults_0(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, true);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 440.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_1(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, true);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 550.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_2(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, false);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 660.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_3(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, false);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 770.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_4(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, false);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 880.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_5(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, false);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 990.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_6(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, false);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 1100.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_7(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, false);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 1210.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_8(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, false);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 1320.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_9(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, false);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 1430.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_10(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, false);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 1540.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_11(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, false);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 1650.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_12(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, false);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 1760.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_13(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, false);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 1870.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_14(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, false);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 1980.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}
static void AudioChannel_get_defaults_15(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, PROP_ENABLED, false);
  obs_data_set_default_double(settings, PROP_FREQUENCY, 2090.0);
  obs_data_set_default_double(settings, PROP_AMPLITUDE, 0.3);
}

// Array of function pointers
static const char *(*sine_get_name_functions[])(void *) = {
    AudioChannel_get_name_0,  AudioChannel_get_name_1,  AudioChannel_get_name_2,  AudioChannel_get_name_3, AudioChannel_get_name_4,  AudioChannel_get_name_5,
    AudioChannel_get_name_6,  AudioChannel_get_name_7,  AudioChannel_get_name_8,  AudioChannel_get_name_9, AudioChannel_get_name_10, AudioChannel_get_name_11,
    AudioChannel_get_name_12, AudioChannel_get_name_13, AudioChannel_get_name_14, AudioChannel_get_name_15};

static void (*sine_get_defaults_functions[])(obs_data_t *) = {
    AudioChannel_get_defaults_0,  AudioChannel_get_defaults_1,  AudioChannel_get_defaults_2,  AudioChannel_get_defaults_3,
    AudioChannel_get_defaults_4,  AudioChannel_get_defaults_5,  AudioChannel_get_defaults_6,  AudioChannel_get_defaults_7,
    AudioChannel_get_defaults_8,  AudioChannel_get_defaults_9,  AudioChannel_get_defaults_10, AudioChannel_get_defaults_11,
    AudioChannel_get_defaults_12, AudioChannel_get_defaults_13, AudioChannel_get_defaults_14, AudioChannel_get_defaults_15};

// Function to initialize all 16 sine channel sources
void initialize_AudioChannel_sources()
{
  uint32_t i_U32;

  for (i_U32 = 0; i_U32 < PLUGIN_MAX_AUDIO_CHANNELS; i_U32++)
  {
    sprintf(sine_source_ids[i_U32], "AudioChannel_%d", i_U32 + 1);
    sprintf(sine_source_names[i_U32], "Sine Channel %d", i_U32 + 1);

    AudioChannel_source_infos[i_U32] = {
        .id = sine_source_ids[i_U32],
        .type = OBS_SOURCE_TYPE_INPUT,
        .output_flags = OBS_SOURCE_AUDIO,
        .get_name = sine_get_name_functions[i_U32],
        .create = AudioChannel_create,
        .destroy = AudioChannel_destroy,
        .get_defaults = sine_get_defaults_functions[i_U32],
        .get_properties = AudioChannel_get_properties,
        .update = AudioChannel_update,
        .video_tick = AudioChannel_tick,
    };
  }
}

bool obs_module_load(void)
{
  bool Rts_B = false;
  WSADATA wsa;
  uint32_t i_U32;

  obs_log(LOG_INFO, "Plugin loaded successfully (version %s)", PLUGIN_VERSION);

  GL_pObsLogger = new ObsLogger();
  GL_pEvsPcieWinIoLogger = new EvsHwLGPL::CMsgLogger(GL_pObsLogger, 512);
  GL_pEvsPcieWinIoLogger->ChangeMsgSev(EvsHwLGPL::SEV_DEBUG);
  EvsHwLGPL::SetGlobalLoggerPointer(GL_pEvsPcieWinIoLogger);

  // char p[512];
  // DWORD l=GetCurrentDirectoryA(sizeof(p),p);
  // obs_log(LOG_INFO, "GetCurrentDirectoryA %d %s", l, p);

  Rts_B = LoadCliArgFromJson("../../obs-plugins/64bit/obs-evs-pcie-win-io-config.json", GL_CliArg_X);
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
      obs_log(LOG_INFO, "Initialize winsock");
      if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
      {
        obs_log(LOG_ERROR, "WSAStartup failed");
      }
      else
      {
        Rts_B = true;
      }
    }
  }
  /*
  obs_get_main_texture(): Returns the gs_texture_t representing the entire rendered canvas.

obs_add_raw_video_callback(): Use this if you want the frame after it has been converted to the output format (like NV12 or I420) but before it is encoded. This
is often better for "Virtual Camera" style plugins.

gs_stage_texture(): Used to move the frame data from the GPU to the CPU if you need to perform non-graphics processing (like AI analysis or saving to a file).

  */
  obs_register_source(&MultiMedia_source_info);

  // Always register 16 separate sine channel sources for Option 2
  obs_log(LOG_INFO, "Registering 16 individual sine channel sources");
  initialize_AudioChannel_sources();
  for (i_U32 = 0; i_U32 < PLUGIN_MAX_AUDIO_CHANNELS; i_U32++)
  {
    obs_register_source(&AudioChannel_source_infos[i_U32]);
    obs_log(LOG_INFO, "Registered sine channel %d", i_U32 + 1);
  }
  obs_log(LOG_INFO, "Both audio modes available - use 'Split Audio Mode' property to switch");

  // obs_register_source(&udp_stream_filter_info);
  // obs_register_output(&raw_dump_info);
  //  Add a button to the Tools menu
  // obs_frontend_add_tools_menu_item("Toggle Raw Stream Dump", on_menu_click, nullptr);

  obs_log(LOG_INFO, "obs_module_load returns %s", Rts_B ? "true" : "false");
  // ADD_MESSAGE(EvsHwLGPL::SEV_INFO, "hello world\n");
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
  WSACleanup();
}
/*
Looking at the #if 0 section in your code, these are experimental/disabled features that were developed but not currently active. Here's what each component was
designed to do:

1. capture_frame Function
Purpose: Frame Capture from OBS Canvas

Captures the final rendered frame from OBS's main output canvas
Uses obs_get_main_texture() to get the complete composed scene
Implements GPU-to-CPU transfer using staging surfaces
Would allow you to access raw pixel data of the final OBS output

// Gets the final composed frame that OBS is outputting
gs_texture_t *main_tex = obs_get_main_texture();
gs_stage_texture(staged_surf, main_tex);  // GPU -> CPU transfer

2. Raw Dump Output Component
Purpose: Complete OBS Output Recording

Video Dumper: Records final composed video frames to video_stream.yuv
Audio Dumper: Records final audio mix to audio_stream.pcm
Multi-channel Audio: Handles up to 16 audio channels
Raw Format: Saves uncompressed/unencoded data
Key Functions:

raw_dump_video(): Captures final scene video (after all filters/effects)
raw_dump_audio(): Captures final audio mix (48kHz, 32-bit float)
raw_dump_start/stop(): Controls recording session
3. on_menu_click Function
Purpose: OBS Tools Menu Integration

Adds "Toggle Raw Stream Dump" to OBS Tools menu
Provides user interface to start/stop raw dumping
Creates global dumper instance on-demand
// Would add this to OBS Tools menu:
obs_frontend_add_tools_menu_item("Toggle Raw Stream Dump", on_menu_click, nullptr);
Why These Are Disabled (#if 0)
Development/Testing Features: These were experimental tools for analyzing OBS output
Performance Impact: GPU-to-CPU transfers and file I/O could impact real-time performance
Frontend API Issues: Comment mentions build issues with obs-frontend-api.h
Debugging Purpose: Useful for verifying your plugin's integration with OBS pipeline
Potential Use Cases
Plugin Development: Verify your audio/video is correctly integrated into OBS
Quality Analysis: Compare raw output with encoded streams
Performance Testing: Measure pipeline latency and throughput
Format Validation: Ensure proper YUV/PCM format handling
These components represent a complete OBS output capture system that could be enabled for debugging or advanced use cases, but are currently disabled to keep
the plugin focused on its primary EVS PCIe functionality.
*/
#if 0
struct udp_stream_filter
{
  obs_source_t *pSource_X;
  SOCKET sock;
  struct sockaddr_in dest;
  bool started;
};


static const char *udp_filter_get_name(void *unused)
{
  UNUSED_PARAMETER(unused);
  return "UYVY UDP Streamer (127.0.0.1:5000)";
}

static void *udp_filter_create(obs_data_t *settings, obs_source_t *pSource_X)
{
  UNUSED_PARAMETER(settings);
  obs_log(LOG_INFO, ">>>udp_filter_create");
  auto *ctx = (udp_stream_filter *)bzalloc(sizeof(udp_stream_filter));
  ctx->pSource_X = pSource_X;
  ctx->sock = INVALID_SOCKET;
  ctx->started = false;

  ctx->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (ctx->sock == INVALID_SOCKET)
  {
    obs_log(LOG_ERROR, "udp_filter_create: socket() failed");
  }
  else
  {
    memset(&ctx->dest, 0, sizeof(ctx->dest));
    ctx->dest.sin_family = AF_INET;
    ctx->dest.sin_port = htons(5000);
    //ctx->dest.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (inet_pton(AF_INET, "127.0.0.1", &ctx->dest.sin_addr) != 1)
    {
      obs_log(LOG_ERROR, "udp_filter_create: inet_pton failed");
    }
    else
    {
      ctx->started = true;
    }
    obs_log(LOG_INFO, "udp_filter_create: UDP socket ready to 127.0.0.1:5000");
  }
  return ctx;
}

static void udp_filter_destroy(void *_pData)
{
  auto *ctx = (udp_stream_filter *)data;
  obs_log(LOG_INFO, ">>>udp_filter_destroy");
  if (!ctx)
  {
    return;
  }

  if (ctx->sock != INVALID_SOCKET)
  {
    closesocket(ctx->sock);
    ctx->sock = INVALID_SOCKET;
  }
  if (ctx->started)
  {
    ctx->started = false;
  }
  bfree(ctx);
}

static obs_properties_t *udp_filter_properties(void *_pData)
{
  UNUSED_PARAMETER(data);
  obs_properties_t *pProperty_X = obs_properties_create();
  // Minimal: no configurable properties for now
  return pProperty_X;
}

// This callback receives frames from the attached pSource_X.
// The OBS filter callback name used here is `filter_video` (OBS expects this member for filters).
static struct obs_source_frame *udp_filter_video(void *_pData, struct obs_source_frame *frame)
{
  auto *ctx = (udp_stream_filter *)data;
  obs_log(LOG_INFO, ">>>udp_filter_video");
  if (!ctx || !ctx->started || !frame)
  {
    return frame;
  }

  // Only forward UYVY frames
  if (frame->format != VIDEO_FORMAT_UYVY)
  {
    // Drop non-UYVY but still pass the frame through
    return frame;
  }

  // Compute contiguous byte size for plane 0 (UYVY is packed: 2 bytes per pixel)
  size_t bytes = (size_t)frame->linesize[0] * (size_t)frame->VideoHeight_U32;
  if (bytes > 0xF000)
  {
    obs_log(LOG_WARNING, "udp_filter_video: frame size too large (%zu bytes), clipping", bytes);
    bytes = 0xF000;
  }
  const char *buf = reinterpret_cast<const char *>(frame->data[0]);

  int sent = sendto(ctx->sock, buf, static_cast<int>(bytes), 0, reinterpret_cast<struct sockaddr *>(&ctx->dest), static_cast<int>(sizeof(ctx->dest)));
  if (sent == SOCKET_ERROR)
  {
    obs_log(LOG_WARNING, "udp_filter_video: sendto failed (%d)", WSAGetLastError());
  }

  return frame;
}

// Register filter info (append this declaration somewhere near the other obs_source_info structs)
static struct obs_source_info udp_stream_filter_info = {
    .id = "uyvy_udp_stream_filter",
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_VIDEO,
    .get_name = udp_filter_get_name,
    .create = udp_filter_create,
    .destroy = udp_filter_destroy,
    .get_properties = udp_filter_properties,
    .filter_video = udp_filter_video,
};
/*
show me how to do  Texture Sharing (GPU-to-GPU)

To implement Texture Sharing (Zero-Copy), you must shift from an "Asynchronous" pSource_X (pushing raw pixels) to a "Synchronous" pSource_X (rendering directly to the
GPU).

In this mode, you manage a gs_texture_t object and use the video_render callback to draw it. This bypasses the RAM-to-RAM memcpy entirely.

1. Update Source Flags
First, you must remove the OBS_SOURCE_ASYNC_VIDEO flag from your obs_source_info struct. This tells OBS that you will handle the rendering yourself.

C++
struct obs_source_info my_source = {
    .id = "my_texture_source",
    .type = OBS_SOURCE_TYPE_INPUT,
    // Note: No ASYNC_VIDEO flag here
    .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
    .get_name = my_get_name,
    .create = my_create,
    .destroy = my_destroy,
    .video_render = my_video_render, // The magic happens here
    .get_width = my_get_width,
    .get_height = my_get_height,
};
2. Manage the Texture
You create the texture once (usually in .create) and update it only when your data changes.

Important: Any call to gs_ functions (Graphics System) must be wrapped in obs_enter_graphics() / obs_leave_graphics() unless it's inside the video_render
callback.

C++
// Inside your create or update function:
obs_enter_graphics();
if (!pContext_X->texture) {
    pContext_X->texture = gs_texture_create(VideoWidth_U32, VideoHeight_U32, GS_RGBA, 1, NULL, GS_DYNAMIC);
}

// Updating the texture with new data (this is the only "copy", RAM to GPU)
uint8_t *ptr;
uint32_t linesize;
if (gs_texture_map(pContext_X->texture, &ptr, &linesize)) {
    memcpy(ptr, pContext_X->buffer, linesize * VideoHeight_U32);
    gs_texture_unmap(pContext_X->texture);
}
obs_leave_graphics();
3. The video_render Callback
This function is called by OBS every time it draws a frame. Because you are using OBS_SOURCE_CUSTOM_DRAW, you are responsible for drawing the texture to the
screen.

C++
static void my_video_render(void *_pData, gs_effect_t *effect) {
    struct my_context *pContext_X = data;

    if (!pContext_X->texture) return;

    // Get the default "Draw" effect (simple texture shader)
    gs_effect_t *draw_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);

    // Set the texture as the 'image' parameter for the shader
    gs_effect_set_texture(gs_effect_get_param_by_name(draw_effect, "image"),
                          pContext_X->texture);

    // Draw the texture onto the current render target (the OBS canvas)
    while (gs_effect_loop(draw_effect, "Draw")) {
        obs_source_draw(pContext_X->texture, 0, 0, 0, 0, 0);
    }
}

*/
void capture_frame(void *_pData, uint32_t cx, uint32_t cy)
{
  gs_texture_t *main_tex = obs_get_main_texture();
  if (!main_tex)
  {
    return;
  }

  // Create or recreate staging surface if dimensions changed
  if (!staged_surf || gs_stagesurface_get_width(staged_surf) != cx || gs_stagesurface_get_height(staged_surf) != cy)
  {
    if (staged_surf)
    {
      gs_stagesurface_destroy(staged_surf);
    }
    // Create staging surface with same format as the texture
    enum gs_color_format format = gs_texture_get_color_format(main_tex);
    staged_surf = gs_stagesurface_create(cx, cy, format);
  }

  if (!staged_surf)
  {
    return;
  }

  // Stage the texture (GPU -> CPU memory)
  gs_stage_texture(staged_surf, main_tex);

  // Map and access the pixel data
  uint8_t *data_ptr = nullptr;
  uint32_t linesize = 0;

  if (gs_stagesurface_map(staged_surf, &data_ptr, &linesize))
  {
    // Now you have access to the pixel data
    // data_ptr contains the pixel data in the format specified during creation
    // linesize is the byte stride per row

    uint32_t VideoWidth_U32 = gs_stagesurface_get_width(staged_surf);
    uint32_t VideoHeight_U32 = gs_stagesurface_get_height(staged_surf);

    // Process or write pixel data here
    // For example, write to file:
    // fwrite(data_ptr, 1, linesize * VideoHeight_U32, output_file);

    gs_stagesurface_unmap(staged_surf);
  }
}

// Structure to hold our plugin state
struct raw_dump_context
{
  obs_output_t *output;
  FILE *video_file = nullptr;
  FILE *audio_file = nullptr;
  bool active = false;
};

// --- Callbacks ---

static const char *raw_dump_get_name(void *unused)
{
  return "Raw Stream dumper";
}

static void *raw_dump_create(obs_data_t *settings, obs_output_t *output)
{
  auto *pContext_X = new raw_dump_context();
  pContext_X->output = output;
  return pContext_X;
}

static void raw_dump_destroy(void *_pData)
{
  auto *pContext_X = static_cast<raw_dump_context *>(data);
  delete pContext_X;
}

static bool raw_dump_start(void *_pData)
{
  auto *pContext_X = static_cast<raw_dump_context *>(data);
  //Will be created in E:\pro\obs-studio\build_x64\rundir\Debug\bin\64bit
  pContext_X->video_file = fopen("video_stream.yuv", "ab"); // Append Binary
  pContext_X->audio_file = fopen("audio_stream.pcm", "ab");

  if (!pContext_X->video_file || !pContext_X->audio_file)
  {
    return false;
  }

  // Connect to the main mix
  if (!obs_output_begin_data_capture(pContext_X->output, 0))
  {
    return false;
  }

  pContext_X->active = true;
  return true;
}

static void raw_dump_stop(void *_pData, uint64_t ts)
{
  auto *pContext_X = static_cast<raw_dump_context *>(data);

  obs_output_end_data_capture(pContext_X->output);
  pContext_X->active = false;

  if (pContext_X->video_file)
  {
    fclose(pContext_X->video_file);
  }
  if (pContext_X->audio_file)
  {
    fclose(pContext_X->audio_file);
  }
}

// Receives Video: Final Scene (all filters/sources applied)
static void raw_dump_video(void *_pData, struct video_data *frame)
{
  auto *pContext_X = static_cast<raw_dump_context *>(data);

  // Video frames are planar. We write each plane sequentially.
  // For NV12: data[0] is Y, data[1] is UV interleaved.
  for (int i = 0; i < MAX_AV_PLANES; i++)
  {
    if (frame->data[i] && frame->linesize[i] > 0)
    {
      // Note: In a production plugin, you'd account for padding/stride.
      // For a basic dump, we write the full linesize x VideoHeight_U32.
      uint32_t VideoHeight_U32 = obs_output_get_height(pContext_X->output);
      if (i > 0)
      {
        VideoHeight_U32 /= 2; // Simple assumption for YUV420/NV12 chroma
      }

      fwrite(frame->data[i], 1, frame->linesize[i] * VideoHeight_U32, pContext_X->video_file);
    }
  }
}

// Receives Audio: Final Mix (48Khz, Planar Float)
static void raw_dump_audio(void *_pData, struct audio_data *frames)
{
  auto *pContext_X = static_cast<raw_dump_context *>(data);

  // Audio is planar. We write Channel 0 then Channel 1 etc.
  // OBS Audio is typically 32-bit Float.
    for (i_U32 = 0; i_U32 < PLUGIN_MAX_AUDIO_CHANNELS; i_U32++)
  {
    if (frames->data[i])
    {
      fwrite(frames->data[i], sizeof(float), frames->frames, pContext_X->audio_file);
    }
  }
}

// --- Registration ---

struct obs_output_info raw_dump_info = {
    .id = "raw_dump_output",
    .flags = OBS_OUTPUT_VIDEO | OBS_OUTPUT_AUDIO,
    .get_name = raw_dump_get_name,
    .create = raw_dump_create,
    .destroy = raw_dump_destroy,
    .start = raw_dump_start,
    .stop = raw_dump_stop,
    .raw_video = raw_dump_video,
    .raw_audio = raw_dump_audio,
};


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

#endif
