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
/*
For working with the 16-channel interleaved audio from our plugin in Multi-Channel Mode, here are the best OBS Studio options for routing, effects, and channel
ganging:

Built-in OBS Audio Tools:
-Audio Filters (Per Source):
-Gain: Adjust individual channel levels
-Compressor: Dynamic range control
-Noise Gate: Eliminate background noise
-EQ: 3-band equalizer
-Limiter: Prevent clipping
-VST 2.x Plugin: Load external VST effects
Audio Monitoring:
-Audio Advanced Properties: Set monitoring to "Monitor Only" or "Monitor and Output"
-Audio Mixer: Individual mute/unmute per source
Recommended Third-Party Plugins:
1. ReaPlugs (Free VST Suite)
-ReaEQ: Advanced equalizer with multiple bands
-ReaComp: Professional compressor
-ReaGate: Advanced noise gate
-ReaChannelMap: ⭐ Perfect for channel routing and ganging!
2. Audio Routing & Channel Management:
-Audio Monitor Plugin: Advanced audio routing and monitoring
-Source Record Plugin: Record individual channels separately
-Audio Output Capture: Capture system audio for routing
3. Professional VST Plugins:
-Waves plugins: Professional audio processing
-FabFilter: High-quality EQ and dynamics
-Reaper ReaPlugs: Free and powerful
-Channel Ganging to Create Stereo:
Method 1: Using ReaChannelMap (Recommended)
-Install ReaPlugs VST suite
-Add "VST 2.x Plugin" filter to your obs-evs-pcie-win-io source
-Select "ReaChannelMap"
-Configure channel routing:
-Input channels 1+2 → Output Left+Right
-Input channels 3+4 → Output Left+Right (different mix)
-Apply effects per stereo pair
Method 2: Using OBS Scene Collections
-Create multiple Audio Sources that reference the same plugin instance
-Use Audio Advanced Properties to assign different channels
-Group sources for stereo effects
Method 3: External Audio Software
-VoiceMeeter: Virtual audio mixer for advanced routing
-Virtual Audio Cable: Route audio between applications
-JACK Audio: Professional audio routing (Windows/Linux)
Practical Workflow Example:
Quick Setup Steps:
-Download ReaPlugs: Free from Cockos Reaper website
-Install VST plugins: Place in OBS VST folder
-Add VST Filter: Right-click your source → Filters → VST 2.x Plugin
-Configure Routing: Use ReaChannelMap to gang channels into stereo pairs
-Apply Effects: Add additional VST filters for each stereo pair
-ReaChannelMap is particularly powerful because it can:
? Gang any 2 channels into stereo
? Apply different routing matrices
? Adjust levels per channel
? Mix multiple input channels to single outputs

Configuration Spécifique de ReaChannelMap pour Ganger les Canaux
Installation et Accès:
Télécharger ReaPlugs: www.reaper.fm/reaplugs/
Installer dans le dossier VST d'OBS (généralement C:\Program Files\obs-studio\obs-plugins\64bit\)
Redémarrer OBS Studio
Configuration dans OBS:
Clic droit sur votre source "obs-evs-pcie-win-io source"
Filtres ? + ? VST 2.x Plugin
Sélectionner: ReaChannelMap
Ouvrir l'interface du plugin
Configuration ReaChannelMap pour 8 Paires Stéréo:
Configuration Matrix (16 canaux ? 8 paires stéréo):

INPUT (vos 16 canaux)    ?    OUTPUT (8 paires stéréo)
+-----------------------------------------------------+
¦ Canal 1  ? Output Left 1    (Paire Stéréo A)       ¦
¦ Canal 2  ? Output Right 1                           ¦
¦ Canal 3  ? Output Left 2    (Paire Stéréo B)       ¦
¦ Canal 4  ? Output Right 2                           ¦
¦ Canal 5  ? Output Left 3    (Paire Stéréo C)       ¦
¦ Canal 6  ? Output Right 3                           ¦
¦ ...                                                 ¦
¦ Canal 15 ? Output Left 8    (Paire Stéréo H)       ¦
¦ Canal 16 ? Output Right 8                           ¦
+-----------------------------------------------------+

Interface ReaChannelMap - Étapes Détaillées:
1. Configuration des Inputs:
Inputs: Mettre à 16 (vos 16 canaux source)
Outputs: Mettre à 16 (8 paires × 2 canaux)
2. Matrice de Routage:
Dans la grille, pour chaque paire:

Paire Stéréo 1 (Canaux 1+2):
- Input 1 ? Output 1 (Left):  Gain = 1.0
- Input 2 ? Output 2 (Right): Gain = 1.0

Paire Stéréo 2 (Canaux 3+4):
- Input 3 ? Output 3 (Left):  Gain = 1.0
- Input 4 ? Output 4 (Right): Gain = 1.0

3. Contrôles Avancés:

Pour chaque paire, vous pouvez:
+- Gain individuel par canal (-8 à +12dB)
+- Pan control (si mono ? stéréo)
+- Phase invert (bouton f)
+- Mute individuel par canal
+- Solo pour isolation

Presets Recommandés:
Preset 1: 8 Paires Stéréo Simples

Channels 1+2  ? Stereo Pair A (Sine 440Hz + 550Hz)
Channels 3+4  ? Stereo Pair B (Sine 660Hz + 770Hz)
Channels 5+6  ? Stereo Pair C (Sine 880Hz + 990Hz)
Channels 7+8  ? Stereo Pair D (Sine 1100Hz + 1210Hz)
Channels 9-16 ? Disabled ou gain = 0

Preset 2: Mix Créatif

Channels 1+3  ? Stereo Pair A (Mix harmonies)
Channels 2+4  ? Stereo Pair B (Mix complémentaires)
Channels 5+7  ? Stereo Pair C
Channels 6+8  ? Stereo Pair D
Contrôle en Temps Réel:
? Gain temps réel pour chaque canal
? Mute instantané par paire
? Monitoring par canal ou paire
? Sauvegarde des réglages dans la scène OBS
Cette configuration vous donnera un contrôle total sur le groupement de vos 16 canaux en paires stéréo, avec la possibilité d'appliquer des effets différents
sur chaque paire par la suite!

Persistence:
OBS Handles Persistence Automatically:
? Scene Collection Files: Settings saved in .json files
? Source Duplication: Settings copied when duplicating sources
? Scene Switching: Settings maintained across scene changes
? OBS Restart: All settings restored on restart
3. Where Settings Are Stored:

Windows: %APPDATA%\obs-studio\basic\scenes\*.json
Each scene collection contains:
{
  "sources": [
    {
      "id": "obs-evs-pcie-win-io",
      "settings": {
        "use_sine_generator": true,
        "split_audio_mode": false,
        "sine_enabled_1": true,
        "sine_frequency_1": 440.0,
        "sine_amplitude_1": 0.3,
        // ... all your settings
      }
    }
  ]
}

4. Your Plugin Property Flow:
1. Source Creation ? v210_get_defaults() ? Sets initial values
2. User Changes UI ? v210_update() ? Updates context variables
3. OBS Auto-Save ? Settings written to scene collection
4. OBS Restart ? Settings loaded ? v210_update() called with saved values

Advanced Persistence Options (if needed):
Manual Save/Load (rarely needed):
// Save custom data
obs_data_t *save_data = obs_save_source(source);
// Load custom data
obs_load_source(source, save_data);
*/
#include <winsock2.h>
#include <ws2tcpip.h>
#include <obs-module.h>
#include <plugin-support.h>
// #include <obs-frontend-api.h>  // Comment out for now if it's causing build issues
#include "PluginUtil.h"
#include "AudioVideoHelper.h"
#include "Shader.h"

#define IN10BITS
#define VIDEO_BAR  // Max perf
#define MOVING_BAR // Low perf as we generate a full new picture at every frame
#define AUDIO_SIN  // Max perf

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")
static const char *PLUGIN_INPUT_NAME = "obs-evs-pcie-win-io source";
#define PLUGIN_AUDIO_CHANNELS 16

// Structure for individual sine wave audio channels (Option 2)
struct sine_channel_source
{
  obs_source_t *source;
  uint32_t audio_sample_rate; // 48000 Hz
  size_t audio_frame_size;    // bytes per audio frame
  uint8_t *audio_buffer;
  double audio_phase;
  double audio_remainder_seconds;

  // Sine wave parameters
  double frequency;   // Frequency for this channel
  double amplitude;   // Amplitude for this channel
  bool enabled;       // Enable/disable this channel
  int channel_number; // Channel number (1-16)
};

struct v210_source
{
  obs_source_t *source;
  FILE *video_file;
  // Graphics resources
  gs_effect_t *effect;
  gs_texture_t *texture;

  // Shader Parameters
  gs_eparam_t *param_image;
  gs_eparam_t *param_width;
  gs_eparam_t *param_alpha;

  // Data handling
  uint8_t *buffer; // Pointer to your CPU v210 buffer
  uint32_t width;  // Video Width (e.g. 1920)
  uint32_t height; // Video Height (e.g. 1080)
  uint32_t fps;
  size_t frame_size;
  float q_alpha; // The alpha value (default 255.0)
  uint32_t line_pos;

  // Audio members
  FILE *audio_file;
  uint32_t audio_sample_rate; // 48000 Hz
  uint32_t audio_channels;    // typically 2 for stereo
  size_t audio_frame_size;    // bytes per audio frame
  uint8_t *audio_buffer;
  double audio_phase;
  double audio_remainder_seconds;

  // Multi-channel sine wave parameters (16 channels)
  double sine_frequencies[16];  // Frequency for each channel
  double sine_amplitudes[16];   // Amplitude for each channel
  bool sine_enabled[16];        // Enable/disable each channel
  double sine_phases[16];       // Phase tracking for each channel
  uint8_t *channel_buffers[16]; // Independent buffer for each channel (for separate mode)
  bool use_sine_generator;      // Toggle between sine and file audio
  bool split_audio_mode;        // Runtime control: true = separate sources, false = multi-channel

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

static const char *v210_get_name(void *type_data)
{
  obs_log(LOG_INFO, ">>>v210_get_name %p->%s", type_data, PLUGIN_INPUT_NAME);
  return PLUGIN_INPUT_NAME;
}

static void *v210_create(obs_data_t *settings, obs_source_t *source)
{
  v210_source *context = (v210_source *)bzalloc(sizeof(v210_source));
  obs_log(LOG_INFO, ">>>v210_create %p %p", settings, source);
  context->source = source;

  // Hardcoded for your specific use case, or pull from settings
#if defined(IN10BITS)
  context->width = 1920;
  context->height = 1080;
  context->fps = 30;
  context->frame_size =
      EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::ComputeImageSize(EVS::EvsPcieIoApi::VideoStd_1080p_59_94, FOURCC_V210, context->width, context->height, false);
  //  EVS::EvsPcieIoApi::CEvsPcieIoApiHelper::ComputeImageSize(EVS::EvsPcieIoApi::VideoStd_1080i_59_94, FOURCC_V210, context->width, context->height, false);
  context->buffer = (uint8_t *)bmalloc(context->frame_size);
  fill_uyvy_color_bars(context->buffer, context->width, context->height, true);
#else
  context->width = 640;
  context->height = 480;
  context->fps = 60;
  context->frame_size = context->width * context->height * 2;
  context->buffer = (uint8_t *)bmalloc(context->frame_size);
  fill_uyvy_color_bars(context->buffer, context->width, context->height, false);
#endif

  // Create texture
  obs_enter_graphics();
  context->texture = gs_texture_create(context->width, context->height, GS_BGRA, 1, NULL, GS_DYNAMIC);
  obs_leave_graphics();

  // Audio setup: 48 kHz, 16-channel, 32-bit signed int
  context->audio_sample_rate = 48000;
  context->audio_channels = 16;                                 // 16-channel audio (mode-independent)
  context->audio_frame_size = sizeof(int32_t) * 16 * 16 * 1024; // Large buffer for both modes
  context->audio_buffer = (uint8_t *)bmalloc(context->audio_frame_size);

  // Initialize multi-channel sine wave parameters
  context->use_sine_generator = true; // Default to sine generator
  context->split_audio_mode = false;  // Default to multi-channel mode
  for (int i = 0; i < 16; i++)
  {
    context->sine_frequencies[i] = 440.0 + (i * 110.0);                          // Different frequencies: 440, 550, 660...
    context->sine_amplitudes[i] = 0.3;                                           // Default amplitude
    context->sine_enabled[i] = (i < 2);                                          // Enable first 2 channels by default
    context->sine_phases[i] = 0.0;                                               // Initialize phase
    context->channel_buffers[i] = (uint8_t *)bmalloc(context->audio_frame_size); // Buffer per channel (for split mode)
  }

  const char *p = obs_data_get_string(settings, "file_path");
  obs_log(LOG_INFO, ">>>v210_create obs_data_get_string '%s'", p);

  char path[512];
#if defined(IN10BITS)
  sprintf(path, "%s1920x1080@29.97i_clp_0.yuv10", GL_CliArg_X.BaseDirectory_S.c_str());
#else
  sprintf(path, "%s640x480@59.94p_clp_0.yuv8", GL_CliArg_X.BaseDirectory_S.c_str());
#endif
  context->video_file = fopen(path, "rb");
  obs_log(LOG_INFO, ">>>v210_create video '%s' %p", path, context->video_file);

  sprintf(path, "%s16xS24L32@48000_6_0.pcm", GL_CliArg_X.BaseDirectory_S.c_str());
  context->audio_file = fopen(path, "rb");
  obs_log(LOG_INFO, ">>>v210_create audio '%s' %p", path, context->audio_file);

  context->q_alpha = 255.0f; // 128.0f; // 255.0f; // Default 0xFF

  // Compile the Shader
  obs_enter_graphics();
  char *errors = NULL;
  context->effect = gs_effect_create(GL_v210_effect_code, "v210_effect_embedded", &errors);

  if (errors)
  {
    obs_log(LOG_ERROR, ">>>v210_create Shader compile error: %s", errors);
    bfree(errors);
  }

  if (context->effect)
  {
    context->param_image = gs_effect_get_param_by_name(context->effect, "image");
    context->param_width = gs_effect_get_param_by_name(context->effect, "width_pixels");
    context->param_alpha = gs_effect_get_param_by_name(context->effect, "alpha_val");
  }
  obs_leave_graphics();
  obs_log(LOG_INFO, ">>>v210_create ->%p", context);
  return context;
}

static void v210_destroy(void *data)
{
  v210_source *context = (v210_source *)data;
  obs_log(LOG_INFO, ">>>v210_destroy %p", data);
  if (context->video_file)
  {
    fclose(context->video_file);
  }
  if (context->audio_file)
  {
    fclose(context->audio_file);
  }
  bfree(context->buffer);
  bfree(context->audio_buffer);

  // Free channel buffers (always allocated now)
  for (int i = 0; i < 16; i++)
  {
    if (context->channel_buffers[i])
    {
      bfree(context->channel_buffers[i]);
    }
  }

  obs_enter_graphics();
  if (context->texture)
  {
    gs_texture_destroy(context->texture);
  }
  if (context->effect)
  {
    gs_effect_destroy(context->effect);
  }
  obs_leave_graphics();

  bfree(context);
}

static uint32_t v210_get_width(void *data)
{
  v210_source *context = (v210_source *)data;
  // obs_log(LOG_INFO, ">>>v210_get_width %p->%d", data, context->width);
  return context->width;
}

static uint32_t v210_get_height(void *data)
{
  v210_source *context = (v210_source *)data;
  // obs_log(LOG_INFO, ">>>v210_get_height %p->%d", data, context->height);
  return context->height;
}

static obs_properties_t *v210_get_properties(void *data)
{
  // data is your v210_source* context
  obs_properties_t *props = obs_properties_create();
  obs_log(LOG_INFO, ">>>v210_get_properties %p->%p", data, props);

  // Audio Source Type
  obs_properties_add_bool(props, "use_sine_generator", "Use Sine Wave Generator (unchecked = use files)");

  // Audio Mode Selection
  obs_properties_add_bool(props, "split_audio_mode", "Split Audio Mode (checked = 16 separate sources, unchecked = single 16-channel source)");

  // Add a file picker for the Video File
  obs_properties_add_path(props, "video_file_path", "Video YUV File", OBS_PATH_FILE, "Raw Files (*.yuv8 *.yuv10)", nullptr);

  // Add a file picker for the Audio File
  obs_properties_add_path(props, "audio_file_path", "Audio PCM File", OBS_PATH_FILE, "PCM Files (*.pcm)", nullptr);

  // Multi-channel sine wave controls
  for (int i = 0; i < 16; i++)
  {
    char prop_name[64];
    char prop_desc[64];

    sprintf(prop_name, "sine_enabled_%d", i + 1);
    sprintf(prop_desc, "Channel %d Enabled", i + 1);
    obs_properties_add_bool(props, prop_name, prop_desc);

    sprintf(prop_name, "sine_frequency_%d", i + 1);
    sprintf(prop_desc, "Channel %d Frequency (Hz)", i + 1);
    obs_properties_add_float_slider(props, prop_name, prop_desc, 20.0, 20000.0, 10.0);

    sprintf(prop_name, "sine_amplitude_%d", i + 1);
    sprintf(prop_desc, "Channel %d Amplitude", i + 1);
    obs_properties_add_float_slider(props, prop_name, prop_desc, 0.0, 1.0, 0.01);
  }

  return props;
}
static void v210_get_defaults(obs_data_t *settings)
{
  obs_log(LOG_INFO, ">>>v210_get_defaults %p", settings);

  obs_data_set_default_bool(settings, "use_sine_generator", true);
  obs_data_set_default_bool(settings, "split_audio_mode", false); // Default to multi-channel mode

  // Set defaults for multi-channel sine waves
  for (int i = 0; i < 16; i++)
  {
    char prop_name[64];

    sprintf(prop_name, "sine_enabled_%d", i + 1);
    obs_data_set_default_bool(settings, prop_name, (i < 8)); // Enable first 8 channels

    sprintf(prop_name, "sine_frequency_%d", i + 1);
    obs_data_set_default_double(settings, prop_name, 440.0 + (i * 110.0)); // Different frequencies

    sprintf(prop_name, "sine_amplitude_%d", i + 1);
    obs_data_set_default_double(settings, prop_name, 0.3);
  }
}
static void v210_update(void *data, obs_data_t *settings)
{
  auto *context = (v210_source *)data;
  obs_log(LOG_INFO, ">>>v210_update %p %p", data, settings);

  // Update sine generator toggle
  context->use_sine_generator = obs_data_get_bool(settings, "use_sine_generator");
  context->split_audio_mode = obs_data_get_bool(settings, "split_audio_mode");

  // Update multi-channel sine wave settings
  for (int i = 0; i < 16; i++)
  {
    char prop_name[64];

    sprintf(prop_name, "sine_enabled_%d", i + 1);
    context->sine_enabled[i] = obs_data_get_bool(settings, prop_name);

    sprintf(prop_name, "sine_frequency_%d", i + 1);
    context->sine_frequencies[i] = obs_data_get_double(settings, prop_name);

    sprintf(prop_name, "sine_amplitude_%d", i + 1);
    context->sine_amplitudes[i] = obs_data_get_double(settings, prop_name);
  }

  // 1. Handle Video File Update
  const char *video_path = obs_data_get_string(settings, "video_file_path");
  if (video_path && *video_path)
  {
    if (context->video_file)
    {
      fclose(context->video_file);
    }
    context->video_file = fopen(video_path, "rb");
    obs_log(LOG_INFO, "Video source updated to: %s", video_path);
  }

  // 2. Handle Audio File Update
  const char *audio_path = obs_data_get_string(settings, "audio_file_path");
  if (audio_path && *audio_path)
  {
    if (context->audio_file)
    {
      fclose(context->audio_file);
    }
    context->audio_file = fopen(audio_path, "rb");
    obs_log(LOG_INFO, "Audio source updated to: %s", audio_path);
  }
}
static void v210_video_tick(void *data, float seconds)
{
  auto *context = (v210_source *)data;
  auto Now = std::chrono::high_resolution_clock::now();
  context->DeltaTickInUs = std::chrono::duration_cast<std::chrono::microseconds>(Now - context->TickIn).count();
  context->TickIn = std::chrono::high_resolution_clock::now();
  if (!context->source)
  {
    return;
  }

  // 1. Add current frame time to our accumulator
  context->audio_remainder_seconds += (double)seconds;

  // 2. Calculate exactly how many whole samples fit into our accumulated time
  uint32_t samples_to_generate = (uint32_t)(context->audio_sample_rate * context->audio_remainder_seconds);

  // 3. Subtract the time we are actually using from the accumulator
  // This preserves the fractional "leftover" time for the next tick
  context->audio_remainder_seconds -= (double)samples_to_generate / context->audio_sample_rate;
  int32_t *pSample_S32 = (int32_t *)context->audio_buffer;

  if (context->use_sine_generator && samples_to_generate > 0)
  {
    if (context->split_audio_mode)
    {
      // Option 2: Individual channels are handled by separate sources
      // This main source only generates a simple test tone
      GenerateAudioSinusData(440.0, 0.3 * 0x7FFFFFFF, (double)context->audio_sample_rate, samples_to_generate, pSample_S32, context->audio_phase);

      // Output single test audio
      struct obs_source_audio audio_frame = {0};
      audio_frame.data[0] = context->audio_buffer;
      audio_frame.frames = samples_to_generate;
      audio_frame.speakers = SPEAKERS_MONO;
      audio_frame.samples_per_sec = context->audio_sample_rate;
      audio_frame.format = AUDIO_FORMAT_32BIT;
      audio_frame.timestamp = obs_get_video_frame_time() - util_mul_div64(samples_to_generate, 1000000000ULL, context->audio_sample_rate);
      obs_source_output_audio(context->source, &audio_frame);
    }
    else
    {
      // Option 3: Generate proper 16-channel interleaved audio
      // Clear buffer first
      memset(pSample_S32, 0, samples_to_generate * 16 * sizeof(int32_t));

      // Generate each enabled channel into interleaved buffer
      for (int channel = 0; channel < 16; channel++)
      {
        if (context->sine_enabled[channel])
        {
          double amplitude = context->sine_amplitudes[channel] * 0x7FFFFFFF;

          // Generate samples for this channel
          for (uint32_t sample = 0; sample < samples_to_generate; sample++)
          {
            int32_t sample_value = (int32_t)(amplitude * sin(context->sine_phases[channel]));
            // Interleaved: sample_index = sample * 16 + channel
            pSample_S32[sample * 16 + channel] = sample_value;

            // Update phase
            context->sine_phases[channel] += (2.0 * M_PI * context->sine_frequencies[channel]) / context->audio_sample_rate;
            if (context->sine_phases[channel] > 2.0 * M_PI)
            {
              context->sine_phases[channel] -= 2.0 * M_PI;
            }
          }
        }
      }

      // Output 16-channel interleaved audio
      struct obs_source_audio audio_frame = {0};
      audio_frame.data[0] = context->audio_buffer;
      audio_frame.frames = samples_to_generate;
      audio_frame.speakers = SPEAKERS_UNKNOWN; // 16-channel layout
      audio_frame.samples_per_sec = context->audio_sample_rate;
      audio_frame.format = AUDIO_FORMAT_32BIT;
      audio_frame.timestamp = obs_get_video_frame_time() - util_mul_div64(samples_to_generate, 1000000000ULL, context->audio_sample_rate);
      obs_source_output_audio(context->source, &audio_frame);
    }
  }
  else if (!context->use_sine_generator)
  {
    // --- FILE READING ---
    if (context->audio_file)
    {
      // 1. Read the mono samples
      size_t audio_read = fread(pSample_S32, sizeof(int32_t), samples_to_generate, context->audio_file);

      // Handle Looping
      if (audio_read < samples_to_generate)
      {
        fseek(context->audio_file, 0, SEEK_SET);
        fread(pSample_S32 + audio_read, sizeof(int32_t), samples_to_generate - audio_read, context->audio_file);
      }

      // 2. Scale 24-bit sign-extended to 32-bit Full Scale
      // Use a separate index to avoid modifying the base pointer 'pSample_S32'
      for (uint32_t i = 0; i < samples_to_generate; i++)
      {
        // Shift left by 8 to move the 24-bit data to the top of the 32-bit container
        //  pSample_S32[i] = pSample_S32[i] << 8;
      }

      // Output file-based audio
      struct obs_source_audio audio_frame = {0};
      audio_frame.data[0] = context->audio_buffer;
      audio_frame.frames = samples_to_generate;
      audio_frame.speakers = SPEAKERS_MONO;
      audio_frame.samples_per_sec = context->audio_sample_rate;
      audio_frame.format = AUDIO_FORMAT_32BIT;
      audio_frame.timestamp = obs_get_video_frame_time() - util_mul_div64(samples_to_generate, 1000000000ULL, context->audio_sample_rate);
      obs_source_output_audio(context->source, &audio_frame);
    }
    else
    {
      // Fallback: If no file, output silence so the mixer doesn't "freeze"
      memset(pSample_S32, 0, samples_to_generate * sizeof(int32_t));
    }
  }
  context->TickOut = std::chrono::high_resolution_clock::now();
  context->TickElapsedTimeInUs = std::chrono::duration_cast<std::chrono::microseconds>(context->TickOut - context->TickIn).count();
  obs_log(LOG_INFO, ">>>v210_video_tick Delta %lld Elapsed %lld uS", context->DeltaTickInUs, context->TickElapsedTimeInUs);
}
// This is where we draw
static void v210_video_render(void *data, gs_effect_t *effect)
{
  v210_source *context = (v210_source *)data;
  auto Now = std::chrono::high_resolution_clock::now();
  context->DeltaRendererInUs = std::chrono::duration_cast<std::chrono::microseconds>(Now - context->RendererIn).count();
  context->RendererIn = std::chrono::high_resolution_clock::now();
  if (!context->buffer || !context->effect)
  {
    return;
  }

  // 1. Calculate v210 Stride
  // Standard v210 stride is usually 128-byte aligned (48 pixels)
  // Formula: (Width + 47) / 48 * 128
  uint32_t stride_bytes = ((context->width + 47) / 48) * 128;

  // Texture width in "integers" (pixels for the GPU texture)
  // Since 1 integer = 4 bytes, we divide stride by 4.
  uint32_t tex_width = stride_bytes / 4;

  // 2. Manage Texture
  if (!context->texture || gs_texture_get_width(context->texture) != tex_width || gs_texture_get_height(context->texture) != context->height)
  {
    if (context->texture)
    {
      gs_texture_destroy(context->texture);
    }

    // Use GS_RGBA to store 32-bit words
    context->texture = gs_texture_create(tex_width, context->height, GS_RGBA, 1, NULL, GS_DYNAMIC);
  }

  // 3. Upload Data
  if (context->texture)
  {
    if (!context->video_file)
    {
      return;
    }
#if defined(VIDEO_BAR)
#if defined(IN10BITS)
#if defined(MOVING_BAR)
    fill_uyvy_color_bars(context->buffer, context->width, context->height, true);
    draw_line_avx2(context->buffer, context->width, context->height, 255, 0, 0, context->line_pos, 2, true, true);
#endif
#else
#if defined(MOVING_BAR)
    fill_uyvy_color_bars(context->buffer, context->width, context->height, false);
    draw_line_avx2(context->buffer, context->width, context->height, 255, 0, 0, context->line_pos, 2, true, false);
#endif
#endif
    context->line_pos += 2;
    if (context->line_pos > context->height)
    {
      context->line_pos = 0;
    }
#else
    // Read one frame
    size_t read = fread(context->buffer, 1, context->frame_size, context->video_file);
    // Loop file if EOF
    if (read < context->frame_size)
    {
      fseek(context->video_file, 0, SEEK_SET);
      // obs_log(LOG_INFO, ">>>raw_video_tick %p sz %d not enought", context->video_file, read);
      return;
    }
#endif
    gs_texture_set_image(context->texture, context->buffer, stride_bytes, false);
  }

  // 4. Draw with Custom Shader
  // We override the default 'effect' passed in by OBS with our custom one
  gs_effect_t *cur_effect = context->effect;

  gs_effect_set_texture(context->param_image, context->texture);
  gs_effect_set_float(context->param_width, (float)context->width);
  gs_effect_set_float(context->param_alpha, context->q_alpha); // 255.0

  while (gs_effect_loop(cur_effect, "Draw"))
  {
    // Draw sprite to fill the canvas output, not the texture size
    gs_draw_sprite(context->texture, 0, context->width, context->height);
  }
  context->RendererOut = std::chrono::high_resolution_clock::now();
  context->RendererElapsedTimeInUs = std::chrono::duration_cast<std::chrono::microseconds>(context->RendererOut - context->TickIn).count();
  obs_log(LOG_INFO, ">>>v210_video_render Delta %lld Elapsed %lld uS", context->DeltaRendererInUs, context->RendererElapsedTimeInUs);
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
struct obs_source_info v210_source_info = {
    .id = PLUGIN_INPUT_NAME,
    .type = OBS_SOURCE_TYPE_INPUT,
    .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_AUDIO,
    .get_name = v210_get_name,
    .create = v210_create,
    .destroy = v210_destroy,
    .get_width = v210_get_width,
    .get_height = v210_get_height,
    .get_defaults = v210_get_defaults,
    .get_properties = v210_get_properties,
    .update = v210_update,
    .video_tick = v210_video_tick,
    .video_render = v210_video_render,
};

// Individual Sine Channel Source Functions (Option 2)
static const char *sine_channel_get_name(void *type_data)
{
  // Return a generic name, specific channel number will be set during registration
  return "Sine Channel";
}

static void *sine_channel_create(obs_data_t *settings, obs_source_t *source)
{
  sine_channel_source *context = (sine_channel_source *)bzalloc(sizeof(sine_channel_source));
  context->source = source;

  // Audio setup: 48 kHz, mono, 32-bit signed int
  context->audio_sample_rate = 48000;
  context->audio_frame_size = sizeof(int32_t) * 16 * 1024;
  context->audio_buffer = (uint8_t *)bmalloc(context->audio_frame_size);
  context->audio_phase = 0.0;
  context->audio_remainder_seconds = 0.0;

  // Default sine wave parameters - will be overridden by settings
  context->frequency = 440.0;
  context->amplitude = 0.3;
  context->enabled = true;
  context->channel_number = 1;

  return context;
}

static void sine_channel_destroy(void *data)
{
  sine_channel_source *context = (sine_channel_source *)data;
  if (context->audio_buffer)
  {
    bfree(context->audio_buffer);
  }
  bfree(context);
}

static obs_properties_t *sine_channel_get_properties(void *data)
{
  obs_properties_t *props = obs_properties_create();

  obs_properties_add_bool(props, "enabled", "Enabled");
  obs_properties_add_float_slider(props, "frequency", "Frequency (Hz)", 20.0, 20000.0, 10.0);
  obs_properties_add_float_slider(props, "amplitude", "Amplitude", 0.0, 1.0, 0.01);

  return props;
}

static void sine_channel_get_defaults(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", true);
  obs_data_set_default_double(settings, "frequency", 440.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}

static void sine_channel_update(void *data, obs_data_t *settings)
{
  sine_channel_source *context = (sine_channel_source *)data;

  context->enabled = obs_data_get_bool(settings, "enabled");
  context->frequency = obs_data_get_double(settings, "frequency");
  context->amplitude = obs_data_get_double(settings, "amplitude");
}

static void sine_channel_tick(void *data, float seconds)
{
  sine_channel_source *context = (sine_channel_source *)data;

  if (!context->source || !context->enabled)
  {
    return;
  }

  // Add current frame time to our accumulator
  context->audio_remainder_seconds += (double)seconds;

  // Calculate exactly how many whole samples fit into our accumulated time
  uint32_t samples_to_generate = (uint32_t)(context->audio_sample_rate * context->audio_remainder_seconds);

  // Subtract the time we are actually using from the accumulator
  context->audio_remainder_seconds -= (double)samples_to_generate / context->audio_sample_rate;

  if (samples_to_generate > 0)
  {
    int32_t *pSample_S32 = (int32_t *)context->audio_buffer;
    double amplitude = context->amplitude * 0x7FFFFFFF;

    // Generate sine wave using existing function
    GenerateAudioSinusData(context->frequency, amplitude, (double)context->audio_sample_rate, samples_to_generate, pSample_S32, context->audio_phase);

    // Output the audio frame
    struct obs_source_audio audio_frame = {0};
    audio_frame.data[0] = context->audio_buffer;
    audio_frame.frames = samples_to_generate;
    audio_frame.speakers = SPEAKERS_MONO;
    audio_frame.samples_per_sec = context->audio_sample_rate;
    audio_frame.format = AUDIO_FORMAT_32BIT;
    audio_frame.timestamp = obs_get_video_frame_time() - util_mul_div64(samples_to_generate, 1000000000ULL, context->audio_sample_rate);

    obs_source_output_audio(context->source, &audio_frame);
  }
}

// Create 16 separate source info structures
struct obs_source_info sine_channel_source_infos[16];

// Static strings for source IDs and names
static char sine_source_ids[16][64];
static char sine_source_names[16][64];

// Static name functions for each channel
static const char *sine_channel_get_name_0(void *)
{
  return sine_source_names[0];
}
static const char *sine_channel_get_name_1(void *)
{
  return sine_source_names[1];
}
static const char *sine_channel_get_name_2(void *)
{
  return sine_source_names[2];
}
static const char *sine_channel_get_name_3(void *)
{
  return sine_source_names[3];
}
static const char *sine_channel_get_name_4(void *)
{
  return sine_source_names[4];
}
static const char *sine_channel_get_name_5(void *)
{
  return sine_source_names[5];
}
static const char *sine_channel_get_name_6(void *)
{
  return sine_source_names[6];
}
static const char *sine_channel_get_name_7(void *)
{
  return sine_source_names[7];
}
static const char *sine_channel_get_name_8(void *)
{
  return sine_source_names[8];
}
static const char *sine_channel_get_name_9(void *)
{
  return sine_source_names[9];
}
static const char *sine_channel_get_name_10(void *)
{
  return sine_source_names[10];
}
static const char *sine_channel_get_name_11(void *)
{
  return sine_source_names[11];
}
static const char *sine_channel_get_name_12(void *)
{
  return sine_source_names[12];
}
static const char *sine_channel_get_name_13(void *)
{
  return sine_source_names[13];
}
static const char *sine_channel_get_name_14(void *)
{
  return sine_source_names[14];
}
static const char *sine_channel_get_name_15(void *)
{
  return sine_source_names[15];
}

// Static defaults functions for each channel
static void sine_channel_get_defaults_0(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", true);
  obs_data_set_default_double(settings, "frequency", 440.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_1(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", true);
  obs_data_set_default_double(settings, "frequency", 550.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_2(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", false);
  obs_data_set_default_double(settings, "frequency", 660.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_3(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", false);
  obs_data_set_default_double(settings, "frequency", 770.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_4(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", false);
  obs_data_set_default_double(settings, "frequency", 880.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_5(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", false);
  obs_data_set_default_double(settings, "frequency", 990.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_6(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", false);
  obs_data_set_default_double(settings, "frequency", 1100.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_7(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", false);
  obs_data_set_default_double(settings, "frequency", 1210.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_8(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", false);
  obs_data_set_default_double(settings, "frequency", 1320.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_9(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", false);
  obs_data_set_default_double(settings, "frequency", 1430.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_10(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", false);
  obs_data_set_default_double(settings, "frequency", 1540.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_11(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", false);
  obs_data_set_default_double(settings, "frequency", 1650.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_12(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", false);
  obs_data_set_default_double(settings, "frequency", 1760.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_13(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", false);
  obs_data_set_default_double(settings, "frequency", 1870.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_14(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", false);
  obs_data_set_default_double(settings, "frequency", 1980.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}
static void sine_channel_get_defaults_15(obs_data_t *settings)
{
  obs_data_set_default_bool(settings, "enabled", false);
  obs_data_set_default_double(settings, "frequency", 2090.0);
  obs_data_set_default_double(settings, "amplitude", 0.3);
}

// Array of function pointers
static const char *(*sine_get_name_functions[])(void *) = {
    sine_channel_get_name_0,  sine_channel_get_name_1,  sine_channel_get_name_2,  sine_channel_get_name_3, sine_channel_get_name_4,  sine_channel_get_name_5,
    sine_channel_get_name_6,  sine_channel_get_name_7,  sine_channel_get_name_8,  sine_channel_get_name_9, sine_channel_get_name_10, sine_channel_get_name_11,
    sine_channel_get_name_12, sine_channel_get_name_13, sine_channel_get_name_14, sine_channel_get_name_15};

static void (*sine_get_defaults_functions[])(obs_data_t *) = {
    sine_channel_get_defaults_0,  sine_channel_get_defaults_1,  sine_channel_get_defaults_2,  sine_channel_get_defaults_3,
    sine_channel_get_defaults_4,  sine_channel_get_defaults_5,  sine_channel_get_defaults_6,  sine_channel_get_defaults_7,
    sine_channel_get_defaults_8,  sine_channel_get_defaults_9,  sine_channel_get_defaults_10, sine_channel_get_defaults_11,
    sine_channel_get_defaults_12, sine_channel_get_defaults_13, sine_channel_get_defaults_14, sine_channel_get_defaults_15};

// Function to initialize all 16 sine channel sources
void initialize_sine_channel_sources()
{
  for (int i = 0; i < 16; i++)
  {
    sprintf(sine_source_ids[i], "sine_channel_%d", i + 1);
    sprintf(sine_source_names[i], "Sine Channel %d", i + 1);

    sine_channel_source_infos[i] = {
        .id = sine_source_ids[i],
        .type = OBS_SOURCE_TYPE_INPUT,
        .output_flags = OBS_SOURCE_AUDIO,
        .get_name = sine_get_name_functions[i],
        .create = sine_channel_create,
        .destroy = sine_channel_destroy,
        .get_defaults = sine_get_defaults_functions[i],
        .get_properties = sine_channel_get_properties,
        .update = sine_channel_update,
        .video_tick = sine_channel_tick,
    };
  }
}

bool obs_module_load(void)
{
  bool Rts_B = false;
  WSADATA wsa;

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
  obs_register_source(&v210_source_info);

  // Always register 16 separate sine channel sources for Option 2
  obs_log(LOG_INFO, "Registering 16 individual sine channel sources");
  initialize_sine_channel_sources();
  for (int i = 0; i < 16; i++)
  {
    obs_register_source(&sine_channel_source_infos[i]);
    obs_log(LOG_INFO, "Registered sine channel %d", i + 1);
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

#if 0

#define IN10BITS
#define CUSTOM_DRAW
//#define AUDIO_SIN
extern struct obs_source_info raw_10bit_info;
extern struct obs_source_info udp_stream_filter_info;
extern struct obs_output_info raw_dump_info;
void on_menu_click(void *private_data);
// Variables to store our texture references
gs_stagesurf_t *staged_surf = nullptr;
//home dir is C:\pro\obs-studio\bin\64bit



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

  // Audio members
  FILE *audio_file;
  uint32_t audio_sample_rate; // 48000 Hz
  uint32_t audio_channels;    // typically 2 for stereo
  size_t audio_frame_size;    // bytes per audio frame
  uint8_t *audio_buffer;
  double audio_phase;
  double audio_remainder_seconds;
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
  fill_uyvy_color_bars(context->buffer, context->width, context->height, true);
#else
  context->frame_size = context->width * context->height * 2;
  fill_uyvy_color_bars(context->buffer, context->width, context->height, false);
#endif
  context->buffer = (uint8_t *)bmalloc(context->frame_size);
  // Create texture
  obs_enter_graphics();
  context->texture = gs_texture_create(context->width, context->height, GS_BGRA, 1, NULL, GS_DYNAMIC);
  obs_leave_graphics();

  // Audio setup: 48 kHz, stereo (2 channels), 32-bit signed int
  context->audio_sample_rate = 48000;
  context->audio_channels = 1;
  context->audio_frame_size = context->audio_channels * sizeof(int32_t) * 16 * 1024;
  context->audio_buffer = (uint8_t *)bmalloc(context->audio_frame_size);



  const char *p = obs_data_get_string(settings, "file_path");
  obs_log(LOG_INFO, ">>>raw_create obs_data_get_string '%s'", p);

  char path[512];
#if defined(IN10BITS)
  sprintf(path, "%s1920x1080@29.97i_clp_0.yuv10", GL_CliArg_X.BaseDirectory_S.c_str());
#else
  sprintf(path, "%s640x480@59.94p_clp_0.yuv8", GL_CliArg_X.BaseDirectory_S.c_str());
#endif  
  context->file = fopen(path, "rb");
  obs_log(LOG_INFO, ">>>raw_create video '%s' %p", path, context->file);

  sprintf(path, "%s16xS24L32@48000_6_0.pcm", GL_CliArg_X.BaseDirectory_S.c_str());
  context->audio_file = fopen(path, "rb");
  obs_log(LOG_INFO, ">>>raw_create audio '%s' %p", path, context->audio_file);

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
  if (context->audio_file)
  {
    fclose(context->audio_file);
  }
  bfree(context->buffer);
  bfree(context->audio_buffer);
  bfree(context);
}

// 4. Main Video Loop (Tick)



static void raw_video_tick(void *data, float seconds)
{
  auto *context = (raw_10bit_source *)data;
  if (!context->source)
  {
    return;
  }

// 1. Add current frame time to our accumulator
  context->audio_remainder_seconds += (double)seconds;

  // 2. Calculate exactly how many whole samples fit into our accumulated time
  uint32_t samples_to_generate = (uint32_t)(context->audio_sample_rate * context->audio_remainder_seconds);

  // 3. Subtract the time we are actually using from the accumulator
  // This preserves the fractional "leftover" time for the next tick
  context->audio_remainder_seconds -= (double)samples_to_generate / context->audio_sample_rate;
  int32_t *pSample_S32 = (int32_t *)context->audio_buffer;

#if defined(AUDIO_SIN)
  if (samples_to_generate > 0)
  {
    const double frequency = 440.0;
    const double amplitude = 0.5 * 2147483647.0;
    const double sample_rate = (double)context->audio_sample_rate;
    const double twopi = 2.0 * M_PI;

    // 2. Generate exactly 'samples_to_generate'
    for (uint32_t i = 0; i < samples_to_generate; i++)
    {
      pSample_S32[i] = (int32_t)(amplitude * sin(context->audio_phase));

      context->audio_phase += (twopi * frequency) / sample_rate;
      if (context->audio_phase > twopi)
      {
        context->audio_phase -= twopi;
      }
    }
  }
#else
  // --- FILE READING ---
  if (context->audio_file)
  {
    // 1. Read the mono samples
    size_t audio_read = fread(pSample_S32, sizeof(int32_t), samples_to_generate, context->audio_file);

    // Handle Looping
    if (audio_read < samples_to_generate)
    {
      fseek(context->audio_file, 0, SEEK_SET);
      fread(pSample_S32 + audio_read, sizeof(int32_t), samples_to_generate - audio_read, context->audio_file);
    }

    // 2. Scale 24-bit sign-extended to 32-bit Full Scale
    // Use a separate index to avoid modifying the base pointer 'pSample_S32'
    for (uint32_t i = 0; i < samples_to_generate; i++)
    {
      // Shift left by 8 to move the 24-bit data to the top of the 32-bit container
    //  pSample_S32[i] = pSample_S32[i] << 8;
    }
  }
  else
  {
    // Fallback: If no file, output silence so the mixer doesn't "freeze"
    memset(pSample_S32, 0, samples_to_generate * sizeof(int32_t));
  }
#endif
  // 3. Output the frame
  struct obs_source_audio audio_frame = {0};
  audio_frame.data[0] = context->audio_buffer;
  audio_frame.frames = samples_to_generate; // MUST match the loop count
  audio_frame.speakers = SPEAKERS_MONO;
  audio_frame.samples_per_sec = context->audio_sample_rate;
  audio_frame.format = AUDIO_FORMAT_32BIT;
  //audio_frame.timestamp = obs_get_video_frame_time();
  audio_frame.timestamp = obs_get_video_frame_time() - util_mul_div64(samples_to_generate, 1000000000ULL, context->audio_sample_rate);
  obs_source_output_audio(context->source, &audio_frame);

  context->frame_count++;
}

static void prev_raw_video_tick(void *data, float seconds)
{
  auto *context = (raw_10bit_source *)data;
  if (!context->file)
  {
    return;
  }

#if defined(CUSTOM_DRAW)
  auto t1 = std::chrono::high_resolution_clock::now();
  auto t2 = std::chrono::high_resolution_clock::now();
#else
  // Read one frame
  size_t read = fread(context->buffer, 1, context->frame_size, context->file);
  // Loop file if EOF
  if (read < context->frame_size)
  {
    fseek(context->file, 0, SEEK_SET);
    // obs_log(LOG_INFO, ">>>raw_video_tick %p sz %d not enought", context->file, read);
    return;
  }
#if defined(IN10BITS)
  auto t1 = std::chrono::high_resolution_clock::now();
  // Convert v210 to UYVY
  //v210_to_uyvy_avx2_vcl((const uint32_t *)context->buffer, context->buffer, context->width, context->height);
 v210_to_uyvy_avx2_opt((const uint32_t *)context->buffer, context->buffer, context->width, context->height);
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
#endif

    // Read and output audio
  // Calculate audio samples needed for this video frame
  //uint32_t audio_samples = context->audio_sample_rate / context->fps;
  //size_t audio_bytes_needed = audio_samples * sizeof(int32_t);

// 1. Calculate how many samples to read for this "tick"
  // seconds is the time since the last tick (e.g., 0.016 for 60fps)
  uint32_t samples_to_read = (uint32_t)(context->audio_sample_rate * seconds);
  size_t audio_bytes_needed = samples_to_read * sizeof(int32_t) * context->audio_channels;

  
  if (audio_bytes_needed >= context->audio_frame_size)
  {
    obs_log(LOG_INFO, ">>>raw_video_tick audio %p sz %d not enought", context->audio_file, audio_bytes_needed);
    audio_bytes_needed = context->audio_frame_size;
  }

  size_t audio_read = fread(context->audio_buffer, 1, audio_bytes_needed, context->audio_file);

  if (audio_read == audio_bytes_needed)
  {
    uint32_t i_U32;
    int32_t *pSample_S32 = ((int32_t *)context->audio_buffer);


    // --- SINE WAVE GENERATION ---
    uint32_t audio_samples = context->audio_sample_rate / context->fps;
    const double frequency = 440.0;
    const double amplitude = 0.5 * 2147483647.0; // 50% volume to avoid clipping
    const double sample_rate = (double)context->audio_sample_rate;

    for (i_U32 = 0; i_U32 < audio_samples; i_U32++)
    {
      // Calculate the sine value
      pSample_S32[i_U32] = (int32_t)(amplitude * sin(context->audio_phase));

      // Advance the phase
      context->audio_phase += (2.0 * M_PI * frequency) / sample_rate;

      // Keep phase within 0 to 2*PI to prevent precision loss over time
      if (context->audio_phase > 2.0 * M_PI)
      {
        context->audio_phase -= 2.0 * M_PI;
      }
    }





    struct obs_source_audio audio_frame = {0};
    audio_frame.data[0] = context->audio_buffer;
    audio_frame.frames = (uint32_t)(audio_read / (sizeof(int32_t) * context->audio_channels));
    audio_frame.speakers = SPEAKERS_MONO; // SPEAKERS_STEREO;
    audio_frame.samples_per_sec = context->audio_sample_rate;
    // Use the current system video time to keep audio "live"
    audio_frame.timestamp = obs_get_video_frame_time();
#if 1
    audio_frame.format = AUDIO_FORMAT_32BIT;
    /*
    int32_t Sample_S32;
    for (i_U32 = 0; i_U32 < samples_to_read; i_U32++, pSample_S32++)
    {
      Sample_S32 = *pSample_S32;
      Sample_S32 = Sample_S32 << 8;
      *pSample_S32 = Sample_S32;
    }
    */
#else

    // Change format to Float
    audio_frame.format = AUDIO_FORMAT_FLOAT;

    // Convert your S32 samples to Float
    float *pSample_F32 = (float *)context->audio_buffer;
    int32_t *pInput_S32 = (int32_t *)context->audio_buffer;

    for (i_U32 = 0; i_U32 < samples_to_read; i_U32++)
    {
      // Convert int32 range to -1.0 ... 1.0
      pSample_F32[i_U32] = (float)pInput_S32[i_U32] / 2147483648.0f;
    }
#endif

    obs_source_output_audio(context->source, &audio_frame);
  }
  else
  {
    // Audio EOF - loop or stop
    fseek(context->audio_file, 0, SEEK_SET);
  }

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
/*
static obs_properties_t *raw_get_properties(void *data)
{
  obs_log(LOG_INFO, ">>>raw_get_properties");
  obs_properties_t *props = obs_properties_create();

  // The first string "file_path" is the KEY used by obs_data_get_string
  obs_properties_add_path(props, "file_path", "Select Raw YUV File", OBS_PATH_FILE, "Raw Files (*.yuv8 *.yuv8)", nullptr);

  return props;
}
*/
static void raw_video_render(void *data, gs_effect_t *effect)
{
  auto *context = (raw_10bit_source *)data;
  //UNUSED_PARAMETER(context);
  obs_log(LOG_INFO, ">>>raw_video_render");
#if 0
  effect = obs_get_base_effect(OBS_EFFECT_SOLID);

  gs_effect_set_color(gs_effect_get_param_by_name(effect, "color"), 0xFFFF0000);

  while (gs_effect_loop(effect, "Solid"))
  { 
    gs_draw_sprite(NULL, 0, context->width, context->height);
  }
#endif

#if defined(IN10BITS)
  auto t1 = std::chrono::high_resolution_clock::now();
  // Convert v210 to UYVY
  // v210_to_uyvy_avx2_vcl((const uint32_t *)context->buffer, context->buffer, context->width, context->height);
  v210_to_uyvy_avx2_opt((const uint32_t *)context->buffer, context->buffer, context->width, context->height);
  auto t2 = std::chrono::high_resolution_clock::now();

#endif

  // Get the default "Draw" effect (simple texture shader)
  gs_effect_t *draw_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);

  // Set the texture as the 'image' parameter for the shader
  //gs_effect_set_texture(???, context->texture);

  // Draw the texture onto the current render target (the OBS canvas)
  while (gs_effect_loop(draw_effect, "Draw"))
  {
    obs_source_draw(context->texture, 0, 0, 0, 0, 0);
  }
}
static obs_properties_t *raw_get_properties(void *data)
{
  // data is your raw_10bit_source* context
  obs_properties_t *props = obs_properties_create();

  // Add a file picker for the Video File
  obs_properties_add_path(props, "video_file_path", "Video YUV File", OBS_PATH_FILE, "Raw Files (*.yuv8 *.yuv10)", nullptr);

  // Add a file picker for the Audio File
  obs_properties_add_path(props, "audio_file_path", "Audio PCM File", OBS_PATH_FILE, "PCM Files (*.pcm)", nullptr);

  return props;
}
static void raw_update(void *data, obs_data_t *settings)
{
  auto *context = (raw_10bit_source *)data;

  // 1. Handle Video File Update
  const char *video_path = obs_data_get_string(settings, "video_file_path");
  if (video_path && *video_path)
  {
    if (context->file)
    {
      fclose(context->file);
    }
    context->file = fopen(video_path, "rb");
    obs_log(LOG_INFO, "Video source updated to: %s", video_path);
  }

  // 2. Handle Audio File Update
  const char *audio_path = obs_data_get_string(settings, "audio_file_path");
  if (audio_path && *audio_path)
  {
    if (context->audio_file)
    {
      fclose(context->audio_file);
    }
    context->audio_file = fopen(audio_path, "rb");
    obs_log(LOG_INFO, "Audio source updated to: %s", audio_path);
  }

  // Reset frame count to sync video/audio from the start of the new files
  context->frame_count = 0;
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
#if defined(CUSTOM_DRAW)
    .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW | OBS_SOURCE_AUDIO,
#else
    .output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC | OBS_SOURCE_AUDIO,
#endif
    .get_name = raw_get_name,
    .create = raw_create,
    .destroy = raw_destroy,
    .get_width = [](void *data) { return ((raw_10bit_source *)data)->width; },
    .get_height = [](void *data) { return ((raw_10bit_source *)data)->height; },
    .get_properties = raw_get_properties,
    .update = raw_update,
#if defined(CUSTOM_DRAW)
    .video_tick = raw_video_tick,     // OBS_SOURCE_ASYNC
    .video_render = raw_video_render, // OBS_SOURCE_CUSTOM_DRAW
#else
    .video_tick = raw_video_tick, // OBS_SOURCE_ASYNC
#endif
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
void capture_frame(void *data, uint32_t cx, uint32_t cy)
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

    uint32_t width = gs_stagesurface_get_width(staged_surf);
    uint32_t height = gs_stagesurface_get_height(staged_surf);

    // Process or write pixel data here
    // For example, write to file:
    // fwrite(data_ptr, 1, linesize * height, output_file);

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
  auto *context = new raw_dump_context();
  context->output = output;
  return context;
}

static void raw_dump_destroy(void *data)
{
  auto *context = static_cast<raw_dump_context *>(data);
  delete context;
}

static bool raw_dump_start(void *data)
{
  auto *context = static_cast<raw_dump_context *>(data);
  //Will be created in E:\pro\obs-studio\build_x64\rundir\Debug\bin\64bit
  context->video_file = fopen("video_stream.yuv", "ab"); // Append Binary
  context->audio_file = fopen("audio_stream.pcm", "ab");

  if (!context->video_file || !context->audio_file)
  {
    return false;
  }

  // Connect to the main mix
  if (!obs_output_begin_data_capture(context->output, 0))
  {
    return false;
  }

  context->active = true;
  return true;
}

static void raw_dump_stop(void *data, uint64_t ts)
{
  auto *context = static_cast<raw_dump_context *>(data);

  obs_output_end_data_capture(context->output);
  context->active = false;

  if (context->video_file)
  {
    fclose(context->video_file);
  }
  if (context->audio_file)
  {
    fclose(context->audio_file);
  }
}

// Receives Video: Final Scene (all filters/sources applied)
static void raw_dump_video(void *data, struct video_data *frame)
{
  auto *context = static_cast<raw_dump_context *>(data);

  // Video frames are planar. We write each plane sequentially.
  // For NV12: data[0] is Y, data[1] is UV interleaved.
  for (int i = 0; i < MAX_AV_PLANES; i++)
  {
    if (frame->data[i] && frame->linesize[i] > 0)
    {
      // Note: In a production plugin, you'd account for padding/stride.
      // For a basic dump, we write the full linesize x height.
      uint32_t height = obs_output_get_height(context->output);
      if (i > 0)
      {
        height /= 2; // Simple assumption for YUV420/NV12 chroma
      }

      fwrite(frame->data[i], 1, frame->linesize[i] * height, context->video_file);
    }
  }
}

// Receives Audio: Final Mix (48Khz, Planar Float)
static void raw_dump_audio(void *data, struct audio_data *frames)
{
  auto *context = static_cast<raw_dump_context *>(data);

  // Audio is planar. We write Channel 0 then Channel 1 etc.
  // OBS Audio is typically 32-bit Float.
  for (int i = 0; i < PLUGIN_AUDIO_CHANNELS; i++)
  {
    if (frames->data[i])
    {
      fwrite(frames->data[i], sizeof(float), frames->frames, context->audio_file);
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
