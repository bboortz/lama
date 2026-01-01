
#ifndef LORA_H
#define LORA_H

#include <RadioLib.h>

// Create SX1276 instance
// SX1276 radio = new Module(config.loraPinCs, config.loraPinIrq, config.loraPinRst, RADIOLIB_NC);
// SX1276 radio = new Module(DEFAULT_LORA_PIN_CS, DEFAULT_LORA_PIN_IRQ, DEFAULT_LORA_PIN_RST,
// RADIOLIB_NC);
Module* loraModule = nullptr;
SX1276* radio      = nullptr;

// RX TX intervals
uint32_t lastTxTime = 0;
uint32_t lastRxTime = 0;

#endif
