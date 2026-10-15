   /*
Pilot Hobbies Scout32 - For ESP32
Version 2.3 - Should compile with ESP32 Boards Library 1.0.6+

I recommend this new motor driver variation over the older ESC / Modified Servo code.
The motor driver approach is cheaper and less prone to drift and calibration errors.
Version 2.2 is still included for use with Servos and ESCs.

MX1508 Changes to Pinout:
GPIO12 - IN1 - Left FWD PWM
GPIO15 - IN3 - Right FWD PWM
GPIO13 - IN2 - Left REV
GPIO14 - IN4 - Right REV

Updates since 2.2
- Uses 2 less PWM channels, freeing up channels for servos and such.
- Replaced hard coded PWM / GPIO numbers with named variables for easy redefinitions.

ToDo: Make a centralized function for motor control. Will probably never happen.

Note about WROVER Boards:
You will need to change pin definitions for the camera to function.
This also requires moving LED indicator to GPIO2 instead of GPIO4 or else crash :(
There are probably more sane pinouts for the WROVER as well such as 
setting motor driver pins to use GPIO pairs 12,14 for forward and 32,33 for reverse.
Search code comments for "WROVER"

*/

// Define Network SSID & Password
// Set ap to 1 to use Scout32 as Standalone Access Point with default IP 192.168.4.1
// Set ap to 0 to connect to a router using DHCP with hostname espressif
bool ap = 1; 
const char* ssid = "Scout32"; //AP Name or Router SSID
const char* hostname = "Scout32"; //For access via Hostname
const char* password = ""; //Password. Leave blank for open network.

//AP Settings
int channel = 11; // Channel for AP Mode
int hidden = 0; // Probably leave at zero
int maxconnection = 1; // Only allow one at a time

// Libraries. If you get errors compiling, please downgrade ESP32 by Espressif.
// Use version 1.0.6 (Tools, Manage Libraries).
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Camera Pin Definitions - Define to suit your board

// ESP32-CAM (Standard)
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

/*
// ESP32-WROVER
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 21
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 19
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 5
#define Y2_GPIO_NUM 4
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
*/

// Webserver / Controls Function
void startCameraServer();

//Define Motor Driver GPIO Pins
//These definitions work on WROVER as-is
//WROVER GPIO 12,14 for forward and 32,33 for reverse may look neater
extern const int MotorLeftForward = 12; // LF GPIO Pin
extern const int MotorRightForward = 15; // RF GPIO Pin
extern const int MotorLeftReverse = 13; // LR GPIO Pin
extern const int MotorRightReverse = 14; // RR GPIO Pin

//Define LED GPIO Pin
extern const int LEDPIN = 4; // ESP32-CAM Integrated LED on GPIO4 - WROVER change to GPIO2

//PWM Channels
extern const int MotorLeftPWM = 2; // Left Motor PWM Channel
extern const int MotorRightPWM = 3; // Right Motor PWM Channel
extern const int LEDPWM = 4; // LED PWM Channel

void initMotor() // Here we setup PWM, and attach it to physical pins.
{
  ledcSetup(MotorLeftPWM, 8000, 16); // 8000 hz PWM, 16-bit resolution.
  ledcSetup(MotorRightPWM, 8000, 16); // 8000 hz PWM, 16-bit resolution.
  ledcSetup(LEDPWM, 5000, 8); // 5000 hz PWM, 8-bit resolution.
  ledcAttachPin(MotorLeftForward, MotorLeftPWM);
  ledcAttachPin(MotorRightForward, MotorRightPWM);
  ledcAttachPin(LEDPIN, LEDPWM);
  pinMode(MotorLeftReverse, OUTPUT);
  pinMode(MotorRightReverse, OUTPUT);
}

void setup() 
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Prevent brownouts by silencing them. You probably want to keep this.
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();


// Camera Configuration - Again, don't touch.
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
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  //drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 1);       // -2 to 2
  s->set_saturation(s, 1);     // -2 to 2
  s->set_gainceiling(s, (gainceiling_t)2);  // 0 to 6

  
  // Initialize Motor and LED Drivers
  initMotor();
  
 if(!ap){
  // Connect to Router
  Serial.println("ssid: " + (String)ssid);
  Serial.println("password: " + (String)password);
  Serial.println("WiFi is Client Scout32");
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
 } else {
  // Setup Access Point
  Serial.println("ssid: " + (String)ssid);
  Serial.println("password: " + (String)password);
  Serial.println("WiFi is Standalone Scout32");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid,password,channel,hidden,maxconnection);
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("' to connect");
  esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20); //Possible Fix for Interference??
}

  //Flash LED as ready indicator
  for (int i=0;i<5;i++) 
    {
      ledcWrite(LEDPWM,0);  // flash led
      delay(200);
      ledcWrite(LEDPWM,10);
      delay(200);    
    }       

  //Start Webserver
  startCameraServer();

}

void loop() {
  // Uncomment below for WiFi Signal Strength via Serial
  // Serial.printf("RSSi: %ld dBm\n",WiFi.RSSI()); 
}
