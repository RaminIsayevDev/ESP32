#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

const char* ssid = "ALHN-3405";
const char* password = "0554540509";

WebServer server(80);

const char* loginIndex = 
  "<form name='loginForm'>"
  "<input name='pass' type='password' placeholder='Password'>"
  "<input type='submit' value='Login'></form>";

const char* serverIndex = 
  "<form method='POST' action='/update' enctype='multipart/form-data'>"
  "<input type='file' name='update'>"
  "<input type='submit' value='Update'></form>";

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });

  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  delay(1);
}