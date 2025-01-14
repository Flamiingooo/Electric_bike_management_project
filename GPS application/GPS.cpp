/****************************************************************************************************
 *  BTS SNIR2 2024 - Author: Flamiingooo                                                           *
 *  Context:                                                                                       *
 *  This program implements a prototype for an embedded module designed for an electric bike.      *
 *  It uses an LCD display to show real-time data, retrieves the bike's GPS position, and provides *
 *  a battery level simulation using a potentiometer. When the battery drops below 15%, the module *
 *  activates a Wi-Fi access point to communicate with an Android app and trigger a buzzer alarm.  *
 ***************************************************************************************************/


#include <Arduino.h>
#include <TFT_eSPI.h>   // Software library for the LCD display.
#include "indicator.h"  // Battery level indicator
#include <TinyGPS++.h>  // Library for GPS data management.

#define simulation_in 13  // Analog input for simulating the battery level.
#define buzzer 12         // Digital output for controlling the buzzer.

TFT_eSPI tft = TFT_eSPI(); // Instantiation of an LCD display object.
TinyGPSPlus gps;           // Creation of an instance of the TinyGPSPlus object.
String latitude = "0";
String longitude = "0";
char recoveredFrame[256] = "";
int frameIndex = 0;

void extractCoordinates()
{
  if (gps.location.isValid())
  {
    latitude = String(gps.location.lat(), 6);
    longitude = String(gps.location.lng(), 6);

    int satellites = gps.satellites.value();
    Serial.print("Number of satellites: ");
    Serial.println(satellites);
  }
}

void setup(void)
{
  pinMode(simulation_in, INPUT);
  pinMode(buzzer, OUTPUT);
  tft.begin();               // Initialization of the LCD display.
  tft.setRotation(1);        // Set display to landscape mode.
  tft.fillScreen(TFT_BLACK);

  Serial.println("TEST of the embedded module prototype on the bike");
  digitalWrite(buzzer, HIGH); 
  delay(3000);
  digitalWrite(buzzer,LOW);

  Serial.println("Initializing GPS communication");
  Serial2.begin(9600); // GPS communication
}

/*****************************************************************************************************
 *  The main code retrieves the bike's position, displays it on the LCD screen,                     *
 *  and simulates battery autonomy (0 to 100%) using a potentiometer.                               *
 *  When the battery drops below 15%, the module activates its Wi-Fi in access point mode to         *
 *  communicate with the technician's Android app and trigger the buzzer.                           *
 *****************************************************************************************************/

void loop()
{
  int xpos = 180, ypos = 30, gap = 4, radius = 135; // Parameters for the battery level indicator.
  int battery = analogRead(simulation_in) * 100 / 4095;
  ringMeter(tft, battery, 0, 100, xpos, ypos, radius, "%", RED2GREEN); // Draw the indicator on the screen.

  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(0, 0, 4);
  
  /****************GPS Section****************/
  if (Serial2.available() > 0) // Check if data is available.
  {
    char character = Serial2.read();

    if (frameIndex < sizeof(recoveredFrame) - 1) // Check if there is space in the buffer for a new character.
    { 
      recoveredFrame[frameIndex] = character;
      frameIndex++;
    }

    // Check for a new line.
    if (character == '\n')
    {
      recoveredFrame[frameIndex] = '\0'; // End the string.
      frameIndex = 0;

      // Compare the first 6 characters with "$GNGGA".
      if (strncmp(recoveredFrame, "$GNGGA", 6) == 0)
      {
        for (int i = 0; i < strlen(recoveredFrame); i++)
        {
          gps.encode(recoveredFrame[i]); // Process the frame to extract latitude and longitude.
        }
        extractCoordinates(); 
      }
      memset(recoveredFrame, 0, sizeof(recoveredFrame)); // Reset the buffer.
    }

    tft.setTextColor(TFT_BLUE); 
    tft.setCursor(0, 0, 4);  
    tft.print("LAT = "); tft.println(latitude);   
    tft.print("LONG = "); tft.println(longitude);
  }

  /****************Wi-Fi Section****************/
  // Activate Wi-Fi access point and communicate with the Android app.
  /*if (battery < 15)
  {
    // Zeikrom251's code.
  }
}
