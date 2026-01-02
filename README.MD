# Pulsrr — Video Sequencer & Mixer (VJ Tool)

Pulsrr is a **Linux-based video mixer and sequencer** designed for **VJ and live visual performance**.  
It allows users to mix **1 to 4 video layers**, apply real-time effects, create sequences, and export the final render as an **MP4 (H.264)** video.

---

## Overview

The core workflow is simple:

1. Load **1–4 videos**, each with independent FPS and scale (quality)
2. Videos are parsed into frames using **FFmpeg**
3. Apply live effects per layer (grayscale, speed, alpha)
4. Click **Update** to render and preview the mixed result
5. Save the result as a **sequence** (duration + scale)
6. Chain multiple sequences together
7. Switch to **Playback mode** to preview the final result
8. Export the full render as an **MP4 video**

---

## Project structure

```
├── .gitignore
├── Makefile
├── media/
│ ├── aphorisms.txt
│ ├── DS-TERM.TTF
│ ├── icon.png
│ └── logo.png
├── src/
│ ├── main.c
│ ├── modals/
│ │ ├── modal_add_sequence.c
│ │ ├── modal_add_sequence.h
│ │ ├── modal_download.c
│ │ ├── modal_download.h
│ │ ├── modal_fx.c
│ │ ├── modal_fx.h
│ │ ├── modal_load_video.c
│ │ └── modal_load_video.h
│ ├── components/
│ │ ├── component_aphorism.c
│ │ ├── component_aphorism.h
│ │ ├── component_layer.c
│ │ ├── component_layer.h
│ │ ├── component_screen.c
│ │ ├── component_screen.h
│ │ ├── component_sequencer.c
│ │ └── component_sequencer.h
│ ├── sdl/
│ │ ├── sdl.c
│ │ └── sdl.h
│ ├── utils/
│ │ ├── utils.c
│ │ └── utils.h
│ └── styles/
│ └── style.css
├── build/
└── README.MD
```

## Technology Stack

- **GTK 3** — UI
- **SDL2** — Rendering engine
- **FFmpeg** — Video decoding & encoding
- **X11 only**  
  > SDL cannot be embedded in GTK under Wayland.  
  > The application explicitly forces X11.

**Target platform:** Linux

---

## Architecture Overview

### Rendering Modes

SDL operates in two distinct modes:

- **Live Mode**
  - Interactive editing
  - Effects are adjustable in real time
  - Used to build sequences

- **Playback Mode**
  - Plays stored sequences sequentially
  - Effects are frozen
  - Used for final preview and export

---

## Layer System

The application supports **up to 4 layers**, displayed as blocks on the left panel.

### Layer Structure

```c
typedef struct Layer {
    char *frame_folder;

    SDL_Surface **frames;
    SDL_Surface **frames_gray;

    SDL_Texture **textures;
    SDL_Texture **textures_gray;

    int     current_frame;
    int     frame_count;
    Uint32  last_tick;

    double  speed;
    int     fps;
    Uint8   alpha;
    int     grayscale;
    int     blend_mode;

    int     width;
    int     height;

    LayerState state;
} Layer;
```

```
typedef enum {
    LAYER_EMPTY,
    LAYER_UP_TO_DATE,
    LAYER_MODIFIED
} LayerState;
```

## Layer Capabilities

Each layer supports independent control of:

- **FPS**
- **Scale (quality)**
- **Speed**
- **Alpha transparency** (0–255)
- **Grayscale** (on / off)

---

## Modals

### Load Layer Modal
- Select video file
- Set FPS
- Set scale (quality)

### Add Sequence Modal
- Set duration
- Set scale (quality)

### FX Modal
- Real-time effect control
- UI locked during FX processing

---

## Update & Rendering Pipeline

When **Update** is clicked:

1. Layer states are checked
2. Only layers marked as `LAYER_MODIFIED` are processed
3. A background thread preloads textures
4. The UI displays:
   - Loading screen
   - Progress bar
   - Log output
5. The final mixed render appears when processing completes

---

## Sequences

- A sequence is a snapshot of the live mix
- Stored entirely in memory
- Multiple sequences can be chained

### Playback Mode
- Sequences are rendered sequentially
- Effects are locked
- Used for validation before export

---

## Export

- Output format: **MP4 (H.264)**
- Multithreaded encoding
- SDL rendering is paused for performance
- Final video concatenates all sequences

---

## Sequence Folder Structure

```text
sequences/
└── sequence_X/
    ├── Frames_1/
    ├── Frames_2/
    ├── Frames_3/
    ├── Frames_4/
    ├── fx.txt
    ├── mixed_frames/
```

# Add Sequence — Test Plan & User Cases

This section defines all manual test cases required to validate the **Add Sequence** feature.

---

## 1. Modal & UI Safety

- Modal blocks all interaction during processing
- Buttons disabled during processing
- No crashes on rapid input
- Progress bar updates correctly
- Log auto-scrolls
- UI restored after completion or abort
- No GTK warnings

---

## 2. Sequence Folder Creation

- `sequences/` auto-created if missing
- `sequence_X` index increments correctly
- No overwrite of existing sequences
- Folder structure strictly respected

---

## 3. Layer Presence & Validation

- Empty layer skipped safely
- Alpha = 0 blends correctly
- Grayscale uses grayscale frames
- Speed < 1.0 repeats frames correctly
- Speed ≥ 1.0 skips frames correctly
- Missing frames logged and skipped safely

---

## 4. Abort Conditions

- All layers empty → abort safely
- No mixed frames created
- No MP4 generated
- `fx.txt` still written
- FFmpeg failure logged, no crash

---

## 5. FX Consistency

- `fx.txt` always contains all 4 layers
- FX values match live state
- Empty layers use default values
- Rendering uses only `fx.txt`

---

## 6. Frame Generation

- Resolution matches selected scale
- Background is black
- Correct layer order (0 → 3)
- Continuous frame naming
- Frame count = `duration × 25 FPS`
- No memory leaks

---

## 7. Video Encoding

- Encoding only occurs if frames exist
- Correct resolution and duration
- Encoding success and failure logged
- No crash on failure

---

## 8. Stress & Edge Cases

- Minimum and maximum duration
- Large frame counts
- Mixed speeds
- Combined FX
- Single-layer sequences

---

## 9. Behavioral Consistency

- Playback matches live render
- Deterministic output
- No dependency on live runtime state


