Quick overview:

its a video mixer. it takes between 1 and 4 videos and mix it, allow user to apply some filter on each layer (like speed, grayscale yes/no, alpha transparency (0-255).

How it works ?

I use GTK with SDL, i mean Gtk embed an SDL window on x11 envirnoment (not working on wayland). SDL has 2 mode: Live mode and playback mode, live mode allow user to create each sequence and see the render on screen. Lets start by the Live mode workflow :  User Start the software. he cans load between 1-4 videos by click on Load button of each layer. it open a load layer modal, on this modal user can set the quality (scale) and FPS of each layer independantly. then click on export. on export clicked those videos are converted to frames by ffmpeg in respected folder Frames_1 Frames_2 Frames_3 Frames_4 (folder can be empty if nothing loaded by user). once frames well loaded user can click on FX button of each layer to set some FX. then user click on Update button. this trigger and SDL function who preload all the frames for efficient rendering and display the render loop when loading finish.

if user is happy with the sequence he created he can click on Add sequence button. this will open a modal asking user for sequence duration and quality (scale) then click on add sequence. it will create and store the sequence 

to see all sequence added he can click on playback button, this put SDL on playback mode and display the final render (all sequence in a row with effect applied) user can download the finale render. or switch to live mode again and create more sequences 

Modal_fx;
- Add grab_add & grab_remove to focus on modal, set the button sensitive FALSE while FX processing

layer loading:
- Error handling with the loading form (file format incorect etc...)
- How should the export thread control FFmpeg and cancellation?

Layer component:

- the width of the preview is a bit too big, reduce it. (see bg duplication)

Sequence component:

- the height is too big (bg duplication) need to decrease it a bit.

Modal Add_Sequence: 
- Add grab_add & grab_remove to focus on modal, set the button sensitive FALSE while FX processing


# Add Sequence – Test Plan & User Cases

This part defines all manual test cases required to validate the **Add Sequence** feature, including UI safety, frame generation, FX consistency, and failure handling.

---

## 1. Modal & UI Safety

- Modal blocks **all interactions** with the rest of the application while processing
- Back button is disabled during processing
- Export button is disabled during processing
- User cannot interact with any widget during generation
- No crash if user clicks rapidly before processing starts
- UI is fully re-enabled after completion or abort
- Progress bar updates correctly and never freezes
- Modal log auto-scrolls to latest entry
- No GTK warnings or critical errors during modal lifetime

---

## 2. Sequence Folder Creation

- `sequences/` directory is created if missing
- `sequence_X/` index increments correctly
- Existing sequences are never overwritten
- Folder structure is exactly:

sequences/
sequence_X/
Frames_1/
Frames_2/
Frames_3/
Frames_4/
fx.txt
mixed_frames/ (only if generation succeeds)
sequence_X.mp4 (only if encoding succeeds)


---

## 3. Layer Presence & Validation

- Layer with **no frames**:
  - Skipped entirely during mixing
  - Warning logged in modal log
  - Still written to `fx.txt` with default values
- Layer with frames but **alpha = 0**:
  - Loaded normally
  - Blended correctly (invisible but valid)
- Layer with grayscale enabled:
  - Uses grayscale frames only
- Layer speed < 1.0:
  - Frame repetition logic is correct
- Layer speed ≥ 1.0:
  - Frame skipping logic is correct
- Missing `Frames_X` directory:
  - Logged
  - Layer skipped safely

---

## 4. Abort Conditions

- **All layers empty or invalid**:
  - Warning shown in modal log
  - Process aborts immediately
  - No `mixed_frames/` directory created
  - No mp4 generated
  - `fx.txt` still created
- Frame loading failure (`IMG_Load` fails):
  - Frame skipped
  - No crash
- ffmpeg missing or encoding failure:
  - Logged
  - No crash

---

## 5. FX Consistency

- `fx.txt` always contains **all 4 layers**
- FX values match live state at export time:
  - speed
  - gray (0 or 1)
  - alpha
- Empty layers still appear with default FX values
- No runtime/live memory is used during rendering (FX read only from `fx.txt`)

---

## 6. Frame Generation

- Mixed frame resolution matches selected scale
- Background is solid black
- Layers are blended in correct order (layer 0 → layer 3)
- Frame naming is continuous:
  - `frame_00001.png` → `frame_XXXXX.png`
- Frame count = `duration × 25 FPS`
- All SDL surfaces are freed (no memory leaks)

---

## 7. Video Encoding

- ffmpeg is executed only if `mixed_frames/` exists
- mp4 resolution matches mixed frames
- mp4 duration matches expected length
- Encoding success is logged
- Encoding failure is logged and does not crash

---

## 8. Stress & Edge Cases

- Minimum duration
- Maximum duration
- Very high number of frames per layer
- Mixed speeds (0.25, 0.5, 1, 2, 4)
- Grayscale + alpha combined
- One valid layer, three empty layers
- All layers valid but extremely short sequences

---

## 9. Behavioral Consistency

- Output behavior matches live mix logic
- Speed math is identical to SDL live render
- No dependency on global live render state
- Deterministic output:
  - Same inputs always produce the same results

---



