#ifndef LORA_PACKET_H
#define LORA_PACKET_H

struct ParsedLoraPacket {
  String user;
  int    seq;
  int    msecs;
  float  rsnr;
};


#endif