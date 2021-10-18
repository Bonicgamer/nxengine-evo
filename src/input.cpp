
#include "input.h"

#include "Utils/Logger.h"
#include "console.h"
#include "game.h"
#include "nx.h"
#include "settings.h"
#include "sound/SoundManager.h"

#include "libretro/LibretroManager.h"

#include <array>

in_action mappings[INPUT_COUNT];

bool inputs[INPUT_COUNT];
bool lastinputs[INPUT_COUNT];
in_action last_sdl_action;

int ACCEPT_BUTTON = JUMPKEY;
int DECLINE_BUTTON = FIREKEY;


bool input_init(void)
{
  memset(inputs, 0, sizeof(inputs));
  memset(lastinputs, 0, sizeof(lastinputs));
  memset(mappings, -1, sizeof(mappings));
  for (int i = 0; i < INPUT_COUNT; i++)
  {
    mappings[i].key   = -1;
    mappings[i].jbut  = -1;
    mappings[i].jhat  = -1;
    mappings[i].jaxis = -1;
  }

  mappings[LEFTKEY].key      = RETRO_DEVICE_ID_JOYPAD_LEFT;
  mappings[RIGHTKEY].key     = RETRO_DEVICE_ID_JOYPAD_RIGHT;
  mappings[UPKEY].key        = RETRO_DEVICE_ID_JOYPAD_UP;
  mappings[DOWNKEY].key      = RETRO_DEVICE_ID_JOYPAD_DOWN;
  mappings[FIREKEY].key      = RETRO_DEVICE_ID_JOYPAD_A;
  mappings[JUMPKEY].key      = RETRO_DEVICE_ID_JOYPAD_B;
  mappings[STRAFEKEY].key    = RETRO_DEVICE_ID_JOYPAD_Y;
  mappings[PREVWPNKEY].key   = RETRO_DEVICE_ID_JOYPAD_L;
  mappings[NEXTWPNKEY].key   = RETRO_DEVICE_ID_JOYPAD_R;
  mappings[INVENTORYKEY].key = RETRO_DEVICE_ID_JOYPAD_SELECT;
  mappings[MAPSYSTEMKEY].key = RETRO_DEVICE_ID_JOYPAD_X;
  mappings[ESCKEY].key       = RETRO_DEVICE_ID_JOYPAD_START;
  return 0;
}

void rumble(float str, uint32_t len)
{
  LibretroManager* lrm = LibretroManager::getInstance();

  if (settings->rumble && lrm->can_rumble) {
    lrm->rumble_interface.set_rumble_state(0, RETRO_RUMBLE_STRONG, 0xffff * str);
    lrm->rumble_interface.set_rumble_state(0, RETRO_RUMBLE_WEAK, 0xffff * str);
    lrm->rumble_timer = len;
  }
}

// set the SDL key that triggers an input
void input_remap(int keyindex, in_action sdl_key)
{
  //LOG_DEBUG("input_remap(%d => %d)", keyindex, sdl_key.key);
  //	in_action old_mapping = input_get_mapping(keyindex);
  //	if (old_mapping != -1)
  //		mappings[old_mapping] = 0xff;

  //mappings[keyindex] = sdl_key;
}

// get which SDL key triggers a given input
in_action input_get_mapping(int keyindex)
{
  return mappings[keyindex];
}

int input_get_action(int32_t sdlkey)
{
  for (int i = 0; i < INPUT_COUNT; i++)
  {
    if (1 << mappings[i].key == sdlkey)
    {
      return i;
    }
  }
  return -1;
}

int input_get_action_but(int32_t jbut)
{
  return -1;
}

int input_get_action_hat(int32_t jhat, int32_t jvalue)
{
  return -1;
}

int input_get_action_axis(int32_t jaxis, int32_t jvalue)
{
  return -1;
}

const std::string input_get_name(int index)
{
  static std::array<std::string, 28> input_names = {"Left",         "Right",
                                                    "Up",           "Down",
                                                    "Jump",         "Fire",
                                                    "Strafe",       "Wpn Prev",
                                                    "Wpn Next",     "Inventory",
                                                    "Map",          "Pause",
                                                    "f1",           "f2",
                                                    "f3",           "f4",
                                                    "f5",           "f6",
                                                    "f7",           "f8",
                                                    "f9",           "f10",
                                                    "f11",          "f12",
                                                    "freeze frame", "frame advance",
                                                    "debug fly",    "Enter"};

  if (index < 0 || index >= INPUT_COUNT)
    return "invalid";

  return input_names[index];
}

void input_set_mappings(in_action *array)
{
  memset(mappings, 0xff, sizeof(mappings));
  for (int i = 0; i < INPUT_COUNT; i++)
    mappings[i] = array[i];
}

/*
void c------------------------------() {}
*/

// keys that we don't want to send to the console
// even if the console is up.
static int IsNonConsoleKey(int key)
{
  //static const int nosend[] = {SDLK_LEFT, SDLK_RIGHT, 0};

  //for (int i = 0; nosend[i]; i++)
  //  if (key == nosend[i])
  //    return true;

  return false;
}

void input_poll(void)
{
  LibretroManager::getInstance()->input_poll_cb();

  int32_t key;
  int ino; //, key;

  int16_t buttons = LibretroManager::getInstance()->input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);

  for (int i = 0; i < 16; i++) {
    key = buttons & (1 << i);
    bool keyStatus = (key > 0);
    ino = input_get_action(1 << i); // mappings[key];

    bool justPressed = false;
    if (ino != -1) {
      justPressed = (!inputs[ino] && keyStatus);
      inputs[ino] = keyStatus;
    }

    if (justPressed)
    {
      last_sdl_action.key = key;
    }
  }
}

void input_close(void) { }

/*
void c------------------------------() {}
*/

static const int buttons[] = {JUMPKEY, FIREKEY, STRAFEKEY, ACCEPT_BUTTON, DECLINE_BUTTON, 0};

bool buttondown(void)
{
  for (int i = 0; buttons[i]; i++)
  {
    if (inputs[buttons[i]])
      return 1;
  }

  return 0;
}

bool buttonjustpushed(void)
{
  for (int i = 0; buttons[i]; i++)
  {
    if (inputs[buttons[i]] && !lastinputs[buttons[i]])
      return 1;
  }

  return 0;
}

bool justpushed(int k)
{
  return (inputs[k] && !lastinputs[k]);
}
