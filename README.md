<img src="https://github.com/drngominhtu/MecanumCar_ESP32_Webcontroller/blob/main/web_mecanumcontrol.PNG" alt="Minh hoạ UI" width="400"/>

# MecanumCar_ESP32_Webcontroller

## Giới thiệu

Dự án **MecanumCar_ESP32_Webcontroller** giúp bạn điều khiển một xe robot sử dụng bánh Mecanum dựa trên vi điều khiển ESP32 thông qua giao diện web. Xe có thể di chuyển linh hoạt (tiến, lùi, trái, phải, xoay, di chuyển hình vuông), điều khiển servo, đo khoảng cách bằng cảm biến siêu âm (SR04), và hiển thị dữ liệu radar thời gian thực.

## Tính năng chính

- **Điều khiển xe qua giao diện web**: Kết nối WiFi Access Point từ ESP32, truy cập web UI, gửi lệnh di chuyển (F, B, L, R, S) hoặc di chuyển hình vuông với độ dài cạnh tùy chỉnh.
- **Điều khiển servo**: Điều chỉnh góc servo thủ công hoặc tự động quét (auto, radar).
- **Đo khoảng cách với cảm biến siêu âm**: Xem giá trị đo trực tiếp, kiểm tra/diagnostics cảm biến SR04.
- **Chế độ radar**: Hiển thị dữ liệu radar trực quan trên canvas, quét môi trường xung quanh robot.
- **API REST**: Các endpoint như `/cmd`, `/square`, `/servo`, `/radar`, `/radar-data`, `/distance`, `/test-sr04` phục vụ điều khiển và truy cập dữ liệu.

## Yêu cầu phần cứng

- ESP32 Dev Module
- Xe sử dụng động cơ bánh Mecanum (4 bánh)
- Driver động cơ (L298N hoặc tương đương)
- Servo (SG90 hoặc tương tự)
- Cảm biến siêu âm HC-SR04
- Nguồn cấp phù hợp cho động cơ và ESP32

## Cài đặt & Sử dụng

1. **Cài đặt các thư viện Arduino cần thiết** (Servo, ESPAsyncWebServer, SPIFFS, v.v).
2. **Nạp code lên ESP32**.
3. **Kết nối thiết bị của bạn vào WiFi Access Point** với tên `ESP32-Robot`, mật khẩu `12345678`.
4. **Truy cập trình duyệt web** tại địa chỉ IP mà ESP32 cung cấp (thường là `192.168.4.1`).
5. **Sử dụng giao diện web** để điều khiển xe, servo, radar, theo dõi giá trị cảm biến, v.v.

## Giao diện web

- **Điều khiển chuyển động cơ bản và nâng cao**: Nút tiến/lùi/trái/phải/dừng, di chuyển hình vuông với thanh kéo chỉnh độ dài cạnh.
- **Điều khiển servo**: Chọn góc hoặc bật chế độ auto/radar.
- **Radar**: Hiển thị trực quan góc quét và khoảng cách vật cản.
- **Xem số liệu cảm biến**: Dữ liệu khoảng cách thực tế, diagnostics, thông tin phần cứng cảm biến.

## Một số endpoint API

- `GET /cmd?val=F|B|L|R|S`: Điều khiển cơ bản (forward, backward, left, right, stop)
- `GET /square?size=30`: Di chuyển hình vuông với cạnh 30cm
- `GET /servo?angle=90`: Xoay servo đến góc 90°
- `GET /radar`: Bắt đầu quét radar
- `GET /radar-data`: Lấy dữ liệu radar (JSON)
- `GET /distance`: Lấy giá trị đo khoảng cách (JSON)
- `GET /test-sr04`: Diagnostic cảm biến SR04

## Đóng góp

Chào mừng mọi đóng góp, chỉnh sửa, hoặc câu hỏi! Hãy tạo issue hoặc pull request trên GitHub.

## Giấy phép

MIT License

---

**Tác giả:** drngominhtu
