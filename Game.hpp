#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>

#include "BulletManager.hpp"
#include "Camera.hpp"
#include "CharacterClass.hpp"
#include "Demonlora.hpp"
#include "ExpManager.hpp"
#include "MenuSystem.hpp"
#include "MonsterManager.hpp"
#include "Player.hpp"
#include "PlayerStats.hpp"
#include "SaveSystem.hpp"
#include "ScoreSystem.hpp"
#include "SkillManager.hpp"
#include "SoundManager.hpp"
#include "TileMap.hpp"
#include "WaveManager.hpp"
#include "dokho.hpp"

enum class UpgradeType {
  Damage,
  AttackSpeed,
  Regen,
  Knife,
  LightningRing,
  Garlic,
  HolyBible
};

struct UpgradeOption {
  UpgradeType type;
  std::string title;
  std::string desc;
  sf::Color color = sf::Color::White;
};

enum class GameState { Menu, Playing, GameOver, Victory };

class Game {
 public:
  Game();
  ~Game();
  void run();

 private:
  void processEvents(const sf::Event& event);
  void update(float dt);
  void render();
  void renderHUD();
  void renderUpgradeScreen();
  void renderGameOver();
  void renderVictory();

  void buildUpgradeOptions();
  void applyUpgrade(UpgradeType t);
  void updateHover(sf::Vector2i mousePixel);

  void applyCharacterClass(int charIndex);
  void applyDifficulty();
  void endGame();
  void restartGame();

  sf::RenderWindow window_;
  TileMap tileMap_;
  Player player_;
  Camera camera_;
  sf::Clock clock_;

  MonsterManager monsters_;
  BulletManager bullets_;
  ExpManager expManager_;
  PlayerStats stats_;
  SkillManager skillMgr_;
  WaveManager waveMgr_;

  ScoreSystem score_;
  SaveData saveData_;
  Difficulty difficulty_ = Difficulty::Easy;

  GameState gameState_ = GameState::Menu;
  bool paused_ = false;
  int hoveredCard_ = -1;
  int selectedChar_ = 0;

  std::array<UpgradeOption, 3> upgradeOptions_;
  std::unordered_map<UpgradeType, sf::Texture> upgradeIcons_;
  float levelUpTimer_ = 0.f;
  float victoryAnimTime_ = 0.f;

  std::string hudMessage_;
  float hudMessageTimer_ = 0.f;

  sf::Font font_;
  bool fontLoaded_ = false;
  MenuSystem menu_{window_, font_};

  static constexpr float MAX_DT = 0.05f;
  bool debugMode_ = false;
  bool pauseMenuOpen_ = false;
  bool settingsOpen_ = false;

  void renderPauseMenu();
  void handlePauseMenuClick(sf::Vector2f mouseUI);

  // ── Final Boss tracking ───────────────────────────────────────────────────
  DemonLord* finalBossPtr_ = nullptr;
  DemonPhase lastDemonPhase_ = DemonPhase::Phase1;
  bool victoryShown_ = false;
  int orbValue(int base) const;
};