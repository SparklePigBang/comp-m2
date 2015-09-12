#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <tuple>
#include <vector>

#include "const.hpp"
#include "cpu.hpp"
#include "cursor.hpp"
#include "drawing3D.hpp"
#include "drawing3Db.hpp"
#include "drawing2D.hpp"
#include "output.hpp"
#include "printer.hpp"
#include "ram.hpp"
#include "renderer.hpp"
#include "util.hpp"
#include "view.hpp"

using namespace std;

extern "C" {
  extern volatile sig_atomic_t pleaseExit;
  void setEnvironment();
  void resetEnvironment();
}

void startInteractiveMode();
void drawScreen();
void switchBitUnderCursor();
void eraseByteUnderCursor();
char readStdin(bool drawCursor);
void sleepAndCheckForKey();
void exec();
void run();
string getFreeFileName();
void saveRamToFile();
void switchDrawing();
void userInput();
void prepareOutput();
bool getBool(char c);
void writeInstructionBitToRam(int address, int bitIndex, bool bitValue);
void writeDataBitToRam(int address, int bitIndex, bool bitValue);
void writeLineToRam(string line, int address);
void loadRamFromFileStream(ifstream* fileStream);
void checkIfInputIsPiped();
void loadRamIfFileSpecified(int argc, const char* argv[]);
void selectView();

//////////////////////////
////////// VARS //////////
//////////////////////////

// Two global variables.
bool interactivieMode;
bool executionCanceled = false;

// Main components.
Printer printer;
Ram ram = Ram(printer);
Cpu cpu = Cpu(&ram);

// Graphic representation of the computer's state.
vector<vector<string>> buffer;

// Cycle counter.
int executionCounter = 0;

// Saved state of a ram. Loaded after execution ends.
map<AddrSpace, vector<vector<bool>>> savedRamState;

// Object for keeping track of, and moving around cursor.
Cursor cursor = Cursor(ram);

// Two views.
View VIEW_3D = View(drawing3D, LIGHTBULB_ON_3D, LIGHTBULB_OFF_3D);
View VIEW_3D_B = View(drawing3Db, LIGHTBULB_ON_3D_B, LIGHTBULB_OFF_3D_B);
View VIEW_2D = View(drawing2D, LIGHTBULB_ON_2D, LIGHTBULB_OFF_2D);
View *selectedView = &VIEW_3D;

// Whether next key should be read as a char whose value shall thence be
// inserted into ram.
bool insertChar = false;

//////////////////////////
////////// MAIN //////////
//////////////////////////

int main(int argc, const char* argv[]) {
  srand(time(NULL));
  checkIfInputIsPiped();
  loadRamIfFileSpecified(argc, argv);
  if (interactivieMode) {
    startInteractiveMode();
  } else {
    exec();
  }
}

void startInteractiveMode() {
  selectView();
  setEnvironment();
  prepareOutput();
  clearScreen();
  redrawScreen();
  userInput();
}

//////////////////////////
/////// FUNCTIONS ////////
//////////////////////////

void selectView() {
  const char* term = std::getenv("TERM");
  if (strcmp(term, "linux") == 0) {
    selectedView = &VIEW_3D_B;
  } else if (strcmp(term, "rxvt") == 0) {
    selectedView = &VIEW_2D;
  }

  // if(const char* env_p = std::getenv("PATH"))
  //       std::cout << "Your PATH is: " << env_p << '\n';
}

void drawScreen() {
  buffer = Renderer::renderState(printer, ram, cpu, cursor, *selectedView);
  int i = 0;
  for (vector<string> line : buffer) {
    replaceLine(line, i++);
  }
}

void switchBitUnderCursor() {
  bool bitValue = cursor.getBit(); 
  cursor.setBit(!bitValue);
}

void eraseByteUnderCursor() {
  cursor.setWord(Util::getBoolByte(0));
}

char readStdin(bool drawCursor) {
  char c = 0;
  errno = 0;
  ssize_t num = read(0, &c, 1);
  if (num == -1 && errno == EINTR) {
    // Exits if ctrl-c was pressed.
    if (pleaseExit) {
      exit(0);
    }
    redrawScreen();
    return readStdin(drawCursor);
  }
  return c;
}

/*
 * Runs every cycle.
 */
void sleepAndCheckForKey() {
  usleep(FQ*1000);
  // Exits if ctrl-c was pressed.
  if (pleaseExit) {
    exit(0);
  }
  // Pauses execution if a key was hit, and waits for another key hit.
  if (int keyCode = Util::getKey()) {
    // If escape was pressed.
    if (keyCode == 27) {
      executionCanceled = true;
      return;
    }
    // "Press key to continue."
    keyCode = readStdin(false);
    // If esc key was pressed.
    if (keyCode == 27) {
      executionCanceled = true;
    }
  }
}

void exec() {
  while(!executionCanceled) {
    bool shouldContinue = cpu.step();
    if (interactivieMode) {
      redrawScreen();
    }
    if (!shouldContinue) {
      return;
    }
    if (interactivieMode) {
      sleepAndCheckForKey();
    }
  }
}

/*
 * Saves the state of the ram and starts the execution of a program.
 * When execution stops, due to it reaching last address or user pressing 
 * 'esc', it loads back the saved state of the ram, and resets the cpu.
 */
void run() { 
  if (executionCounter > 0) {
    printer.printEmptyLine();
  }
  savedRamState = ram.state;
  exec();
  // If 'esc' was pressed then it doesn't wait for keypress at the end.
  if (executionCanceled) {
    executionCanceled = false;
  } else {
    readStdin(false);
  }
  ram.state = savedRamState;
  cpu = Cpu(&ram);
  redrawScreen();
  executionCounter++;
}

string getFreeFileName() {
  int i = 0;
  while (Util::fileExists(SAVE_FILE_NAME + to_string(++i)));
  return SAVE_FILE_NAME + to_string(i);
}

void saveRamToFile() {
  string fileName = getFreeFileName();
  ofstream fileStream(fileName);
  fileStream << ram.getString();
  fileStream.close();
}

void switchDrawing() {
  if (*selectedView == VIEW_3D) {
    selectedView = &VIEW_3D_B;
  } else if (*selectedView == VIEW_3D_B) {
    selectedView = &VIEW_2D;
  } else {
    selectedView = &VIEW_3D;
  }
  prepareOutput();
  clearScreen();
  redrawScreen();
  userInput();
}

void userInput() {
  while(1) {
    char c = readStdin(true);
    if (insertChar) {
      insertChar = false;
      if (c == 27) {  // Esc
        continue;
      }
      cursor.setWord(Util::getBoolByte(c));
      cursor.increaseY();
    } else {
      switch (c) {
        // UP
        case 107: // k
        case 65:  // A, part of escape seqence of up arrow
          cursor.decreaseY();
          break;
        // DOWN
        case 106: // j
        case 66:  // B, part of escape seqence of down arrow
          cursor.increaseY();
          break;
        // RIGHT
        case 108: // l
        case 67:  // C, part of escape seqence of rigth arrow
          cursor.increaseX();
          break;
        // LEFT
        case 104: // h
        case 68:  // D, part of escape seqence of left arrow
          cursor.decreaseX();
          break;
        // SWAP UP
        case 75:  // K
        case 53:  // 5, part of escape seqence of page up
          cursor.moveByteUp();
          break;
        // SWAP DOWN
        case 74:  // J
        case 54:  // 6, part of escape seqence of page down
          cursor.moveByteDown();
          break;
        // SAVE
        case 119:  // w
        case 115:  // s
          saveRamToFile();
          break;
        // FLIP
        case 32:  // space
          switchBitUnderCursor();
          break;
        // DELETE
        case 51:  // 3, part of escape seqence of delete key
          eraseByteUnderCursor();
          break;
        // SWITCH ADR SPACE
        case 116:  // t
        case 9:  // tab
          cursor.switchAddressSpace();
          break;
        // RUN
        case 10:  // enter
          run();
          break;
        case 122:  // z
          switchDrawing();
          break;
        case 105: { // i
          if (cursor.getAddressSpace() == DATA) {
            insertChar = true;
          }
          break;
        }
        case 102:  // f
          cursor.setBit(true);
          cursor.increaseX();
          break;
        case 100:  // d
          cursor.setBit(false);
          cursor.increaseX();
          break;
        case 111:  // o
          cursor.increaseY();
          cursor.setBitIndex(0);
      }
    }
    redrawScreen();
  }
}

/*
 * Initializes 'output.cpp' by sending dimensions of a 'drawing' and 
 * a 'drawScreen' callback function, that output.c will use on every 
 * screen redraw.
 */
void prepareOutput() {
  setOutput(&drawScreen, selectedView->width, selectedView->height);
}

bool getBool(char c) {
  return c == '*';
}

void writeInstructionBitToRam(int address, int bitIndex, bool bitValue) {
  ram.state[CODE].at(address).at(bitIndex) = bitValue;
}

void writeDataBitToRam(int address, int bitIndex, bool bitValue) {
  ram.state[DATA].at(address).at(bitIndex) = bitValue;
}

void writeLineToRam(string line, int address) {
  int bitIndex = 0;
  for (char c : line) {
    if (address < RAM_SIZE) { 
      writeInstructionBitToRam(address, bitIndex, getBool(c));
    } else {
      writeDataBitToRam(address-RAM_SIZE, bitIndex, getBool(c));
    }
    if (++bitIndex >= WORD_SIZE) {
      return;
    }
  }
}

void loadRamFromFileStream(ifstream* fileStream) {
  int address = 0;
  while (!fileStream->eof()) {
    string line;
    getline(*fileStream, line);
    bool lineEmptyOrAComment = line.empty() || line[0] == '#';
    if (lineEmptyOrAComment) {
      continue;
    }
    writeLineToRam(line, address);
    if (++address >= 2*RAM_SIZE) {
      return;
    }
  } 
}

void checkIfInputIsPiped() {
  interactivieMode = !Util::inputIsPiped();
}

void loadRamIfFileSpecified(int argc, const char* argv[]) {
  if (argc <= 1) {
    return;
  }
  ifstream fileStream;    
  fileStream.open(argv[1]);   
  if (fileStream.fail()) {
    fprintf(stderr, "Invalid filename '%s'. Aborting ram load.", argv[1]);
  } else {
    loadRamFromFileStream(&fileStream);
    fileStream.close();  
  }
}



