#include <Arduino.h>

constexpr gpio_num_t IRQ_PIN = GPIO_NUM_0;

volatile bool irqFlag = false;

/**
 * ISR — MUST be IRAM safe
 */
void IRAM_ATTR onGpioInterrupt()
{
    irqFlag = true;   // ONLY set a flag
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("ESP32 ISR minimal test");

    pinMode(IRQ_PIN, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(IRQ_PIN),
        onGpioInterrupt,
        FALLING
    );

    Serial.println("Interrupt attached");
}

void loop()
{
    if (irqFlag) {
        irqFlag = false;
        Serial.println("IRQ received");
    }
    Serial.println("lala");

    delay(10);
}
