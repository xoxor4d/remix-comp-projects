1. copy _general/UltimateAsiLoader.dll to your game folder and rename it to "winmm.dll" (next to GH3.exe)
2. copy all of assets/gh3 to your game folder and replace any when prompted
3. copy "a_gh3-rtx.asi" into your game folder

_________________
Commandline Args:
-no_dinput_hook     :: disable direct input hook if its causing any issues
-nobeep             :: no beep on startup

_________________
Hotkeys:
F4: toggle imgui menu
F5: toggle shader/fixed function rendering

_________________
Fullscreen mode:
If you want to play in fullscreen mode, open ".trex/bridge.conf" and change
"client.forceWindowed = True" to "client.forceWindowed = False" or comment the line using #