#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

// Define the size of the array
#define ARRAY_SIZE 6

// Define the starting address in the EEPROM memory
#define EEPROM_ADDRESS 0


// bottom_led_num, top_led_num, sensore_correction, temp_red, led_count, led_brightness
int configuration_array[ARRAY_SIZE] = {50, 250, 0, 60, 300, 50};

#define SSID "Vagabonds_sauna_tempr"
#define SSPASS "warmitup"


// GPIO where the DS18B20 is connected to
const int oneWireBus = 2;     
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

//Initial temperatureC
float temperature_C = 0;

// The access points IP address and net mask
// It uses the default Google DNS IP address 8.8.8.8 to capture all 
// Android dns requests
IPAddress apIP(8, 8, 8, 8);
IPAddress netMsk(255, 255, 255, 0);

// DNS server 
const byte DNS_PORT = 53; 
DNSServer dnsServer;

// Web server
ESP8266WebServer server(80);

//LED Strip
#define LED_PIN 4
int led_count = 300;
Adafruit_NeoPixel strip(led_count, LED_PIN, NEO_GRB + NEO_KHZ800);

//initial params, could be overloaded from ESP8266 memory and changed through web interface

//This variable represents the number of the LED that will light up when the temperature exceeds lowest_temp.
//It is used when a portion of the LED strip at the beginning is not used for indication.
int bottom_led_num = 50;
//This variable represents the number of the LED that will be last one to light up when the temperature exceeds highest_temp.
//It is used when a portion of the LED strip at the end is not used for indication.
int top_led_num = 250;
//This variable represents in °C the temperature at which (or more) all LEDs between bottom_led_num and top_led_num will light up.
float highest_temp = 80;
//This variable represents in °C the temperature at which (or less) all LEDs between bottom_led_num and top_led_num will NOT light up.
float lowest_temp = 0;
//This variable represents the temperature, in °C, above which the LEDs should be colored red.
float temp_red = 60;
//This value will be added to the reading obtained from the temperature sensor. Use it for correction if needed, such as when the temperature sensor is placed in a part of the sauna where the temperature is lower than the perceived temperature.
float sensore_correction = 0;
int led_brightness = 50;


// check if this string is an IP address
boolean isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

// checks if the request is for the controllers IP, if not we redirect automatically to the
// captive portal 
boolean captivePortal() {
  if (!isIp(server.hostHeader())) {
    Serial.println("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send(302, "text/plain", "");   
    server.client().stop(); 
    return true;
  }
  return false;
}

void handleRoot() {
  if (captivePortal()) { 
    return;
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  
  String p = "<!DOCTYPE html><html><head><title>Vagabond Temp sensor</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"><meta name=\"mobile-web-app-capable\" content=\"yes\"><meta name=\"apple-mobile-web-app-capable\" content=\"yes\"></head>";
  p += "<body><h1>Value from sensore:"+String(temperature_C)+"</h1>";
  p += "<button onclick=\"javascript:location.reload()\">Reload</button>";
  p += "<button onclick=\"javascript:window.location.href = '/testled';\">Test LED</button>";
  p += "<form action=\"/setvals\">";
  p += "<label for=\"firstled\">First LED num:</label>";
  p += "<input type=\"text\" id=\"firstled\" name=\"firstled\" value=\""+String(bottom_led_num)+"\" required><br>";
  p += "<label for=\"lastled\">Last LED num:</label>";
  p += "<input type=\"text\" id=\"lastled\" name=\"lastled\" value=\""+String(top_led_num)+"\" required><br>";
  p += "<label for=\"temp_red\">Red zone from tempr:</label>";
  p += "<input type=\"text\" id=\"temp_red\" name=\"temp_red\" value=\""+String(temp_red)+"\" required><br>";
  p += "<label for=\"sensore_correction\">sensore correction:</label>";
  p += "<input type=\"text\" id=\"sensore_correction\" name=\"sensore_correction\" value=\""+String(sensore_correction)+"\" required><br>";
  p += "<label for=\"led_count\">led_count:</label>";
  p += "<input type=\"text\" id=\"led_count\" name=\"led_count\" value=\""+String(led_count)+"\" required><br>";
  p += "<label for=\"led_count\">led_brightness in %:</label>";
  p += "<input type=\"text\" id=\"led_brightness\" name=\"led_brightness\" value=\""+String(led_brightness)+"\" required><br>";
  p += "<input type=\"submit\" value=\"Submit\">";
  p += "</form>";
  p += "</body><p>Setting up and configuration:";
  p += "1. Attach any 5-meter LED strip from the box to the wooden thermometer. Double-sided duct tape is fine, just duct tape too. Just make it reversible.</br>";
  p += "2. Count how many LEDs from the tail (where all wires are) to the place where the sign 0C is on the thermometer. Put this value in <b>First LED num</b></br>";
  p += "3. Count how many LEDs to the end of the scale. Put this value in <b>Last LED num</b></br>";
  p += "4. Ask the sauna lead what the minimum temperature is considered good enough and put this in <b>Red zone from temp</b></br>";
  p += "5. Warm up the sauna to the minimum acceptable temperature by feeling. Go to this page and check the temperature sensor value. If it's less than 'Red zone from temp', put the delta between the numbers in <b>sensor correction</b></br>";
  p += "---</br>";
  p += "</p></html>";
  server.send(200, "text/html", p);
}

void handleNotFound() {
  if (captivePortal()) { 
    return;
  }
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");

  for (uint8_t i = 0; i < server.args(); i++) {
    message += String(F(" ")) + server.argName(i) + F(": ") + server.arg(i) + F("\n");
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(404, "text/plain", message);
}


void offLedStrip() {
  Serial.println("Reset LEDS");
  int max = led_count;
  for (int i=0; i<max; i++) {
    strip.setPixelColor(i, strip.Color(0,0,0));
  }
  strip.show();
}

void showTemp(float temp) {
  //Serial.print("Showing temp: ");
  //Serial.println(temp);
  
  float temp_diff = highest_temp - lowest_temp;
  //Serial.print("temp_diff: ");
  //Serial.println(temp_diff);
  int led_counts = top_led_num - bottom_led_num;
  //Serial.print("led_counts: ");
  //Serial.println(led_counts);

  int led_num_to_start_red = int(led_counts * ((temp_red - lowest_temp)/temp_diff))+bottom_led_num;
  //Serial.print("led_num_to_start_red: ");
  //Serial.println(led_num_to_start_red);

  int leds_to_turn = 0;
  if (temp>lowest_temp) {
    if (temp>=highest_temp) {
      leds_to_turn = led_counts;
    } else {
      leds_to_turn = int(led_counts * ((temp - lowest_temp)/temp_diff));
    }
    //Serial.print("leds_to_turn: ");
    //Serial.println(leds_to_turn);
    for (int i=0; i<led_count; i++) {
      if (i<bottom_led_num || i>leds_to_turn+bottom_led_num) {
        strip.setPixelColor(i, strip.Color(0,0,0));
      } else {
        uint32_t led_color = strip.Color(0,0,0);
        int color_element_val = int(255*led_brightness/100);
        if (i>=led_num_to_start_red) {
          led_color = strip.Color(color_element_val,0,0);
        } else {
          led_color = strip.Color(0,0,color_element_val);
        }
        strip.setPixelColor(i, led_color);
      }
    }
    strip.show();
  }
}

//Testing with 1/5 brightnesss
void testLEDStrip() {
  Serial.println("Testing LEDs");
  int max = led_count;
  for (int i=0; i<max; i++) {
    Serial.print("Red ");
    Serial.println(i);
    strip.setPixelColor(i,strip.Color(50,0,0));
    strip.show();
    delay(5);
  }
  for (int i=0; i<max; i++) {
    Serial.print("Green ");
    Serial.println(i);
    strip.setPixelColor(i,strip.Color(0,50,0));
    strip.show();
    delay(5);
  }
  for (int i=0; i<max; i++) {
    Serial.print("Blue ");
    Serial.println(i);
    strip.setPixelColor(i,strip.Color(0,0,50));
    strip.show();
    delay(5);
  }
  handleRoot();
}

void setVals() {
  bottom_led_num = server.arg("firstled").toInt();
  Serial.print("firstled:");
  Serial.println(bottom_led_num);

  top_led_num = server.arg("lastled").toInt();
  Serial.print("lastled:");
  Serial.println(top_led_num);

  sensore_correction = server.arg("sensore_correction").toInt();
  Serial.print("sensore_correction:");
  Serial.println(sensore_correction);

  temp_red = server.arg("temp_red").toInt();
  Serial.print("temp_red:");
  Serial.println(temp_red);

  led_count = server.arg("led_count").toInt();
  Serial.print("led_count:");
  Serial.println(led_count);

  led_brightness = server.arg("led_brightness").toInt();
  Serial.print("led_brightness:");
  Serial.println(led_brightness);


  configuration_array[0] = bottom_led_num;
  configuration_array[1] = top_led_num;
  configuration_array[2] = sensore_correction;
  configuration_array[3] = temp_red;
  configuration_array[4] = led_count;
  configuration_array[5] = led_brightness;


  writeArrayToEEPROM();

  handleRoot();
}

void writeArrayToEEPROM() {
  // Iterate over the array elements
  for (int i = 0; i < ARRAY_SIZE; i++) {
    // Calculate the EEPROM address for the current element
    int address = EEPROM_ADDRESS + i * sizeof(int);
    Serial.print("Writing element to adress");
    Serial.println(address);

    // Write the element to EEPROM
    EEPROM.put(address, configuration_array[i]);
    Serial.print("Writing element ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(configuration_array[i]);
    delay(10); // Delay for EEPROM write operation
  }
  
  EEPROM.commit(); // Save changes to EEPROM
  Serial.println("Array saved to EEPROM.");
}

void readArrayFromEEPROM() {
  // Read and print the array elements from EEPROM
  for (int i = 0; i < ARRAY_SIZE; i++) {
    // Calculate the EEPROM address for the current element
    int address = EEPROM_ADDRESS + i * sizeof(int);
    Serial.print("Reading element from adress");
    Serial.println(address);
    // Read the element from EEPROM
    int val = 0;
    EEPROM.get(address, val);
    configuration_array[i] = val;
    // Print the element
    Serial.print("Element ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(configuration_array[i]);
    delay(10);
  }

  bottom_led_num = configuration_array[0];
  top_led_num = configuration_array[1];
  sensore_correction = configuration_array[2];
  temp_red = configuration_array[3];
  led_count = configuration_array[4];
  led_brightness = configuration_array[5];
  
  Serial.println("Array read from EEPROM.");
}


void setup() {
  delay(1000);
  EEPROM.begin(512);
  Serial.begin(9600);
  readArrayFromEEPROM();

  sensors.begin();
 
  strip.begin();

  Serial.println();
  Serial.println("Configuring access point...");
  WiFi.softAPConfig(apIP, apIP, netMsk);
  // its an open WLAN access point without a password parameter
  WiFi.softAP(SSID, SSPASS);
  delay(1000);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  /* Setup the web server */
  server.on("/", handleRoot);
  server.on("/generate_204", handleRoot);
  server.on("/testled", testLEDStrip);
  server.on("/setvals", setVals);
  server.onNotFound(handleNotFound);
  server.begin(); // Web server start
  Serial.println("HTTP server started");

}


int update_loop_num = -1;

void loop() {
  if (update_loop_num<0 || update_loop_num>5) {
    update_loop_num = 0;
    sensors.requestTemperatures(); 
    temperature_C = sensors.getTempCByIndex(0);
    //Serial.print(temperatureC);
    //Serial.println("ºC");
    showTemp(temperature_C+sensore_correction);
  } else {
    update_loop_num++;
    //Serial.print(".");
  }

  // put your main code here, to run repeatedly:
  //DNS
  dnsServer.processNextRequest();
  //HTTP
  server.handleClient();
}