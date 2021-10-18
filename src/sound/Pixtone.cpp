/* Based on */
/* SIMPLE CAVE STORY MUSIC PLAYER (Organya) */
/* Written by Joel Yliluoma -- http://iki.fi/bisqwit/ */
/* https://bisqwit.iki.fi/jutut/kuvat/programming_examples/doukutsu-org/orgplay.cc */

#include "Pixtone.h"

#include "../ResourceManager.h"
#include "../common/misc.h"
#include "../Utils/Logger.h"
#include "../config.h"

#include "sslib.h"

// libretro-common
#include <audio/audio_resampler.h>
#include <audio/conversion/float_to_s16.h>
#include <audio/conversion/s16_to_float.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <sstream>
#include <vector>

// using std::fgetc;

namespace NXE
{
namespace Sound
{

static void mySfxCallback(int chan, int slot)
{
    Pixtone::getInstance()->pxtSoundDone(chan);
}

static struct
{
  int8_t table[256];
} wave[PXT_NO_MODELS];

double fgetv(FILE *fp) // Load a numeric value from text file; one per line.
{
  char Buf[4096], *p = Buf;
  Buf[4095] = '\0';
  if (!std::fgets(Buf, sizeof(Buf) - 1, fp))
    return 0.0;
  // Ignore empty lines. If the line was empty, try next line.
  if (!Buf[0] || Buf[0] == '\r' || Buf[0] == '\n')
    return fgetv(fp);
  while (*p && *p++ != ':')
  {
  }                         // Skip until a colon character.
  return std::strtod(p, 0); // Parse the value and return it.
}

bool stPXSound::load(const std::string &fname)
{
  FILE *fp;

  fp = myfopen(widen(fname).c_str(), widen("rb").c_str());
  if (!fp)
  {
    LOG_WARN("pxt->load: file '{}' not found.", fname);
    return false;
  }

  auto f  = [=]() { return (int32_t)fgetv(fp); };
  auto fu = [=]() { return (uint32_t)fgetv(fp); };

  for (auto &c : channels)
  {
    c = {
        f() != 0,
        fu(),                                       // enabled, length
        {wave[f() % 6].table, fgetv(fp), f(), f()}, // carrier wave
        {wave[f() % 6].table, fgetv(fp), f(), f()}, // frequency wave
        {wave[f() % 6].table, fgetv(fp), f(), f()}, // amplitude wave
        {f(), {{f(), f()}, {f(), f()}, {f(), f()}}}, // envelope
        nullptr
    };
  }
  fclose(fp);
  return true;
}

void stPXSound::freeBuf()
{
  uint32_t i;

  // free up the buffers
  for (i = 0; i < PXT_NO_CHANNELS; i++)
  {
    if (this->channels[i].buffer)
    {
      free(this->channels[i].buffer);
      this->channels[i].buffer = nullptr;
    }
  }

  if (this->final_buffer)
  {
    free(this->final_buffer);
    this->final_buffer = nullptr;
  }
}

int32_t stPXSound::allocBuf()
{
  uint32_t topbufsize = 64;
  uint32_t i;

  freeBuf();

  // allocate buffers for each enabled channel
  for (i = 0; i < PXT_NO_CHANNELS; i++)
  {
    if (this->channels[i].enabled)
    {
      this->channels[i].buffer = (signed char *)malloc(this->channels[i].nsamples);
      if (!this->channels[i].buffer)
      {
        LOG_ERROR("AllocBuffers (pxt): out of memory (channels)!");
        return -1;
      }

      if (this->channels[i].nsamples > topbufsize)
        topbufsize = this->channels[i].nsamples;
    }
  }

  // allocate the final buffer
  this->final_buffer = (signed char *)malloc(topbufsize);
  if (!this->final_buffer)
  {
    LOG_ERROR("AllocBuffers (pxt): out of memory (finalbuffer)!");
    return -1;
  }

  this->final_size = topbufsize;

  return topbufsize;
}

bool stPXSound::render()
{
  uint32_t i, s;
  int16_t mixed_sample;
  int16_t *middle_buffer;
  int32_t bufsize;

  bufsize = this->allocBuf();
  if (bufsize == -1)
    return 1; // error

  // --------------------------------
  //  render all the channels
  // --------------------------------
  for (i = 0; i < PXT_NO_CHANNELS; i++)
  {
    if (this->channels[i].enabled)
    {
      this->channels[i].synth();
    }
  }

  // ----------------------------------------------
  //  mix the channels [generate final_buffer]
  // ----------------------------------------------
  // lprintf("final_size = %d final_buffer = %08x\n", snd->final_size, snd->final_buffer);

  middle_buffer = (int16_t *)malloc(this->final_size * 2);

  memset(middle_buffer, 0, this->final_size * 2);

  for (i = 0; i < PXT_NO_CHANNELS; i++)
  {
    if (this->channels[i].enabled)
    {
      for (s = 0; s < this->channels[i].nsamples; s++)
      {
        middle_buffer[s] += this->channels[i].buffer[s];
      }
    }
  }

  for (s = 0; s < this->final_size; s++)
  {
    mixed_sample = middle_buffer[s];

    if (mixed_sample > 127)
      mixed_sample = 127;
    else if (mixed_sample < -127)
      mixed_sample = -127;

    this->final_buffer[s] = (char)mixed_sample;
  }

  free(middle_buffer);
  return 0;
}

int32_t stPXEnvelope::evaluate(int32_t i) const // Linearly interpolate between the key points:
{
  int32_t prevval = initial, prevtime = 0;
  int32_t nextval = 0, nexttime = 256;
  for (int32_t j = 2; j >= 0; --j)
    if (i < p[j].time)
    {
      nexttime = p[j].time;
      nextval  = p[j].val;
    }
  for (int32_t j = 0; j <= 2; ++j)
    if (i >= p[j].time)
    {
      prevtime = p[j].time;
      prevval  = p[j].val;
    }
  if (nexttime <= prevtime)
    return prevval;
  return (i - prevtime) * (nextval - prevval) / (nexttime - prevtime) + prevval;
}

void stPXChannel::synth()
{
  if (!enabled)
    return;

  auto &c = carrier, &f = frequency, &a = amplitude;
  double mainpos = c.offset, maindelta = 256 * c.pitch / nsamples;
  for (size_t i = 0; i < nsamples; ++i)
  {
    auto s = [=](double p = 1) { return 256 * p * i / nsamples; };
    // Take sample from each of the three signal generators:
    int freqval = f.wave[0xFF & int(f.offset + s(f.pitch))] * f.level;
    int ampval  = a.wave[0xFF & int(a.offset + s(a.pitch))] * a.level;
    int mainval = c.wave[0xFF & int(mainpos)] * c.level;
    // Apply amplitude & envelope to the main signal level:
    buffer[i] = mainval * (ampval + 4096) / 4096 * envelope.evaluate(s()) / 4096;
    // Apply frequency modulation to maindelta:
    mainpos += maindelta * (1 + (freqval / (freqval < 0 ? 8192. : 2048.)));
  }
}

Pixtone::Pixtone() {}
Pixtone::~Pixtone() {}

Pixtone *Pixtone::getInstance()
{
  return Singleton<Pixtone>::get();
}

std::function<void(int chan)> sfxCallback;

void mySfxCallback(int chan)
{
  sfxCallback(chan);
}

bool Pixtone::init()
{
  if (_inited)
  {
    LOG_ERROR("pxt_init: pxt module already initialized");
    return false;
  }
  else
    _inited = true;

  for (uint16_t i = 0; i < 256; i++)
    _sound_fx[i].channel = -1;

  for (uint32_t seed = 0, i = 0; i < 256; ++i)
  {
    seed                       = (seed * 214013) + 2531011;          // Linear congruential generator
    wave[MOD_SINE].table[i]    = 0x40 * std::sin(i * 3.1416 / 0x80); // Sine
    wave[MOD_TRI].table[i]     = ((0x40 + i) & 0x80) ? 0x80 - i : i; // Triangle
    wave[MOD_SAWUP].table[i]   = -0x40 + i / 2;                      // Sawtooth up
    wave[MOD_SAWDOWN].table[i] = 0x40 - i / 2;                       // Sawtooth down
    wave[MOD_SQUARE].table[i]  = 0x40 - (i & 0x80);                  // Square
    wave[MOD_NOISE].table[i]   = (signed char)(seed >> 16) / 2;      // Pseudorandom
  }

  uint32_t slot;

  LOG_INFO("Loading Sound FX...");

  std::string path = ResourceManager::getInstance()->getPathForDir("pxt/");
  for (slot = 1; slot <= NUM_SOUNDS; slot++)
  {
    std::ostringstream filename;
    filename << path << "fx" << std::hex << std::setw(2) << std::setfill('0') << slot << ".pxt";
    stPXSound snd;

    if (!snd.load(filename.str()))
      continue;
    snd.render();

    // upscale the sound to 16-bit for SDL_mixer then throw away the now unnecessary 8-bit data
    _prepareToPlay(&snd, slot);
    snd.freeBuf();
  }

  return true;
}

void Pixtone::shutdown()
{
  for (uint32_t i = 0; i <= NUM_SOUNDS; i++)
  {
    if (_sound_fx[i].chunk)
    {
      free(_sound_fx[i].chunk->buffer);
      free(_sound_fx[i].chunk);
      _sound_fx[i].chunk = nullptr;
    }
    for (int i = 0; i < NUM_RESAMPLED_BUFFERS; i ++)
    {
      if (_sound_fx[i].resampled[i])
      {
        free(_sound_fx[i].resampled[i]->buffer);
        free(_sound_fx[i].resampled[i]);
        _sound_fx[i].resampled[i] = nullptr;
      }
    }
  }
}

int Pixtone::play(int32_t chan, int32_t slot, int32_t loop)
{
  if (_sound_fx[slot].chunk)
  {
    chan                    = SSPlayChunk(chan, _sound_fx[slot].chunk, slot, loop, mySfxCallback);
    _sound_fx[slot].channel = chan;
    _slots[chan]            = slot;

    if (chan < 0)
    {
      LOG_ERROR("Pixtone::play: Mix_PlayChannel returned error");
    }
    return chan;
  }
  else
  {
    LOG_ERROR("Pixtone::play: sound slot {} not rendered", slot);
    return -1;
  }
}

int Pixtone::playResampled(int32_t chan, int32_t slot, int32_t loop, uint32_t percent)
{
  if (_sound_fx[slot].chunk)
  {
    uint32_t resampled_rate = SAMPLE_RATE * (percent / 100);

    int i;
    int idx = -1;
    int rslot = 0;

    for (i = 0; i < NUM_RESAMPLED_BUFFERS; i++)
    {
      if (resampled_rate == _sound_fx[slot].resampled_rate[i])
      {
        idx = i; // found
      }
      if (_sound_fx[slot].resampled[i] == NULL)
      {
        if (rslot == 0)
          rslot = i;
      }
    }

    if (idx == -1)
    {
      SSChunk *rchunk       = (SSChunk*)malloc(sizeof(SSChunk));

      if (!rchunk)
        return -1;

      _sound_fx[slot].resampled[rslot] = rchunk;
      _sound_fx[slot].resampled_rate[rslot] = resampled_rate;

      double ratio    = static_cast<double>(percent / 100);
      //double ratio    = (double)resampled_rate / (double)SAMPLE_RATE;

      const retro_resampler_t *resampler = NULL;
      void *resampler_data = NULL;

      retro_resampler_realloc(&resampler_data,
          &resampler,
          "nearest",
          RESAMPLER_QUALITY_DONTCARE,
          ratio);

      if (resampler && resampler_data)
      {
        struct resampler_data info;

        float *float_buf = (float*)malloc(_sound_fx[slot].chunk->length * 2 * 
            sizeof(float));

        convert_s16_to_float(float_buf,
            _sound_fx[slot].chunk->buffer, _sound_fx[slot].chunk->length * 2, 1.0);

        float *float_resample_buf = (float*)malloc(_sound_fx[slot].chunk->length * 2 * 
            ratio * sizeof(float));

        info.data_in       = (const float*)float_buf;
        info.data_out      = float_resample_buf;
        /* a 'frame' consists of two channels, so we set this
        * to the number of samples irrespective of channel count */
        info.input_frames  = _sound_fx[slot].chunk->length;
        info.output_frames = 0;
        info.ratio         = ratio;

        resampler->process(resampler_data, &info);
        resampler->free(resampler_data);

        /* number of output_frames does not increase with 
        * multiple channels, but assume we need space for 2 */
        rchunk->buffer = (int16_t*)malloc(info.output_frames * 2 * sizeof(int16_t));
        rchunk->length = info.output_frames;
        convert_float_to_s16(rchunk->buffer,
            float_resample_buf, info.output_frames * 2);

        free(float_buf);
        free(float_resample_buf);
      }
      idx = rslot;
    }

    chan = SSPlayChunk(chan, _sound_fx[slot].resampled[idx], slot, loop, mySfxCallback);
    _sound_fx[slot].channel = chan;
    _slots[chan]            = slot;

    if (chan < 0)
    {
      LOG_ERROR("Pixtone::playResampled: Mix_PlayChannel returned error");
    }
    return chan;
  }
  else
  {
    LOG_ERROR("Pixtone::playResampled: sound slot {} not rendered", slot);
    return -1;
  }
}

int Pixtone::prepareResampled(int32_t slot, uint32_t percent)
{
  if (_sound_fx[slot].chunk)
  {
    uint32_t resampled_rate = SAMPLE_RATE * (percent / 100);

    int i;
    int idx = -1;
    int rslot = 0;

    for (i = 0; i < NUM_RESAMPLED_BUFFERS; i++)
    {
      if (resampled_rate == _sound_fx[slot].resampled_rate[i])
      {
        idx = i; // found
      }
      if (_sound_fx[slot].resampled[i] == NULL)
      {
        if (rslot == 0)
          rslot = i;
      }
    }

    if (idx == -1) // not found
    {
      SSChunk *rchunk       = (SSChunk*)malloc(sizeof(SSChunk));

      if (!rchunk)
        return -1;

      _sound_fx[slot].resampled[rslot] = rchunk;
      _sound_fx[slot].resampled_rate[rslot] = resampled_rate;

      double ratio    = static_cast<double>(percent / 100);
      //double ratio    = (double)resampled_rate / (double)SAMPLE_RATE;

      const retro_resampler_t *resampler = NULL;
      void *resampler_data = NULL;

      retro_resampler_realloc(&resampler_data,
          &resampler,
          "nearest",
          RESAMPLER_QUALITY_DONTCARE,
          ratio);

      if (resampler && resampler_data)
      {
        struct resampler_data info;

        float *float_buf = (float*)malloc(_sound_fx[slot].chunk->length * 2 * 
            sizeof(float));

        convert_s16_to_float(float_buf,
            _sound_fx[slot].chunk->buffer, _sound_fx[slot].chunk->length * 2, 1.0);

        float *float_resample_buf = (float*)malloc(_sound_fx[slot].chunk->length * 2 * 
            ratio * sizeof(float));

        info.data_in       = (const float*)float_buf;
        info.data_out      = float_resample_buf;
        /* a 'frame' consists of two channels, so we set this
        * to the number of samples irrespective of channel count */
        info.input_frames  = _sound_fx[slot].chunk->length;
        info.output_frames = 0;
        info.ratio         = ratio;

        resampler->process(resampler_data, &info);
        resampler->free(resampler_data);

        /* number of output_frames does not increase with 
        * multiple channels, but assume we need space for 2 */
        rchunk->buffer = (int16_t*)malloc(info.output_frames * 2 * sizeof(int16_t));
        rchunk->length = info.output_frames;
        convert_float_to_s16(rchunk->buffer,
            float_resample_buf, info.output_frames * 2);

        free(float_buf);
        free(float_resample_buf);
      }
    }
  }
  else
  {
    LOG_ERROR("Pixtone::prepareResampled: sound slot {} not rendered", slot);
    return -1;
  }
  return 0;
}

void Pixtone::stop(int32_t slot)
{
  if (_sound_fx[slot].channel != -1)
  {
    SSAbortChannel(_sound_fx[slot].channel);
    if (_sound_fx[slot].channel != -1)
    {
      _slots[_sound_fx[slot].channel] = -1;
    }
  }
}

void Pixtone::pxtSoundDone(int channel)
{
  if (_slots[channel] != -1)
  {
    _sound_fx[_slots[channel]].channel = -1;
  }
}

// _prepareToPlay based on audio_mix.c audio_mix_load_wav_file
void Pixtone::_prepareToPlay(stPXSound *snd, int32_t slot)
{
  int8_t *buf                = snd->final_buffer;
  int16_t value;
  int ap;
  int i;
  SSChunk *chunk       = (SSChunk*)malloc(sizeof(SSChunk));

  if (!chunk)
    return;

  _sound_fx[slot].chunk = chunk;

  /* numsamples does not know or care about
   * multiple channels, but we need space for 2 */
  int16_t *outbuffer = (int16_t*)malloc(snd->final_size * 2 * 2);

   for(i=ap=0;i<snd->final_size;i++)
   {
     value = buf[i];
     value *= 200;

     outbuffer[ap++] = value;	// left ch
     outbuffer[ap++] = value;	// right ch
   }

  if (SAMPLE_RATE != 22050)
  {
    double ratio = (double)SAMPLE_RATE / 22050.0;

    const retro_resampler_t *resampler = NULL;
    void *resampler_data = NULL;

    retro_resampler_realloc(&resampler_data,
          &resampler,
          "nearest",
          RESAMPLER_QUALITY_DONTCARE,
          ratio);

    if (resampler && resampler_data)
    {
      struct resampler_data info;

      float *float_buf = (float*)malloc(snd->final_size * 2 * 
            sizeof(float));

      /* why is *3 needed instead of just *2? Does the 
       * sinc driver require more space than we know about? */
      float *float_resample_buf = (float*)malloc(snd->final_size * 2 * 
            ratio * sizeof(float));

      convert_s16_to_float(float_buf,
            outbuffer, snd->final_size * 2, 1.0);

      info.data_in       = (const float*)float_buf;
      info.data_out      = float_resample_buf;
      /* a 'frame' consists of two channels, so we set this
       * to the number of samples irrespective of channel count */
      info.input_frames  = snd->final_size;
      info.output_frames = 0;
      info.ratio         = ratio;

      resampler->process(resampler_data, &info);
      resampler->free(resampler_data);

      /* number of output_frames does not increase with 
       * multiple channels, but assume we need space for 2 */
      chunk->buffer = (int16_t*)malloc(info.output_frames * 2 * sizeof(int16_t));
      chunk->length = info.output_frames;
      convert_float_to_s16(chunk->buffer,
            float_resample_buf, info.output_frames * 2);

      free(float_buf);
      free(float_resample_buf);
      free(outbuffer);
    }
  }
  else
  {
    chunk->buffer = outbuffer;
    chunk->length = snd->final_size;
  }
}

} // namespace Sound
} // namespace NXE
