/*
 * =======================================================================
 *                             CONFIG FILE
 * =======================================================================
 * All user-configurable settings are located here.
 * =======================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

// =======================================================================
//                           1. CHOOSE WIFI MODE
// =======================================================================
// --- Uncomment (เอา // ออก) แค่ 1 บรรทัด เพื่อเลือกโหมดที่ต้องการ ---
//#define WIFI_MODE   MODE_AP       // โหมดปล่อย Wi-Fi ของตัวเอง (Access Point)
#define WIFI_MODE   MODE_STA      // โหมดเชื่อมต่อกับ Wi-Fi อื่น (Station)
//#define WIFI_MODE   MODE_AP_STA   // โหมดผสม (AP + Station)

// --- กำหนดค่าสำหรับแต่ละโหมด ---
#define MODE_STA    1
#define MODE_AP     2
#define MODE_AP_STA 3
// =======================================================================


// =======================================================================
//                       2. CONFIGURE CREDENTIALS
// =======================================================================
// --- Credentials สำหรับโหมด Station (STA) ---
const char* ssid = "LEOLEOLEO";         // <--- ใส่ชื่อ Wi-Fi ของคุณ
const char* password = "11111111"; // <--- ใส่รหัสผ่าน Wi-Fi ของคุณ

// --- Credentials สำหรับโหมด Access Point (AP) ---
const char* ap_ssid = "ESP8266-Logger";     // ชื่อ Wi-Fi ที่จะให้ ESP ปล่อย
const char* ap_password = "password123";    // รหัสผ่าน (ต้องมี 8 ตัวอักษรขึ้นไป)
// =======================================================================


// =======================================================================
//                       3. OTHER CONFIGURATIONS
// =======================================================================
const int chipSelect = 4;             // SD Card Chip Select pin
const long interval = 1000;           // Log data interval (milliseconds)
const long wifiCheckInterval = 10000; // WiFi check interval (milliseconds)
// =======================================================================


// =======================================================================
//                          GLOBAL VARIABLES
// =======================================================================
// --- Web Server Instance ---
AsyncWebServer server(80);

// --- Logging State ---
enum LoggingState { IDLE, LOGGING, PAUSED };
LoggingState currentState = IDLE;
File logFile;
String currentLogFileName = "";
unsigned long previousMillis = 0;

// --- WiFi Check Timer ---
unsigned long previousWiFiCheckMillis = 0;

// --- Memory Check ---
uint32_t lastFreeHeap = 0;

#endif // CONFIG_H
