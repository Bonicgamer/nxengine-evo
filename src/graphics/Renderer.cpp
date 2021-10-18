#include <algorithm>
#include <cassert>
// graphics routines
#include "../ResourceManager.h"
#include "../config.h"
#include "../game.h"
#include "../map.h"
#include "../nx.h"
#include "../pause/dialog.h"
#include "../version.h"
#include "Renderer.h"
#include "nx_icon.h"
#include "../Utils/Logger.h"

#include "../libretro/LibretroManager.h"

#include <cstdlib>
#include <sstream>
#include <iomanip>

#include <formats/image.h>

namespace NXE
{
namespace Graphics
{

Renderer::Renderer() {}
Renderer::~Renderer() {}

Renderer *Renderer::getInstance()
{
  return Singleton<Renderer>::get();
}

bool Renderer::init(int resolution)
{
  _current_res = resolution;
  if (!initVideo())
    return false;

  if (!font.load())
    return false;

  if (!sprites.init())
    return false;

  return true;
}

void Renderer::close()
{
  LOG_INFO("Renderer::Close()");
  font.cleanup();
  sprites.close();
  //SDL_ShowCursor(true);
  //SDL_DestroyWindow(_window);
  //_window = NULL;

  image_texture_free(_spot_light);
  free(_spot_light);
  _spot_light = NULL;

  free(frame_buf);
  frame_buf = NULL;
}

bool Renderer::isWindowVisible()
{
  return true;
}

bool Renderer::initVideo()
{
  const NXE::Graphics::gres_t *res = getResolutions(true);

  scale        = res[_current_res].scale;
  screenHeight = res[_current_res].base_height;
  screenWidth  = res[_current_res].base_width;
  widescreen   = res[_current_res].widescreen;

  std::string spotpath = ResourceManager::getInstance()->getPath("spot.png");

  _spot_light = static_cast<texture_image*>(malloc(sizeof(texture_image)));
  image_texture_load(_spot_light, spotpath.c_str());

  this->frame_buf = static_cast<uint8_t*>(malloc(screenWidth * screenHeight * 4 * sizeof(uint8_t)));
  for (int i = 0; i < screenWidth * screenHeight * 4; i++)
    this->frame_buf[i] = 0;

  return true;
}

bool Renderer::flushAll()
{
  LOG_DEBUG("Renderer::flushAll()");
  sprites.flushSheets();
  tileset.reload();
  map_flush_graphics();
  //if (!font.load())
  //  return false;

  return true;
}

void Renderer::setFullscreen(bool enable) { }

bool Renderer::setResolution(int r, bool restoreOnFailure)
{
  LOG_INFO("Renderer::setResolution({})", r);
  if (r == _current_res)
    return 0;

  if (r == 0)
  {
    scale = 1;
    widescreen = false;
  }
  else
  {
    const NXE::Graphics::gres_t *res = getResolutions();
    scale        = res[r].scale;
    screenHeight = res[r].base_height;
    screenWidth  = res[r].base_width;
    widescreen   = res[r].widescreen;
  }

  LOG_INFO("Setting scaling {}", scale);

  _current_res = r;

  free(this->frame_buf);

  this->frame_buf = static_cast<uint8_t*>(malloc(screenWidth * screenHeight * 4 * sizeof(uint8_t)));
  for (int i = 0; i < screenWidth * screenHeight * 4; i++)
    this->frame_buf[i] = 0;

  if (!flushAll())
    return false;

  recalc_map_offsets();
  textbox.RecalculateOffsets();

  return true;
}

const Graphics::gres_t *Renderer::getResolutions(bool full_list)
{
  static NXE::Graphics::gres_t res[]
      = {//      description, screen_w, screen_h, render_w, render_h, scale_factor, widescreen, enabled
         // 4:3
         {(char *)"---", 0, 0, 0, 0, 1, false, true},
         {(char *)"320x240", 320, 240, 320, 240, 1, false, true},
         {(char *)"640x480", 640, 480, 320, 240, 1, false, true},
         //        {(char*)"800x600",   800,      600,      320,      240,      2.5,          false,      true },
         //        //requires float scalefactor
         {(char *)"1024x768", 1024, 768, 341, 256, 1, false, true},
         {(char *)"1280x1024", 1280, 1024, 320, 256, 1, false, true},
         {(char *)"1600x1200", 1600, 1200, 320, 240, 1, false, true},
         // widescreen
         {(char *)"480x272", 480, 272, 480, 272, 1, true, true},
         {(char *)"1360x768", 1360, 768, 454, 256, 1, true, true},
         {(char *)"1366x768", 1366, 768, 455, 256, 1, true, true},
         {(char *)"1440x900", 1440, 900, 480, 300, 1, true, true},
         {(char *)"1600x900", 1600, 900, 533, 300, 1, true, true},
         {(char *)"1920x1080", 1920, 1080, 480, 270, 1, true, true},
         {(char *)"2560x1440", 2560, 1440, 512, 288, 1, true, true},
         {(char *)"3840x2160", 3840, 2160, 480, 270, 1, true, true},
         {NULL, 0, 0, 0, 0, 0, false, false}};
  return res;
}

int Renderer::getResolutionCount()
{
  int i;
  const gres_t *res = getResolutions();

  for (i = 0; res[i].name; i++)
    ;
  return i;
}

void Renderer::showLoadingScreen() { }

void Renderer::flip()
{
    LibretroManager::getInstance()->video_cb(frame_buf, screenWidth, screenHeight, screenWidth * 4 * sizeof(uint8_t));
}

// blit the specified portion of the surface to the screen
void Renderer::drawSurface(Surface *src, int dstx, int dsty, int srcx, int srcy, int wd, int ht)
{
  assert(src->texture());

  NXRect srcrect, dstrect;

  srcrect.x = srcx;
  srcrect.y = srcy;
  srcrect.w = wd;
  srcrect.h = ht;

  dstrect.x = dstx;
  dstrect.y = dsty;
  dstrect.w = wd;
  dstrect.h = ht;

  if (_need_clip)
    clipScaled(srcrect, dstrect);

  texture_image* tex = src->texture();

  unsigned rshift;
  unsigned gshift;
  unsigned bshift;
  unsigned ashift;
  image_texture_set_color_shifts(&rshift, &gshift, &bshift, &ashift, tex);

  for (int y = 0; y < dstrect.h && y < screenHeight; y++) {
    if (y + dstrect.y < 0)
      continue;
    for (int x = 0; x < dstrect.w && x < screenWidth; x++) {
      if (x + dstrect.x < 0)
        continue;

      uint32_t clr = tex->pixels[(x + srcrect.x) + (y + srcrect.y) * tex->width];
      uint8_t r = (clr >> rshift) & 0xff;
      uint8_t g = (clr >> gshift) & 0xff;
      uint8_t b = (clr >> bshift) & 0xff;
      uint8_t a = (clr >> ashift) & 0xff;

      if (a > 0)
        drawPixel(x + dstrect.x, y + dstrect.y, r, g, b, src->alpha);
    }
  }
}

// blit the specified portion of the surface to the screen
void Renderer::drawSurfaceMirrored(Surface *src, int dstx, int dsty, int srcx, int srcy, int wd, int ht)
{
  assert(src->texture());

  NXRect srcrect, dstrect;

  srcrect.x = srcx;
  srcrect.y = srcy;
  srcrect.w = wd;
  srcrect.h = ht;

  dstrect.x = dstx;
  dstrect.y = dsty;
  dstrect.w = wd;
  dstrect.h = ht;

  if (_need_clip)
    clipScaled(srcrect, dstrect);

  texture_image* tex = src->texture();

  unsigned rshift;
  unsigned gshift;
  unsigned bshift;
  unsigned ashift;
  image_texture_set_color_shifts(&rshift, &gshift, &bshift, &ashift, tex);

  for (int y = 0; y < dstrect.h && y < screenHeight; y++) {
    if (y + dstrect.y < 0)
      continue;
    for (int x = 0; x < dstrect.w && x < screenWidth; x++) {
      if (x + dstrect.x < 0)
        continue;

      uint32_t clr = tex->pixels[(x + srcrect.x) + (y + srcrect.y) * tex->width];
      uint8_t r = (clr >> rshift) & 0xff;
      uint8_t g = (clr >> gshift) & 0xff;
      uint8_t b = (clr >> bshift) & 0xff;
      uint8_t a = (clr >> ashift) & 0xff;

      if (a > 0)
        drawPixel(x + dstrect.x, y + dstrect.y, r, g, b, src->alpha);
    }
  }
}

// blit the specified surface across the screen in a repeating pattern
void Renderer::blitPatternAcross(Surface *sfc, int x_dst, int y_dst, int y_src, int height)
{
  NXRect srcrect, dstrect;

  srcrect.x = 0;
  srcrect.w = sfc->width();
  srcrect.y = y_src;
  srcrect.h = height;

  dstrect.w = srcrect.w;
  dstrect.h = srcrect.h;

  int x      = x_dst;
  int y      = y_dst;
  int destwd = screenWidth;

  assert(!_need_clip && "clip for blitpattern is not implemented");

  texture_image* tex = sfc->texture();

  unsigned rshift;
  unsigned gshift;
  unsigned bshift;
  unsigned ashift;
  image_texture_set_color_shifts(&rshift, &gshift, &bshift, &ashift, tex);
  do
  {
    dstrect.x = x;
    dstrect.y = y;
      for (int y = 0; y < dstrect.h && y < screenHeight; y++) {
        if (y + dstrect.y < 0)
          continue;
        for (int x = 0; x < dstrect.w && x < screenWidth; x++) {
          if (x + dstrect.x < 0)
            continue;

          uint32_t clr = tex->pixels[(x + srcrect.x) + (y + srcrect.y) * tex->width];
          uint8_t r = (clr >> rshift) & 0xff;
          uint8_t g = (clr >> gshift) & 0xff;
          uint8_t b = (clr >> bshift) & 0xff;
          uint8_t a = (clr >> ashift) & 0xff;

          if (a > 0)
            drawPixel(x + dstrect.x, y + dstrect.y, r, g, b, sfc->alpha);
        }
      }
    x += sfc->width();
  } while (x < destwd);
}

void Renderer::drawLine(int x1, int y1, int x2, int y2, NXColor color) { }

void Renderer::drawRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b)
{
  NXRect rects[4];

  rects[0] = {x1, y1, (x2 - x1) + 1, 1};
  rects[1] = {x1, y2, (x2 - x1) + 1, 1};
  rects[2] = {x1, y1, 1, (y2 - y1) + 1};
  rects[3] = {x2, y1, 1, (y2 - y1) + 1};

  for (int i = 0; i < 4; i++) {
    for (int y = rects[i].y; y < rects[i].y + rects[i].h && y < screenHeight; y++) {
      for (int x = rects[i].x; x < rects[i].x + rects[i].w && x < screenWidth; x++) {
        drawPixel(x, y, r, g, b, 255);
      }
    }
  }
}

void Renderer::fillRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b)
{
  NXRect rect;

  rect.x = x1;
  rect.y = y1;
  rect.w = ((x2 - x1) + 1);
  rect.h = ((y2 - y1) + 1);

  for (int y = rect.y; y < rect.y + rect.h && y < screenHeight; y++) {
    for (int x = rect.x; x < rect.x + rect.w && x < screenWidth; x++) {
      drawPixel(x, y, r, g, b, 255);
    }
  }
}


void Renderer::tintScreen()
{
  for (int y = 0; y < screenHeight; y++) {
    for (int x = 0; x < screenWidth; x++) {
        drawPixel(x, y, 0, 0, 0, 150);
    }
  }
}

void Renderer::drawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  if (x >= screenWidth)
    return;
  if (y >= screenHeight)
    return;
  if (x < 0)
    return;
  if (y < 0)
    return;

  uint8_t* buf = frame_buf + (x + screenWidth * y) * 4;

  if (a == 255) {
    buf[0] = b;
    buf[1] = g;
    buf[2] = r;
  } else {
    buf[0] -= (((float)a / 255.00f)*(buf[0] - b));
    buf[1] -= (((float)a / 255.00f)*(buf[1] - g));
    buf[2] -= (((float)a / 255.00f)*(buf[2] - r));
  }
}

void Renderer::clearScreen(uint8_t r, uint8_t g, uint8_t b)
{
  uint32_t clr = 0;
  clr |= r << 16;
  clr |= g << 8;
  clr |= b;
  for (int i = 0; i < screenHeight * screenWidth; i++)
      reinterpret_cast<uint32_t*>(frame_buf)[i] = clr;
}

void Renderer::setClip(int x, int y, int w, int h)
{
  _need_clip = true;

  _clip_rect.x = x;
  _clip_rect.y = y;
  _clip_rect.w = w;
  _clip_rect.h = h;
}

void Renderer::clearClip()
{
  _need_clip = false;
}

bool Renderer::isClipSet()
{
  return _need_clip;
}

void Renderer::clip(NXRect &srcrect, NXRect &dstrect)
{
  int w,h;
  int dx, dy;

  w = dstrect.w;
  h = dstrect.h;

  dx = _clip_rect.x - dstrect.x;
  if (dx > 0)
  {
    w -= dx;
    dstrect.x += dx;
    srcrect.x += dx;
  }
  dx = dstrect.x + w - _clip_rect.x - _clip_rect.w;
  if (dx > 0)
    w -= dx;

  dy = _clip_rect.y - dstrect.y;
  if (dy > 0)
  {
    h -= dy;
    dstrect.y += dy;
    srcrect.y += dy;
  }
  dy = dstrect.y + h - _clip_rect.y - _clip_rect.h;
  if (dy > 0)
    h -= dy;

  dstrect.w = w;
  dstrect.h = h;
  srcrect.w = w;
  srcrect.h = h;
}

void Renderer::clipScaled(NXRect &srcrect, NXRect &dstrect)
{
  int w, h;
  int dx, dy;

  w = dstrect.w;
  h = dstrect.h;

  dx = _clip_rect.x - dstrect.x;
  if (dx > 0)
  {
    w -= dx;
    dstrect.x += dx;
    srcrect.x += dx;
  }
  dx = dstrect.x + w - _clip_rect.x - _clip_rect.w;
  if (dx > 0)
    w -= dx;

  dy = _clip_rect.y - dstrect.y;
  if (dy > 0)
  {
    h -= dy;
    dstrect.y += dy;
    srcrect.y += dy;
  }
  dy = dstrect.y + h - _clip_rect.y - _clip_rect.h;
  if (dy > 0)
    h -= dy;

  dstrect.w = srcrect.w = w;
  dstrect.h = srcrect.h = h;
}

void Renderer::saveScreenshot() { }

void Renderer::drawSpotLight(int x, int y, Object* o, int r, int g, int b, int upscale)
{
  NXRect dstrect;
  int width = o->Width() / CSFI;
  int height = o->Height() / CSFI;

  dstrect.x = (x - (width * (upscale / 2)) + (width / 2));
  dstrect.y = (y - (height * (upscale / 2)) + (height / 2));
  dstrect.w = width * upscale;
  dstrect.h = height * upscale;

  float x_factor = ((float)_spot_light->width/(float)dstrect.w);
  float y_factor = ((float)_spot_light->height/(float)dstrect.h);

  uint32_t imgX = 0;
  uint32_t imgY = 0;

  for (int y = 0; y < dstrect.h && y < screenHeight; y++) {
    imgY = floor(y * y_factor);
    for (int x = 0; x < dstrect.w && x < screenWidth; x++) {
      imgX = floor(x * x_factor);
      uint8_t* pixes = reinterpret_cast<uint8_t*>(_spot_light->pixels);
      uint32_t pos = (imgX + (_spot_light->width * imgY)) * 4;
      drawPixel(x + dstrect.x, y + dstrect.y, r, g, b, pixes[pos + 3]);
    }
  }
}



};
};
