#include <Wire.h>

// MXP23017 lib: https://github.com/blemasle/arduino-mcp23017
#include <MCP23017.h>

// MCP23017 IO Expander i2c pins
constexpr int PIN_SDA    = 21;   // ESP32 SDA
constexpr int PIN_SCL    = 22;   // ESP32 SCL
constexpr uint8_t I2C_ADDRESS = 0x20; // I2C Adress of the chip

// Configuration i/o
constexpr int PIN_LED    = 27;   // Status LED
constexpr int PIN_BUTTON = 18;   // Button input

// Action output
constexpr int PIN_RELAY  = 25;   // Relay output

// If not 0 the relay will be pulsed for the given ms. This is usefull for locks that need a short pulse only.
constexpr int RELAY_PULSE = 200; //0; 

constexpr int EMERGENCY_UNLOCK_MS = 3000;

MCP23017 mcp_1(I2C_ADDRESS);
MCP23017 mcp_2(I2C_ADDRESS + 1);

// MCP Output pin configuration
constexpr uint8_t O_A1 = 0;
constexpr uint8_t O_B1 = 1;
constexpr uint8_t O_A2 = 2;
constexpr uint8_t O_B2 = 3;
constexpr uint8_t O_A3 = 4;
constexpr uint8_t O_B3 = 5;
constexpr uint8_t O_A4 = 6;
constexpr uint8_t O_B4 = 7;
constexpr uint8_t O_A5 = 8;
constexpr uint8_t O_B5 = 9;

const uint8_t outputPins[] = {
  0,  // O_A1
  1,  // O_B1
  2,  // O_A2
  3,  // O_B2
  4,  // O_A3  
  5,  // O_B3
  6,  // O_A4
  7,  // O_B4
  16, // O_A5 // 2nd io expander
  17  // O_B5 // 2nd io expander
};
const int numOutputs = sizeof(outputPins) / sizeof(outputPins[0]);

// MCP Input pin configuration
constexpr uint8_t I_C1 = 0;
constexpr uint8_t I_D1 = 1;
constexpr uint8_t I_C2 = 2;
constexpr uint8_t I_D2 = 3;
constexpr uint8_t I_C3 = 4;
constexpr uint8_t I_D3 = 5;
constexpr uint8_t I_C4 = 6;
constexpr uint8_t I_D4 = 7;
constexpr uint8_t I_C5 = 8;
constexpr uint8_t I_D5 = 9;

const uint8_t inputPins[]  = {
  9, // I_C1 // 2nd io expander
  25, // I_D1 // 2nd io expander
  8,  // I_C2  
  24,  // I_D2
  10, // I_C3
  11, // I_D3
  12, // I_C4
  13, // I_D4   
  14, // I_C5
  15  // I_D5
};
const int numInputs = sizeof(inputPins) / sizeof(inputPins[0]);


// Configure the possible lock-combinations.
// The first value must be an Output (O_)
// The second value must be an Input (I_)
const uint8_t config0[][2] = { 
  {O_A1, I_D5},
  {O_B2, I_D1},
  {O_A3, I_C2},
  {O_B4, I_D3},
  {O_A5, I_C3}
};
const uint8_t config1[][2] = { 
  {O_B1, I_D2},
  {O_A1, I_C4},
  {O_B2, I_D1},
  {O_A2, I_D5},
  {O_B3, I_C1},
  {O_A3, I_C3},
  {O_B4, I_C2},
  {O_A4, I_D3},
  {O_B5, I_D4}
};

// All available configs must be registered here.
const uint8_t* configs[] = {
  (const uint8_t*)config0,
  (const uint8_t*)config1,
};
// And here.
const int configLengths[] = {
  sizeof(config0)/sizeof(config0[0]),
  sizeof(config1)/sizeof(config1[0]),
};

const int numConfigs = sizeof(configs)/sizeof(configs[0]);

// ---- State ----
int currentConfig = 0;
unsigned long lastButtonMsStart = 0;

bool relayOpen = false;

/**
 * Maps the pin to the mcp chip to be used.
 */
MCP23017 mapPinToMcp(uint8_t pin) {
  if (pin <= 15) {
    return mcp_1;
  } else {
    return mcp_2;
  }
}

/**
 * Maps the pin to the mcp chip pin (simple modulo)
 */
uint8_t mapMcpPin(uint8_t pin) {
  return pin % 16;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL);
  mcp_1.init();
  mcp_2.init();

  // Initialize outputs
  // Initialize them as inputs to prevent shortcuts done by connecting two outputs.
  // Only one output at a time is switched to output mode at a time when needed!
  for (int i = 0; i < numOutputs; i++) {
    MCP23017 mcp = mapPinToMcp(outputPins[i]);
    mcp.pinMode(mapMcpPin(outputPins[i]), INPUT);
  }
  // Initialize inputs (plain INPUT, external pull-downs required)
  for (int i = 0; i < numInputs; i++) {
    MCP23017 mcp = mapPinToMcp(inputPins[i]);
    mcp_1.pinMode(mapMcpPin(inputPins[i]), INPUT);
  }

  // Setup the other things.
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  // Uses pullup -> button must pull-down to ground! (e.g. connect the input to ground)
  pinMode(PIN_BUTTON, INPUT_PULLUP); 

  Serial.println("RiddleBox is ready.");
}

void unlock(bool unlocked) {
  if (RELAY_PULSE == 0) {
    // Drive directly, as long as the lock is unlocked.
    digitalWrite(PIN_RELAY, unlocked ? HIGH : LOW);
  } else {
    if (!relayOpen && unlocked) {
      relayOpen = true;
      digitalWrite(PIN_RELAY, HIGH);
      delay(RELAY_PULSE);
      digitalWrite(PIN_RELAY, LOW);
    } else if (!unlocked) {
      relayOpen = false;
    }
  }
}

void loop() {
  unsigned long buttonPressedTime = millis() - lastButtonMsStart;
  if (lastButtonMsStart != 0 && buttonPressedTime > EMERGENCY_UNLOCK_MS) {
    // Signal that you have waited log enough to open the lock with emergency unlock
    digitalWrite(PIN_LED, HIGH);
  }

  // First check if the config switcher button was pressed (pressed==LOW). (debounced)
  if (digitalRead(PIN_BUTTON) == LOW) {
    if (lastButtonMsStart == 0) {
      lastButtonMsStart = millis();
    }
  } else {
    if (lastButtonMsStart != 0) {
      Serial.print("button pressed");
      Serial.println(buttonPressedTime);
      if (buttonPressedTime > EMERGENCY_UNLOCK_MS) {
        digitalWrite(PIN_LED, LOW);

        // Emergency unlock - press the button for more than 5 seconds
        relayOpen = false;
        unlock(true);
      } else if (buttonPressedTime > 250) {
        // Switch config if the button was pressed less than 5 seconds.
        currentConfig = (currentConfig + 1) % numConfigs;
        Serial.print("Switched to config ");
        Serial.println(currentConfig);
        showConfigNumber(currentConfig + 1);
      }
    }

    lastButtonMsStart = 0;
  }  

  // Then check all connections to find out if the lock is unlocked.
  bool unlocked = checkExactConfig(currentConfig);
  if (unlocked && !relayOpen && RELAY_PULSE != 0) {
    // Wait a bit before unlocking.
    delay(1000);
  }
  unlock(unlocked);

  // For debugging
  //delay(1000);
}

// Test exactly one output-input pair
bool pairConnected(uint8_t outPin, uint8_t inPin) {
  

  // Drive selected output HIGH
  MCP23017 mcp = mapPinToMcp(outPin);
  uint8_t pin = mapMcpPin(outPin);
  mcp.pinMode(pin, OUTPUT);
  mcp.digitalWrite(pin, HIGH);
  delayMicroseconds(300);

  // Read input
  MCP23017 mcp_read = mapPinToMcp(outPin);
  int val = mcp_read.digitalRead(mapMcpPin(inPin));

  // Reset output back to high-Z
  mcp.digitalWrite(pin, LOW);
  mcp.pinMode(pin, INPUT);

  return (val == HIGH);
}

// Check for exact config: required pairs only, no extras
bool checkExactConfig(int cfgIndex) {
  const uint8_t* cfg = configs[cfgIndex];
  int len = configLengths[cfgIndex];

  Serial.println("🔍 Live connections:");

  // These loops could be much more efficient,
  // however for this small project its just good enough^^ 
  // That is not my masterpiece software but it just works :-)

  // Collect all active pairs
  bool activePairs[numOutputs][numInputs] = {false};
  for (int o = 0; o < numOutputs; ++o) {
    // Drive selected output HIGH
    MCP23017 mcp = mapPinToMcp(outputPins[o]);
    uint8_t pin = mapMcpPin(outputPins[o]);
    mcp.pinMode(pin, OUTPUT);
    mcp.digitalWrite(pin, HIGH);
    delayMicroseconds(300);

    for (int i = 0; i < numInputs; ++i) {
      // Read input
      MCP23017 mcp_input = mapPinToMcp(inputPins[i]);
      int val = mcp_input.digitalRead(mapMcpPin(inputPins[i]));
      if (val == HIGH) {
        activePairs[o][i] = true;
        Serial.print("* O");
        Serial.print(outputPins[o]);
        Serial.print(" -> I");
        Serial.println(inputPins[i]);
      } else {
        activePairs[o][i] = false;
      }
    }

    // Reset output back to high-Z (short cut protection)
    mcp.digitalWrite(pin, LOW);
    mcp.pinMode(pin, INPUT);
    delayMicroseconds(300);
  }

  Serial.println("Check connections:");

  // Check if all required pairs are present
  for (int c = 0; c < len; ++c) {
    uint8_t outIndex = *(cfg + c*2 + 0);
    uint8_t inIndex = *(cfg + c*2 + 1);
    
    uint8_t outPin = outputPins[outIndex];
    uint8_t inPin  = inputPins[inIndex];

    if (!activePairs[outIndex][inIndex]) {
      Serial.println("✘ Required pair missing!");
      Serial.print("  O");
      Serial.print(outPin);
      Serial.print(" -> I");
      Serial.println(inPin);
      return false;
    }
  }

  // Check if there are extra pairs
  for (int o = 0; o < numOutputs; ++o) {
    for (int i = 0; i < numInputs; ++i) {
      if (activePairs[o][i]) {
        bool required = false;
        for (int c = 0; c < len; ++c) {
          uint8_t outIndex = *(cfg + c*2 + 0);
          uint8_t inIndex = *(cfg + c*2 + 1);
          
          uint8_t outPin = outputPins[outIndex];
          uint8_t inPin  = inputPins[inIndex];
          if (outputPins[o] == outPin && inputPins[i] == inPin) {
            required = true;
            break;
          }
        }
        if (!required) {
          Serial.print("✘ Extra connection detected: O");
          Serial.print(outputPins[o]);
          Serial.print(" -> I");
          Serial.println(inputPins[i]);
          return false;
        }
      }
    }
  }

  Serial.println("✔ Exact config, relay ON");
  return true;
}

// Blink LED with the number of current config
void showConfigNumber(int number) {
  for (int i = 0; i < number; i++) {
    digitalWrite(PIN_LED, HIGH); delay(180);
    digitalWrite(PIN_LED, LOW);  delay(180);
  }
}
