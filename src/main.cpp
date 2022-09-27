#include <Arduino.h>
// Not actually using SPI, just here to shut the message up
#define SPI_DATA 23
#define SPI_CLOCK 18
#include <FastLED.h>

#define DATA_PIN_1 15
#define DATA_PIN_2 2
#define DATA_PIN_3 0
#define DATA_PIN_4 4
#define LED_TYPE WS2811
#define COLOR_ORDER RGB
#define NUM_LEDS    64

#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  120

struct Interval {
    int begin;
    int end;
};

struct StripMapping {
    int strip;
    struct Interval Virtual;
    struct Interval Physical;
};

// For the virtual strips:
// Big part of the F: 3180 mm, small part (the lobe): 1800 mm
// The strip is 60 LEDs/m or 16.67 mm per LED
const int virt_num_leds[] = {3180 * 60 / 1000, 1800 * 60 / 1000, 3180 * 60 / 1000, 1800 * 60 / 1000};
const int virt_total_leds = virt_num_leds[0] + virt_num_leds[1] + virt_num_leds[2] + virt_num_leds[3];

CRGB virtual_leds[virt_total_leds];

struct StripMapping mappings[] = {
        // part 1
        {0, {30, 150}, {70, 190}}

};

CRGB LED1[200];
CRGB LED2[200];
CRGB LED3[200];
CRGB LED4[200];
CRGB* real_leds[] = {
        reinterpret_cast<CRGB *>(&LED1),
        reinterpret_cast<CRGB *>(&LED2),
        reinterpret_cast<CRGB *>(&LED3),
        reinterpret_cast<CRGB *>(&LED4),
};

void setup() {
    FastLED.addLeds<LED_TYPE,DATA_PIN_1,COLOR_ORDER>(real_leds[0], NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE,DATA_PIN_2,COLOR_ORDER>(real_leds[1], NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE,DATA_PIN_3,COLOR_ORDER>(real_leds[2], NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE,DATA_PIN_4,COLOR_ORDER>(real_leds[3], NUM_LEDS).setCorrection(TypicalLEDStrip);

    /// Initialize network stuff here
}

void map_leds(struct StripMapping mapping) {
    for (auto i = mapping.Physical.begin; i < mapping.Physical.end; i++) {
        // Do the interpolation
    }

    // The fill_gradient function does something related to what we need, and very efficient with fixed point numbers
    // Can be used for inspiration, but let's go for an easy solution first.
    CHSV c1 = CHSV(0, 200, 255);
    CHSV c2 = CHSV(50, 200, 255);
    fill_gradient(real_leds[0], mapping.Physical.begin, c1, mapping.Physical.end, c2);
}

void loop() {
    /** Receive/create the colors for the virtual strip here **/

    /** Map the virtual strips to the real strips **/
    for (auto mapping: mappings) {
        // Write mapping here
        map_leds(mapping);
    }
}