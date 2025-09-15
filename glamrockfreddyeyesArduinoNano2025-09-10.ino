/*
  Glamrock Freddy Eyes Controller (Arduino Nano Version - Fully Non-Blocking)
  For Arduino Nano and two 16-LED WS2212B rings.
  Control 20 preset light shows via a physical pushbutton and rotary encoder.
  All animations are non-blocking to ensure the controls are always responsive.
  Uses a robust state-machine for reading the rotary encoder.

  --- BILL OF MATERIALS ---
  - 1x Arduino Nano (or compatible board)
  - 2x 16-LED WS2812B NeoPixel Rings
  - 1x 5-pin Rotary Encoder Module (KY-040 or similar)
  - 1x Momentary Pushbutton
  - 2x 330 Ohm Resistors
  - 1x 1000uF Capacitor
  - 1x 5V Power Supply (2A+ recommended, e.g., USB Power Bank)
  - 1x Breadboard and Jumper Wires

  --- CONTROLS GUIDE ---
  - Rotary Encoder Dial: Turn clockwise or counter-clockwise to cycle through
    animations one by one.
  - External Pushbutton (Single Press): Cycles forward to the next animation (jumps 1).
  - Encoder Button (Single Press): Jumps 5 animations forward.
  - Encoder Button (Press and Hold for 1+ sec): Resets to the first animation (case 0).

  --- HOW TO MODIFY ANIMATIONS ---
  To add or remove an animation, follow these three steps:
  1. ADD/REMOVE THE FUNCTION: Create a new void function for your animation, or
      delete/comment out an existing one from the "Light Show Functions" section.

  2. ADD/REMOVE THE CASE: In the `loop()` function, find the `switch (currentPreset)`
      block. Add a new `case` line that calls your new function, or delete/comment
      out the case for the animation you are removing. Make sure the case numbers
      are sequential starting from 0.

  3. UPDATE TOTAL_PRESETS: Change the number in the `#define TOTAL_PRESETS` line
      at the top of the code to match the new total number of animations. This is
      crucial for the buttons and encoder to work correctly.

  --- WIRING GUIDE ---
  POWER:
    - 5V Power Supply (+) -> Breadboard Positive Rail
    - 5V Power Supply (-) -> Breadboard Negative Rail
    - Arduino 5V Pin      -> Breadboard Positive Rail
    - Arduino GND Pin     -> Breadboard Negative Rail (Common Ground)
    - 1000uF Capacitor    -> Across Positive and Negative Rails

  LED RINGS:
    - Left Eye 5V         -> Breadboard Positive Rail
    - Left Eye GND        -> Breadboard Negative Rail
    - Left Eye Din        -> 330 Ohm Resistor -> Arduino Pin D6

    - Right Eye 5V        -> Breadboard Positive Rail
    - Right Eye GND       -> Breadboard Negative Rail
    - Right Eye Din       -> 330 Ohm Resistor -> Arduino Pin D5

  CONTROLS:
    - Pushbutton Leg 1    -> Arduino Pin D9
    - Pushbutton Leg 2    -> Breadboard Negative Rail

    - Encoder CLK         -> Arduino Pin D2
    - Encoder DT          -> Arduino Pin D3
    - Encoder SW          -> Arduino Pin D4
    - Encoder '+'         -> Breadboard Positive Rail
    - Encoder 'GND'       -> Breadboard Negative Rail
*/

#include <Adafruit_NeoPixel.h>

// Pin definitions for the LED rings
#define LEFT_EYE_PIN    6
#define RIGHT_EYE_PIN   5

// Pin definition for the external button
#define BUTTON_PIN      9

// Pin definitions for the rotary encoder
#define ENCODER_CLK_PIN 2 // Interrupt 0 on Uno/Nano
#define ENCODER_DT_PIN  3 // Interrupt 1 on Uno/Nano
#define ENCODER_SW_PIN  4 // Switch pin for the encoder button

// Number of LEDs in each ring and total number of animations
#define NUM_LEDS 16
#define TOTAL_PRESETS 20 // Total number of animation cases (0-19)

// Create NeoPixel objects for each eye
Adafruit_NeoPixel leftEye(NUM_LEDS, LEFT_EYE_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel rightEye(NUM_LEDS, RIGHT_EYE_PIN, NEO_GRB + NEO_KHZ800);

// Current light show preset - volatile because it's changed in an ISR
volatile uint8_t currentPreset = 0;
uint8_t lastPreset = 255; // Used to detect when a preset changes

// --- Variables for Button Debouncing ---
int buttonState;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// --- Variables for Encoder Button ---
int lastEncoderButtonState = HIGH;
unsigned long encoderButtonDownTime = 0;
bool longPressActionDone = false;


// --- Variables for Rotary Encoder State Machine ---
volatile byte aFlag = 0;
volatile byte bFlag = 0;

// --- State Variables for Non-Blocking Animations ---
unsigned long lastUpdateTime = 0; // Generic timer for animations
uint16_t animationCounter = 0;      // Generic counter for animation steps
uint16_t animationSubCounter = 0; // Generic sub-counter

void setup() {
  Serial.begin(115200);
  Serial.println("Glamrock Freddy Eyes Controller - Final Version");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
  
  // Setup Encoder Dial Pins
  pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
  pinMode(ENCODER_DT_PIN, INPUT_PULLUP);

  // Attach a single interrupt for one encoder pin
  // The logic inside the ISR will determine the direction from the second pin's state.
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT_PIN), encoderIsr, CHANGE);

  leftEye.begin();
  rightEye.begin();
  leftEye.show();
  rightEye.show();
}

void loop() {
  checkButton();
  checkEncoderButton();

  // Check if the preset has changed to reset animations
  if (currentPreset != lastPreset) {
    animationCounter = 0; // Reset counters
    animationSubCounter = 0;
    lastUpdateTime = 0;      // Reset timer
    allOff();              // Clear the eyes
    leftEye.setBrightness(255); // Reset brightness
    rightEye.setBrightness(255);
    lastPreset = currentPreset;
    Serial.print("Preset changed to: ");
    Serial.println(currentPreset);
  }

  // Run the selected light show
  switch (currentPreset) {
    case 0: solidBlue(); break;
    case 1: flickeringWhite(20, 150); break;
    case 2: redAlert(); break;
    case 3: colorWipe(0, 255, 0, 50); break; // Green
    case 4: rainbowCycle(20); break;
    case 5: theaterChase(255, 0, 0, 50); break; // Red
    case 6: cylonScanner(0, 0, 255, 40); break; // Blue
    case 7: strobe(255, 255, 255, 50); break; // White
    case 8: breathingEffect(0, 0, 255); break; // Blue
    case 9: glitchEffect(50); break;
    case 10: heartbeat(255, 0, 0, 75); break; // Red
    case 11: sparks(20); break;
    case 12: powerCycle(255, 255, 0, 50); break; // Yellow
    case 13: staticEffect(10); break;
    case 14: twoColorChase(leftEye.Color(0,0,255), leftEye.Color(255,255,0), 100); break; // Blue & Yellow
    case 15: policeLights(100); break;
    case 16: larsonScanner(255, 0, 0, 30, 40); break; // Red
    case 17: meteor(255, 255, 255, 30); break; // White
    case 18: colorTwinkle(50); break;
    case 19: greenMatrix(40); break;
    default: allOff(); break;
  }
}

// Checks the external pushbutton (jumps 1)
void checkButton() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        // Jump 1 preset forward
        currentPreset = (currentPreset + 1) % TOTAL_PRESETS;
      }
    }
  }
  lastButtonState = reading;
}

// Checks the encoder's pushbutton (short press jumps 5, long press resets to 0)
void checkEncoderButton() {
  int reading = digitalRead(ENCODER_SW_PIN);

  // If the button is pressed
  if (reading == LOW) {
    // If it was just pressed (state changing from HIGH to LOW)
    if (lastEncoderButtonState == HIGH) {
      encoderButtonDownTime = millis(); // Record the time it was pressed
      longPressActionDone = false;      // Reset the long press flag
    }
    // Check if the button has been held for 1 second AND the long press action hasn't been done yet
    if ((millis() - encoderButtonDownTime > 1000) && !longPressActionDone) {
      currentPreset = 0;         // Reset to case 0
      longPressActionDone = true; // Mark the long press action as complete
    }
  } 
  // If the button is released
  else {
    // If it was just released (state changing from LOW to HIGH)
    if (lastEncoderButtonState == LOW) {
      // And if a long press action was NOT completed, it's a short press
      if (!longPressActionDone) {
        currentPreset = (currentPreset + 5) % TOTAL_PRESETS; // Jump 5 presets forward
      }
    }
  }
  lastEncoderButtonState = reading; // Save the current state for the next loop
}


// --- Interrupt Service Routine for Rotary Encoder ---
void encoderIsr() {
  // Read the state of both pins to determine the direction
  if (digitalRead(ENCODER_CLK_PIN) != digitalRead(ENCODER_DT_PIN)) {
    // Turned Clockwise
    currentPreset = (currentPreset + 1) % TOTAL_PRESETS;
  } else {
    // Turned Counter-Clockwise
    currentPreset = (currentPreset - 1 + TOTAL_PRESETS) % TOTAL_PRESETS;
  }
}

// --- Light Show Functions (Fully Non-Blocking) ---

// 0: Solid Blue
void solidBlue() {
  fillEyes(leftEye.Color(0, 0, 255));
}

// 1: Flickering White
void flickeringWhite(int minWait, int maxWait) {
    if (millis() - lastUpdateTime > random(minWait, maxWait)) {
        lastUpdateTime = millis();
        int r = random(100, 255);
        fillEyes(leftEye.Color(r, r, r));
    }
}

// 2: Red Alert
void redAlert() {
  fillEyes(leftEye.Color(255, 0, 0));
}

// 3: Color Wipe
void colorWipe(uint8_t r, uint8_t g, uint8_t b, uint8_t wait) {
  if (millis() - lastUpdateTime > wait) {
    lastUpdateTime = millis();
    leftEye.setPixelColor(animationCounter, leftEye.Color(r, g, b));
    rightEye.setPixelColor(animationCounter, rightEye.Color(r, g, b));
    leftEye.show();
    rightEye.show();
    animationCounter = (animationCounter + 1) % (NUM_LEDS + 1);
  }
}

// 4: Rainbow Cycle
void rainbowCycle(uint8_t wait) {
  if (millis() - lastUpdateTime > wait) {
    lastUpdateTime = millis();
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      leftEye.setPixelColor(i, wheel(((i * 256 / NUM_LEDS) + animationCounter) & 255));
      rightEye.setPixelColor(i, wheel(((i * 256 / NUM_LEDS) + animationCounter) & 255));
    }
    leftEye.show();
    rightEye.show();
    animationCounter = (animationCounter + 1) % 256;
  }
}

// 5: Theater Chase
void theaterChase(uint8_t r, uint8_t g, uint8_t b, uint8_t wait) {
  if (millis() - lastUpdateTime > wait) {
    lastUpdateTime = millis();
    allOff();
    for (int i = animationCounter; i < NUM_LEDS; i = i + 3) {
      leftEye.setPixelColor(i, leftEye.Color(r, g, b));
      rightEye.setPixelColor(i, rightEye.Color(r, g, b));
    }
    leftEye.show();
    rightEye.show();
    animationCounter = (animationCounter + 1) % 3;
  }
}

// 6: Cylon Scanner
void cylonScanner(uint8_t r, uint8_t g, uint8_t b, uint8_t wait) {
  if (millis() - lastUpdateTime > wait) {
    lastUpdateTime = millis();
    allOff();
    if (animationSubCounter == 0) { // Forward
      leftEye.setPixelColor(animationCounter, r, g, b);
      rightEye.setPixelColor(animationCounter, r, g, b);
      animationCounter++;
      if (animationCounter >= NUM_LEDS) {
        animationCounter = NUM_LEDS - 2;
        animationSubCounter = 1;
      }
    } else { // Backward
      leftEye.setPixelColor(animationCounter, r, g, b);
      rightEye.setPixelColor(animationCounter, r, g, b);
      if (animationCounter > 0) animationCounter--;
      else animationSubCounter = 0;
    }
    leftEye.show();
    rightEye.show();
  }
}

// 7: Strobe
void strobe(uint8_t r, uint8_t g, uint8_t b, int wait) {
    if (millis() - lastUpdateTime > wait) {
        lastUpdateTime = millis();
        if (animationCounter % 2 == 0) {
            fillEyes(leftEye.Color(r, g, b));
        } else {
            allOff();
        }
        animationCounter++;
    }
}

// 8: Breathing Effect
void breathingEffect(uint8_t r, uint8_t g, uint8_t b) {
  float breath = (exp(sin(millis() / 2000.0 * PI)) - 0.36787944) * 108.0;
  uint8_t brightness = constrain((int)breath, 0, 255);
  leftEye.setBrightness(brightness);
  rightEye.setBrightness(brightness);
  fillEyes(leftEye.Color(r, g, b));
}

// 9: Glitch Effect
void glitchEffect(int wait) {
    if (millis() - lastUpdateTime > wait) {
        lastUpdateTime = millis();
        if (animationCounter == 0) {
            if (random(10) > 7) {
                int randomPixel = random(NUM_LEDS);
                leftEye.setPixelColor(randomPixel, leftEye.Color(random(255), random(255), random(255)));
                rightEye.setPixelColor(randomPixel, rightEye.Color(random(255), random(255), random(255)));
                leftEye.show();
                rightEye.show();
                animationCounter = 1;
            }
        } else {
            allOff();
            animationCounter = 0;
        }
    }
}

// 10: Heartbeat
void heartbeat(uint8_t r, uint8_t g, uint8_t b, int wait) {
    if (millis() - lastUpdateTime > wait) {
        lastUpdateTime = millis();
        animationCounter++;
        switch(animationCounter) {
            case 1: fillEyes(leftEye.Color(r,g,b)); break; // Pulse 1
            case 2: allOff(); break;
            case 3: fillEyes(leftEye.Color(r,g,b)); break; // Pulse 2
            case 4: allOff(); break;
            case 5: break; // Pause
            case 6: break;
            case 7: break;
            case 8: animationCounter = 0; break; // Reset
        }
    }
}

// 11: Sparks
void sparks(int wait) {
    if (millis() - lastUpdateTime > wait) {
        lastUpdateTime = millis();
        if (animationCounter == 0) {
            if (random(10) > 7) {
                int pixel = random(NUM_LEDS);
                leftEye.setPixelColor(pixel, 255, 255, 255);
                rightEye.setPixelColor(pixel, 255, 255, 255);
                leftEye.show();
                rightEye.show();
                animationCounter = 1;
            }
        } else {
            allOff();
            animationCounter = 0;
        }
    }
}

// 12: Power Cycle
void powerCycle(uint8_t r, uint8_t g, uint8_t b, int wait) {
  if (millis() - lastUpdateTime > wait) {
    lastUpdateTime = millis();
    if (animationSubCounter == 0) { // Powering up
      leftEye.setPixelColor(animationCounter, r, g, b);
      rightEye.setPixelColor(animationCounter, r, g, b);
      animationCounter++;
      if (animationCounter >= NUM_LEDS) {
        animationCounter = 0;
        animationSubCounter = 1;
        lastUpdateTime = millis();
      }
    } else if (animationSubCounter == 1) { // Holding
      if (millis() - lastUpdateTime > 1000) {
        animationSubCounter = 2;
        animationCounter = 0;
      }
    } else { // Powering down
      leftEye.setPixelColor(animationCounter, 0, 0, 0);
      rightEye.setPixelColor(animationCounter, 0, 0, 0);
      animationCounter++;
      if (animationCounter >= NUM_LEDS) {
        animationCounter = 0;
        animationSubCounter = 0;
      }
    }
    leftEye.show();
    rightEye.show();
  }
}

// 13: Static Effect
void staticEffect(int wait) {
    if (millis() - lastUpdateTime > wait) {
        lastUpdateTime = millis();
        allOff();
        int pixel = random(NUM_LEDS);
        int intensity = random(50, 150);
        leftEye.setPixelColor(pixel, intensity, intensity, intensity);
        rightEye.setPixelColor(pixel, intensity, intensity, intensity);
        leftEye.show();
        rightEye.show();
    }
}

// 14: Two Color Chase
void twoColorChase(uint32_t c1, uint32_t c2, int wait) {
  if (millis() - lastUpdateTime > wait) {
    lastUpdateTime = millis();
    for (int i = 0; i < NUM_LEDS; i++) {
      if ((i + animationCounter) % 4 < 2) {
        leftEye.setPixelColor(i, c1);
        rightEye.setPixelColor(i, c1);
      } else {
        leftEye.setPixelColor(i, c2);
        rightEye.setPixelColor(i, c2);
      }
    }
    leftEye.show();
    rightEye.show();
    animationCounter = (animationCounter + 1) % 4;
  }
}

// 15: Police Lights
void policeLights(int wait) {
    if (millis() - lastUpdateTime > wait) {
        lastUpdateTime = millis();
        animationCounter++;
        if (animationCounter % 2 == 0) {
            for(int i=0; i<NUM_LEDS; i++) {
                leftEye.setPixelColor(i, 255, 0, 0); // Red
                rightEye.setPixelColor(i, 0, 0, 0);  // Off
            }
        } else {
            for(int i=0; i<NUM_LEDS; i++) {
                leftEye.setPixelColor(i, 0, 0, 0);   // Off
                rightEye.setPixelColor(i, 0, 0, 255); // Blue
            }
        }
        leftEye.show();
        rightEye.show();
    }
}

// 16: Larson Scanner (Knight Rider)
void larsonScanner(uint8_t r, uint8_t g, uint8_t b, int wait, int fade_delay) {
    // Fade all pixels
    for(int i=0; i<NUM_LEDS; i++) {
        uint32_t color = leftEye.getPixelColor(i);
        uint8_t red = (color >> 16) & 0xFF;
        uint8_t green = (color >> 8) & 0xFF;
        uint8_t blue = color & 0xFF;
        red = (red <= 10) ? 0 : red - 10;
        green = (green <= 10) ? 0 : green - 10;
        blue = (blue <= 10) ? 0 : blue - 10;
        leftEye.setPixelColor(i, red, green, blue);
        rightEye.setPixelColor(i, red, green, blue);
    }

    if (millis() - lastUpdateTime > wait) {
        lastUpdateTime = millis();
        if (animationSubCounter == 0) { // Forward
            leftEye.setPixelColor(animationCounter, r, g, b);
            rightEye.setPixelColor(animationCounter, r, g, b);
            animationCounter++;
            if (animationCounter >= NUM_LEDS) {
                animationCounter = NUM_LEDS - 2;
                animationSubCounter = 1;
            }
        } else { // Backward
            leftEye.setPixelColor(animationCounter, r, g, b);
            rightEye.setPixelColor(animationCounter, r, g, b);
            if (animationCounter > 0) animationCounter--;
            else animationSubCounter = 0;
        }
    }
    leftEye.show();
    rightEye.show();
}

// 17: Meteor
void meteor(uint8_t r, uint8_t g, uint8_t b, int wait) {
    // Fade all pixels
    for(int i=0; i<NUM_LEDS; i++) {
        uint32_t color = leftEye.getPixelColor(i);
        uint8_t red = (color >> 16) & 0xFF;
        uint8_t green = (color >> 8) & 0xFF;
        uint8_t blue = color & 0xFF;
        red = (red <= 15) ? 0 : red - 15;
        green = (green <= 15) ? 0 : green - 15;
        blue = (blue <= 15) ? 0 : blue - 15;
        leftEye.setPixelColor(i, red, green, blue);
        rightEye.setPixelColor(i, red, green, blue);
    }
    
    if (millis() - lastUpdateTime > wait) {
        lastUpdateTime = millis();
        leftEye.setPixelColor(animationCounter, r, g, b);
        rightEye.setPixelColor(animationCounter, r, g, b);
        animationCounter = (animationCounter + 1) % NUM_LEDS;
    }
    leftEye.show();
    rightEye.show();
}

// 18: Color Twinkle
void colorTwinkle(int wait) {
    if (millis() - lastUpdateTime > wait) {
        lastUpdateTime = millis();
        allOff();
        if (random(10) > 5) {
            int pixel = random(NUM_LEDS);
            leftEye.setPixelColor(pixel, wheel(random(255)));
            rightEye.setPixelColor(pixel, wheel(random(255)));
            leftEye.show();
            rightEye.show();
        }
    }
}

// 19: Green Matrix
void greenMatrix(int wait) {
    // Fade all pixels
    for(int i=0; i<NUM_LEDS; i++) {
        uint32_t color = leftEye.getPixelColor(i);
        uint8_t green = (color >> 8) & 0xFF;
        green = (green <= 20) ? 0 : green - 20;
        leftEye.setPixelColor(i, 0, green, 0);
        rightEye.setPixelColor(i, 0, green, 0);
    }

    if (millis() - lastUpdateTime > wait) {
        lastUpdateTime = millis();
        if (random(10) > 6) {
            int pixel = random(NUM_LEDS);
            leftEye.setPixelColor(pixel, 0, 255, 0);
            rightEye.setPixelColor(pixel, 0, 255, 0);
        }
    }
    leftEye.show();
    rightEye.show();
}


// --- Helper Functions ---

void fillEyes(uint32_t color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leftEye.setPixelColor(i, color);
    rightEye.setPixelColor(i, color);
  }
  leftEye.show();
  rightEye.show();
}

void allOff() {
  leftEye.clear();
  rightEye.clear();
  leftEye.show();
  rightEye.show();
}

uint32_t wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return leftEye.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return leftEye.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return leftEye.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
