#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"

const char* ssid = "Wifi_SSID";
const char* password = "Wifi_Password";

const char* serverName = "192.168.0.22";  // Cambia la dirección IP según tu configuración
const int serverPort = 5001;              // Puerto del servidor
const char* uploadEndpoint = "/upload";  

const int captureInterval = 900000; // Intervalo de captura en milisegundos (5 s)

// Configuración de la cámara
// Cambia los pines según la configuración de tu ESP32-CAM
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());

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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }
}

void loop() {
  static unsigned long previousCaptureMillis = 0;

  unsigned long currentMillis = millis();
  if (currentMillis - previousCaptureMillis >= captureInterval) {
    previousCaptureMillis = currentMillis;
    captureAndSendPhoto();
  }

  // Otro código del bucle si lo tienes
}

void captureAndSendPhoto() {
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  WiFiClient client;

  Serial.println("Connecting to server...");
  if (client.connect(serverName, serverPort)) {
    Serial.println("Connection successful!");

    String boundary = "----WebKitFormBoundary" + String(millis());
    
    String formData = "--" + boundary + "\r\n" +
                    "Content-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\n" +
                    "Content-Type: image/jpeg\r\n\r\n";
                      
    String formDataEnd = "\r\n--" + boundary + "--\r\n";

    size_t imageLen = fb->len;
    size_t totalLen = formData.length() + imageLen + formDataEnd.length();

    client.println("POST " + String(uploadEndpoint) + " HTTP/1.1");
    client.println("Host: " + String(serverName));
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.println();

    client.print(formData);

    for (size_t i = 0; i < imageLen; i++) {
      client.write(fb->buf[i]);
    }

    client.print(formDataEnd);
    delay(10);

    Serial.println("Image uploaded successfully");

    client.stop();
  } else {
    Serial.println("Connection to server failed");
  }

  esp_camera_fb_return(fb);
}