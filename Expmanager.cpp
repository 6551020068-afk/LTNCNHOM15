#include "ExpManager.hpp"

#include <algorithm>
#include <iostream>

void ExpManager::spawnOrb(sf::Vector2f pos, int value) {
  orbs_.emplace_back(pos, value);
}

int ExpManager::update(float dt, sf::Vector2f playerPos) {
  int gained = 0;
  for (auto& orb : orbs_) {
    orb.update(dt, playerPos);
    if (orb.isCollected()) gained += orb.getValue();
  }
  orbs_.erase(std::remove_if(orbs_.begin(), orbs_.end(),
                             [](const ExpOrb& o) { return o.isCollected(); }),
              orbs_.end());
  return gained;
}

void ExpManager::draw(sf::RenderTarget& target) const {
  for (auto& orb : orbs_) orb.draw(target);
}

bool ExpManager::addExp(int amount) {
  if (amount <= 0) return false;
  currentExp_ += amount;
  totalExp_ += amount;
  bool up = false;
  while (currentExp_ >= expRequired(level_)) {
    currentExp_ -= expRequired(level_);
    ++level_;
    up = true;
    std::cout << "[EXP] Level UP -> Lv" << level_ << "\n";
  }
  return up;
}