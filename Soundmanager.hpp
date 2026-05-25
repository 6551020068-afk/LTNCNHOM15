#pragma once
// ════════════════════════════════════════════════════════════
//  SoundManager.hpp  —  Quản lý toàn bộ âm thanh game
//  Fixed for SFML 3: sf::Sound has no default constructor
// ════════════════════════════════════════════════════════════

#include <SFML/Audio.hpp>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>  // ← thêm
#include <string>
#include <vector>

class SoundManager {
 public:
  // ── Enum SFX ─────────────────────────────────────────────
  enum class SFX {
    SHOOT = 0,
    HIT_MONSTER,
    PLAYER_HIT,
    LEVEL_UP,
    PICKUP_EXP,
    UI_CLICK,
    GAMEOVER,
    COUNT
  };

  // ── Enum BGM ─────────────────────────────────────────────
  enum class BGM { NONE, GAME_BGM, BOSS_BGM };

  // ── Singleton ─────────────────────────────────────────────
  static SoundManager& get() {
    static SoundManager instance;
    return instance;
  }

  // ── Khởi tạo ─────────────────────────────────────────────
  bool init() {
    bool allOk = true;

    static const char* SFX_FILES[static_cast<int>(SFX::COUNT)] = {
        "audio/shoot.wav",        "audio/hit_monster.wav",
        "audio/player_hit.wav",   "audio/level_up.wav",
        "audio/pickup_exp.wav",   "audio/click.wav",
        "audio/bgm_gameover.mp3",
    };

    for (int i = 0; i < static_cast<int>(SFX::COUNT); ++i) {
      if (!buffers_[i].loadFromFile(SFX_FILES[i])) {
        std::cerr << "[SoundManager] Missing SFX: " << SFX_FILES[i] << "\n";
        allOk = false;
        bufferOk_[i] = false;
      } else {
        bufferOk_[i] = true;
      }
    }

    // Khuếch đại âm lượng của TOÀN BỘ hiệu ứng âm thanh (SFX)
    for (int idx = 0; idx < static_cast<int>(SFX::COUNT); ++idx) {
      // Bỏ qua nhạc Game Over để không bị rè do khuếch đại âm lượng
      if (idx == static_cast<int>(SFX::GAMEOVER)) continue;

      if (bufferOk_[idx]) {
        const std::int16_t* samples = buffers_[idx].getSamples();
        std::size_t count = buffers_[idx].getSampleCount();
        unsigned int channelCount = buffers_[idx].getChannelCount();
        unsigned int sampleRate = buffers_[idx].getSampleRate();

        std::vector<std::int16_t> newSamples(count);
        for (std::size_t i = 0; i < count; ++i) {
          int amplified = static_cast<int>(samples[i]) * 3;
          if (amplified > 32767) amplified = 32767;
          if (amplified < -32768) amplified = -32768;
          newSamples[i] = static_cast<std::int16_t>(amplified);
        }

        std::vector<sf::SoundChannel> channelMap;
        if (channelCount == 1) {
          channelMap = {sf::SoundChannel::Mono};
        } else {
          channelMap = {sf::SoundChannel::FrontLeft,
                        sf::SoundChannel::FrontRight};
        }
        // FIX: handle [[nodiscard]] warning bằng cách gán vào biến
        [[maybe_unused]] bool ok = buffers_[idx].loadFromSamples(
            newSamples.data(), count, channelCount, sampleRate, channelMap);
      }
    }

    // ── FIX CHÍNH: SFML 3 — sf::Sound không có default constructor.
    // Dùng std::optional<sf::Sound> thay vì sf::Sound trực tiếp.
    // Khởi tạo pool SAU khi buffers_ đã load xong.
    soundPool_.clear();
    soundPool_.reserve(MAX_SOUNDS);
    // Tìm buffer đầu tiên hợp lệ để khởi tạo các Sound trong pool
    int firstOk = -1;
    for (int i = 0; i < static_cast<int>(SFX::COUNT); ++i) {
      if (bufferOk_[i]) {
        firstOk = i;
        break;
      }
    }
    for (int i = 0; i < MAX_SOUNDS; ++i) {
      if (firstOk >= 0) {
        // emplace sf::Sound với một buffer hợp lệ, sau đó stop ngay
        soundPool_.emplace_back(std::in_place, buffers_[firstOk]);
        soundPool_.back()->stop();
      } else {
        // Không có buffer nào load được — để optional rỗng
        soundPool_.emplace_back(std::nullopt);
      }
    }

    music_ = std::make_unique<sf::Music>();
    initialized_ = true;
    std::cout << "[SoundManager] Init xong. SFX ok=" << countOk() << "/"
              << static_cast<int>(SFX::COUNT) << "\n";
    return allOk;
  }

  // ── Phát SFX ─────────────────────────────────────────────
  void play(SFX sfx, float pitch = 1.0f) {
    if (!initialized_ || !sfxEnabled_ || muted_) return;
    int idx = static_cast<int>(sfx);
    if (!bufferOk_[idx]) return;

    sf::Sound* slot = findFreeSlot();
    if (!slot) return;

    slot->setBuffer(buffers_[idx]);
    slot->setVolume(sfxVolume_);

    // Nhạc Game Over sẽ giữ nguyên Pitch ban đầu, không thay đổi ngẫu nhiên
    if (sfx == SFX::GAMEOVER) {
      slot->setPitch(pitch);
    } else {
      slot->setPitch(pitch + pitchVariance());
    }

    slot->play();
  }

  void playVaried(SFX sfx) { play(sfx, 1.0f); }

  // ── tick(): GỌI MỖI FRAME từ Game::update() ──────────────
  void tick() {
    if (musicOpened_) {
      // Chỉ tự động đóng nhạc nếu nó ĐANG được bật (đáng ra phải đang phát)
      // nhưng status lại là Stopped (tức là đã phát hết bài).
      // Tránh reset nhầm khi ta chủ động chưa play() (vì đang tắt nhạc).
      if (music_->getStatus() == sf::Music::Status::Stopped && musicEnabled_ &&
          !muted_) {
        musicOpened_ = false;
        currentBgm_ = BGM::NONE;
      }
    }
  }

  // ── BGM ──────────────────────────────────────────────────
  void playMusic(BGM bgm, bool loop = true) {
    if (currentBgm_ == bgm && musicOpened_) return;

    if (musicOpened_) {
      music_->stop();
      musicOpened_ = false;
    }
    currentBgm_ = bgm;

    if (bgm == BGM::NONE) return;

    const char* file = bgmFile(bgm);
    music_ = std::make_unique<sf::Music>();
    if (!music_->openFromFile(file)) {
      std::cerr << "[SoundManager] Missing BGM: " << file << "\n";
      music_ = std::make_unique<sf::Music>();
      currentBgm_ = BGM::NONE;
      return;
    }
    musicOpened_ = true;
    music_->setLooping(loop);
    music_->setVolume(musicVolume_);
    if (musicEnabled_ && !muted_) {
      music_->play();
    }
  }

  void stopMusic() {
    if (musicOpened_) {
      music_->stop();
      musicOpened_ = false;
    }
    currentBgm_ = BGM::NONE;
  }

  void pauseMusic() {
    if (musicOpened_) music_->pause();
  }

  void resumeMusic() {
    if (musicOpened_ && musicEnabled_ && !muted_) music_->play();
  }

  void stopAll() {
    if (musicOpened_) {
      music_->stop();
      musicOpened_ = false;
    }
    currentBgm_ = BGM::NONE;
    for (auto& s : soundPool_) {
      if (s.has_value()) s->stop();
    }
    std::cout << "[SoundManager] Stopped all sounds and music.\n";
  }

  // ── Volume ───────────────────────────────────────────────
  void setSfxVolume(float v) { sfxVolume_ = std::max(0.f, std::min(100.f, v)); }

  void setMusicVolume(float v) {
    musicVolume_ = std::max(0.f, std::min(100.f, v));
    if (musicOpened_) music_->setVolume(musicVolume_);
  }

  float getSfxVolume() const { return sfxVolume_; }
  float getMusicVolume() const { return musicVolume_; }

  // ── Mute ─────────────────────────────────────────────────
  void setMuted(bool muted) {
    muted_ = muted;
    setSfxVolume(muted ? 0.f : savedSfxVol_);
    setMusicVolume(muted ? 0.f : savedMusicVol_);

    if (muted_) {
      for (auto& s : soundPool_) {
        if (s.has_value()) s->stop();
      }
      if (musicOpened_) music_->pause();
    } else {
      if (musicOpened_ && musicEnabled_ && currentBgm_ != BGM::NONE) {
        music_->play();
      }
    }
  }

  void toggleMute() {
    if (!muted_) {
      savedSfxVol_ = sfxVolume_;
      savedMusicVol_ = musicVolume_;
    }
    setMuted(!muted_);
  }

  bool isMuted() const { return muted_; }

  // ── SFX enable/disable ───────────────────────────────────
  bool isSfxEnabled() const { return sfxEnabled_; }

  void toggleSfx() {
    sfxEnabled_ = !sfxEnabled_;
    if (!sfxEnabled_) {
      for (auto& s : soundPool_) {
        if (s.has_value()) s->stop();
      }
    }
  }

  // ── Music enable/disable ─────────────────────────────────
  bool isMusicEnabled() const { return musicEnabled_; }

  void toggleMusic() {
    musicEnabled_ = !musicEnabled_;
    if (!musicOpened_) return;
    if (musicEnabled_) {
      music_->setVolume(musicVolume_);
      if (currentBgm_ != BGM::NONE && !muted_) music_->play();
    } else {
      music_->pause();
    }
  }

 private:
  SoundManager() = default;
  SoundManager(const SoundManager&) = delete;
  SoundManager& operator=(const SoundManager&) = delete;

  static constexpr int MAX_SOUNDS = 16;

  std::array<sf::SoundBuffer, static_cast<int>(SFX::COUNT)> buffers_;
  std::array<bool, static_cast<int>(SFX::COUNT)> bufferOk_ = {};

  // ── FIX: dùng optional vì sf::Sound không có default constructor trong SFML
  // 3
  std::vector<std::optional<sf::Sound>> soundPool_;

  std::unique_ptr<sf::Music> music_;
  BGM currentBgm_ = BGM::NONE;
  bool musicOpened_ = false;

  float sfxVolume_ = 100.f;
  float musicVolume_ = 100.f;
  float savedSfxVol_ = 100.f;
  float savedMusicVol_ = 100.f;
  bool muted_ = false;
  bool initialized_ = false;
  bool sfxEnabled_ = true;
  bool musicEnabled_ = true;

  sf::Sound* findFreeSlot() {
    for (auto& s : soundPool_) {
      if (s.has_value() && s->getStatus() == sf::Sound::Status::Stopped) {
        return &s.value();
      }
    }
    // Tất cả slot đang bận → cướp slot đầu tiên
    if (soundPool_[0].has_value()) {
      soundPool_[0]->stop();
      return &soundPool_[0].value();
    }
    return nullptr;
  }

  static float pitchVariance() {
    return (static_cast<float>(std::rand() % 11) - 5) * 0.01f;
  }

  int countOk() const {
    int n = 0;
    for (bool ok : bufferOk_) n += ok ? 1 : 0;
    return n;
  }

  static const char* bgmFile(BGM bgm) {
    switch (bgm) {
      case BGM::GAME_BGM:
        return "audio/bgm_game.ogg";
      case BGM::BOSS_BGM:
        return "audio/bgm_boss.ogg";
      default:
        return "";
    }
  }
};