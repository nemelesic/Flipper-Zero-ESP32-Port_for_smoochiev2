#include "furi_hal_speaker.h"
#include "furi_hal_resources.h"
#include "boards/board.h"

#include <driver/ledc.h>
#include <driver/gpio.h>
#include <esp_log.h>

#define SPEAKER_LEDC_TIMER      LEDC_TIMER_1
#define SPEAKER_LEDC_CHANNEL    LEDC_CHANNEL_1
#define SPEAKER_LEDC_SPEED      LEDC_LOW_SPEED_MODE

static const char* TAG = "FuriHalSpeaker";
static bool speaker_initialized = false;
static bool speaker_acquired = false;

void furi_hal_speaker_init(void) {
    if(speaker_initialized) return;

#if defined(BOARD_PIN_SPEAKER) && (BOARD_PIN_SPEAKER != UINT16_MAX)
    // Настраиваем аппаратный таймер ШИМ
    ledc_timer_config_t timer_conf = {
        .speed_mode = SPEAKER_LEDC_SPEED,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = SPEAKER_LEDC_TIMER,
        .freq_hz = 1000, 
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    // Привязываем ШИМ строго к GPIO 44 (Ваша линия RX)
    ledc_channel_config_t channel_conf = {
        .speed_mode = SPEAKER_LEDC_SPEED,
        .channel = SPEAKER_LEDC_CHANNEL,
        .timer_sel = SPEAKER_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BOARD_PIN_SPEAKER,
        .duty = 0, 
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

    speaker_initialized = true;
    ESP_LOGI(TAG, "PWM Speaker successfully bound to GPIO%d", BOARD_PIN_SPEAKER);
#endif
}

void furi_hal_speaker_deinit(void) {
    if(!speaker_initialized) return;
#if defined(BOARD_PIN_SPEAKER) && (BOARD_PIN_SPEAKER != UINT16_MAX)
    ledc_stop(SPEAKER_LEDC_SPEED, SPEAKER_LEDC_CHANNEL, 0);
    gpio_reset_pin(BOARD_PIN_SPEAKER);
    speaker_initialized = false;
#endif
}

void furi_hal_speaker_start(float frequency, float volume) {
    if(!speaker_initialized) furi_hal_speaker_init();
    if(frequency < 20.0f || frequency > 20000.0f) return;

#if defined(BOARD_PIN_SPEAKER) && (BOARD_PIN_SPEAKER != UINT16_MAX)
    // Меняем частоту ШИМ (задает высоту ноты для пищалки)
    ledc_set_freq(SPEAKER_LEDC_SPEED, SPEAKER_LEDC_TIMER, (uint32_t)frequency);
    
    // Идеальная громкость меандра для Smoochie моста — 50% заполнения (127 из 255)
    uint32_t duty = (volume > 0.0f) ? 127 : 0; 
    
    ledc_set_duty(SPEAKER_LEDC_SPEED, SPEAKER_LEDC_CHANNEL, duty);
    ledc_update_duty(SPEAKER_LEDC_SPEED, SPEAKER_LEDC_CHANNEL);
#endif
}

void furi_hal_speaker_stop(void) {
    if(!speaker_initialized) return;
#if defined(BOARD_PIN_SPEAKER) && (BOARD_PIN_SPEAKER != UINT16_MAX)
    // Сбрасываем ШИМ в ноль для идеальной тишины
    ledc_set_duty(SPEAKER_LEDC_SPEED, SPEAKER_LEDC_CHANNEL, 0);
    ledc_update_duty(SPEAKER_LEDC_SPEED, SPEAKER_LEDC_CHANNEL);
#endif
}

/* --- Системные функции-заглушки для устранения ошибок линковщика Furi --- */
bool furi_hal_speaker_acquire(uint32_t timeout) {
    (void)timeout;
    speaker_acquired = true;
    return true;
}

void furi_hal_speaker_release(void) {
    furi_hal_speaker_stop();
    speaker_acquired = false;
}

bool furi_hal_speaker_is_mine(void) {
    return true; 
}
