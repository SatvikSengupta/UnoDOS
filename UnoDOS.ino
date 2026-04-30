// UnoDOS 
// https://github.com/SatvikSengupta/UnoDOS
// under GNU GPL V3
#include <EEPROM.h>

#define BUFFER_SIZE 64
#define MAX_FILES 16
#define EEPROM_SIZE 1024

struct FileRecord {
  uint8_t type;         // 0: Empty, 1: File, 2: Folder
  uint8_t parent_idx;   
  char name[9];         
  uint16_t start_addr;  
  uint16_t size;        
}; 

#define FAT_SIZE (MAX_FILES * sizeof(FileRecord))

char cmdBuffer[BUFFER_SIZE];
uint8_t bufferIndex = 0;
uint8_t current_dir = 255; 
char current_dir_name[9] = ""; 

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; } 
  
  Serial.println(F("Starting UnoDOS v1.0..."));
  Serial.println(F("HAL, BATCH, and FAT16 Loaded."));
  Serial.print(F("Mounting Disk... "));
  
  FileRecord firstRecord;
  EEPROM.get(0, firstRecord);
  if (firstRecord.type > 2) {
    Serial.println(F("UNFORMATTED. Type FORMAT."));
  } else {
    Serial.println(F("OK."));
    // Look for AUTOEXEC.BAT on boot
    runBatch("AUTOEXEC.BAT");
  }
  printPrompt();
}

void loop() {
  if (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (bufferIndex > 0) {
        Serial.println(); 
        cmdBuffer[bufferIndex] = '\0'; 
        executeCommand(cmdBuffer);     
      } else {
        Serial.println();
      }
      bufferIndex = 0; 
      printPrompt();
    }
    else if (c == '\b' || c == 127) {
      if (bufferIndex > 0) {
        bufferIndex--;
        Serial.print(F("\b \b")); 
      }
    }
    else if (bufferIndex < BUFFER_SIZE - 1) {
      cmdBuffer[bufferIndex++] = c;
      Serial.print(c); 
    }
  }
}

void printPrompt() {
  Serial.print(F("C:\\"));
  if (current_dir != 255) {
    Serial.print(current_dir_name);
    Serial.print(F("\\"));
  }
  Serial.print(F("> "));
}

int freeMemory() {
  extern int __heap_start, *__brkval;
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// --- BATCH PROCESSOR ---
void runBatch(const char* filename) {
  FileRecord record;
  bool found = false;
  for (int i = 0; i < MAX_FILES; i++) {
    EEPROM.get(i * sizeof(FileRecord), record);
    if (record.type == 1 && record.parent_idx == current_dir && strcmp(record.name, filename) == 0) {
      found = true;
      char lineBuf[BUFFER_SIZE];
      int lineIdx = 0;
      
      for (int j = 0; j < record.size; j++) {
        char c = (char)EEPROM.read(record.start_addr + j);
        if (c == '\n' || c == '\r' || c == '|') { // allow '|' as a line break for easy typing
          if (lineIdx > 0) {
            lineBuf[lineIdx] = '\0';
            executeCommand(lineBuf); // Execute the line
            lineIdx = 0;
            delay(100); // Give the system a tiny breather between commands
          }
        } else {
          if (lineIdx < BUFFER_SIZE - 1) lineBuf[lineIdx++] = c;
        }
      }
      // Catch the last line if it didn't end in a newline
      if (lineIdx > 0) {
        lineBuf[lineIdx] = '\0';
        executeCommand(lineBuf);
      }
      break;
    }
  }
  if (!found && strcmp(filename, "AUTOEXEC.BAT") != 0) Serial.println(F("Batch file not found."));
}

void writeFile(const char* filename, const char* text, bool append) {
  uint16_t next_addr = FAT_SIZE;
  FileRecord record;
  for (int i = 0; i < MAX_FILES; i++) {
    EEPROM.get(i * sizeof(FileRecord), record);
    if (record.type == 1) { 
      uint16_t end_addr = record.start_addr + record.size;
      if (end_addr > next_addr) next_addr = end_addr;
    }
  }
  
  int textLen = strlen(text);
  if (next_addr + textLen > EEPROM_SIZE) {
    Serial.println(F("Error: Disk Full."));
    return;
  }
  
  for (int i = 0; i < textLen; i++) {
    EEPROM.update(next_addr + i, text[i]);
  }
  
  // Find empty slot for FAT
  for (int i = 0; i < MAX_FILES; i++) {
    EEPROM.get(i * sizeof(FileRecord), record);
    if (record.type == 0) { 
      record.type = 1;
      record.parent_idx = current_dir;
      strncpy(record.name, filename, 8);
      record.name[8] = '\0';
      record.start_addr = next_addr;
      record.size = textLen;
      EEPROM.put(i * sizeof(FileRecord), record);
      if(!append) Serial.println(F("File saved."));
      return;
    }
  }
  Serial.println(F("Error: Directory full."));
}

void deleteFile(const char* filename) {
  FileRecord record;
  for (int i = 0; i < MAX_FILES; i++) {
    EEPROM.get(i * sizeof(FileRecord), record);
    if (record.type != 0 && record.parent_idx == current_dir && strcmp(record.name, filename) == 0) {
      record.type = 0; 
      EEPROM.put(i * sizeof(FileRecord), record);
      return;
    }
  }
}

// --- CORE OS LOGIC ---
void executeCommand(char* cmdRaw) {
  char cmdCopy[BUFFER_SIZE];
  strcpy(cmdCopy, cmdRaw); // Preserve raw string for WRITE command
  
  // Tokenize the command
  char* cmd = strtok(cmdCopy, " ");
  char* arg1 = strtok(NULL, " ");
  char* arg2 = strtok(NULL, " ");
  
  if (cmd == NULL) return;
  for (int i = 0; cmd[i]; i++) cmd[i] = toupper(cmd[i]);
  if (arg1 != NULL) for (int i = 0; arg1[i]; i++) arg1[i] = toupper(arg1[i]);
  if (arg2 != NULL) for (int i = 0; arg2[i]; i++) arg2[i] = toupper(arg2[i]);

  // --- COMMAND ROUTER ---
  
  if (strcmp(cmd, "HELP") == 0) {
    Serial.println(F("SYS: MEM, CLS, FORMAT"));
    Serial.println(F("FAT: DIR, MD, CD, WRITE, TYPE, DEL, EDIT"));
    Serial.println(F("EXE: RUN [file]"));
    Serial.println(F("HAL: PINMODE [pin] [IN/OUT], DWRITE [pin] [HIGH/LOW], AREAD [pin]"));
  } 
  else if (strcmp(cmd, "MEM") == 0) {
    Serial.print(F("Free RAM: ")); Serial.println(freeMemory());
  }
  else if (strcmp(cmd, "CLS") == 0) {
    for (int i = 0; i < 50; i++) {
     Serial.println();
    };
  }
  else if (strcmp(cmd, "FORMAT") == 0) {
    for (int i = 0; i < EEPROM_SIZE; i++) { EEPROM.update(i, 0); }
    current_dir = 255; current_dir_name[0] = '\0';
    Serial.println(F("Disk formatted."));
  }
  else if (strcmp(cmd, "DIR") == 0) {
    int fileCount = 0; FileRecord record;
    for (int i = 0; i < MAX_FILES; i++) {
      EEPROM.get(i * sizeof(FileRecord), record);
      if (record.type != 0 && record.parent_idx == current_dir) {
        if (record.type == 2) Serial.print(F("<DIR>    "));
        else Serial.print(F("         "));
        Serial.print(record.name);
        if (record.type == 1) { Serial.print(F("\t\t")); Serial.print(record.size); Serial.print(F(" B")); }
        Serial.println(); fileCount++;
      }
    }
    Serial.print(fileCount); Serial.println(F(" File(s)"));
  }
  // --- DIRECTORY MANAGEMENT ---
  else if (strcmp(cmd, "MD") == 0 && arg1 != NULL) {
     FileRecord record;
     for (int i = 0; i < MAX_FILES; i++) {
        EEPROM.get(i * sizeof(FileRecord), record);
        if (record.type == 0) {
            record.type = 2; record.parent_idx = current_dir;
            strncpy(record.name, arg1, 8); record.name[8] = '\0';
            EEPROM.put(i * sizeof(FileRecord), record); return;
        }
     }
  }
  else if (strcmp(cmd, "CD") == 0 && arg1 != NULL) {
    if (strcmp(arg1, "..") == 0 && current_dir != 255) {
      FileRecord record; EEPROM.get(current_dir * sizeof(FileRecord), record);
      current_dir = record.parent_idx;
      if (current_dir == 255) current_dir_name[0] = '\0';
      else { EEPROM.get(current_dir * sizeof(FileRecord), record); strcpy(current_dir_name, record.name); }
    } else {
      FileRecord record; bool found = false;
      for (int i = 0; i < MAX_FILES; i++) {
        EEPROM.get(i * sizeof(FileRecord), record);
        if (record.type == 2 && record.parent_idx == current_dir && strcmp(record.name, arg1) == 0) {
          current_dir = i; strcpy(current_dir_name, record.name); found = true; break;
        }
      }
      if (!found) Serial.println(F("Path not found."));
    }
  }
  // --- FILE MANAGEMENT ---
  else if (strcmp(cmd, "WRITE") == 0 && arg1 != NULL) {
    char* textStart = strchr(cmdRaw, ' '); // Find first space
    if (textStart) textStart = strchr(textStart + 1, ' '); // Find second space
    if (textStart) writeFile(arg1, textStart + 1, false);
  }
  else if (strcmp(cmd, "TYPE") == 0 && arg1 != NULL) {
    FileRecord record; bool found = false;
    for (int i = 0; i < MAX_FILES; i++) {
      EEPROM.get(i * sizeof(FileRecord), record);
      if (record.type == 1 && record.parent_idx == current_dir && strcmp(record.name, arg1) == 0) {
        for (int j = 0; j < record.size; j++) {
          char c = (char)EEPROM.read(record.start_addr + j);
          if (c == '|') Serial.println(); else Serial.print(c);
        }
        Serial.println(); found = true; break;
      }
    }
    if (!found) Serial.println(F("File not found."));
  }
  else if (strcmp(cmd, "DEL") == 0 && arg1 != NULL) {
    deleteFile(arg1); Serial.println(F("Deleted."));
  }
  else if (strcmp(cmd, "EDIT") == 0 && arg1 != NULL) {
    // Quick append-only editor. Reads text, asks for next line, rewrites file.
    char* textStart = strchr(cmdRaw, ' ');
    if (textStart) textStart = strchr(textStart + 1, ' ');
    if (textStart) {
        // Find existing file, append the new text using the pipe character as a newline
        char newText[BUFFER_SIZE];
        strcpy(newText, "|"); // append newline
        strcat(newText, textStart + 1);
        writeFile(arg1, newText, true); 
        Serial.println(F("Line appended."));
    } else {
        Serial.println(F("Usage: EDIT [file] [new line of text]"));
    }
  }
  // --- BATCH RUNNER ---
  else if (strcmp(cmd, "RUN") == 0 && arg1 != NULL) {
    runBatch(arg1);
  }
  // --- HARDWARE ABSTRACTION LAYER (HAL) ---
  else if (strcmp(cmd, "PINMODE") == 0 && arg1 != NULL && arg2 != NULL) {
    int pin = atoi(arg1);
    if (strcmp(arg2, "OUT") == 0) pinMode(pin, OUTPUT);
    else pinMode(pin, INPUT);
    Serial.println(F("OK"));
  }
  else if (strcmp(cmd, "DWRITE") == 0 && arg1 != NULL && arg2 != NULL) {
    int pin = atoi(arg1);
    if (strcmp(arg2, "HIGH") == 0) digitalWrite(pin, HIGH);
    else digitalWrite(pin, LOW);
    Serial.println(F("OK"));
  }
  else if (strcmp(cmd, "AREAD") == 0 && arg1 != NULL) {
    int pin = atoi(arg1); // Works for "14" (A0) or just pass actual pin number
    Serial.println(analogRead(pin));
  }
  else {
    Serial.println(F("Bad command or syntax."));
  }
}
