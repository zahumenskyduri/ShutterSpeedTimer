#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const uint8_t photodiode1_PIN = A0;
const uint8_t photodiode2_PIN = A1;
const uint8_t photodiode3_PIN = A2;

const uint8_t button_PIN = 4;

const int threshold = 30;
const unsigned long timeOut = 10000;

unsigned long timeOutStart;
bool timeOutStarted = false;

unsigned long timeStart1;
unsigned long timeStart2;
unsigned long timeStart3;

unsigned long time1;
unsigned long time2;
unsigned long time3;

enum State {
  DETECTING,
  MEASURING,
  RESULT
};

State state1 = DETECTING;
State state2 = DETECTING;
State state3 = DETECTING;

// -------------------- Fraction display helpers (NO snapping) --------------------
// Shows shutter time as:
//  - fractions for < 1s: 1/125, 1/60, 1/8, etc. (from the measured value, not snapped)
//  - seconds for >= 1s: 2", 1.3", etc.

void printShutterFractionFromMicros(unsigned long tMicros) {
  float s = tMicros / 1000000.0f;

  // >= 1 second: show seconds with a quote mark
  if (s >= 1.0f) {
    // Keep it compact on OLED: 1 decimal for <10s, else integer
    if (s < 10.0f) {
      display.print(s, 1);
    } else {
      display.print((unsigned long)(s + 0.5f));
    }
    display.print("\"");
    return;
  }

  // < 1 second: show as 1/N (using the measured value directly)
  // N = round(1/s)
  if (s <= 0.000001f) {  // extremely small / invalid
    display.print("1/?");
    return;
  }

  unsigned long denom = (unsigned long)(1.0f / s + 0.5f); // rounded
  if (denom < 1) denom = 1;

  display.print("1/");
  display.print(denom);
}
// -------------------------------------------------------------------------------

void setup() {
  pinMode(photodiode1_PIN, INPUT);
  pinMode(photodiode2_PIN, INPUT);
  pinMode(photodiode3_PIN, INPUT);
  pinMode(button_PIN, INPUT);

  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.clearDisplay();
  display.display();
}

void detectLight() {
  int reading1 = analogRead(photodiode1_PIN);
  if (reading1 >= threshold && state1 == DETECTING) {
    timeStart1 = micros();
    state1 = MEASURING;
  }

  int reading2 = analogRead(photodiode2_PIN);
  if (reading2 >= threshold && state2 == DETECTING) {
    timeStart2 = micros();
    state2 = MEASURING;
  }

  int reading3 = analogRead(photodiode3_PIN);
  if (reading3 >= threshold && state3 == DETECTING) {
    timeStart3 = micros();
    state3 = MEASURING;
  }
}

void measureTime() {
  int reading1 = analogRead(photodiode1_PIN);
  if (reading1 < threshold && state1 == MEASURING) {
    time1 = micros() - timeStart1;
    state1 = RESULT;
  }

  int reading2 = analogRead(photodiode2_PIN);
  if (reading2 < threshold && state2 == MEASURING) {
    time2 = micros() - timeStart2;
    state2 = RESULT;
  }

  int reading3 = analogRead(photodiode3_PIN);
  if (reading3 < threshold && state3 == MEASURING) {
    time3 = micros() - timeStart3;
    state3 = RESULT;
  }
}

void runMeasurement() {
  while (true) {
    switch (state1) {
      case DETECTING: detectLight(); break;
      case MEASURING: measureTime(); break;
      default: break;
    }

    switch (state2) {
      case DETECTING: detectLight(); break;
      case MEASURING: measureTime(); break;
      default: break;
    }

    switch (state3) {
      case DETECTING: detectLight(); break;
      case MEASURING: measureTime(); break;
      default: break;
    }

    if (state1 == RESULT && state2 == RESULT && state3 == RESULT) {
      timeOutStart = millis();
      timeOutStarted = true;
      break;
    }

    buttonReset();
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(24, 0);
  display.println("Shutter Timer");

  display.setCursor(0, 16);
  display.print("Time 1: ");
  switch (state1) {
    case DETECTING:
      display.println("Detecting...");
      break;
    case RESULT:
      printShutterFractionFromMicros(time1);
      display.println();
      break;
    default: break;
  }

  display.setCursor(0, 32);
  display.print("Time 2: ");
  switch (state2) {
    case DETECTING:
      display.println("Detecting...");
      break;
    case RESULT:
      printShutterFractionFromMicros(time2);
      display.println();
      break;
    default: break;
  }

  display.setCursor(0, 48);
  display.print("Time 3: ");
  switch (state3) {
    case DETECTING:
      display.println("Detecting...");
      break;
    case RESULT:
      printShutterFractionFromMicros(time3);
      display.println();
      break;
    default: break;
  }

  display.display();
}

void buttonReset() {
  if (digitalRead(button_PIN) == LOW) {
    return;
  }
  timeOutStarted = false;
  state1 = DETECTING;
  state2 = DETECTING;
  state3 = DETECTING;
}

void loop() {
  buttonReset();
  updateDisplay();

  if (!timeOutStarted) {
    runMeasurement();
  }

  if (timeOutStarted && millis() - timeOutStart >= timeOut) {
    state1 = DETECTING;
    state2 = DETECTING;
    state3 = DETECTING;
    timeOutStarted = false;
  }
}
