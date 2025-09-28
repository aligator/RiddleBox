// First iteration of the code - POC works :-)

#include <Wire.h>
#include <MCP23017.h>

constexpr int PIN_SDA    = 21;   // ESP32 SDA
constexpr int PIN_SCL    = 22;   // ESP32 SCL
constexpr int PIN_LED    = 25;   // Status LED
constexpr int PIN_BUTTON = 18;   // Button input
constexpr int PIN_RELAY  = 26;   // Relay output

MCP23017 mcp(0x20);

// ---- Outputs (Port A) ----
const uint8_t outputPins[] = {0, 1, 2, 3, 4, 5, 6, 7};   // GPAx
const int numOutputs = sizeof(outputPins) / sizeof(outputPins[0]);

// ---- Inputs (Port B) ----
const uint8_t inputPins[]  = {8, 9, 10, 11, 12, 13, 14, 15};   // GPBx
const int numInputs = sizeof(inputPins) / sizeof(inputPins[0]);

// ---- Configurations: {outputIndex, inputIndex} ----
const uint8_t config0[][2] = { {6,8}, {7,9} };
const uint8_t config1[][2] = { {0,10}, {1,11} };
const uint8_t config2[][2] = { {2,8}, {3,11}, {6,9} };

const uint8_t* configs[] = {
  (const uint8_t*)config0,
  (const uint8_t*)config1,
  (const uint8_t*)config2
};
const int configLengths[] = {
  sizeof(config0)/sizeof(config0[0]),
  sizeof(config1)/sizeof(config1[0]),
  sizeof(config2)/sizeof(config2[0])
};
const int numConfigs = sizeof(configs)/sizeof(configs[0]);

// ---- State ----
int currentConfig = 0;
unsigned long lastButtonMs = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(PIN_SDA, PIN_SCL);
  mcp.init();

  // Initialize outputs
  for (int i = 0; i < numOutputs; i++) {
    mcp.pinMode(outputPins[i], OUTPUT);
    mcp.digitalWrite(outputPins[i], LOW);
  }
  // Initialize inputs (plain INPUT, external pull-downs required)
  for (int i = 0; i < numInputs; i++) {
    mcp.pinMode(inputPins[i], INPUT);
  }

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  Serial.println("Puzzle box ready.");
}

void loop() {
  // Debounced button press to cycle configs
  if (digitalRead(PIN_BUTTON) == LOW && millis() - lastButtonMs > 250) {
    lastButtonMs = millis();
    currentConfig = (currentConfig + 1) % numConfigs;
    Serial.print("Switched to config ");
    Serial.println(currentConfig);
    showConfigNumber(currentConfig + 1);
  }

  bool unlocked = checkExactConfig(currentConfig);
  digitalWrite(PIN_RELAY, unlocked ? HIGH : LOW);

  delay(200);
}

// Test exactly one output-input pair
bool pairConnected(uint8_t outPin, uint8_t inPin) {
  // Put all outputs into high-Z
  for (int o = 0; o < numOutputs; ++o) {
    mcp.pinMode(outputPins[o], INPUT);
  }

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

  // Collect all active pairs
  bool activePairs[numOutputs][numInputs] = {false};
  for (int o = 0; o < numOutputs; ++o) {
    for (int i = 0; i < numInputs; ++i) {
      if (pairConnected(outputPins[o], inputPins[i])) {
        activePairs[o][i] = true;
        Serial.print("  O");
        Serial.print(outputPins[o]);
        Serial.print(" -> I");
        Serial.println(inputPins[i]);
      }
    }
  }

  // Check if all required pairs are present
  for (int c = 0; c < len; ++c) {
    uint8_t outPin = *(cfg + c*2 + 0);
    uint8_t inPin  = *(cfg + c*2 + 1);
    if (!pairConnected(outPin, inPin)) {
      Serial.println("✘ Required pair missing!");
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
