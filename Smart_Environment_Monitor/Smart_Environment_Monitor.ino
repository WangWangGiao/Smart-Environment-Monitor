#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// DHT11 sensor
#define DHTPIN 4
#define DHTTYPE DHT11

// KY-037 sound sensor 
#define SOUND_PIN 34

// Touch pin (touch channel T7)
#define TOUCH_PIN 27

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_SDA 21
#define OLED_SCL 22

// Settings
#define TOUCH_RATIO 0.7      // touched when reading is below 70% of the normal value
#define SOUND_THRESHOLD 25   // this much above the quiet level = "Sound Detected"
#define SOUND_MAX 400        // this much above the quiet level = 100%

DHT dht(DHTPIN, DHTTYPE);

Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1
);

// Page: 1 = Environment, 2 = Sound
int page = 1;

// DHT11 values
float temperature = 0;
float humidity = 0;
bool dhtOk = false;
int dhtFailCount = 0;
unsigned long lastDhtTime = 0;

// Sound values
int quietLevel = 0;      // sound level in a quiet room (measured at start-up)
int soundLevel = 0;      // smoothed sound level
int soundPercent = 0;    
bool soundDetected = false;

// Touch values
int touchNormal = 0;     // touch reading when NOT touched (measured at start-up)
int touchThreshold = 0;  // below this = touched
int touchCount = 0;      // how many checks in a row the pin was touched

// Read the KY-037 for 50 ms and return (highest - lowest) reading.
// Quiet = small difference, loud = big difference.
int readSoundLevel() {
  int minValue = 4095;
  int maxValue = 0;
  unsigned long startTime = millis();

  while (millis() - startTime < 50) {
    int value = analogRead(SOUND_PIN);
    if (value < minValue) minValue = value;
    if (value > maxValue) maxValue = value;
    delayMicroseconds(200);  // about 5000 readings per second
  }
  return maxValue - minValue;
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED initialization failed!");
    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setCursor(0, 0);
  display.println("Calibrating...");
  display.setCursor(0, 16);
  display.println("Keep quiet and do");
  display.setCursor(0, 26);
  display.println("NOT touch the pin.");
  display.display();

  // Measure the normal touch value (pin not touched)
  touchRead(TOUCH_PIN);  // first reading starts the touch sensor
  delay(100);
  long touchTotal = 0;
  for (int i = 0; i < 20; i++) {
    touchTotal += touchRead(TOUCH_PIN);
    delay(50);
  }
  touchNormal = touchTotal / 20;
  touchThreshold = touchNormal * TOUCH_RATIO;

  // Measure the sound level in a quiet room
  long soundTotal = 0;
  for (int i = 0; i < 20; i++) {
    soundTotal += readSoundLevel();
  }
  quietLevel = soundTotal / 20;
  soundLevel = quietLevel;

  Serial.println("ESP32 Smart Environment Monitor");
  Serial.print("Touch normal value: ");
  Serial.print(touchNormal);
  Serial.print("  (touched below ");
  Serial.print(touchThreshold);
  Serial.println(")");
  Serial.print("Quiet sound level: ");
  Serial.println(quietLevel);
  Serial.println("------------------------------------");
}

void loop() {
  // 1. Read DHT11 every 2 seconds
  if (millis() - lastDhtTime >= 2000) {
    lastDhtTime = millis(); // use millis, so the loop does not stop
    readDHT();
  }

  // 2. Read sound sensor
  readSound();

  // 3. Check touch pin
  checkTouch();

  // 4. Show the current page
  if (page == 1) {
    showEnvironmentPage();
  } else {
    showSoundPage();
  }
}

void readDHT() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    dhtFailCount++;
    if (dhtFailCount >= 3) {
      dhtOk = false;  // show error only after 3 failed reads in a row
    }
    return;
  }

  temperature = t;
  humidity = h;
  dhtOk = true;
  dhtFailCount = 0;

  // Serial output
  Serial.print("Temperature: ");
  Serial.print(temperature, 1);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(humidity, 1);
  Serial.println(" %");

  Serial.print("Sound level: ");
  Serial.print(soundLevel);
  Serial.print("  (quiet ");
  Serial.print(quietLevel);
  Serial.println(")");

  Serial.print("Touch value: ");
  Serial.print(touchRead(TOUCH_PIN));
  Serial.print("  (touched below ");
  Serial.print(touchThreshold);
  Serial.println(")");

  Serial.println("------------------------------------");
}

void readSound() {
  int newLevel = readSoundLevel();

  // Smoothing: 3 parts old value + 1 part new value
  soundLevel = (soundLevel * 3 + newLevel) / 4; //75% of the previous value (soundLevel) with 25% of the newest reading

  int aboveQuiet = soundLevel - quietLevel;
  if (aboveQuiet < 0) {
    aboveQuiet = 0;
  }

  soundPercent = map(aboveQuiet, 0, SOUND_MAX, 0, 100);
  soundPercent = constrain(soundPercent, 0, 100);

  soundDetected = (aboveQuiet > SOUND_THRESHOLD);
}

void checkTouch() {
  int touchValue = touchRead(TOUCH_PIN);

  if (touchValue < touchThreshold) {
    touchCount++;    // finger is on the pin
  } else {
    touchCount = 0;  // finger removed
  }

  // Change page only when touched for 2 checks in a row (debounce).
  // If the finger stays on the pin, touchCount keeps going up (3, 4, 5...)
  // so the page does NOT keep changing.
  if (touchCount == 2) {
    if (page == 1) {
      page = 2;
    } else {
      page = 1;
    }
    Serial.print("Touch detected! Now showing page ");
    Serial.println(page);
  }

  if (touchCount > 100) {
    touchCount = 100;  // stop the counter from growing forever
  }
}

// White title bar with black text
void drawTitle(String title, String pageNumber) {
  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(3, 3);
  display.print(title);
  display.setCursor(107, 3);
  display.print(pageNumber);
  display.setTextColor(SSD1306_WHITE);
}

// "Status:" with the status word below it.
// Warnings are shown as black text on a white box.
void drawStatus(String status, bool warning) {
  display.setCursor(0, 36);
  display.print("Status:");

  if (warning) {
    display.fillRect(0, 44, status.length() * 6 + 3, 9, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  }
  display.setCursor(2, 45);
  display.print(status);
  display.setTextColor(SSD1306_WHITE);
}

// Line and touch instruction at the bottom
void drawFooter(String text) {
  display.drawLine(0, 54, 127, 54, SSD1306_WHITE);
  display.setCursor(0, 56);
  display.print(text);
}

void showEnvironmentPage() {
  display.clearDisplay();
  drawTitle("ENVIRONMENT", "1/2");

  // Temperature
  display.setCursor(0, 17);
  display.print("Temperature: ");
  if (dhtOk) {
    display.print(temperature, 1);
    display.print(" C");
  } else {
    display.print("--");
  }

  // Humidity
  display.setCursor(0, 27);
  display.print("Humidity   : ");
  if (dhtOk) {
    display.print(humidity, 0);
    display.print(" %");
  } else {
    display.print("--");
  }

  // Environment status
  String status;
  if (!dhtOk) {
    status = "Sensor Error";
  } else if (temperature > 30) {
    status = "Hot";
  } else if (temperature < 20) {
    status = "Cold";
  } else if (humidity > 70) {
    status = "Humid";
  } else if (humidity < 30) {
    status = "Dry";
  } else {
    status = "Normal";
  }
  drawStatus(status, status != "Normal");

  drawFooter("Touch > Sound");
  display.display();
}

void showSoundPage() {
  display.clearDisplay();
  drawTitle("SOUND MONITOR", "2/2");

  // Sound level
  display.setCursor(0, 17);
  display.print("Sound Level: ");
  display.print(soundPercent);
  display.print(" %");

  // Sound level bar
  display.drawRect(0, 27, 128, 7, SSD1306_WHITE);
  display.fillRect(2, 29, soundPercent * 124 / 100, 3, SSD1306_WHITE);

  // Sound status
  if (soundDetected) {
    drawStatus("Sound Detected", true);
  } else {
    drawStatus("Quiet", false);
  }

  drawFooter("Touch > Environment");
  display.display();
}
