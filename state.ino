String getState() {
  String loraState = "ERR";
  if (radiolibState == 0 && rxState == 0 && txState == 0) {
    loraState = "OK";
  }
  unsigned long rxAgo = (lastRxTime > 0) ? (millis() - lastRxTime) / 1000 : 999;

  return padRight(getSystemState(), 5) + " " + 
         padRight(loraState, 5) + " " + 
         padRight(getNetworkState(), 4) + " " +
         padRight(String(rxAgo), 3); 
}

void setSystemState(system_state_enum stateobj) {
  systemState = stateobj;
}

String getSystemState() {

  switch ( systemState ) {
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
      
    case FAILED:
      return "FAILED";
  }

  return "FAILED";
  
}

void setNetworkState(network_state_enum stateobj) {
  networkState = stateobj;
}

String getNetworkState() {

  switch ( networkState ) {
    case LOST: 
      return "LOST";
    
    case CONNECTED:
      return "CONN";
  }

  return "FAILED";
  
}

int doRadiolibState(int state) {
  if (state != RADIOLIB_ERR_NONE) {
    setSystemState(FAILED);
    return 1;
  }

  return 0;
}

void setStatus(system_state_enum newState, String newFunc) {
 setSystemState(newState);
 currentMethod = newFunc;
 updateStatus();
}
