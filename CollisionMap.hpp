#pragma once
#include <bitset>
#include <cstdint>

#include "Constants.hpp"

class CollisionMap {
 public:
  static constexpr int TOTAL = Constants::MAP_WIDTH * Constants::MAP_HEIGHT;
  CollisionMap() { bits_.reset(); }
  void addLayer(const uint32_t* data, int size) {
    for (int i = 0; i < size && i < TOTAL; ++i)
      if ((data[i] & Constants::TILE_ID_MASK) != 0) bits_.set(i);
  }
  bool isSolid(int tx, int ty) const {
    if (tx < 0 || tx >= Constants::MAP_WIDTH || ty < 0 ||
        ty >= Constants::MAP_HEIGHT)
      return false;
    return bits_.test(ty * Constants::MAP_WIDTH + tx);
  }
  // Dùng TILE_RENDER_W/H (32) để chuyển pixel → tile index
  bool isSolidPixel(float wx, float wy) const {
    return isSolid(static_cast<int>(wx / Constants::TILE_RENDER_W),
                   static_cast<int>(wy / Constants::TILE_RENDER_H));
  }

 private:
  std::bitset<TOTAL> bits_;
};