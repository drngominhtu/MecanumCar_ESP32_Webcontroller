#include <WiFi.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <driver/ledc.h>

// ================= MotorController Class =================
class MotorController {
  public:
    // Motor pins
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
    }

    void forward()    { 
        isMoving = true; 
        ledcWrite(MR1_ch, 0);   ledcWrite(MR2_ch, 255); ledcWrite(ML1_ch, 0);   ledcWrite(ML2_ch, 255); 
    }
    
    void backward()   { 
        isMoving = true;
        ledcWrite(MR1_ch, 255); ledcWrite(MR2_ch, 0);   ledcWrite(ML1_ch, 255); ledcWrite(ML2_ch, 0); 
    }
    
    void left()       { 
        isMoving = true;
        ledcWrite(MR1_ch, 0);   ledcWrite(MR2_ch, 155); ledcWrite(ML1_ch, 155); ledcWrite(ML2_ch, 0); 
    }
    
    void right()      { 
        isMoving = true;
        ledcWrite(MR1_ch, 155); ledcWrite(MR2_ch, 0);   ledcWrite(ML1_ch, 0);   ledcWrite(ML2_ch, 155); 
    }
    
    void stop() {
        isMoving = false;
        Serial.println("Stop function called");
        ledcWrite(MR1_ch, 0); ledcWrite(MR2_ch, 0);
        ledcWrite(ML1_ch, 0); ledcWrite(ML2_ch, 0);
    }
    
    // Hàm di chuyển theo hình vuông
    void moveSquare(int sideLength) {
        if (isMoving) {
            stop(); // Dừng nếu đang di chuyển
            return;
        }
        
        // Tính toán thời gian để đi được một cạnh với độ dài sideLength (cm)
        // Giả sử tốc độ trung bình của robot khoảng 20 cm/s
        int moveTimeMs = (sideLength * 50); // Thời gian di chuyển (ms)
        int turnTimeMs = 650; // Thời gian quay 90 độ (ms)
        
        // Đi theo 4 cạnh hình vuông
        for (int i = 0; i < 4; i++) {
            forward();
            delay(moveTimeMs);
            stop();
            delay(200);
            right(); // Quay phải 90 độ
            delay(turnTimeMs);
            stop();
            delay(200);
        }
    }
};

// ================== WebController Class ==================
class WebController {
  public:
    const char* ssid = "ESP32-Robot";
    const char* password = "12345678";
    WebServer server{80};
    MotorController& motor;

    WebController(MotorController& m) : motor(m) {}

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

        server.begin();
        Serial.println("HTTP server started");
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
    
    void handleSquare() {
        if (server.hasArg("size")) {
            int sideLength = server.arg("size").toInt();
            // Giới hạn kích thước để tránh quá lớn
            if (sideLength < 10) sideLength = 10;
            if (sideLength > 100) sideLength = 100;
            
            // Thực hiện di chuyển theo hình vuông trong một task riêng biệt
            // để không chặn xử lý web
            xTaskCreate(
                [](void* parameter) {
                    int size = *((int*)parameter);
                    delete (int*)parameter;
                    motor.moveSquare(size);
                    vTaskDelete(NULL);
                },
                "SquareTask",
                4096,
                new int(sideLength),
                1,
                NULL
            );
            
            server.send(200, "text/plain", "Moving in square with side length: " + 
                          String(sideLength) + " cm");
        } else {
            server.send(400, "text/plain", "Missing size parameter");
        }
    }
};

// ================== Global Objects ==================
MotorController motor;
WebController web(motor);

// ================== Arduino Setup/Loop ==================
void setup() {
    Serial.begin(115200);
    motor.setup();
    web.setup();
}

void loop() {
    web.handleClient();
}
