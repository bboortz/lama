
#ifndef LORA_H
#define LORA_H

// Create SX1276 instance
SX1276 radio = new Module(LORA_CS, LORA_IRQ, LORA_RST, RADIOLIB_NC);

// RX TX intervals
unsigned long lastTxTime = 0;
unsigned long lastRxTime = 0; 
#endif


