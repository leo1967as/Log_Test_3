/*
 * =======================================================================
 *                           WIFI HANDLER
 * =======================================================================
 * Handles WiFi setup and connection maintenance.
 * =======================================================================
 */

#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

void setupWiFi() {
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
}

void checkWiFi() {
  // Only check in STA mode if we are not connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
  }
}

void loopWiFi() {
  #if WIFI_MODE == MODE_STA || WIFI_MODE == MODE_AP_STA
    unsigned long currentMillis = millis();
    if (currentMillis - previousWiFiCheckMillis >= wifiCheckInterval) {
      previousWiFiCheckMillis = currentMillis;
      checkWiFi();
    }
  #endif
}

#endif // WIFI_HANDLER_H
