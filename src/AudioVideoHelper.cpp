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
#include <AudioVideoHelper.h>
#include "vectorclass.h"
#include <algorithm>
#include <cstring>        // for memset
#define _USE_MATH_DEFINES // for C++
#include <math.h>
#if 0
// SMPTE color bars in YUV (BT.601) for 8 bars (White, Yellow, Cyan, Green, Magenta, Red, Blue, Black)
// Y, U, V values for each bar (8-bit, full range)
static const uint8_t BAR_Y[8] = {235, 210, 170, 145, 106, 81, 41, 16};
static const uint8_t BAR_U[8] = {128, 16, 166, 54, 202, 90, 240, 128};
static const uint8_t BAR_V[8] = {128, 146, 16, 34, 202, 218, 112, 128};

void fill_uyvy_color_bars(uint8_t *buffer, int width, int height, bool is_10bit)
{
  int bar_width = width / 8;

  for (int bar = 0; bar < 8; ++bar)
  {
    int x_start = bar * bar_width;
    int x_end = (bar == 7) ? width : (bar + 1) * bar_width;

    // Upscale 8-bit to 10-bit by shifting left 2 bits
    uint32_t y = BAR_Y[bar] << 2;
    uint32_t u = BAR_U[bar] << 2;
    uint32_t v = BAR_V[bar] << 2;

    if (is_10bit)
    {
      // V210 Format: Packs 6 pixels into 32-bit chunks
      // Ensure x_start and x_end alignment for simplified logic
      for (int x = x_start; x < x_end; x += 6)
      {
        for (int h = 0; h < height; ++h)
        {
          // v210 row stride: (width + 47) / 48 * 128 bytes
          uint32_t *row = (uint32_t *)(buffer + h * ((width + 47) / 48 * 128));
          uint32_t *pixel_block = row + (x / 6) * 4;

          // Word 0: [2-bit pad][10-bit V0][10-bit Y0][10-bit U0]
          pixel_block[0] = (v << 20) | (y << 10) | u;
          // Word 1: [2-bit pad][10-bit Y2][10-bit U1][10-bit Y1]
          pixel_block[1] = (y << 20) | (u << 10) | y;
          // Word 2: [2-bit pad][10-bit U2][10-bit Y3][10-bit V1]
          pixel_block[2] = (u << 20) | (y << 10) | v;
          // Word 3: [2-bit pad][10-bit Y5][10-bit V2][10-bit Y4]
          pixel_block[3] = (y << 20) | (v << 10) | y;
        }
      }
    }
    else
    {
      // Original 8-bit UYVY logic
      for (int x = x_start; x < x_end; x += 2)
      {
        for (int h = 0; h < height; ++h)
        {
          uint8_t *row = buffer + h * width * 2;
          int offset = x * 2;
          row[offset + 0] = (uint8_t)BAR_U[bar];
          row[offset + 1] = (uint8_t)BAR_Y[bar];
          row[offset + 2] = (uint8_t)BAR_V[bar];
          row[offset + 3] = (uint8_t)BAR_Y[bar];
        }
      }
    }
  }
}
#endif
void rgb_to_yuv_fast(uint8_t r, uint8_t g, uint8_t b, uint16_t &y, uint16_t &u, uint16_t &v)
{
  // BT.601 Full Range using 8-bit fixed point (scale by 256)
  // Y = 0.299R + 0.587G + 0.114B
  // U = -0.169R - 0.331G + 0.500B + 128
  // V = 0.500R - 0.419G - 0.081B + 128

  y = (uint16_t)((77 * r + 150 * g + 29 * b) >> 8);

  // We use int32 for intermediates to handle negative results before the +128 offset
  int32_t tu = (-43 * r - 85 * g + 128 * b) >> 8;
  int32_t tv = (128 * r - 107 * g - 21 * b) >> 8;

  u = (uint16_t)(tu + 128);
  v = (uint16_t)(tv + 128);

  // Optional: Clamp to 0-255 range to prevent overflow artifacts
  y = std::clamp<uint16_t>(y, 0, 255);
  u = std::clamp<uint16_t>(u, 0, 255);
  v = std::clamp<uint16_t>(v, 0, 255);
}

// RGB to YUV BT.601 Full Range helper
void rgb_to_yuv(uint8_t r, uint8_t g, uint8_t b, uint16_t &y, uint16_t &u, uint16_t &v)
{
  y = (uint16_t)((0.299 * r) + (0.587 * g) + (0.114 * b));
  u = (uint16_t)(-0.169 * r - 0.331 * g + 0.500 * b + 128);
  v = (uint16_t)(0.500 * r - 0.419 * g - 0.081 * b + 128);
}

void draw_line_avx2(uint8_t *buffer, int width, int height, uint8_t r, uint8_t g, uint8_t b, int pos, int thickness, bool horizontal, bool is_10bit)
{
  uint16_t Y, U, V;
  rgb_to_yuv(r, g, b, Y, U, V);

  // Round position and thickness to even to respect UYVY/V210 grouping
  pos &= ~1;
  thickness = (thickness + 1) & ~1;

  if (is_10bit)
  {
    // V210: 10-bit upscale
    uint32_t y = Y << 2, u = U << 2, v = V << 2;
    uint32_t w0 = (v << 20) | (y << 10) | u;
    uint32_t w1 = (y << 20) | (u << 10) | y;
    uint32_t w2 = (u << 20) | (y << 10) | v;
    uint32_t w3 = (y << 20) | (v << 10) | y;
    __m256i vec = _mm256_setr_epi32(w0, w1, w2, w3, w0, w1, w2, w3);
    int stride = ((width + 47) / 48) * 128;

    if (horizontal)
    {
      int y_pos = std::clamp(pos, 0, height - 1);
      int y_end = std::clamp(pos + thickness, 0, height);
      for (int h = y_pos; h < y_end; ++h)
      {
        uint32_t *row = (uint32_t *)(buffer + h * stride);
        for (int x = 0; x <= width - 12; x += 12)
        {
          _mm256_storeu_si256((__m256i *)(row + (x / 6) * 4), vec);
        }
      }
    }
    else
    { // Vertical
      int x_start = std::clamp(pos, 0, width - 6);
      for (int h = 0; h < height; ++h)
      {
        uint32_t *block = (uint32_t *)(buffer + h * stride) + (x_start / 6) * 4;
        for (int t = 0; t < thickness; t += 6)
        {
          block[0] = w0;
          block[1] = w1;
          block[2] = w2;
          block[3] = w3;
        }
      }
    }
  }
  else
  {
    // 8-bit UYVY
    uint32_t uyvy_pixel = (Y << 24) | (V << 16) | (Y << 8) | (uint8_t)U;
    __m256i vec = _mm256_set1_epi32(uyvy_pixel);
    int stride = width * 2;

    if (horizontal)
    {
      int y_pos = std::clamp(pos, 0, height - 1);
      int y_end = std::clamp(pos + thickness, 0, height);
      for (int h = y_pos; h < y_end; ++h)
      {
        uint8_t *row = buffer + h * stride;
        for (int x = 0; x <= width - 16; x += 16)
        {
          _mm256_storeu_si256((__m256i *)(row + x * 2), vec);
        }
      }
    }
    else
    { // Vertical
      int x_start = std::clamp(pos, 0, width - 2);
      for (int h = 0; h < height; ++h)
      {
        uint32_t *row32 = (uint32_t *)(buffer + h * stride + x_start * 2);
        for (int t = 0; t < thickness; t += 2)
        {
          *row32 = uyvy_pixel; // Writes 2 pixels at once
          row32++;
        }
      }
    }
  }
}
// 8-bit Constants
static const uint8_t BAR_Y8[8] = {235, 210, 170, 145, 106, 81, 41, 16};
static const uint8_t BAR_U8[8] = {128, 16, 166, 54, 202, 90, 240, 128};
static const uint8_t BAR_V8[8] = {128, 146, 16, 34, 202, 218, 112, 128};

// 10-bit Constants (Upscaled)
static const uint16_t BAR_Y10[8] = {940, 840, 680, 580, 424, 324, 164, 64};
static const uint16_t BAR_U10[8] = {512, 64, 664, 216, 808, 360, 960, 512};
static const uint16_t BAR_V10[8] = {512, 584, 64, 136, 808, 872, 448, 512};

void fill_uyvy_color_bars(uint8_t *buffer, int width, int height, bool is_10bit)
{
  const int bar_width = width / 8;

  for (int bar = 0; bar < 8; ++bar)
  {
    const int x_start = bar * bar_width;
    const int x_end = (bar == 7) ? width : (bar + 1) * bar_width;

    if (is_10bit)
    {
      const uint32_t y = BAR_Y10[bar];
      const uint32_t u = BAR_U10[bar];
      const uint32_t v = BAR_V10[bar];

      // V210 Packing: 6 pixels in 4 words (128 bits)
      const uint32_t w0 = (v << 20) | (y << 10) | u;
      const uint32_t w1 = (y << 20) | (u << 10) | y;
      const uint32_t w2 = (u << 20) | (y << 10) | v;
      const uint32_t w3 = (y << 20) | (v << 10) | y;

      const __m256i v210_vec = _mm256_setr_epi32(w0, w1, w2, w3, w0, w1, w2, w3);
      const int stride = ((width + 47) / 48) * 128;

      for (int h = 0; h < height; ++h)
      {
        uint32_t *row = (uint32_t *)(buffer + (h * stride));
        int x = x_start;
        for (; x <= x_end - 12; x += 12)
        {
          _mm256_storeu_si256((__m256i *)(row + (x / 6) * 4), v210_vec);
        }
        // Scalar cleanup (6-pixel granularity for V210)
        for (; x < x_end; x += 6)
        {
          uint32_t *block = row + (x / 6) * 4;
          block[0] = w0;
          block[1] = w1;
          block[2] = w2;
          block[3] = w3;
        }
      }
    }
    else
    {
      const uint8_t y = BAR_Y8[bar];
      const uint8_t u = BAR_U8[bar];
      const uint8_t v = BAR_V8[bar];

      // UYVY Packing: [Y V Y U]
      const uint32_t uyvy_word = (y << 24) | (v << 16) | (y << 8) | u;
      const __m256i uyvy_vec = _mm256_set1_epi32(uyvy_word);
      const int stride = width * 2;

      for (int h = 0; h < height; ++h)
      {
        uint8_t *row = buffer + (h * stride);
        int x = x_start;
        for (; x <= x_end - 16; x += 16)
        {
          _mm256_storeu_si256((__m256i *)(row + x * 2), uyvy_vec);
        }
        // Scalar cleanup (2-pixel granularity for UYVY)
        for (; x < x_end; x += 2)
        {
          row[x * 2 + 0] = u;
          row[x * 2 + 1] = y;
          row[x * 2 + 2] = v;
          row[x * 2 + 3] = y;
        }
      }
    }
  }
}

// Transforming v210 (10-bit 4:2:2) to UYVY (8-bit 4:2:2)
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

// Helpers for limiting values to valid YUV range
static inline int clamp_y(int v)
{
  return (v < 64) ? 64 : (v > 940) ? 940 : v;
}
static inline int clamp_c(int v)
{
  return (v < 64) ? 64 : (v > 960) ? 960 : v;
}

uint32_t bgra_to_v210_avx2(const uint8_t *src, uint8_t *dst, int width, int height)
{
  // v210 requires the stride to be 128-byte aligned (or at least word aligned),
  // effectively padding lines to multiples of 6 pixels (4 words).
  // Stride in bytes = ((Width + 5) / 6) * 16
  int stride_bytes = (width + 5) / 6 * 16;
  int total_bytes = stride_bytes * height;

  // Process 12 pixels at a time (Two v210 blocks = 32 bytes output)
  int width_aligned_12 = (width / 12) * 12;

  // Constants for RGB->YUV Rec.601 conversion (Fixed point 1.8.8)
  // Y = ( 66*R + 129*G +  25*B + 128) >> 8 + 16
  const __m256i y_r = _mm256_set1_epi16(66);
  const __m256i y_g = _mm256_set1_epi16(129);
  const __m256i y_b = _mm256_set1_epi16(25);
  const __m256i y_add = _mm256_set1_epi16(128);
  const __m256i y_offset = _mm256_set1_epi16(16 << 2); // 16 shifted for 10-bit (64)

  // U = (-38*R -  74*G + 112*B + 128) >> 8 + 128
  const __m256i u_r = _mm256_set1_epi16(-38);
  const __m256i u_g = _mm256_set1_epi16(-74);
  const __m256i u_b = _mm256_set1_epi16(112);
  const __m256i uv_add = _mm256_set1_epi16(128);
  const __m256i uv_offset = _mm256_set1_epi16(128 << 2); // 128 shifted for 10-bit (512)

  // V = (112*R -  94*G -  18*B + 128) >> 8 + 128
  const __m256i v_r = _mm256_set1_epi16(112);
  const __m256i v_g = _mm256_set1_epi16(-94);
  const __m256i v_b = _mm256_set1_epi16(-18);

  // Shuffle masks to separate B, G, R, A from BGRA stream
  // We load 12 pixels, so we need masks to arrange them into 16-bit integers
  // Pattern to pick byte 0, fill 0, byte 4, fill 0... (Blue)
  // Note: This naive approach requires unpacking.

  // MASK to select 8-bit components from 16-bit interleaved data
  const __m256i mask_low_byte = _mm256_set1_epi16(0x00FF);

  // V210 Packing Shuffles
  // We need to map linear Y0..Y5, U0..U2, V0..V2 into the v210 pattern.
  // The shuffle bytes below are indices into a register formed by packing (U, Y, V).
  // This part is highly specific to the variable names used in the loop below.

  uint8_t *current_dst_row = dst;
  const uint8_t *current_src_row = src;

  for (int y = 0; y < height; ++y)
  {
    uint32_t *d = (uint32_t *)current_dst_row;
    const uint8_t *s = current_src_row;
    int x = 0;

    // --- AVX2 LOOP (12 pixels) ---
    for (; x < width_aligned_12; x += 12)
    {
      // 1. Load 12 Pixels (48 bytes).
      // We load 2x32 bytes to cover it (overlapping or aligned).
      // Src is BGRA.
      __m256i p0_7 = _mm256_loadu_si256((const __m256i *)(s));       // Pixels 0-7
      __m256i p4_11 = _mm256_loadu_si256((const __m256i *)(s + 16)); // Pixels 4-11 (Overlaps!)

      // We need to organize this into R, G, B channels (16-bit)
      // Since data is B G R A, we can shuffle.
      // But we need 12 items. Let's process as two sets of 6 pixels?
      // Better: Process 8 pixels (p0_7) and 4 pixels (lower half of p8_15, derived from p4_11).
      // Actually, simplest is to shuffle p0_7 to get R,G,B (8 items)
      // and shuffle p4_11 to get R,G,B (items 8-11).

      // To simplify logic, let's treat pixels 0-7 (Vec A) and Pixels 6-13 (Wait, 8-11).
      // Let's reload strictly.
      __m128i s0 = _mm_loadu_si128((const __m128i *)(s));      // px 0-3
      __m128i s1 = _mm_loadu_si128((const __m128i *)(s + 16)); // px 4-7
      __m128i s2 = _mm_loadu_si128((const __m128i *)(s + 32)); // px 8-11

      // Expand to 16-bit B, G, R. (Ignore Alpha)
      // Planarize using pmovzx or shuffle.
      // B channel
      __m256i b0 = _mm256_cvtepu8_epi16(_mm_shuffle_epi8(s0, _mm_setr_epi8(0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1))); // 0..3
      __m256i b1 = _mm256_cvtepu8_epi16(_mm_shuffle_epi8(s1, _mm_setr_epi8(0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1))); // 4..7
      __m256i b2 = _mm256_cvtepu8_epi16(_mm_shuffle_epi8(s2, _mm_setr_epi8(0, 4, 8, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1))); // 8..11

      // Combine into one register for 0..7 and another for 8.. (Wait, registers hold 16 shorts)
      // We have 12 pixels. Let's put 0..7 in Reg1, 8..11 in Reg2.
      __m256i b_low = _mm256_or_si256(b0, _mm256_slli_si256(b1, 8)); // Pixels 0-7
      __m256i b_high = b2;                                           // Pixels 8-11 (lower 64 bits valid)

      // Repeat for G and R
      // G channel (Offset 1)
      __m256i g0 = _mm256_cvtepu8_epi16(_mm_shuffle_epi8(s0, _mm_setr_epi8(1, 5, 9, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)));
      __m256i g1 = _mm256_cvtepu8_epi16(_mm_shuffle_epi8(s1, _mm_setr_epi8(1, 5, 9, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)));
      __m256i g2 = _mm256_cvtepu8_epi16(_mm_shuffle_epi8(s2, _mm_setr_epi8(1, 5, 9, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)));
      __m256i g_low = _mm256_or_si256(g0, _mm256_slli_si256(g1, 8));
      __m256i g_high = g2;

      // R channel (Offset 2)
      __m256i r0 = _mm256_cvtepu8_epi16(_mm_shuffle_epi8(s0, _mm_setr_epi8(2, 6, 10, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)));
      __m256i r1 = _mm256_cvtepu8_epi16(_mm_shuffle_epi8(s1, _mm_setr_epi8(2, 6, 10, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)));
      __m256i r2 = _mm256_cvtepu8_epi16(_mm_shuffle_epi8(s2, _mm_setr_epi8(2, 6, 10, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)));
      __m256i r_low = _mm256_or_si256(r0, _mm256_slli_si256(r1, 8));
      __m256i r_high = r2;

      // 2. Compute Y (For all 12 pixels)
      // We use mullo (16-bit mul) because coeff*val < 32768
      // Y = ((R*66 + G*129 + B*25 + 128) >> 8) + 16
      // We multiply, add, then shift.
      auto compute_Y = [&](__m256i r, __m256i g, __m256i b) {
        __m256i sum = _mm256_add_epi16(_mm256_mullo_epi16(r, y_r), _mm256_mullo_epi16(g, y_g));
        sum = _mm256_add_epi16(sum, _mm256_mullo_epi16(b, y_b));
        sum = _mm256_add_epi16(sum, y_add);
        sum = _mm256_srli_epi16(sum, 8);
        // Shift to 10-bit range (<< 2) + offset (16<<2)
        return _mm256_add_epi16(_mm256_slli_epi16(sum, 2), y_offset);
      };

      __m256i Y_0_7 = compute_Y(r_low, g_low, b_low);
      __m256i Y_8_11 = compute_Y(r_high, g_high, b_high);

      // 3. Compute U and V (Subsampled 4:2:2)
      // We need 6 U values and 6 V values.
      // Horizontal Pair Average: (P0+P1)/2, (P2+P3)/2...
      // hadd adds adjacent pairs.
      // _mm256_hadd_epi16(A, B) -> A0+A1, A2+A3... B0+B1...
      // Note: hadd works within 128-bit lanes.
      // r_low (0..7) -> hadd -> (0+1, 2+3, 4+5, 6+7, 0+1?? No lane crossing)

      __m256i r_sum = _mm256_hadd_epi16(r_low, r_high); // Lane0: 0+1..6+7. Lane1: 8+9, 10+11, garbage
      __m256i g_sum = _mm256_hadd_epi16(g_low, g_high);
      __m256i b_sum = _mm256_hadd_epi16(b_low, b_high);

      // Shift right by 1 to divide by 2
      __m256i r_sub = _mm256_srli_epi16(r_sum, 1);
      __m256i g_sub = _mm256_srli_epi16(g_sum, 1);
      __m256i b_sub = _mm256_srli_epi16(b_sum, 1);

      auto compute_UV = [&](__m256i r, __m256i g, __m256i b, __m256i wr, __m256i wg, __m256i wb) {
        __m256i sum = _mm256_add_epi16(_mm256_mullo_epi16(r, wr), _mm256_mullo_epi16(g, wg));
        sum = _mm256_add_epi16(sum, _mm256_mullo_epi16(b, wb));
        sum = _mm256_add_epi16(sum, uv_add);
        sum = _mm256_srli_epi16(sum, 8);
        return _mm256_add_epi16(_mm256_slli_epi16(sum, 2), uv_offset);
      };

      __m256i U_all = compute_UV(r_sub, g_sub, b_sub, u_r, u_g, u_b);
      __m256i V_all = compute_UV(r_sub, g_sub, b_sub, v_r, v_g, v_b);

      // 4. Pack into v210 (The tricky part)
      // We have:
      // Y_0_7 (Indices 0-7), Y_8_11 (Indices 0-3 in lower lane)
      // U_all (Indices 0-3 in lower lane are U0..U3. Indices 0-1 in upper lane are U4..U5)
      // V_all (Same)

      // We need to consolidate Y, U, V into one continuous register or mapped registers.
      // Let's create a "Y_Final" register with Y0..Y11 packed linearly.
      // Y_8_11 is in the lower 64 bits of the Y_8_11 register.
      // We need to move it to the upper 64 bits of Y_0_7? No, Y_0_7 is full.
      // We actually need to pick specific Y's for specific words.

      // v210 Block 0 (Words 0-3): Y0..Y5, U0..U2, V0..V2
      // v210 Block 1 (Words 4-7): Y6..Y11, U3..U5, V3..V5

      // Let's create registers for the 3 bit-slots in the final 32-bit words:
      // Slot A (Bits 0-9), Slot B (Bits 10-19), Slot C (Bits 20-29).
      // We need 8 integers total (256-bit register).

      // Mapping Table for the 8 Output Words:
      // W0: U0, Y0, V0
      // W1: Y1, U1, Y2
      // W2: V1, Y3, U2
      // W3: Y4, V2, Y5
      // -- Block Boundary --
      // W4: U3, Y6, V3
      // W5: Y7, U4, Y8
      // W6: V4, Y9, U5
      // W7: Y10, V5, Y11

      // We need to construct vectors of 16-bit integers and then PACK them.
      // However, shuffles operate within 128-bit lanes.
      // W0-W3 (Block 0) depends on Y0-Y5, U0-U2, V0-V2.
      // W4-W7 (Block 1) depends on Y6-Y11, U3-U5, V3-V5.
      // This is clean! Lane 0 handles Block 0, Lane 1 handles Block 1.

      // Prepare Sources for Lane 0:
      // Y_src0: Y0..Y7 (Already in Y_0_7 lower lane)
      // U_src0: U0..U3 (In U_all lower lane)
      // V_src0: V0..V3 (In V_all lower lane)

      // Prepare Sources for Lane 1:
      // Y_src1: Y8..Y11. We need Y6, Y7 too.
      // Y6, Y7 are in the upper 32-bits of Y_0_7's lower lane? No, Y_0_7 is 8 shorts.
      // Y_0_7 = [Y0 Y1 Y2 Y3 | Y4 Y5 Y6 Y7] (128-bit lane view is wrong here, it's 256).
      // AVX2 Registers: [0..3, 4..7] is not how it works. It's [0..7] in one 128-bit lane? No.
      // __m256i is two 128-bit lanes.
      // Lane 0: shorts 0-7. Lane 1: shorts 8-15.
      // Y_0_7: Lane 0 has Y0..Y7. Lane 1 is empty (we didn't load there).
      // Wait, compute_Y was 256-bit.
      // r_low was 256 bit: [0..7 (Lane0)] | [Garbage (Lane1)].
      // So Y_0_7 has Y0..Y7 in Lane 0.
      // Y_8_11 has Y8..Y11 in Lane 0 (because we loaded into low bits of r_high).

      // We need to fix the lanes.
      // We want Lane 0 to contain Data for Block 0 (Px 0-5)
      // We want Lane 1 to contain Data for Block 1 (Px 6-11)

      // Data needed for Lane 0: Y0-Y5, U0-U2, V0-V2
      // Data needed for Lane 1: Y6-Y11, U3-U5, V3-V5

      // Current State:
      // Y_0_7: [Y0..Y7] [0..0]
      // Y_8_11: [Y8..Y11..] [0..0]
      // U_all: [U0..U3 (Lane0 low), U4..U5 (Lane1 low? No, hadd result)]
      // _mm256_hadd_epi16 on [A0..A7][B0..B7] produces [A0+1..A6+7][B0+1..B6+7]
      // So U_all: Lane 0 has U0..U3. Lane 1 has U4..U5.

      // Fix Y Layout:
      // Combine Y0-Y7 and Y8-Y11 into one register where:
      // Lane 0: Y0..Y7 (We only need 0-5)
      // Lane 1: Y6..Y11 (We have Y6,Y7 in Lane0, Y8-11 in another reg).
      // Permute Y_0_7 to move Y6,Y7 to Lane 1?
      // Actually, `vpermt2w` (AVX512) would be nice. In AVX2, we use `vpermq` or insert.

      // Let's create a combined Y register:
      // Lane 0: Y0..Y7
      // Lane 1: Y4..Y11 (We need indices relative to 0 for shuffle).
      // We can construct Lane 1 by: `vpalignr` or combining parts of Y_0_7 and Y_8_11.
      // Or simpler: Just store them to stack and reload? No, slow.
      // Let's build "Y_Lane1_Source".
      // We need [Y6 Y7 Y8 Y9 Y10 Y11 X X].
      // We have [Y0..Y7] and [Y8..Y11].
      // Extract Y8..Y11 (64 bits) from Y_8_11.
      // Extract Y6..Y7 (32 bits) from Y_0_7 (High part of Lane 0).

      // Y_Block0 (Lane 0): Directly use Y_0_7. (Indices 0..5 are valid).
      // Y_Block1 (Lane 1): We need a register with Y6..Y11 in positions 0..5.
      // Y6 is at idx 6 in Y_0_7.
      // Shift Y_0_7 right by 6 shorts? No cross-lane shift in AVX2 easily.
      // But we can extract 128-bit lane.
      __m128i y_lane0 = _mm256_castsi256_si128(Y_0_7);
      __m128i y_part2 = _mm256_castsi256_si128(Y_8_11); // Y8..Y11

      // Need Y6..Y7 from y_lane0 (bytes 12-15).
      // Combine [Y6 Y7] [Y8 Y9 Y10 Y11] [X X]
      __m128i y_lane1 = _mm_alignr_epi8(y_part2, y_lane0, 12); // Val2<< | Val1 >> 12.
      // alignr shifts right. We want y_lane0 high bytes at bottom.
      // alignr(A, B, 12) -> Takes A(items shift in) and B(shifted out).
      // We want [Y6 Y7 (from lane0)] followed by [Y8... (from part2)].
      // correct usage: _mm_alignr_epi8(y_part2, y_lane0, 12).
      // This puts bytes 12-15 of lane0 (Y6, Y7) at pos 0. Then bytes 0-11 of part2 (Y8..). Perfect.

      // Now reconstruct 256-bit Y source.
      __m256i Y_combined = _mm256_inserti128_si256(_mm256_castsi128_si256(y_lane0), y_lane1, 1);

      // Fix U/V Layout:
      // U_all: Lane 0 has U0..U3. Lane 1 has U4..U5.
      // Block 0 needs U0..U2. (Lane 0 has them).
      // Block 1 needs U3..U5.
      // Lane 1 currently has U4, U5 at pos 0, 1.
      // Lane 0 has U3 at pos 3.
      // We need a register for Lane 1 that has U3, U4, U5 at pos 0, 1, 2.
      // Same alignr trick.
      __m128i u_lane0 = _mm256_castsi256_si128(U_all);
      __m128i u_part2 = _mm256_extracti128_si256(U_all, 1);
      __m128i u_lane1 = _mm_alignr_epi8(u_part2, u_lane0, 6); // Offset 3 shorts = 6 bytes. U3..U5.
      __m256i U_combined = _mm256_inserti128_si256(_mm256_castsi128_si256(u_lane0), u_lane1, 1);

      __m128i v_lane0 = _mm256_castsi256_si128(V_all);
      __m128i v_part2 = _mm256_extracti128_si256(V_all, 1);
      __m128i v_lane1 = _mm_alignr_epi8(v_part2, v_lane0, 6);
      __m256i V_combined = _mm256_inserti128_si256(_mm256_castsi128_si256(v_lane0), v_lane1, 1);

      // Shuffles for Bit Positions
      // Indices are bytes. 16-bit elements = index * 2.
      // Block Pattern:
      // Slot A (0-9):   U0(0), Y1(2), V1(2), Y4(8)
      // Slot B (10-19): Y0(0), U1(2), Y3(6), V2(4)
      // Slot C (20-29): V0(0), Y2(4), U2(4), Y5(10)

      // Byte Masks (for pshufb). 0x80 clears.
      // Note: Each lane processes 4 output words (16 bytes).
      // Input data is 16-bit. pshufb works on bytes.
      // To pick `short` at index 2, we need bytes 4, 5.

      // Slot A Source Selection:
      // W0: U0 (Idx 0) -> Bytes 0, 1 from U_vec
      // W1: Y1 (Idx 1) -> Bytes 2, 3 from Y_vec
      // W2: V1 (Idx 1) -> Bytes 2, 3 from V_vec
      // W3: Y4 (Idx 4) -> Bytes 8, 9 from Y_vec

      // Since sources are different registers (Y, U, V), we can't do one shuffle.
      // But look: Slot A uses U, Y, V, Y.
      // Slot B uses Y, U, Y, V.
      // Slot C uses V, Y, U, Y.
      // It's mixed.

      // Strategy: Gather all necessary components into one register per Slot via OR?
      // Better: Mask and OR.
      // Slot A = Shuffle(U) & Mask_U  | Shuffle(Y) & Mask_Y ...

      // Mask for Slot A (Target 32-bit words 0..3)
      // W0: U, W1: Y, W2: V, W3: Y
      const __m256i shuf_A_Y = _mm256_setr_epi8(-1, -1, 2, 3, -1, -1, 8, 9, -1, -1, 2, 3, -1, -1, 8, 9,  // Lane 1 repeat (relative)
                                                -1, -1, 2, 3, -1, -1, 8, 9, -1, -1, 2, 3, -1, -1, 8, 9); // Wait, setr is for 32 bytes? Yes.
      // Actually, simply use -1 for unused slots.

      // Let's define the byte sequences for the 16 bytes in a lane:
      // W0(0-3), W1(4-7), W2(8-11), W3(12-15).
      // We are building the LOW 10 bits (Slot A).
      // We will pack them into 32-bit integers later.
      // First, let's just get the 16-bit values into the right "slots" (0, 1, 2, 3).
      // Then we can convert 16-bit to 32-bit and shift.

      // Actually, let's assemble 3 vectors of 32-bit integers:
      // VecA: The values for bits 0-9.
      // VecB: The values for bits 10-19.
      // VecC: The values for bits 20-29.

      // This requires mapping 16-bit source to 32-bit dest.
      // W0 takes U0. Source U0 is bytes 0,1. Dest is bytes 0,1 (and 2,3 zero).

      // Slot A (Bits 0-9)
      // Lane0: U0, Y1, V1, Y4
      __m256i val_A = _mm256_or_si256(
          _mm256_shuffle_epi8(U_combined, _mm256_setr_epi8(0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                           -1, -1, -1, -1, -1, -1)), // U0 / U3
          _mm256_or_si256(_mm256_shuffle_epi8(Y_combined, _mm256_setr_epi8(-1, -1, -1, -1, 2, 3, -1, -1, -1, -1, -1, -1, 8, 9, -1, -1, -1, -1, -1, -1, 2, 3, -1,
                                                                           -1, -1, -1, -1, -1, 8, 9, -1, -1)), // Y1, Y4 / Y7, Y10
                          _mm256_shuffle_epi8(V_combined, _mm256_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 2, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                           -1, -1, 2, 3, -1, -1, -1, -1, -1, -1)) // V1 / V4
                          ));

      // Slot B (Bits 10-19)
      // Lane0: Y0, U1, Y3, V2
      __m256i val_B =
          _mm256_or_si256(_mm256_shuffle_epi8(Y_combined, _mm256_setr_epi8(0, 1, -1, -1, -1, -1, -1, -1, 6, 7, -1, -1, -1, -1, -1, -1, 0, 1, -1, -1, -1, -1, -1,
                                                                           -1, 6, 7, -1, -1, -1, -1, -1, -1)), // Y0, Y3
                          _mm256_or_si256(_mm256_shuffle_epi8(U_combined, _mm256_setr_epi8(-1, -1, -1, -1, 2, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                           -1, -1, 2, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)), // U1
                                          _mm256_shuffle_epi8(V_combined, _mm256_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, 5, -1, -1, -1, -1,
                                                                                           -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, 5, -1, -1)) // V2
                                          ));

      // Slot C (Bits 20-29)
      // Lane0: V0, Y2, U2, Y5
      __m256i val_C =
          _mm256_or_si256(_mm256_shuffle_epi8(V_combined, _mm256_setr_epi8(0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, -1, -1, -1, -1,
                                                                           -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)), // V0
                          _mm256_or_si256(_mm256_shuffle_epi8(Y_combined, _mm256_setr_epi8(-1, -1, -1, -1, 4, 5, -1, -1, -1, -1, -1, -1, 10, 11, -1, -1, -1, -1,
                                                                                           -1, -1, 4, 5, -1, -1, -1, -1, -1, -1, 10, 11, -1, -1)), // Y2, Y5
                                          _mm256_shuffle_epi8(U_combined, _mm256_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                                           -1, -1, -1, -1, -1, -1, 4, 5, -1, -1, -1, -1, -1, -1)) // U2
                                          ));

      // Compose Final 32-bit Words
      // ValA | (ValB << 10) | (ValC << 20)
      __m256i final_vec = _mm256_or_si256(val_A, _mm256_slli_epi32(val_B, 10));
      final_vec = _mm256_or_si256(final_vec, _mm256_slli_epi32(val_C, 20));

      // Store 32 bytes (2 v210 blocks)
      _mm256_storeu_si256((__m256i *)d, final_vec);

      s += 48; // 12 pixels * 4 bytes
      d += 8;  // 8 uint32_t output
    }

    // --- SCALAR FALLBACK ---
    // Handle remaining pixels (if width is not a multiple of 12)
    // Note: v210 blocks must be multiples of 6 pixels.
    // We process in groups of 6 until done.
    for (; x < width; x += 6)
    {
      // If less than 6 pixels remain, v210 usually requires padding the rest of the 128-bit block with 0.
      // We will process the valid pixels and pad the rest.

      uint32_t yuv[6][3]; // [Pixel][Y,U,V]

      for (int k = 0; k < 6; ++k)
      {
        if (x + k < width)
        {
          uint8_t b = s[k * 4 + 0];
          uint8_t g = s[k * 4 + 1];
          uint8_t r = s[k * 4 + 2];

          // RGB -> YUV
          int y_val = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
          int u_val = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
          int v_val = ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;

          yuv[k][0] = clamp_y(y_val) << 2; // 10-bit
          yuv[k][1] = clamp_c(u_val) << 2;
          yuv[k][2] = clamp_c(v_val) << 2;
        }
        else
        {
          yuv[k][0] = 64 << 2;  // Black Y
          yuv[k][1] = 512 << 2; // Neutral U
          yuv[k][2] = 512 << 2; // Neutral V
        }
      }

      // Subsample 4:2:2 (Average pairs)
      // U0 = (U0+U1)/2, U1 = (U2+U3)/2, U2 = (U4+U5)/2
      uint32_t U[3], V[3];
      for (int k = 0; k < 3; ++k)
      {
        U[k] = (yuv[k * 2][1] + yuv[k * 2 + 1][1]) >> 1;
        V[k] = (yuv[k * 2][2] + yuv[k * 2 + 1][2]) >> 1;
      }

      // Pack 6 pixels into 4 words
      // Word 0: U0, Y0, V0
      d[0] = (U[0]) | (yuv[0][0] << 10) | (V[0] << 20);
      // Word 1: Y1, U1, Y2
      d[1] = (yuv[1][0]) | (U[1] << 10) | (yuv[2][0] << 20);
      // Word 2: V1, Y3, U2
      d[2] = (V[1]) | (yuv[3][0] << 10) | (U[2] << 20);
      // Word 3: Y4, V2, Y5
      d[3] = (yuv[4][0]) | (V[2] << 10) | (yuv[5][0] << 20);

      s += 24; // 6 pixels * 4 bytes
      d += 4;
    }

    // Pad line to 128-byte alignment if needed (handled by stride)
    current_dst_row += stride_bytes;
    current_src_row += width * 4;
  }

  return total_bytes;
}

void GenerateAudioSinusData(const double frequency, const double amplitude, const double sample_rate, uint32_t samples_to_generate, int32_t *pSample_S32,
                            double &_rAudioPhase_lf)
{
  // const double frequency = 440.0;
  // const double amplitude = 0.5 * 2147483647.0;
  // const double sample_rate = (double)context->audio_sample_rate;
  const double twopi = 2.0 * M_PI;

  // 2. Generate exactly 'samples_to_generate'
  for (uint32_t i = 0; i < samples_to_generate; i++)
  {
    pSample_S32[i] = (int32_t)(amplitude * sin(_rAudioPhase_lf));

    _rAudioPhase_lf += (twopi * frequency) / sample_rate;
    if (_rAudioPhase_lf > twopi)
    {
      _rAudioPhase_lf -= twopi;
    }
  }
}

void GenerateMultiChannelAudioSinusData(const double *frequencies, const double *amplitudes, const bool *channel_enabled, int num_channels,
                                        const double sample_rate, uint32_t samples_to_generate, int32_t *pSample_S32, double *channel_phases)
{
  const double twopi = 2.0 * M_PI;

  // Clear the output buffer first
  memset(pSample_S32, 0, samples_to_generate * num_channels * sizeof(int32_t));

  // Generate samples for each channel
  for (int channel = 0; channel < num_channels; channel++)
  {
    if (!channel_enabled[channel])
    {
      continue; // Skip disabled channels
    }

    const double frequency = frequencies[channel];
    const double amplitude = amplitudes[channel];
    double &phase = channel_phases[channel];

    // Generate samples for this channel
    for (uint32_t sample = 0; sample < samples_to_generate; sample++)
    {
      // Interleaved format: sample_index = sample * num_channels + channel
      int32_t sample_value = (int32_t)(amplitude * sin(phase));
      pSample_S32[sample * num_channels + channel] = sample_value;

      // Update phase for next sample
      phase += (twopi * frequency) / sample_rate;
      if (phase > twopi)
      {
        phase -= twopi;
      }
    }
  }
}