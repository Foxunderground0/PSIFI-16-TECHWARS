#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPUpdateServer.h>
#include "dialogues.h"
#include "web_handler_functions.h"
#include "file_handler_functions.h"
#include "led_handler_functions.h"
#include "ESP8266TimerInterrupt.h"
#include "file_names.cpp"
#include "utils.h"
#include "buttons.h"
#include <MD_MAX72xx.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
//#include <ESPAsyncTCP.h>
//#include <ESPAsyncWebServer.h>

#define TEST 1                   // Enable Testing Of Hardware
#define STATION_MODE_SELECTOR 0  // WIFI Acesspoint Modes
#define SERIAL_ENABLE 0

#define PWD "00f0f96de7"

const unsigned long TEXTSCROLLDELAY = 80;
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 1

#define CLK_PIN 14   //14 or SCK
#define DATA_PIN 13  //13 or MOSI
#define CS_PIN 12    //12 or SS

// Select a Timer Clock
#define USING_TIM_DIV1 false    // for shortest and most accurate timer
#define USING_TIM_DIV16 false    // for medium time and medium accurate timer
#define USING_TIM_DIV256 true  // for longest timer but least accurate. Default

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

long TIMER_INTERVAL_MS = 4000;  //4ms
unsigned long long lastScrollMillis = 0;

/*Button map

   A
   B  E
   C  D
*/

uint8_t initialBeepCounter = 0;


const int button_pin_a = 15;
const int button_pin_b = 16;
const int button_pin_c = 0;
const int button_pin_d = 4;
const int button_pin_e = 2;

// Variables for button states and last press time
volatile bool state_a = HIGH;
volatile bool state_b = HIGH;
volatile bool state_c = HIGH;
volatile bool state_d = HIGH;
volatile bool state_e = HIGH;

unsigned long lastPressTime_a = 0;
unsigned long lastPressTime_b = 0;
unsigned long lastPressTime_c = 0;
unsigned long lastPressTime_d = 0;
unsigned long lastPressTime_e = 0;

const unsigned long debounceThreshold = 1;  // Adjust the threshold as needed
const unsigned long force_off_time_threshold = 100 - 5;

const int buzzer_pin = 5;
const int led_pin_y = 3;
const int led_pin_g = 7;

unsigned int bright = 0xf;

// For Morse Code

const int dotDuration = 200; // Duration of a dot in milliseconds
const int dashDuration = 600; // Duration of a dash in milliseconds
const int pauseDuration = 200; // Short pause duration in milliseconds
const int wordPauseDuration = 6000; // Pause between words duration in milliseconds
const int characterPauseDuration = 3000;

const char* ssid = "Storm PTCL";
const char* password_wifi = "35348E80687?!";

long long last_beep_millis = 0;
long long threshold_millis = 0;

long long on_beep = 20;
long long off_beep = 1000;

String rawData = "36633";
String teamName = "team_number_6";

bool hasFirstPageBeenAcessed = false;
bool isSomethingDone = false;
bool dialogReady = false;
bool scene_dialogue_completed = true;
bool scan_for_rssi = false;
bool isBeeping = false;
volatile bool is_scrolling = true;
bool isGame = true; // Serves JS game at the start

long long story_scene = 0;
long long scene_dialogue_count = 0;

long ledRunIndex = 0;
bool ledRunState = true;

const String dialogue_file_path = "/dialogue_persist.bin";

String pastDialogue = "--Insert past dialogue here--";
String key = "";  // Global variable to store the key

// Init Server Object
ESP8266WebServer server(80);

// Create an instance of the HTTPUpdateServer class
ESP8266HTTPUpdateServer httpUpdater;

#if STATION_MODE_SELECTOR
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
#endif

uint8_t scroll_state = 0;            // Current state of scrolling
const char* scrolling_text = "TECHWARS  ";
bool scroll_text_ptr_semaphore = 0;
char* scroll_text_ptr = (char*)scrolling_text; // Pointer to the text to be scrolled
uint8_t current_char_width;          // Width of the current character
uint8_t char_buffer[8];              // Character buffer array
uint8_t sub_char_scroll_count = 0;

int getStrengthOfSSID(String ssid_to_scan) {
  int numNetworks = WiFi.scanNetworks();
  if (numNetworks != 0) {
    for (int i = 0; i < numNetworks; i++) {
      if (WiFi.SSID(i) == ssid_to_scan) {
        int strength = WiFi.RSSI(i);
        return strength;
      }
    }
  }
  return -127;
}

double DBToLinear(long long val) {
  return (double)pow(10, val / 10.0);
}

// Beep the buzzer, and alternate the threshold millis depending if currently on beep or off beep
void IRAM_ATTR beep() {
  return;
  if (scan_for_rssi == true) {
    //handleAllButtons();
    if (millis() - last_beep_millis > threshold_millis) {
      isBeeping = !isBeeping;
      digitalWrite(buzzer_pin, isBeeping);
      last_beep_millis = millis();
      isBeeping ? threshold_millis = on_beep : threshold_millis = off_beep;
    }
  }
}

double mapDouble(double x, double in_min, double in_max, double out_min, double out_max) {
  return (double)(x - in_min) * (out_max - out_min) / (double)(in_max - in_min) + out_min;
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  SerialPrintLn("Connected to Wi-Fi sucessfully.");
  SerialPrint("IP address: ");
  SerialPrintLn(WiFi.localIP());
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  SerialPrintLn("Disconnected from Wi-Fi, trying to connect...");
  WiFi.disconnect();
  WiFi.begin(ssid, password_wifi);
}

void beepDot() {
  digitalWrite(buzzer_pin, HIGH);
  mx.control(MD_MAX72XX::INTENSITY, 0xf);
  delay(dotDuration);
  digitalWrite(buzzer_pin, LOW);
  mx.control(MD_MAX72XX::INTENSITY, 0x0);
  delay(pauseDuration);
}

void beepDash() {
  digitalWrite(buzzer_pin, HIGH);
  mx.control(MD_MAX72XX::INTENSITY, 0xf);
  delay(dashDuration);
  digitalWrite(buzzer_pin, LOW);
  mx.control(MD_MAX72XX::INTENSITY, 0x0);
  delay(pauseDuration);
}

int morseCount = 0;
unsigned long long lastCharacterPauseCheck = 0;

void morseCode(String message) {
  if (millis() - lastCharacterPauseCheck > characterPauseDuration) {
    const char morseCodes[36][6] = {
      {'.', '-'},   // A
      {'-', '.', '.', '.'},   // B
      {'-', '.', '-', '.'},   // C
      {'-', '.', '.'},   // D
      {'.', '.'},   // E
      {'.', '.', '-', '.'},   // F
      {'-', '-', '.'},   // G
      {'.', '.', '.', '.'},   // H
      {'.', '.'},   // I
      {'.', '-', '-', '-'},   // J
      {'-', '.', '-'},   // K
      {'.', '-', '.', '.'},   // L
      {'-', '-'},   // M
      {'-', '.'},   // N
      {'-', '-', '-'},   // O
      {'.', '-', '-', '.'},   // P
      {'-', '-', '.', '-'},   // Q
      {'.', '-', '.'},   // R
      {'.', '.', '.'},   // S
      {'-'},   // T
      {'.', '.', '-'},   // U
      {'.', '.', '.', '-'},   // V
      {'.', '-', '-'},   // W
      {'-', '.', '.', '-'},   // X
      {'-', '.', '-', '-'},   // Y
      {'-', '-', '.', '.'},   // Z
      {'.', '-', '.', '-', '-'},   // 1
      {'.', '.', '-', '.', '-'},   // 2
      {'.', '.', '.', '-', '-'},   // 3
      {'.', '.', '.', '.', '-'},   // 4
      {'.', '.', '.', '.', '.'},   // 5
      {'-', '.', '.', '.', '.'},   // 6
      {'-', '-', '.', '.', '.'},   // 7
      {'-', '-', '-', '.', '.'},   // 8
      {'-', '-', '-', '-', '.'},   // 9
      {'-', '-', '-', '-', '-'}    // 0
    };

    if (morseCount >= message.length()) {
      morseCount = 0;
      delay(5000);
    }

    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        mx.setPoint(i, j, true);
      }
    }

    char currentChar = toUpperCase(message[morseCount]);

    int index;
    if (currentChar >= 'A' && currentChar <= 'Z') {
      index = currentChar - 'A';
    } else if (currentChar >= '0' && currentChar <= '9') {
      index = currentChar - '0' + 25; // Map digits to Morse code array
    } else {
      // Handle other characters or invalid input
      index = -1; // Or any value that indicates an error
    }

    // Check if the character is valid before processing Morse code
    if (index != -1) {
      for (int j = 0; j < sizeof(morseCodes[index]) / sizeof(morseCodes[index][0]); j++) {
        if (morseCodes[index][j] == '.') {
          beepDot();
        } else if (morseCodes[index][j] == '-') {
          beepDash();
        }
      }
    }

    lastCharacterPauseCheck = millis();
    morseCount++;
  }
}

void displayBinaryString(String str) {
  int numRows = min(8, (int)str.length());

  for (int row = 0; row < numRows; row++) {
    char currentChar = str[row];
    for (int col = 0; col < 8; col++) {
      int bit = (currentChar >> (7 - col)) & 1; // Reverse the order of columns
      mx.setPoint(row, 7 - col, bit); // Set LEDs starting from the least significant bit
      delay(20);
    }
  }
}

void setup() {

#if SERIAL_ENABLE
  Serial.begin(115200);
#endif

  SerialPrintLn();

#if STATION_MODE_SELECTOR
  //Register wifi event handlers
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
#endif

  // Set the button pins to input
  pinMode(button_pin_a, INPUT);
  //pinMode(button_pin_b, INPUT);
  pinMode(button_pin_c, INPUT);
  pinMode(button_pin_d, INPUT);
  pinMode(button_pin_e, INPUT);

  pinMode(buzzer_pin, OUTPUT);
  //pinMode(led_pin_y, OUTPUT);
  //pinMode(led_pin_g, OUTPUT);
  digitalWrite(buzzer_pin, LOW);
  //digitalWrite(led_pin_y, LOW);
  //digitalWrite(led_pin_g, LOW);

  randomSeed(37);

  // MATRIX BEGIN
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 0xf);

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      mx.setPoint(ledRunIndex / 8, ledRunIndex % 8, ledRunState);
      ledRunIndex++;
    }
  }
  ledRunIndex = 0;

  // WIFI CONFIG
  if (STATION_MODE_SELECTOR) {
    // Connect to the "Storm PTCL" WiFi network with the specified password_wifi
    WiFi.begin(ssid, password_wifi);
    SerialPrint("Connecting to WiFi ");
    while (WiFi.status() != WL_CONNECTED) {
      SerialPrint(".");
      scrollTextBlocking("Booting");
      //delay(100);
    }

    SerialPrintLn(".");
    SerialPrintLn("Connected to WiFi");
    SerialPrint("IP address: ");
    SerialPrintLn(WiFi.localIP());
  } else {
    // Configure the Access Point
    WiFi.softAP("CARDY000006", PWD, random(1, 12), 0);


    // Get the IP address of the Access Point
    IPAddress apIP = WiFi.softAPIP();
    SerialPrint("Access Point IP address: ");
    SerialPrintLn(apIP);
  }

  // File system initialised
  if (!LittleFS.begin()) {
    SerialPrintLn("LittleFS initialization failed!");
  } else {
    SerialPrintLn("LittleFS OK");
  }

  if (!checkIfAllDataFileExists()) {
    SerialPrintLn("Not all required files are on little FS!");
  } else {
    SerialPrintLn("Data Files on Little FS OK");
  }

  SerialPrintLn("Reading presistant data");
  if (fileExists(dialogue_file_path)) {
    readPersistedDialogue(story_scene, scene_dialogue_count);
    SerialPrintLn("File exists. Values read from file:");
    SerialPrintLn("story_scene: " + String(story_scene));
    SerialPrintLn("scene_dialogue_count: " + String(scene_dialogue_count));
  } else {
    // If the file doesn't exist, create it with initial data "0,0"
    updatePersistedDialogue(0LL, 0LL);
    SerialPrintLn("File doesn't exist. Created with initial values 0, 0.");
  }
  SerialPrintLn("File reading done");

  // OTA CONFIG

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");


  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    SerialPrintLn("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    SerialPrintLn("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      SerialPrintLn("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      SerialPrintLn("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      SerialPrintLn("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      SerialPrintLn("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      SerialPrintLn("End Failed");
    }
  });

  ArduinoOTA.begin();
  SerialPrintLn("OTA OK");

  // TIMER CONFIG
  /*
    if (ITimer.attachInterruptInterval(10000, beep)) { // 1ms
    SerialPrintLn("ITimer OK");
    } else {
    SerialPrintLn("Can't set ITimer correctly");
    }
  */
  /*

    if (ITimer.attachInterruptInterval(1000, scrollTextNonBlocking)) { //Scroll Text on the display
    SerialPrintLn("Scroll Interrupt OK");
    } else {
    SerialPrintLn("Can't set Scroll Interrupts");
    }
  */

  /*
    if (ITimer.attachInterruptInterval(100 * 1000, checkForceOffTimeThreshold)) { //100ms
      SerialPrintLn("Force On Interrupt OK");
    } else {
      SerialPrintLn("Can't set Force Off Interrupts");
    }
    //ITimer.disableTimer();
  */

  // SERVER CONFIG
  server.on("/", HTTP_GET, [&]() {
    handleRoot(server, dialogReady, isGame);
 // hasFirstPageBeenAcessed = true;
  });

  server.on("/bootTime", HTTP_GET, [&]() {
    handleBootTime(server);
  });

  server.on("/entered", HTTP_GET, [&]() {
    handleCMD(server, teamName, buzzer_pin);
  });

  server.on("/rawData", HTTP_GET, [&]() {
    handleRawData(server, rawData);
  });

  server.on("/dialogReady", HTTP_GET, [&]() {
    handleDialogReady(server, dialogReady, scene_dialogue_completed);
  });

  server.on("/pastDialogue", HTTP_GET, [&]() {
    handlePastDialogue(server, dialogues, scene_dialogue_completed, story_scene, scene_dialogue_count);
  });

  server.on("/latestDialogue", HTTP_GET, [&]() {
    handleLatestDialogue(server, dialogues, buzzer_pin, story_scene, scene_dialogue_count, dialogues_count, dialogReady, scene_dialogue_completed, scan_for_rssi);
  });

  server.on("/littleFS", HTTP_GET, [&]() {
    handleFSContent(server, dialogue_file_path);
  });

  server.on("/video", HTTP_GET, [&]() {
    handleMKV(server);
  });

  server.on("/reset", HTTP_GET, [&]() {
    ESP.reset();
  });

  server.on("/verified", HTTP_GET, [&]() {
    handleVerified(server, isGame);
  });

  server.on("/getLoginInfo", HTTP_GET, [&]() {
    handleLoginInfo(server);
  });

  server.on("/defuse", HTTP_GET, [&]() {
    handleDefuse(server);
  });

  server.on("/qr", HTTP_GET, [&]() {
    handleQR(server);
  });
  

  server.onNotFound([]() {
    serveFileIfExists(server);
  });

  server.begin();
  SerialPrintLn("Web Server OK");

  // Setup the HTTPUpdateServer
  httpUpdater.setup(&server);
  SerialPrintLn("HTTP OTA Update Server OK");

  MDNS.begin("cardy");
  SerialPrintLn("MDNS Server OK");

  // Output Chip ID
  SerialPrint("Chip ID: 0x");
  SerialPrintLn2(ESP.getChipId(), HEX);

  if (TEST) {
    // Set Led Pins High
    //digitalWrite(led_pin_y, HIGH);
    //digitalWrite(led_pin_g, HIGH);

    //Test Buzzer
    digitalWrite(buzzer_pin, HIGH);
    delay(100);
    digitalWrite(buzzer_pin, LOW);

    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        mx.setPoint(i, j, true);
        delay(10);
      }
    }

    for (char a = 0; a < 0xf; a++) {
      mx.control(MD_MAX72XX::INTENSITY, a);
      delay(50);
    }

    for (char a = 0xf; a > 0x0; a--) {
      mx.control(MD_MAX72XX::INTENSITY, a);
      delay(50);
    }

    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        mx.setPoint(i, j, false);
        delay(20);
      }
    }

    //scrollTextBlocking(("Booted in: " + String(micros())).c_str());

    // Set LED pins low
    //digitalWrite(led_pin_y, LOW);
    //digitalWrite(led_pin_g, LOW);

    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        mx.setPoint(i, j, false);
        delay(10);
      }
    }

  } else {
    // Set matrix to all on
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        mx.setPoint(i, j, true);
        delay(20);
      }
    }
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        mx.setPoint(i, j, false);
        delay(20);
      }
    }
  }

  delay(100);

  SerialPrint("Boot Completed in: ");
  SerialPrint(millis());
  SerialPrint("ms ");
  SerialPrint(micros());
  SerialPrintLn("us");

  //String message = "35348E80687"; // Replace this with the string you want to encode
  //morseCode(message);

  //String message = "ABCDEFGH"; // Replace this with your 8-character string
  //displayBinaryString(message);
  //delay(100000);

  /*
    while (readNextUnenteredKey()) {
      displayBinaryString(key);  // Process the key
      markKeyAsUsed(key);
      delay(4000);
    }
  */
  readNextUnenteredKey();

  if (story_scene == 0) {
    updateScrollTextNonBlocking("PWD On me     ");
    while (story_scene == 0 && WiFi.softAPgetStationNum() <= 0) {
      scrollTextNonBlocking();
      delay(10);
    }

    scene_dialogue_count = 0;
    story_scene = 1;

    updateScrollTextNonBlocking("Go to http://cardy.local/   ");
   // while (story_scene == 0 && 	hasFirstPageBeenAcessed == false) {
   //   scrollTextNonBlocking();
   //   server.handleClient();
   //   delay(10);
   // }

  }
}

void loop() {
  if (scene_dialogue_completed == true) {
    server.handleClient();
    if (scan_for_rssi == true) {
      isSomethingDone = true;
      //ITimer.enableTimer();


      int signalStrength = getStrengthOfSSID("HP-LASERJET-1881");

      for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
          mx.setPoint(i, j, false);
          //delay(20);
        }
      }

      long duration = mapDouble(max(signalStrength, -90), -90, -20, 2000, 1);  // Map signal strength to duration
      mx.control(MD_MAX72XX::INTENSITY, map(max(signalStrength, -90), -90, -20, 0x0, 0xf));
      //off_beep = duration;
      for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
          mx.setPoint(ledRunIndex / 8, ledRunIndex % 8, ledRunState);
          ledRunIndex++;

          if (ledRunIndex == map(max(signalStrength, -90), -90, -20, 0, 64)) {
            break;
          }
        }
        if (ledRunIndex == map(max(signalStrength, -90), -90, -20, 0, 64)) {
          break;
        }
      }

      ledRunIndex = 0;
      //ITimer.setInterval(duration * 1000, beep);
      SerialPrintLn(duration);

      if (signalStrength <= -20) {
        // Start beeping
        //isBeeping = true;
      } else {
        // Stop beeping when signal strength is greater than -20
        isBeeping = false;
        dialogReady = true;
        scan_for_rssi = false;
      }
    }

    if (story_scene == 5) {
      isSomethingDone = true;
      String message = "13218"; // Replace this with the string you want to encode
      morseCode(message);
      server.handleClient();
      //delay(100);
    }

    if (story_scene == 12) {  // final stage
      isSomethingDone = true;
      if (analogRead(A0) > 1020) {  // if the resistor is cut
        digitalWrite(buzzer_pin, HIGH);
        delay(20);
        digitalWrite(buzzer_pin, LOW);
        delay(50);
        digitalWrite(buzzer_pin, HIGH);
        delay(20);
        digitalWrite(buzzer_pin, LOW);
        delay(50);
        digitalWrite(buzzer_pin, HIGH);
        delay(20);
        digitalWrite(buzzer_pin, LOW);
        

        dialogReady = true;
      }
      server.handleClient();
      //delay(100);
    }

    if (story_scene == 8) {
      isSomethingDone = true;
      String message = "8js14Lac"; // Replace this with your 8-character string
      displayBinaryString(message);
      server.handleClient();
    }
  }

  if (!scan_for_rssi && isSomethingDone != true) {
    scrollTextNonBlocking();
    mx.control(MD_MAX72XX::INTENSITY, bright);
  }


    if (state_a == LOW && story_scene == 1 && initialBeepCounter < 200) {
    digitalWrite(buzzer_pin, HIGH);
    delay(100);
    digitalWrite(buzzer_pin, LOW);
    dialogReady = true;
    //scene_dialogue_completed = true;
    //scan_for_rssi = true;
    delay(100);
    initialBeepCounter++;
  }

  /*
    if (state_a == LOW || state_b == LOW || state_c == LOW || state_d == LOW || state_e == LOW) {
      // If any button is pressed, set intensity to 0xf
      mx.control(MD_MAX72XX::INTENSITY, 0xf);
    } else {
      // If no button is pressed, set intensity to 0x0
      mx.control(MD_MAX72XX::INTENSITY, 0x0);
    }
  */

  // Adjust bright variable based on button states
  if (state_b == LOW && bright > 0x0) {
    bright--;
    delay(100);
  }

  if (state_c == LOW && bright < 0xF) {
    bright++;
    delay(100);
  }

  if (state_d == LOW) {
    digitalWrite(buzzer_pin, HIGH);
  } else {
    digitalWrite(buzzer_pin, LOW);
  }

  /*
    for (char a = 0; a < 0xf; a++) {
      mx.control(MD_MAX72XX::INTENSITY, a);
      delay(2);
    }

    for (char a = 0xf; a > 0x0; a--) {
      mx.control(MD_MAX72XX::INTENSITY, a);
      delay(2);
    }
  */



  server.handleClient();
  ArduinoOTA.handle();
  isSomethingDone = false;

  delay(10); // This is necessary for stability
}
