/******************************************************************************
  SparkFun_Photon_Weather_Basic_Soil.ino
  SparkFun Photon Weather Shield basic example with soil moisture and temp
  Joel Bartlett @ SparkFun Electronics
  Original Creation Date: May 18, 2015
  This sketch prints the temperature, humidity, barrometric preassure, altitude,
  to the Seril port.

  Hardware Connections:
	This sketch was written specifically for the Photon Weather Shield,
	which connects the HTU21D and MPL3115A2 to the I2C bus by default.
  If you have an HTU21D and/or an MPL3115A2 breakout,	use the following
  hardware setup:
      HTU21D ------------- Photon
      (-) ------------------- GND
      (+) ------------------- 3.3V (VCC)
       CL ------------------- D1/SCL
       DA ------------------- D0/SDA

    MPL3115A2 ------------- Photon
      GND ------------------- GND
      VCC ------------------- 3.3V (VCC)
      SCL ------------------ D1/SCL
      SDA ------------------ D0/SDA

  Development environment specifics:
  	IDE: Particle Dev
  	Hardware Platform: Particle Photon
                       Particle Core

  This code is beerware; if you see me (or any other SparkFun
  employee) at the local, and you've found our code helpful,
  please buy us a round!
  Distributed as-is; no warranty is given.
*******************************************************************************/
#include "HTU21D.h"
#include "SparkFun_MPL3115A2.h"
#include "core_hal.h"

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

float humidity = 0;
float tempf = 0;
float pascals = 0;
float altf = 0;
float baroTemp = 0;

int count = 0;

HTU21D htu = HTU21D();//create instance of HTU21D Temp and humidity sensor
MPL3115A2 baro = MPL3115A2();//create instance of MPL3115A2 barrometric sensor

void printInfo();
void getTempHumidity();
void getBaro();
void calcWeather();

//---------------------------------------------------------------
void setup()
{
    Serial.begin(9600);   // open serial over USB at 9600 baud

    //Initialize both on-board sensors
    //Initialize both on-board sensors
    while(! htu.begin()){
  	    Serial.println("HTU21D not found");
  	    delay(1000);
  	}
  	Serial.println("HTU21D OK");

  	while(! baro.begin()) {
          Serial.println("MPL3115A2 not found");
          delay(1000);
     }
     Serial.println("MPL3115A2 OK");

     //MPL3115A2 Settings
     //baro.setModeBarometer();//Set to Barometer Mode
     baro.setModeAltimeter();//Set to altimeter Mode

     baro.setOversampleRate(7); // Set Oversample to the recommended 128
     baro.enableEventFlags(); //Necessary register calls to enble temp, baro ansd alt

}
//---------------------------------------------------------------
void loop()
{
    // Button press needs to be 1 second long
    if (HAL_Core_Mode_Button_Pressed(1000)) {
        if (!Spark.connected()) {
            WiFi.on();
            Spark.connect();
        }
        else {
            Spark.disconnect();
            WiFi.off();
        }
        // delay 2.1 more seconds before resetting debounce time
        // in case user is trying to enter Listening Mode
        delay(2100);
        HAL_Core_Mode_Button_Reset();
    }

    //Get readings from all sensors
    calcWeather();
    printInfo();

    /*
    //Rather than use a delay, keeping track of a counter allows the photon to
    //still take readings and do work in between printing out data.
    count++;
    //alter this number to change the amount of time between each reading
    if(count == 5)//prints roughly every 10 seconds for every 5 counts
    {
       printInfo();
       count = 0;
    }
    */
    Spark.process();
}
//---------------------------------------------------------------
void printInfo()
{
    //This function prints the weather data out to the default Serial Port
    static int errors = 0;
    static int readings = 0;

    //Take the temp reading from each sensor and average them.
    // Serial.print("Temp:");
    // Serial.print((tempf+humidity)/2);
    // Serial.print("F, ");
    // if (abs(baroTemp)>1000) error++;
    // if (abs(tempf)>1000) error++;

    //Serial.println("");
    //return;

    //Or you can print each temp separately
    Serial.print("HTU21D Temp:");
    Serial.print(tempf);
    Serial.print("F, ");
    if (abs(tempf)>1000) errors++;

    Serial.print("Humidity:");
    Serial.print(humidity);
    Serial.print("%, ");
    if (abs(humidity)>200) errors++;

    Serial.print("Baro Temp:");
    Serial.print(baroTemp);
    Serial.print("F, ");
    //Serial.println();
    if (abs(baroTemp)>1000) errors++;

    Serial.print("Pressure:");
    Serial.print(pascals);
    Serial.print("Pa, ");
    if (abs(pascals)>900) errors++;

    Serial.print("Altitude:");
    Serial.print(altf);
    Serial.print("ft, ");
    if (abs(altf)>3200) errors++;

    readings += 5;
    Serial.print("Errors/Total:");
    Serial.print(errors);
    Serial.print("/");
    Serial.print(readings);
    Serial.println();

}
//---------------------------------------------------------------
void getTempHumidity()
{
    float temp = 0;

    temp = htu.readTemperature();
    tempf = (temp * 9)/5 + 32;

    humidity = htu.readHumidity();
}
//---------------------------------------------------------------
void getBaro()
{
  baroTemp = baro.readTempF();//get the temperature in F
  //delay(100);

  pascals = baro.readPressure();//get pressure in Pascals
  //delay(20);

  altf = baro.readAltitudeFt();//get altitude in feet
  //delay(100);
}
//---------------------------------------------------------------
void calcWeather()
{
    getTempHumidity();
    getBaro();
}
//---------------------------------------------------------------
