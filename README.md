# Ray Tracer (C++)

A ray tracer written in C++ as coursework for CSC-335. It renders spheres with
lambertian, metal and dielectric materials, checkered textures and an animated
scene. The rendered frames were assembled into a video with ffmpeg.

The animation and scene changes are my own work. The ray tracing code is based on
Peter Shirley's *Ray Tracing in One Weekend* (https://www.raytracingoneweekend.com/).

## Contents

- `main.cpp` — ray tracer and scene.
- `CMakeLists.txt` — CMake build.
- `animation.mp4` — rendered animation.

## Build

```sh
cmake -B build
cmake --build build
./build/raytracer_animation
```

The raw PPM frames are not included because of their size. Run the executable
from an empty directory: it overwrites `frame_0.ppm` … `frame_35.ppm` in the
current working directory and stops with an error if a frame cannot be written.
