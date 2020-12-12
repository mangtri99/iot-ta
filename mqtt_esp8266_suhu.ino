#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Arduino_JSON.h>

Adafruit_MLX90614 mlx = Adafruit_MLX90614();
// Update these with values suitable for your network.

const char* ssid = "Imissu";
const char* password = "12345678";
const char* mqtt_server = "broker.mqtt-dashboard.com";
const char* host = "http://035a567a436d.ngrok.io/mqtt-iot/public/api/user/";
const char* hostPost = "http://035a567a436d.ngrok.io/mqtt-iot/public/api/user";

unsigned long lastTime = 0;
unsigned long timerDelay = 5000;
String sensorReadings;
const char* sensorReadingsArr[3];

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

String suhu_str;
String fahre_str;
String no_pasien;
String nama_user_str;
String id_user_str;

char suhu[50];
char fahre[50];
char nama_user[50];
char idUser[50];


int id_user;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String messageTemp;
  String save;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    save += (char)payload[i];
  }
  
  Serial.println();
  
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

    if (strcmp(topic, "iot12/save") == 0) {
      if(save){
        Serial.print("Post...");
        delay(500);
        postData();
        save="";
      }
    }

    if (strcmp(topic, "iot12/id") == 0) {
        if(messageTemp){
          Serial.print("Data Masuk : ");
          Serial.println(messageTemp);
          no_pasien = String(messageTemp);
          getData();
          messageTemp="";
      }   
    } 
  }

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("iot12/celcius", "hello world");
      // ... and resubscribe
      client.subscribe("iot12/#");
//      client.subscribe("iot12/id");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  Serial.println("Adafruit MLX90614 test"); 
  mlx.begin(); 
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}


void getData(){
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
              
      sensorReadings = httpGETRequest(host);
      Serial.println(sensorReadings);
      JSONVar myObject = JSON.parse(sensorReadings);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }

      Serial.print("JSON object = ");
      Serial.println(myObject);
      Serial.println(myObject["id"]);
      id_user = myObject["id"];
      Serial.println(myObject["name"]);
      nama_user_str = myObject["name"];
      id_user_str = String(id_user);
      id_user_str.toCharArray(idUser, id_user_str.length() + 1);
      nama_user_str.toCharArray(nama_user, nama_user_str.length() + 1);
      client.publish("iot12/user", nama_user);      
      
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

String httpGETRequest(const char* host) {
  String url;
  HTTPClient http;
  url = host + no_pasien;
  http.begin(url);
  int httpResponseCode = http.GET();
  Serial.println(url);
  String payload = "{}";

  if(httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else{
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();

  return payload;
}

void postData(){
  String suhu = "37";
  String oksigen = "92";
  String sistole = "125";
  String diastole = "80";
  String bpm = "88";
  String user_id = "2";
  String postJSON = "{\"user_id\":\"" + user_id + "\",\"suhu\":\"" + suhu + "\", \"bpm\":\"" + bpm + "\", \"oksigen\":\"" + oksigen + "\", \"sistole\":\"" + sistole + "\", \"diastole\":\"" + diastole + "\"}";
  HTTPClient http;
  http.begin(hostPost);
  http.addHeader("Content-Type", "application/json");
  Serial.print("httpRequestData: ");
  Serial.println(postJSON);

  int httpResponseCode = http.POST(postJSON);
  if (httpResponseCode = 200){
    client.publish("iot12/status", "Berhasil disimpan");
  }
  else{
    client.publish("iot12/status", "gagal disimpan");
  }
  if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
  http.end();
  delay(1000);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempC()); 
    Serial.print("*C\tObject = "); Serial.print(mlx.readObjectTempC()); Serial.println("*C");
    Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempF()); 
    Serial.print("*F\tObject = "); Serial.print(mlx.readObjectTempF()); Serial.println("*F");

    suhu_str = String(mlx.readObjectTempC());
    suhu_str.toCharArray(suhu, suhu_str.length() + 1);

    fahre_str = String(mlx.readObjectTempF());
    fahre_str.toCharArray(fahre, fahre_str.length() + 1);
    
    client.publish("iot12/celcius", suhu);
    client.publish("iot12/fahre", fahre);
  }
}
