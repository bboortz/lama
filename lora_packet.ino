
void printLoraPacket(LoraPacket pkt, size_t pktLength) {
  LoraPacketBeatPayload p = {};
  memcpy(&p, pkt.payload, sizeof(LoraPacketBeatPayload));

  Serial.printf(
      "pktLength: %u - "
      "ver: %u type: %u seq: %u node: %u payloadLen: %u msecs: %u rsnr: %.2f\n",
      (unsigned)pktLength,
      pkt.header.version,
      pkt.header.type,
      pkt.header.seq,
      pkt.header.src.node,
      pkt.header.payloadLen,
      p.msecs,
      p.rsnr);
}

bool validateLoraPacket(const LoraPacket& pkt, size_t pktLength) {
  // Check magic number FIRST - don't trust other fields if magic is wrong
  if (pkt.header.magic != LORA_PACKET_MAGIC) {
    return false;
  }

  // Check version
  if (pkt.header.version != LORA_PACKET_VERSION) {
    return false;
  }

  // Minimum size check
  if (pktLength < LORA_HEADER_SIZE) {
    return false;
  }

  // Maximum size check
  if (pktLength > LORA_PACKET_MAXLENGTH) {
    return false;
  }

  // Payload size sanity
  if (pkt.header.payloadLen > LORA_MAX_PAYLOAD) {
    return false;
  }

  // Validate actual packet size matches header + declared payload
  size_t expectedLen = LORA_HEADER_SIZE + pkt.header.payloadLen;
  if (pktLength != expectedLen) {
    Serial.printf("Packet length mismatch: expected %u, got %u\n",
                  (unsigned)expectedLen,
                  (unsigned)pktLength);
    return false;
  }

  // Type-specific payload validation
  if (pkt.header.type == PKT_BEAT && pkt.header.payloadLen != LORA_BEAT_PAYLOAD_SIZE) {
    return false;
  }

  return true;
}

void handleLoraBeat(LoraPacket pkt, size_t pktLength) {
  float freqErr = radio->getFrequencyError();
  float rssi    = radio->getRSSI();
  float snr     = radio->getSNR();

  // LoraPacketBeatPayload* p = (LoraPacketBeatPayload*)pkt.payload;
  LoraPacketBeatPayload p = {};
  memcpy(&p, pkt.payload, sizeof(LoraPacketBeatPayload));

  // printLoraPacket(pkt, pktLength);

  setLoraNetworkState(CONNECTED);

  // Add to Lora RX Statistics
  addToLoraRxStats(pkt.header.src.node, pkt.header.seq, p.msecs, snr, rssi, freqErr, p.rsnr);

  // Update node tracking
  updateNodeStats(pkt.header.src.node, rssi, snr, freqErr, pkt.header.seq);
}

LoraPacket parseLoraPacket(const uint8_t* data, size_t pktLength) {
  LoraPacket pkt = createEmptyPacket();

  // Minimum size check
  if (pktLength < sizeof(LoraPacketHeader)) {
    addError("packet too small");
    return pkt;
  }

  // Copy header only
  memcpy(&pkt.header, data, sizeof(LoraPacketHeader));

  // Validate header & length
  if (!validateLoraPacket(pkt, pktLength)) {
    addError("wrong packet parsed");
    return pkt;
  }

  // Copy payload safely
  memcpy(pkt.payload, data + sizeof(LoraPacketHeader), pkt.header.payloadLen);

  return pkt;
}

void decodeLoraPacket(void) {
  // String data;
  uint8_t data[64];
  rxState = radio->readData(data, 64);
  if (handleRadiolibState(rxState)) {
    return;
  }

  size_t pktLength = radio->getPacketLength();
  if (pktLength > 0) {
    // Parse packet - much cleaner!
    LoraPacket packet = parseLoraPacket(data, pktLength);
    // printLoraPacket(packet, pktLength);

    loraRxPacketCount++;
    lastRxTime = millis();
    handleLoraBeat(packet, pktLength);
  }
}

int transmitLoraPacket(LoraPacket* pkt) {
  size_t len = sizeof(LoraPacketHeader) + pkt->header.payloadLen;

  // Validate packet
  if (!validateLoraPacket(*pkt, len)) {  // <-- dereference pointer
    addError("wrong TX packet");
    return 1;
  }

  txState = radio->transmit((uint8_t*)pkt, len);
  if (handleRadiolibState(txState)) {
    return txState;
  }

  // printLoraPacket(*pkt, len);

  LoraPacketBeatPayload p = {};
  memcpy(&p, pkt->payload, sizeof(LoraPacketBeatPayload));
  txMessage = padRight("S:" + String(loraTxPacketCount), 5) + " "
              + padRight("M:" + String(p.msecs), 5);

  return txState;
}

LoraPacket createLoraPacket(LoraPacketType        type,
                            uint8_t               nodeId,
                            uint16_t              seq,
                            LoraPacketBeatPayload payload) {
  LoraPacketNode   src = {nodeId};
  LoraPacketHeader header =
      {LORA_PACKET_MAGIC, LORA_PACKET_VERSION, type, seq, src, sizeof(LoraPacketBeatPayload)};
  LoraPacket pkt = {header};

  memcpy(pkt.payload, &payload, sizeof(payload));

  size_t actualSize = sizeof(LoraPacketHeader) + pkt.header.payloadLen;
  if (!validateLoraPacket(pkt, actualSize)) {
    addError("wrong packet creation");
  }

  return pkt;
}

LoraPacket createLoraBeatPacket(uint8_t nodeId, uint16_t seq, uint16_t msecs, float rsnr) {
  LoraPacketBeatPayload payload = {msecs, rsnr};
  LoraPacket            pkt     = createLoraPacket(PKT_BEAT, nodeId, seq, payload);
  return pkt;
}

LoraPacket createEmptyPacket() {
  uint8_t          seq        = 0;
  size_t           payloadLen = 0;
  LoraPacketType   type       = {PKT_EMPTY};
  LoraPacketNode   src        = {0};
  LoraPacketHeader header = {LORA_PACKET_MAGIC, LORA_PACKET_VERSION, type, seq, src, payloadLen};
  LoraPacket       pkt    = {header};

  size_t actualSize = sizeof(LoraPacketHeader) + pkt.header.payloadLen;
  if (!validateLoraPacket(pkt, actualSize)) {
    addError("wrong empty packet");
  }

  return pkt;
}

int sendLoraBeat() {
  int        m   = millis() % 100;
  LoraPacket pkt = createLoraBeatPacket(config.nodeId, loraTxPacketCount, m, getLastSnr());

  txState = transmitLoraPacket(&pkt);
  return txState;
}
