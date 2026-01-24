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
#pragma once
#include <cstdint>
void fill_uyvy_color_bars(uint8_t *buffer, int width, int height, bool is_10bit);
uint32_t v210_to_uyvy_avx2_vcl(const uint32_t *src, uint8_t *dst, int width, int height);
uint32_t v210_to_uyvy_avx2_opt(const uint32_t *__restrict src, uint8_t *__restrict dst, int width, int height);
uint32_t bgra_to_v210_avx2(const uint8_t *src, uint8_t *dst, int width, int height);

void rgb_to_yuv_fast(uint8_t r, uint8_t g, uint8_t b, uint16_t &y, uint16_t &u, uint16_t &v);
void rgb_to_yuv(uint8_t r, uint8_t g, uint8_t b, uint16_t &y, uint16_t &u, uint16_t &v);
void draw_line_avx2(uint8_t *buffer, int width, int height, uint8_t r, uint8_t g, uint8_t b, int pos, int thickness, bool horizontal, bool is_10bit);
