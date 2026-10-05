/*
  BhramaraNetra - INMP441 Audio Recorder
  ---------------------------------------

  INMP441:
    VDD  -> 3.3V
    GND  -> GND
    SCK  -> GPIO 26
    WS   -> GPIO 25
    SD   -> GPIO 33
    L/R  -> GND

  Output:
    16-bit signed PCM
    16 kHz
    Mono

  Serial:
    921600 baud
*/

#include <Arduino.h>
#include <driver/i2s.h>

#define I2S_PORT I2S_NUM_0

// -------------------------
// INMP441 pins
// -------------------------

#define I2S_BCLK 26
#define I2S_WS   25
#define I2S_SD   33


// -------------------------
// Audio configuration
// -------------------------

#define SAMPLE_RATE 16000
#define BLOCK_SIZE 512

#define BAUD_RATE 921600


// -------------------------
// Buffers
// -------------------------

int32_t i2sBuffer[BLOCK_SIZE];
int16_t pcmBuffer[BLOCK_SIZE];


// ============================================================
// Setup I2S
// ============================================================

void setupI2S() {

  i2s_config_t config = {

    .mode = (i2s_mode_t)(
      I2S_MODE_MASTER |
      I2S_MODE_RX
    ),

    .sample_rate = SAMPLE_RATE,

    .bits_per_sample =
      I2S_BITS_PER_SAMPLE_32BIT,

    .channel_format =
      I2S_CHANNEL_FMT_ONLY_LEFT,

    .communication_format =
      I2S_COMM_FORMAT_I2S,

    .intr_alloc_flags =
      ESP_INTR_FLAG_LEVEL1,

    .dma_buf_count = 8,

    .dma_buf_len = 64,

    .use_apll = false,

    .tx_desc_auto_clear = false,

    .fixed_mclk = 0
  };


  // Install driver

  esp_err_t result = i2s_driver_install(
    I2S_PORT,
    &config,
    0,
    NULL
  );

  if (result != ESP_OK) {

    Serial.println("I2S_DRIVER_ERROR");

    while (true) {
      delay(1000);
    }
  }


  // Pin configuration

  i2s_pin_config_t pins = {

    .bck_io_num = I2S_BCLK,

    .ws_io_num = I2S_WS,

    .data_out_num =
      I2S_PIN_NO_CHANGE,

    .data_in_num = I2S_SD
  };


  result = i2s_set_pin(
    I2S_PORT,
    &pins
  );

  if (result != ESP_OK) {

    Serial.println("I2S_PIN_ERROR");

    while (true) {
      delay(1000);
    }
  }


  // Clear old samples

  i2s_zero_dma_buffer(I2S_PORT);
}


// ============================================================
// Setup
// ============================================================

void setup() {

  Serial.begin(BAUD_RATE);

  delay(2000);

  setupI2S();

  // Give Python a text marker first
  Serial.println("AUDIO_READY");

  delay(1000);

  Serial.println("AUDIO_START");
}


// ============================================================
// Main loop
// ============================================================

void loop() {

  size_t bytesRead = 0;


  // Read 32-bit I2S samples

  esp_err_t result = i2s_read(
    I2S_PORT,
    i2sBuffer,
    sizeof(i2sBuffer),
    &bytesRead,
    portMAX_DELAY
  );


  if (result != ESP_OK) {
    return;
  }


  int samplesRead =
    bytesRead / sizeof(int32_t);


  // Convert to 16-bit PCM

  for (int i = 0; i < samplesRead; i++) {

    /*
      INMP441 gives 24-bit audio
      stored inside a 32-bit I2S word.

      Shift to get useful audio range.
    */

    int32_t sample =
      i2sBuffer[i] >> 14;


    // Clip

    if (sample > 32767)
      sample = 32767;

    if (sample < -32768)
      sample = -32768;


    pcmBuffer[i] =
      (int16_t)sample;
  }


  // Send RAW binary PCM

  Serial.write(
    (uint8_t *)pcmBuffer,
    samplesRead * sizeof(int16_t)
  );
}