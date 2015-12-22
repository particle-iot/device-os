/***************************************************
  This code is based on an example for the Adafruit VS1053 Codec Breakout

  Designed specifically to work with the Adafruit VS1053 Codec Breakout
  ----> https://www.adafruit.com/products/1381

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  Adapted by Paul Kourany for the Particle Photon
  https://github.com/pkourany/Adafruit_VS1053_Library

  BSD license, all text above must be included in any redistribution
****************************************************/
#include "application.h"

SerialDebugOutput debugOutput(9600, ALL_LEVEL);

#include "vs1053/Adafruit_VS1053.h"
#include "vs1053/SD.h"

#if (PLATFORM_ID == 0) || (PLATFORM_ID >= 3)

#define BREAKOUT_RESET D7 // VS1053 reset pin (output)
#define BREAKOUT_CS    D5 // VS1053 chip select pin (output)
#define BREAKOUT_XDCS  D6 // VS1053 Data/command select pin (output)
#define CARD_SDCS      A0 // Card chip select pin
#define DREQ           A1 // VS1053 Data request, ideally an Interrupt pin

#endif

// relay pins
#define LFT_EYE D0
#define RGT_EYE D1
#define SILLY_STRING_1 D2
#define SILLY_STRING_2 D3

// fn sigs
void play();
int remotePlay(String track);
void pauseOrResume();
int remotePauseOrResume(String noop);
void stop();
int remoteStop(String noop);

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(BREAKOUT_RESET, BREAKOUT_CS, BREAKOUT_XDCS, DREQ, CARD_SDCS);

// how long the actuator will stay down to spray the silly string
int spitDuration = 500;
int sprayCan = SILLY_STRING_1; // D2 or D3
int autoMode = 0;
int spitDistance = 5;

SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {
  // Particle.connect();
  Serial.begin(9600);

  delay(5000); // Allow board to settle

  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println("Couldn't find VS1053. Not doing anything else...");
     while (1);
  }
  Serial.println("VS1053 found.");

  SD.begin(CARD_SDCS);    // initialise the SD card
  Serial.println("SD initialized.");

  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(1,1);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int

  Particle.function("play", remotePlay);


  // Play one file, don't return until complete
  // Serial.println(F("Playing track 001"));
  //musicPlayer.playFullFile("track001.mp3");

  // Play another file in the background, REQUIRES interrupts!
  /*Serial.println("Playing track");
  musicPlayer.startPlayingFile("track002.mp3");*/
}

void loop() {
  // File is playing in the background
  // if (! (musicPlayer.stopped() || musicPlayer.paused() ) ) {
    /*Serial.println("Done playing music");*/
    /*while (1);*/
    play();
    delay(5000);
  // }

  /*delay(100);*/
}

void play() {
  Serial.println("Playing track.");
  musicPlayer.startPlayingFile("track001.mp3");
}

int remotePlay(String track) {
  play();
  return 1;
}

void pauseOrResume() {
  if (! musicPlayer.paused()) {
    Serial.println("pausing music");
    musicPlayer.pausePlaying(true);
  } else {
    Serial.println("resuming music");
    musicPlayer.pausePlaying(false);
  }
}

int remotePauseOrResume(String noop) {
  pauseOrResume();
  return 1;
}

void stop() {
  musicPlayer.stopPlaying();
}

int remoteStop(String noop) {
  stop();
  return 1;
}
