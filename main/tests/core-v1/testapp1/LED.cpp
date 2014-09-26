
#include "application.h"
#include "unit-test/unit-test.h"

void assertLEDColor(uint8_t r, uint8_t g, uint8_t b, bool equal) {
    uint8_t rgb[3];
    LED_RGB_Get(rgb);
        
    if (equal) {
        assertEqual(rgb[0], r);
        assertEqual(rgb[1], g);
        assertEqual(rgb[2], b);
    }
    else if(rgb[0]==r && rgb[1]==g && rgb[2]==b) {
        char buf[50];
        sprintf(buf, "%d,%d,%d",r,g,b);
        assertNotEqual(buf, buf);
    }
}

void assertLEDColorIs(uint8_t r, uint8_t g, uint8_t b) {
    assertLEDColor(r, g, b, true);
}

void assertLEDColorIsNot(byte r, byte g, byte b) {
    assertLEDColor(r, g, b, false);
}

void assertLEDColorIs(int rgb) {
    assertLEDColor(rgb>>16 & 0xFF, rgb>>8 & 0xFF, rgb&0xFF, true);
}

void assertLEDColorIsNot(int rgb) {
    assertLEDColor(rgb>>16 & 0xFF, rgb>>8 & 0xFF, rgb&0xFF, false);
}

/**
 */
uint8_t ledAdjust(uint8_t value, uint8_t brightness=255) {
    return (value*brightness)>>8;
}

test(LED_ControlledReturnsFalseWhenNoControl) {
    // when
    RGB.control(false);
    // then
    assertFalse(RGB.controlled());
}

test(LED_ControlledReturnsTrueWhenControlled) {
    // when
    RGB.control(true);
    // then
    assertTrue(RGB.controlled());
}

test(LED_ChangesWhenNotControlled) {
    // when
    RGB.control(false);
    // then
    uint8_t rgbInitial[3];
    uint8_t rgbChanged[3];
    LED_RGB_Get(rgbInitial);    
    delay(75);    
    LED_RGB_Get(rgbChanged);
    
    assertFalse(rgbInitial[0]==rgbChanged[0] && rgbInitial[1]==rgbChanged[1] && rgbInitial[2]==rgbChanged[2]);    
}

test(LED_StaticWhenControlled) {
    // given
    RGB.control(true);
    RGB.brightness(255);
    
    // when
    RGB.color(30,60,90);
    
    // then
    uint8_t rgbExpected[3] = {ledAdjust(30), ledAdjust(60), ledAdjust(90)};
    uint8_t rgbInitial[3];
    uint8_t rgbChanged[3];
    LED_RGB_Get(rgbInitial);    
    delay(75);    
    LED_RGB_Get(rgbChanged);
    for (int i=0; i<3; i++)
        assertEqual(rgbInitial[i], rgbExpected[i]);    
    
    for (int i=0; i<3; i++)
        assertEqual(rgbInitial[i], rgbChanged[i]);    
}

test(LED_SettingRGBAfterOverrideShouldChangeLED) {
    // given
    RGB.control(true);
    RGB.brightness(255);
    
    // when
    RGB.color(10,20,30);
    
    // then
    assertLEDColorIs(ledAdjust(10),ledAdjust(20),ledAdjust(30));
}

test(LED_SettingRGBWithoutOverrideShouldNotChangeLED) {
    // given
    RGB.control(false);
    
    // when
    RGB.color(10,20,30);
    
    // then
    assertLEDColorIsNot(ledAdjust(10),ledAdjust(20),ledAdjust(30));    
}

test(LED_BrightnessChangesColor) {
    // given
    RGB.control(true);
    RGB.brightness(255);
    RGB.color(255,127,0);
    
    // when
    RGB.brightness(128);
    
    // then
    assertLEDColorIs(ledAdjust(255,128), ledAdjust(127,128), ledAdjust(0,128));
}

test(LED_BrightnessIsPersisted) {
    // given
    RGB.control(true);
    RGB.brightness(128);
    RGB.color(255,255,255);
    
    // when
    RGB.control(false);
    RGB.control(true);
    RGB.color(255,255,0);
    
    assertLEDColorIs(ledAdjust(255,128),ledAdjust(255,128),0);
}
