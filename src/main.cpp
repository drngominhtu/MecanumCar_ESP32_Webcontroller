#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <driver/ledc.h>
#include <ESP32Servo.h>

// ================= UltrasonicController Class =================
class UltrasonicController {
  public:
    const int TRIG_PIN = 5;   // GPIO 5
    const int ECHO_PIN = 18;  // GPIO 18
    float distance = 0.0;
    unsigned long lastMeasurement = 0;
    const unsigned long MIN_MEASUREMENT_INTERVAL = 60; // Tối thiểu 60ms giữa các lần đo
    
    void setup() {
        Serial.printf("Setting up SR04: TRIG=%d, ECHO=%d\n", TRIG_PIN, ECHO_PIN);
        
        // Đặt pinMode rõ ràng
        pinMode(TRIG_PIN, OUTPUT);
        pinMode(ECHO_PIN, INPUT);
        
        // Đảm bảo trigger ở LOW ban đầu
        digitalWrite(TRIG_PIN, LOW);
        delay(500);
        
        Serial.println("Ultrasonic SR04 initialized: Trig=D5, Echo=D18");
        
        // Test đo khoảng cách sau khi khởi tạo
        delay(1000);
        Serial.println("=== Initial SR04 Test ===");
        float testDist = measureDistanceStable();
        Serial.printf("Initial test result: %.2f cm\n", testDist);
        Serial.println("========================\n");
    }
    
    // Hàm đo khoảng cách ổn định hơn
    float measureDistanceStable() {
        // Kiểm tra interval tối thiểu
        unsigned long currentTime = millis();
        if (currentTime - lastMeasurement < MIN_MEASUREMENT_INTERVAL) {
            return distance; // Trả về giá trị cũ nếu đo quá nhanh
        }
        
        Serial.printf("[SR04] Starting stable measurement at %lu ms\n", currentTime);
        
        // Đảm bảo TRIG ở LOW đủ lâu
        digitalWrite(TRIG_PIN, LOW);
        delayMicroseconds(5);
        
        // Gửi trigger pulse chuẩn 10μs
        digitalWrite(TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG_PIN, LOW);
        
        // Sử dụng pulseIn với timeout hợp lý
        long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout = ~5m max
        
        lastMeasurement = currentTime;
        
        if (duration == 0) {
            Serial.println("[SR04] ❌ No pulse detected");
            return -1;
        }
        
        // Tính khoảng cách với công thức chính xác
        // Speed of sound = 343 m/s = 0.0343 cm/μs
        float calculatedDistance = (duration * 0.0343) / 2;
        
        // Lọc nhiễu và giá trị bất thường
        if (calculatedDistance < 2) {
            Serial.printf("[SR04] ⚠️ Too close: %.2f cm, clamping to 2cm\n", calculatedDistance);
            calculatedDistance = 2;
        } else if (calculatedDistance > 400) {
            Serial.printf("[SR04] ⚠️ Too far: %.2f cm, clamping to 400cm\n", calculatedDistance);
            calculatedDistance = 400;
        }
        
        // Lọc nhiễu bằng cách so sánh với giá trị trước
        if (distance > 0) {
            float diff = abs(calculatedDistance - distance);
            if (diff > 100) { // Thay đổi quá lớn, có thể là nhiễu
                Serial.printf("[SR04] ⚠️ Large change detected: %.2f->%.2f cm, averaging\n", distance, calculatedDistance);
                calculatedDistance = (distance + calculatedDistance) / 2;
            }
        }
        
        distance = calculatedDistance;
        Serial.printf("[SR04] ✅ Stable distance: %.2f cm (duration: %ld μs)\n", distance, duration);
        
        return distance;
    }
    
    // Hàm đo với multiple samples để tăng độ chính xác
    float measureDistanceAverage() {
        const int samples = 3;
        float validReadings[samples];
        int validCount = 0;
        
        Serial.println("[SR04] Taking multiple samples...");
        
        for (int i = 0; i < samples; i++) {
            delay(50); // Delay giữa các lần đo
            
            digitalWrite(TRIG_PIN, LOW);
            delayMicroseconds(5);
            digitalWrite(TRIG_PIN, HIGH);
            delayMicroseconds(10);
            digitalWrite(TRIG_PIN, LOW);
            
            long duration = pulseIn(ECHO_PIN, HIGH, 30000);
            
            if (duration > 0) {
                float dist = (duration * 0.0343) / 2;
                if (dist >= 2 && dist <= 400) {
                    validReadings[validCount] = dist;
                    validCount++;
                    Serial.printf("[SR04] Sample %d: %.2f cm\n", i+1, dist);
                }
            }
        }
        
        if (validCount == 0) {
            Serial.println("[SR04] ❌ No valid readings");
            return -1;
        }
        
        // Tính trung bình
        float sum = 0;
        for (int i = 0; i < validCount; i++) {
            sum += validReadings[i];
        }
        float average = sum / validCount;
        
        distance = average;
        Serial.printf("[SR04] ✅ Average of %d samples: %.2f cm\n", validCount, average);
        
        return average;
    }
    
    // Wrapper function cho compatibility
    float measureDistance() {
        return measureDistanceStable();
    }
    
    // Test với fake data để kiểm tra web interface
    float getFakeDistance() {
        static float fakeVal = 10.0;
        static bool increasing = true;
        
        if (increasing) {
            fakeVal += 3.0;
            if (fakeVal >= 80) increasing = false;
        } else {
            fakeVal -= 3.0;
            if (fakeVal <= 10) increasing = true;
        }
        
        return fakeVal;
    }
    
    String getDistanceJSON() {
        // Thử đo thật trước
        float dist = measureDistanceStable();
        
        // Nếu lỗi, dùng fake data để test web
        if (dist < 0) {
            Serial.println("[SR04] Using fake data for web test");
            dist = getFakeDistance();
        }
        
        String status = (dist > 0) ? "ok" : "error";
        
        String json = "{";
        json += "\"distance\":" + String(dist, 1) + ",";
        json += "\"unit\":\"cm\",";
        json += "\"status\":\"" + status + "\",";
        json += "\"timestamp\":" + String(millis());
        json += "}";
        return json;
    }
    
    void continuousMeasurement() {
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 2000) { // 2 giây một lần
            Serial.println("\n--- Continuous SR04 Test ---");
            float dist = measureDistanceStable();
            Serial.printf("Continuous result: %.2f cm\n", dist);
            Serial.println("-----------------------------\n");
            lastUpdate = millis();
        }
    }
    
    // Diagnostic function
    void diagnostics() {
        Serial.println("\n=== SR04 Diagnostics ===");
        
        // Test TRIG pin
        pinMode(TRIG_PIN, OUTPUT);
        digitalWrite(TRIG_PIN, LOW);
        delay(10);
        int trigLow = digitalRead(TRIG_PIN);
        digitalWrite(TRIG_PIN, HIGH);
        delay(10);
        int trigHigh = digitalRead(TRIG_PIN);
        digitalWrite(TRIG_PIN, LOW);
        
        Serial.printf("TRIG pin test: LOW=%d, HIGH=%d\n", trigLow, trigHigh);
        
        // Test ECHO pin
        pinMode(ECHO_PIN, INPUT);
        int echoState = digitalRead(ECHO_PIN);
        Serial.printf("ECHO pin state: %d\n", echoState);
        
        // Test multiple measurements
        Serial.println("Testing 5 consecutive measurements:");
        for (int i = 0; i < 5; i++) {
            delay(100);
            float dist = measureDistanceStable();
            Serial.printf("Test %d: %.2f cm\n", i+1, dist);
        }
        
        Serial.println("========================\n");
    }
};

// ================= ServoController Class =================
class ServoController {
  public:
    const int SERVO_PIN = 25;
    const int SERVO_CHANNEL = 8;
    Servo myServo;
    int currentAngle = 0;
    bool isAutoMode = false;
    bool direction = true;
    bool isRadarMode = false;
    
    void setup() {
        myServo.attach(SERVO_PIN, 500, 2400);
        myServo.write(0);
        currentAngle = 0;
        Serial.println("Servo initialized at pin 25");
        
        // Test servo
        Serial.println("Testing servo movement...");
        delay(1000);
        myServo.write(90);
        delay(1000);
        myServo.write(0);
        Serial.println("Servo test complete");
    }
    
    void setAngle(int angle) {
        if (angle < 0) angle = 0;
        if (angle > 180) angle = 180;
        myServo.write(angle);
        currentAngle = angle;
        Serial.printf("Servo moved to %d degrees\n", angle);
    }
    
    void startAutoRotation() {
        isAutoMode = true;
        isRadarMode = false;
        direction = true;
        Serial.println("Starting auto rotation 0-180-0...");
    }
    
    void startRadarMode() {
        isRadarMode = true;
        isAutoMode = false;
        direction = true;
        currentAngle = 0;
        myServo.write(0);
        Serial.println("Starting radar mode...");
    }
    
    void stopAutoRotation() {
        isAutoMode = false;
        isRadarMode = false;
        Serial.println("Stopped auto rotation");
    }
    
    void updateAutoRotation() {
        if (!isAutoMode && !isRadarMode) return;
        
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate < 50) return;
        lastUpdate = millis();
        
        if (direction) {
            currentAngle += 2; // Giảm tốc độ xuống 2 độ
            if (currentAngle >= 180) {
                currentAngle = 180;
                direction = false;
                if (isRadarMode) Serial.println("Radar: Reached 180°, changing direction");
            }
        } else {
            currentAngle -= 2;
            if (currentAngle <= 0) {
                currentAngle = 0;
                direction = true;
                if (isRadarMode) Serial.println("Radar: Reached 0°, changing direction");
            }
        }
        
        myServo.write(currentAngle);
        
        // Debug servo position khi ở chế độ radar
        if (isRadarMode) {
            static int lastPrintedAngle = -1;
            if (currentAngle != lastPrintedAngle && currentAngle % 10 == 0) {
                Serial.printf("Radar sweep: %d°\n", currentAngle);
                lastPrintedAngle = currentAngle;
            }
        }
    }
};

// ================= MotorController Class =================
class MotorController {
  public:
    const int MR1 = 14, MR2 = 12, ML1 = 26, ML2 = 27;
    const int MR1_ch = 0, MR2_ch = 1, ML1_ch = 2, ML2_ch = 3;
    bool isMoving = false;

    void setup() {
        pinMode(MR1, OUTPUT); pinMode(MR2, OUTPUT);
        pinMode(ML1, OUTPUT); pinMode(ML2, OUTPUT);
        ledcAttachPin(MR1, MR1_ch); ledcAttachPin(MR2, MR2_ch);
        ledcAttachPin(ML1, ML1_ch); ledcAttachPin(ML2, ML2_ch);
        ledcSetup(MR1_ch, 2000, 8); ledcSetup(MR2_ch, 2000, 8);
        ledcSetup(ML1_ch, 2000, 8); ledcSetup(ML2_ch, 2000, 8);
        stop();
        Serial.println("Motor controller initialized");
    }

    void forward() { 
        isMoving = true; 
        ledcWrite(MR1_ch, 0); ledcWrite(MR2_ch, 255); 
        ledcWrite(ML1_ch, 0); ledcWrite(ML2_ch, 255); 
        Serial.println("Motor: Forward");
    }
    
    void backward() { 
        isMoving = true;
        ledcWrite(MR1_ch, 255); ledcWrite(MR2_ch, 0); 
        ledcWrite(ML1_ch, 255); ledcWrite(ML2_ch, 0); 
        Serial.println("Motor: Backward");
    }
    
    void left() { 
        isMoving = true;
        ledcWrite(MR1_ch, 0); ledcWrite(MR2_ch, 155); 
        ledcWrite(ML1_ch, 155); ledcWrite(ML2_ch, 0); 
        Serial.println("Motor: Left");
    }
    
    void right() { 
        isMoving = true;
        ledcWrite(MR1_ch, 155); ledcWrite(MR2_ch, 0); 
        ledcWrite(ML1_ch, 0); ledcWrite(ML2_ch, 155); 
        Serial.println("Motor: Right");
    }
    
    void stop() {
        isMoving = false;
        ledcWrite(MR1_ch, 0); ledcWrite(MR2_ch, 0);
        ledcWrite(ML1_ch, 0); ledcWrite(ML2_ch, 0);
        Serial.println("Motor: Stop");
    }
    
    void moveSquare(int sideLength) {
        if (isMoving) { stop(); return; }
        
        Serial.printf("Starting square movement with side length: %d cm\n", sideLength);
        
        int moveTimeMs = (sideLength * 50);
        int turnTimeMs = 650;
        
        for (int i = 0; i < 4; i++) {
            Serial.printf("Square side %d/4\n", i+1);
            forward(); delay(moveTimeMs); stop(); delay(200);
            right(); delay(turnTimeMs); stop(); delay(200);
        }
        
        Serial.println("Square movement completed");
    }
};

// ================== WebController Class ==================
class WebController {
  public:
    const char* ssid = "ESP32-Robot";
    const char* password = "12345678";
    WebServer server;
    MotorController& motor;
    ServoController& servo;
    UltrasonicController& ultrasonic;

    WebController(MotorController& m, ServoController& s, UltrasonicController& u) 
        : server(80), motor(m), servo(s), ultrasonic(u) {}

    void setup() {
        if (!SPIFFS.begin(true)) {
            Serial.println("An Error has occurred while mounting SPIFFS");
            return;
        }
        WiFi.softAP(ssid, password);
        Serial.println("Access Point Started");
        Serial.print("IP address: ");
        Serial.println(WiFi.softAPIP());

        server.on("/", [this]() { handleFile("/index.html", "text/html"); });
        server.on("/style.css", [this]() { handleFile("/style.css", "text/css"); });
        server.on("/script.js", [this]() { handleFile("/script.js", "application/javascript"); });
        server.on("/cmd", [this]() { handleCmd(); });
        server.on("/square", [this]() { handleSquare(); });
        server.on("/servo", [this]() { handleServo(); });
        server.on("/radar", [this]() { handleRadar(); });
        server.on("/radar-data", [this]() { handleRadarData(); });
        server.on("/test-sr04", [this]() { handleTestSR04(); });
        server.on("/distance", [this]() { handleDistance(); });

        server.begin();
        Serial.println("HTTP server started");
        Serial.println("Available endpoints:");
        Serial.println("  GET /test-sr04 - Test SR04 sensor");
        Serial.println("  GET /distance - Get current distance");
        Serial.println("  GET /radar-data - Get radar data");
    }

    void handleClient() {
        server.handleClient();
    }

  private:
    void handleFile(const char* path, const char* type) {
        File file = SPIFFS.open(path, "r");
        if (!file) {
            server.send(404, "text/plain", "File Not Found");
            return;
        }
        server.streamFile(file, type);
        file.close();
    }

    void handleCmd() {
        if (server.hasArg("val")) {
            char cmd = server.arg("val")[0];
            switch (cmd) {
                case 'F': motor.forward(); break;
                case 'G': motor.backward(); break;
                case 'L': motor.left(); break;
                case 'R': motor.right(); break;
                case 'S': motor.stop(); break;
            }
            server.send(200, "text/plain", "OK");
        }
    }
    
    void handleServo() {
        if (server.hasArg("angle")) {
            int angle = server.arg("angle").toInt();
            servo.setAngle(angle);
            server.send(200, "text/plain", "Servo moved to " + String(angle) + " degrees");
        } else if (server.hasArg("auto")) {
            String autoMode = server.arg("auto");
            if (autoMode == "start") {
                servo.startAutoRotation();
                server.send(200, "text/plain", "Auto rotation started");
            } else if (autoMode == "stop") {
                servo.stopAutoRotation();
                server.send(200, "text/plain", "Auto rotation stopped");
            }
        } else {
            server.send(400, "text/plain", "Missing parameter");
        }
    }
    
    void handleRadar() {
        if (server.hasArg("mode")) {
            String mode = server.arg("mode");
            if (mode == "start") {
                servo.startRadarMode(); // Auto sweep 0->180->0
                server.send(200, "text/plain", "Radar sweep started");
            } else if (mode == "stop") {
                servo.stopAutoRotation(); // Stop sweep
                servo.setAngle(90); // Move to center
                server.send(200, "text/plain", "Radar stopped, servo centered");
            }
        } else {
            server.send(400, "text/plain", "Missing mode parameter");
        }
    }
    
    void handleRadarData() {
        Serial.println("[API] Radar data requested");
        
        // Sử dụng measurement ổn định
        float distance = ultrasonic.measureDistanceStable();
        
        // Nếu cảm biến lỗi, dùng fake data
        if (distance < 0) {
            distance = ultrasonic.getFakeDistance();
            Serial.println("[API] Using fake data for radar");
        }
        
        int angle = servo.currentAngle;
        
        String json = "{";
        json += "\"angle\":" + String(angle) + ",";
        json += "\"distance\":" + String(distance, 1) + ",";
        json += "\"timestamp\":" + String(millis()) + ",";
        json += "\"status\":\"" + String(distance > 0 ? "ok" : "error") + "\"";
        json += "}";
        
        Serial.printf("[API] Radar data: angle=%d°, distance=%.1f cm\n", angle, distance);
        server.send(200, "application/json", json);
    }
    
    void handleDistance() {
        Serial.println("[API] Distance data requested");
        String json = ultrasonic.getDistanceJSON();
        Serial.printf("[API] Returning distance JSON: %s\n", json.c_str());
        server.send(200, "application/json", json);
    }
    
    void handleSquare() {
        if (server.hasArg("size")) {
            int sideLength = server.arg("size").toInt();
            if (sideLength < 10) sideLength = 10;
            if (sideLength > 100) sideLength = 100;
            
            struct TaskParams {
                int size;
                MotorController* motorPtr;
            };
            
            TaskParams* params = new TaskParams{sideLength, &motor};
            
            xTaskCreate(
                [](void* parameter) {
                    TaskParams* params = static_cast<TaskParams*>(parameter);
                    params->motorPtr->moveSquare(params->size);
                    delete params;
                    vTaskDelete(NULL);
                },
                "SquareTask",
                4096,
                params,
                1,
                NULL
            );
            
            server.send(200, "text/plain", "Moving in square with side length: " + 
                          String(sideLength) + " cm");
        } else {
            server.send(400, "text/plain", "Missing size parameter");
        }
    }
    
    void handleTestSR04() {
        Serial.println("[API] Comprehensive SR04 test requested");
        
        String response = "=== SR04 Comprehensive Test ===\n";
        
        // Chạy diagnostics
        ultrasonic.diagnostics();
        
        // Test stable measurement
        response += "--- Stable Measurement ---\n";
        float stableDist = ultrasonic.measureDistanceStable();
        response += "Stable method: " + String(stableDist, 2) + " cm\n";
        
        // Test average measurement
        response += "--- Average Measurement ---\n";
        float avgDist = ultrasonic.measureDistanceAverage();
        response += "Average method: " + String(avgDist, 2) + " cm\n";
        
        // Test multiple quick measurements
        response += "--- Quick Tests (should show caching) ---\n";
        for (int i = 0; i < 3; i++) {
            float quickDist = ultrasonic.measureDistanceStable();
            response += "Quick " + String(i+1) + ": " + String(quickDist, 1) + " cm\n";
            delay(10); // Deliberately too fast
        }
        
        // Hardware info
        response += "--- Hardware Info ---\n";
        response += "TRIG: GPIO " + String(ultrasonic.TRIG_PIN) + "\n";
        response += "ECHO: GPIO " + String(ultrasonic.ECHO_PIN) + "\n";
        response += "Min interval: 60ms\n";
        response += "Max range: 400cm\n";
        response += "Min range: 2cm\n";
        
        response += "========================";
        
        server.send(200, "text/plain", response);
    }
};

// ================== Global Objects ==================
MotorController motor;
ServoController servo;
UltrasonicController ultrasonic;
WebController web(motor, servo, ultrasonic);

// ================== Arduino Setup/Loop ==================
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== ESP32 Robot Car Starting ===");
    
    // Khởi tạo từng module
    Serial.println("Initializing Motor Controller...");
    motor.setup();
    
    Serial.println("Initializing Servo Controller...");
    servo.setup();
    
    Serial.println("Initializing Ultrasonic Sensor...");
    ultrasonic.setup();
    
    Serial.println("Initializing Web Server...");
    web.setup();
    
    Serial.println("=== Setup Complete ===\n");
    
    // Test toàn bộ hệ thống - sửa lại tên hàm
    Serial.println("Running system tests...");
    ultrasonic.diagnostics(); // Đổi từ testConnection() thành diagnostics()
    
    Serial.println("System ready! Monitoring started...\n");
}

void loop() {
    // Xử lý web requests
    web.handleClient();
    
    // Cập nhật servo (auto rotation/radar)
    servo.updateAutoRotation();
    
    // Đo khoảng cách liên tục với interval phù hợp
    ultrasonic.continuousMeasurement();
    
    // System monitoring với interval dài hơn
    static unsigned long lastSystemCheck = 0;
    if (millis() - lastSystemCheck > 60000) { // Mỗi 60 giây
        Serial.printf("\n[SYSTEM] Uptime: %lu ms\n", millis());
        Serial.printf("[SYSTEM] Free heap: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("[SYSTEM] WiFi clients: %d\n", WiFi.softAPgetStationNum());
        
        // Chạy diagnostics định kỳ
        if (millis() > 300000) { // Sau 5 phút
            ultrasonic.diagnostics();
        }
        
        lastSystemCheck = millis();
    }
    
    // Nhỏ delay để tránh watchdog timeout
    delay(1);
}
