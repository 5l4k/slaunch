<img width="64" height="64" alt="icon" src="https://github.com/user-attachments/assets/0a70ab24-5098-4e7b-b6ce-dba046e9230e"/>  for the Nintendo Switch

[Discord](https://discord.gg/dv28MgtaNn) 

<img width="1280" height="720" alt="2026092401121200-A082AE4E5DA891D87084ACEACFDFF4A9" src="https://github.com/user-attachments/assets/fc5fcfc0-1348-454c-a057-2459c590818a" />
<img width="1280" height="720" alt="2026092323471800-A082AE4E5DA891D87084ACEACFDFF4A9" src="https://github.com/user-attachments/assets/a70ddc69-c610-4c96-9c30-7dcecc787b1d" />

<img width="1280" height="720" alt="2026092323492100-A082AE4E5DA891D87084ACEACFDFF4A9" src="https://github.com/user-attachments/assets/8bb60ac8-9c7f-4063-a435-9d5e31cc17e0" />
<img width="1280" height="720" alt="2026092323512500-A082AE4E5DA891D87084ACEACFDFF4A9" src="https://github.com/user-attachments/assets/ee98c207-121e-4938-857b-cbbd519c0d7d" />

<img width="1280" height="720" alt="2026092323472900-A082AE4E5DA891D87084ACEACFDFF4A9" src="https://github.com/user-attachments/assets/301dde03-aaad-4990-9e10-1253490fdafe" />

## Features

* **Theming & Customization:** Supports 5 built-in themes, custom user wallpapers, icon packs, custom fonts, and several UI modes. Includes layout tuning and customizable tile sizes/colors.
* **Video Wallpapers:** Supports looped, silent `.mp4` video backgrounds utilizing the Switch's native NVDEC hardware decoder.
* **Interactive Widgets:** Lua-based home screen widgets that can be dragged and repositioned via touch.
* **Background Audio:** Native support for background music playback (MP3/OGG/FLAC) and custom UI sound effects.
* **Per-Account Settings:** Fully independent profiles. Two users on the same console can have completely different themes, UI layouts, pinned homebrew, favorites, and hidden entries.
* **USB File Transfer (MTP):** Connect to a PC via USB while the menu is open to directly copy files to/from the SD card. Disconnects automatically when launching a game or app.
* **Content Filtering:** Opt-in adult and extreme-violence filter for downloaded game covers and news updates based on ratings (ESRB/PEGI/USK) and Steam tags.
* **System Localization:** Fully translatable UI using custom `.txt` files. Automatically reads the console's shared font for native CJK (Chinese, Japanese, Korean) rendering, with an included Noto Sans CJK font for fallback.
* **Diagnostic Logging:** Writes detailed boot, ECS, and daemon logs directly to the SD card to troubleshoot bring-up issues without a debugger.

## Build Instructions

1. **Install devkitPro and Dependencies:**
On Linux/WSL2, import the devkitPro keys and install the necessary Switch libraries:

```bash
sudo pacman-key --recv-keys BC26F752D25B92CE272E0F44F7FD5492264BB9D0 --keyserver keyserver.ubuntu.com
sudo pacman-key --lsign-key BC26F752D25B92CE272E0F44F7FD5492264BB9D0
sudo pacman -U https://pkg.devkitpro.org/devkitpro-keyring.pkg.tar.xz

```

Ensure `dkp-libs` and `dkp-linux` are added to your `/etc/pacman.conf`, then install:

```bash
sudo pacman -Sy switch-dev switch-sdl2 switch-sdl2_ttf switch-sdl2_image switch-sdl2_mixer switch-curl switch-mbedtls

```

*(Note: Using SDL2_image 2.8 and SDL2_mixer 2.8 instead of devkitPro's default 2.0.4 is recommended to prevent JPEG decoding crashes).*


2. **Configure Environment Variables:**
Export the required devkitPro paths so the Makefiles can locate the compiler:

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITA64=$DEVKITPRO/devkitA64
export PATH=$DEVKITA64/bin:$PATH

```


3. **Compile libstratosphere:**
Clone Atmosphere 1.11.2 and build the library into your devkitPro directory:

```bash
git clone --depth=1 --branch 1.11.2 https://github.com/Atmosphere-NX/Atmosphere /opt/atmosphere
make -C /opt/atmosphere/libraries/libstratosphere -j$(nproc)

mkdir -p $DEVKITPRO/AtmosphereLibs/include $DEVKITPRO/AtmosphereLibs/lib
cp -r /opt/atmosphere/libraries/libstratosphere/include/. $DEVKITPRO/AtmosphereLibs/include/
cp -r /opt/atmosphere/libraries/libvapours/include/.      $DEVKITPRO/AtmosphereLibs/include/
find /opt/atmosphere/libraries/libstratosphere -name '*.a' -exec cp {} $DEVKITPRO/AtmosphereLibs/lib/ \;

```


4. **Build sLaunch:**
Run `make` in the sLaunch root directory. This compiles the daemon, applet, and bundles all assets into the `SdOut/` folder.

* Use `make package` to create a deployable zip file.
* Individual targets available: `make ssystem`, `make smenu`, `make sinstaller`, `make hbloader`, `make assets`.


## Credits

* **uLaunch (Xortroll & contributors):** Architecture, applet/daemon model, ECS content-serving, and NPDMs (GPLv2).
* **nx-hbloader (switchbrew):** Foundation for the bundled homebrew loader fork (ISC).
* **Atmosphère (haze):** USB file transfer protocol (GPLv2).
* **MeepCat55:** Creator of the bundled `Minimal` icon pack.
* **Xelu:** Free Controller and Key Prompts used for UI buttons (CC0).
* **averne / FFmpeg (nvtegra branch) & SwitchWave:** NVDEC hardware-acceleration backend used for video wallpaper decoding (GPLv2+).
