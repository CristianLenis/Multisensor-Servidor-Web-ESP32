#include <WiFi.h>
#include <EEPROM.h>
#include "DHT.h"
#include <Adafruit_Sensor.h>

const char* ssid     = "NombreRed";
const char* password = "Contraseña";

#define DHTTYPE DHT11

const int DHTPin = 27;
DHT dht(DHTPin, DHTTYPE);

static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];

#define EEPROM_SIZE 4

const int output = 2;
const int redRGB = 14;
const int greenRGB = 12;
const int blueRGB = 13;
const int motionSensor = 25;
const int ldr = 33;
String outputState = "Apagado";

long now = millis();
long lastMeasure = 0;
boolean startTimer = false;

int selectedMode = 0;
int timer = 0;
int ldrThreshold = 0;
int armMotion = 0;
int armLdr = 0;
String modes[4] = { "Manual", "Sensor De Movimiento", "Sensor de Luz", "Sensor de Luz y Movimiento" };

String valueString = "0";
int pos1 = 0;
int pos2 = 0;
// Variable to store the HTTP request
String header;
WiFiServer server(80);

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void setup() {
  dht.begin();

  Serial.begin(115200);

  pinMode(motionSensor, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);
  
  Serial.println("start...");
  if(!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("failed to initialise EEPROM"); 
    delay(1000);
  }

  Serial.println(" bytes read from Flash . Values are:");
  for(int i = 0; i < EEPROM_SIZE; i++) {
    Serial.print(byte(EEPROM.read(i))); 
    Serial.print(" ");
  }
  
  pinMode(output, OUTPUT);
  pinMode(redRGB, OUTPUT);
  pinMode(greenRGB, OUTPUT);
  pinMode(blueRGB, OUTPUT);

  if(!EEPROM.read(0)) {
    outputState = "Apagado";
    digitalWrite(output, HIGH);
  }
  else {
    outputState = "Encendido";
    digitalWrite(output, LOW);
  }
  selectedMode = EEPROM.read(1);
  timer = EEPROM.read(2);
  ldrThreshold = EEPROM.read(3);
  configureMode();
  
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop() {
  WiFiClient client = server.available();   
  if (client) {                             
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          
    String currentLine = "";                
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  
      currentTime = millis();
      if (client.available()) {             
        char c = client.read();             
        Serial.write(c);                    
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();                     
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<title>ESP32 MultiSensor</title>");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/Apagado buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>*{background-color: #E0E5EC;}");
            client.println("html { font-family: system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".bold {font-weight: bold;}");
            client.println(".boton {font-family: system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, 'Open Sans', 'Helvetica Neue', sans-serif;background-color: #E0E5EC;border: none;border-radius: 10px; box-shadow:  5px 5px 16px #A3B1C6, -9px -9px 16px #FFFFFF; text-align-last: center;}");
            client.println(".seleccion {height: 35px;width: 150px;}");
            client.println(".bloque {box-shadow: 2px 2px 16px 0 #A3B1C6 inset, -4px -4px 12px 0 #FFFFFF inset; border-radius: 50px; margin-left: auto; margin-right: auto; justify-content: center;  align-items: center;}");
            client.println(".button {background-color: #4CAF50; border: 5px solid #E0E5EC; color: white;padding: 16px 40px;box-shadow:  5px 5px 16px #A3B1C6, -9px -9px 16px #FFFFFF;border-radius: 50px;text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;border: 5px solid #E0E5EC;color: white;padding: 16px 40px;box-shadow:  5px 5px 16px #A3B1C6, -9px -9px 16px #FFFFFF;border-radius: 50px;}");
            client.println(".sensor {height: 35px; width: 100px; display: flex;}");
            client.println(".estado {height: 35px; width: 250px; display: flex;}");
            client.println(".time {margin: 10px; text-align-last: center; height: 20px; width: 48px;}");
            client.println(".mov {height: 35px; width: 265px; display: flex;}");
            client.println(".luz {height: 35px; width: 240px; display: flex;}");
            client.println(".dht {height: 40px; width: 200px;}</style></head>");
  
  
            
           

            // Request example: GET /?mode=0& HTTP/1.1 - sets mode to Manual (0)
            if(header.indexOf("GET /?mode=") >= 0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              selectedMode = valueString.toInt();
              EEPROM.write(1, selectedMode);
              EEPROM.commit();
              configureMode();
            }
            // Change the output state - turn GPIOs on and Apagado
            else if(header.indexOf("GET /?state=On") >= 0) {
              outputOn();
            } 
            else if(header.indexOf("GET /?state=Apagado") >= 0) {
              outputApagado();
            }
            // Set timer value
            else if(header.indexOf("GET /?timer=") >= 0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              timer = valueString.toInt();
              EEPROM.write(2, timer);
              EEPROM.commit();
              Serial.println(valueString);
            }
            // Set LDR Threshold value
            else if(header.indexOf("GET /?ldrthreshold=") >= 0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              ldrThreshold = valueString.toInt();
              EEPROM.write(3, ldrThreshold);
              EEPROM.commit();
              Serial.println(valueString);
            }
            
            // Web Page Heading
            client.println("<body><br><h1>ESP32 Servidor Web: MultiSensor</h1><br>");
            // Drop down menu to select mode
            client.println("<p><strong>Modo Seleccionado:</strong> " + modes[selectedMode] + "</p>");
            client.println("<select id=\"mySelect\" onchange=\"setMode(this.value)\" class=\"boton seleccion bold\">");
            client.println("<option>Cambiar Modo");
            client.println("<option value=\"0\">Manual");
            client.println("<option value=\"1\">Sensor de Movimiento");
            client.println("<option value=\"2\">Sensor de Luz");
            client.println("<option value=\"3\">Sensor de Luz y Movimiento</select>");
            client.println("<br>");
            client.println("<br>");
          
            // Display current state, and ON/Apagado buttons for output 
            client.println("<br>");
            client.println("<br>");
            client.println("<p class=\"bloque bold estado\">Estado del Foco: " + outputState + "</p>");
            // If the output is Apagado, it displays the ON button       
            if(selectedMode == 0) {
              if(outputState == "Apagado") {
                client.println("<p><button class=\"button bold\" onclick=\"outputOn()\">Encender</button></p><br>");
              } 
              else {
                client.println("<p><button class=\"button button2 bold\" onclick=\"outputApagado()\">Apagar</button></p><br>");
              }
            }
            else if(selectedMode == 1) {
              client.println("<br>");
              client.println("<center> <p class=\"bloque mov bold\">Tiempo Encendido: <input class=\"bloque time\" type=\"number\" name=\"txt\" value=\"" + String(EEPROM.read(2)) + "\" onchange=\"setTimer(this.value)\" min=\"0\" max=\"255\"></p></center>");
              client.println("<br>");                
            }
            else if(selectedMode == 2) {
              client.println("<br>");
              client.println("<center> <p class=\"bloque luz bold\">Umbral de Luz: <input class=\"bloque time\" type=\"number\" name=\"txt\" value=\"" + String(EEPROM.read(3)) + "\" onchange=\"setThreshold(this.value)\" min=\"0\" max=\"100\"></p></center>");
              client.println("<br>");                
            }
            else if(selectedMode == 3) {
              client.println("<br>");
              client.println("<center> <p class=\"bloque mov bold\">Tiempo Encendido: <input class=\"bloque time\" type=\"number\" name=\"txt\" value=\"" + String(EEPROM.read(2)) + "\" onchange=\"setTimer(this.value)\" min=\"0\" max=\"255\"></p></center>");
                               
              client.println("<center> <p class=\"bloque luz bold\">Umbral de Luz: <input class=\"bloque time\" type=\"number\" name=\"txt\" value=\"" + String(EEPROM.read(3)) + "\" onchange=\"setThreshold(this.value)\" min=\"0\" max=\"100\"></p>");  
              client.println("<br>");                           
            }
            // Get and display DHT sensor readings
            if(header.indexOf("GET /?sensor") >= 0) {
              // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
              float h = dht.readHumidity();
              // Read temperature as Celsius (the default)
              float t = dht.readTemperature();
              // Read temperature as Fahrenheit (isFahrenheit = true)
              float f = dht.readTemperature(true);
              // Check if any reads failed and exit early (to try again).
              if (isnan(h) || isnan(t) || isnan(f)) {
                Serial.println("Failed to read from DHT sensor!");
                strcpy(celsiusTemp,"50");
                strcpy(fahrenheitTemp, "50");
                strcpy(humidityTemp, "50");         
              }
              else {
                // Computes temperature values in Celsius + Fahrenheit and Humidity
                float hic = dht.computeHeatIndex(t, h, false);       
                dtostrf(hic, 6, 2, celsiusTemp);             
                float hif = dht.computeHeatIndex(f, h);
                dtostrf(hif, 6, 2, fahrenheitTemp);         
                dtostrf(h, 6, 2, humidityTemp);
                // You can delete the following Serial.prints, it s just for debugging purposes
                /*Serial.print("Humidity: "); Serial.print(h); Serial.print(" %\t Temperature: "); 
                Serial.print(t); Serial.print(" *C "); Serial.print(f); 
                Serial.print(" *F\t Heat index: "); Serial.print(hic); Serial.print(" *C "); 
                Serial.print(hif); Serial.print(" *F"); Serial.print("Humidity: "); 
                Serial.print(h); Serial.print(" %\t Temperature: "); Serial.print(t);
                Serial.print(" *C "); Serial.print(f); Serial.print(" *F\t Heat index: ");
                Serial.print(hic); Serial.print(" *C "); Serial.print(hif); Serial.println(" *F");*/
              }
              client.println("<center><p class=\"bloque sensor\">");
              client.println(celsiusTemp);
              client.println("*C</p></center><center><p class=\"bloque sensor\">");
              client.println(fahrenheitTemp);
              client.println("*F</p></center><center></div><p class=\"bloque sensor\">");
              client.println(humidityTemp);
              client.println("%</p></center></div>");
              client.println("<p><a href=\"/\"><button class=\"boton dht bold\">Ocultar Lecturas del Sensor</button></a></p>");
            }
            else {
              client.println("<p><a href=\"?sensor\"><button class=\"boton dht bold\">Mostrar Lecturas del Sensor</button></a></p>");
            }
            client.println("<script> function setMode(value) { var xhr = new XMLHttpRequest();"); 
            client.println("xhr.open('GET', \"/?mode=\" + value + \"&\", true);"); 
            client.println("xhr.send(); setInterval(function(){ location.reload(true); }, 1500); } ");
            client.println("function setTimer(value) { var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?timer=\" + value + \"&\", true);"); 
            client.println("xhr.send(); setInterval(function(){ location.reload(true); }, 1500); } ");
            client.println("function setThreshold(value) { var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?ldrthreshold=\" + value + \"&\", true);"); 
            client.println("xhr.send(); setInterval(function(){ location.reload(true); }, 1500); } ");
            client.println("function outputOn() { var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?state=On\", true);"); 
            client.println("xhr.send(); setInterval(function(){ location.reload(true); }, 1500); } ");
            client.println("function outputApagado() { var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?state=Apagado\", true);"); 
            client.println("xhr.send(); setInterval(function(){ location.reload(true); }, 1500); } ");
            client.println("function updateSensorReadings() { var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?sensor\", true);"); 
            client.println("xhr.send(); setInterval(function(){ location.reload(true); }, 1500); }</script></body></html>");
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
  }
  
  // Starts a timer to turn on/Apagado the output according to the time value or LDR reading
  now = millis();
  
  // Mode selected (1): Auto PIR
  if(startTimer && armMotion && !armLdr) {
    if(outputState == "Apagado") {
      outputOn();
    }
    else if((now - lastMeasure > (timer * 1000))) {
      outputApagado();
      startTimer = false;     
    }
  }  
  
  // Mode selected (2): Auto LDR
  // Read current LDR value and turn the output accordingly
  if(armLdr && !armMotion) {
    int ldrValue = map(analogRead(ldr), 0, 4095, 0, 100); 
    //Serial.println(ldrValue);
    if(ldrValue > ldrThreshold && outputState == "Encendido") {
      outputApagado();
    }
    else if(ldrValue < ldrThreshold && outputState == "Apagado") {
      outputOn();
    }
    delay(100);
  }
  
  // Mode selected (3): Auto PIR and LDR
  if(startTimer && armMotion && armLdr) {
    int ldrValue = map(analogRead(ldr), 0, 4095, 0, 100);
    //Serial.println(ldrValue);
    if(ldrValue > ldrThreshold) {
      outputApagado();
      startTimer = false;
    }
    else if(ldrValue < ldrThreshold && outputState == "Apagado") {
      outputOn();
    }
    else if(now - lastMeasure > (timer * 1000)) {
      outputApagado();
      startTimer = false;
    }
  }
}

// Checks if motion was detected and the sensors are armed. Then, starts a timer.
void detectsMovement() {
  if(armMotion || (armMotion && armLdr)) {
    Serial.println("MOVIMIENTO DETECTADO!!!");
    startTimer = true;
    lastMeasure = millis();
  }  
}
void configureMode() {
  // Mode: Manual
  if(selectedMode == 0) {
    armMotion = 0;
    armLdr = 0;
    // RGB LED color: red
    digitalWrite(redRGB, HIGH);
    digitalWrite(greenRGB, LOW);
    digitalWrite(blueRGB, LOW);
  }
  // Mode: Auto PIR
  else if(selectedMode == 1) {
    outputApagado();
    armMotion = 1;
    armLdr = 0;
    // RGB LED color: green
    digitalWrite(redRGB, LOW);
    digitalWrite(greenRGB, HIGH);
    digitalWrite(blueRGB, LOW);
  }
  // Mode: Auto LDR
  else if(selectedMode == 2) {
    armMotion = 0;
    armLdr = 1;
    // RGB LED color: blue    
    digitalWrite(redRGB, LOW);
    digitalWrite(greenRGB, LOW);
    digitalWrite(blueRGB, HIGH);
  }
  // Mode: Auto PIR and LDR
  else if(selectedMode == 3) {
    outputApagado();
    armMotion = 1;
    armLdr = 1;
    // RGB LED color: purple    
    digitalWrite(redRGB, HIGH);
    digitalWrite(greenRGB, LOW);
    digitalWrite(blueRGB, HIGH);
  }
}

// Change output pin to on or Apagado
void outputOn() {
  Serial.println("GPIO On");
  outputState = "Encendido";
  digitalWrite(output, LOW);
  EEPROM.write(0, 1);
  EEPROM.commit();
}
void outputApagado() { 
  Serial.println("GPIO Apagado");
  outputState = "Apagado";
  digitalWrite(output, HIGH);
  EEPROM.write(0, 0);
  EEPROM.commit();
}
