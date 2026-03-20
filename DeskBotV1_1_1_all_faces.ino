#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Pins ---
#define BUZZER_PIN 12 // D6
#define TOUCH_PIN 14  // D5

// --- Timers & States ---
unsigned long currentMillis = 0;
unsigned long lastFaceChange = 0;
int currentFaceIndex = 0;

// Touch Sensor Variables
bool touchActive = false;
unsigned long touchStartTime = 0;
unsigned long lastTapTime = 0;
int tapCount = 0;

// Override variables for when you touch the sensor
bool isOverridden = false;
unsigned long overrideEndTime = 0;
int overrideFace = 0; // 1=Victory, 2=Shocked, 3=Sleepy

// Sound lock so it only plays once per face change
int lastPlayedFace = -1;

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TOUCH_PIN, INPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); 
  }
  display.clearDisplay();
  display.display();
}

void loop() {
  currentMillis = millis();
  handleTouchInput();

  display.clearDisplay();

  if (isOverridden) {
    // If the sensor was touched, show the touch reaction
    if (currentMillis < overrideEndTime) {
      if (overrideFace == 1) drawVictory();
      else if (overrideFace == 2) drawShocked();
      else if (overrideFace == 3) drawSleepy();
    } else {
      isOverridden = false; // Reaction over, go back to normal
      lastPlayedFace = -1;  // Reset sound lock
    }
  } else {
    // --- 5 Second Rotation Logic ---
    if (currentMillis - lastFaceChange >= 5000) {
      currentFaceIndex++;
      if (currentFaceIndex > 6) currentFaceIndex = 0; // Loop back to 0
      lastFaceChange = currentMillis;
    }

    // Draw the current face in the rotation
    switch (currentFaceIndex) {
      case 0: drawHappy(); playSound(0); break;
      case 1: drawSad(); playSound(1); break;
      case 2: drawAngry(); playSound(2); break;
      case 3: drawSearching(); playSound(3); break;
      case 4: drawSuspicious(); playSound(4); break;
      case 5: drawDizzy(); playSound(5); break;
      case 6: drawGlitch(); playSound(6); break;
    }
  }

  display.display(); 
}

// --- TOUCH SENSOR LOGIC ---
void handleTouchInput() {
  bool isTouched = digitalRead(TOUCH_PIN) == HIGH;

  if (isTouched && !touchActive) {
    // Finger just pressed down
    touchActive = true;
    touchStartTime = currentMillis;
  } 
  else if (!isTouched && touchActive) {
    // Finger just lifted
    touchActive = false;
    unsigned long touchDuration = currentMillis - touchStartTime;

    if (touchDuration > 800) {
      // LONG PRESS (Hold for ~1 sec)
      triggerOverride(3, 3000); // Show Sleepy face for 3 seconds
      tapCount = 0;
    } else {
      // SHORT TAP
      tapCount++;
      lastTapTime = currentMillis;
    }
  }

  // Check if taps are finished (wait 300ms to see if a second tap comes)
  if (tapCount > 0 && (currentMillis - lastTapTime > 300)) {
    if (tapCount == 1) {
      triggerOverride(1, 3000); // 1 Tap -> Victory
    } else if (tapCount >= 2) {
      triggerOverride(2, 3000); // 2 Taps -> Shocked
    }
    tapCount = 0; // Reset
  }
}

void triggerOverride(int faceCode, int duration) {
  isOverridden = true;
  overrideFace = faceCode;
  overrideEndTime = currentMillis + duration;
  lastPlayedFace = -1; // Force sound to play
  
  // Play immediate sound for the touch reactions
  if (faceCode == 1) { // Victory
    tone(BUZZER_PIN, 1000, 100); delay(100); tone(BUZZER_PIN, 1500, 200);
  } else if (faceCode == 2) { // Shocked
    tone(BUZZER_PIN, 2000, 50);
  }
}

// --- SOUND MANAGER ---
void playSound(int faceIndex) {
  if (lastPlayedFace != faceIndex) {
    if (faceIndex == 1) { // Sad
      tone(BUZZER_PIN, 800, 150); delay(200); tone(BUZZER_PIN, 600, 300);
    } else if (faceIndex == 2) { // Angry
      tone(BUZZER_PIN, 300, 400);
    } else if (faceIndex == 3) { // Searching
      tone(BUZZER_PIN, 1200, 50); delay(500); tone(BUZZER_PIN, 1200, 50);
    } else if (faceIndex == 5) { // Dizzy
      tone(BUZZER_PIN, 900, 200); delay(100); tone(BUZZER_PIN, 700, 300);
    } else if (faceIndex == 6) { // Glitch
      tone(BUZZER_PIN, 2500, 30); delay(50); tone(BUZZER_PIN, 150, 40);
    }
    lastPlayedFace = faceIndex;
  }
}

// ==========================================
// --- FACE DRAWING FUNCTIONS ---
// ==========================================

void drawHappy() {
  display.fillRoundRect(28, 12, 24, 40, 10, SSD1306_WHITE);
  display.fillRoundRect(76, 12, 24, 40, 10, SSD1306_WHITE);
}

void drawSad() {
  display.fillRoundRect(28, 18, 24, 34, 10, SSD1306_WHITE);
  display.fillRoundRect(76, 18, 24, 34, 10, SSD1306_WHITE);
  display.fillTriangle(23, 13, 57, 13, 23, 33, SSD1306_BLACK);
  display.fillTriangle(71, 13, 105, 13, 105, 33, SSD1306_BLACK);
}

void drawAngry() {
  display.fillRoundRect(34, 16, 22, 36, 8, SSD1306_WHITE);
  display.fillRoundRect(72, 16, 22, 36, 8, SSD1306_WHITE);
  // Angry eyebrows cutting inward
  display.fillTriangle(30, 10, 60, 30, 60, 10, SSD1306_BLACK);
  display.fillTriangle(98, 10, 68, 30, 68, 10, SSD1306_BLACK);
}

void drawSearching() {
  // Animates o O to O o every 500ms
  bool toggle = (currentMillis / 500) % 2 == 0;
  
  if (toggle) {
    display.fillCircle(40, 32, 8, SSD1306_WHITE);  // Small o
    display.fillCircle(88, 32, 20, SSD1306_WHITE); // Big O
  } else {
    display.fillCircle(40, 32, 20, SSD1306_WHITE); // Big O
    display.fillCircle(88, 32, 8, SSD1306_WHITE);  // Small o
  }
}

void drawSuspicious() {
  // One normal eye, one squinting eye
  display.fillRoundRect(28, 12, 24, 40, 10, SSD1306_WHITE);
  display.fillRoundRect(76, 28, 24, 8, 4, SSD1306_WHITE);
}

void drawDizzy() {
  // Draw X X
  display.drawLine(28, 16, 52, 48, SSD1306_WHITE);
  display.drawLine(52, 16, 28, 48, SSD1306_WHITE);
  display.drawLine(76, 16, 100, 48, SSD1306_WHITE);
  display.drawLine(100, 16, 76, 48, SSD1306_WHITE);
}

void drawGlitch() {
  // Randomized boxes and lines for a glitch effect
  for(int i=0; i<8; i++) {
    int x = random(0, 128);
    int y = random(0, 64);
    int w = random(10, 40);
    int h = random(2, 10);
    display.fillRect(x, y, w, h, SSD1306_WHITE);
  }
  // Rapid invert
  if ((currentMillis / 100) % 2 == 0) {
    display.invertDisplay(true);
  } else {
    display.invertDisplay(false);
  }
}

void drawVictory() {
  // Happy arches ^ ^
  display.drawCircle(40, 40, 16, SSD1306_WHITE);
  display.fillRect(20, 40, 40, 20, SSD1306_BLACK); // Cut bottom half
  
  display.drawCircle(88, 40, 16, SSD1306_WHITE);
  display.fillRect(68, 40, 40, 20, SSD1306_BLACK); // Cut bottom half
}

void drawShocked() {
  // Hollow circles O O
  display.drawCircle(40, 24, 18, SSD1306_WHITE);
  display.drawCircle(88, 24, 18, SSD1306_WHITE);
}

void drawSleepy() {
  // Sleepy lines _ _
  display.fillRoundRect(28, 44, 24, 6, 3, SSD1306_WHITE);
  display.fillRoundRect(76, 44, 24, 6, 3, SSD1306_WHITE);
}