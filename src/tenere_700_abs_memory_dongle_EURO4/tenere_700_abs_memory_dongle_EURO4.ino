#include <SPI.h>
#include <mcp2515.h>
#include <EEPROM.h>

#define FLOOD_PREVENTION 4
#define ONE_SECOND 1000

/*******************************************************************************
*---------------------------- CAN Message examples ----------------------------*
********************************************************************************
Message type | CAN_ID | CAN_DLC | DATA0 | DATA1 | DATA2 | DATA3 | DATA4 | DATA5
ABS ON       | 0x268  | 0x06    | 0x00  | 0x01  | 0x00  | 0x00  | 0x00  | 0x00
ABS OFF      | 0x268  | 0x06    | 0x00  | 0x01  | 0x00  | 0x00  | 0x00  | 0x1C
Button press | 0x2A0  | 0x01    | 0x80
*******************************************************************************/

struct can_frame canMsg;
struct can_frame buttonPressMsg;

MCP2515 mcp2515(10);

const unsigned long canId = 0x2A0;
const unsigned long absId = 0x268; // ABS ECU ID

const unsigned long absInit = 0x18; // Initialization byte
const int eepromAddress = 0;

enum AbsState { ABS_ON, ABS_OFF };
AbsState currentState = ABS_ON;

byte lastState[6]; // Holds the last state read from EEPROM

void processAbsStateChange(AbsState state) {
  if (&state != nullptr) {
    switch (state) {
    case ABS_ON:
      Serial.println("ABS ON by default. No state change required.");
      break;
    case ABS_OFF:
      Serial.println("ABS OFF.");
      break;
    }
  } else {
    Serial.println("Error: state is null.");
  }
}

void sendAbsButtonPressed() {
  for (int i = 0; i < 500; i++) {
    Serial.print("Message sent [");
    Serial.print(i + 1);
    Serial.println("/500]");
    if (mcp2515.sendMessage(&buttonPressMsg) != MCP2515::ERROR_OK) { // Send button press message
      Serial.println("Error sending message from CAN bus.");
    }
    delay(FLOOD_PREVENTION);
  }
}

void restoreLastSavedState() {
  Serial.println("Restoring state...");
  // Read saved data from EEPROM
  for (uint8_t i = 0; i < 6; i++) {
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
    sendAbsButtonPressed();
  } else { // Default to ABS ON
    currentState = ABS_ON;
    Serial.println("Restored state: ABS ON");
  }

  processAbsStateChange(currentState);
}

void updateEepromIfChanged(const byte *newData, size_t length) {
  if (newData != nullptr) {
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
  } else {
    Serial.println("Error: newData is null.");
    return;
  }
}

void setup() {

  buttonPressMsg.can_id  = canId;
  buttonPressMsg.can_dlc = 1;
  buttonPressMsg.data[0] = 0x80;

  Serial.begin(115200);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Serial.println("Setup complete. Listening for CAN messages...");
}

void loop(){
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    if (canMsg.can_id == absId) {
      if (canMsg.data[5] == absInit) {
        Serial.println("Received initialization message.");
        delay(ONE_SECOND);
        restoreLastSavedState();
      } else if (canMsg.data[5] == 0x1A) {
        Serial.println("Received 0x1A. Sending last saved ABS state...");
        delay(ONE_SECOND);
        processAbsStateChange((AbsState)lastState[5]);
      } else if (canMsg.data[5] != lastState[5] && canMsg.data[5] != 0x1A) {
        Serial.println("State change detected. Updating EEPROM...");
        updateEepromIfChanged(canMsg.data, canMsg.can_dlc);
        lastState[5] = canMsg.data[5];
      }
    }
  }
}