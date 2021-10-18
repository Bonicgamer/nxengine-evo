#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "ResourceManager.h"
#include "common/glob.h"
#include "common/misc.h"
#include "settings.h"

#include "libretro/LibretroManager.h"

#include <json.hpp>

// libretro-common
#include <streams/file_stream.h>

bool ResourceManager::fileExists(const std::string &filename)
{
  return filestream_exists(filename.c_str());
}

std::string ResourceManager::getBasePath()
{
  return std::string(LibretroManager::getInstance()->game_dir) + "/";
}

std::string ResourceManager::getUserPrefPath()
{
  return std::string(LibretroManager::getInstance()->save_dir) + "/";
}

ResourceManager::ResourceManager()
{
  findLanguages();
  findMods();
}

ResourceManager::~ResourceManager() {}

ResourceManager *ResourceManager::getInstance()
{
  return Singleton<ResourceManager>::get();
}

void ResourceManager::shutdown() {}

std::string ResourceManager::getPath(const std::string &filename, bool localized)
{
  std::vector<std::string> _paths;

  //std::string userResBase = getUserPrefPath();

  //if (!userResBase.empty())
  //{
  //  if (!_mod.empty())
  //  {
  //    if (localized) _paths.push_back(userResBase + "data/mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
  //    _paths.push_back(userResBase + "data/mods/" + _mod + "/" + filename);
  //  }
  //  if (localized) _paths.push_back(userResBase + "data/lang/" + std::string(settings->language) + "/" + filename);
  //  _paths.push_back(userResBase + "data/" + filename);
  //}

  std::string _data = getBasePath() + "data/";

  if (!_mod.empty())
  {
    if (localized) _paths.push_back(_data + "mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
    _paths.push_back(_data + "mods/" + _mod + "/" + filename);
  }

  if (localized) _paths.push_back(_data + "lang/" + std::string(settings->language) + "/" + filename);
  _paths.push_back(_data + filename);

  for (auto &_tryPath: _paths)
  {
    if (fileExists(_tryPath))
      return _tryPath;
  }

  return _paths.back();
}

std::string ResourceManager::getPrefPath(const std::string &filename)
{
  return std::string(getUserPrefPath()) + std::string(filename);
}

std::string ResourceManager::getPathForDir(const std::string &dir)
{
  return getPath(dir, false);
}

inline std::vector<std::string> glob(const std::string &pat)
{
  Glob search(pat);
  std::vector<std::string> ret;
  while (search)
  {
    ret.push_back(search.GetFileName());
    search.Next();
  }
  return ret;
}

void ResourceManager::findLanguages()
{
  std::vector<std::string> langs = glob(getPathForDir("lang/") + "*");
  _languages.push_back("english");
  for (auto &l : langs)
  {
    std::ifstream ifs(widen(l + "/system.json"));
    if (ifs.is_open())
    {
      ifs.close();
      _languages.push_back(l.substr(l.find_last_of('/') + 1));
    }
  }
}

std::vector<std::string> &ResourceManager::languages()
{
  return _languages;
}

void ResourceManager::findMods()
{
  std::vector<std::string> mods=glob(getPathForDir("mods/")+"*");
  for (auto &l: mods)
  {
//    std::cout << l << std::endl;
    std::ifstream ifs(widen(l+"/mod.json"), std::ifstream::in | std::ifstream::binary);
    if (ifs.is_open())
    {
      nlohmann::json modfile = nlohmann::json::parse(ifs);
      _mods.insert(
        {
          l.substr(l.find_last_of('/')+1),
          Mod{l.substr(l.find_last_of('/')+1), modfile["name"].get<std::string>(), modfile["skip-intro"].get<bool>()}
        }
      );

      ifs.close();
    }
  }
}

Mod& ResourceManager::mod(std::string& name)
{
  return _mods[name];
}

void ResourceManager::setMod(std::string name)
{
  _mod = name;
}

Mod& ResourceManager::mod()
{
  return _mods[_mod];
}

bool ResourceManager::isMod()
{
  return !_mod.empty();
}

std::map<std::string, Mod> &ResourceManager::mods()
{
  return _mods;
}
