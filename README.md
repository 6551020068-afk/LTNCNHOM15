# Warrior Survivors

## Giới thiệu

**Warrior Survivors** là một tựa game sinh tồn tự động tấn công (roguelite time survival) lấy cảm hứng từ _Vampire Survivors_, được phát triển bằng ngôn ngữ **C++** và thư viện đồ họa **SFML**. Trong game, người chơi sẽ chọn một nhân vật để chiến đấu chống lại hàng ngàn quái vật, thu thập kinh nghiệm, nâng cấp kỹ năng và cố gắng sinh tồn trong 10 phút để đối mặt với trùm cuối.

## Tính năng nổi bật

- **4 Lớp nhân vật độc đáo:**
  - **Rogue:** Di chuyển cực nhanh, vũ khí khởi đầu: _Phong Dao (Knife)_.
  - **Mage:** Sát thương cao, vũ khí khởi đầu: _Sấm Sét (Lightning Ring)_.
  - **Druid:** Sinh lực dồi dào, vũ khí khởi đầu: _Vòng Tỏi (Garlic)_.
  - **Cleric:** Cân bằng công thủ, vũ khí khởi đầu: _Sách Thánh (Holy Bible)_.
- **Hệ thống Kỹ năng & Tiến hóa (Evolution):**
  - Nâng cấp các loại vũ khí và chỉ số (Sát thương, Tốc độ đánh, Hồi máu) khi lên cấp.
  - Khi kỹ năng đạt cấp độ tối đa (Max Level), kỹ năng có thể tiến hóa thành dạng tối thượng với sức mạnh vượt trội (Thousand Edge, Thunder Loop, Soul Eater, Unholy Vespers).
- **Hệ thống Wave & Sự kiện bản đồ:**
  - Game kéo dài 10 phút với độ khó tăng liên tục theo thời gian.
  - Quái vật đa dạng với nhiều hành vi khác nhau (FlyEye, Zombie, Slime, Ghost).
  - Cứ mỗi 2 phút sẽ xuất hiện một Mini Boss (Zombie King, Giant Eye, Slime Lord, Death Knight).
  - Trùm cuối Demon Lord xuất hiện ở phút thứ 10 với 3 giai đoạn (Phases) chiến đấu đầy thử thách.
- **Hai chế độ chơi:** Dễ (Easy) và Khó (Hard).
- **Hệ thống Lưu trữ (Save System):** Tự động lưu kỷ lục (High Score) và tiến trình sau mỗi vòng chơi.

## Yêu cầu hệ thống

- Trình biên dịch C++ hỗ trợ **C++17** trở lên.
- Thư viện đồ họa **SFML 3**.

## Cài đặt và Biên dịch

Bạn cần thiết lập dự án C++ và liên kết với các module của SFML bao gồm:

- `sfml-graphics`
- `sfml-window`
- `sfml-system`
- `sfml-audio`

Đảm bảo rằng thư mục `hinh anh` và `audio` nằm cùng cấp với file thực thi (`.exe`) để game có thể tải được toàn bộ tài nguyên.

## Hướng dẫn điều khiển

- **W, A, S, D** hoặc **Phím mũi tên**: Di chuyển nhân vật.
- **Chuột trái**: Tương tác với Menu, Chọn nâng cấp kỹ năng.
- **1, 2, 3**: Phím tắt để chọn kỹ năng nhanh khi bảng Level Up hiện ra.
- **ESC**: Tạm dừng game (Pause Menu) hoặc Quay lại.
- **M**: Quay về Main Menu.
- **R**: Chơi lại (khi Game Over hoặc Chiến thắng).

### Phím tắt Demo (Debug mode)

- **F1**: Bật/Tắt chế độ Debug (hiển thị Hitbox của nhân vật, quái vật và tầm đánh).
- **F2**: Triệu hồi Mini Boss FlyEye ngay tại vị trí người chơi.
- **F3**: Triệu hồi Mini Boss Ghost.
- **F4**: Triệu hồi Final Boss (Demon Lord).
- **F5 / F6**: Ép Demon Lord chuyển sang Phase 2 hoặc Phase 3.
- **F7**: Demo Map Event - Spawn quái theo hình Vòng tròn (Ring).
- **F8**: Demo Map Event - Spawn quái theo hình Chữ thập (Cross).
- **F9**: Demo Map Event - Spawn quái bao vây (Surround).
- **N**: Lên cấp ngay lập tức (Nhận EXP để mở bảng chọn kỹ năng).
- **L**: Kích hoạt trạng thái Chiến thắng (Victory) ngay lập tức.

## Cấu trúc lưu trữ (Save)

Điểm số và dữ liệu hệ thống được lưu ở dạng văn bản thuần túy trong file `savegame.dat`. Nếu file bị mất hoặc lỗi, hệ thống sẽ tự động khôi phục về trạng thái mặc định.
