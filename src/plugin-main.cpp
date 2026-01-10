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
#include <winsock2.h>
#include <ws2tcpip.h>
#include <nlohmann/json.hpp>
#include <obs-module.h>
#include <plugin-support.h>
#include <BoardResourceManager.h>
#include <fstream>
#include "SimpleRecorder.h"
#include "vectorclass.h"
#include <libevs-pcie-win-io-api/src/EvsPcieIoHelpers.h>

#define IN10BITS
extern struct obs_source_info raw_10bit_info;
extern struct obs_source_info udp_stream_filter_info;
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
//Transforming v210 (10-bit 4:2:2) to UYVY (8-bit 4:2:2) 
uint32_t v210_to_uyvy_avx2_vcl(const uint32_t *src, uint8_t *dst, int width, int height)
{
  int total_dwords = (width + 5) / 6 * 4 * height;
  uint8_t *original_dst = dst;
  // Process 8 DWORDs (12 pixels) at a time
  for (int i = 0; i <= total_dwords - 8; i += 8)
  {
    // Load 8 DWORDs (256 bits)
    Vec8ui w = Vec8ui().load(src + i);

    // We need to extract 10-bit components and shift to 8-bit
    // Component 0: [9:0]   -> bits 2-9
    // Component 1: [19:10] -> bits 12-19
    // Component 2: [29:20] -> bits 22-29

    // Extract and shift down to 8-bit
    Vec8ui c0 = (w >> 2) & 0xFF;
    Vec8ui c1 = (w >> 12) & 0xFF;
    Vec8ui c2 = (w >> 22) & 0xFF;

    // Now we must interleave these into UYVY order
    // This requires custom shuffling since v210 is non-linear across words
    // Below is the logical mapping for the first 4 DWORDS (6 pixels):
    // Word 0: U0(c0), Y0(c1), V0(c2)
    // Word 1: Y1(c0), U1(c1), Y2(c2)
    // Word 2: V1(c0), Y3(c1), U2(c2)
    // Word 3: Y4(c0), V2(c1), Y5(c2)

    // For high performance, we use a Permute/Shuffle to align bytes.
    // To simplify, we'll store the extracted bytes into a temporary buffer
    // and let the compiler's auto-vectorizer optimize the final pack.
    alignas(32) uint32_t b0[8], b1[8], b2[8];
    c0.store(b0);
    c1.store(b1);
    c2.store(b2);

    // Manual packing of the 12 pixels (24 bytes) from the 8 DWORDs
    // This loop handles the "un-packing" of the v210 structure
    for (int j = 0; j < 8; j += 4)
    {
      // Pixel Group (6 pixels)
      *dst++ = (uint8_t)b0[j];     // U0
      *dst++ = (uint8_t)b1[j];     // Y0
      *dst++ = (uint8_t)b2[j];     // V0
      *dst++ = (uint8_t)b0[j + 1]; // Y1
      *dst++ = (uint8_t)b1[j + 1]; // U1
      *dst++ = (uint8_t)b2[j + 1]; // Y2
      *dst++ = (uint8_t)b0[j + 2]; // V1
      *dst++ = (uint8_t)b1[j + 2]; // Y3
      *dst++ = (uint8_t)b2[j + 2]; // U2
      *dst++ = (uint8_t)b0[j + 3]; // Y4
      *dst++ = (uint8_t)b1[j + 3]; // V2
      *dst++ = (uint8_t)b2[j + 3]; // Y5
    }
  }
  return static_cast<uint32_t>(dst - original_dst);
}

// Optimized v210 to UYVY converter using AVX2
// Returns the size of the processed buffer in bytes.
uint32_t v210_to_uyvy_avx2_opt(const uint32_t *__restrict src, uint8_t *__restrict dst, int width, int height)
{
  // Total 32-bit words to process
  // v210 packs 6 pixels into 4 words (128 bits).
  int total_dwords = (width + 5) / 6 * 4 * height;
  uint8_t *original_dst = dst;

  // Constants for bit manipulation
  // We shift right to align the 10-bit MSBs to the 8-bit output positions
  // R0 (Comp 0): (Word >> 2) & 0xFF
  // R1 (Comp 1): (Word >> 12) & 0xFF -> We shift >> 4 and mask 0xFF00
  // R2 (Comp 2): (Word >> 22) & 0xFF -> We shift >> 6 and mask 0xFF0000
  const __m256i mask_comp0 = _mm256_set1_epi32(0x000000FF);
  const __m256i mask_comp1 = _mm256_set1_epi32(0x0000FF00);
  const __m256i mask_comp2 = _mm256_set1_epi32(0x00FF0000);

  // Shuffle Mask: Rearrange bytes from 32-bit words into UYVY stream
  // Input Word Layout (Little Endian): [00 V0 Y0 U0]
  // We want to extract bytes at indices 0,1,2 (Word0), 4,5,6 (Word1), etc.
  // -1 denotes zeroing out the remaining bytes (padding)
  const __m256i shuffle_mask = _mm256_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, -1, -1, -1, -1, // Lane 0
                                                0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, -1, -1, -1, -1  // Lane 1 (Same relative pattern)
  );

  int i = 0;
  // Main Loop: Process 8 DWORDs (2 blocks, 12 pixels, 24 bytes output) per iteration
  for (; i <= total_dwords - 8; i += 8)
  {
    // 1. Load 256 bits (8 DWORDs)
    __m256i w = _mm256_loadu_si256((const __m256i *)(src + i));

    // 2. Extract Components
    // Instead of shifting each component down to 0-255 and then shifting back up
    // to combine them, we shift them directly to their target byte slot in the 32-bit word.
    __m256i c0 = _mm256_and_si256(_mm256_srli_epi32(w, 2), mask_comp0); // 0x000000UU
    __m256i c1 = _mm256_and_si256(_mm256_srli_epi32(w, 4), mask_comp1); // 0x0000YY00
    __m256i c2 = _mm256_and_si256(_mm256_srli_epi32(w, 6), mask_comp2); // 0x00VV0000

    // 3. Combine into packed 32-bit words: 0x00VVYYUU
    __m256i packed = _mm256_or_si256(c0, _mm256_or_si256(c1, c2));

    // 4. Compact the valid bytes (remove the 0x00 high byte from every word)
    // Result per 128-bit lane: [Garbage][Valid 12 Bytes] (Little Endian)
    __m256i uyvy = _mm256_shuffle_epi8(packed, shuffle_mask);

    // 5. Store Result using Overlapping Stores (Faster than masking)
    // We have 24 bytes of valid data total (12 in lower lane, 12 in upper lane).

    // Store Lower Lane (16 bytes written, valid bytes 0-11)
    _mm_storeu_si128((__m128i *)dst, _mm256_castsi256_si128(uyvy));

    // Store Upper Lane (16 bytes written, valid bytes 12-23)
    // We write to offset 12. This overwrites the 4 bytes of garbage from the previous store
    // with the correct data from the start of this lane.
    _mm_storeu_si128((__m128i *)(dst + 12), _mm256_extracti128_si256(uyvy, 1));

    dst += 24;
  }

  // Scalar Fallback for remaining items (v210 is usually block aligned to 4 words)
  // There will be at most 1 block (4 words) remaining.
  /*
  you can remove the scalar fallback, but you must ensure your input dimensions meet specific alignment requirements.

Condition 1: The "Safe Delete" (Width Alignment)
You can simply delete the scalar loop if you guarantee that your image width is a multiple of 12 pixels.

Why?

v210 stores pixels in "blocks" of 6 pixels (128 bits / 4 words).

Your AVX2 loop processes 2 blocks at a time (12 pixels / 8 words).

Standard resolutions like 1920x1080, 1280x720, and 3840x2160 are all multiples of 12.

If your width is standard, total_dwords is always divisible by 8, so the scalar loop is unreachable code.

Condition 2: The "Vectorized Tail" (Handling Odd Blocks)
If you have weird widths (e.g., a width that is a multiple of 6 but not 12), you might have one block (4 words) left over at the end
  */
  for (; i < total_dwords; i++)
  {
    uint32_t val = src[i];
    // Decode 10-bit values
    uint8_t u = (val >> 2) & 0xFF;
    uint8_t y = (val >> 12) & 0xFF;
    uint8_t v = (val >> 22) & 0xFF;

    // Map v210 words to UYVY sequence
    // The pattern repeats every 4 words.
    int word_idx = i % 4;

    if (word_idx == 0)
    { // U0, Y0, V0
      dst[0] = u;
      dst[1] = y;
      dst[2] = v;
      dst += 3;
    }
    else if (word_idx == 1)
    { // Y1, U1, Y2
      dst[0] = u;
      dst[1] = y;
      dst[2] = v; // Note: Logic inputs are effectively Y, U, Y here
      dst += 3;
    }
    else if (word_idx == 2)
    { // V1, Y3, U2
      dst[0] = u;
      dst[1] = y;
      dst[2] = v;
      dst += 3;
    }
    else
    { // Y4, V2, Y5
      dst[0] = u;
      dst[1] = y;
      dst[2] = v;
      dst += 3;
    }
  }

  return static_cast<uint32_t>(dst - original_dst);
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
  WSADATA wsa;

  obs_log(LOG_INFO, "Plugin loaded successfully (version %s)", PLUGIN_VERSION);
  obs_log(LOG_INFO, "2222");
  
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

  obs_register_source(&raw_10bit_info);
  obs_register_source(&udp_stream_filter_info);
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
  WSACleanup();
}



struct raw_10bit_source
{
  obs_source_t *source;
  FILE *file;
  uint32_t width;
  uint32_t height;
  uint32_t fps;
  size_t frame_size;
  uint64_t frame_count;
  gs_texture_t *texture;
  // Buffer for one frame
  uint8_t *buffer;
};
struct udp_stream_filter
{
  obs_source_t *source;
  SOCKET sock;
  struct sockaddr_in dest;
  bool started;
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
#if defined(IN10BITS)  
  context->width = 1920;
  context->height = 1080;
  context->fps = 30;
#else
  context->width = 640;
  context->height = 480;
  context->fps = 60;
#endif

#if defined(IN10BITS)
  context->frame_size =
      EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::ComputeImageSize(EVS::EvsPcieIoApi::VideoStd_1080p_59_94, FOURCC_V210, context->width, context->height, false);
//  EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::ComputeImageSize(EVS::EvsPcieIoApi::VideoStd_1080i_59_94, FOURCC_V210, context->width, context->height, false);
#else
  context->frame_size = context->width * context->height * 2;
  fill_uyvy_color_bars(context->buffer, context->width, context->height);
#endif
  context->buffer = (uint8_t *)bmalloc(context->frame_size);
  // Create texture
  obs_enter_graphics();
  context->texture = gs_texture_create(context->width, context->height, GS_BGRA, 1, NULL, GS_DYNAMIC);
  obs_leave_graphics();

  const char *p = obs_data_get_string(settings, "file_path");
  obs_log(LOG_INFO, ">>>raw_create obs_data_get_string '%s'", p);

  char path[512];
#if defined(IN10BITS)
  sprintf(path, "%s1920x1080@29.97i_clp_0.yuv10", GL_CliArg_X.BaseDirectory_S.c_str());
#else
  sprintf(path, "%s640x480@59.94p_clp_0.yuv8", GL_CliArg_X.BaseDirectory_S.c_str());
#endif  
  context->file = fopen(path, "rb");
  obs_log(LOG_INFO, ">>>raw_create '%s' %p", path, context->file);

  return context;
}

// 3. Destroy the Source
static void raw_destroy(void *data)
{
  auto *context = (raw_10bit_source *)data;

  obs_log(LOG_INFO, ">>>raw_destroy %p", context->file);
  obs_enter_graphics();
  if (context->texture)
  {
    gs_texture_destroy(context->texture);
  }
  obs_leave_graphics();
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
  size_t read = fread(context->buffer, 1, context->frame_size, context->file);
  // Loop file if EOF
  if (read < context->frame_size)
  {
    fseek(context->file, 0, SEEK_SET);
    //obs_log(LOG_INFO, ">>>raw_video_tick %p sz %d not enought", context->file, read);
    return;
  }
#if defined(IN10BITS)
  auto t1 = std::chrono::high_resolution_clock::now();
  // Convert v210 to UYVY
  v210_to_uyvy_avx2_vcl((const uint32_t *)context->buffer, context->buffer, context->width, context->height);
//  v210_to_uyvy_avx2_opt((const uint32_t *)context->buffer, context->buffer, context->width, context->height);
  auto t2 = std::chrono::high_resolution_clock::now();

#endif
  //struct obs_source_frame frame = {0};
  struct obs_source_frame2 frame = {0};
  frame.width = context->width;
  frame.height = context->height;
  frame.format = VIDEO_FORMAT_UYVY; // VIDEO_FORMAT_P010;

  // P010 is semi-planar: Y in plane 0, interleaved UV in plane 1
//  frame.data[0] = context->buffer;
//  frame.data[1] = context->buffer + (context->width * context->height * 2);
  frame.data[0] = context->buffer;

  //frame.linesize[0] = context->width * 2;
  //frame.linesize[1] = context->width * 2;
  frame.linesize[0] = context->width * 2;

  // Timing using std::chrono
  auto duration = std::chrono::nanoseconds(1000000000 / context->fps);
  //frame.timestamp = context->frame_count * duration.count();
  frame.timestamp = obs_get_video_frame_time();

  //obs_source_output_video(context->source, &frame);
  // Use obs_source_output_video2 which is the new standard

  // Color Information
  frame.trc = VIDEO_TRC_SRGB;  //For 10 ,bits frame.trc = VIDEO_TRC_PQ; or VIDEO_TRC_HLG; for HDR.
  frame.range = VIDEO_RANGE_PARTIAL;
  // Color setup - This fills color_matrix, color_range_min, and color_range_max
  video_format_get_parameters(VIDEO_CS_709, VIDEO_RANGE_PARTIAL, frame.color_matrix, frame.color_range_min, frame.color_range_max);

  frame.timestamp = obs_get_video_frame_time();
  //frame.flip = false; // Set to true only if the image appears upside down in OBS
  obs_source_output_video2(context->source, &frame);

  context->frame_count++;
#if defined(IN10BITS)
  auto t3 = std::chrono::high_resolution_clock::now();
  auto duration_conversion = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
  obs_log(LOG_INFO, ">>>raw_video_tick v210_to_uyvy_avx2_vcl took %lld us", duration_conversion);
  auto duration_output = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
  obs_log(LOG_INFO, ">>>raw_video_tick obs_source_output_video2 to end took %lld us", duration_output);
#endif
 // obs_log(LOG_INFO, ">>>raw_video_tick %p sz %d cnt %d ts %ld", context->file, read, context->frame_count, frame.timestamp);
  //obs_log(LOG_INFO, ">>>raw_video_tick %p cnt %d ts %ld", context->file, context->frame_count, frame.timestamp);
}
static obs_properties_t *raw_get_properties(void *data)
{
  obs_log(LOG_INFO, ">>>raw_get_properties");
  obs_properties_t *props = obs_properties_create();

  // The first string "file_path" is the KEY used by obs_data_get_string
  obs_properties_add_path(props, "file_path", "Select Raw YUV File", OBS_PATH_FILE, "Raw Files (*.yuv8 *.yuv8)", nullptr);

  return props;
}
static void raw_video_render(void *data, gs_effect_t *effect)
{
  auto *context = (raw_10bit_source *)data;
  //UNUSED_PARAMETER(context);
  obs_log(LOG_INFO, ">>>raw_video_render");
  effect = obs_get_base_effect(OBS_EFFECT_SOLID);

  gs_effect_set_color(gs_effect_get_param_by_name(effect, "color"), 0xFFFF0000);

  while (gs_effect_loop(effect, "Solid"))
  { 
    gs_draw_sprite(NULL, 0, context->width, context->height);
  }
}
// 5. Register the Source Info
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
struct obs_source_info raw_10bit_info = {
    .id = PLUGIN_INPUT_NAME, //"obs-evs-pcie-win-io source",
    .type = OBS_SOURCE_TYPE_INPUT,
    //.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
    .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC,
    .get_name = raw_get_name,
    .create = raw_create,
    .destroy = raw_destroy,
    .get_width = [](void *data) { return ((raw_10bit_source *)data)->width; },
    .get_height = [](void *data) { return ((raw_10bit_source *)data)->height; },
    //.video_render = raw_video_render,  //OBS_SOURCE_CUSTOM_DRAW
    .video_tick = raw_video_tick,       //OBS_SOURCE_ASYNC
};

static const char *udp_filter_get_name(void *unused)
{
  UNUSED_PARAMETER(unused);
  return "UYVY UDP Streamer (127.0.0.1:5000)";
}

static void *udp_filter_create(obs_data_t *settings, obs_source_t *source)
{
  UNUSED_PARAMETER(settings);
  obs_log(LOG_INFO, ">>>udp_filter_create");
  auto *ctx = (udp_stream_filter *)bzalloc(sizeof(udp_stream_filter));
  ctx->source = source;
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

static void udp_filter_destroy(void *data)
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

static obs_properties_t *udp_filter_properties(void *data)
{
  UNUSED_PARAMETER(data);
  obs_properties_t *props = obs_properties_create();
  // Minimal: no configurable properties for now
  return props;
}

// This callback receives frames from the attached source.
// The OBS filter callback name used here is `filter_video` (OBS expects this member for filters).
static struct obs_source_frame *udp_filter_video(void *data, struct obs_source_frame *frame)
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
  size_t bytes = (size_t)frame->linesize[0] * (size_t)frame->height;
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

To implement Texture Sharing (Zero-Copy), you must shift from an "Asynchronous" source (pushing raw pixels) to a "Synchronous" source (rendering directly to the
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
if (!context->texture) {
    context->texture = gs_texture_create(width, height, GS_RGBA, 1, NULL, GS_DYNAMIC);
}

// Updating the texture with new data (this is the only "copy", RAM to GPU)
uint8_t *ptr;
uint32_t linesize;
if (gs_texture_map(context->texture, &ptr, &linesize)) {
    memcpy(ptr, context->buffer, linesize * height);
    gs_texture_unmap(context->texture);
}
obs_leave_graphics();
3. The video_render Callback
This function is called by OBS every time it draws a frame. Because you are using OBS_SOURCE_CUSTOM_DRAW, you are responsible for drawing the texture to the
screen.

C++
static void my_video_render(void *data, gs_effect_t *effect) {
    struct my_context *context = data;

    if (!context->texture) return;

    // Get the default "Draw" effect (simple texture shader)
    gs_effect_t *draw_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);

    // Set the texture as the 'image' parameter for the shader
    gs_effect_set_texture(gs_effect_get_param_by_name(draw_effect, "image"),
                          context->texture);

    // Draw the texture onto the current render target (the OBS canvas)
    while (gs_effect_loop(draw_effect, "Draw")) {
        obs_source_draw(context->texture, 0, 0, 0, 0, 0);
    }
}

*/