# RPGMakerXpCustomContainer

A custom RGSS container/Game.exe which allows for patching RGSS dlls, directory rerouting and faster boot times

## Feature set

The RGSS container allows for more flexability with RPG Maker XP.

* Swap RGSSAD header and XOR keys(requires recompile)
* Ability to attach a debugger
* Directory swapping
* FastBoot to launch games faster
* More informative errors
* Ability to build and inject custom ruby modules

## Usage

1. Download the latest binary, and replace your `Game.exe` with the custom container.
2. Open your `Game.ini` in a text editor like notepad
3. Append

```
[Container]
RGSSAD=Game.rgssad
FastBoot=1
AllowDebugger=0
Dlls=
```
4. Make any changes as needed

## Confguration options

### RGSSAD

The RGSSAD option allows you to specify where to load your RGSSAD(encrypted archive) and or which name you'd like to use for it.

### FastBoot

The fastboot options leads to RPG Maker games booting much faster than usual, it skips a step which the original RPG Maker container does which is spend 500 ticks handling windows messages.

### AllowDebugger

Allow debugger patches `IsDebuggerPresent()` to allow a debugger to be attached to RPG Maker without an error coming up or the game closing.

### DLLs

Dlls lets you set a new directory for where to scan for newly loaded DLLs. This is typically useful if you have a lot of DLLs and you want to keep your main directory clean so users can easily find where to run the game.

## Building
Build using Visual Studio 2019
