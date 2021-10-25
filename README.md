## To those who think this is a real libretro port
This is just a small thing that I worked on and is in no way an official
libretro port. I'm not too great with programing and there are probably a
bunch of mistakes that I've made. I recommend that you instead use normal
NXEngine-evo or the official NXEngine-libretro.

## Why I made this
I wanted learn more about libretro and how it works, so I went and tried
porting NXEngine-evo with only some programing experiance and zero experiance
with the libretro API. It works so that makes me happy :)

## Running in Retroarch
For me, Retroarch doesn't want to open Doukutsu.exe even though the core is
loaded. A work-around I found is putting a png file in the same directory as
Doukutsu.exe and then opening the png file in Retroarch with the core loaded. I
don't know if there is a better way of doing it and this works so I haven't
been bothered to look up a better way.

## Sound
I have no idea about how to mix audio chunks and stuff, so I just took
sslib.c/h from NXEngine-libretro and modified it a little bit.

## Known issues
* Speed: On my system I'm able to go almost 10x speed disabling the framerate,
  while in NXEngine-libretro I get over 50x speed. I think the problem is that
  I'm just manualy writting to a buffer and not using opengl or anything special
  for rendering.
* Core fight: The sound effect for what I think is supposed to be rushing water
  sounds wrong (and probably other things that use resampled audio).
* Ogg: NXEngine-evo has the ability to play the new sound tracks from CS+, but
  I am lazy and haven't bothered to get that working (or even buy CS+ to get
  the sound tracks).
* Mods: NXEngine-evo has modding support which I haven't tested to see if it
  works. It might work, but I don't know.
* Reseting: Reseting from the libretro fronted seems to act weird. Reseting
  from in-game works fine.
* Menus: Trying to close the pause menu by pressing the pause menu button
  closes and then re-opens the menu. Pressing the accept button on Return
  closes the menu, but passes the button to the game (If accept is jump, you
  jump).
* Building: I've only built and tested this on Arch Linux. I have no clue
  whether or not it will work fine on other OS's or if I needed to put
  something extra in the CMakeLists.txt files.
