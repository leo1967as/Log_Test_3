/*
 * =======================================================================
 *                          UTILITIES FILE
 * =======================================================================
 * Contains helper functions for system maintenance like watchdog feeding
 * and memory monitoring.
 * =======================================================================
 */

#ifndef UTILITIES_H
#define UTILITIES_H

void feedWatchdog() {
  ESP.wdtFeed();
  // Serial.println("Watchdog fed ✅"); // Optional: uncomment for debugging
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

  if (lastFreeHeap != 0 && currentHeap < lastFreeHeap - 2000) {
    Serial.println("⚠️ Possible Memory Leak Detected!");
  }
  lastFreeHeap = currentHeap;
  Serial.println("=========================");
}

void checkMemory() {
  uint32_t freeHeap = ESP.getFreeHeap();
  // Serial.print("Free Heap Memory: "); // Optional: uncomment for debugging
  // Serial.println(freeHeap);

  // Restart if memory is critically low to prevent crashes
  if (freeHeap < 5000) {
    Serial.println("WARNING: Memory critically low! Restarting...");
    delay(100);
    ESP.restart();
  }
}

#endif // UTILITIES_H
