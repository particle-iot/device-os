#include "application.h"
#include "audio_hal.h"
#include "i2s_hal.h"

SYSTEM_MODE(MANUAL);

static volatile bool buttonClicked = false;
static void onButtonClick(system_event_t ev, int button_data) {
    buttonClicked = true;
}

void demo_dmic_i2s_recorder() {
    RGB.control(true);
    RGB.color(0, 255, 0);
    hal_audio_init(HAL_AUDIO_OUT_DEVICE_NONE, HAL_AUDIO_MODE_STEREO, HAL_AUDIO_SAMPLE_RATE_48K, HAL_AUDIO_WORD_LEN_16);
    hal_i2s_init();
    System.on(button_click, onButtonClick);

    size_t voiceDataSize = 48 * 1024 * 10;
    void* voiceData = nullptr;
    voiceData = malloc(voiceDataSize);

    while (1) {
        if (buttonClicked) {
            buttonClicked = false;

            RGB.color(255, 0, 0); // red
            delay(1000);
            hal_audio_read_dmic(voiceData, voiceDataSize);
            RGB.color(0, 0, 255); // blue
            delay(1000);
            uint8_t *data = (uint8_t *)voiceData;
            for (size_t i = 0; i < voiceDataSize / 512; i++) {
                while (!hal_i2s_ready()) {
                    ;
                }
                hal_i2s_play(&data[i * 512], 512);
            }
            RGB.color(0, 255, 0); // green
        }
    }
}

void setup() {
    demo_dmic_i2s_recorder();
}

void loop() {
}
