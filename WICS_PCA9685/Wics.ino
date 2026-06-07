// ==============================================
//     WICS PCA9685 addon to nowRail v1.1.0
//     2026-06-06 (c) 2026 Bo Holmqvist
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
// const int SDA_PIN = 4; // XIAO ESP32-S3 STD
// const int SCL_PIN = 5; // XIAO ESP32-S3 STD

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

// ======================================================================
// CONFIG PARSER WITH AUTOMATIC PCA9685 HARDWARE DETECTION & DIAGNOSTICS
// ======================================================================
bool loadConfig() {
    // 1. Clear error.txt before the new execution
    File clearLog = LittleFS.open("/error.txt", "w");
    if (clearLog) {
        clearLog.close(); 
    }

    // 2. DETECT CONNECTED PCA9685 MODULES (Addresses 0x40 - 0x43)
    bool boardConnected[] = {false, false, false, false}; 
    Serial.println("\n--- Scanning for connected PCA9685 modules ---");
    
    Wire.begin(); // Forces I2C bus to start before scanning
    
    for (int b = 0; b < 4; b++) {
        int currentAddr = 64 + b; // 64 is decimal for 0x40
        
        Wire.beginTransmission(currentAddr);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            boardConnected[b] = true;
            Serial.printf("Found PCA9685 board at address: 0x%02X\n", currentAddr);
            
            // Force Open Drain mode immediately when a board is found (OUTDRV = 0)
            Wire.beginTransmission(currentAddr);
            Wire.write(0x01); // MODE2 Register
            Wire.write(0x01); // Sets OUTDRV = 0 (Open Drain) and OUTNE = 01
            Wire.endTransmission();

            // Clear any leftover register memory by forcing ALL ports to a safe OFF state
            Wire.beginTransmission(currentAddr);
            Wire.write(0xFA); // ALL_LED_ON_L Register
            Wire.write(0x00); // ON L = 0
            Wire.write(0x00); // ON H = 0
            Wire.write(0x00); // OFF L = 0
            Wire.write(0x10); // OFF H = 0x10 (Forces full hardware shutdown on all pins)
            Wire.endTransmission();
        }
    }
    Serial.println("----------------------------------------------");

    // 3. Open configuration file
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("ERROR: Cannot open config file!");
        logConfigError("ERROR: Cannot open config file");
        return false;
    }

    bool hasErrors = false;
    int lineNumber = 0;

    // Structs to save OK data before activating nowRail myLayout commands
    struct ServoConfig { int addr, port, dcc, a0, a1, time; };
    struct LedConfig { int addr, port, dcc, logic, effect, maxb, effb; };
    
    std::vector<ServoConfig> validServos;
    std::vector<LedConfig> validLeds;

    while (file.available()) {
        lineNumber++;
        String line = file.readStringUntil('\n');
        line.trim();

        // Skip empty lines and pure comments
        if (line.length() == 0 || line.startsWith("#")) continue;

        // Strip comments at the end of the line
        int commentPos = line.indexOf('#');
        if (commentPos >= 0) {
            line = line.substring(0, commentPos);
            line.trim();
        }

        char typeStr[16] = {0};
        int address, port, dcc;

        // Read address as hexadecimal directly (e.g., "40" in file becomes 64 / 0x40)
        if (sscanf(line.c_str(), " %15[^,], %x, %d, %d", typeStr, &address, &port, &dcc) < 4) {
            hasErrors = true;
            Serial.printf("Row %d: Format error: (Type,Address,Port,DCC)\n", lineNumber);
            logConfigError("Format error: (Type,Address,Port,DCC)", lineNumber);
            continue;
        }

        // Hardcoded interval validation (0x40 = 64, 0x43 = 67)
        if (address < 64 || address > 67) {
            hasErrors = true;
            Serial.printf("Row %d: Address outside allowed interval (40-43): 0x%02X\n", lineNumber, address);
            logConfigError("Address outside allowed interval (40-43): " + String(address, HEX), lineNumber);
            continue;
        }

        // Only skips the row if board is missing (does NOT halt the system)
        int boardIndex = address - 64; 
        if (!boardConnected[boardIndex]) {
            Serial.printf("Row %d: Hardware warning: Board 0x%02X is defined but NOT physically connected! Skipping line.\n", lineNumber, address);
            logConfigError("Hardware warning: Board 0x" + String(address, HEX) + " is defined but NOT physically connected! Skipping line.", lineNumber);
            continue; 
        }

        if (port < 0 || port > 15) {
            hasErrors = true;
            Serial.printf("Row %d: Port outside allowed interval (0-15): %d\n", lineNumber, port);
            logConfigError("Port outside allowed interval (0-15): " + String(port), lineNumber);
            continue;
        }
        if (dcc < 1 || dcc > 9999) {
            hasErrors = true;
            Serial.printf("Row %d: DCC outside allowed interval (1-9999): %d\n", lineNumber, dcc);
            logConfigError("DCC outside allowed interval (1-9999): " + String(dcc), lineNumber);
            continue;
        }

        // Convert the type string to uppercase
        for (int i = 0; typeStr[i]; i++) typeStr[i] = toupper(typeStr[i]);

        // ====================
        // SERVO HANDLING
        // ====================
        if (strcmp(typeStr, "SERVO") == 0) {
            int a0, a1, time, enabled;
            if (sscanf(line.c_str(), " %*[^,], %*x, %*d, %*d, %d, %d, %d, %d", &a0, &a1, &time, &enabled) < 4) {
                hasErrors = true;
                Serial.printf("Row %d: Servo missing parameters!\n", lineNumber);
                logConfigError("Servo missing parameters (Need: Angle-0,Angle-1,Move-speed,Enabled)", lineNumber);
                continue;
            }

            // FIXED: SERVO validation matched with nowRail's SERVOMIN (450) and SERVOMAX (2000)
            if (a0 < 0 || a0 > 360 || a1 < 0 || a1 > 360) {
                hasErrors = true;
                Serial.printf("Row %d: Servo pulse values outside allowed interval (0-360 us)!\n", lineNumber);
                logConfigError("Servo pulse values outside allowed interval (0-360 us)", lineNumber);
                continue;
            }
            if (time < 1 || time > 60000) continue;
            if (enabled < 0 || enabled > 1) continue;

            if (enabled == 1) {
                validServos.push_back({address, port, dcc, a0, a1, time});
            }
        } 
        // ====================
        // LED HANDLING
        // ====================
        else if (strcmp(typeStr, "LED") == 0) {
            int logic, effect, maxb, effb, enabled;
            if (sscanf(line.c_str(), " %*[^,], %*x, %*d, %*d, %d, %d, %d, %d, %d", &logic, &effect, &maxb, &effb, &enabled) < 5) {
                hasErrors = true;
                Serial.printf("Row %d: LED missing parameters!\n", lineNumber);
                logConfigError("LED missing parameters", lineNumber);
                continue;
            }

            // LED validation
            if (maxb < 0 || maxb > 4095 || effb < 0 || effb > 4095) {
                hasErrors = true;
                Serial.printf("Row %d: LED brightness outside allowed interval (0-4095)!\n", lineNumber);
                logConfigError("LED brightness outside allowed interval (0-4095)", lineNumber);
                continue;
            }
            if (logic < 0 || logic > 1) continue;
            if (effect < 0 || effect > 4) continue;
            if (enabled < 0 || enabled > 1) continue;

            if (enabled == 1) {
                validLeds.push_back({address, port, dcc, logic, effect, maxb, effb});
            }
        } 
        else {
            hasErrors = true;
            Serial.printf("Row %d: Unknown TYPE (Must be LED or SERVO): %s\n", lineNumber, typeStr);
            logConfigError("Unknown TYPE (Must be LED or SERVO): " + String(typeStr), lineNumber);
        }
    }

    file.close();

    // If critical syntax/format errors occurred, abort execution
    if (hasErrors) {
        Serial.println("Critical syntax error in configuration. No commands executed!");
        return false;
    }

    // Delete error.txt completely if the configuration is accepted (warnings are okay)
    LittleFS.remove("/error.txt");

    // Snygg efteranalys och klassificering av korten till Serial Monitor (Från din gamla kod)
    Serial.println("Analyzing and registering components to nowRail...");
    for (int boardAddr = 64; boardAddr <= 67; boardAddr++) {
        bool hasServos = false; bool hasLeds = false;
        for (const auto& s : validServos) { if (s.addr == boardAddr) { hasServos = true; break; } }
        for (const auto& l : validLeds)   { if (l.addr == boardAddr) { hasLeds = true; break; } }

        if (hasServos) {
            Serial.printf("-> Board 0x%02X classified as SERVO. nowRail handling frequencies.\n", boardAddr);
        } else if (hasLeds) {
            Serial.printf("-> Board 0x%02X classified as LED. nowRail handling frequencies.\n", boardAddr);
        }
    }

    // Execute all approved servo and led commands via nowRail
    for (const auto& s : validServos) {
       myLayout.addPCA9685Servo(s.addr, s.port, s.dcc, s.a0, s.a1, s.time);
    }
    for (const auto& l : validLeds) {
        myLayout.addPCA9685Led(l.addr, l.port, l.dcc, l.logic, l.effect, l.maxb, l.effb);
    }

    Serial.printf("SUCCESS: Loaded %d Valid LEDs and %d Valid Servos into nowRail.\n", validLeds.size(), validServos.size());
    return true;
}


// =====================================
// CHECK and RETRY LOAD OF CONFIGURATION
// =====================================
bool loadConfigWithRetry() {
    const int MAX_ATTEMPTS = 5;       // Number of retries (X)
    const unsigned long RETRY_DELAY = 10000; // Waiting time in ms (Y) -> 10 seconds

    for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
        Serial.printf("Attempt %d of %d to load configuration...\n", attempt, MAX_ATTEMPTS);

        if (LittleFS.exists(CONFIG_FILE)) {
            // File exist - run Parser
            if (loadConfig()) {
                return true; // Success!
            }
            // If loadConfig() returned false there is an error in the file. Halt!
               return false; 
            // ESP.restart(); // Restart the Microcontroller
        }

        Serial.println("Configuration file is missing!");
        
        // If last attempt - break
        if (attempt == MAX_ATTEMPTS) break;

        Serial.println("Waiting for Configuration file to be uploaded ...");
        wStatusBlink(7, 200); // Visual indication by internal LED

        // Non-Blocking wait
        unsigned long startWait = millis();
        while (millis() - startWait < RETRY_DELAY) {
            updateStatusBlink();
            delay(1);
            yield();
        }
    }

    return false; // All attempts failed
}

// ========================
//  RELOAD OF CONFIGURATION
// ========================
void checkConfigReload() {
    // Check after intervall (X ms)
    if (millis() - lastCheckTime < FILE_CHECK_INTERVAL) return;
    lastCheckTime = millis();

    // If file not exist reset filesize. Ready for new 
    if (!LittleFS.exists(CONFIG_FILE)) {
        lastFileSize = 0; 
        return;
    }

    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) return;

    size_t currentSize = file.size();
    file.close();

    // First run after File exist 
    if (lastFileSize == 0) {
        lastFileSize = currentSize;
        
        // Load configuration directly (New file or Startup) 
        Serial.println("New configuration file found. Loading...");
        if (loadConfig()) {
            wStatusBlink(2, 300); // Visual indication OK
        } else {
            wStatusBlink(5, 50); // Visual indication FAIL
        }
        return;
    }

    // If filesize changed during run - Reload 
    if (currentSize != lastFileSize) {
        Serial.println("CONFIG CHANGE DETECTED → reloading");
        lastFileSize = currentSize;

        if (loadConfig()) {
            wStatusBlink(2, 300); // Visual indication OK
        } else {
            wStatusBlink(5, 50); // Visual indication FAIL
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

  // XIAO ESP32-S3
  pinMode(XIAO_LED, OUTPUT);
  digitalWrite(XIAO_LED, LED_OFF);
   // Wire.begin(SDA_PIN, SCL_PIN);
   // Wire.begin(SDA, SCL); // Use default PINS
   
  // ========================
  // Check LittleFS
  // ========================
  if (!LittleFS.begin(true)) {
    Serial.println("CRITICAL ERROR: LittleFS mount FAILED!. System on HOLD!");
    wStatusBlink(5, 50); // Visual indication CRITICAL ERROR
    
    // System on HOLD 
    while (true) {
      updateStatusBlink(); // Blink
      delay(1);
      yield();             // Watchdog-timer 
    }
  }

  // Read or create wifi.txt
  initWiFiConfig();

  // ==================================
  // Start AccessPoint & Web if enabled
  // ==================================
  if (wifiStatus == "on") {
    wics_configureWiFi(WIFICHANNEL, true); // nowRail channel 
  } else {
    Serial.println("WICS configuration WiFi CLOSED. (wifi=off in wifi.txt)");
  }
  
    // ==========================
  // Check & Load Configuration
  // ==========================
  if (!loadConfigWithRetry()) {
    logConfigError("Error: No configuration found. System on Hold! REBOOT NEEDED!");
    
    // Visual indication ERROR
    wStatusBlink(5, 50);
    // System on HOLD
    while (true) {
      updateStatusBlink(); // Blink
      delay(1);
      yield();             // Watchdog-timern
    }
  } 
  else {
    // Save the configuration file size 
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (file) {
        lastFileSize = file.size();
        file.close();
    } else {
        lastFileSize = 0;
    }
    
    // And timer (for the chech in loop)
    lastCheckTime = millis();
  }


  // ========================
  // Success - WICS INI Done!
  // ========================
  Serial.println("--- WICS Init Done ---");
  
  // Visual indication OK
  wStatusBlink(2, 300);
  
  // Non blocking Wait for visual indication
  while (blinkRemaining > 0) {
    updateStatusBlink();
    delay(1);
    yield();
  }
}

/// ==============================================
// ==============================================
// WICS RUN FUNCTION (Loop) Called from nowRail
// ==============================================
// ==============================================
void runWics() {
  // Handle WEB requests
      if (wifiStatus == "on") {
      server.handleClient(); 
    }
  updateStatusBlink(); // If internal LED should blink 
  checkConfigReload(); // Check if new config is uploaded
}
