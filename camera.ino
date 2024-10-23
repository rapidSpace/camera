#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

const char* ssid = "你的wifi名称";
const char* password = "你的wifi密码";

// Destination IP and port
const char* udpAddress = "Python 端的 IP"; // Python 端的 IP
const int udpPort = 12345;  // 目标端口

WiFiUDP udp;
int packetSize = 512; // 每个数据包的最大大小（字节）

void sendImageUDP(const uint8_t* buffer, size_t length) {
  size_t offset = 0;
  while (offset < length) {
    size_t chunkSize = min((size_t)packetSize, length - offset);
    udp.beginPacket(udpAddress, udpPort);
    udp.write(buffer + offset, chunkSize);
    if (udp.endPacket() <= 0) {
      Serial.println("Failed to send packet");
      return; // 发送失败时，退出函数
    }
    offset += chunkSize;
    delay(1);  // 每个片段发送后加一点延时，避免网络拥塞
  }

  // 发送结束标志
  uint8_t endFlag = 0xFF; // 结束标志，可以根据需要选择其他值
  udp.beginPacket(udpAddress, udpPort);
  udp.write(&endFlag, sizeof(endFlag));
  udp.endPacket();
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  // Camera configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QQVGA; // 设置较小的分辨率
  config.jpeg_quality = 30;  // 适当压缩图像质量
  config.fb_count = 1;

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Initialize UDP
  udp.begin(udpPort);
  Serial.println("UDP initialized");
}

void loop() {
  // Capture frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Send the frame buffer in chunks
  sendImageUDP(fb->buf, fb->len);

  // Return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);

  // Wait before sending the next frame
  delay(50);
}
