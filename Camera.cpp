#include "Camera.hpp"

#include <algorithm>

using namespace Constants;

Camera::Camera(sf::RenderWindow& window) {
  // Spawn ở giữa map (dùng TILE_RENDER_W/H cho world coords)
  const float spawnX = MAP_WIDTH * TILE_RENDER_W / 2.f;
  const float spawnY = MAP_HEIGHT * TILE_RENDER_H / 2.f;

  // Zoom x2: viewport = WIN/2
  const float vpW = static_cast<float>(WIN_W) * DEFAULT_ZOOM;
  const float vpH = static_cast<float>(WIN_H) * DEFAULT_ZOOM;

  view_ = sf::View(
      sf::FloatRect({spawnX - vpW / 2.f, spawnY - vpH / 2.f}, {vpW, vpH}));
  (void)window;
}

void Camera::update(float dt, sf::Vector2f target) {
  const sf::Vector2f current = view_.getCenter();
  float alpha = 1.f - std::pow(SMOOTH_FACTOR, dt);
  sf::Vector2f newCenter = current + (target - current) * alpha;

  sf::Vector2f half = view_.getSize() / 2.f;
  newCenter.x = std::max(half.x, std::min(newCenter.x, mapW_ - half.x));
  newCenter.y = std::max(half.y, std::min(newCenter.y, mapH_ - half.y));

  view_.setCenter(newCenter);
}

void Camera::handleEvent(const sf::Event& event) {
  if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
    if (key->code == sf::Keyboard::Key::Equal ||
        key->code == sf::Keyboard::Key::Add)
      view_.zoom(ZOOM_IN_FACTOR);
    else if (key->code == sf::Keyboard::Key::Hyphen ||
             key->code == sf::Keyboard::Key::Subtract)
      view_.zoom(ZOOM_OUT_FACTOR);
  }
}

void Camera::reset() {
  view_.setCenter(
      {MAP_WIDTH * TILE_RENDER_W / 2.f, MAP_HEIGHT * TILE_RENDER_H / 2.f});
}