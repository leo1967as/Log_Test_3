#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

// =======================================================================
//                           1. CHOOSE WIFI MODE
// =======================================================================
// --- Uncomment (เอา // ออก) แค่ 1 บรรทัด เพื่อเลือกโหมดที่ต้องการ ---
//#define WIFI_MODE   MODE_AP       // โหมดปล่อย Wi-Fi ของตัวเอง (Access Point)
//#define WIFI_MODE   MODE_STA      // โหมดเชื่อมต่อกับ Wi-Fi อื่น (Station)
#define WIFI_MODE   MODE_AP_STA   // โหมดผสม (AP + Station)

// --- กำหนดค่าสำหรับแต่ละโหมด ---
#define MODE_STA    1
#define MODE_AP     2
#define MODE_AP_STA 3
// =======================================================================


// =======================================================================
//                        2. CONFIGURE CREDENTIALS
// =======================================================================
// --- Credentials สำหรับโหมด Station (STA) ---
const char* ssid = "LEOLEOLEO";         // <--- ใส่ชื่อ Wi-Fi ของคุณ
const char* password = "11111111"; // <--- ใส่รหัสผ่าน Wi-Fi ของคุณ

// --- Credentials สำหรับโหมด Access Point (AP) ---
const char* ap_ssid = "ESP8266-Logger";     // ชื่อ Wi-Fi ที่จะให้ ESP ปล่อย
const char* ap_password = "password123";    // รหัสผ่าน (ต้องมี 8 ตัวอักษรขึ้นไป)
// =======================================================================


// --- Global Variables ---
uint32_t lastFreeHeap = 0;
const int chipSelect = 4;
AsyncWebServer server(80);

enum LoggingState { IDLE, LOGGING, PAUSED };
LoggingState currentState = IDLE;
File logFile;
String currentLogFileName = "";
unsigned long previousMillis = 0;
const long interval = 1000;

// Variables สำหรับ checkWiFi (ใช้ในโหมด STA และ AP_STA)
unsigned long previousWiFiCheckMillis = 0;
const long wifiCheckInterval = 10000; 

// --- Function Prototypes ---
void feedWatchdog();
void debugMemory();
void checkMemory();
void saveState();
void loadState();

#if WIFI_MODE == MODE_STA || WIFI_MODE == MODE_AP_STA
// ฟังก์ชันนี้จะถูกคอมไพล์เฉพาะเมื่ออยู่ในโหมด STA หรือ AP_STA
void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Attempting reconnection in background...");
    // ไม่ต้องเรียก WiFi.disconnect() เพราะอาจจะยังอยู่ในสถานะพยายามเชื่อมต่อ
    WiFi.begin(ssid, password);
  }
}
#endif


void setup() {
  Serial.begin(115200);
  Serial.println("\n\nBooting device...");

  // --- Initialize Filesystems ---
  if (!LittleFS.begin()) {
    Serial.println("FATAL: LittleFS mount failed.");
    return;
  }
  Serial.println("LittleFS mounted successfully.");

  if (!SD.begin(chipSelect)) {
    Serial.println("WARNING: SD Card mount failed. Check wiring or card.");
    // ไม่ return เพราะอาจจะยังอยากใช้เว็บแม้ไม่มี SD card
  } else {
    Serial.println("SD Card initialized.");
  }

  loadState(); 

  // --- WiFi Setup based on selected mode ---
  #if WIFI_MODE == MODE_STA
    Serial.println("\nConfiguring WiFi in Station (STA) Mode...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to " + String(ssid));
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

  #elif WIFI_MODE == MODE_AP
    Serial.println("\nConfiguring WiFi in Access Point (AP) Mode...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("AP Started!");
    Serial.print("Connect to WiFi: " + String(ap_ssid));
    Serial.print("\nGo to IP Address: http://");
    Serial.println(WiFi.softAPIP());

  #elif WIFI_MODE == MODE_AP_STA
    Serial.println("\nConfiguring WiFi in Access Point + Station (AP_STA) Mode...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ap_ssid, ap_password); // เริ่ม AP
    WiFi.begin(ssid, password); // เริ่มเชื่อมต่อ STA

    Serial.println("AP Started!");
    Serial.print("  - Connect to WiFi: " + String(ap_ssid));
    Serial.print("\n  - Go to AP IP: http://");
    Serial.println(WiFi.softAPIP());
    
    Serial.print("Connecting to " + String(ssid));
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi (STA) Connected!");
    Serial.print("  - STA IP Address: ");
    Serial.println(WiFi.localIP());
  #endif

  // --- Web Server Routes (เหมือนเดิมสำหรับทุกโหมด) ---
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/script.js", "text/javascript");
  });
  
  server.on("/files", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/files.html", "text/html");
  });
  server.on("/index", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.on("/files.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/files.css", "text/css");
  });
  server.on("/files.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/files.js", "text/javascript");
  });

  server.on("/list-csv", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    File root = SD.open("/");
    if (!root) {
        request->send(500, "application/json", "{\"error\":\"Could not open SD root\"}");
        return;
    }
    while(true){
      File entry = root.openNextFile();
      if (!entry) break;
      String entryName = entry.name();
      if (entryName.endsWith(".csv")) {
        doc.add(entryName);
      }
      entry.close();
    }
    root.close();
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });
  
  server.on("/control", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("action", true)) {
      String action = request->getParam("action", true)->value();
      String statusMessage = "Unknown state";
      bool stateChanged = false;

      if (action == "start") {
        if (currentState == IDLE) {
          int fileNum = 0;
          do {
            fileNum++;
            currentLogFileName = "/log_" + String(fileNum) + ".csv";
          } while (SD.exists(currentLogFileName));
          
          logFile = SD.open(currentLogFileName, FILE_WRITE);
          if (logFile) {
            logFile.println("Timestamp,Flowrate,Temperature");
            currentState = LOGGING;
            statusMessage = "Recording started";
            stateChanged = true;
          } else {
            statusMessage = "Failed to create file on SD";
          }
        } else if (currentState == PAUSED) {
          currentState = LOGGING;
          statusMessage = "Recording resumed";
          stateChanged = true;
        } else {
          statusMessage = "Already recording";
        }
      } else if (action == "stop") {
        if (currentState != IDLE) {
          if(logFile) logFile.close();
          currentState = IDLE;
          statusMessage = "Recording stopped. File saved.";
          stateChanged = true;
        } else {
          statusMessage = "Already stopped";
        }
      } else if (action == "pause") {
         if (currentState == LOGGING) {
          currentState = PAUSED;
          statusMessage = "Recording paused";
          stateChanged = true;
        } else {
          statusMessage = "Not currently recording";
        }
      } else if (action == "status") { /* Just return status */ }
      
      String currentStatusStr = "Idle";
      if(currentState == LOGGING) currentStatusStr = "Recording";
      if(currentState == PAUSED) currentStatusStr = "Paused";

      if (stateChanged) saveState();
      
      request->send(200, "application/json", "{\"status\":\"" + currentStatusStr + "\", \"message\":\"" + statusMessage + "\"}");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("file")) {
      String fileName = request->getParam("file")->value();
      String fullPath = "/" + fileName;

      if (SD.exists(fullPath)) {
        // Open the file from the SD card
        File file = SD.open(fullPath, "r");
        if (file) {
          // Send the File object. The server will stream its contents.
          // The 'fileName' parameter sets the name of the file in the user's download prompt.
          request->send(file, fileName, "text/csv", true);
          // Note: The AsyncWebServer will close the file for you once it's done sending.
        } else {
          request->send(500, "text/plain", "Server Error: Could not open file.");
        }
      } else {
        request->send(404, "text/plain", "File Not Found");
      }
    } else {
      request->send(400, "text/plain", "Bad Request: 'file' parameter missing.");
    }
  });

  server.on("/delete", HTTP_DELETE, [](AsyncWebServerRequest *request){
    if (request->hasParam("file")) {
      String fileName = "/" + request->getParam("file")->value();
      if (SD.exists(fileName)) {
        if (SD.remove(fileName)) {
          request->send(200, "application/json", "{\"success\":true, \"message\":\"File deleted\"}");
        } else {
          request->send(500, "application/json", "{\"success\":false, \"message\":\"Error deleting file\"}");
        }
      } else {
        request->send(404, "application/json", "{\"success\":false, \"message\":\"File not found\"}");
      }
    } else {
      request->send(400, "application/json", "{\"success\":false, \"message\":\"Bad Request\"}");
    }
  });
  
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("Web server started.");
  lastFreeHeap = ESP.getFreeHeap();
  Serial.print("Initial Free Heap: ");
  Serial.println(lastFreeHeap);
}

void loop() {
  yield();
  unsigned long currentMillis = millis();

  // ตรวจสอบการเชื่อมต่อ Wi-Fi (เฉพาะโหมดที่จำเป็น)
  #if WIFI_MODE == MODE_STA || WIFI_MODE == MODE_AP_STA
    if (currentMillis - previousWiFiCheckMillis >= wifiCheckInterval) {
      previousWiFiCheckMillis = currentMillis;
      checkWiFi();
    }
  #endif
  
  if (currentState == LOGGING) {
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      float flowrate = random(0, 1000) / 10.0;
      float temp = random(150, 300) / 10.0;
      String dataString = String(currentMillis) + "," + String(flowrate) + "," + String(temp);
      
      if (logFile) {
        logFile.println(dataString);
        logFile.flush();
        Serial.println("Logged: " + dataString);
      }
      feedWatchdog();
      checkMemory();
      // debugMemory(); // อาจจะปิดไว้ถ้าไม่ต้องการ log เยอะเกินไป
    }
  }
}


void feedWatchdog()
{
  ESP.wdtFeed();
  Serial.println("Watchdog fed ✅");
}

void debugMemory()
{
  uint32_t currentHeap = ESP.getFreeHeap();
  Serial.println("=== Memory Debug Info ===");
  Serial.print("Free Heap: ");
  Serial.println(currentHeap);

  Serial.print("Max Allocatable Block: ");
  Serial.println(ESP.getMaxFreeBlockSize());

  Serial.print("Heap Fragmentation (%): ");
  Serial.println(ESP.getHeapFragmentation());

  if (lastFreeHeap != 0 && currentHeap < lastFreeHeap - 2000)
  {
    Serial.println("⚠️ Possible Memory Leak Detected!");
  }
  lastFreeHeap = currentHeap;

  Serial.println("=========================");
}

void checkMemory()
{
  uint32_t freeHeap = ESP.getFreeHeap();
  Serial.print("Free Heap Memory: ");
  Serial.println(freeHeap);

  if (freeHeap < 5000)
  {
    Serial.println("WARNING: Memory critically low! Restarting...");
    delay(100);
    ESP.restart();
  }
}

void saveState()
{
  JsonDocument doc;
  doc["currentState"] = currentState;
  doc["fileName"] = currentLogFileName;

  File stateFile = LittleFS.open("/status.json", "w");
  if (!stateFile)
  {
    Serial.println("Failed to open state file for writing");
    return;
  }

  serializeJson(doc, stateFile);
  stateFile.close();
  Serial.println("State saved.");
}

void loadState()
{
  if (LittleFS.exists("/status.json"))
  {
    File stateFile = LittleFS.open("/status.json", "r");
    if (stateFile)
    {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, stateFile);
      if (error)
      {
        Serial.println("Failed to read state file, using default state.");
      }
      else
      {
        currentState = static_cast<LoggingState>(doc["currentState"].as<int>());
        currentLogFileName = doc["fileName"].as<String>();

        Serial.println("State loaded:");
        Serial.print("  - Current State: ");
        Serial.println(currentState);
        Serial.print("  - Log File Name: ");
        Serial.println(currentLogFileName);

        if (currentState == LOGGING || currentState == PAUSED)
        {
          logFile = SD.open(currentLogFileName, "a");
          if (!logFile)
          {
            Serial.println("Failed to re-open log file. Resetting state.");
            currentState = IDLE;
          }
        }
      }
      stateFile.close();
    }
  }
  else
  {
    Serial.println("No state file found. Starting with default state.");
  }
}
