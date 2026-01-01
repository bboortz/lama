String getState() {
  String loraState = "ERR";
  if (radiolibState == 0 && rxState == 0 && txState == 0) {
    loraState = "OK";
  }

  return padRight(getSystemState(), 5) + " " + padRight(loraState, 5) + " "
         + padRight("L:" + getLoraNetworkState(), 3) + " "
         + padRight("W:" + getWifiNetworkState(), 3);
}

void setSystemState(SystemStateEnum stateobj) {
  systemState = stateobj;
}

String getSystemState() {
  switch (systemState) {
    case INIT:
      return "INIT";

    case READY:
      return "READY";

    case DO_RX:
      return "DO_RX";

    case DO_TX:
      return "DO_TX";

    case IN_RX:
      return "IN_RX";

    case IN_TX:
      return "IN_TX";

    case IDLE:
      return "IDLE";

    case ERROR:
      return "ERROR";

    case FATAL:
      return "FATAL";
  }

  return "FATAL";
}

void setLoraNetworkState(NetworkStateEnum stateobj) {
  loraNetworkState = stateobj;
}

String getLoraNetworkState() {
  switch (loraNetworkState) {
    case LOST:
      return "L";

    case CONNECTED:
      return "C";
  }

  return "FAILED";
}

void setWifiNetworkState(NetworkStateEnum stateobj) {
  wifiNetworkState = stateobj;
}

String getWifiNetworkState() {
  switch (wifiNetworkState) {
    case LOST:
      return "L";

    case CONNECTED:
      return "C";
  }

  return "FAILED";
}

void setStatusInternal(SystemStateEnum newState, const char* newMethod) {
  setSystemState(newState);
  currentMethod = (String)newMethod;
  displayStatus();
}
