# 15-Puzzle Fixed-Timestep Physics Simulation (C99 & Raylib)

A deterministic fixed-timestep physics simulation implementing grid-constrained sliding block puzzle dynamics.

## Key Features
- **Fixed-Timestep Accumulator**: Deterministic simulation execution independent of render FPS.
- **Physical Tile Integration**: Smooth velocity and position integration towards logical target slots.
- **Snapshot Interpolation**: Sub-frame visual lerping via snapshot alpha blending.
- **One-Shot Input Queueing**: Input commands consumed once per key-stroke to prevent multi-step over-triggering.

## Build Instructions (CMake)

```bash
mkdir build
cd build
cmake ..
make
./15_puzzle_physics
```
