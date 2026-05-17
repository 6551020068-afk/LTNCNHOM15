#pragma once
// ════════════════════════════════════════════════════════════
//  CharacterClass.hpp  —  Định nghĩa 4 class nhân vật
//
//  Rogue   : Dao (Knife)          — tốc độ cao
//  Mage    : Sấm (Lightning)      — damage cao
//  Druid   : Tỏi (Garlic)         — HP cao
//  Cleric  : Sách Thánh (Bible)   — cân bằng, orbit damage
//
//  MenuSystem đọc DEFS[] để hiển thị card chọn nhân vật.
//  Game::applyCharacterClass() áp stat + skill khởi đầu.
// ════════════════════════════════════════════════════════════
#include <SFML/Graphics.hpp>
#include <string>

// ── Loại skill khởi đầu ──────────────────────────────────────
enum class StartingSkill { Knife, Lightning, Garlic, HolyBible };

// ── Dữ liệu 1 class nhân vật ─────────────────────────────────
struct CharClassDef {
  std::string name;            // Tên hiển thị
  std::string icon;            // Emoji / ký tự icon
  std::string iconPath;        // Đường dẫn ảnh icon nhân vật
  std::string weaponIconPath;  // Đường dẫn ảnh icon vũ khí khởi đầu
  std::string weaponName;      // Tên vũ khí khởi đầu
  std::string description;     // Mô tả ngắn
  std::string statLine;        // Dòng stat hiển thị trong card
  sf::Color color;             // Màu chủ đạo của class

  StartingSkill startSkill;  // Skill được trang bị ngay từ đầu

  // Stat bonus so với base (PlayerStats)
  int bonusHp;       // +HP tối đa
  int bonusDamage;   // +Damage
  float bonusSpeed;  // +Speed (pixels/s)

  // ── Đường dẫn tới spritesheet animation ────────────────────
  std::string spriteSheetPath;
};

// ── Struct tương thích với MenuSystem cũ ─────────────────────
using CharInfo = CharClassDef;

// ════════════════════════════════════════════════════════════
//  DEFS — 4 class, index khớp với selectedChar_
//    0 = Rogue   (Knife)
//    1 = Mage    (Lightning)
//    2 = Druid   (Garlic)
//    3 = Cleric  (HolyBible)
// ════════════════════════════════════════════════════════════
namespace CharacterClass {

// Số lượng class — dùng thay vì hardcode "3" hay "4" ở mọi nơi
inline constexpr int CLASS_COUNT = 4;

inline const CharClassDef DEFS[CLASS_COUNT] = {

    // ── 0: Rogue ─────────────────────────────────────────
    {
        "Rogue",
        "",
        "hinh anh\\icon_rogue.png",
        "hinh anh\\icon_knife.png",
        "    Phong Dao",
        "Sat thu nhanh nhen.\nPhong dao xuyen qua ke thu,\nvut vao bong toi.",
        " HP 98    DMG 1    SPD +30",
        sf::Color(220, 180, 60),  // vàng đồng
        StartingSkill::Knife,
        /*bonusHp=*/-2,
        /*bonusDamage=*/0,
        /*bonusSpeed=*/30.f,
        /*spriteSheetPath=*/"hinh anh\\roue_animation.png",
        // animation của
        // Rogue
    },

    // ── 1: Mage ──────────────────────────────────────────
    {
        "Mage",
        "",
        "hinh anh\\icon_mage.png",
        "hinh anh\\icon_lightning.png",
        "  Tia Set",
        "Phap su thieu dot.\nTia set danh bai ke dich\ntrong mot no phap.",
        " HP 100   DMG 2    SPD  0",
        sf::Color(100, 160, 255),  // xanh điện
        StartingSkill::Lightning,
        /*bonusHp=*/0,
        /*bonusDamage=*/1,
        /*bonusSpeed=*/0.f,
        /*spriteSheetPath=*/"hinh anh\\spritesheet_Porta_Ladonna.png",
    },

    // ── 2: Druid ─────────────────────────────────────────
    {
        "Druid",
        "",
        "hinh anh\\icon_druid.png",
        "hinh anh\\icon_Garlic.png",
        "   Vong Toi",
        "Phap su tu nhien.\nMui toi quet sach quan thu\nxung quanh nguoi.",
        " HP 105   DMG 1    SPD -20",
        sf::Color(100, 220, 100),  // xanh lá
        StartingSkill::Garlic,
        /*bonusHp=*/5,
        /*bonusDamage=*/0,
        /*bonusSpeed=*/-20.f,
        /*spriteSheetPath=*/"hinh anh\\spritesheet_Poe_Ratcho.png",
    },

    // ── 3: Cleric ────────────────────────────────────────
    {
        "Cleric",
        "",
        "hinh anh\\icon_cleric.png",
        "hinh anh\\icon_bible.png",
        "    Sach Thanh",
        "Thanh chien duc tin.\nSach thanh bay vong quanh,\nbao ve linh hon.",
        " HP 102   DMG 1    SPD  0",
        sf::Color(200, 170, 255),  // tím thánh
        StartingSkill::HolyBible,
        /*bonusHp=*/2,
        /*bonusDamage=*/0,
        /*bonusSpeed=*/0.f,
        /*spriteSheetPath=*/"hinh anh\\spritesheet_Dommario.png",
    },
};

}  // namespace CharacterClass