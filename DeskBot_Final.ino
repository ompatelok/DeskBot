#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

// --- HARDWARE SETUP ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUZZER_PIN 12 // D6
#define TOUCH_PIN 14  // D5

// --- WI-FI & AWS CONFIG ---
const char* ssid = "ROME";
const char* password = "helloworld222";
const char* aws_endpoint = "a260nvvzszz66f-ats.iot.us-east-1.amazonaws.com";

// ==============================================================================
// 1. Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

// 2. Device Certificate
static const char AWS_CERT_CRT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIURY14cJGxrQez0J8lR/RMgsDnktkwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI2MDMxNDE1MzYz
NloXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALasCtrAuY4rnNsizifx
hrHVm/ARbdzVUswOi14Sh4l8gcAYyzV65BdvZkUsp6PCJRakHAoW8bsls7l7FlKW
Bl4n1tzdqqHqk88FX0g4xBGblg3AFdl3h3dnx9dOu/pyZQcgaIJGALgPVb9kxXBY
wzWmLYIm/92u/TzCP5x/yrOP7KFacp7H5gGXs5z0EqkL/xcKVSLRHd/Rw+G+yp3E
tgKop5LWTkwyHgmenCiXHPSXkwPiQZTbjRDiYhEH7gNB2LDlHzA+YjuCVAYt0XFQ
ovHzjwcoTwP8m8mld/IoHFwy51VGJaGoA1y82RvmP0TQ+75clFJDB5xrtO90uKHb
1GcCAwEAAaNgMF4wHwYDVR0jBBgwFoAUAUfPzMZqOeQNGA2Y3TjlyyxPa88wHQYD
VR0OBBYEFDKF41JDNYbSjFmwR6LaIsyHfTvOMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQAW5yW5rEyMiLJuJFkk+FDgMdJa
/aiKD8GBc0WflILLTh4ZWve+eCnH8BFszSU+Bg0C11Tdo+By/8axWT4hi/w3hELs
mETt59bCH2kFcHQBGlswijMy1a5YvQdrBjoJOkLMiGED9iDMMrqbLOojXrkwLVoQ
8JhWQkbfXiNmWvoA8khqWcSdmWHDDiESjsDEiUVn8Xoss1zLIPiNdJqXiDAY34jw
Q+/yU/hzvyQ6VRgGRzesLi3MWlJ0s580iPyAJ70ytFKwCSB3iXfpq1UqGMPe3x53
bjwFY0GFMq3tYT1jf47V4R++Fql74oEe4ZvHRcks02DpPm6WrOnIqv5btQj1
-----END CERTIFICATE-----
)EOF";

// 3. Private Key
static const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAtqwK2sC5jiuc2yLOJ/GGsdWb8BFt3NVSzA6LXhKHiXyBwBjL
NXrkF29mRSyno8IlFqQcChbxuyWzuXsWUpYGXifW3N2qoeqTzwVfSDjEEZuWDcAV
2XeHd2fH1067+nJlByBogkYAuA9Vv2TFcFjDNaYtgib/3a79PMI/nH/Ks4/soVpy
nsfmAZeznPQSqQv/FwpVItEd39HD4b7KncS2AqinktZOTDIeCZ6cKJcc9JeTA+JB
lNuNEOJiEQfuA0HYsOUfMD5iO4JUBi3RcVCi8fOPByhPA/ybyaV38igcXDLnVUYl
oagDXLzZG+Y/RND7vlyUUkMHnGu073S4odvUZwIDAQABAoIBAQCx0FJYkdWNsKJl
JUIr241TuiyqySx6FCUegOHk1oCcslfS18+m5VL2BPg2Sbt9SlSz71dj8uWnWJHT
8R7PkT3tlTYvlI6uQhtTH1WahbdFtH+iLuKV5yY0cw8yZG/S4Fu7Ti6s8NtuT28W
eU8/hweuR25xDOlvrxDOff9RpYt4sPDucEdznnV9/B35hwQpAW7FqS6+gs6wu8/s
mWqHf5aDUmT1Tq8h4rexDJat1PKzOZE37MCxiwU/SK6NYJ3Ud3t3YWWDhMgb4XvC
3IKtG7T8P90MLFtxit5LhjCuIt1gpYFNT+i2ueWWnQDN+1y0qgj7mgI6+Wbr6YjR
ymODGoOBAoGBAOSLidn9LaKFUiZbXTKdqAhqxgdK+UR01c59m2wN5fPfa5NPXxFp
iBqwvNo1NFQxcgRFvsAzx9bxYHyyk/lPvVhhqcFzOpHZqtGlY5+Kj5Hd7eMjFD+u
JBKwNfflDswCSxQADPvzzA1nfFnloOpTaPwEgbeQ1Xqrb8V4Zggy9OahAoGBAMyd
xIBxGlPO5jJv81usoZAO6Zo+BZd6wMHjQxEgBYwSbclsgNfOR3MTZKDBEsvwmI7X
UNtdGOrBWT7rxGLLlv41EHC/dBzp9B1jPJKrnkhJwG9223BuqRRWhKnbPoYJt0cH
sFq25NP7jMK+1GfT4l7HA2jVo3HG0pVYf4kDKcYHAoGASbz/0DaQt8QeOkP2MPOP
GJAiro2xGTY9Ga+LKvTdAa9PTSspuDt9b2cgt6K87IE7kUdTgT3zt1UP1YvklGb2
54IzPDhKaG57X8Ll/r69Dco2C7fwu48bEnCKKR5dhNMkmCJgDhad+qEuDO862P/q
eo2VsL1pM3gGywbKnNCcFaECgYAbilypgQ+vGhkHzuOtgiXY0vkERrbP9bi+IA2l
325/tPdL9iu9YEljpseTj4ktI/wUtcxvSi+RYWbWvjsXpRl0q8XDtzE0txShY/sC
LtFGH48kMZZFos9lKmP+zMC0n9goWlOVUiqokaIXkEHcm7tLUHQYBzFRnhY3/cpk
saVMvwKBgBc9ASfk46guyg/eSeCjkEHu4ZxcP+kQq0saN8OrBPpV32Sc0878rQAz
4VMao71t9I/raDta1RCNp7FXOg0E/Kl0IpBsxfwTf5A4SqZ/TfB2DAX82Ny12U9E
quX1jXDV+DHhwSNRabwXXsjA+s7969HIEt/CBtClu43w+IroVTGB
-----END RSA PRIVATE KEY-----
)EOF";
// ==============================================================================

// --- Cloud & State Variables ---
WiFiClientSecure net;
BearSSL::X509List cert(AWS_CERT_CRT);
BearSSL::PrivateKey key(AWS_CERT_PRIVATE);
BearSSL::X509List rootCA(AWS_CERT_CA);
PubSubClient client(net);

String currentAIState = "Good Posture";
bool stateChanged = false;

// --- OS UI Variables ---
int uiMode = 0; // 0=Face, 1=Time, 2=Notifications
unsigned long currentMillis = 0;

// Touch Sensor
bool touchActive = false;
unsigned long touchStartTime = 0;
unsigned long lastTapTime = 0;
int tapCount = 0;
bool isOverridden = false;
int overrideType = 0; 
unsigned long overrideEndTime = 0;

// Animation
float currentLookX = 0, targetLookX = 0;    
float currentSquish = 0, targetSquish = 0;  
unsigned long lastLookChange = 0;
unsigned long lastBlinkTime = 0;

// Dynamic Tips
const char* healthTips[] = {"Hydrate! Water!", "Blink your eyes.", "Sit up straight.", "Take a deep breath."};
int currentTipIndex = 0;
unsigned long lastTipChange = 0;

// --- ROBUST BUZZER FUNCTION ---
// If you have an active buzzer, change 'tone(BUZZER_PIN, freq)' to 'digitalWrite(BUZZER_PIN, HIGH)'
void playBeep(int freq, int duration) {
  tone(BUZZER_PIN, freq);
  delay(duration);
  noTone(BUZZER_PIN); // Force the buzzer to stop ringing
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TOUCH_PIN, INPUT);
  
  Wire.begin();
  Wire.setClock(400000L); 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // Boot Screen
  display.clearDisplay(); display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 25); display.print("Connecting Wi-Fi..."); display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  // Sync Clock
  display.clearDisplay(); display.setCursor(10, 25); display.print("Syncing Time..."); display.display();
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); // IST Offset
  time_t now = time(nullptr);
  while (now < 100000) { delay(500); now = time(nullptr); }

  // Connect AWS
  display.clearDisplay(); display.setCursor(10, 25); display.print("Connecting AWS..."); display.display();
  net.setTrustAnchors(&rootCA);
  net.setClientRSACert(&cert, &key);
  client.setServer(aws_endpoint, 8883);
  client.setCallback(messageReceived);

  connectAWS();
}

void connectAWS() {
  while (!client.connected()) {
    if (client.connect("DeskBot_Robot")) {
      client.subscribe("deskbot/posture");
      playBeep(1500, 200);
    } else {
      delay(3000);
    }
  }
}

void messageReceived(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];
  
  StaticJsonDocument<200> doc;
  if (!deserializeJson(doc, message)) {
    String newState = doc["state"];
    if (newState != currentAIState) {
      currentAIState = newState;
      stateChanged = true;
    }
  }
}

void loop() {
  currentMillis = millis();
  
  if (!client.connected()) connectAWS();
  client.loop(); // Keep AWS alive

  handleTouch();

  // Rotate dynamic tip
  if (currentMillis - lastTipChange > 15000) {
    currentTipIndex = (currentTipIndex + 1) % 4;
    lastTipChange = currentMillis;
  }

  // Handle AWS Audio Alerts
  if (stateChanged) {
    if (currentAIState == "Slouching") {
      playBeep(800, 150); playBeep(600, 300); // Sad Beep
    } else if (currentAIState == "Good Posture") {
      playBeep(1200, 100); playBeep(1800, 150); // Good Beep
    }
    stateChanged = false; 
    uiMode = 0; // Force UI back to face to show the warning
  }

  // Draw OS
  display.clearDisplay();

  if (isOverridden) {
    if (currentMillis < overrideEndTime) {
      if (overrideType == 1) drawShyFace();
      else if (overrideType == 2) drawGlitchFace();
    } else {
      isOverridden = false; targetSquish = 0;
    }
  } else {
    if (uiMode == 0) {
      if (currentAIState == "Good Posture") drawFluidFace();
      else if (currentAIState == "Slouching") drawAngryFace(); 
      else if (currentAIState == "Not At Desk") drawSleepyFace();
    } else if (uiMode == 1) {
      drawTimeDateMenu();
    } else if (uiMode == 2) {
      drawNotificationMenu();
    }
  }

  display.display(); 
}

// ==========================================
// --- TOUCH LOGIC ---
// ==========================================
void handleTouch() {
  bool isTouched = digitalRead(TOUCH_PIN) == HIGH;
  if (isTouched && !touchActive) { touchActive = true; touchStartTime = currentMillis; } 
  else if (!isTouched && touchActive) {
    touchActive = false;
    if (currentMillis - touchStartTime > 600) { uiMode = 0; playBeep(1000, 150); tapCount = 0; } 
    else { tapCount++; lastTapTime = currentMillis; }
  }

  if (tapCount > 0 && (currentMillis - lastTapTime > 350)) {
    if (tapCount == 1) { uiMode = (uiMode + 1) % 3; playBeep(2000, 50); } 
    else if (tapCount == 2) { uiMode = 0; isOverridden = true; overrideType = 1; overrideEndTime = currentMillis + 3000; playBeep(1500, 100); playBeep(2000, 150); } 
    else if (tapCount >= 3) { uiMode = 0; isOverridden = true; overrideType = 2; overrideEndTime = currentMillis + 1500; playBeep(150, 200); }
    tapCount = 0; 
  }
}

// ==========================================
// --- DRAWING FUNCTIONS ---
// ==========================================
void drawFluidFace() {
  if (currentMillis - lastLookChange > random(3000, 6000)) { targetLookX = random(-15, 16); lastLookChange = currentMillis; }
  if (currentMillis - lastBlinkTime > 4000) { targetSquish = 38; if (currentSquish > 36) { targetSquish = 0; lastBlinkTime = currentMillis; } } else { targetSquish = 0; }

  currentLookX += (targetLookX - currentLookX) * 0.15;
  currentSquish += (targetSquish - currentSquish) * 0.35;
  float breathOffset = sin(currentMillis / 350.0) * 2.0;

  int drawHeight = 40 + breathOffset - currentSquish;
  if (drawHeight < 4) drawHeight = 4; 
  int drawY = 12 + (40 - drawHeight) / 2;

  display.fillRoundRect(28 + currentLookX, drawY, 26, drawHeight, 10, SSD1306_WHITE);
  display.fillRoundRect(76 + currentLookX, drawY, 26, drawHeight, 10, SSD1306_WHITE);
}

void drawAngryFace() {
  display.fillRoundRect(34, 16, 22, 36, 8, SSD1306_WHITE);
  display.fillRoundRect(72, 16, 22, 36, 8, SSD1306_WHITE);
  display.fillTriangle(30, 10, 60, 30, 60, 10, SSD1306_BLACK);
  display.fillTriangle(98, 10, 68, 30, 68, 10, SSD1306_BLACK);
}

void drawSleepyFace() {
  display.setTextSize(1);
  display.setCursor(90, 10 - (currentMillis/100 % 10)); display.print("Z");
  display.setCursor(105, 5 - (currentMillis/100 % 15)); display.print("z");
  display.fillRoundRect(28, 44, 24, 6, 3, SSD1306_WHITE);
  display.fillRoundRect(76, 44, 24, 6, 3, SSD1306_WHITE);
}

void drawShyFace() {
  display.drawCircle(36, 40, 16, SSD1306_WHITE); display.fillRect(16, 40, 40, 20, SSD1306_BLACK); 
  display.drawCircle(92, 40, 16, SSD1306_WHITE); display.fillRect(72, 40, 40, 20, SSD1306_BLACK); 
  display.fillCircle(15, 35, 4, SSD1306_WHITE);  display.fillCircle(113, 35, 4, SSD1306_WHITE);
}

void drawGlitchFace() {
  for(int i=0; i<6; i++) display.fillRect(random(0, 128), random(0, 64), random(10, 50), random(2, 8), SSD1306_WHITE);
  if ((currentMillis / 50) % 2 == 0) display.invertDisplay(true); else display.invertDisplay(false);
}

void drawTimeDateMenu() {
  time_t now = time(nullptr); struct tm* timeinfo = localtime(&now);
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 0); display.print("LOCAL TIME"); display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  if (timeinfo->tm_year < 100) return;

  char timeStr[10]; int hour12 = timeinfo->tm_hour % 12; if (hour12 == 0) hour12 = 12; 
  sprintf(timeStr, "%02d:%02d", hour12, timeinfo->tm_min);
  display.setTextSize(3); display.setCursor(20, 20); display.print(timeStr);
  display.setTextSize(1); display.setCursor(110, 20); display.print(timeinfo->tm_hour >= 12 ? "PM" : "AM");

  char dateStr[20]; sprintf(dateStr, "%02d/%02d/%04d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);
  display.setCursor(30, 50); display.print(dateStr);
}

void drawNotificationMenu() {
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 0); display.print("SYSTEM ALERTS"); display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  display.setCursor(0, 18); display.print("> Wi-Fi: Active");
  display.setCursor(0, 32); display.print(String("> AI: ") + currentAIState);
  display.fillRect(0, 46, 128, 18, SSD1306_WHITE); display.setTextColor(SSD1306_BLACK);
  display.setCursor(3, 51); display.print(healthTips[currentTipIndex]);
}