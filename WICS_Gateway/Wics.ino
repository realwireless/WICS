// ==============================================
// WICS GATEWAY v1.1.0 addOn for nowRail v2.1.0
// 2026-06-09 (c) Bo Holmqvist
// ==============================================
#include <Wire.h> // Sets in nowrail_user_setup.h
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "LittleFS.h"
#include <WiFi.h>
#include <WebServer.h>

// Internal LED
#define RGB_LED_PIN 8

// CONFIG FILE
#define CONFIG_FILE "/gateway.txt"
#define MAX_RETRIES 5
#define RETRY_DELAY_MS 60000
#define FILE_CHECK_INTERVAL 15000

size_t lastFileSize = 0;
unsigned long lastCheckTime = 0;

// I2C
const int SDA_PIN = 11;
const int SCL_PIN = 10;

// Default RX485E Buttons
int buttonA = 21;
int buttonB = 20;
int buttonC = 19;
int buttonD = 18;

// OLED & System monitor
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define MONITOR_LINES 4
#define LINE_WIDTH 21
char monitorBuffer[MONITOR_LINES][LINE_WIDTH + 1];  // +1 för '\0'

bool oledIsOn = true;
unsigned long lastActivityTime = 0;
#define OLED_SLEEP_TIME 300000  // 5 minuter (300000 ms)

bool showStartupScreen = true;
unsigned long startupTimer = 0;
#define STARTUP_DURATION 5000

// MasterClock
byte clockHour;
byte clockMinutes;
byte clockSeconds;
byte clockDay;
byte clockSpeed;

// Config default values
byte hourA = 0;
byte minuteA = 0;
byte secondA = 0;
byte dayA = 0;
byte clockSpeedB = 1;
byte clockSpeedC = 1;

// Current (starting WIFI channel from nowRail)
uint8_t currentWiFiChannel = WIFICHANNEL;

// Max number of rules that can be running
#define MAX_TIMEDEX 20
#define MAX_TIMEACC 20
#define MAX_CHANNEL_RULES 5

// D-Button settings
#define MAX_D_BUTTON_RULES 12
struct DButtonRule {
  int clicks;
  int dccAddr;
  int logicState;
  bool active;
};
DButtonRule dButtonRules[MAX_D_BUTTON_RULES];
int dButtonRuleCount = 0;

bool countingClicks = false;
unsigned long clickTimer = 0;
int clickCount = 0;
#define CLICK_TIMEOUT 3000 // Tidsfönster i millisekunder (3 sekunder)

// Memory Datastructure
struct TimeDexRule {
  int hour;
  bool hourIsInterval;     // NY: Sparar om timme är ett intervall
  int minute;
  bool minuteIsInterval;   // NY: Sparar om minut är ett intervall
  int second;
  bool secondIsInterval;   // NY: Sparar om sekund är ett intervall
  int day;
  bool dayIsInterval;      // NY: Sparar om dag är ett intervall
  char command[64];
  bool active;
};

struct TimeAccRule {
  int hour;
  bool hourIsInterval;     // NY: Sparar om timme är ett intervall
  int minute;
  bool minuteIsInterval;   // NY: Sparar om minut är ett intervall
  int second;
  bool secondIsInterval;   // NY: Sparar om sekund är ett intervall
  int day;
  bool dayIsInterval;      // NY: Sparar om dag är ett intervall
  int dccAddr;
  int logicState;
  bool active;
};

// Time schedule
TimeDexRule timedexRules[MAX_TIMEDEX];
int timedexCount = 0;

TimeAccRule timeaccRules[MAX_TIMEACC];
int timeaccCount = 0;

// Time Matches and handle -1 as * and interval (*/x)
bool timeMatches(int ruleValue, bool isInterval, byte clockValue) {
  if (!isInterval) {
    return (ruleValue == -1 || ruleValue == clockValue);
  }
  
  // Is interval
  if (ruleValue > 0) {
    return (clockValue % ruleValue == 0);
  }
  
  return false;
}

// Channel rules data structure
struct ChannelRule {
  int dccAddr;
  int logicState;
  int targetChannel;
  bool active;
};

ChannelRule channelRules[MAX_CHANNEL_RULES];
int channelRuleCount = 0;

// Wi-Fi Configuration
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

// ==============================================
// ==============================================
   //       #### WICS FUNCTIONS ###
// ==============================================
// ==============================================

// ======================================
//     BLINK Internal NEOPIXEL LED
// ======================================
void blinkRGB(int times, uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < times; i++) {
    neopixelWrite(RGB_LED_PIN, r, g, b);
    delay(200);
    neopixelWrite(RGB_LED_PIN, 0, 0, 0);
    delay(200);
  }
}

// ======================================
//  Print to Both Serial and OLED     
// ======================================
void printSmart(String text, int textSize = 1, int row = 0, bool clearFirst = true) {
  Serial.println(text);

  if (clearFirst) display.clearDisplay();

  int y = row * (textSize * 8);

  display.setTextSize(textSize);
  display.setCursor(0, y);
  display.println(text);
  display.display();
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


// ======================================
//   DISPLAY BUTTON FEEDBACK (Clicks)
// ======================================
void showButtonMessage(int clicks) {
  display.fillRect(0, 48, SCREEN_WIDTH, 8, SSD1306_BLACK);
  display.setCursor(0, 48); 
  if (clicks > 0) {
    display.printf("Button D: %d clicks...", clicks);
  }
  display.display();
}

// ======================================
//          MASTER CLOCK
// ======================================
void readMasterClock() {
  clockHour = myLayout.rtcHours();
  clockMinutes = myLayout.rtcMinutes();
  clockSeconds = myLayout.rtcSeconds();
  clockDay = myLayout.rtcDays();
  clockSpeed = myLayout.rtcClockSpeed();
}

void updateOledTime() {
  readMasterClock();

  if (clockHour > 23) return;

  char buf[24];

  sprintf(buf, "T:%02d:%02d:%02d D:%d S:%d",
          clockHour,
          clockMinutes,
          clockSeconds,
          clockDay,
          clockSpeed);

  display.fillRect(0, 8, SCREEN_WIDTH, 10, SSD1306_BLACK);
  display.setCursor(0, 8);
  display.setTextSize(1);
  display.println(buf);
  display.display();
}

// ======================================
// TIME PARSER (*, specific time or */interval) WITH VALIDATION
// ======================================
void parseTimeField(const char* str, int &value, bool &isInterval, int maxLimit) {
  // Safe check if string is null or empty
  if (!str || str[0] == '\0') {
    value = -2; // Error code for missing value
    isInterval = false;
    return;
  }

  if (str[0] == '*') {
    if (str[1] == '/') {
      value = atoi(&str[2]); // Extracts the number after e.g. */5
      isInterval = true;
      
      // Validate that the interval interval is reasonable (must be at least 1 and not exceed maximum)
      if (value < 1 || value > maxLimit) {
        value = -2; // Error code for invalid interval
      }
    } else {
      value = -1;            // Pure asterisk (*) means every execution
      isInterval = false;
    }
  } else {
    value = atoi(str);       // Standard number (e.g. 15)
    isInterval = false;
    
    // Validate standard time ranges (e.g. 0-23 for hours, 0-59 for minutes)
    if (value < 0 || value > maxLimit) {
      value = -2; // Error code for value out of range
    }
  }
}

// ======================================
//        CHECK TIME EVENTS
// ======================================
void checkTimeEvents(byte clockHour, byte clockMinute, byte clockSecond, byte clockDay) {
  
  // 1. Check TIMEDEX-rules
  for (int i = 0; i < timedexCount; i++) {
    if (timedexRules[i].active) {
      if (timeMatches(timedexRules[i].hour, timedexRules[i].hourIsInterval, clockHour) &&
          timeMatches(timedexRules[i].minute, timedexRules[i].minuteIsInterval, clockMinute) &&
          timeMatches(timedexRules[i].second, timedexRules[i].secondIsInterval, clockSecond) &&
          timeMatches(timedexRules[i].day, timedexRules[i].dayIsInterval, clockDay)) {
        
        registerActivity(); 
        myLayout.sendDCCEXCustomCmd(timedexRules[i].command);
        char mLine[22];
        formatLine21(mLine, "TIME DCCE 1 S%.7s", timedexRules[i].command);
        oledMonitor(mLine);
        Serial.print("[TIMEDEX TRIGGERED]: ");
        Serial.println(timedexRules[i].command);
      }
    }
  }

  // 2. Check TIMEACC-rules
  for (int i = 0; i < timeaccCount; i++) {
    if (timeaccRules[i].active) {
      if (timeMatches(timeaccRules[i].hour, timeaccRules[i].hourIsInterval, clockHour) &&
          timeMatches(timeaccRules[i].minute, timeaccRules[i].minuteIsInterval, clockMinute) &&
          timeMatches(timeaccRules[i].second, timeaccRules[i].secondIsInterval, clockSecond) &&
          timeMatches(timeaccRules[i].day, timeaccRules[i].dayIsInterval, clockDay)) {
        
        myLayout.sendAccessoryCommand(timeaccRules[i].dccAddr, timeaccRules[i].logicState, MESSRESPREQ);
        
        Serial.print("[TIMEACC TRIGGERED] DCC: ");
        Serial.print(timeaccRules[i].dccAddr);
        Serial.print(" State: ");
        Serial.println(timeaccRules[i].logicState);
      }
    }
  }
}

// =================================
//   HANDLE RX450E A-D BUTTONS
// =================================
void handleButtons() {
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50; 

  if (millis() - lastDebounceTime < debounceDelay) return;

  bool curA = digitalRead(buttonA);
  bool curB = digitalRead(buttonB);
  bool curC = digitalRead(buttonC);
  bool curD = digitalRead(buttonD);

  static bool lastA = LOW;
  static bool lastB = LOW;

  static bool lastC = LOW;
  static bool lastD = LOW;

  // Flag for ButtonChanged
  bool stateChanged = false;

  // Button A Logic
  if (lastA == LOW && curA == HIGH) {
    myLayout.sendClockTimeChange(hourA, minuteA, secondA, dayA);
    Serial.println("Button A pressed");
    stateChanged = true;
  }

  // Button B Logic
  if (lastB == LOW && curB == HIGH) {
    if (clockSpeed != clockSpeedB) {
      myLayout.sendClockSpeedChange(clockSpeedB);
      clockSpeed = clockSpeedB;
    } else {
      myLayout.sendClockSpeedChange(1);
      clockSpeed = 1;
    }
    Serial.println("Button B pressed");
    stateChanged = true;
  }

  // Button C Logic
  if (lastC == LOW && curC == HIGH) {
    myLayout.sendClockSpeedChange(clockSpeedC);
    clockSpeed = clockSpeedC;
    Serial.println("Button C pressed");
    stateChanged = true;
  }

  // Button D Logic
  if (lastD == LOW && curD == HIGH) {
    registerActivity(); 
  
    if (!countingClicks) {
      // First click (0) waking upp and waiting for clicks
      countingClicks = true;
      clickCount = 0; 
      Serial.println("D-Button: Awakened. Click to select...");
    } else {
      // Counting clicks 1 - 9
      clickCount++;   
      if (clickCount > 9) {
        clickCount = 1; // Roll over skip 0
      }
    }
  
    // Update timer for each click
    clickTimer = millis(); 

    // Force update of System monitor
    oledMonitor(""); 

    Serial.printf("Button D pressed. Total clicks: %d\n", clickCount);
    stateChanged = true;
  }

  lastA = curA;
  lastB = curB;
  lastC = curC;
  lastD = curD;

  // Update debounce timer if button is pressed
  if (stateChanged) {
    lastDebounceTime = millis();
  }
}

// ========================
// Execute D-Button command
// ========================
void executeButtonCommand(int clicks) {
  // If 9 click - restart the Microcontroller
  if (clicks == 9) {
    display.fillRect(0, 48, SCREEN_WIDTH, 8, SSD1306_BLACK);
    display.setCursor(0, 48); display.println("RESTARTING...");
    display.display();
    Serial.println("Restart triggered via 9 clicks");
    delay(500);
    ESP.restart();
  }

  // Search for rule in  gateway.txt
  for (int i = 0; i < dButtonRuleCount; i++) {
    if (dButtonRules[i].active && dButtonRules[i].clicks == clicks) {
      
      // Skicka DCC-kommandot
      myLayout.sendAccessoryCommand(
        dButtonRules[i].dccAddr, 
        dButtonRules[i].logicState, 
        MESSRESPNOTREQ
      );
      
      Serial.printf("SUCCESS: Executed D-Button command for %d clicks. Sent DCC: %d, State: %d\n", 
                    clicks, dButtonRules[i].dccAddr, dButtonRules[i].logicState);
      return; 
    }
  }
  
  Serial.printf("No active rule found for %d clicks.\n", clicks);
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
// CONFIG FILE PARSER WITH FULL VALIDATION & ERROR LOGGING 
// ======================================================================
bool loadConfig() {
  // Temporary counters 
  int tempTimedexCount = 0;
  int tempTimeaccCount = 0;
  int tempDButtonRuleCount = 0;
  int tempChannelRuleCount = 0;

  bool hasErrors = false;
  int lineNumber = 0;

  // Clear error.txt
  File clearLog = LittleFS.open("/error.txt", "w");
  if (clearLog) {
    clearLog.close(); 
  }

  File file = LittleFS.open(CONFIG_FILE, "r");
  if (!file) {
    logConfigError("ERROR: Cannot open config file");
    return false;
  }

  Serial.println("Loading config with full validation...");
  char line[128];
  char errBuf[128]; // Buffer för dynamiska felmeddelanden

  while (file.available()) {
    lineNumber++; // Öka radnumret för varje rad som läses
    size_t len = file.readBytesUntil('\n', line, sizeof(line) - 1);
    line[len] = '\0';

    // Remove comments
    char *comment = strchr(line, '#');
    if (comment) *comment = '\0';

    // Remove tab & space
    char *ptr = line;
    while (*ptr == ' ' || *ptr == '\t') ptr++;
    if (*ptr == '\0') continue;

    // Get TYPE
    char type[20];
    if (sscanf(ptr, "%19[^,]", type) < 1) continue;

    // TYPE i CAPITALS
    for (int i = 0; type[i]; i++) type[i] = toupper(type[i]);

    // 1. A-BUTTON (SETCLOCK)
    if (strcmp(type, "A-BUTTON") == 0) {
      int h, m, s, d;
      if (sscanf(ptr, "%*[^,],%d,%d,%d,%d", &h, &m, &s, &d) == 4) {
        if (h >= 0 && h <= 23 && m >= 0 && m <= 59 && s >= 0 && s <= 59 && d >= 0 && d <= 6) {
          hourA = h; minuteA = m; secondA = s; dayA = d;
          Serial.println("A-BUTTON OK");
        } else {
          hasErrors = true;
          snprintf(errBuf, sizeof(errBuf), "A-BUTTON values out of range (H:%d M:%d S:%d D:%d)", h, m, s, d);
          logConfigError(errBuf, lineNumber);
        }
      } else {
        hasErrors = true;
        logConfigError("Malformed A-BUTTON line", lineNumber);
      }
    }
    
    // 2. BUTTONS (ASSIGN PIN)
    else if (strcmp(type, "BUTTONS") == 0) {
      int pinA, pinB, pinC, pinD;
      if (sscanf(ptr, "%*[^,],%d,%d,%d,%d", &pinA, &pinB, &pinC, &pinD) == 4) {
        if (pinA >= 0 && pinA <= 50 && pinB >= 0 && pinB <= 50 && pinC >= 0 && pinC <= 50 && pinD >= 0 && pinD <= 50) {
          buttonA = pinA; buttonB = pinB; buttonC = pinC; buttonD = pinD;
          Serial.printf("BUTTONS OK -> A:%d, B:%d, C:%d, D:%d\n", buttonA, buttonB, buttonC, buttonD);
        } else {
          hasErrors = true;
          snprintf(errBuf, sizeof(errBuf), "BUTTONS GPIO pins out of range: A:%d B:%d C:%d D:%d", pinA, pinB, pinC, pinD);
          logConfigError(errBuf, lineNumber);
        }
      } else {
        hasErrors = true;
        logConfigError("Malformed BUTTONS line", lineNumber);
      }
    }
    
    // 3. B-BUTTON (Toggle Clock Speed)
    else if (strcmp(type, "B-BUTTON") == 0) {
      int v;
      if (sscanf(ptr, "%*[^,],%d", &v) == 1) {
        if (v >= 0 && v <= 255) {
          clockSpeedB = v;
          Serial.println("B-BUTTON OK");
        } else {
          hasErrors = true;
          snprintf(errBuf, sizeof(errBuf), "B-BUTTON out of range (0-255): %d", v);
          logConfigError(errBuf, lineNumber);
        }
      } else {
        hasErrors = true;
        logConfigError("Malformed B-BUTTON line", lineNumber);
      }
    }
    
    // 4. C-BUTTON (Clock Speed)
    else if (strcmp(type, "C-BUTTON") == 0) {
      int v;
      if (sscanf(ptr, "%*[^,],%d", &v) == 1) {
        if (v >= 0 && v <= 255) {
          clockSpeedC = v;
          Serial.println("C-BUTTON OK");
        } else {
          hasErrors = true;
          snprintf(errBuf, sizeof(errBuf), "C-BUTTON out of range (0-255): %d", v);
          logConfigError(errBuf, lineNumber);
        }
      } else {
        hasErrors = true;
        logConfigError("Malformed C-BUTTON line", lineNumber);
      }
    }
    
    // 5. DELAYEDACC
    else if (strcmp(type, "DELAYEDACC") == 0) {
      int tAcc, tLogic, acc, accLogic, delayMs;
      if (sscanf(ptr, "%*[^,],%d,%d,%d,%d,%d", &tAcc, &tLogic, &acc, &accLogic, &delayMs) == 5) {
        if (tAcc >= 1 && tAcc <= 9999 && acc >= 1 && acc <= 9999 && 
            (tLogic == 0 || tLogic == 1) && (accLogic == 0 || accLogic == 1) && 
            delayMs >= 0 && delayMs <= 60000) {
          
          if (!hasErrors) {
            myLayout.addDelayedAccTrigger(tAcc, tLogic, acc, accLogic, delayMs);
            Serial.println("DELAYEDACC OK");
          }
        } else {
          hasErrors = true;
          snprintf(errBuf, sizeof(errBuf), "DELAYEDACC values invalid (Trig:%d L:%d Acc:%d L:%d Delay:%d)", tAcc, tLogic, acc, accLogic, delayMs);
          logConfigError(errBuf, lineNumber);
        }
      } else {
        hasErrors = true;
        logConfigError("Malformed DELAYEDACC line", lineNumber);
      }
    }
    
    // 6. D-BUTTON
    else if (strcmp(type, "D-BUTTON") == 0) {
      int clicks, dccAddr, logicState, active;
      if (sscanf(ptr, "%*[^,],%d,%d,%d,%d", &clicks, &dccAddr, &logicState, &active) == 4) {
        if (active != 1) continue;

        if (clicks >= 1 && clicks <= 9 && dccAddr >= 1 && dccAddr <= 9999 && (logicState == 0 || logicState == 1)) {
          if (tempDButtonRuleCount < MAX_D_BUTTON_RULES) {
            dButtonRules[tempDButtonRuleCount].clicks     = clicks;
            dButtonRules[tempDButtonRuleCount].dccAddr    = dccAddr;
            dButtonRules[tempDButtonRuleCount].logicState = logicState;
            dButtonRules[tempDButtonRuleCount].active     = true;
            tempDButtonRuleCount++;
            Serial.printf("D-BUTTON RULE LOADED: %d clicks -> DCC %d\n", clicks, dccAddr);
          } else {
            hasErrors = true;
            logConfigError("Max D-BUTTON rules exceeded", lineNumber);
          }
        } else {
          hasErrors = true;
          snprintf(errBuf, sizeof(errBuf), "D-BUTTON values invalid (Clicks:%d DCC:%d Logic:%d)", clicks, dccAddr, logicState);
          logConfigError(errBuf, lineNumber);
        }
      } else {
        hasErrors = true;
        logConfigError("Malformed D-BUTTON line", lineNumber);
      }
    }
        // 7. TIMEDEX
    else if (strcmp(type, "TIMEDEX") == 0) {
      char hStr[6], mStr[6], sStr[6], dStr[6];
      char command[64];
      int active;
      if (sscanf(ptr, "%*[^,],%5[^,],%5[^,],%5[^,],%5[^,],%63[^,],%d", hStr, mStr, sStr, dStr, command, &active) == 6) {
        if (active == 1) {
          if (tempTimedexCount < MAX_TIMEDEX) {
            // Pass the max limits: Hours: 23, Minutes: 59, Seconds: 59, Days: 6
            parseTimeField(hStr, timedexRules[tempTimedexCount].hour,   timedexRules[tempTimedexCount].hourIsInterval, 23);
            parseTimeField(mStr, timedexRules[tempTimedexCount].minute, timedexRules[tempTimedexCount].minuteIsInterval, 59);
            parseTimeField(sStr, timedexRules[tempTimedexCount].second, timedexRules[tempTimedexCount].secondIsInterval, 59);
            parseTimeField(dStr, timedexRules[tempTimedexCount].day,    timedexRules[tempTimedexCount].dayIsInterval, 6);
            
            // Check if any of the fields returned an error (-2)
            if (timedexRules[tempTimedexCount].hour == -2 || timedexRules[tempTimedexCount].minute == -2 ||
                timedexRules[tempTimedexCount].second == -2 || timedexRules[tempTimedexCount].day == -2) {
              hasErrors = true;
              logConfigError("TIMEDEX time or interval values out of range", lineNumber);
              continue;
            }

            strncpy(timedexRules[tempTimedexCount].command, command, sizeof(timedexRules[tempTimedexCount].command) - 1);
            timedexRules[tempTimedexCount].active  = true;
            tempTimedexCount++;
            Serial.print("TIMEDEX LOADED: "); Serial.println(command);
          } else {
            hasErrors = true;
            logConfigError("Max TIMEDEX rules exceeded", lineNumber);
          }
        }
      } else {
        hasErrors = true;
        logConfigError("Malformed TIMEDEX line", lineNumber);
      }
    }

    
       // 8. TIMEACC
    else if (strcmp(type, "TIMEACC") == 0) {
      char hStr[6], mStr[6], sStr[6], dStr[6];
      int dccAddr, logicState, active;
      if (sscanf(ptr, "%*[^,],%5[^,],%5[^,],%5[^,],%5[^,],%d,%d,%d", hStr, mStr, sStr, dStr, &dccAddr, &logicState, &active) == 7) {
        if (active == 1) {
          if (dccAddr >= 1 && dccAddr <= 9999 && (logicState == 0 || logicState == 1)) {
            if (tempTimeaccCount < MAX_TIMEACC) {
              // Pass the max limits: Hours: 23, Minutes: 59, Seconds: 59, Days: 6
              parseTimeField(hStr, timeaccRules[tempTimeaccCount].hour,       timeaccRules[tempTimeaccCount].hourIsInterval, 23);
              parseTimeField(mStr, timeaccRules[tempTimeaccCount].minute,     timeaccRules[tempTimeaccCount].minuteIsInterval, 59);
              parseTimeField(sStr, timeaccRules[tempTimeaccCount].second,     timeaccRules[tempTimeaccCount].secondIsInterval, 59);
              parseTimeField(dStr, timeaccRules[tempTimeaccCount].day,        timeaccRules[tempTimeaccCount].dayIsInterval, 6);
              
              // Check if any of the fields returned an error (-2)
              if (timeaccRules[tempTimeaccCount].hour == -2 || timeaccRules[tempTimeaccCount].minute == -2 ||
                  timeaccRules[tempTimeaccCount].second == -2 || timeaccRules[tempTimeaccCount].day == -2) {
                hasErrors = true;
                logConfigError("TIMEACC time or interval values out of range", lineNumber);
                continue;
              }

              timeaccRules[tempTimeaccCount].dccAddr    = dccAddr;
              timeaccRules[tempTimeaccCount].logicState = logicState;
              timeaccRules[tempTimeaccCount].active     = true;
              tempTimeaccCount++;
              Serial.print("TIMEACC LOADED FOR DCC: "); Serial.println(dccAddr);
            } else {
              hasErrors = true;
              logConfigError("Max TIMEACC rules exceeded", lineNumber);
            }
          } else {
            hasErrors = true;
            snprintf(errBuf, sizeof(errBuf), "TIMEACC DCC/Logic invalid (DCC:%d Logic:%d)", dccAddr, logicState);
            logConfigError(errBuf, lineNumber);
          }
        }
      } else {
        hasErrors = true;
        logConfigError("Malformed TIMEACC line", lineNumber);
      }
    }


    // 9. CHANNEL
    else if (strcmp(type, "CHANNEL") == 0) {
      int dccAddr, logicState, targetChannel, active;
      if (sscanf(ptr, "%*[^,],%d,%d,%d,%d", &dccAddr, &logicState, &targetChannel, &active) == 4) {
        if (active == 1) {
          if (dccAddr >= 1 && dccAddr <= 9999 && 
              (logicState == 0 || logicState == 1) && 
              (targetChannel == 1 || targetChannel == 6 || targetChannel == 11)) {
            
            if (tempChannelRuleCount < MAX_CHANNEL_RULES) {
              channelRules[tempChannelRuleCount].dccAddr       = dccAddr;
              channelRules[tempChannelRuleCount].logicState    = logicState;
              channelRules[tempChannelRuleCount].targetChannel = targetChannel;
              channelRules[tempChannelRuleCount].active        = true;
              tempChannelRuleCount++;
              Serial.printf("CHANNEL RULE LOADED: DCC %d\n", dccAddr);
            } else {
              hasErrors = true;
              logConfigError("Max CHANNEL rules exceeded", lineNumber);
            }
          } else {
            hasErrors = true;
            snprintf(errBuf, sizeof(errBuf), "CHANNEL values invalid");
            logConfigError(errBuf, lineNumber);
          }
        }
      } else {
        hasErrors = true;
        logConfigError("Malformed CHANNEL line", lineNumber);
      }
    }

  } // while (file.available())

  file.close();

  // Final check. If errors - Abort!
  if (hasErrors) {
    Serial.println("Critical error in system configuration. No new rules applied!");
    return false;
  }

  // OK - Activate Counters 
  timedexCount = tempTimedexCount;
  timeaccCount = tempTimeaccCount;
  dButtonRuleCount = tempDButtonRuleCount;
  channelRuleCount = tempChannelRuleCount;

  // All Good - Delete Error.txt if exist
  LittleFS.remove("/error.txt");
  Serial.println("Config loaded and fully verified.");
  return true;
}

// ==============================================
// Execute WiFi Channel update if rule exist
// Is called fron nowRail from nowAccComRec
// ==============================================
// Run from nowAccComRec when ACC command is sent out
void wics_checkAndTriggerChannelChange(int accNum, byte accInst) {
  for (int i = 0; i < channelRuleCount; i++) {
    if (channelRules[i].active && 
        channelRules[i].dccAddr == accNum && 
        channelRules[i].logicState == accInst) {
      
      Serial.printf("[WICS] DCC Match found for address %d! Requesting channel change to: %d\n", accNum, channelRules[i].targetChannel);
      
      // Change nowRail channel
      myLayout.changeWifiChannel(channelRules[i].targetChannel);
      break;
    }
  }
}

// ======================================
//       RETRY LOAD CONFIGURATION
// ======================================
bool loadConfigWithRetry() {
  for (int i = 1; i <= MAX_RETRIES; i++) {
    Serial.printf("Attempt %d/%d\n", i, MAX_RETRIES);
    if (LittleFS.exists(CONFIG_FILE)) {
      if (loadConfig()) {
        return true; 
      }
    }
    unsigned long start = millis();
    while (millis() - start < RETRY_DELAY_MS) {
      blinkRGB(1, 0, 0, 255); 
      delay(100);             
    }
  }
  return false; 
}

// ======================================
//        RELOAD CONFIG FILE
// ======================================
void checkConfigReload() {
  if (millis() - lastCheckTime < FILE_CHECK_INTERVAL) return;
  lastCheckTime = millis();

  if (!LittleFS.exists(CONFIG_FILE)) return;

  File file = LittleFS.open(CONFIG_FILE, "r");
  if (!file) return;

  size_t size = file.size();
  file.close();

  if (lastFileSize == 0) {
    lastFileSize = size;
    return;
  }

  if (size != lastFileSize) {
    Serial.println("CONFIG CHANGED");
    lastFileSize = size;
    loadConfig();
    blinkRGB(2, 0, 0, 255); 
  }
}

// ======================================
//   OLED SYSTEM MONITOR FUNCTIONS
// ======================================
void pushMonitorLine(const char* newLine) {
  if (strcmp(monitorBuffer[0], newLine) == 0) return;

  for (int i = MONITOR_LINES - 1; i > 0; i--) {
    strncpy(monitorBuffer[i], monitorBuffer[i - 1], LINE_WIDTH);
  }
  strncpy(monitorBuffer[0], newLine, LINE_WIDTH);
  monitorBuffer[0][LINE_WIDTH] = '\0';
}

void oledOff() {
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  oledIsOn = false;
}

void oledOn() {
  display.ssd1306_command(SSD1306_DISPLAYON);
  oledIsOn = true;
}

void registerActivity() {
  lastActivityTime = millis();
  if (!oledIsOn) oledOn();   
}

// Print to OLED Display 
// Print to OLED Display 
void oledMonitor(const char* mLine) {
  if (strlen(mLine) > 0) {
    pushMonitorLine(mLine);
    registerActivity();   
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // 1. RITA UT "M" FÖRST LÄNGST TILL HÖGER (Pixel 120)
  #ifdef MASTERCLOCK_ON
    display.setCursor(120, 0);
    display.print("M");
  #endif

  // 2. RITA SEDAN UT KLOCKRADEN FRÅN VÄNSTER (Pixel 0)
  readMasterClock();
  char buf[24]; 
  
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d D%d S%d C%d", 
           clockHour, clockMinutes, clockSeconds, clockDay, clockSpeed, currentWiFiChannel); 
  
  display.setCursor(0, 0);
  display.println(buf); // Nu gör vi en vanlig println() då M redan ligger på plats

  // --- Resten av skärmen ritas ut exakt som vanligt ---
  display.setCursor(0, 8);  display.println("Type Addr L Function");
  display.setCursor(0, 16); display.println("---- ---- - -----------");

  // Print rows from buffer (y = 24, 32, 40, 48)
  for (int i = 0; i < MONITOR_LINES; i++) {
    int y = 24 + (i * 8);
    if (i == 0) {
      display.fillRect(0, y, SCREEN_WIDTH, 8, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(0, y); display.println(monitorBuffer[i]);
      display.setTextColor(SSD1306_WHITE);
    } else {
      display.setCursor(0, y); display.println(monitorBuffer[i]);
    }
  }

  // Last row dynamic (y = 56)
  display.setCursor(0, 56);
  if (countingClicks) {
    display.fillRect(0, 56, SCREEN_WIDTH, 8, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.printf("Button D: %d clicks...", clickCount);
    display.setTextColor(SSD1306_WHITE); 
  } else {
    display.println("WICS System Monitor");
  }

  display.display();
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
  
  clockSpeed = 1;
  myLayout.sendClockSpeedChange(clockSpeed);  
  pinMode(RGB_LED_PIN, OUTPUT);

  if (!LittleFS.begin(true)) {
    while (true) { blinkRGB(5, 255, 0, 0); }
  }

  Wire.setPins(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true) { blinkRGB(5, 255, 0, 0); }
  }

  for (int i = 0; i < MONITOR_LINES; i++) {
    memset(monitorBuffer[i], 0, LINE_WIDTH + 1);
  }

  display.setTextColor(SSD1306_WHITE);
  display.cp437(true);
  printSmart("WICS Start", 2, 2, true);
  
  //   // Read or create wifi.txt
  initWiFiConfig();
  
  // ==================================
  // Start AccessPoint & Web if enabled
  // ==================================
  if (wifiStatus == "on") {
    wics_configureWiFi(WIFICHANNEL, true); // nowRail channel 
  } else {
    Serial.println("WICS configuration WiFi CLOSED. (wifi=off in wifi.txt)");
  }

   // ==========================================
  // Check & Load Configuration (Large Board)
  // ==========================================
  if (!loadConfigWithRetry()) {
    // Keep the large board's original display message
    printSmart("CONFIG FAIL", 1, 7, false);
    logConfigError("Error: No configuration found. System on Hold!");
    
    // System on HOLD - Keep the Red RGB Blink
    while (true) { 
      blinkRGB(5, 255, 0, 0); 
      delay(1);
      yield(); // Avoid triggering watchdog
    }
  } else {
    // This prevents the loop from triggering a false reload
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (file) {
      lastFileSize = file.size();
      file.close();
    }
  }

  
  // Set PIN for Buttons A-D
  pinMode(buttonA, INPUT_PULLUP);
  pinMode(buttonB, INPUT_PULLUP);
  pinMode(buttonC, INPUT_PULLUP);
  pinMode(buttonD, INPUT_PULLUP);

  // Success
  printSmart("CONFIG OK", 1, 7, false);
  blinkRGB(2, 0, 255, 0);
  startupTimer = millis();
  oledMonitor("WICS 0123 1 RUNNING  ");
}

// ==============================================
// ==============================================
// WICS RUN FUNCTION (Loop) Called from nowRail
// ==============================================
// ==============================================
void runWics() {
   // Time-Out Control for failed Accessories
   wics_checkAccTimeout(); 

   // RX480E Buttons
   handleButtons();

  // Update OLED System Monitor
  static unsigned long lastMonitorRefresh = 0;
  if (millis() - lastMonitorRefresh > 1000) {
    lastMonitorRefresh = millis(); 
    readMasterClock(); 
    char emptyLine[22] = "";  
    oledMonitor(emptyLine); //Clear system Monitor
  }
    
    //Check Time Events once a second
    //checkTimeEvents(clockHour, clockMinutes, clockSeconds, clockDay);
        
    // Handle WEB requests
      if (wifiStatus == "on") {
      server.handleClient(); 
    }
  
  // Check if new Configuration File need to be read and loaded
  checkConfigReload();

  // OLED Screen Saver
  if (oledIsOn && (millis() - lastActivityTime > OLED_SLEEP_TIME)) {
    oledOff();
  }

  // D-Button - Counting the Clicks
  if (countingClicks && millis() - clickTimer > CLICK_TIMEOUT) {
    countingClicks = false; // Restore
      // Run D-ButtonCommand based on clics
    executeButtonCommand(clickCount); 
      // Clear screen
    showButtonMessage(0); 
  }
}