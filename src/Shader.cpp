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
#include <Shader.h>
//#include <graphics/graphics.h>
//#include <graphics/vec4.h>
//#include <string>

/* ========================================================================= */
/* 1. EMBEDDED SHADER CODE (v210 Unpacker)                                   */
/* ========================================================================= */
// We embed this as a raw string literal to avoid external file dependencies.
// In a real production plugin, you might load this from a .effect file.

const char *GL_v210_unpacker_effect_code = R"EFFECT(
uniform float4x4 ViewProj;
uniform texture2d image;
uniform float width_pixels;
uniform float alpha_val = 255.0;

sampler_state texture_sampler {
    Filter   = Point;
    AddressU = Clamp;
    AddressV = Clamp;
};

struct VertData {
    float4 pos : POSITION;
    float2 uv  : TEXCOORD0;
};

struct PixelData {
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

PixelData VSDefault(VertData v_in) {
    PixelData v_out;
    v_out.pos = mul(float4(v_in.pos.xyz, 1.0), ViewProj);
    v_out.uv  = v_in.uv;
    return v_out;
}

// Reconstruct 32-bit integer from GS_RGBA (which is uploaded as bytes)
uint LoadV210Word(int x, int y) {
    float4 raw = image.Load(int3(x, y, 0));
    // Multiply by 255.5 to perform safe rounding from float 0..1 to int 0..255
    uint b0 = uint(raw.r * 255.5);
    uint b1 = uint(raw.g * 255.5);
    uint b2 = uint(raw.b * 255.5);
    uint b3 = uint(raw.a * 255.5);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

// Convert 10-bit YUV to RGB
float3 YUV10ToRGB(float y_int, float u_int, float v_int) {
    float y = (y_int - 64.0) / 876.0;
    float u = (u_int - 512.0) / 896.0;
    float v = (v_int - 512.0) / 896.0;
    float r = y + 1.5748 * v;
    float g = y - 0.1873 * u - 0.4681 * v;
    float b = y + 1.8556 * u;
    return saturate(float3(r, g, b));
}

float4 PSUnpackV210(PixelData p_in) : SV_Target {
    int px = int(p_in.uv.x * width_pixels);
    int tex_y = int(p_in.uv.y * image.Length.y); 

    int block_idx = px / 6;
    int pixel_sub_idx = px % 6;
    int tex_x_base = block_idx * 4; 

    uint w0 = LoadV210Word(tex_x_base + 0, tex_y);
    uint w1 = LoadV210Word(tex_x_base + 1, tex_y);
    uint w2 = LoadV210Word(tex_x_base + 2, tex_y);
    uint w3 = LoadV210Word(tex_x_base + 3, tex_y);

    float Y=0, U=0, V=0;
    uint mask = 0x3FF; 

    if (pixel_sub_idx == 0) {
        U = float(w0 & mask); Y = float((w0 >> 10) & mask); V = float((w0 >> 20) & mask);
    } else if (pixel_sub_idx == 1) {
        U = float(w0 & mask); Y = float(w1 & mask); V = float((w0 >> 20) & mask);
    } else if (pixel_sub_idx == 2) {
        U = float((w1 >> 10) & mask); Y = float((w1 >> 20) & mask); V = float(w2 & mask);
    } else if (pixel_sub_idx == 3) {
        U = float((w1 >> 10) & mask); Y = float((w2 >> 10) & mask); V = float(w2 & mask);
    } else if (pixel_sub_idx == 4) {
        U = float((w2 >> 20) & mask); Y = float(w3 & mask); V = float((w3 >> 10) & mask);
    } else {
        U = float((w2 >> 20) & mask); Y = float((w3 >> 20) & mask); V = float((w3 >> 10) & mask);
    }

    return float4(YUV10ToRGB(Y, U, V), alpha_val / 255.0);
}

technique Draw {
    pass {
        vertex_shader = VSDefault(v_in);
        pixel_shader  = PSUnpackV210(p_in);
    }
}
)EFFECT";