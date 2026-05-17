#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>

#include "Constants.hpp"

class Camera {
 public:
  explicit Camera(sf::RenderWindow& window);
  void update(float dt, sf::Vector2f target);
  void handleEvent(const sf::Event& event);
  void reset();
  const sf::View& getView() const { return view_; }

 private:
  sf::View view_;

  // Map size tính bằng pixel thế giới (dùng TILE_RENDER)
  float mapW_ =
      static_cast<float>(Constants::MAP_WIDTH * Constants::TILE_RENDER_W);
  float mapH_ =
      static_cast<float>(Constants::MAP_HEIGHT * Constants::TILE_RENDER_H);

  static constexpr float DEFAULT_ZOOM = 1.2f;
  static constexpr float ZOOM_IN_FACTOR = 0.9f;
  static constexpr float ZOOM_OUT_FACTOR = 1.1f;
  static constexpr float SMOOTH_FACTOR = 0.01f;
};