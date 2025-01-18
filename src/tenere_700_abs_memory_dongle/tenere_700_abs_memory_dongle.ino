#include <SPI.h>
#include <mcp2515.h>
#include <EEPROM.h>

struct can_frame canMsg;
struct can_frame absOnMsg;
struct can_frame absOffMsg;
struct can_frame buttonPressMsg;

MCP2515 mcp2515(10);

const unsigned long canId = 0x2A0;
const unsigned long absId = 0x268;    // ABS ECU ID

const unsigned long absInit = 0x18;  // Initialization byte
const int eepromAddress = 0;

const float delayMs = 4;

enum AbsState { ABS_ON, ABS_OFF };
AbsState currentState = ABS_ON;

byte lastState[6];    // Holds the last state read from EEPROM

void processAbsStateChange(AbsState state) {
    switch (state) {
        case ABS_ON:
            Serial.println("ABS ON by default. No state change required.");
            break;
        case ABS_OFF:
            break;
    }
}

void restoreLastSavedState() {
    // Read saved data from EEPROM
    for (int i = 0; i < 6; i++) {
        lastState[i] = EEPROM.read(eepromAddress + i);
    }

    Serial.println("Restored saved ABS state:");
    for (byte i : lastState) {
        Serial.print(i, HEX);
        Serial.print(" ");
    }
    Serial.println();

    if (lastState[5] == 0x1C) { // ABS OFF
        currentState = ABS_OFF;
        Serial.println("Restored state: ABS OFF");
        mcp2515.sendMessage(&buttonPressMsg); // Send button press message
    } else { // Default to ABS ON
        currentState = ABS_ON;
        Serial.println("Restored state: ABS ON");
    }

    processAbsStateChange(currentState);
}

void updateEepromIfChanged(const byte* newData, size_t length) {
    bool dataChanged = false;
    for (size_t i = 0; i < length; i++) {
        if (EEPROM.read(eepromAddress + i) != newData[i]) {
            EEPROM.update(eepromAddress + i, newData[i]);
            dataChanged = true;
        }
    }
    if (dataChanged) {
        Serial.println("EEPROM updated.");
    }
}

void setup() {

  absOnMsg.can_id  = canId;
  absOnMsg.can_dlc = 6;
  absOnMsg.data[0] = 0x00;
  absOnMsg.data[1] = 0x01;
  absOnMsg.data[2] = 0x00;
  absOnMsg.data[3] = 0x00;
  absOnMsg.data[4] = 0x00;
  absOnMsg.data[5] = 0x00;

  absOffMsg.can_id = canId;
  absOffMsg.can_dlc = 6;
  absOffMsg.data[0] = 0x00;
  absOffMsg.data[1] = 0x01;
  absOffMsg.data[2] = 0x00;
  absOffMsg.data[3] = 0x00;
  absOffMsg.data[4] = 0x00;
  absOffMsg.data[5] = 0x1C;

  buttonPressMsg.can_id = canId;
  buttonPressMsg.can_dlc = 1;
  buttonPressMsg.data[0] = 0x80;

  Serial.begin(115200);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();

  Serial.println("Waiting for ABS initialization...");

  while (true) {
    if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      if(canMsg.can_id == absId && canMsg.data[5] == absInit) {
        Serial.println("Received initialization message. Restoring state...");
        delay(1000);
        restoreLastSavedState();
        break;
      }
    }
  }

  Serial.println("Setup complete. Listening for CAN messages...");
}

void loop() {

    if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      if(canMsg.can_id == absId) {
        if (canMsg.data[5] == absInit) {
          Serial.println("Received ABS initialization message. Restoring state...");
          delay(3000);
          restoreLastSavedState();
        } else if (canMsg.data[5] != lastState[5] && canMsg.data[5] != 0x1A) {
          Serial.println("State change detected. Updating EEPROM...");
          updateEepromIfChanged(canMsg.data, canMsg.can_dlc);
          lastState[5] = canMsg.data[5];
        } else if (canMsg.data[5] == 0x1A) {
          Serial.println("Received 0x1A. Sending last saved ABS state...");
          delay(1000);
          processAbsStateChange((AbsState)lastState[5]);
        }
      }
    }
}