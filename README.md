# AirGuitar

**Air guitar simulator — real-time hand tracking via TFLite, physics-driven audio, all in C++.**

![build](https://img.shields.io/badge/build-passing-brightgreen) ![C++20](https://img.shields.io/badge/C++-20-blue) ![macOS](https://img.shields.io/badge/platform-macOS-lightgrey) ![License](https://img.shields.io/badge/license-Apache%202.0-green)

> Target: <15 ms round-trip latency from camera frame to audio output on Apple Silicon.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     MAIN THREAD (JUCE)                          │
│   ┌──────────────┐  ┌─────────────┐  ┌──────────┐  ┌────────┐ │
│   │  Application  │  │ MainWindow  │  │MainComp. │  │  HUD   │ │
│   │   (JUCEApp)   │─▶│ (DocWindow) │─▶│(OpenGL   │─▶│ Overlay│ │
│   └──────────────┘  └─────────────┘  │  canvas)  │  └────────┘ │
│                                       └─────┬─────┘            │
├─────────────────────────────────────────────┼───────────────────┤
│                    VISION THREAD             │                  │
│   ┌──────────┐  ┌─────────────┐  ┌──────────┘                  │
│   │  Camera  │─▶│ HandPipeline │  │  results (landmarks)       │
│   │ (OpenCV) │  │ (inference   │  │  via atomic swap           │
│   │ 1280×720 │  │  orchestr.)  │  │                            │
│   └──────────┘  └──────┬───────┘                               │
│                        │                                       │
│             ┌──────────┼──────────┐                             │
│             ▼          ▼          ▼                             │
│      ┌──────────┐ ┌──────────┐ ┌──────────┐                    │
│      │  Palm    │ │  Hand    │ │  Pose    │                    │
│      │ Detector │ │ Landmark │ │ Landmark │                    │
│      │ (every   │ │  (warp + │ │          │                    │
│      │  3rd f.) │ │  infer)  │ │          │                    │
│      └──────────┘ └──────────┘ └──────────┘                    │
└─────────────────────────────────────────────────────────────────┘
```

---

## Features

- **Real-time hand tracking** — MediaPipe Palm Detection + Hand Landmark models via TFLite C++ API
- **Full-body pose landmarks** — 33-keypoint pose model for strumming arm tracking
- **Hybrid audio engine** — Karplus-Strong synthesis for plucked strings + sample playback layer (WIP)
- **Physics simulation** — 240 Hz fixed timestep loop with string vibration, fret press detection (WIP)
- **Low-latency pipeline** — Decoupled camera capture, async inference, and audio callback threads

---

## Requirements

- macOS 14+ (Sonoma)
- Xcode 16+ (command line tools)
- CMake 3.24+
- Homebrew

---

## Quick Start

```bash
# Install system dependency
brew install opencv

# Clone
git clone https://github.com/clefspear/AirGuitar.git
cd AirGuitar

# Run setup (downloads TFLite models)
./scripts/setup.sh

# Configure and build
cmake --preset debug
cmake --build --preset debug

# Run
./build/debug/src/AirGuitar.app/Contents/MacOS/AirGuitar
```

The first build takes 3–5 minutes as it downloads and compiles TFLite from source (cached in `~/.cache/` for subsequent builds).

---

## Run Tests

```bash
ctest --preset debug
```

---

## Project Structure

```
AirGuitar/
├── CMakeLists.txt              # Root build file
├── CMakePresets.json           # Debug / Release presets
├── cmake/                      # CMake modules
│   ├── FetchJUCE.cmake
│   ├── FetchTFLite.cmake
│   ├── FetchModels.cmake
│   └── CompilerWarnings.cmake
├── src/
│   ├── main.cpp                # Entry point
│   ├── App/                    # JUCE application shell
│   │   ├── Application.h/.cpp
│   │   └── MainComponent.h/.cpp
│   └── Vision/                 # TFLite vision pipeline
│       ├── Camera.h/.cpp
│       ├── TFLiteRuntime.h/.cpp
│       ├── PalmDetector.h/.cpp
│       ├── HandLandmarker.h/.cpp
│       ├── PoseLandmarker.h/.cpp
│       ├── HandPipeline.h/.cpp
│       └── LandmarkData.h
├── tests/
│   ├── CMakeLists.txt
│   ├── VisionTests.cpp         # 27 test cases
│   └── PhysicsTests.cpp        # Placeholders
├── scripts/
│   ├── setup.sh                # One-shot setup
│   └── download_models.sh      # Model fetcher
├── models/                     # .tflite files (downloaded)
├── resources/                  # Guitar samples (user-provided)
└── python/                     # Training scripts
    └── train_chord_classifier.py
```

---

## Model Pipeline

```
                 ┌──────────────────┐
                 │  Camera Frame     │
                 │  (1280×720 RGB)   │
                 └────────┬─────────┘
                          ▼
              ┌──────────────────────┐
              │   Palm Detector      │  ← runs every 3rd frame
              │  (TFLite, 192×192)   │
              └──────────┬───────────┘
                         │ detected palm?
                    ┌────┴────┐
                    ▼         ▼
              ┌──────────┐  ┌──────────────────┐
              │  Skip    │  │ Crop + Warp      │
              │  frame   │  │ to 224×224       │
              └──────────┘  └────────┬─────────┘
                                     ▼
                          ┌──────────────────┐
                          │ Hand Landmarker   │
                          │ (TFLite, 21 pts)  │
                          └────────┬─────────┘
                                   ▼
                          ┌──────────────────┐
                          │ Pose Landmarker   │
                          │ (TFLite, 33 pts)  │
                          └────────┬─────────┘
                                   ▼
                          ┌──────────────────┐
                          │  atomic<Results>  │──→ Main thread
                          └──────────────────┘
```

---

## Configuration

| Setting | Default | Description |
|---|---|---|
| Camera resolution | 1280×720 @ 60 fps | Matches MediaPipe training resolution |
| Inference rate | Variable (async) | Palm detect every 3rd frame, hand every frame |
| Model format | TFLite (FP32) | MediaPipe models from `storage.googleapis.com` |

Model paths are set in `cmake/FetchModels.cmake` and downloaded to `models/*.tflite`.

---

## Tech Stack

- **TensorFlow Lite** 2.16.1 — model inference (via FetchContent, built from source)
- **OpenCV** 4.13 — camera capture + image preprocessing
- **JUCE** 8 — application shell, audio, OpenGL rendering
- **Catch2** 3.6 — unit testing
- **CMake** — build system with FetchContent for dependencies

---

## License

Apache 2.0
