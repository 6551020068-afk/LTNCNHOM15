#include "TileMap.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>

using namespace Constants;

// ── Tìm file PNG theo nhiều đường dẫn ───────────────────────
static std::string findAsset(const std::string& filename) {
  std::string base = filename;
  if (base.size() > 4 && base.substr(base.size() - 4) == ".tsx")
    base = base.substr(0, base.size() - 4) + ".png";

  return base;
}

// ────────────────────────────────────────────────────────────
void TileMap::addTileset(const std::string& filename, int firstGid, int tileW,
                         int tileH) {
  TilesetInfo info;
  info.filename = filename;
  info.firstGid = firstGid;
  info.tileW = tileW;
  info.tileH = tileH;
  tilesets_.push_back(std::move(info));
}

void TileMap::addLayer(const std::string& name, const uint32_t* data,
                       int size) {
  layers_.push_back({name, data, size, true});
}

bool TileMap::loadTilesets() {
  bool allOk = true;
  for (auto& ts : tilesets_) {
    std::string path = findAsset(ts.filename);
    if (!ts.texture.loadFromFile(path)) {
      std::cerr << "[TileMap] WARN: cannot load '" << path << "'\n";
      ts.columns = 1;
      ts.tileCount = 1;
      allOk = false;
    } else {
      auto sz = ts.texture.getSize();
      // Dùng tileW/tileH đúng của tileset (32x32) thay vì TILE_W (16)
      ts.columns = static_cast<int>(sz.x) / ts.tileW;
      ts.tileCount = ts.columns * (static_cast<int>(sz.y) / ts.tileH);
      std::cout << "[TileMap] Loaded '" << path << "'  " << sz.x << "x" << sz.y
                << "  tileSize=" << ts.tileW << "x" << ts.tileH
                << "  cols=" << ts.columns << "  tiles=" << ts.tileCount
                << '\n';
    }
  }
  return allOk;
}

// ────────────────────────────────────────────────────────────
int TileMap::findTilesetIndex(int gid) const {
  int lo = 0, hi = static_cast<int>(tilesets_.size()) - 1, result = -1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    if (tilesets_[mid].firstGid <= gid) {
      result = mid;
      lo = mid + 1;
    } else
      hi = mid - 1;
  }
  return result;
}

// ────────────────────────────────────────────────────────────
void TileMap::drawTile(sf::RenderTarget& target, uint32_t rawGid, float destX,
                       float destY) const {
  if (rawGid == 0) return;

  const bool flipH = (rawGid & FLIP_H) != 0;
  const bool flipV = (rawGid & FLIP_V) != 0;
  const bool flipAD = (rawGid & FLIP_AD) != 0;
  const int gid = static_cast<int>(rawGid & TILE_ID_MASK);
  if (gid == 0) return;

  const int tsIdx = findTilesetIndex(gid);
  if (tsIdx < 0) return;
  const TilesetInfo& ts = tilesets_[tsIdx];
  if (ts.columns <= 0) return;

  const int localId = gid - ts.firstGid;
  const int srcCol = localId % ts.columns;
  const int srcRow = localId / ts.columns;

  sf::Sprite sprite(ts.texture);
  // Cắt đúng kích thước tile từ .tsx (tileW x tileH)
  sprite.setTextureRect(
      sf::IntRect(sf::Vector2i(srcCol * ts.tileW, srcRow * ts.tileH),
                  sf::Vector2i(ts.tileW, ts.tileH)));

  // Scale: tile nguồn (ts.tileW) → tile hiển thị (TILE_RENDER_W = 32)
  // Vì cả 2 tileset đều 32x32 và TILE_RENDER = 32 nên scale = 1.0
  // Nhưng giữ công thức động để đúng mọi trường hợp
  const float sx =
      static_cast<float>(TILE_RENDER_W) / static_cast<float>(ts.tileW);
  const float sy =
      static_cast<float>(TILE_RENDER_H) / static_cast<float>(ts.tileH);
  const float W = static_cast<float>(TILE_RENDER_W);
  const float H = static_cast<float>(TILE_RENDER_H);

  // 8 trạng thái flip/rotate theo đặc tả Tiled
  if (!flipAD && !flipH && !flipV) {
    sprite.setScale({sx, sy});
    sprite.setPosition({destX, destY});
  } else if (!flipAD && flipH && !flipV) {
    sprite.setScale({-sx, sy});
    sprite.setPosition({destX + W, destY});
  } else if (!flipAD && !flipH && flipV) {
    sprite.setScale({sx, -sy});
    sprite.setPosition({destX, destY + H});
  } else if (!flipAD && flipH && flipV) {
    sprite.setScale({-sx, -sy});
    sprite.setPosition({destX + W, destY + H});
  } else if (flipAD && !flipH && !flipV) {
    sprite.setRotation(sf::degrees(90.f));
    sprite.setScale({sx, sy});
    sprite.setPosition({destX + W, destY});
  } else if (flipAD && flipH && !flipV) {
    sprite.setRotation(sf::degrees(-90.f));
    sprite.setScale({sx, sy});
    sprite.setPosition({destX, destY + H});
  } else if (flipAD && !flipH && flipV) {
    sprite.setRotation(sf::degrees(90.f));
    sprite.setScale({sx, -sy});
    sprite.setPosition({destX + W, destY + H});
  } else {
    sprite.setRotation(sf::degrees(-90.f));
    sprite.setScale({sx, -sy});
    sprite.setPosition({destX, destY});
  }

  target.draw(sprite);
}

// ────────────────────────────────────────────────────────────
void TileMap::renderLayers(sf::RenderTarget& target, const sf::View& view,
                           int startLayer, int endLayer) const {
  const sf::Vector2f center = view.getCenter();
  const sf::Vector2f vsz = view.getSize();
  const float left = center.x - vsz.x / 2.f;
  const float top = center.y - vsz.y / 2.f;
  const float right = left + vsz.x;
  const float bottom = top + vsz.y;

  const int tileLeft = std::max(0, static_cast<int>(left / TILE_RENDER_W) - 1);
  const int tileTop = std::max(0, static_cast<int>(top / TILE_RENDER_H) - 1);
  const int tileRight =
      std::min(MAP_WIDTH - 1, static_cast<int>(right / TILE_RENDER_W) + 1);
  const int tileBottom =
      std::min(MAP_HEIGHT - 1, static_cast<int>(bottom / TILE_RENDER_H) + 1);

  endLayer = std::min(endLayer, static_cast<int>(layers_.size()));

  for (int l = startLayer; l < endLayer; ++l) {
    const MapLayer& layer = layers_[l];
    if (!layer.visible || !layer.data) continue;
    for (int ty = tileTop; ty <= tileBottom; ++ty)
      for (int tx = tileLeft; tx <= tileRight; ++tx) {
        const uint32_t rawGid = layer.data[ty * MAP_WIDTH + tx];
        if (rawGid == 0) continue;
        drawTile(target, rawGid, static_cast<float>(tx * TILE_RENDER_W),
                 static_cast<float>(ty * TILE_RENDER_H));
      }
  }
}