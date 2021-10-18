#pragma once

#include <libretro.h>
#include "../Singleton.h"
#include "../settings.h"
#include "../graphics/Renderer.h"

class LibretroManager
{

public:
    static LibretroManager *getInstance();

    char* game_dir;
    char* save_dir;

    bool retro_60hz;

    void getSettings(Settings *setfile);

    retro_environment_t environ_cb;
    retro_video_refresh_t video_cb;
    retro_audio_sample_batch_t audio_cb;
    retro_input_poll_t input_poll_cb;
    retro_input_state_t input_state_cb;

    retro_rumble_interface rumble_interface;
    bool can_rumble;

    uint32_t rumble_timer = 0;

protected:
  friend class Singleton<LibretroManager>;

  LibretroManager();
  ~LibretroManager();
  LibretroManager(const LibretroManager &) = delete;
  LibretroManager &operator=(const LibretroManager &) = delete;
};
