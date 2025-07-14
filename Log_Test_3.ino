#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>


uint32_t lastFreeHeap = 0;

// WiFi Credentials
const char* ssid = "LEOLEOLEO";
const char* password = "11111111";

unsigned long previousWiFiCheckMillis = 0;
const long wifiCheckInterval = 10000; // เช็คทุก 10 วินาที


// SD Card Chip Select pin
const int chipSelect = 4;

// Web Server
AsyncWebServer server(80);

// Logging state
enum LoggingState { IDLE, LOGGING, PAUSED };
LoggingState currentState = IDLE;
File logFile;
String currentLogFileName = "";
unsigned long previousMillis = 0;
const long interval = 1000; // Log data every 5 seconds


void feedWatchdog() {
  ESP.wdtFeed();
  Serial.println("Watchdog fed ✅");
}

void debugMemory() {
  uint32_t currentHeap = ESP.getFreeHeap();
  Serial.println("=== Memory Debug Info ===");
  Serial.print("Free Heap: ");
  Serial.println(currentHeap);

  Serial.print("Max Allocatable Block: ");
  Serial.println(ESP.getMaxFreeBlockSize());

  Serial.print("Heap Fragmentation (%): ");
  Serial.println(ESP.getHeapFragmentation());

  // เช็คว่าหน่วยความจำลดลงเรื่อย ๆ (memory leak)
  if (lastFreeHeap != 0 && currentHeap < lastFreeHeap - 2000) { // ถ้าลดลงเกิน 2000 bytes
    Serial.println("⚠️ Possible Memory Leak Detected!");
  }
  lastFreeHeap = currentHeap;

  Serial.println("=========================");
}

void checkMemory() {
  uint32_t freeHeap = ESP.getFreeHeap();
  Serial.print("Free Heap Memory: ");
  Serial.println(freeHeap);

  if (freeHeap < 5000) { // ถ้าน้อยกว่า 5KB
    Serial.println("WARNING: Memory critically low! Restarting...");
    delay(100); // flush serial
    ESP.restart();  // รีเซ็ต MCU
  }
}

// ฟังก์ชันสำหรับบันทึกสถานะลงในไฟล์
void saveState() {
  // สร้าง JSON object
  JsonDocument doc;
  doc["currentState"] = currentState;
  doc["fileName"] = currentLogFileName;

  // เปิดไฟล์ status.json เพื่อเขียนทับ
  File stateFile = LittleFS.open("/status.json", "w");
  if (!stateFile) {
    Serial.println("Failed to open state file for writing");
    return;
  }
  
  // เขียนข้อมูล JSON ลงไฟล์
  serializeJson(doc, stateFile);
  stateFile.close();
  Serial.println("State saved.");
}

// ฟังก์ชันสำหรับโหลดสถานะจากไฟล์
void loadState() {
  if (LittleFS.exists("/status.json")) {
    File stateFile = LittleFS.open("/status.json", "r");
    if (stateFile) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, stateFile);
      if (error) {
        Serial.println("Failed to read state file, using default state.");
      } else {
        currentState = static_cast<LoggingState>(doc["currentState"].as<int>());
        currentLogFileName = doc["fileName"].as<String>();
        
        Serial.println("State loaded:");
        Serial.print("  - Current State: "); Serial.println(currentState);
        Serial.print("  - Log File Name: "); Serial.println(currentLogFileName);

        // ถ้าสถานะล่าสุดคือ LOGGING หรือ PAUSED ให้เปิดไฟล์ log เดิมขึ้นมาเพื่อเขียนต่อ
        if (currentState == LOGGING || currentState == PAUSED) {
          logFile = SD.open(currentLogFileName, "a");
          if (!logFile) {
            Serial.println("Failed to re-open log file. Resetting state.");
            currentState = IDLE;
          }
        }
      }
      stateFile.close();
    }
  } else {
    Serial.println("No state file found. Starting with default state.");
  }
}

void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Attempting reconnection in background...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
  }
}



void setup() {
  Serial.begin(115200);

  // --- Initialize LittleFS ---
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  Serial.println("LittleFS mounted successfully");

  // --- Initialize SD Card ---
  if (!SD.begin(chipSelect)) {
    Serial.println("Card Mount Failed. Check wiring or card.");
    return;
  }
  Serial.println("SD Card initialized.");

  // --- โหลดสถานะล่าสุด ---
  loadState(); 


  // --- WiFi Connection ---
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // --- Web Server Routes ---

  // Serve main page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Serve main page CSS and JS
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/script.js", "text/javascript");
  });
  
  // Serve file manager page
  server.on("/files", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/files.html", "text/html");
  });
    // Serve file manager page
  server.on("/index", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Serve file manager CSS and JS
  server.on("/files.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/files.css", "text/css");
  });
  server.on("/files.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/files.js", "text/javascript");
  });

  // API to list CSV files from SD card
  server.on("/list-csv", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    File root = SD.open("/");
    while(true){
      File entry = root.openNextFile();
      if (!entry) break;
      String entryName = entry.name();
      if (entryName.endsWith(".csv")) {
        JsonObject fileObj = doc.add<JsonObject>();
        fileObj["name"] = entryName;
      }
      entry.close();
    }
    root.close();
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // API for logging control
  server.on("/control", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("action", true)) {
      String action = request->getParam("action", true)->value();
      String statusMessage = "Unknown state";
      bool stateChanged = false; // ตัวแปรเช็คว่าสถานะเปลี่ยนหรือไม่

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
                      stateChanged = true; // สถานะเปลี่ยน
          } else {
            statusMessage = "Failed to create file on SD";
          }
        } else if (currentState == PAUSED) {
          currentState = LOGGING;
          statusMessage = "Recording resumed";
                    stateChanged = true; // สถานะเปลี่ยน
        } else {
          statusMessage = "Already recording";
        }
      } else if (action == "stop") {
        if (currentState != IDLE) {
          if(logFile) logFile.close();
          currentState = IDLE;
          statusMessage = "Recording stopped. File saved.";
                  stateChanged = true; // สถานะเปลี่ยน
        } else {
          statusMessage = "Already stopped";
        }
      } else if (action == "pause") {
         if (currentState == LOGGING) {
          currentState = PAUSED;
          statusMessage = "Recording paused";
          stateChanged = true; // สถานะเปลี่ยน
        } else {
          statusMessage = "Not currently recording";
        }
      } else if (action == "status") {
        // Just return current status without changing it
      }
      
      String currentStatusStr = "Idle";
      if(currentState == LOGGING) currentStatusStr = "Recording";
      if(currentState == PAUSED) currentStatusStr = "Paused";

            // ถ้าสถานะมีการเปลี่ยนแปลง ให้บันทึก
      if (stateChanged) {
        saveState();
      }
      request->send(200, "application/json", "{\"status\":\"" + currentStatusStr + "\", \"message\":\"" + statusMessage + "\"}");
    } else {
      request->send(400, "text/plain", "Bad Request");
    }
  });

  // Handle file downloads

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

  // Handle file deletions
  server.on("/delete", HTTP_DELETE, [](AsyncWebServerRequest *request){
    if (request->hasParam("file")) {
      String fileName = "/" + request->getParam("file")->value();
      if (SD.exists(fileName)) {
        if (SD.remove(fileName)) {
          request->send(200, "application/json", "{\"success\":true, \"message\":\"File deleted successfully\"}");
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
  Serial.print("Initial Free Heap Memory: ");
  Serial.println(ESP.getFreeHeap());
  lastFreeHeap = ESP.getFreeHeap();


}

void loop() {
    yield();  // ป้อน watchdog ป้องกันค้าง (จำเป็นบน ESP8266)
  unsigned long currentMillis = millis();

  // เช็ค WiFi ทุก wifiCheckInterval
  if (currentMillis - previousWiFiCheckMillis >= wifiCheckInterval) {
    previousWiFiCheckMillis = currentMillis;
    checkWiFi();
  }
  
  if (currentState == LOGGING) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      // Generate dummy data
      float flowrate = random(0, 1000) / 10.0;
      float temp = random(150, 300) / 10.0;
      
      String dataString = String(currentMillis) + "," + String(flowrate) + "," + String(temp);
      
      if (logFile) {
        logFile.println(dataString);
        logFile.flush(); // Ensure data is written to SD card
        Serial.println("Logged: " + dataString);
      }
      feedWatchdog();   // ป้อน watchdog manual
      checkMemory();    // เช็คว่าหน่วยความจำพอไหม
      debugMemory();    // debug memory + เช็ค memory leak
    }
  }
}
