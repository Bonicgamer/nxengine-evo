#include "Surface.h"
#include "Renderer.h"
#include "../Utils/Logger.h"

#include <formats/image.h>

namespace NXE
{
namespace Graphics
{

Surface::Surface()
    : _texture(nullptr)
    , _width(0)
    , _height(0)
{
}

Surface::~Surface()
{
  cleanup();
}

// load the surface from a .pbm or bitmap file
bool Surface::loadImage(const std::string &pbm_name, bool use_colorkey)
{
  cleanup();

  _texture = static_cast<texture_image*>(malloc(sizeof(texture_image)));
  if (image_texture_load(_texture, pbm_name.c_str()) <= 0)
      return false;
  //LOG_INFO("{}", _texture->width);

  _width = _texture->width;
  _height = _texture->height;

  if (use_colorkey)
  {
    uint8_t* pixes = reinterpret_cast<uint8_t*>(_texture->pixels);
    for (int y = 0; y < _height; y++) {
      for (int x = 0; x < _width; x++) {
        if (pixes[x * 4 + y * 4 * _width + 0] == 0 &&
            pixes[x * 4 + y * 4 * _width + 1] == 0 &&
            pixes[x * 4 + y * 4 * _width + 2] == 0
           )
            pixes[x * 4 + y * 4 * _width + 3] = 0;
      }
    }
  }

  if (!_texture)
  {
    //LOG_ERROR("Surface::LoadImage: SDL_CreateTextureFromSurface failed: {}", SDL_GetError());
    return false;
  }

  return true;
}

Surface *Surface::fromFile(const std::string &pbm_name, bool use_colorkey)
{
  Surface *sfc = new Surface;
  if (!sfc->loadImage(pbm_name, use_colorkey))
  {
    delete sfc;
    return nullptr;
  }

  return sfc;
}

int Surface::width()
{
  return _width;
}

int Surface::height()
{
  return _height;
}

texture_image* Surface::texture()
{
  return _texture;
}

void Surface::cleanup()
{
  if (_texture)
  {
    image_texture_free(static_cast<texture_image*>(_texture));
    free(_texture);
    _texture = nullptr;
  }
}

}; // namespace Graphics
}; // namespace NXE
