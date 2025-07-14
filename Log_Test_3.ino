/*
 * =======================================================================
 *                            MAIN FILE
 * =======================================================================
 * This is the main sketch file. It includes all the necessary custom
 * header files and calls their respective setup and loop functions.
 * =======================================================================
 */

// --- Include Core Libraries ---
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

// --- Include Custom Header Files ---
#include "Config.h"
#include "Utilities.h"
#include "FileHandler.h"
#include "WiFiHandler.h"
#include "WebServerHandler.h"


void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial to connect
  Serial.println("\n\nBooting device...");

  // Initialize filesystems and load previous state
  setupLittleFS();
  setupSDCard();
  loadState();

  // Initialize and connect WiFi
  setupWiFi();

  // Initialize web server routes and start server
  setupWebServer();

  // Initial Memory Check
  lastFreeHeap = ESP.getFreeHeap();
  Serial.print("Initial Free Heap: ");
  Serial.println(lastFreeHeap);
  Serial.println("=========================");
  Serial.println("Setup complete. System is running.");
  Serial.println("=========================");
}

void loop() {
  // Using yield() is good practice in the main loop to prevent watchdog resets,
  // especially if other loop functions are blocking.
  yield();

  // Handle WiFi connection maintenance
  loopWiFi();

  // Handle data logging based on the current state
  loopLogging();
}
