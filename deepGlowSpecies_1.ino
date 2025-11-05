//Make really cool features with LEDSSSSSSSSS 

//Also made a really cool array that is wild n out.

#include <FastLED.h>
#include "Wire.h"

//LED SETUP
#define LED_PIN     3
#define NUM_LEDS    160
#define BRIGHTNESS  100
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

//SENSOR SETUP
const int MPU_addr = 0x68;
int16_t ax, ay, az;

//MOTION DETECTION - Variance method without memory
float motionLevel = 0;
const float motionThreshold = 2.0; // Adjust this value
bool isMoving = false;

// Store recent readings for variance calculation
int16_t accel_history[10][3]; // Store last 10 readings
int history_index = 0;
bool history_full = false;

// Motion timing variables
unsigned long motionStartTime = 0;
unsigned long motionDuration = 0;
const unsigned long longMotionThreshold = 3000; // 3 seconds of continuous motion
bool isLongMotion = false;

//ANIMATION VARIABLES
uint8_t rainbowIndex = 0;
unsigned long lastStrobeTime = 0;
bool strobeState = false;

// Missing variables for rainbowFadeEffect
uint8_t currentHue = 0;
uint8_t targetHue = 0;
unsigned long lastHueChange = 0;

void setup() {
  Serial.begin(9600);
  delay(3000);
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  
  // Initialize history array
  for(int i = 0; i < 10; i++) {
    accel_history[i][0] = 0;
    accel_history[i][1] = 0; 
    accel_history[i][2] = 0;
  }
  
  // Initialize hue change timer
  lastHueChange = millis();
  
  Serial.println("Variance Motion Detection with Blue Pulse");
  Serial.println("Still = Rainbow | Moving = Blue Pulse");
}

void loop() {
  readSensor();
  detectMotion();
  updateLEDs();
  
  FastLED.show();
  delay(20);
}

void readSensor() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 6, true);
  
  ax = Wire.read() << 8 | Wire.read();
  ay = Wire.read() << 8 | Wire.read();
  az = Wire.read() << 8 | Wire.read();
  
  // Store in history
  accel_history[history_index][0] = ax;
  accel_history[history_index][1] = ay;
  accel_history[history_index][2] = az;
  
  history_index++;
  if (history_index >= 10) {
    history_index = 0;
    history_full = true;
  }
}

void detectMotion() {
  if (!history_full) return; // Need full history first
  
  // Calculate variance over the last 10 readings
  float sum_x = 0, sum_y = 0, sum_z = 0;
  float variance_x = 0, variance_y = 0, variance_z = 0;
  
  // Calculate averages
  for(int i = 0; i < 10; i++) {
    sum_x += accel_history[i][0];
    sum_y += accel_history[i][1];
    sum_z += accel_history[i][2];
  }
  float avg_x = sum_x / 10.0;
  float avg_y = sum_y / 10.0;
  float avg_z = sum_z / 10.0;
  
  // Calculate variance
  for(int i = 0; i < 10; i++) {
    variance_x += pow(accel_history[i][0] - avg_x, 2);
    variance_y += pow(accel_history[i][1] - avg_y, 2);
    variance_z += pow(accel_history[i][2] - avg_z, 2);
  }
  
  float totalVariance = (variance_x + variance_y + variance_z) / 1000000.0;
  
  // Smooth the motion value
  motionLevel = motionLevel * 0.8 + totalVariance * 0.2;
  
  // Direct threshold check - no memory
  bool previousMoving = isMoving;
  isMoving = (motionLevel > motionThreshold);
  
  // Track motion timing
  if (isMoving && !previousMoving) {
    // Motion just started
    motionStartTime = millis();
    isLongMotion = false;
  } else if (!isMoving && previousMoving) {
    // Motion just stopped
    isLongMotion = false;
  } else if (isMoving) {
    // Continuous motion - check duration
    motionDuration = millis() - motionStartTime;
    if (motionDuration >= longMotionThreshold) {
      isLongMotion = true;
    }
  }
  
  // Debug output
  Serial.print("Variance: ");
  Serial.print(totalVariance, 2);
  Serial.print(" | Smoothed: ");
  Serial.print(motionLevel, 2);
  Serial.print(" | Moving: ");
  Serial.print(isMoving ? "YES" : "NO");
  Serial.print(" | Long Motion: ");
  Serial.println(isLongMotion ? "YES" : "NO");
}

void updateLEDs() {
  if (isMoving && isLongMotion) {
    responsiveBluePulse();  // Blue pulse when moving for a long time
  } else if (isMoving) {
    strobeEffect();  // Strobe when moving for short time
  } else {
    //rainbowEffect();    // Rainbow when still
    rainbowFadeEffect(); 
  }
}

void rainbowEffect() {
  rainbowIndex += 1;
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t colorIndex = rainbowIndex + (i * 3);
    leds[i] = ColorFromPalette(RainbowColors_p, colorIndex, 255, LINEARBLEND);
  }
}

void rainbowFadeEffect() {
  // Smoothly fade from one rainbow color to the next
  
  // Change target color every 3 seconds
  if (millis() - lastHueChange > 3000) {
    currentHue = targetHue;
    targetHue += 43; // Move to next rainbow color (256/6 ≈ 43)
    if (targetHue >= 256) targetHue = 0;
    lastHueChange = millis();
  }
  
  // Smoothly interpolate between current and target hue
  static uint8_t displayHue = 0;
  if (displayHue != targetHue) {
    // Smooth transition between colors
    int16_t hueDistance = targetHue - displayHue;
    if (hueDistance > 128) hueDistance -= 256;
    if (hueDistance < -128) hueDistance += 256;
    
    if (abs(hueDistance) > 1) {
      displayHue += (hueDistance > 0) ? 1 : -1;
    } else {
      displayHue = targetHue;
    }
  }
  
  // Fill entire strip with the current color
  fill_solid(leds, NUM_LEDS, CHSV(displayHue, 255, 200));
}

void strobeEffect() {
  unsigned long strobeSpeed = 100 - constrain(motionLevel * 5, 0, 80);
  
  if (millis() - lastStrobeTime > strobeSpeed) {
    strobeState = !strobeState;
    lastStrobeTime = millis();
    
    if (strobeState) {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
    } else {
      fill_solid(leds, NUM_LEDS, CRGB::Red);
    }
  }
}

// More advanced blue pulse with motion-responsive speed
void responsiveBluePulse() {
  // Slower, calmer pulse - reduced speed range
  uint8_t pulseSpeed = 20 + constrain(motionLevel * 10, 0, 20); // 10-30 BPM (much slower)
  uint8_t pulse = beatsin8(pulseSpeed, 0, 255); // Higher minimum brightness for calmer effect
  fill_solid(leds, NUM_LEDS, CHSV(160, 255, pulse)); // Hue 160 = most saturated blue
}
