#include "LibretroManager.h"
#include "../graphics/Renderer.h"


LibretroManager::LibretroManager() { }

LibretroManager::~LibretroManager() {}

LibretroManager *LibretroManager::getInstance()
{
  return Singleton<LibretroManager>::get();
}

void LibretroManager::getSettings(Settings *setfile)
{
    bool use169 = false;

    struct retro_variable var;

    var.key = { "nx-169" };
    var.value = NULL;
    if (this->environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (!strcmp(var.value, "true")) {
            use169 = true;
        }
    }

    if (use169) {
        if (setfile->resolution > 0) {
            NXE::Graphics::Renderer::getInstance()->setResolution(6);
        }
        setfile->resolution = 6;
    } else {
        if (setfile->resolution > 0) {
            NXE::Graphics::Renderer::getInstance()->setResolution(1);
        }
        setfile->resolution = 1;
    }

    var.key = { "nx-strafing" };
    var.value = NULL;
    if (this->environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (!strcmp(var.value, "true")) {
            setfile->strafing = true;
        } else {
            setfile->strafing = false;
        }
    }

    var.key = { "nx-ani-facepics" };
    var.value = NULL;
    if (this->environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (!strcmp(var.value, "true")) {
            setfile->animated_facepics = true;
        } else {
            setfile->animated_facepics = false;
        }
    }

    var.key = { "nx-accept-key" };
    var.value = NULL;
    if (this->environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (!strcmp(var.value, "Jump")) {
            ACCEPT_BUTTON = JUMPKEY;
        } else {
            ACCEPT_BUTTON = FIREKEY;
        }
    }

    var.key = { "nx-decline-key" };
    var.value = NULL;
    if (this->environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (!strcmp(var.value, "Fire")) {
            DECLINE_BUTTON = FIREKEY;
        } else {
            DECLINE_BUTTON = JUMPKEY;
        }
    }

    var.key = { "nx-lights" };
    var.value = NULL;
    if (this->environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (!strcmp(var.value, "true")) {
            setfile->lights = true;
        } else {
            setfile->lights = false;
        }
    }

    var.key = { "nx-rumble" };
    var.value = NULL;
    if (this->environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (!strcmp(var.value, "true")) {
            setfile->rumble = true;
        } else {
            setfile->rumble = false;
        }
    }

    var.key = { "nx-fps" };
    var.value = NULL;
    if (this->environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value) {
        if (!strcmp(var.value, "true")) {
            this->retro_60hz = true;
        } else {
            this->retro_60hz = false;
        }
    }
}
