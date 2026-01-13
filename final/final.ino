
// This is your code where calibration is not working fine/
#include <Wire.h>
#include <MPU6050.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>

const char* ssid = "FiOS-YN4HE";
const char* password = "roof376open5348net";

MPU6050 mpu;
AsyncWebServer server(80);

// MongoDB Configuration (Python bridge server)
const char* mongoServer = "http://192.168.1.187:5000"; // Change to your PC's IP
const char* mongoEndpoint = "/store";

bool sessionActive = false;
bool calibrated = false;

unsigned long startTime = 0;
unsigned long lastPushTime = 0;
float durations = 0;
int pushupCount = 0;

float minZ = 10, maxZ = -10;
float downThreshold, upThreshold;
bool goingDown = false;
float totalCalories = 0;

const float userWeight = 70.0; // kg

// Session data structure
struct SessionData {
  int pushupCount;
  float totalCalories;
  float duration;
  unsigned long timestamp;
  int maxPushups;
  float avgPushTime;
};

SessionData currentSession;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Push-up Counter Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { 
      font-family: 'Segoe UI', Arial, sans-serif; 
      text-align: center; 
      margin: 0;
      padding: 20px;
      background: white;
      color: black;
      min-height: 100vh;
    }
    .container {
      max-width: 1200px;
      margin: 0 auto;
      padding: 20px;
    }
    .header {
      text-align: left;
      margin-bottom: 30px;
    }
    .header h1 {
      font-size: 36px;
      margin-bottom: 5px;
    }
    .header p {
      font-size: 18px;
      opacity: 0.9;
    }
    .stats-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
      gap: 20px;
      margin-bottom: 40px;
    }
    .stat-card {
      background: rgba(255, 255, 255, 0.1);
      backdrop-filter: blur(10px);
      border-radius: 15px;
      padding: 25px;
      /* box-shadow: 0 8px 32px #A5D9FE; */
      border: 3px solid black;
    }
    .stat-card:nth-child(1), .stat-card:nth-child(2){
      color: #ff4757;
    }
      .stat-card:nth-child(3), .stat-card:nth-child(4){
      color: #2F9E44;
    }
    .stat-card h3 {
      margin-top: 0;
      font-size: 16px;
      opacity: 0.8;
    }
    .stat-value {
      font-size: 48px;
      font-weight: bold;
      margin: 10px 0;
    }
    .controls {
      margin: 30px 0;
    }
    button {
      padding: 12px 25px;
      margin: 10px;
      font-size: 18px;
      border: 2px solid black;
      border-radius: 10px;
      background: white;
      color: #A5D9FE;
      cursor: pointer;
      font-weight: bold;
      transition: all 0.3s ease;
      box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
    }
    button:hover {
      transform: translateY(-3px);
      box-shadow: 0 6px 20px rgba(0, 0, 0, 0.3);
    }
    button:active {
      transform: translateY(-1px);
    }
    #stopBtn {
      background: #ff4757;
      color: white;
    }
    #startBtn {
      background: #A5D9FE;
      color: white;
    }
    .history-section {
      background: rgba(255, 255, 255, 0.1);
      border-radius: 15px;
      padding: 20px;
      margin-top: 30px;
    }
    #historyList {
      text-align: left;
      max-height: 200px;
      overflow-y: auto;
    }
    .feedback {
      background: rgba(255, 255, 255, 0.2);
      border-radius: 10px;
      padding: 15px;
      margin-top: 20px;
      font-size: 18px;
      font-weight: bold;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>Hi Areeb, Good to see you again!</h1>
      <p>Let's pushup together</p>
    </div>

    <div class="controls">
      <button onclick="sendCmd('calibrate')" id="calibrateBtn"> Calibrate Device</button>
      <button onclick="sendCmd('start')" id="startBtn"> Start Session</button>
      <button onclick="sendCmd('stop')" id="stopBtn" style="display:none;"> Stop Session</button>
    </div>

    <div class="stats-grid">
      <div class="stat-card">
        <h3>Alltime Pushing Count</h3>
        <div class="stat-value" id="totalCount">0</div>
        <p>Lifetime pushups</p>
      </div>
      <div class="stat-card">
        <h3>Alltime Calories Burned</h3>
        <div class="stat-value" id="totalCalories">0</div>
        <p>kcal</p>
      </div>
      <div class="stat-card">
        <h3>Highest Pushing Count</h3>
        <div class="stat-value" id="maxCount">0</div>
        <p>Your Best</p>
      </div>
      <div class="stat-card">
        <h3>Session Duration</h3>
        <div class="stat-value" id="duration">0</div>
        <p>seconds</p>
      </div>
    </div>

    <div class="feedback" id="feedback"></div>

    <div class="history-section">
      <h1 style="text-align: left;">Session History</h1>
      <div id="historyList">No sessions yet</div>
      <button onclick="loadHistory()" style="margin-top:15px;">Refresh History</button>
    </div>
  </div>

  <script>
    let sessionActive = false;
    let totalLifetimePushups = 0;
    let totalLifetimeCalories = 0;
    let maxPushups = 0;

    function sendCmd(cmd) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var data = JSON.parse(this.responseText);
          
          // Update current session stats
          document.getElementById("duration").innerHTML = data.duration || "0";
          document.getElementById("maxCount").innerHTML = data.maxCount || maxPushups;
          
          // Update feedback
          if (data.feedback) {
            document.getElementById("feedback").innerHTML = data.feedback;
            document.getElementById("feedback").style.display = "block";
          }
          
          // Button visibility
          if (cmd === 'start') {
            sessionActive = true;
            document.getElementById("startBtn").style.display = "none";
            document.getElementById("stopBtn").style.display = "inline-block";
            document.getElementById("calibrateBtn").style.display = "none";
          } else if (cmd === 'stop') {
            sessionActive = false;
            document.getElementById("startBtn").style.display = "inline-block";
            document.getElementById("stopBtn").style.display = "none";
            document.getElementById("calibrateBtn").style.display = "inline-block";
            
            // Add to history
            if (data.count > 0) {
              addToHistory(data);
            }
          }
        }
      };
      xhttp.open("GET", "/action?cmd=" + cmd, true);
      xhttp.send();
    }

    function loadHistory() {
      fetch('/history')
        .then(res => res.json())
        .then(data => {
          if (data.sessions && data.sessions.length > 0) {
            let html = "<ul>";
            data.sessions.forEach(session => {
              const date = new Date(session.timestamp).toLocaleString();
              html += `<li>${date}: ${session.pushupCount} pushups, ${session.calories.toFixed(2)} kcal, ${session.duration}s</li>`;
            });
            html += "</ul>";
            document.getElementById("historyList").innerHTML = html;
          }
        });
    }

    function addToHistory(data) {
      const now = new Date().toLocaleString();
      const historyItem = document.createElement("div");
      historyItem.innerHTML = `<strong>${now}:</strong> ${data.count} pushups, ${data.calories} kcal`;
      document.getElementById("historyList").appendChild(historyItem);
      
      // Update lifetime stats
      totalLifetimePushups += parseInt(data.count);
      totalLifetimeCalories += parseFloat(data.calories);
      if (data.count > maxPushups) maxPushups = data.count;
      
      document.getElementById("totalCount").innerHTML = totalLifetimePushups;
      document.getElementById("totalCalories").innerHTML = totalLifetimeCalories.toFixed(2);
      document.getElementById("maxCount").innerHTML = maxPushups;
    }

    // Auto-update stats during session
    setInterval(() => {
      if (sessionActive) {
        fetch('/status')
          .then(res => res.json())
          .then(data => {
            document.getElementById("duration").innerHTML = data.duration || "0";
            if (data.count) {
              document.getElementById("maxCount").innerHTML = 
                Math.max(data.count, maxPushups);
            }
          });
      }
    }, 2000);

    // Load history on page load
    window.onload = loadHistory;
  </script>
</body>
</html>
)rawliteral";

// MongoDB Functions
void saveToMongoDB(SessionData session) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  String url = String(mongoServer) + String(mongoEndpoint);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  String jsonData = "{";
  jsonData += "\"pushupCount\":" + String(session.pushupCount) + ",";
  jsonData += "\"totalCalories\":" + String(session.totalCalories, 3) + ",";
  jsonData += "\"duration\":" + String(session.duration, 1) + ",";
  jsonData += "\"timestamp\":" + String(session.timestamp) + ",";
  jsonData += "\"maxPushups\":" + String(session.maxPushups) + ",";
  jsonData += "\"avgPushTime\":" + String(session.avgPushTime, 2);
  jsonData += "}";
  
  int httpCode = http.POST(jsonData);
  
  if (httpCode == HTTP_CODE_OK) {
    Serial.println("✓ Session saved to MongoDB");
  } else {
    Serial.println("✗ MongoDB save failed: " + String(httpCode));
  }
  
  http.end();
}

void calibrateSensor() {
  Serial.println("\nCalibrating... move slowly up and down once or twice.");
  minZ = 10; maxZ = -10;

  unsigned long t0 = millis();
  while (millis() - t0 < 5000) {
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);
    float z = az / 16384.0;
    if (z < minZ) minZ = z;
    if (z > maxZ) maxZ = z;
    delay(20);
  }

  downThreshold = minZ + (maxZ - minZ) * 0.3;
  upThreshold   = minZ + (maxZ - minZ) * 0.7;
  calibrated = true;

  Serial.println("Calibration complete!");
}

void startSession() {
  if (!calibrated) {
    Serial.println("Please calibrate first!");
    return;
  }
  sessionActive = true;
  pushupCount = 0;
  totalCalories = 0;
  startTime = millis();
  lastPushTime = startTime;
  durations = 0;
  
  // Initialize session data
  currentSession = {0, 0.0, 0.0, 0, 0, 0.0};
  
  Serial.println("Session started!");
}

void stopSession() {
  sessionActive = false;
  
  // Calculate final stats
  currentSession.pushupCount = pushupCount;
  currentSession.totalCalories = totalCalories;
  currentSession.duration = durations;
  currentSession.timestamp = millis();
  currentSession.maxPushups = pushupCount;
  currentSession.avgPushTime = (pushupCount > 0) ? (durations / pushupCount) : 0;
  
  // Save to MongoDB
  saveToMongoDB(currentSession);
  
  Serial.println("Session stopped! Data saved.");
}

float calcMET(float pushTime) {
  if (pushTime <= 1.0) return 10.0;
  if (pushTime <= 2.0) return 8.0;  
  if (pushTime <= 3.0) return 7.0;
  return 6.0;
}

void detectPushup() {
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  float z = az / 16384.0;

  if (z < downThreshold && !goingDown) {
    goingDown = true;
  } 
  else if (z > upThreshold && goingDown) {
    pushupCount++;
    unsigned long now = millis();
    float pushTime = (now - lastPushTime) / 1000.0;
    lastPushTime = now;
    durations += pushTime;

    float MET = calcMET(pushTime);
    float calories = MET * userWeight * (pushTime / 3600.0);
    totalCalories += calories;

    Serial.printf("Push-up #%d | Time: %.2fs | MET: %.1f | Cal: %.4f\n", 
                  pushupCount, pushTime, MET, calories);
    goingDown = false;
  }
  delay(50);
}

String makeJSON(bool includeFeedback = false) {
  String json = "{";
  json += "\"count\":" + String(pushupCount);
  json += ",\"calories\":" + String(totalCalories, 3);
  json += ",\"duration\":" + String(durations, 1);
  json += ",\"maxCount\":" + String(pushupCount);
  
  if (includeFeedback) {
    if (pushupCount > 0) {
      json += ",\"feedback\":\"🔥 Great session! You completed " + String(pushupCount) + " pushups!\"";
    } else {
      json += ",\"feedback\":\"💪 Ready for your workout? Let's go!\"";
    }
  }
  
  json += "}";
  return json;
}

String makeHistoryJSON() {
  String json = "{\"sessions\":[";
  // For now, return current session
  json += "{";
  json += "\"pushupCount\":" + String(pushupCount) + ",";
  json += "\"calories\":" + String(totalCalories, 3) + ",";
  json += "\"duration\":" + String(durations, 1) + ",";
  json += "\"timestamp\":" + String(millis());
  json += "}";
  json += "]}";
  return json;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  mpu.initialize();
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println("IP: " + WiFi.localIP().toString());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/action", HTTP_GET, [](AsyncWebServerRequest *request){
    String cmd;
    if (request->hasParam("cmd")) cmd = request->getParam("cmd")->value();

    if (cmd == "calibrate") {
      // calibrateSensor();
      // request->send(200, "application/json", "{\"status\":\"calibrated\"}");

      xTaskCreate(
        [](void*) {
          calibrateSensor();
          vTaskDelete(NULL);
        },
        "calibTask", 4096, NULL, 1, NULL
      );
      String json = "{\"status\":\"calibrating\"}";
      request->send(200, "application/json", json);
    } 
    else if (cmd == "start") {
      startSession();
      request->send(200, "application/json", makeJSON());
    }
    else if (cmd == "stop") {
      stopSession();
      request->send(200, "application/json", makeJSON(true));
    }
    else if (cmd == "status") {
      request->send(200, "application/json", makeJSON());
    }
  });

  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", makeHistoryJSON());
  });

  server.begin();
}

void loop() {
  if (sessionActive) detectPushup();
}
