// ==============================================
//     WICS PCA9685 addon to nowRail v1.01
//     2026-06-02 (c) 2026 Bo Holmqvist
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
// CONFIG PARSER FOR SERVO/LED WITH FULL VALIDATION (English Version)
// ======================================================================
bool loadConfig() {
    // Clear error.txt before the new execution
    File clearLog = LittleFS.open("/error.txt", "w");
    if (clearLog) {
        clearLog.close(); 
    }

    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
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

        // Read all 4 common arguments and allow flexible spaces via " %d"
        if (sscanf(line.c_str(), " %15[^,], %d, %d, %d", typeStr, &address, &port, &dcc) < 4) {
            hasErrors = true;
            logConfigError("Format error: (Type,Address,Port,DCC)", lineNumber);
            continue;
        }

        // Common validation of hardware data
        if (address < 40 || address > 43) {
            hasErrors = true;
            logConfigError("Address outside allowed interval (40-43): " + String(address), lineNumber);
            continue;
        }
        if (port < 0 || port > 15) {
            hasErrors = true;
            logConfigError("Port outside allowed interval (0-15): " + String(port), lineNumber);
            continue;
        }
        if (dcc < 1 || dcc > 9999) {
            hasErrors = true;
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
            if (sscanf(line.c_str(), " %*[^,], %*d, %*d, %*d, %d, %d, %d, %d", &a0, &a1, &time, &enabled) < 4) {
                hasErrors = true;
                logConfigError("Servo missing parameters (Need: Angle-0,Angle-1,Move-speed,Enabled)", lineNumber);
                continue;
            }

            // SERVO validation
            if (a0 < 450 || a0 > 2000) {
                hasErrors = true;
                logConfigError("Angle-0 outside allowed interval (450-2000): " + String(a0), lineNumber);
                continue;
            }
            if (a1 < 450 || a1 > 2000) {
                hasErrors = true;
                logConfigError("Angle-1 outside allowed interval (450-2000): " + String(a1), lineNumber);
                continue;
            }
            if (time < 1 || time > 60000) {
                hasErrors = true;
                logConfigError("Move-speed outside allowed interval (1-60000): " + String(time), lineNumber);
                continue;
            }
            if (enabled < 0 || enabled > 1) {
                hasErrors = true;
                logConfigError("Enabled must be 0 or 1: " + String(enabled), lineNumber);
                continue;
            }

            if (enabled == 1) {
                validServos.push_back({address, port, dcc, a0, a1, time});
            }
        } 
        // ====================
        // LED HANDLING
        // ====================
        else if (strcmp(typeStr, "LED") == 0) {
            int logic, effect, maxb, effb, enabled;
            if (sscanf(line.c_str(), " %*[^,], %*d, %*d, %*d, %d, %d, %d, %d, %d", &logic, &effect, &maxb, &effb, &enabled) < 5) {
                hasErrors = true;
                logConfigError("LED missing parameters (Need: Logic,Effect,Max_bright,Effect_bright,Enabled)", lineNumber);
                continue;
            }

            // LED validation
            if (logic < 0 || logic > 1) {
                hasErrors = true;
                logConfigError("Logic must be 0 or 1: " + String(logic), lineNumber);
                continue;
            }
            if (effect < 0 || effect > 4) {
                hasErrors = true;
                logConfigError("Effect outside allowed interval (0-4): " + String(effect), lineNumber);
                continue;
            }
            if (maxb < 0 || maxb > 4095) {
                hasErrors = true;
                logConfigError("Max_bright outside allowed interval (0-4095): " + String(maxb), lineNumber);
                continue;
            }
            if (effb < 0 || effb > 4095) {
                hasErrors = true;
                logConfigError("Effect_bright outside allowed interval (0-4095): " + String(effb), lineNumber);
                continue;
            }
            if (enabled < 0 || enabled > 1) {
                hasErrors = true;
                logConfigError("Enabled must be 0 or 1: " + String(enabled), lineNumber);
                continue;
            }

            if (enabled == 1) {
                validLeds.push_back({address, port, dcc, logic, effect, maxb, effb});
            }
        } 
        // INVALID TYPE
        else {
            hasErrors = true;
            logConfigError("Unknown TYPE (Must be LED or SERVO): " + String(typeStr), lineNumber);
        }
    }

    file.close();

    // If errors occurred, abort execution and do not register any commands
    if (hasErrors) {
        Serial.println("Critical error in configuration. No commands executed!");
        return false;
    }

    // Delete error.txt completely if the configuration is perfect
    LittleFS.remove("/error.txt");

    // Execute all approved servo and led commands
    for (const auto& s : validServos) {
        myLayout.addPCA9685Servo(s.addr, s.port, s.dcc, s.a0, s.a1, s.time);
    }
    for (const auto& l : validLeds) {
        myLayout.addPCA9685Led(l.addr, l.port, l.dcc, l.logic, l.effect, l.maxb, l.effb);
    }

    Serial.println("OK: Configuration validated and executed.");
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
  
  Serial.println("---WICS Init Start---");

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
    logConfigError("Error: No configuration found. System on Hold!");
    
    // Visual indication ERROR
    wStatusBlink(5, 50);
    // System on HOLD
    while (true) {
      updateStatusBlink(); // Blink
      delay(1);
      yield();             // Watchdog-timern
    }
  } else {
    // This prevents the loop from triggering a false reload
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
  
  // Visual indication OK
  wStatusBlink(2, 300);
  
  // Non blocking Wait for visual indication
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
  // Handle WEB requests
      if (wifiStatus == "on") {
      server.handleClient(); 
    }
  updateStatusBlink(); // If internal LED should blink 
  checkConfigReload(); // Check if new config is uploaded
}
