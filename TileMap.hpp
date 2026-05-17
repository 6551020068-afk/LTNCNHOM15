#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

#include "Constants.hpp"

struct TilesetInfo {
  std::string filename;
  int firstGid = 0;
  int tileCount = 0;
  int columns = 0;
  int tileW = 32;  // kích thước tile trong PNG (đọc từ .tsx)
  int tileH = 32;
  sf::Texture texture;
};

struct MapLayer {
  std::string name;
  const uint32_t* data = nullptr;
  int size = 0;
  bool visible = true;
};

class TileMap {
 public:
  // filename: tên file PNG (hoặc .tsx — sẽ tự đổi sang .png)
  // tileW/tileH: kích thước 1 tile trong PNG (lấy từ <tileset tilewidth=
  // tileheight=>)
  void addTileset(const std::string& filename, int firstGid, int tileW = 32,
                  int tileH = 32);
  void addLayer(const std::string& name, const uint32_t* data, int size);
  bool loadTilesets();

  void renderLayers(sf::RenderTarget& target, const sf::View& view,
                    int startLayer, int endLayer) const;

  int layerCount() const { return static_cast<int>(layers_.size()); }

 private:
  std::vector<TilesetInfo> tilesets_;
  std::vector<MapLayer> layers_;

  int findTilesetIndex(int gid) const;
  void drawTile(sf::RenderTarget& target, uint32_t rawGid, float destX,
                float destY) const;
};