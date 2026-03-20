#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- SAFE PIN MAPPING ---
const int touchPin = 14;  // D5 on NodeMCU 
const int buzzerPin = 12; // D6 on NodeMCU 

// Variables for Touch Timing
unsigned long touchStartTime = 0;
bool isTouching = false;
bool glitchTriggered = false;

// Variables for Idle Blinking
unsigned long previousMillis = 0;
const long blinkInterval = 4000; 
bool isBlinking = false;
unsigned long blinkStartTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(touchPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed. Check wiring!"));
    for(;;);
  }

  // Boot Sound
  tone(buzzerPin, 1500, 150);
  delay(200);
  tone(buzzerPin, 2500, 200);

  display.clearDisplay();
  drawHappyEyes();
  display.display();
}

void loop() {
  int touchState = digitalRead(touchPin);
  unsigned long currentMillis = millis();

  // --- TOUCH LOGIC (Short Tap vs Long Hold) ---
  if (touchState == HIGH) {
    if (!isTouching) {
      // Finger just touched the sensor
      isTouching = true;
      touchStartTime = currentMillis;
      glitchTriggered = false;
    } 
    else {
      // Finger is STILL on the sensor. Check how long!
      if ((currentMillis - touchStartTime > 2000) && !glitchTriggered) {
        // Held for 2 seconds! Trigger the Glitch!
        triggerCyberGlitch();
        glitchTriggered = true; // Prevent it from looping constantly
        previousMillis = millis(); // Reset blink timer
      }
    }
  } 
  else {
    // Finger was removed
    if (isTouching) {
      // If they removed it BEFORE 2 seconds, it was a short tap!
      if (!glitchTriggered) {
        triggerShyReaction();
        previousMillis = millis(); // Reset blink timer
      }
      isTouching = false;
    }

    // --- IDLE STATE: RANDOM BLINKING ---
    if (!isBlinking && (currentMillis - previousMillis >= blinkInterval)) {
      isBlinking = true;
      blinkStartTime = currentMillis;
      display.clearDisplay();
      drawClosedEyes(); 
      display.display();
    }
    
    if (isBlinking && (currentMillis - blinkStartTime >= 150)) {
      isBlinking = false;
      previousMillis = currentMillis;
      display.clearDisplay();
      drawHappyEyes(); 
      display.display();
    }
  }
}

// --- REACTIONS & ANIMATIONS ---

void triggerShyReaction() {
  tone(buzzerPin, 2000, 100); 
  delay(150);
  tone(buzzerPin, 1800, 150);
  
  display.clearDisplay();
  drawShyEyes();
  display.display();
  
  delay(1000); // Hold the shy face
  display.clearDisplay();
  drawHappyEyes();
  display.display();
}

void triggerCyberGlitch() {
  // Loop 15 times rapidly to create chaos
  for (int i = 0; i < 15; i++) {
    display.clearDisplay();
    
    // Draw random static lines across the screen
    for (int j = 0; j < 8; j++) {
      display.drawLine(random(0, 128), random(0, 64), random(0, 128), random(0, 64), SSD1306_WHITE);
    }

    // Flash random terminal text
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(random(0, 40), random(0, 50));
    
    // Randomly pick what text to flash
    int textChoice = random(0, 3);
    if (textChoice == 0) display.print("ERR_MEM_OVERFLOW");
    else if (textChoice == 1) display.print("SYS.REBOOT()");
    else display.print("0x00000000");

    display.display();
    
    // Play erratic, random buzzer frequencies
    tone(buzzerPin, (int)random(500, 4000), 30);
    delay(random(20, 80)); // Random stutter
  }

  // End the glitch with a "Boot Sequence"
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("> SECURE_CONNECTION_OK");
  display.setCursor(0, 35);
  display.print("> REBOOTING...");
  display.display();
  
  tone(buzzerPin, 1000, 400); // Long solid beep
  delay(1000);
  
  // Back to normal
  display.clearDisplay();
  drawHappyEyes();
  display.display();
}

// --- BASIC DRAWING FUNCTIONS ---

void drawHappyEyes() {
  display.fillRoundRect(20, 20, 35, 25, 8, SSD1306_WHITE);
  display.fillRoundRect(73, 20, 35, 25, 8, SSD1306_WHITE);
}

void drawClosedEyes() {
  display.fillRect(20, 30, 35, 4, SSD1306_WHITE);
  display.fillRect(73, 30, 35, 4, SSD1306_WHITE);
}

void drawShyEyes() {
  display.fillRoundRect(25, 30, 25, 15, 6, SSD1306_WHITE);
  display.fillRoundRect(78, 30, 25, 15, 6, SSD1306_WHITE);
  display.drawLine(25, 50, 30, 45, SSD1306_WHITE);
  display.drawLine(35, 50, 40, 45, SSD1306_WHITE);
  display.drawLine(88, 50, 93, 45, SSD1306_WHITE);
  display.drawLine(98, 50, 103, 45, SSD1306_WHITE);
}