#include <libretro.h>
#include <cstdint>
#include <cstdlib>
#include <string.h>

#include "../game.h"
#include "../graphics/Renderer.h"
#include "../input.h"
#include "../map.h"
#include "../profile.h"
#include "../settings.h"
#include "../statusbar.h"
#include "../trig.h"
#include "../tsc.h"
#include "../nx.h"
 
using namespace NXE::Graphics;
#include "../ResourceManager.h"
#include "../caret.h"
#include "../console.h"
#include "../screeneffect.h"
#include "../sound/SoundManager.h"
#include "../sound/Organya.h"
#include "../Utils/Logger.h"
using namespace NXE::Utils;

#include "LibretroManager.h"

int fps                  = 0;
int flipacceltime;
bool freshstart;

static unsigned g_frame_cnt;

// sslib.cpp
void mixaudio(int16_t *stream, size_t len_samples);

unsigned retro_get_tick(void)
{
	return g_frame_cnt;
}

static void map_init() 
{
    // SSS/SPS persists across stage transitions until explicitly
    // stopped, or you die & reload. It seems a bit risky to me,
    // but that's the spec.
    if (game.switchstage.mapno >= MAPNO_SPECIALS)
    {
      NXE::Sound::SoundManager::getInstance()->stopLoopSfx();
    }

    // enter next stage, whatever it may be
    if (game.switchstage.mapno == LOAD_GAME || game.switchstage.mapno == LOAD_GAME_FROM_MENU)
    {
      if (game.switchstage.mapno == LOAD_GAME_FROM_MENU)
        freshstart = true;

      LOG_DEBUG("= Loading game =");
      if (game_load(settings->last_save_slot))
      {
        //fatal("savefile error");
        //goto ingame_error;
      }
      fade.set_full(FADE_IN);
    }
    else if (game.switchstage.mapno == TITLE_SCREEN)
    {
      //LOG_DEBUG("= Title screen =");
      game.curmap = TITLE_SCREEN;
    }
    else
    {
      if (game.switchstage.mapno == NEW_GAME || game.switchstage.mapno == NEW_GAME_FROM_MENU)
      {
        bool show_intro = (game.switchstage.mapno == NEW_GAME_FROM_MENU || ResourceManager::getInstance()->isMod());
          memset(game.flags, 0, sizeof(game.flags));
          memset(game.skipflags, 0, sizeof(game.skipflags));
          textbox.StageSelect.ClearSlots();

          game.quaketime = game.megaquaketime = 0;
          game.showmapnametime                = 0;
          game.debug.god                      = 0;
          game.running                        = true;
          game.frozen                         = false;

          // fully re-init the player object
          Objects::DestroyAll(true);
          game.createplayer();

          player->maxHealth = 3;
          player->hp        = player->maxHealth;

          game.switchstage.mapno        = STAGE_START_POINT;
          game.switchstage.playerx      = 10;
          game.switchstage.playery      = 8;
          game.switchstage.eventonentry = (show_intro) ? 200 : 91;

          fade.set_full(FADE_OUT);
      }

      // slide weapon bar on first intro to Start Point
      if (game.switchstage.mapno == STAGE_START_POINT && game.switchstage.eventonentry == 91)
      {
        freshstart = true;
      }

      // switch maps
      load_stage(game.switchstage.mapno);

      player->x = (game.switchstage.playerx * TILE_W) * CSFI;
      player->y = (game.switchstage.playery * TILE_H) * CSFI;
    }

    // start the level
    game.initlevel();

    if (freshstart)
      weapon_introslide();
}

static inline void run_tick()
{
    input_poll();

    game.tick();

    memcpy(lastinputs, inputs, sizeof(lastinputs));

    NXE::Sound::SoundManager::getInstance()->runFade();
}

void retro_set_environment(retro_environment_t cb)
{
    LibretroManager::getInstance()->environ_cb = cb;

    struct retro_variable variables[] = {
        { "nx-strafing",
            "Strafing; false|true" },
        { "nx-ani-facepics",
            "Animated facepics; false|true" },
        { "nx-169",
            "Use 16:9; false|true" },
        { "nx-accept-key",
            "Accept button; Jump|Fire" },
        { "nx-decline-key",
            "Decline button; Fire|Jump" },
        { "nx-lights",
            "Lights; false|true" },
        { "nx-rumble",
            "Force feedback; false|true" },
        { "nx-fps",
            "Do 60hz; false|true" },
        { NULL, NULL },
    };

   static const struct retro_controller_description controllers[] = {
      { "Nintendo DS", RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0) },
   };

   static const struct retro_controller_info ports[] = {
      { controllers, 1 },
      { NULL, 0 },
   };

   cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

    cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
}

void retro_set_video_refresh(retro_video_refresh_t cb) { LibretroManager::getInstance()->video_cb = cb; }

void retro_set_audio_sample(retro_audio_sample_t cb) { }

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { LibretroManager::getInstance()->audio_cb = cb; }

void retro_set_input_poll(retro_input_poll_t cb) { LibretroManager::getInstance()->input_poll_cb = cb; }

void retro_set_input_state(retro_input_state_t cb) { LibretroManager::getInstance()->input_state_cb = cb; }

void retro_init(void)
{
    flipacceltime = 0;

    input_init();

}
void retro_deinit(void) { }

void retro_get_system_info(struct retro_system_info *info)
{
    info->library_name = "NXEngine-evo";
    info->library_version = "2.6.5";
    info->valid_extensions = "exe";
    info->need_fullpath = true;
    info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
    info->geometry.base_width   = Renderer::getInstance()->screenWidth;
    info->geometry.base_height  = Renderer::getInstance()->screenHeight;
    info->geometry.max_width    = Renderer::getInstance()->screenWidth;
    info->geometry.max_height   = Renderer::getInstance()->screenHeight;
    info->geometry.aspect_ratio = (float)Renderer::getInstance()->screenWidth / (float)Renderer::getInstance()->screenHeight;
    info->timing.fps            = 60.0;
    info->timing.sample_rate    = SAMPLE_RATE;
}

void retro_set_controller_port_device(unsigned port, unsigned device) { }

void retro_reset(void)
{
  lastinputs[F2KEY] = true;
  game.reset();
}

static void run_main()
{
    game.switchstage.mapno = -1;

    run_tick();

    if (game.running && game.switchstage.mapno >= 0) {
        game.stageboss.OnMapExit();
        freshstart = false;

        map_init();
    }
}

void retro_run(void)
{
  static unsigned frame_cnt = 0;

  LibretroManager* lrm = LibretroManager::getInstance();

  if (lrm->rumble_timer > 0) {
      lrm->rumble_timer -= (lrm->rumble_timer >= 20) ? 20 : lrm->rumble_timer;
      if (lrm->rumble_timer == 0) {
        lrm->rumble_interface.set_rumble_state(0, RETRO_RUMBLE_WEAK, 0);
        lrm->rumble_interface.set_rumble_state(0, RETRO_RUMBLE_STRONG, 0);
      }
  }


  bool varUpdate = false;
  lrm->environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &varUpdate);
  if (varUpdate) {
      int lstRes = settings->resolution;
      lrm->getSettings(settings);

      if (lstRes != settings->resolution) {
          static retro_system_av_info info;

          info.geometry.base_width   = Renderer::getInstance()->screenWidth;
          info.geometry.base_height  = Renderer::getInstance()->screenHeight;
          info.geometry.max_width    = Renderer::getInstance()->screenWidth;
          info.geometry.max_height   = Renderer::getInstance()->screenHeight;
          info.geometry.aspect_ratio = (float)Renderer::getInstance()->screenWidth / (float)Renderer::getInstance()->screenHeight;
          info.timing.fps            = 60.0f;
          info.timing.sample_rate    = SAMPLE_RATE;

          lrm->environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &info);
      }
  }

  if (lrm->retro_60hz) {
      run_main();
      Renderer::getInstance()->flip();
  } else {
      if (frame_cnt % 6) {
          run_main();
          Renderer::getInstance()->flip();
      } else
          lrm->video_cb(
                  NULL,
                  Renderer::getInstance()->screenWidth,
                  Renderer::getInstance()->screenHeight,
                  Renderer::getInstance()->screenWidth * 4 * sizeof(uint8_t));
  }

  frame_cnt++;
  g_frame_cnt++;

  int16_t samples[(2 * SAMPLE_RATE) / 60 + 1] = {0};

  unsigned frames = (SAMPLE_RATE + (frame_cnt & 1 ? 30 : -30)) / 60;

  NXE::Sound::Organya::getInstance()->_musicCallback(NULL, (uint8_t *)samples, frames * 4);

  mixaudio(samples, frames * 2);
  lrm->audio_cb(samples, frames);
}

bool retro_load_game(const struct retro_game_info *info)
{
    LibretroManager* lrm = LibretroManager::getInstance();

    const char* save_dir = NULL;
    lrm->environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir);
    lrm->save_dir = strdup(save_dir);

    struct retro_game_info_ext *game_info_ext;
    if (!lrm->environ_cb(RETRO_ENVIRONMENT_GET_GAME_INFO_EXT, &game_info_ext)) {
        return false;
    }
    lrm->game_dir = strdup(game_info_ext->dir);

    struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Fire" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Jump" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Map" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Strafe" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Previous Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Next Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"Inventory" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Pause" },
      { 0 },
    };

    lrm->environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

    bool doBitmask = true;
    if (!lrm->environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, &doBitmask))
        return false;

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!lrm->environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
    {
      //log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
      return false;
    }

    lrm->can_rumble = lrm->environ_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &lrm->rumble_interface);

    (void)ResourceManager::getInstance();
    Logger::init(ResourceManager::getInstance()->getPrefPath("debug.log"));

    settings_load();

    if (!Renderer::getInstance()->init(settings->resolution)) {
        //fatal("Failed to initialize graphics.");
        return false;
    }

    if (!NXE::Sound::SoundManager::getInstance()->init()) {
        // fatal("Failed to initialize sound.");
        return 1;
    }

    if (trig_init()) {
        // fatal("Failed trig module init.");
        return false;
    }

    if (textbox.Init()) {
        // fatal("Failed to initialize textboxes.");
        return false;
    }

    if (Carets::init()) {
    //    fatal("Failed to initialize carets.");
        return false;
    }

    if (game.init())
        return 1;
    if (!game.tsc->Init()) {
        //fatal("Failed to initialize script engine.");
        return false;
    }
    game.setmode(GM_NORMAL);
    // set null stage just to have something to do while we go to intro
    game.switchstage.mapno = 0;

    char *profile_name = GetProfileName(settings->last_save_slot);
    if (settings->skip_intro && file_exists(profile_name))
        game.switchstage.mapno = LOAD_GAME;
    else
        game.setmode(GM_INTRO);

    free(profile_name);

    // for debug
    if (game.paused)
    {
        game.switchstage.mapno        = 0;
        game.switchstage.eventonentry = 0;
    }

    game.running = true;
    freshstart   = true;

    map_init();

    return true;
}

void retro_unload_game(void)
{
    game.tsc->Close();
    game.close();
    Carets::close();

    input_close();
    textbox.Deinit();
    NXE::Sound::SoundManager::getInstance()->shutdown();
    Renderer::getInstance()->close();

    free(LibretroManager::getInstance()->save_dir);
    LibretroManager::getInstance()->save_dir = NULL;
    free(LibretroManager::getInstance()->game_dir);
    LibretroManager::getInstance()->game_dir = NULL;
}

void retro_cheat_reset(void) { }

void retro_cheat_set(unsigned index, bool enabled, const char *code) { }

bool retro_load_game_special(
  unsigned game_type,
  const struct retro_game_info *info, size_t num_info
)
{
    return false;
}
/* Gets region of memory. */
void *retro_get_memory_data(unsigned id)
{
    return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    return 0;
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

unsigned retro_get_region(void)
{
    return RETRO_REGION_NTSC;
}

/* Returns the amount of data the implementation requires to serialize
 * internal state (save states).
 * Between calls to retro_load_game() and retro_unload_game(), the
 * returned size is never allowed to be larger than a previous returned
 * value, to ensure that the frontend can allocate a save state buffer once.
 */
size_t retro_serialize_size(void)
{
    return 0;
}

/* Serializes internal state. If failed, or size is lower than
 * retro_serialize_size(), it should return false, true otherwise. */
bool retro_serialize(void *data, size_t size)
{
    return false;
}

bool retro_unserialize(const void *data, size_t size)
{
    return false;
}
