#include <lama_packet.h>
#include <stdio.h>

#include "arduino/arduino_bridge.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PROG_NAME "LAMA"
#define PROG_VER "0.1.0"

#include "nvs_flash.h"

void initNVS() {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // Erase and reinitialize if the partition was truncated or version mismatch
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}

extern "C" void app_main(void) {
  vTaskDelay(pdMS_TO_TICKS(1000));

  printf("hello world!!\n");
  bool works = lama_packet_callable();
  printf("Rust function returns: %d\n\n", works);
  vTaskDelay(pdMS_TO_TICKS(1000));

  initNVS();
  arduino_setup();

  xTaskCreate(
      [](void* param) {
        while (1) {
          arduino_loop();
        }
      },
      "ArduinoLoopTask",
      4096,
      NULL,
      1,
      NULL);
}
