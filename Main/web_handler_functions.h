#pragma once
#include <LittleFS.h>
#include "file_handler_functions.h"
#include "utils.h"

#include "ESP8266TimerInterrupt.h"

extern long long story_scene;
extern bool isBeeping;
extern void IRAM_ATTR beep();
extern bool dialogReady;

String login;
String password;

// Init ESP8266 timer 1
ESP8266Timer ITimer;

extern void updateScrollTextNonBlocking(const char* p);
inline void handleRoot(ESP8266WebServer& server, bool& dialogReady, bool& isGame) {
  if (isGame == true && story_scene == 1) {
    const char* gzFilePath = "/game.html.gz";  // Adjust the path as needed

    // Open the compressed file from LittleFS
    File gzFile = LittleFS.open(gzFilePath, "r");
    if (!gzFile) {
      server.send(500, "text/plain", "Failed to open compressed file for reading");
      return;
    }

    // Send the gzipped file content
    server.streamFile(gzFile, "text/html");

    // Close the file
    gzFile.close();

    updateScrollTextNonBlocking("Feeling confident in your wits, are you? Let's see how you measure up against my easiest challenge   ");
  } else {
    const char* gzFilePath = "/index.html.gz";  // Adjust the path as needed

    // Open the compressed file from LittleFS
    File gzFile = LittleFS.open(gzFilePath, "r");
    if (!gzFile) {
      server.send(500, "text/plain", "Failed to open compressed file for reading");
      return;
    }

    // -- Apparently it automatically detects that the encoding is .gz so we dont explicitly need to define this --
    // Set the Content-Encoding header to indicate gzip compression
    //server.sendHeader("Content-Encoding", "gzip");

    // Send the gzipped file content
    server.streamFile(gzFile, "text/html");

    // Close the file
    gzFile.close();

    // Set dialogReady to true after sending the response
    dialogReady = true;
  }
}

inline void handleDefuse(ESP8266WebServer& server) {
  if (story_scene == 6) {
    const char* gzFilePath = "/login.html.gz";  // Adjust the path as needed

    // Open the compressed file from LittleFS
    File gzFile = LittleFS.open(gzFilePath, "r");
    if (!gzFile) {
      server.send(500, "text/plain", "Failed to open compressed file for reading");
      return;
    }

    // Send the gzipped file content
    server.streamFile(gzFile, "text/html");

    // Close the file
    gzFile.close();
  } else {
    server.send(403, "text/plain", "Hold your horses my love\nBaki ke challanges karlo pehlay");
  }
}

inline void handleMKV(ESP8266WebServer& server) {
  const char* filePath = "/output_final.mkv";  // Adjust the path as needed

  // Open the file from LittleFS
  File file = LittleFS.open(filePath, "r");
  if (!file) {
    server.send(500, "text/plain", "Failed to open file for reading");
    return;
  }

  // Get the filename from the path
  String filename = String(filePath);
  int lastSlashIndex = filename.lastIndexOf('/');
  if (lastSlashIndex != -1) {
    filename = filename.substring(lastSlashIndex + 1);
  }

  // Set the Content-Disposition header to suggest a filename
  server.sendHeader("Content-Disposition", "inline; filename=" + filename);

  // Send the file content
  server.streamFile(file, "video/x-matroska");

  // Close the file
  file.close();
}

inline void handleBootTime(ESP8266WebServer& server) {
  // Respond with the current value of millis
  server.send(200, "text/plain", String(millis()) + "." + String(micros()).substring(String(micros()).length() - 3));
}

inline void handleFSContent(ESP8266WebServer& server, const String& dialogue_file_path) {
  File file = LittleFS.open(dialogue_file_path, "r");
  if (file) {
    // Read the content of the file
    String fileContent = file.readString();
    file.close();

    // Send the file content as the response
    server.send(200, "text/plain", fileContent);
  } else {
    // If the file cannot be opened, send an error response
    server.send(500, "text/plain", "Failed to open file for reading");
  }
}

inline void handleDialogReady(ESP8266WebServer& server, bool& dialogReady, bool& scene_dialogue_completed) {
  server.send(200, "text/plain", (dialogReady ? "1" : "0"));
  scene_dialogue_completed = !dialogReady;  // Update the scene_dialogue_completed based on dialogReady
  dialogReady = false;
}

void handlePastDialogue(ESP8266WebServer& server, const String(&dialogues)[][20], bool& scene_dialogue_completed, long long& story_scene, long long& scene_dialogue_count) {
  String response = "";

  // Concatenate strings from dialogues[story_scene][0] to dialogues[story_scene][scene_dialogue_count-1]
  for (long long i = 0; i < scene_dialogue_count; ++i) {
    response += dialogues[story_scene][i];

    // You can add a separator between dialogues if needed, for example, a newline character
    response += "\n";
  }

  server.send(200, "text/plain", response);
  //scene_dialogue_completed = true;
}

inline void handleLatestDialogue(ESP8266WebServer& server, const String(&dialogues)[][20], const int& buzzer_pin, long long& story_scene, long long& scene_dialogue_count, int dialogues_count[], bool& dialogReady, bool& scene_dialogue_completed, bool& scan_for_rssi) {
  scene_dialogue_completed = false;
  String response = dialogues[story_scene][scene_dialogue_count];
  long long num_of_characters = response.length();
  server.send(200, "text/plain", response);

  //Create the buzzer chirping sound
  unsigned long startTime = millis();
  unsigned long elapsedTime = 0;  // Initialize elapsed time to zero

  while (elapsedTime < num_of_characters * 65) {
    unsigned long currentTime = millis();
    elapsedTime = currentTime - startTime;

    int randomFrequency = random(300, 2000);  // Random frequency between 300Hz and 1000Hz
    int randomDelay = random(10, 200);        // Random delay between 10ms and 500ms
    int randomToneTime = random(10, 200);     // Random delay between 10ms and 500ms

    if (elapsedTime + randomDelay < num_of_characters * 65) {
      // Play a tone with the random frequency
      tone(buzzer_pin, randomFrequency, randomToneTime);  // Play the tone for 50ms
      delay(randomDelay);                                 // Add the random delay
    }
  }

  // Done chirping
  noTone(buzzer_pin);

  // Done chirping
  digitalWrite(buzzer_pin, LOW);

  scene_dialogue_count++;

  if (scene_dialogue_count == dialogues_count[story_scene]) {
    scene_dialogue_count = 0;
    story_scene++;
    dialogReady = false;
    scene_dialogue_completed = true;

    if (story_scene == 4) {
      scan_for_rssi = true;
      isBeeping = true;

      if (ITimer.attachInterruptInterval(10000, beep)) {  // 1ms
        SerialPrintLn("ITimer OK");
      } else {
        SerialPrintLn("Can't set ITimer correctly");
      }
    }

  } else {
    dialogReady = true;
  }

  updatePersistedDialogue(story_scene, scene_dialogue_count);
}

inline void handleCMD(ESP8266WebServer& server, const String& teamName, const int& buzzer_pin) {
  String command = server.arg("command");
  SerialPrintLn(command);
  String response = "";

  if (command == "help") {
    response = "stats : Gives System Info";
  } else if (command == "stats") {
    response =
      "<br><div style=\"white-space: pre;\"><span class=\"red\">                                ,(###########                    </span>  | <span class=\"blue\">System Uptime:</span> " + String(millis()) + "." + String(micros()).substring(String(micros()).length() - 3) + " ms <br>" + "<span class=\"red\">                    ######(##         ############               </span>  | <span class=\"blue\">User:</span> " + teamName + "<br>" + "<span class=\"red\">          ###    ##################(      *(#########            </span>  | <span class=\"blue\">IP:</span> " + String(WiFi.localIP().toString().c_str()) + "<br>" + "<span class=\"red\">        ###      ######################(      #########,         </span>  | <span class=\"blue\">Score:</span> " + String(story_scene * 10) + " <br>" + "<span class=\"red\">       ##.              #(#################(     ########        </span>  | <span class=\"blue\">CPU:</span> Tensilica L106 <br>" + "<span class=\"red\">     ,##                       .##############.    (######(      </span>  | <span class=\"blue\">Architecture:</span> 32-bit RISC <br>" + "<span class=\"red\">     ##     #################       ############(    (######     </span>  | <span class=\"blue\">Frequency:</span> 160 MHz <br>" + "<span class=\"red\">    (#   ,######################(#     ###########(    ######    </span>  | <span class=\"blue\">Flash:</span> " + String(ESP.getFreeSketchSpace()) + " / 4,194,304 <br>" + "<span class=\"red\">   ###  #############################     ###########   .####*   </span>  | <span class=\"blue\">Free Heap:</span> " + String(ESP.getFreeHeap()) + "<br>" + "<span class=\"red\">   ##   ########,        ##############/    #########(    ####   </span>  | <span class=\"blue\">Kernel:</span>  Fox-OS <br>" + "<span class=\"red\">   ##   ##########(#(        .(##########(    #########    ###   </span>  | <br>" + "<span class=\"red\">   ##    ##################      #########(    #########    #    </span>  | <br>" + "<span class=\"red\">   ###     ###################     #########*   ########(        </span>  | <br>" + "<span class=\"red\">    ##        ,#################    (########/   #########       </span>  | <br>" + "<span class=\"red\">    .##                ###########   (########    ########,      </span>  | <br>" + "<span class=\"red\">     *##                 ##########   #########   #########      </span>  | <br>" + "<span class=\"red\">       #(    .#######,    /########    ########   .#####(.       </span>  | <br>" + "<span class=\"red\">        (#(  #########     ########    ########    ####/         </span>  | <br>" + "<span class=\"red\">          ##( #######     (########    #######(                  </span>  | <br>" + "<span class=\"red\">            ####         #########(   #########       (##        </span>  | <br>" + "<span class=\"red\">               (###       ########    #######      ####          </span>  | <br>" + "<span class=\"red\">                  .#####                      #####              </span>  | <br>" + "<span class=\"red\">                        (#######(###########(                    </span>  | </div>";
  } else {

    response = "-bash: " + command + ": command not found";
  }

  server.send(200, "text/plain", response);

  digitalWrite(buzzer_pin, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(50);                       // wait for a second
  digitalWrite(buzzer_pin, LOW);   // turn the LED off by making the voltage LOW
}


inline void handleRawData(ESP8266WebServer& server, const String& rawData) {
  // Respond with the current value of rawData
  server.send(200, "text/plain", rawData);
}

inline void handleVerified(ESP8266WebServer& server, bool& isGame) {
  isGame = false;

  // Build an HTML page with a meta refresh tag for redirection
  String htmlResponse = "<html><head><meta http-equiv=\"refresh\" content=\"0;url=/\"></head></html>";

  server.send(200, "text/html", htmlResponse);
}


inline void handleLoginInfo(ESP8266WebServer& server) {
  // Generate 5-digit random numbers for login and password
  login = String(random(10000, 99999));
  password = String(random(10000, 99999));

  key = login + password;
  // Construct the response in JSON-like format
  String response = "{\"username\":\"" + login + "\",\"password\":\"" + password + "\"}";

  // Send the response
  server.send(200, "application/json", response);
}


inline void handleQR(ESP8266WebServer& server) {
  const char* gzFilePath = "/qrcode.png.gz";

  // Open the compressed file from LittleFS
  File gzFile = LittleFS.open(gzFilePath, "r");
  if (!gzFile) {
    server.send(500, "text/plain", "Failed to open compressed file for reading");
    return;
  }

  // Get the file size
  size_t fileSize = gzFile.size();

  // Set the Content-Disposition header to make the browser download the file
  server.sendHeader("Content-Disposition", "attachment; filename=qrcode.png.gz");

  // Set the Content-Encoding header to indicate gzip compression
  server.sendHeader("Content-Encoding", "gzip");

  // Set the Transfer-Encoding header to chunked
  server.sendHeader("Transfer-Encoding", "chunked");

  // Set the Keep-Alive timeout to the maximum value (86400 seconds)
  server.sendHeader("Connection", "Keep-Alive");
  server.sendHeader("Keep-Alive", "timeout=86400");

  // Start the response with a chunked header
  server.send(200, "application/gzip", "");

  // Send the gzipped file content in chunks
  const size_t chunkSize = 1024;
  uint8_t buffer[chunkSize];
  size_t bytesRead;

  unsigned long timeoutMillis = 5000;  // Timeout in milliseconds (adjust as needed)
  unsigned long startTime = millis();

  while ((bytesRead = gzFile.read(buffer, chunkSize)) > 0) {
    char chunkSizeHex[8];
    snprintf(chunkSizeHex, sizeof(chunkSizeHex), "%X\r\n", bytesRead);

    // Send the chunk size in hexadecimal format
    server.sendContent(chunkSizeHex);

    // Send the actual chunk data
    server.sendContent_P(reinterpret_cast<const char*>(buffer), bytesRead);

    // Send CRLF to mark the end of the chunk
    server.sendContent("\r\n");

    // Check if the client has acknowledged the chunks
    if (!server.client().connected() || millis() - startTime > timeoutMillis) {
      // Exit the loop if the client is not connected or the timeout is reached
      break;
    }
  }

  // Send the last chunk with size 0 to indicate the end
  server.sendContent("0\r\n\r\n");

  // End the response
  server.sendContent("");

  // Close the file
  gzFile.close();
}



inline void serveFileIfExists(ESP8266WebServer& server) {
  String endpoint = server.uri();
  endpoint = endpoint.substring(1);

  // Check if the endpoint exists in the array
  for (const String& filename : verificationFilenames) {
    // Remove ".gz" if present
    String fileWithoutGz = filename;
    bool isGzipped = false;

    if (filename.endsWith(".gz")) {
      fileWithoutGz = filename.substring(0, filename.length() - 3);
      isGzipped = true;
    }

    if (fileWithoutGz == endpoint) {
      server.sendHeader("Content-Disposition", "inline; filename=" + filename);
      // Endpoint found in the array, serve the file
      String filePath = "/" + filename;
      if (LittleFS.exists(filePath)) {
        // File exists, serve it
        File file = LittleFS.open(filePath, "r");

        // Set content type based on gzip o0r not
        String contentType = isGzipped ? "application/gzip" : "application/octet-stream";

        server.streamFile(file, "application/octet-stream");

        file.close();
        return;
      } else {
        // File not found, handle accordingly (e.g., send 404)
        server.send(404, "text/plain", "File: " + endpoint + " not found");
        return;
      }
    }
  }

  // If the endpoint is not in the array, send a default response
  server.send(404, "text/plain", "Endpoint: " + endpoint + " not found");
}
