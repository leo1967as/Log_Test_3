/*
 * =======================================================================
 *                           FILE HANDLER
 * =======================================================================
 * Handles LittleFS, SD card, state saving/loading and logging logic.
 * NOW WITH ROW INDEXING INSTEAD OF TIMESTAMP.
 * =======================================================================
 */
#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

// --- Function Prototypes ---
void checkMemory();
void feedWatchdog();

// --- Global variables ---
float latestFlowrate = 0.0;
float latestWeight = 0.0;
unsigned long dataRowIndex = 0; // <--- NEW: Index for the data row inside a file.


void setupLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("FATAL: LittleFS mount failed.");
    while(1) yield();
  }
  Serial.println("LittleFS mounted successfully.");
}

void setupSDCard() {
  if (!SD.begin(chipSelect)) {
    Serial.println("WARNING: SD Card mount failed. Check wiring or card.");
  } else {
    Serial.println("SD Card initialized.");
  }
}

// --- MODIFIED: Now saves the dataRowIndex as well ---
void saveState() {
  JsonDocument doc;
  doc["currentState"] = currentState;
  doc["fileName"] = currentLogFileName;
  doc["dataRowIndex"] = dataRowIndex; // <--- NEW: Save the data row index

  File stateFile = LittleFS.open("/status.json", "w");
  if (!stateFile) {
    Serial.println("Failed to open state file for writing");
    return;
  }
  serializeJson(doc, stateFile);
  stateFile.close();
  Serial.println("State saved.");
}

// --- MODIFIED: Now loads the dataRowIndex ---
void loadState() {
  if (LittleFS.exists("/status.json")) {
    File stateFile = LittleFS.open("/status.json", "r");
    if (stateFile) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, stateFile);
      if (error) {
        Serial.println("Failed to read state file, using default state.");
      } else {
        // Load the data row index. If it doesn't exist or is null, it defaults to 0.
        dataRowIndex = doc["dataRowIndex"] | 0;

        if (SD.begin(chipSelect)) {
           currentState = static_cast<LoggingState>(doc["currentState"].as<int>());
           currentLogFileName = doc["fileName"].as<String>();
  
          Serial.println("State loaded:");
          Serial.print("  - Current State: ");
          Serial.println(currentState);
          Serial.print("  - Log File Name: ");
          Serial.println(currentLogFileName);
          Serial.print("  - Next Data Row Index: "); // <--- NEW: For debugging
          Serial.println(dataRowIndex);
  
          // Re-open file if the device was logging/paused before restart
          if (currentState == LOGGING || currentState == PAUSED) {
            logFile = SD.open(currentLogFileName, "a");
            if (!logFile) {
              Serial.println("Failed to re-open log file. Resetting state to IDLE.");
              currentState = IDLE;
              dataRowIndex = 0; // Reset index if file fails
            } else {
              Serial.println("Successfully re-opened log file: " + currentLogFileName);
            }
          }
        } else {
           Serial.println("SD card not found on boot, cannot restore logging state. Setting to IDLE.");
           currentState = IDLE;
           dataRowIndex = 0; // Reset index if SD fails
        }
      }
      stateFile.close();
    }
  } else {
    Serial.println("No state file found. Starting with default state.");
  }
}

// --- MODIFIED: Use dataRowIndex instead of timestamp ---
void loopLogging() {
  if (currentState == LOGGING) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      float flowrate = random(0, 1000) / 10.0;
      float weight = random(0, 500) / 100.0;

      latestFlowrate = flowrate;
      latestWeight = weight;

      // Create the string for the CSV file using dataRowIndex
      String dataString = String(dataRowIndex) + "," + String(flowrate) + "," + String(weight);
      dataRowIndex++; // Increment index for the next row

      if (logFile) {
        logFile.println(dataString);
        logFile.flush(); 
        Serial.println("Logged: " + dataString);
      } else {
        Serial.println("ERROR: Log file is not open!");
        currentState = IDLE;
        saveState(); // Save the idle state
      }
      
      feedWatchdog();
      checkMemory();
    }
  }
}

#endif // FILE_HANDLER_H
