void setupWifi() {
  if (!config.wifiEnabled || strlen(config.wifiSsid) == 0) {
    Serial.println("WiFi disabled or not configured");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(config.wifiHostname);
  WiFi.begin(config.wifiSsid, config.wifiPassword);

  // Non-blocking connection with timeout
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiAvailable = true;
    setWifiNetworkState(CONNECTED);
    Serial.printf("WiFi connected: %s\n", WiFi.localIP().toString().c_str());

    setupWebServer();
  } else {
    wifiAvailable = false;
    Serial.println("WiFi connection failed");
  }
}

void setupWebServer() {
  server = new WebServer(80);
  if (!server) {
    Serial.println("Failed to create web server");
    return;
  }

  // REST API endpoints - BLOCKING but simpler
  server->on("/api/status", HTTP_GET, []() {
    int           totalExpected = loraRxPacketCount + loraRxPacketLost;
    int           lossPercent = (totalExpected > 0) ? (100 * loraRxPacketLost / totalExpected) : 0;
    unsigned long rxAgo       = (lastRxTime > 0) ? (millis() - lastRxTime) / 1000 : 999;
    unsigned long txAgo       = (lastTxTime > 0) ? (millis() - lastTxTime) / 1000 : 999;

    String json = "{";
    json += "\"nodeId\":" + String(config.nodeId) + ",";
    json += "\"networkId\":" + String(config.networkId) + ",";
    json += "\"user\":\"" + String(config.user) + "\",";
    json += "\"capabilities\":\"0x" + String(config.capabilities, HEX) + "\",";
    json += "\"state\":\"" + getSystemState() + "\",";
    json += "\"loraState\":\"" + getLoraNetworkState() + "\",";
    json += "\"wifiState\":\"" + getWifiNetworkState() + "\",";
    json += "\"rxCount\":" + String(loraRxPacketCount) + ",";
    json += "\"txCount\":" + String(loraTxPacketCount) + ",";
    json += "\"lostCount\":" + String(loraRxPacketLost) + ",";
    json += "\"lossPercent\":" + String(lossPercent) + ",";
    json += "\"lastRxAgo\":" + String(rxAgo) + ",";
    json += "\"lastTxAgo\":" + String(txAgo) + ",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"minFreeHeap\":" + String(ESP.getMinFreeHeap()) + ",";
    json += "\"wifiRssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"loraFreq\":" + String(config.loraFrequency, 1) + ",";
    json += "\"loraBw\":" + String(config.loraBw, 1) + ",";
    json += "\"loraSf\":" + String(config.loraSf) + ",";
    json += "\"loraPower\":" + String(config.loraTxPower);
    json += "}";
    server->send(200, "application/json", json);
  });

  server->on("/api/tx", HTTP_POST, []() {
    if (systemState == IDLE) {
      setSystemState(DO_TX);
      server->send(200, "application/json", "{\"status\":\"transmitting\"}");
    } else {
      server->send(409, "application/json", "{\"error\":\"busy\"}");
    }
  });

  // Known nodes
  server->on("/api/nodes", HTTP_GET, []() {
    String json = "[";
    for (int i = 0; i < nodeCount; i++) {
      if (i > 0)
        json += ",";
      NodeInfo*     n           = &knownNodes[i];
      int           total       = n->rxCount + n->lostCount;
      int           lossPct     = (total > 0) ? (100 * n->lostCount / total) : 0;
      unsigned long lastSeenAgo = (millis() - n->lastSeen) / 1000;

      json += "{";
      json += "\"nodeId\":" + String(n->nodeId) + ",";
      json += "\"name\":\"" + String(n->name) + "\",";
      json += "\"capabilities\":\"0x" + String(n->capabilities, HEX) + "\",";
      json += "\"rxCount\":" + String(n->rxCount) + ",";
      json += "\"lostCount\":" + String(n->lostCount) + ",";
      json += "\"lossPercent\":" + String(lossPct) + ",";
      json += "\"lastSeq\":" + String(n->lastSeq) + ",";
      json += "\"lastRssi\":" + String(n->lastRssi, 1) + ",";
      json += "\"lastSnr\":" + String(n->lastSnr, 1) + ",";
      json += "\"lastSeenAgo\":" + String(lastSeenAgo);
      json += "}";
    }
    json += "]";
    server->send(200, "application/json", json);
  });

  // RX History
  server->on("/api/history", HTTP_GET, []() {
    String json = "[";
    for (int i = 0; i < loraRxHistoryCount; i++) {
      if (i > 0)
        json += ",";
      LoraRxStatsMessage* m = &loraRxHistory[i];

      json += "{";
      json += "\"nodeId\":\"" + String(m->nodeId) + "\",";
      json += "\"seq\":" + String(m->seq) + ",";
      json += "\"msecs\":" + String(m->msecs) + ",";
      json += "\"snr\":" + String(m->snr, 1) + ",";
      json += "\"rssi\":" + String(m->rssi, 1) + ",";
      json += "\"freqErr\":" + String(m->freqErr, 1) + ",";
      json += "\"rsnr\":" + String(m->rsnr, 1);
      json += "}";
    }
    json += "]";
    server->send(200, "application/json", json);
  });

  server->on("/", HTTP_GET, []() {
    server->send_P(200, "text/html", R"=====(
<!DOCTYPE html>
<html><head><title>LAMA Monitor</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{font-family:Arial;margin:0;padding:20px;background:#f5f5f5}
h1{color:#333}
.card{background:white;padding:20px;margin:15px 0;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}
.card h2{margin-top:0;color:#444;border-bottom:2px solid #007bff;padding-bottom:10px}
.stats{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px}
.stat{padding:10px;background:#f8f9fa;border-radius:5px;border-left:3px solid #007bff}
.stat-label{font-size:12px;color:#666;text-transform:uppercase}
.stat-value{font-size:24px;font-weight:bold;color:#333;margin-top:5px}
.stat-unit{font-size:14px;color:#666;margin-left:5px}
table{width:100%;border-collapse:collapse}
th,td{padding:10px;text-align:left;border-bottom:1px solid #ddd}
th{background:#f8f9fa;font-weight:bold;color:#444}
tr:hover{background:#f8f9fa}
button{padding:12px 24px;margin:5px;cursor:pointer;background:#007bff;color:white;border:none;border-radius:5px;font-size:14px}
button:hover{background:#0056b3}
.status-good{color:#28a745}
.status-warning{color:#ffc107}
.status-error{color:#dc3545}
</style>
</head><body>
<div class="container">
<h1>LAMA Monitor</h1>
<div class="card"><h2>System Status</h2><div class="stats" id="systemStats"></div></div>
<div class="card"><h2>LoRa Configuration</h2><div class="stats" id="loraConfig"></div></div>
<div class="card"><h2>Network Statistics</h2><div class="stats" id="networkStats"></div></div>
<div class="card"><h2>Controls</h2>
<button onclick="transmit()">Transmit Now</button>
<button onclick="loadData()">Refresh</button>
<button onclick="location.href='/api/status'">JSON</button>
</div>
<div class="card"><h2>Nodes (<span id="nodeCount">0</span>)</h2>
<table><thead><tr><th>ID</th><th>Name</th><th>RX</th><th>Lost</th><th>Loss%</th><th>RSSI</th><th>SNR</th><th>Seen</th></tr></thead>
<tbody id="nodesBody"></tbody></table></div>
<div class="card"><h2>RX History (<span id="historyCount">0</span>)</h2>
<table><thead><tr><th>Node</th><th>Seq</th><th>RSSI</th><th>SNR</th><th>FreqErr</th><th>rSNR</th><th>ms</th></tr></thead>
<tbody id="historyBody"></tbody></table></div>
</div>
<script>
const loadData=()=>{
fetch('/api/status').then(r=>r.json()).then(s=>{
fetch('/api/nodes').then(r=>r.json()).then(n=>{
fetch('/api/history').then(r=>r.json()).then(h=>{
document.getElementById('systemStats').innerHTML=
'<div class="stat"><div class="stat-label">Node</div><div class="stat-value">'+s.nodeId+'</div></div>'+
'<div class="stat"><div class="stat-label">State</div><div class="stat-value">'+s.state+'</div></div>'+
'<div class="stat"><div class="stat-label">Uptime</div><div class="stat-value">'+Math.floor(s.uptime/3600)+'<span class="stat-unit">h</span></div></div>'+
'<div class="stat"><div class="stat-label">Heap</div><div class="stat-value">'+Math.floor(s.freeHeap/1024)+'<span class="stat-unit">KB</span></div></div>'+
'<div class="stat"><div class="stat-label">WiFi</div><div class="stat-value">'+s.wifiRssi+'<span class="stat-unit">dBm</span></div></div>';
document.getElementById('loraConfig').innerHTML=
'<div class="stat"><div class="stat-label">Freq</div><div class="stat-value">'+s.loraFreq+'<span class="stat-unit">MHz</span></div></div>'+
'<div class="stat"><div class="stat-label">BW</div><div class="stat-value">'+s.loraBw+'<span class="stat-unit">kHz</span></div></div>'+
'<div class="stat"><div class="stat-label">SF</div><div class="stat-value">'+s.loraSf+'</div></div>'+
'<div class="stat"><div class="stat-label">Power</div><div class="stat-value">'+s.loraPower+'<span class="stat-unit">dBm</span></div></div>';
var lc=s.lossPercent>20?'status-error':(s.lossPercent>5?'status-warning':'status-good');
document.getElementById('networkStats').innerHTML=
'<div class="stat"><div class="stat-label">RX</div><div class="stat-value status-good">'+s.rxCount+'</div></div>'+
'<div class="stat"><div class="stat-label">TX</div><div class="stat-value status-good">'+s.txCount+'</div></div>'+
'<div class="stat"><div class="stat-label">Lost</div><div class="stat-value '+lc+'">'+s.lostCount+'</div></div>'+
'<div class="stat"><div class="stat-label">Loss</div><div class="stat-value '+lc+'">'+s.lossPercent+'<span class="stat-unit">%</span></div></div>'+
'<div class="stat"><div class="stat-label">Last RX</div><div class="stat-value">'+s.lastRxAgo+'<span class="stat-unit">s</span></div></div>'+
'<div class="stat"><div class="stat-label">Last TX</div><div class="stat-value">'+s.lastTxAgo+'<span class="stat-unit">s</span></div></div>';
document.getElementById('nodeCount').textContent=n.length;
var nh='';for(var i=0;i<n.length;i++){var nl=n[i].lossPercent>20?'status-error':(n[i].lossPercent>5?'status-warning':'status-good');
nh+='<tr><td>'+n[i].nodeId+'</td><td>'+n[i].name+'</td><td>'+n[i].rxCount+'</td><td>'+n[i].lostCount+'</td><td class="'+nl+'">'+n[i].lossPercent+'%</td><td>'+n[i].lastRssi.toFixed(1)+'</td><td>'+n[i].lastSnr.toFixed(1)+'</td><td>'+n[i].lastSeenAgo+'s</td></tr>';}
document.getElementById('nodesBody').innerHTML=nh;
document.getElementById('historyCount').textContent=h.length;
var hh='';for(var i=0;i<h.length;i++){
hh+='<tr><td>'+h[i].nodeId+'</td><td>'+h[i].seq+'</td><td>'+h[i].rssi.toFixed(1)+'</td><td>'+h[i].snr.toFixed(1)+'</td><td>'+h[i].freqErr.toFixed(1)+'</td><td>'+h[i].rsnr.toFixed(1)+'</td><td>'+h[i].msecs+'</td></tr>';}
document.getElementById('historyBody').innerHTML=hh;
});});});};
const transmit=()=>{fetch('/api/tx',{method:'POST'}).then(()=>{alert('Transmitting...');setTimeout(loadData,1000);});};
loadData();setInterval(loadData,3000);
</script>
</body></html>
)=====");
  });

  server->begin();
  Serial.println("Web server started on port 80");
}

void handleWifi() {
  if (wifiAvailable && server) {
    server->handleClient();  // Call this in main loop
  }
}

String getWifiStatus() {
  switch (WiFi.status()) {
    case WL_IDLE_STATUS:
      return "Idle";

    case WL_CONNECTED:
      return "Connected";

    case WL_DISCONNECTED:
      return "Disconnected";

    // WL_STOPPED not available in this ESP32 Arduino core version
    // case WL_STOPPED:
    //   return "Stopped";
  }

  return "ERROR";
}