// ==============================================
//     WICS PCA9685 addon to nowRail v1.01
//     2026-06-04 (c) 2026 Bo Holmqvist
// ==============================================
#include "LittleFS.h"
#include <WiFi.h>
#include <WebServer.h>
#include <vector> 

// ==============================================
//            WICS DEFINITIONS 
// ==============================================
// Internal LED
#define XIAO_LED 21
#define LED_ON  LOW
#define LED_OFF HIGH

// CONFIG FILE
#define CONFIG_FILE "/pca9685.txt"
#define MAX_RETRIES 5
#define RETRY_DELAY_MS 60000
#define FILE_CHECK_INTERVAL 15000  // check every 15 seconds
size_t lastFileSize = 0;
unsigned long lastCheckTime = 0;

// I2C
const int SDA_PIN = 5; // XIAO ESP32-S3 STD
const int SCL_PIN = 6; // XIAO ESP32-S3 STD

// Structure to hold custom configuration data for LEDs with active effect timers
struct MyLedConfig {
    int addr; int port; int dcc; int logic; int effect; int maxb; int effb;
    bool isActive;              // Tracks if the DCC address turned this light ON
    unsigned long lastEffectUpdate; // Unique timer tracking variable per port
    int currentBrightness;      // Holds ongoing active dimmed state
    bool flashState;            // Used by effect 4 for flip-flop tracking
};

// Structure to hold custom configuration and movement physics for Servos
struct MyServoConfig {
    int addr; int port; int dcc; int angle0; int angle1; int speed;
    float currentPulse; float targetPulse; float stepSize; unsigned long lastPosUpdate;
};

// Vector storage definitions
std::vector<MyLedConfig> customLeds;
std::vector<MyServoConfig> customServos;

// LED BLINK STATE
int blinkRemaining = 0;
unsigned long blinkInterval = 0;
unsigned long lastBlinkTime = 0;
bool ledState = LED_OFF;

// Default WIFI settings
String wifiStatus = "on"; 
String wifiSSID = "";
String wifiPassword = "wics2026"; 

// Create unique WIFI name from MAC
String getUniqueName() {
  uint64_t chipId = ESP.getEfuseMac();
  uint32_t uniquePart = (uint32_t)(chipId >> 32);
  char buf[20];
  snprintf(buf, sizeof(buf), "ESP32_%08X", uniquePart);
  return String(buf);
}
// WEB Server
WebServer server(80);
File myUploadFile; 

// WIFI Channels 
#define MAX_CHANNEL_RULES 5
struct ChannelRule {
  int dccAddr;
  int logicState;
  int targetChannel;
  bool active;
};

ChannelRule channelRules[MAX_CHANNEL_RULES];
int channelRuleCount = 0;

// Current (starting WIFI channel from nowRail)
uint8_t currentWiFiChannel = WIFICHANNEL;

// ==============================================
// ==============================================
   //       #### WICS FUNCTIONS ###
// ==============================================
// ==============================================

// ===========================
// LED BLINK FUNCTIONS
// ===========================
void wStatusBlink(int count, unsigned long speedMs) {
  if (count <= 0 || blinkRemaining > 0) return;
    blinkRemaining = count * 2;
    blinkInterval = speedMs;
    lastBlinkTime = millis();
}

void updateStatusBlink() {
  if (blinkRemaining <= 0) return;
    if (millis() - lastBlinkTime >= blinkInterval) {
      lastBlinkTime = millis();
      ledState = (ledState == LED_OFF) ? LED_ON : LED_OFF;
      digitalWrite(XIAO_LED, ledState);
    blinkRemaining--;
  }
}

// =====================================
// Print to Both Serial and to error.txt
// =====================================
void logConfigError(const String& message, int lineNum = -1) {
  String fullMsg = (lineNum >= 0) ? "Row " + String(lineNum) + ": " + message : message;

  // Write to Serial Monitor
  Serial.println(fullMsg); 
  
  // Append
  File errFile = LittleFS.open("/error.txt", "a"); 
  if (errFile) {
    errFile.println(fullMsg);
    errFile.close();
  }
}

// ================================
// Read wifi.txt or create new file
// ================================
void initWiFiConfig() {
  if (!LittleFS.exists("/wifi.txt")) {
    wifiSSID = getUniqueName();
    
    File configFile = LittleFS.open("/wifi.txt", FILE_WRITE);
    if (configFile) {
      configFile.println("wifi=" + wifiStatus); 
      configFile.println("ssid=" + wifiSSID);
      configFile.println("password=" + wifiPassword);
      configFile.close();
      Serial.println("Created new wifi.txt file!");
    }
    return;
  }

  File configFile = LittleFS.open("/wifi.txt", FILE_READ);
  if (!configFile) return;

  while (configFile.available()) {
    String line = configFile.readStringUntil('\n');
    line.trim();
    
    if (line.startsWith("wifi="))       wifiStatus = line.substring(5);
    else if (line.startsWith("ssid="))  wifiSSID = line.substring(5);
    else if (line.startsWith("password=")) wifiPassword = line.substring(9);
  }
  configFile.close();
}

// ============================
//  Start/update Wi-Fi and WEB
// ============================
void wics_configureWiFi(byte channel, bool isSetup) {
  if (wifiStatus != "on") return;

  // 1. Start or Update AccessPoint
  if (isSetup) {
    Serial.println("wifi=on in wifi.txt. Starting AccessPoint...");
    WiFi.mode(WIFI_AP_STA);
  }

  if (wifiPassword.length() < 8) {
    WiFi.softAP(wifiSSID.c_str(), nullptr, channel);
  } else {
    WiFi.softAP(wifiSSID.c_str(), wifiPassword.c_str(), channel);
  }
  
   // 2. Start the WEB - only during setup()
  if (isSetup) {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/download", HTTP_GET, handleFileDownload);
    server.on("/upload", HTTP_POST, [](){}, handleFileUpload);
    server.begin();
    
    Serial.print("Access Point IP-adress: ");
    Serial.println(WiFi.softAPIP());
    Serial.println(wifiSSID);
    Serial.printf("WICS configuration WiFi is RUNNING on channel: %d\n", channel);
  } else {
    Serial.printf("[WICS Auto-Sync] Channel change detected! AP moved to: %d\n", channel);
  }
}

// ===========================
//   WICS Board WEB Page
// ===========================
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<style>body{font-family:sans-serif;margin:40px;} table{border-collapse:collapse;margin-top:20px;} td,th{border:1px solid #ddd;padding:8px;}</style>";
  html += "<title>WICS Filehandler</title></head><body>";
  
  html += "<h2>WICS Board for nowRail</h2>";
  html += "<p><strong>Connected to:</strong> " + wifiSSID + "</p>";
  html += "<p><strong>IP-address:</strong> " + WiFi.softAPIP().toString() + "</p>";

  html += "<h2>Upload & Replace file</h2>";
  html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
  html += "<input type='file' name='name'> ";
  html += "<input type='submit' value='Upload & Replace'>";
  html += "</form>";

  html += "<h2>Files on the WICS Board:</h2>";
  
  if (LittleFS.exists("/error.txt")) {
    html += "<p style='background:#ffeeee;padding:10px;border-left:5px solid #ff3333;'>";
    html += "<strong>Info:</strong> Error-file exits. ";
    html += "<a href='/download?file=/error.txt' target='_blank'>Show / Download error.txt</a>";
    html += "</p>";
  }

  html += "<table><tr><th>Filename</th><th>Size (Bytes)</th><th>Action</th></tr>";
  
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    String name = String(file.name());
    if (!name.startsWith("/")) name = "/" + name;
    
    html += "<tr><td>" + name + "</td><td>" + String(file.size()) + "</td>";
    html += "<td><a href='/download?file=" + name + "' target='_blank'>Download</a></td></tr>";
    file = root.openNextFile();
  }
  html += "</table></body></html>";
  
  server.send(200, "text/html", html);
}

// ===========================
// Handle HTTP file DOWNLOAD
// ===========================
void handleFileDownload() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Missing file name!");
    return;
  }
  String filename = server.arg("file");
  if (!filename.startsWith("/")) filename = "/" + filename;

  if (LittleFS.exists(filename)) {
    File file = LittleFS.open(filename, FILE_READ);
    server.streamFile(file, "text/plain");
    file.close();
  } else {
    server.send(404, "text/plain", "File not found!");
  }
}

// ===========================
// Handle HTTP file UPLOAD
// ===========================
void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    Serial.printf("Start upload with replace of: %s\n", filename.c_str());
    
    myUploadFile = LittleFS.open(filename, FILE_WRITE);
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (myUploadFile) {
      myUploadFile.write(upload.buf, upload.currentSize);
    }
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    if (myUploadFile) {
      myUploadFile.close();  
      Serial.printf("Upload done! Size: %u bytes\n", upload.totalSize);
      
      if (upload.filename == "wifi.txt") {
        server.send(200, "text/html", "<h3>wifi.txt is updated. Restarting ...</h3>");
        delay(1000);
        ESP.restart();
        return;
      }
      
      server.sendHeader("Location", "/");
      server.send(303); 
    } else {
      server.send(500, "text/plain", "Could not write to the file!");
    }
  }
}

// ===================
//  PCA9685 FUNCTIONS
// ===================
// 1. Lowest level hardware write function (Must be first!)
// Explicit low-level register write bypasses auto-increment dependencies entirely
void setPCA9685PWM(byte addr, byte port, int on, int off) {
  byte regStart = 0x06 + (port * 4);
  
  // 1. Write ON Time Low Byte
  Wire.beginTransmission(addr);
  Wire.write(regStart);
  Wire.write(on & 0xFF);
  Wire.endTransmission();

  // 2. Write ON Time High Byte
  Wire.beginTransmission(addr);
  Wire.write(regStart + 1);
  Wire.write((on >> 8) & 0xFF);
  Wire.endTransmission();

  // 3. Write OFF Time Low Byte
  Wire.beginTransmission(addr);
  Wire.write(regStart + 2);
  Wire.write(off & 0xFF);
  Wire.endTransmission();

  // 4. Write OFF Time High Byte
  Wire.beginTransmission(addr);
  Wire.write(regStart + 3);
  Wire.write((off >> 8) & 0xFF);
  Wire.endTransmission();
}

// 2. High-level brightness mapper for Open Drain outputs
void setPCA9685Brightness(byte addr, byte port, int brightness) {
  int safeValue = constrain(brightness, 0, 4095);
  setPCA9685PWM(addr, port, 0, safeValue);
}

// 3. Old compatibility function for single-board default writes
void setPCA9685Port(byte port, bool turnOn) {
  if (turnOn) {
    setPCA9685PWM(0x40, port, 0, 0x1000); 
  } else {
    setPCA9685PWM(0x40, port, 0x1000, 0);
  }
}

// 4. Safe hardware initialization and Open Drain locking routine
void initPCA9685Board(byte addr, byte hz) {
  // Put board to sleep to allow changes to PRE_SCALE register
  Wire.beginTransmission(addr); Wire.write(0x00); Wire.write(0x30); Wire.endTransmission(); delay(10);
  
  // Calculate prescale value based on datasheet formulas
  float prescaleval = 25000000.0;
  prescaleval /= 4096.0;
  prescaleval /= (float)hz;
  prescaleval -= 1.0;
  byte prescale = floor(prescaleval + 0.5);
  
  // Write frequency 
  Wire.beginTransmission(addr); Wire.write(0xFE); Wire.write(prescale); Wire.endTransmission(); delay(10);
  
  // Force Open Drain structure by clearing OUTDRV bit in MODE2
  Wire.beginTransmission(addr);
  Wire.write(0x01); // MODE2 Register
  Wire.write(0x00); // 0x00 locks Open Drain drive
  Wire.endTransmission();
  delay(10);

  // Wake board back up with auto-increment enabled (0xA0)
  Wire.beginTransmission(addr); Wire.write(0x00); Wire.write(0xA0); Wire.endTransmission(); delay(10);
}

// Non-blocking centralized background engine handling both physics and visual animations
void updateCustomHardware() {
  unsigned long now = millis();

  // 1. PROCESS SMOOTH SERVO MOVEMENTS
  for (auto& s : customServos) {
    if (s.currentPulse != s.targetPulse) {
      long elapsed = now - s.lastPosUpdate;
      if (elapsed > 0) {
        s.lastPosUpdate = now;
        float moveAmnt = s.stepSize * elapsed;
        
        if (s.currentPulse < s.targetPulse) {
          s.currentPulse += moveAmnt; if (s.currentPulse > s.targetPulse) s.currentPulse = s.targetPulse;
        } else {
          s.currentPulse -= moveAmnt; if (s.currentPulse < s.targetPulse) s.currentPulse = s.targetPulse;
        }
        setPCA9685PWM(s.addr, s.port, 0, (int)s.currentPulse);
      }
    }
  }

  // 2. PROCESS LIGHTING EFFECTS (0 = static, 1 = fire, 2 = gas, 3 = welder, 4 = flash)
   for (auto& led : customLeds) {
    if (!led.isActive) {
      // Light is deactivated via DCC: Force completely OFF state (4095 = Cut ground/OFF)
      setPCA9685Brightness(led.addr, led.port, 4095);
      continue;
    }

    // Process active animation layers frame-by-frame
    switch (led.effect) {
      case 0: // Standard static on/off logic
        setPCA9685Brightness(led.addr, led.port, led.maxb);
        break;

      case 1: // Realistic Fire Flicker (Random timing + random quick dimming)
        if (now - led.lastEffectUpdate >= led.lastEffectUpdate % 45 + 30) {
          led.lastEffectUpdate = now;
          int targetBright = random(led.effb, led.maxb);
          setPCA9685Brightness(led.addr, led.port, targetBright);
        }
        break;

      case 2: // Old Gas Light (Steady glow broken up by sudden accidental flickering)
        if (now - led.lastEffectUpdate >= led.lastEffectUpdate % 200 + 50) {
          led.lastEffectUpdate = now;
          if (random(0, 100) > 92) {
            setPCA9685Brightness(led.addr, led.port, random(led.effb, led.maxb / 2)); // Deep drop
          } else {
            setPCA9685Brightness(led.addr, led.port, random(led.maxb - 200, led.maxb)); // Normal hum
          }
        }
        break;

      case 3: // Intense Arc Welder (Violent random sparks bursts with cool-down breaks)
        if (now - led.lastEffectUpdate >= led.lastEffectUpdate % 30 + 5) {
          led.lastEffectUpdate = now;
          if (random(0, 100) > 40) {
            setPCA9685Brightness(led.addr, led.port, random(led.effb, led.maxb)); // Bright blue-white burst
          } else {
            setPCA9685Brightness(led.addr, led.port, 0); // Brief dark arc gap
          }
        }
        break;

 case 4: // Standard Crossing Flashing Signal (Alternates evenly based on fixed timer intervals)
        // Uses the PCA9685FLASHTIMER mapping or falls back to a perfect 500ms level
        if (now - led.lastEffectUpdate >= 500) { 
          led.lastEffectUpdate = now;
          led.flashState = !led.flashState;
          
          // Pass the values raw to alternate the channels perfectly
          setPCA9685Brightness(led.addr, led.port, led.flashState ? led.maxb : led.effb);
        }
        break;          
    } 
  } 
} 

// ======================================================================
// CONFIG PARSER FOR SERVO/LED WITH SEPARATED VALIDATION (WICS v1.0.1)
// ======================================================================
bool loadConfig() {
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("ERROR: Cannot open config file!");
        return false;
    }

    customLeds.clear();
    customServos.clear();
    int lineNumber = 0;

    while (file.available()) {
        lineNumber++;
        String line = file.readStringUntil('\n');
        line.trim();

        if (line.length() == 0 || line.startsWith("#")) continue;

        int commentPos = line.indexOf('#');
        if (commentPos >= 0) {
            line = line.substring(0, commentPos);
            line.trim();
        }

        char typeStr[16] = {0};
        int address, port, dcc;

        // Read address as hexadecimal directly (e.g., "40" in file becomes 64 / 0x40)
        if (sscanf(line.c_str(), " %15[^,], %x, %d, %d", typeStr, &address, &port, &dcc) < 4) continue;
        for (int i = 0; typeStr[i]; i++) typeStr[i] = toupper(typeStr[i]);

        // ====================================================================
        // PARSE SERVO DEFINITIONS (Accepts 450-2000 us, translates to 100-500)
        // ====================================================================
        if (strcmp(typeStr, "SERVO") == 0) {
            int a0, a1, time, enabled;
            if (sscanf(line.c_str(), " %*[^,], %*x, %*d, %*d, %d, %d, %d, %d", &a0, &a1, &time, &enabled) >= 4) {
                
                // 1. Validate against the new microsecond intervals (450-2000 us)
                if (a0 < 450 || a0 > 2000 || a1 < 450 || a1 > 2000) {
                    Serial.printf("Row %d: Servo pulse values outside allowed interval (450-2000 us)!\n", lineNumber);
                    continue; // Skip invalid row
                }
                if (time < 1 || time > 60000) continue;

                if (enabled == 1) {
                    // 2. TRANSLATION: Recompute microseconds into your local raw ticks (100-500)
                    float old_a0 = (float)a0 / 4.88;
                    float old_a1 = (float)a1 / 4.88;

                    if (address >= 40 && address <= 43) { address = address + 24; }
                    
                    // 3. Calculate movement dynamics based on translated values
                    float totalDistance = abs(old_a1 - old_a0);
                    float step = (time > 0) ? (totalDistance / (float)time) : totalDistance;
                    
                    customServos.push_back({
                        address, port, dcc, 
                        (int)old_a0, (int)old_a1, 
                        time, 
                        old_a0, old_a0, 
                        step, 
                        millis()
                    });
                }
            }
        }
        // ==========================================
        // PARSE LED DEFINITIONS (Limits: 0-4095)
        // ==========================================
        else if (strcmp(typeStr, "LED") == 0) {
            int logic, effect, maxb, effb, enabled;
            if (sscanf(line.c_str(), " %*[^,], %*x, %*d, %*d, %d, %d, %d, %d, %d", &logic, &effect, &maxb, &effb, &enabled) >= 5) {
                
                if (maxb < 0 || maxb > 4095 || effb < 0 || effb > 4095) {
                    Serial.printf("Row %d: LED brightness outside allowed interval (0-4095)!\n", lineNumber);
                    continue; // Skip invalid row
                }

                if (enabled == 1) {
                    if (address >= 40 && address <= 43) { address = address + 24; }
                    customLeds.push_back({address, port, dcc, logic, effect, maxb, effb, false, (unsigned long)random(0, 1000), 0, false});
                }
            }
        } 
    }
    file.close();

    // Post-Parsing Auto-Configuration Loop for active layout modules (64 to 67 / 0x40 to 0x43)
    Serial.println("Scanning and auto-configuring detected PCA9685 board types...");
    for (int boardAddr = 64; boardAddr <= 67; boardAddr++) {
        bool hasServos = false; bool hasLeds = false;
        for (const auto& s : customServos) { if (s.addr == boardAddr) { hasServos = true; break; } }
        for (const auto& l : customLeds)   { if (l.addr == boardAddr) { hasLeds = true; break; } }

        if (hasServos) {
            Serial.printf("-> Board 0x%02X classified as SERVO. Setting frequency to 50Hz.\n", boardAddr);
            initPCA9685Board(boardAddr, 50);
        } else if (hasLeds) {
            Serial.printf("-> Board 0x%02X classified as LED. Setting frequency to 200Hz.\n", boardAddr);
            initPCA9685Board(boardAddr, 200);
        }
    }

    // Force absolute startup OFF state on all active Open Drain LED ports
    for (const auto& led : customLeds) { setPCA9685Brightness(led.addr, led.port, 4095); }
    
    // Drive all registered servos to their default Angle-0 home position directly
    for (const auto& s : customServos) { setPCA9685PWM(s.addr, s.port, 0, s.angle0); }

    Serial.printf("SUCCESS: Loaded %d Custom LEDs and %d Custom Servos into active loops.\n", customLeds.size(), customServos.size());
    return true;
}


// =====================================
// CHECK and RETRY LOAD OF CONFIGURATION
// =====================================
bool loadConfigWithRetry() {
    const int MAX_ATTEMPTS = 5;       
    const unsigned long RETRY_DELAY = 10000; 

    for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
        Serial.printf("Attempt %d of %d to load configuration...\n", attempt, MAX_ATTEMPTS);

        if (LittleFS.exists(CONFIG_FILE)) {
            if (loadConfig()) {
                return true; 
            }
            return false; 
        }

        Serial.println("Configuration file is missing!");
        if (attempt == MAX_ATTEMPTS) break;

        Serial.println("Waiting for Configuration file to be uploaded ...");
        wStatusBlink(7, 200); 

        unsigned long startWait = millis();
        while (millis() - startWait < RETRY_DELAY) {
            updateStatusBlink();
            delay(1);
            yield();
        }
    }
    return false; 
}

// ========================
//  RELOAD OF CONFIGURATION
// ========================
void checkConfigReload() {
    if (millis() - lastCheckTime < FILE_CHECK_INTERVAL) return;
    lastCheckTime = millis();

    if (!LittleFS.exists(CONFIG_FILE)) {
        lastFileSize = 0; 
        return;
    }

    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) return;

    size_t currentSize = file.size();
    file.close();

    if (lastFileSize == 0) {
        lastFileSize = currentSize;
        Serial.println("New configuration file found. Loading...");
        if (loadConfig()) {
            wStatusBlink(2, 300); 
        } else {
            wStatusBlink(5, 50); 
        }
        return;
    }

    if (currentSize != lastFileSize) {
        Serial.println("CONFIG CHANGE DETECTED → reloading");
        lastFileSize = currentSize;

        if (loadConfig()) {
            wStatusBlink(2, 300); 
        } else {
            wStatusBlink(5, 50); 
        }
    }
}

// ==============================================
// ==============================================
//     ### WICS INIT (Setup) ###
// ==============================================
// ==============================================
void initWics() {
  Serial.begin(115200);
  uint32_t usbTime = millis();
  while (!Serial && (millis() - usbTime < 2000)) { delay(10); };
  
  Serial.println(F("\n======================================="));
  Serial.println( " ### WICS Init Start ###");
  Serial.printf( "  nowRail Core Version:  %s\n", NOWRAIL_VERSION);
  Serial.printf( "  WICS addOn Version:    %s\n", WICS_VERSION);
  Serial.println(F("\n======================================="));
  
  // Configure internal status LED for Seeed Studio XIAO ESP32-S3
  pinMode(XIAO_LED, OUTPUT);
  digitalWrite(XIAO_LED, LED_OFF);
   
  // ========================
  // Check LittleFS
  // ========================
  if (!LittleFS.begin(true)) {
    Serial.println("CRITICAL ERROR: LittleFS mount FAILED! System on HOLD!");
    wStatusBlink(5, 50); 
    
    // Halt execution if partition layout layer fails to mount
    while (true) {
      updateStatusBlink(); 
      delay(1);
      yield();             
    }
  }

  // Read or generate default fallback configuration files
  initWiFiConfig();

  // ==================================
  // Start AccessPoint & Web if enabled
  // ==================================
  if (wifiStatus == "on") {
    wics_configureWiFi(WIFICHANNEL, true); 
  } else {
    Serial.println("WICS configuration WiFi CLOSED. (wifi=off in wifi.txt)");
  }
  
  // ==========================
  // Check & Load Configuration
  // ==========================
  if (!loadConfigWithRetry()) {
    logConfigError("Error: No configuration found. System on Hold!");
    wStatusBlink(5, 50);
    
    while (true) {
      updateStatusBlink(); 
      delay(1);
      yield();             
    }
  } else {
    // Synchronize initial baseline track storage states
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (file) {
      lastFileSize = file.size();
      file.close();
    }
  }

  // ========================
  // Success - WICS INI Done!
  // ========================
  Serial.println("--- WICS Init Done ---");
  wStatusBlink(2, 300);
  
  // Non-blocking flush loop to output clean completion blink cycles
  while (blinkRemaining > 0) {
    updateStatusBlink();
    delay(1);
    yield();
  }
}

// ==============================================
// ==============================================
// WICS RUN FUNCTION (Loop) Called from nowRail
// ==============================================
// ==============================================
void runWics() {
  // Process incoming web clients if communication layer remains active
  if (wifiStatus == "on") {
    server.handleClient(); 
  }
  updateStatusBlink();    // Process ongoing system heartbeat flash tasks
  checkConfigReload();    // Look for changes across storage file sectors
  updateCustomHardware(); // Independently manages non-blocking frame interpolation for moving points
}



