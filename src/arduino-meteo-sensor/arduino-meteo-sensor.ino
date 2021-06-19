/**
 * Copyright (c) 2021 Artem Hlumov
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <dht_nonblocking.h>
#include <LiquidCrystal.h>

// Define pins for LCD display.
#define LCD_RS_PIN 7
#define LCD_E_PIN 8
#define LCD_DB4_PIN 9
#define LCD_DB5_PIN 10
#define LCD_DB6_PIN 11
#define LCD_DB7_PIN 12
#define LCD_VDD_PIN 3
// Define a data pin for temperature-humidity sensor.
#define DHT_DATA_PIN 2
// Analogue pin of the sound sensor.
#define SOUND_ANALOGUE_PIN A0

// Delta of the noise value when the device should switch on LCD display.
// Should be adjusted according to the sound sensor sensitivity.
#define NOISE_THRESHOLD 5
// Timeout in milliseconds when the display should be switched on after wake up.
#define LCD_DISPLAY_TIMEOUT 5000ul

// Initialize temperature-humidity sensor.
DHT_nonblocking dhtSensor(DHT_DATA_PIN, DHT_TYPE_11);

// Initiaize LCD display.
LiquidCrystal lcd(LCD_RS_PIN, LCD_E_PIN, LCD_DB4_PIN, LCD_DB5_PIN, LCD_DB6_PIN, LCD_DB7_PIN);

// Set up function.
void setup() {
  // Initialize LCD display with amount of rows and columns.
  lcd.begin(16, 2);
  // Input from the temperature-humidity sensor.
  pinMode(DHT_DATA_PIN, INPUT);
  // Digital output to control backlight of the LCD display.
  pinMode(LCD_VDD_PIN, OUTPUT);
}

// When the display is waked up, the variable keeps timestamp of when it happened
// to measure display timeout. Initially it contains -LCD_DISPLAY_TIMEOUT
// to prevent switching on display at the startup.
long lcdOnTimestamp = -LCD_DISPLAY_TIMEOUT;
// Previous value of the noise from the sound sensor to measure delta.
// Initiall it has maximal value to prevent waking up the device at the startup.
int previousNoiseValue = 1024;
// Indicate whether the measurement has been taken while the wake up phase.
// The flag is switched to false when the display turns off.
bool isMeasurementTaken = false;
// Last saved value of temperature.
float temperature;
// Last saved value of humidity.
float humidity;

void loop() {
  // Read noise value from the sound sensor.
  int noiseValue =  analogRead(SOUND_ANALOGUE_PIN);
  // If the noise level reaches the theshold, the display should be switched on.
  // Remember the timestamp of this event.
  if (noiseValue >= previousNoiseValue + NOISE_THRESHOLD) {
    lcdOnTimestamp = millis();
  }
  // Save previous noise value to calculate delta on the next loop.
  previousNoiseValue = noiseValue;
  // While the display should remain switched on.
  if (millis() - lcdOnTimestamp < LCD_DISPLAY_TIMEOUT) {
    // Apply power to the LCD backlight pin.
    digitalWrite(LCD_VDD_PIN, HIGH);
    // If the measurement was no done yet, take it.
    // We take measurements each time the device wakes up.
    if (!isMeasurementTaken) {
      // Try to measure temperature and humidity.
      // Note that we might need to repeate it several times until we get success.
      if(dhtSensor.measure(&temperature, &humidity)) {
        // Print out the first row.
        lcd.setCursor(0, 0);
        lcd.print("T = ");
        lcd.print(temperature, 1);
        lcd.print(" C");
        // Print out the second row.
        lcd.setCursor(0, 1);
        lcd.print("H = ");
        lcd.print(humidity, 1);
        lcd.print(" %");
        // Mark the measurement completed for this display cycle.
        isMeasurementTaken = true;
      }
    }
  } else {
    // Remove prower from the LCD backlight pin.
    digitalWrite(LCD_VDD_PIN, LOW);
    // Drop measurements flag in order to take measurements at the next cycle.
    isMeasurementTaken = false;
  }
}
