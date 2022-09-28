#include <Arduino.h>
#include <Streaming.h>
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

void parse_serial();
void map_leds(struct StripMapping mapping);

// Definition of physical strand lengths
const int NUM_STRANDS = 4;
const int physical_sizes[NUM_STRANDS] = {200, 200, 200, 200};
CRGB* real_leds[NUM_STRANDS] = {};

void setup() {
    Serial.begin(115200);
    Serial.println("Booting");

    // Initialize LED arrays
    for (int i = 0; i < NUM_STRANDS; ++i) {
        real_leds[i] = static_cast<CRGB *>(malloc(physical_sizes[i] * sizeof(CRGB)));
    }

    FastLED.addLeds<LED_TYPE,DATA_PIN_1,COLOR_ORDER>(real_leds[0], NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE,DATA_PIN_2,COLOR_ORDER>(real_leds[1], NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE,DATA_PIN_3,COLOR_ORDER>(real_leds[2], NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE,DATA_PIN_4,COLOR_ORDER>(real_leds[3], NUM_LEDS).setCorrection(TypicalLEDStrip);

    /// Initialize network stuff here
}

// Variables related to parsing serial input
char input_buffer[30];
const char top[] = "\x1B\x5B\x41";
const char down[] = "\x1B\x5B\x42";
const char right[] = "\x1B\x5B\x43";
const char left[] = "\x1B\x5B\x44";
const char page_up[] = "\x1B\x5B\x35\x7E";
const char page_down[] = "\x1B\x5B\x36\x7E";

// Used for finding the LED mapping indexes
int mode = 0;
int selected = 0;
int positions[NUM_STRANDS] = {0};

void loop() {
    parse_serial();

    /** Receive/create the colors for the virtual strip here **/
    // In mode 0, show the selected LED positions for easy mapping
    if (mode == 0) {
        for (int i = 0; i < NUM_STRANDS; ++i) {
            // Fill everything with black, except the selected pixel positions
            fill_solid(real_leds[i], physical_sizes[i], CRGB::Black);
            // The selected strand gets extra saturation
            real_leds[i][positions[i]] = CHSV(i * 30, i == selected ? 255 : 100, 255);
        }
    }
    // In mode 1, do the mapping. No input here yet.
    if (mode == 1) {
        /** Map the virtual strips to the real strips **/
        for (auto mapping: mappings) {
            // Write mapping here
            map_leds(mapping);
        }
    }

    FastLED.show();
    FastLED.delay(1000/FRAMES_PER_SECOND);
}

/** Modulo that only returns positive results **/
inline int pos_mod(int i, int n) {
    return (i % n + n) % n;
}

/**
 * Detects serial commands and acts accordingly
 */
void parse_serial() {
    if (Serial.available()){
        Serial.read(input_buffer, sizeof input_buffer);

        if (strcmp(input_buffer, top) == 0) {
            // Serial << "Up detected!" << endl;
            positions[selected] = pos_mod(positions[selected]+1, physical_sizes[selected]);
        } else if (strcmp(input_buffer, down) == 0) {
            // Serial << "Down detected!" << endl;
            positions[selected] = pos_mod(positions[selected]-1, physical_sizes[selected]);
        } else if (strcmp(input_buffer, page_up) == 0) {
            // Serial << "Page Up detected!" << endl;
            positions[selected] = pos_mod(positions[selected]+10, physical_sizes[selected]);
        } else if (strcmp(input_buffer, page_down) == 0) {
            // Serial << "Page Down detected!" << endl;
            positions[selected] = pos_mod(positions[selected]-10, physical_sizes[selected]);
        } else if (strcmp(input_buffer, right) == 0) {
            // Serial << "Right detected!" << endl;
            selected = pos_mod(selected+1, NUM_STRANDS);
        } else if (strcmp(input_buffer, left) == 0) {
            // Serial << "Left detected!" << endl;
            selected = pos_mod(selected-1, NUM_STRANDS);
        } else if (strcmp(input_buffer, "m") == 0) {
            mode = pos_mod(mode+1, 2);
            Serial << "Mode is now " << mode << endl;
        } else {
            Serial << "Input detected: ";
            for (int i = 0; i < strlen(input_buffer); ++i) {
                Serial << _HEX(input_buffer[i]) << " '" << input_buffer[i] << "' ";
            }
            for (int i = 0; i < strlen(input_buffer); ++i) {
                Serial.print(input_buffer[i], HEX);
            }
            Serial << endl;
        }
        memset(input_buffer, 0, sizeof input_buffer);

        Serial << "Selected: " << selected << ", positions: ";
        for (auto pos : positions) {
            Serial << pos << " ";
        }
        Serial << endl;
    }
}

/**
 * Map a piece of virtual LED strip to a physical one
 * @param mapping The mapping information
 */
void map_leds(struct StripMapping mapping) {
    const int virtual_length = mapping.Virtual.end-mapping.Virtual.begin;
    const int physical_length = mapping.Physical.end-mapping.Physical.begin;

    for (auto i = mapping.Physical.begin; i < mapping.Physical.end; i++) {
        // Calculate which LED we start interpolating at
        int start = i * virtual_length / physical_length;
        // Calculate the ratio for interpolating
        // Not sure if this should be 255 or 256. Normally it would be 255 but this is rounded math, and the blend function uses 256 too.
        uint8_t remainder = (256 * i * virtual_length / physical_length) - 256 * start;

        // We must make sure there is no out-of-bounds here...
        int end = start >= virtual_length ? start : start + 1;
        auto c1 = virtual_leds[start];
        auto c2 = virtual_leds[end];

        // Do the interpolation
        virtual_leds[mapping.strip][i] = blend(c1, c2, remainder);
    }
}
