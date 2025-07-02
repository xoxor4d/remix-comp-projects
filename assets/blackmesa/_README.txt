0. Make sure that you are on "black-mesa-1-0 - The March 1.0 release of the game" beta branch (right click the game on steam - settings - betas)
1. Grab the latest avail. rtx-remix build or nightly (rtx-remix-for-x86-games-.....-release -> https://github.com/NVIDIAGameWorks/dxvk-remix/actions)
2. Drag remix files into "steamapps/common/Black Mesa/bin"
3. [Download UltimateAsiLoader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) OR copy "_general/UltimateAsiLoader.dll" to "steamapps/common/Black Mesa/bin" folder and rename it to "winmm.dll"
4. Copy all files within "assets/blackmesa" to "steamapps/common/Black Mesa" (next to "bms.exe")
5. Launch the game with "run-bms-rtx.bat" or copy everything after "START bms.exe" up until the very last "%*" and paste it into your steam launch args for the game