
// This is my code where callibration is working absolutely fine
#include <Wire.h>
#include <MPU6050.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "FiOS-YN4HE";
const char* password = "roof376open5348net";

MPU6050 mpu;
AsyncWebServer server(80);

bool sessionActive = false;
bool calibrated = false;

unsigned long startTime = 0;
unsigned long lastPushTime = 0;
float durations = 0;
int pushupCount = 0;

float minZ = 10, maxZ = -10; // g = 9.8 m/s2 ~ 10 m/s2
float downThreshold, upThreshold;
bool goingDown = false;
float totalCalories = 0;

const float userWeight = 70.0; // kg

// ---- NEW: Session history for motivating feedback ---- 
#define MAX_SESSIONS 5
int sessionHistory[MAX_SESSIONS] = {0};
int sessionIndex = 0;

// ------------------------------------------------------

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Push-up Counter</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; margin: 40px; }
    button { padding: 10px 20px; margin: 10px; font-size: 16px; }
    h2 { margin-top: 20px; }
  </style>
</head>
<body>
  <script>
    function sendCmd(cmd) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var data = JSON.parse(this.responseText);
          document.getElementById("counts").innerHTML = "Pushups: " + data.count;
          document.getElementById("duration").innerHTML = "Duration: " + data.duration + " s";
          document.getElementById("calories").innerHTML = "Calories: " + data.calories.toFixed(2);
          if (data.feedback) {
            document.getElementById("feedback").innerHTML = data.feedback;
          }
        }
      };
      xhttp.open("GET", "/action?cmd=" + cmd, true);
      xhttp.send();
    }
    setInterval(() => sendCmd('status'), 2000);
  </script>
  <h1>Push-up Counter</h1>
  <button onclick="sendCmd('calibrate')">Calibrate</button>
  <button onclick="sendCmd('start')">Start</button>
  <button onclick="sendCmd('stop')">Stop</button>
  <h2 id="counts"></h2>
  <h2 id="duration"></h2>
  <h2 id="calories"></h2>
  <h2 id="feedback"></h2>
</body>
</html>
)rawliteral";

// ---- NEW: Functions for motivating feedback ----
void updateSessionHistory(int count) {
  sessionHistory[sessionIndex] = count;
  sessionIndex = (sessionIndex + 1) % MAX_SESSIONS;
}

int getMaxPushups() {
  int maxVal = 0;
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (sessionHistory[i] > maxVal) maxVal = sessionHistory[i];
  }
  return maxVal;
}
// ------------------------------------------------------

void calibrateSensor() {
  Serial.println("\nCalibrating... move slowly up and down once or twice.");
  minZ = 10; maxZ = -10;

  unsigned long t0 = millis();
  while (millis() - t0 < 5000) { // Calibrate within 5 seconds
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
  Serial.printf("minZ=%.2f, maxZ=%.2f\n", minZ, maxZ);
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
  Serial.println("Session started!");
}

void stopSession() {
  sessionActive = false;
  Serial.println("Session stopped!");

  int prevMax = getMaxPushups();
  updateSessionHistory(pushupCount);

  if (pushupCount > prevMax) {
    Serial.printf("🔥 New Record! You did %d push-ups, beating your previous best of %d!\n", pushupCount, prevMax);
  }
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

    Serial.printf("Push-up #%d | Time: %.2fs | MET: %.1f | Cal: %.4f\n", pushupCount, pushTime, MET, calories);

    goingDown = false;
  }
  delay(50);
}

String makeJSON(bool includeFeedback = false) {
  String json = "{";
  json += "\"count\":" + String(pushupCount);
  json += ",\"duration\":" + String(durations, 1);
  json += ",\"calories\":" + String(totalCalories, 3);

  if (includeFeedback) {
    int prevMax = getMaxPushups();
    if (pushupCount > prevMax) {
      json += ",\"feedback\":\"🔥 New Record! Keep it up!\"";
    } else {
      json += ",\"feedback\":\"Good job! Aim higher next time!\"";
    }
  }

  json += "}";
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
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/action", HTTP_GET, [](AsyncWebServerRequest *request){
    String cmd;
    if (request->hasParam("cmd")) cmd = request->getParam("cmd")->value();

    if (cmd == "calibrate") {
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
    else if (cmd == "start") startSession();
    else if (cmd == "stop") {
      stopSession();
      String json = makeJSON(true);
      request->send(200, "application/json", json);
    }
    else if (cmd == "status") {
      String json = makeJSON(true);
      request->send(200, "application/json", json);
    }
  });

  server.begin();
}

void loop() {
  if (sessionActive) detectPushup();

}