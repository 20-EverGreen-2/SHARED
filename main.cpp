


#if 1

#include <ESP8266WiFi.h>


const char * ssid = "SSID";
const char * password = "PASSWORD";

WiFiServer server(65000);
WiFiClient client;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN,OUTPUT);
  digitalWrite(LED_BUILTIN,HIGH);
  
  Serial.begin(115200);

  Serial.println();
  Serial.println();
  Serial.println();
  Serial.printf("Try connecting to: %s\n",ssid);
  Serial.printf("ESP MAC: %s\n", WiFi.macAddress().c_str());

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(4000);
    Serial.print(".");
  }

  uint8_t t = 0;
  while (t++ < 6) {

    if (WiFi.status() == WL_CONNECTED) {
      
      // Send info via Serial for debugging purposes
      Serial.println("\n");
      Serial.printf("Succesfully connected to: %s\n", ssid);
      Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
      Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
      Serial.printf("DNS address: %s\n", WiFi.dnsIP().toString().c_str());
      Serial.println("\n");

      server.begin();   // Start of HTTP Server
      Serial.println("HTTP Server started");
      Serial.print("Use this URL to connect: ");
      Serial.printf("http://%s:%d\n", WiFi.localIP().toString().c_str(), server.port());

      break;
    } else {  // Error Handler
      Serial.println("\n");
      Serial.printf("Failed to connect to WiFi AP: %s\n", ssid);
      while(1){

        Serial.println("Waiting for user to press reset button!!!");
        delay(5000);
      }
    }
  }
}

uint32_t Client_Number = 0;
void loop() {
  // put your main code here, to run repeatedly:
  
  while (!(client = server.available())) {
    delay(1);
    // wdt_reset();
  }
  
  Client_Number++;
  Serial.printf("New client Nº: %d\n", Client_Number);
  uint32_t ClientTimeOut = 5000 + millis(); // Client TimeOut for slower connections
  // Wait until the client sends some data
  while(!client.available()) {

    // wdt_reset();
    delay(1);
    // Drop User if TimedOut and restart loop!!!
    if (millis() > ClientTimeOut) {
      Serial.printf("Dropped client Nº: %d\nCause: timed out\n", Client_Number);
      client.stopAll();
      return;
    }
  }
  
  // Show LED blinks on client Connect or sends data
  // Acts like a WiFi activity but nope XD
  for (uint8_t activity = 0; activity < 8; activity++) {
    digitalWrite(LED_BUILTIN,!(digitalRead(LED_BUILTIN)));
    delay(25);
    // wdt_reset();
  }

  Serial.printf("Client Remote IP: %s\n",client.remoteIP().toString().c_str());

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();
 
  // Match the request
 
  int value = digitalRead(LED_BUILTIN);
  if (request.indexOf("/LED=ON") != -1)  {
    digitalWrite(LED_BUILTIN, LOW);
    value = LOW;
  }
  if (request.indexOf("/LED=OFF") != -1)  {
    digitalWrite(LED_BUILTIN, HIGH);
    value = HIGH;
  }
 
// Set ledPin according to the request
//digitalWrite(ledPin, value);
 
  // Return the response
  // Serial.println(__LINE__);
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
 

  client.print("El LED se encuentra ahora: ");
 
  if(value == LOW) {
    client.print("Encendido");
  } else {
    client.print("Apagado");
  }
  client.println("<br><br>");
  client.println("<a href=\"/LED=ON\"\"><button>Encender LED </button></a>");
  client.println("<a href=\"/LED=OFF\"\"><button>Apagar LED </button></a><br />");  
  client.println("</html>");
  client.println("\r\n\r") ;


  delay(1);
  Serial.println("Client disconnected");
  Serial.println("");  
}


// Next ELSE is a Turman code, thanks to him to help me on this long way to get a better codes!!!
#else


#include <ESP8266WiFi.h>

const char* ssid = "SSID";
const char* password ="PASSWORD";


WiFiServer server(65000);
WiFiClient client;



void setup() {

  Serial.begin(115200);
  
  WiFi.begin(ssid,password);
  server.setNoDelay(true);
  server.begin();  

  while (WiFi.status() != WL_CONNECTED) { delay(100); }
  
}

void loop() {

client = server.available();
if (!client) { return;} 

 String req="";
 
 int contador=0;
 while(!client.connected()) {
   delay(1);
   contador++;
   if(contador>10){
     return;
     }
}
 contador=0;
 while(!client.available()) {delay(1);contador++;if(contador>10){return;}}

 char c;
 do{          
    c = client.read();
    req+=c; 
    //Serial.println(req);Serial.println("\r\n");       
 }while(c!='\n');
          
if(req.indexOf("/ ")!=-1){
  
  client.print("<html>HOLA CARACOLA</html>\r\n\r\n");
   
}


client.flush();
client.stop();   
client.stopAll();  

}


#endif










