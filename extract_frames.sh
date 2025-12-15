#!/bin/bash

# Video A
mkdir -p Frames_A
ffmpeg -i input_a.mp4 -vf "scale=854:480,fps=25" Frames_A/frame_%05d.png

# Video B
mkdir -p Frames_B
ffmpeg -i input_b.mp4 -vf "scale=854:480,fps=25" Frames_B/frame_%05d.png

# Video C
mkdir -p Frames_C
ffmpeg -i input_c.mp4 -vf "scale=854:480,fps=25" Frames_C/frame_%05d.png

# Video D
mkdir -p Frames_D
ffmpeg -i input_d.mp4 -vf "scale=854:480,fps=25" Frames_D/frame_%05d.png
