/*
 * =======================================================================
 *                         WEB SERVER HANDLER
 * =======================================================================
 * Sets up all web server routes and starts the server.
 * =======================================================================
 */

#ifndef WEBSERVER_HANDLER_H
#define WEBSERVER_HANDLER_H

// --- Extern global variables from FileHandler.h to make them accessible here ---
extern float latestFlowrate;
extern float latestWeight;
extern unsigned long dataRowIndex;


void setupWebServer() {
  // --- Serve Static Files ---
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

  // ---> ADDED START: API Route for real-time data <---
  // This endpoint provides the latest sensor readings to the frontend.
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument doc;
    doc["timestamp"] = millis();
    doc["flowrate"] = latestFlowrate;
    doc["weight"] = latestWeight;
    
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });
  // ---> ADDED END <---

  // --- API Route to list CSV files from SD Card ---
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

  // --- API Route for controlling logging ---
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
              
              // ---> MODIFICATIONS START <---
              dataRowIndex = 0; // RESET the row index to 0 for the new file
              logFile.println("Index,Flowrate,Weight"); // CHANGE the header
              // ---> MODIFICATIONS END <---
      
              currentState = LOGGING;
              statusMessage = "Recording started to file " + currentLogFileName;
              stateChanged = true;
            } else {
              statusMessage = "Failed to create file on SD Card.";
            }
          } else if (currentState == PAUSED) {
            currentState = LOGGING;
            statusMessage = "Recording resumed.";
            stateChanged = true;
          } else {
            statusMessage = "Already recording.";
          }
      } 
       else if (action == "stop") {
        if (currentState != IDLE) {
          if(logFile) logFile.close();
          currentState = IDLE;
          statusMessage = "Recording stopped. File saved.";
          stateChanged = true;
        } else {
          statusMessage = "Already stopped.";
        }
      } else if (action == "pause") {
         if (currentState == LOGGING) {
          currentState = PAUSED;
          statusMessage = "Recording paused.";
          stateChanged = true;
        } else {
          statusMessage = "Not currently recording.";
        }
      } else if (action == "status") { /* Just return status below */ }
      
      String currentStatusStr = "Idle";
      if(currentState == LOGGING) currentStatusStr = "Recording";
      if(currentState == PAUSED) currentStatusStr = "Paused";

      if (stateChanged) saveState();
      
      request->send(200, "application/json", "{\"status\":\"" + currentStatusStr + "\", \"message\":\"" + statusMessage + "\", \"fileName\":\"" + currentLogFileName.substring(1) + "\"}");
    } else {
      request->send(400, "text/plain", "Bad Request: Missing 'action' parameter.");
    }
  });

  // --- Route for downloading a file ---
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("file")) {
      String fileName = request->getParam("file")->value();
      String fullPath = "/" + fileName;

      if (SD.exists(fullPath)) {
        File file = SD.open(fullPath, "r");
        if (file) {
          request->send(file, fileName, "text/csv", true);
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

  // --- Route for deleting a file ---
  server.on("/delete", HTTP_DELETE, [](AsyncWebServerRequest *request){
    if (request->hasParam("file")) {
      String fileName = "/" + request->getParam("file")->value();
      if (SD.exists(fileName)) {
        if (SD.remove(fileName)) {
          request->send(200, "application/json", "{\"success\":true, \"message\":\"File '" + fileName.substring(1) + "' deleted.\"}");
        } else {
          request->send(500, "application/json", "{\"success\":false, \"message\":\"Error deleting file.\"}");
        }
      } else {
        request->send(404, "application/json", "{\"success\":false, \"message\":\"File not found.\"}");
      }
    } else {
      request->send(400, "application/json", "{\"success\":false, \"message\":\"Bad Request: 'file' parameter missing.\"}");
    }
  });

  server.on("/remount-sd", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("API call received: /remount-sd. Remounting SD card...");
    
    // การ unmount และ mount SD card ใหม่
    SD.end(); 
    delay(100); // ให้เวลาเล็กน้อยก่อน re-initiate

    if (SD.begin(chipSelect)) {
      Serial.println("SD Card remounted successfully.");
      request->send(200, "application/json", "{\"success\":true, \"message\":\"SD Card rescanned successfully.\"}");
    } else {
      Serial.println("ERROR: Failed to remount SD Card during API call.");
      request->send(500, "application/json", "{\"success\":false, \"message\":\"Failed to remount SD Card. Check physical connection.\"}");
    }
  });


  // --- Handle Not Found requests ---
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found");
  });

  // --- Start the server ---
  server.begin();
  Serial.println("Web server started.");
}

#endif // WEBSERVER_HANDLER_H
