#include <Wire.h>

// MXP23017 lib: https://github.com/blemasle/arduino-mcp23017
#include <MCP23017.h>

// MCP23017 IO Expander i2c pins
constexpr int PIN_SDA    = 21;   // ESP32 SDA
constexpr int PIN_SCL    = 22;   // ESP32 SCL
constexpr uint8_t I2C_ADDRESS = 0x20; // I2C Adress of the chip

// Configuration i/o
constexpr int PIN_LED    = 25;   // Status LED
constexpr int PIN_BUTTON = 18;   // Button input

// Action output
constexpr int PIN_RELAY  = 26;   // Relay output

// If not 0 the relay will be pulsed for the given ms. This is usefull for locks that need a short pulse only.
constexpr int RELAY_PULSE = 0; //200; 

MCP23017 mcp(I2C_ADDRESS);

// MCP Output pin configuration
constexpr uint8_t O_A1 = 0;
constexpr uint8_t O_A2 = 1;
constexpr uint8_t O_A3 = 2;
constexpr uint8_t O_A4 = 3;
constexpr uint8_t O_B1 = 4;
constexpr uint8_t O_B2 = 5;
constexpr uint8_t O_B3 = 6;
constexpr uint8_t O_B4 = 7;
const uint8_t outputPins[] = {O_A1, O_A2, O_A3, O_A4, O_B1, O_B2, O_B3, O_B4};
const int numOutputs = sizeof(outputPins) / sizeof(outputPins[0]);

// MCP Input pin configuration
constexpr uint8_t I_C1 = 8;
constexpr uint8_t I_C2 = 9;
constexpr uint8_t I_C3 = 10;
constexpr uint8_t I_C4 = 11;
constexpr uint8_t I_D1 = 12;
constexpr uint8_t I_D2 = 13;
constexpr uint8_t I_D3 = 14;
constexpr uint8_t I_D4 = 15;
const uint8_t inputPins[]  = {I_C1, I_C2, I_C3, I_C4, I_D1, I_D2, I_D3, I_D4};
const int numInputs = sizeof(inputPins) / sizeof(inputPins[0]);


// Configure the possible lock-combinations.
// The first value must be an Output (O_)
// The second value must be an Input (I_)
const uint8_t config0[][2] = { 
  {O_B3,I_C1}, 
  {O_B4,I_C2}
};
const uint8_t config1[][2] = { 
  {O_A1,I_C3},
  {O_B2,I_C4},
  {O_A2,I_D4}
};
const uint8_t config2[][2] = { 
  {O_A3,I_D4},
  {O_A4,I_D1},
  {O_B4,I_D2},
  {O_B3,I_C3}
};

// All available configs must be registered here.
const uint8_t* configs[] = {
  (const uint8_t*)config0,
  (const uint8_t*)config1,
  (const uint8_t*)config2
};
// And here.
const int configLengths[] = {
  sizeof(config0)/sizeof(config0[0]),
  sizeof(config1)/sizeof(config1[0]),
  sizeof(config2)/sizeof(config2[0])
};

const int numConfigs = sizeof(configs)/sizeof(configs[0]);

// ---- State ----
int currentConfig = 0;
unsigned long lastButtonMs = 0;

bool relayOpen = false;

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL);
  mcp.init();

  // Initialize outputs
  // Initialize them as inputs to prevent shortcuts done by connecting two outputs.
  // Only one output at a time is switched to output mode at a time when needed!
  for (int i = 0; i < numOutputs; i++) {
    mcp.pinMode(outputPins[i], INPUT);
  }
  // Initialize inputs (plain INPUT, external pull-downs required)
  for (int i = 0; i < numInputs; i++) {
    mcp.pinMode(inputPins[i], INPUT);
  }

  // Setup the other things.
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  // Uses pullup -> button must pull-down to ground! (e.g. connect the input to ground)
  pinMode(PIN_BUTTON, INPUT_PULLUP); 

  Serial.println("RiddleBox is ready.");
}

void loop() {
  // First check if the config switcher button was pressed. (debounced)
  if (digitalRead(PIN_BUTTON) == LOW && millis() - lastButtonMs > 250) {
    lastButtonMs = millis();
    currentConfig = (currentConfig + 1) % numConfigs;
    Serial.print("Switched to config ");
    Serial.println(currentConfig);
    showConfigNumber(currentConfig + 1);
  }

  // Then check all connections to find out if the lock is unlocked.
  bool unlocked = checkExactConfig(currentConfig);
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

  // For debugging
  //delay(1000);
}

// Test exactly one output-input pair
bool pairConnected(uint8_t outPin, uint8_t inPin) {
  

  // Drive selected output HIGH
  mcp.pinMode(outPin, OUTPUT);
  mcp.digitalWrite(outPin, HIGH);
  delayMicroseconds(300);

  // Read input
  int val = mcp.digitalRead(inPin);

  // Reset output back to high-Z
  mcp.digitalWrite(outPin, LOW);
  mcp.pinMode(outPin, INPUT);

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
    mcp.pinMode(outputPins[o], OUTPUT);
    mcp.digitalWrite(outputPins[o], HIGH);
    delayMicroseconds(300);

    for (int i = 0; i < numInputs; ++i) {
      // Read input
      int val = mcp.digitalRead(inputPins[i]);
      if (val == HIGH) {
        activePairs[o][i] = true;
        Serial.print(o);
        Serial.print("-->");
        Serial.print(i);
        Serial.print("  O");
        Serial.print(outputPins[o]);
        Serial.print(" -> I");
        Serial.println(inputPins[i]);
      } else {
        activePairs[o][i] = false;
      }
    }

    // Reset output back to high-Z (short cut protection)
    mcp.digitalWrite(outputPins[o], LOW);
    mcp.pinMode(outputPins[o], INPUT);
    delayMicroseconds(300);
  }

  // Check if all required pairs are present
  for (int c = 0; c < len; ++c) {
    uint8_t outPin = *(cfg + c*2 + 0);
    uint8_t inPin  = (*(cfg + c*2 + 1))-8; // in pins are shifted by 8
    Serial.print(outPin);
    Serial.print("-->");
    Serial.print(inPin);
    if (!activePairs[outPin][inPin]) {
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
          uint8_t reqOut = *(cfg + c*2 + 0);
          uint8_t reqIn  = *(cfg + c*2 + 1);
          if (outputPins[o] == reqOut && inputPins[i] == reqIn) {
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
  delay(1000);
}
