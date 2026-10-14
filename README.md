# Video Trimmer Pro ✂️

A fast, lightweight, and modern desktop utility for Windows built with **C++20** and **Qt 6 (Widgets)** that trims video files with dual cutting engines: instant lossless stream copying and millisecond-precise re-encoding.

---

## Features

- **Dual Cut Modes:**
  - **Lossless Copy:** Instantaneous cuts without re-encoding by stream copying at keyframes. Zero quality loss and near-zero render time.
  - **Accurate Transcode:** Re-encodes using `libx264` to hit millisecond-accurate cut boundaries down to individual frames.
- **Visual Dual-Handle Range Slider:** Scrub through your footage while interactively dragging start (green) and end (red) trim points on the timeline.
- **In-App Shortcuts:** Dedicated buttons to set in-points `[` and out-points `]` directly from the active playback position.
- **Embedded Playback Preview:** Integrated media player powered by Qt Multimedia to preview selections before exporting.
- **Automatic FFmpeg Management:** Automatically detects local or system FFmpeg installations, or bootstraps an official 64-bit build on first launch.
- **Integrated Auto-Updater:** Silently checks GitHub Releases on startup with one-click download, progress tracking, and seamless installation.
- **Adaptive UI:** Styled with a clean theme optimized for high readability across Windows display modes.

---

## Supported Formats

Supports all common input formats:
`.mp4`, `.mkv`, `.avi`, `.mov`, `.ts`, `.flv`, `.webm`, `.wmv`, `.m4v`

Outputs standardized, universally compatible `.mp4` video files.

---

## Building from Source

1. Clone the repository:
```
   git clone [https://github.com/ripsnortntear/VideoTrimmerPro.git](https://github.com/ripsnortntear/VideoTrimmerPro.git)
   cd VideoTrimmerPro
```
2. Open the x64 Native Tools Command Prompt for Visual Studio.
3. Configure with CMake:
```
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:\Qt\6.12.0\msvc2022_64"
```
4. Build the binary:
```
cmake --build build
```
5. Deploy Qt runtime dependencies:
```
C:\Qt\6.12.0\msvc2022_64\bin\windeployqt.exe --release --multimedia --network build\VideoTrimmer.exe
```
## License
This project is licensed under the GNU General Public License v3.0 (GPLv3). See the LICENSE file for details.
