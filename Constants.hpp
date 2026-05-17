#pragma once
#include <cstdint>

namespace Constants {

// Map dimensions (số tile)
inline constexpr int MAP_WIDTH = 100;
inline constexpr int MAP_HEIGHT = 100;

// Tile size trong file PNG (texcoord)
inline constexpr int TILE_W = 16;
inline constexpr int TILE_H = 16;

// Tile size render trên map (pixel thế giới) = TILE_W * 2
inline constexpr int TILE_RENDER_W = 32;
inline constexpr int TILE_RENDER_H = 32;

// Window dimensions
inline constexpr unsigned int WIN_W = 1280;
inline constexpr unsigned int WIN_H = 720;

// Tile GID helpers
inline constexpr uint32_t FLIP_H = 0x80000000;
inline constexpr uint32_t FLIP_V = 0x40000000;
inline constexpr uint32_t FLIP_AD = 0x20000000;
inline constexpr uint32_t TILE_ID_MASK = 0x1FFFFFFF;

// Camera smooth factor
inline constexpr float SMOOTH_FACTOR = 0.01f;

}  // namespace Constants