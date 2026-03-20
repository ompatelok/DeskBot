#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET    -1 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- SAFE PIN MAPPING ---
const int touchPin = 14;  // D5 on NodeMCU (GPIO 14)
const int buzzerPin = 12; // D6 on NodeMCU (GPIO 12)

// Variables for blinking logic
unsigned long previousMillis = 0;
const long blinkInterval = 4000; // Blinks every 4 seconds
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

  // Play a happy "Boot Up" chirp
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

  // --- INTERACTION: TOUCH DETECTED ---
  if (touchState == HIGH) {
    // Play a "Shy / Cute" sound (descending tones)
    tone(buzzerPin, 2000, 100); 
    delay(150);
    tone(buzzerPin, 1800, 150);
    
    // Draw Shy Face
    display.clearDisplay();
    drawShyEyes();
    display.display();
    
    // Keep shy face on while being touched, plus a small buffer
    delay(1000); 
    
    // Reset the blink timer so it doesn't instantly blink after being shy
    previousMillis = millis(); 
  } 
  
  // --- IDLE STATE: RANDOM BLINKING ---
  else {
    // Check if it's time to start a blink
    if (!isBlinking && (currentMillis - previousMillis >= blinkInterval)) {
      isBlinking = true;
      blinkStartTime = currentMillis;
      
      display.clearDisplay();
      drawClosedEyes(); // Draw the blink
      display.display();
    }
    
    // Check if it's time to end the blink (keep eyes closed for 150ms)
    if (isBlinking && (currentMillis - blinkStartTime >= 150)) {
      isBlinking = false;
      previousMillis = currentMillis;
      
      display.clearDisplay();
      drawHappyEyes(); // Back to normal
      display.display();
    }
  }
}

// --- EXPRESSION COMMAND EXPLANATIONS ---

void drawHappyEyes() {
  // display.fillRoundRect(X, Y, Width, Height, CornerRadius, Color)
  // This command draws the main wide eyes.
  display.fillRoundRect(20, 20, 35, 25, 8, SSD1306_WHITE);
  display.fillRoundRect(73, 20, 35, 25, 8, SSD1306_WHITE);
}

void drawClosedEyes() {
  // display.fillRect(X, Y, Width, Height, Color)
  // This command draws thin horizontal lines to simulate closed eyelids.
  display.fillRect(20, 30, 35, 4, SSD1306_WHITE);
  display.fillRect(73, 30, 35, 4, SSD1306_WHITE);
}

void drawShyEyes() {
  // Draw smaller, lowered eyes to look timid
  display.fillRoundRect(25, 30, 25, 15, 6, SSD1306_WHITE);
  display.fillRoundRect(78, 30, 25, 15, 6, SSD1306_WHITE);
  
  // display.drawLine(startX, startY, endX, endY, Color)
  // This command draws little diagonal "blush" marks under the eyes.
  display.drawLine(25, 50, 30, 45, SSD1306_WHITE);
  display.drawLine(35, 50, 40, 45, SSD1306_WHITE);
  
  display.drawLine(88, 50, 93, 45, SSD1306_WHITE);
  display.drawLine(98, 50, 103, 45, SSD1306_WHITE);
}