0. Make sure that you are on "black-mesa-1-0 - The March 1.0 release of the game" beta branch
1. Grab the latest avail. rtx-remix build or nightly (rtx-remix-for-x86-games-.....-release -> https://github.com/NVIDIAGameWorks/dxvk-remix/actions)
2. Drag files into "steamapps/common/Black Mesa/bin"
3. Copy "_general/UltimateAsiLoader.dll" to "steamapps/common/Black Mesa/bin" folder and rename it to "winmm.dll"
4. Then copy the rest of "assets/blackmesa" to "steamapps/common/Black Mesa" (next to "bms.exe")
5. Launch the game with "run-bms-rtx.bat" or copy everything after "START bms.exe" up until the very last "%*" and paste it into your steam launch args for the game