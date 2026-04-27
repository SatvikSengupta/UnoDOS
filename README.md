# UnoDOS
A DOS-Like Operating System for the Arduino UNO
Creator: Satvik Sengupta ( https://github.com/SatvikSengupta )


![Arduino](https://img.shields.io/badge/Platform-Arduino_Uno-00979C?style=for-the-badge&logo=arduino)
![RAM](https://img.shields.io/badge/RAM-2048_Bytes-red?style=for-the-badge)
![Storage](https://img.shields.io/badge/Storage-1KB_EEPROM-blue?style=for-the-badge)
![License](https://img.shields.io/badge/License-GPLv3-blue?style=for-the-badge)

UnoDOS transforms a standard Arduino Uno into a mini computer. It features a persistent filesystem, a command-line interface, hardware control, and batch script execution, all running entirely within the ATmega328P's internal memory. 

---

## Features

* **Zero External Dependencies:** Runs on the Arduino's default hardware. No external modules needed.
* **EEPROM File System (EFS):** Transforms the Uno's 1,024 bytes of non-volatile EEPROM into a persistent disk drive.
* **The `AUTOEXEC.BAT` Engine:** Write batch scripts to hardware memory. UnoDOS will parse and execute them line-by-line, including running an auto-execute script on boot.
* **Hardware Abstraction Layer (HAL):** Command physical pins directly from the terminal. 
* **Memory Management:** Operates dynamically within 2 KB of SRAM, featuring an onboard `MEM` command to track Heap/Stack collisions in real-time.

---

## Getting Started

### Installation
1. Clone this repository: `git clone https://github.com/SatvikSengupta/UnoDOS.git`
2. Open `UnoDOS.ino` in the Arduino IDE.
3. Flash to an Arduino Uno.
4. Open the Serial Monitor. Set baud rate to **9600** and line endings to **Both NL & CR**.
5. Type `FORMAT` on first boot to initialize the partition table.
6. Enjoy your new computer

---

## Commands

### System & Disk
| Command | Description |
| :--- | :--- |
| `HELP` | Prints the internal command menu. |
| `MEM` | Calculates exact free SRAM (bytes between the heap and stack). |
| `CLS` | Clears the terminal screen via ANSI escape codes. |
| `FORMAT` | Zeros out the EEPROM and initializes the File Allocation Table. |

### Filesystem (FAT16-Lite)
| Command | Description |
| :--- | :--- |
| `DIR` | Lists files, directories, and exact byte counts in the current folder. |
| `MD [dir]` | Creates a new directory. |
| `CD [dir]` | Changes the working directory (supports `CD ..`). |
| `WRITE [file] [text]` | Appends text to a new file. *(Use `|` for line breaks).* |
| `TYPE [file]` | Reads a file from EEPROM and prints it to the console. |
| `DEL [file]` | Soft-deletes a file from the allocation table. |
| `EDIT [file] [text]` | Appends a new line of text to an existing file. |

### Execution & Hardware
| Command | Description |
| :--- | :--- |
| `RUN [file.bat]` | Executes a text file line-by-line as system commands. |
| `PINMODE [pin] [IN/OUT]`| Configures an Arduino pin. |
| `DWRITE [pin] [HIGH/LOW]`| Sets a digital pin state. |
| `AREAD [pin]` | Returns the analog value (0-1023) of a pin (e.g., `AREAD 14` for A0). |

---

## Architecture

### The 1KB Partition Table
The 1,024-byte EEPROM is split into a **File Allocation Table (FAT)** and a **Data Zone**. 
The FAT occupies the first chunk of memory, capable of storing up to 16 `FileRecord` structs. Each struct is 15-16 bytes, tracking the file type (File/Folder), parent index, an 8-character name, the memory start address, and the exact byte size. Storage uses an append-only architecture to maximize the remaining ~760 bytes for raw text data.

### The Batch Processor Workaround
Because the OS cannot load `.EXE` files into SRAM (AVR architecture prohibits executing code from RAM), UnoDOS relies on text-based script parsing. The kernel reads a file character by character from the EEPROM into a 64-byte command buffer. When it hits a newline (or a `|` pipe character, designed for single-line serial environments), it passes that buffer back into the system's own command router.

### The Autoexec Sequence
If a file named `AUTOEXEC.BAT` exists in the root directory, the kernel bypasses the initial `C:\>` prompt and feeds the script to the batch processor immediately upon boot, allowing for headless hardware configuration.

---

## Limitations
* **Append-Only Memory:** Deleting a file removes its FAT entry, but does not defragment the drive. Reclaiming lost bytes requires a full `FORMAT`.
* **Buffer Limits:** The Serial input buffer is strictly capped at 64 bytes to prevent the Stack from crashing into the Heap. Typing a command longer than 64 characters will be truncated.
  
---

## License
This project is licensed under the **GNU General Public License v3.0 (GPLv3)**. 

---

## Contributing
Feel free to fork this project and submit pull requests. 
