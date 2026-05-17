#pragma once
#include <cstdint>

namespace MapData {

constexpr int MAP_WIDTH = 100;
constexpr int MAP_HEIGHT = 100;
constexpr int LAYER_SIZE = MAP_WIDTH * MAP_HEIGHT;  // 10 000

// ── Tileset descriptor ────────────────────────────────────────
struct TilesetInfo {
  const char* filename;
  int firstGid;
  int tileW;  // tilewidth  từ .tsx
  int tileH;  // tileheight từ .tsx
};

constexpr int TILESET_COUNT = 2;
inline constexpr TilesetInfo TILESETS[TILESET_COUNT] = {
    //  filename                    firstGid  tileW  tileH
    {"hinh anh\\Bestiary-Mad_Forest.png", 1, 32, 32},
    {"hinh anh\\terrain.png", 46, 32, 32},
};

// ── Layer names ───────────────────────────────────────────────
constexpr int LAYER_COUNT = 2;
inline constexpr const char* LAYER_NAMES[LAYER_COUNT] = {"floor", "oj"};

// ── Tile data (flat, row-major) ───────────────────────────────
extern const uint32_t LAYER_FLOOR[];  // layer "floor" (id=1)
extern const uint32_t LAYER_CO[];     // layer "oj"    (id=3)

}  // namespace MapData