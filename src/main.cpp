#include <WiFi.h>     //Wifi library
#include "esp_wpa2.h" //wpa2 library for connections to Enterprise networks
#include <HardwareSerial.h>

#include <ArduinoHttpClient.h>

#define EAP_ANONYMOUS_IDENTITY "anonymous@tuke.sk" // anonymous@example.com, or you can use also nickname@example.com
#define EAP_IDENTITY "jc485@uakron.edu"            // nickname@example.com, at some organizations should work nickname only without realm, but it is not recommended
#define EAP_PASSWORD "plaintext_password_yikes"               // password for eduroam account

// SSID NAME
const char *ssid = "eduroam"; // eduroam SSID

char serverAddress[] = "coyboy.ddns.net"; // server address
int port = 8000;
WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);
String authUsername = "bonesmalone";
String authPassword = "spookyboys";
String experimentID = "";

bool flip = true;

void setup()
{
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.print(F("Connecting to network: "));
  Serial.println(ssid);
  WiFi.disconnect(true);                                                                // disconnect form wifi to set new wifi connection
  WiFi.mode(WIFI_STA);                                                                  // init wifi mode
  WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_ANONYMOUS_IDENTITY, EAP_IDENTITY, EAP_PASSWORD); // without CERTIFICATE

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(F("."));
    if (flip == true)
    {
      flip = false;
      digitalWrite(2, HIGH);
    }
    else
    {
      flip = true;
      digitalWrite(2, LOW);
    }
  }
  Serial.println("");
  Serial.println(F("WiFi is connected!"));
  Serial.println(F("IP address set: "));
  Serial.println(WiFi.localIP()); // print LAN IP
  digitalWrite(2, HIGH);
  delay(5000);
  digitalWrite(2, LOW);

  delay(10);
  Serial.println();
  Serial.println("making GET request");
  client.beginRequest();
  client.get("/init/get_experiment_id");
  client.sendBasicAuth(authUsername, authPassword);
  // client.sendHeader("X-CUSTOM-HEADER", "custom_value");
  client.endRequest();

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();
  experimentID = response;
  Serial.print("GET Status code: ");
  Serial.println(statusCode);
  Serial.print("GET Response: ");
  Serial.println(response);
  delay(100);

  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  delay(100);
}

void send_data(String temp, String ph)
{
  digitalWrite(2, HIGH);
  Serial.println("making POST request");
  String postData = "{\"logger_id\": \"1\",\"experiment_id\": " + experimentID + ",\"ph_bytes\": \"" + ph + "\",\"temp_bytes\": \"" + temp + "\"}";
  Serial.println(postData);
  client.beginRequest();
  client.post("/data/r3000sd/");
  client.sendBasicAuth(authUsername, authPassword);
  client.sendHeader(HTTP_HEADER_CONTENT_TYPE, "application/json");
  client.sendHeader(HTTP_HEADER_CONTENT_LENGTH, postData.length());
  client.endRequest();
  client.write((const byte *)postData.c_str(), postData.length());
  // note: the above line can also be achieved with the simpler line below:
  // client.print(postData);

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("POST Status code: ");
  Serial.println(statusCode);
  Serial.print("POST Response: ");
  Serial.println(response);
  digitalWrite(2, LOW);
}

int counter;
String ph = "";
String temp = "";
String bytes = "";

void loop()
{
  if (Serial2.available())
  {
    bytes = Serial2.readStringUntil('\r');
  }
  if (bytes[2] == '2')
  {
    temp = bytes;
  }
  if (bytes[2] == '1')
  {
    ph = bytes;
  }

  if (ph != "" && temp != "")
  {
    counter++;
    temp[0] = '#';
    ph[0] = '#';
    Serial.println("temp");
    Serial.println(temp);
    Serial.println("ph");
    Serial.println(ph);
    if (counter >= 20)
    {
      send_data(temp, ph);
      counter = 0;
    }
    temp = "";
    ph = "";
  }
}
