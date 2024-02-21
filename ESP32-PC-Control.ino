#include <WiFi.h>
#include <WebServer.h>

char ssid[] = "";
char pass[] = "";

WebServer server(80);

const int PowerStatePin = 15;
const int PowerControlPin = 4;
const int ShutdownActivatedPin = 2;

int SecondsCounter = 0;

#define PRESS_DELAY 300 // Задержка активации пина

void WebServerLoop(void *pvParameters) {
  while(true) {
    server.handleClient();
  }
}

void TimerLoop(void *pvParameters) {
  while(true) {
    if(SecondsCounter) {
      SecondsCounter--;
      if(!SecondsCounter) {
        digitalWrite(ShutdownActivatedPin, LOW);
        digitalWrite(PowerControlPin, HIGH);
        delay(PRESS_DELAY);
        digitalWrite(PowerControlPin, LOW);
      }
    }
    delay(1000);
  }
}

void handle_OnConnect() {
  bool state = digitalRead(PowerStatePin);
  if(state) {
    if(!SecondsCounter)
    {
      server.send(200, "text/html", SendHTML("Компьютер включен"));
    }
    else
    {
      char buffer[64];
      sprintf(buffer, "Компьютер выключится через %d секунд", SecondsCounter);
      server.send(200, "text/html", SendHTML(buffer));
    }
  }
  else {
    server.send(200, "text/html", SendHTML("Компьютер выключен"));
  }
}

void handle_TurnOnPC() {
  bool state = digitalRead(PowerStatePin);
  if(!state) {
    digitalWrite(ShutdownActivatedPin, HIGH);
    digitalWrite(PowerControlPin, HIGH);
    delay(PRESS_DELAY);
    digitalWrite(ShutdownActivatedPin, LOW);
    digitalWrite(PowerControlPin, LOW);
    server.send(200, "text/html", SendHTML("Компьютер включен"));
  }
  else {
    server.send(200, "text/html", SendHTML("Компьютер уже включен"));
  }
}

void handle_TurnOffPC() {
  bool state = digitalRead(PowerStatePin);
  if(state) {
    if(!SecondsCounter) {
      SecondsCounter = 60;
      digitalWrite(ShutdownActivatedPin, HIGH);
      server.send(200, "text/html", SendHTML("Компьютер выключится через 60 секунд"));
    }
    else {
      SecondsCounter = 0;
      digitalWrite(ShutdownActivatedPin, LOW);
      server.send(200, "text/html", SendHTML("Действие отменено"));
    }
  }
  else {
    server.send(200, "text/html", SendHTML("Компьютер уже выключен"));
  }
}

void handle_StatePC() {
  bool state = digitalRead(PowerStatePin);
  if(state) {
    server.send(200, "application/json", "{\"value\":true}");
  }
  else {
    server.send(200, "application/json", "{\"value\":false}");
  }
}

void handle_NotFound() {
  server.send(200, "text/plain", "Not found");
}

String SendHTML(const char* text){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>PC Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #3498db;}\n";
  ptr +=".button-on:active {background-color: #2980b9;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>ESP32 PC Control</h1>\n";
  ptr +="<h2>";
  ptr +=text;
  ptr +="</h2>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Start ESP32 PC Control\n");

  pinMode(PowerStatePin, INPUT);
  pinMode(PowerControlPin, OUTPUT);

  pinMode(ShutdownActivatedPin, OUTPUT);

  xTaskCreatePinnedToCore(WebServerLoop, "WebServerLoop", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TimerLoop, "TimerLoop", 4096, NULL, 1, NULL, 0);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  Serial.println("\nWi-Fi connected.");
  Serial.println("Server address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.on("/on", handle_TurnOnPC);
  server.on("/off", handle_TurnOffPC);
  server.on("/state", handle_StatePC);
  server.onNotFound(handle_NotFound);
  server.begin();
}

void loop() {}