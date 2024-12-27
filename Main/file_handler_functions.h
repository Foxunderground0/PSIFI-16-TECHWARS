#pragma once
#include <LittleFS.h>
#include "utils.h"
#include "file_names.cpp"

extern String key;

inline bool fileExists(const String path) {
  return LittleFS.exists(path);
}

inline void readPersistedDialogue(long long& story_scene, long long& scene_dialogue_count) {
  File file = LittleFS.open("/dialogue_persist.bin", "r");
  if (file) {
    // Read the values from the file
    String content = file.readStringUntil('\n');

    // Extract values from the content
    sscanf(content.c_str(), "%lld", &story_scene);

    // Move the file cursor to the next line
    content = file.readStringUntil('\n');

    // Extract the second value
    sscanf(content.c_str(), "%lld", &scene_dialogue_count);

    file.close();

  } else {
    SerialPrintLn("Failed to open file for reading");
  }
}

inline void updatePersistedDialogue(long long newStoryScene, long long newSceneDialogueCount) {
  File file = LittleFS.open("/dialogue_persist.bin", "w");
  if (file) {
    // Write the new values to the file with newline character
    file.printf("%lld\n%lld\n", newStoryScene, newSceneDialogueCount);
    file.close();
  } else {
    SerialPrintLn("Failed to open file for writing");
  }
}

inline bool checkIfAllDataFileExists() {
  for (const String& filename : verificationFilenames) {
    // Check if the file exists
    bool fileExists = LittleFS.exists(filename);

    // Print the filename and its existence to the serial
    SerialPrint("File: ");
    SerialPrint(filename);
    SerialPrint(" - Exists: ");
    SerialPrintLn(fileExists ? "Yes" : "No");

    if (!fileExists) {
      return false;  // If any file is missing, return false
    }
  }

  return true;  // All files exist
}

bool readNextUnenteredKey() {
  File file = LittleFS.open("/keys.txt", "r");

  if (file) {
    while (file.available()) {
      String line = file.readStringUntil('\n');

      // Check if the line starts with a '/'
      if (line.charAt(0) != '/') {
        key = line.substring(0, line.length() - 1);  // Store the unentered key
        file.close();
        return true;
      }
    }

    // No unentered key found
    key = "";
    file.close();
    return false;
  } else {
    SerialPrintLn("Failed to open keys.txt for reading");
    return false;
  }
}

bool markKeyAsUsed(const String& keyToMark) {
  File file = LittleFS.open("/keys.txt", "r");

  if (file) {
    String content = "";
    while (file.available()) {
      String line = file.readStringUntil('\n');

      // Check if the line matches the key to mark
      if (line.startsWith(keyToMark)) {
        content += '/' + line + '\n';  // Add '/' to the start of the line
      } else {
        content += line + '\n';
      }
    }

    file.close();

    // Write the updated content back to the file
    file = LittleFS.open("/keys.txt", "w");
    if (file) {
      file.print(content);
      file.close();
      return true;
    } else {
      SerialPrintLn("Failed to open keys.txt for writing");
      return false;
    }
  } else {
    SerialPrintLn("Failed to open keys.txt for reading");
    return false;
  }
}
