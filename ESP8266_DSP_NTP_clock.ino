// original: https://github.com/mataslauzadis/ESP8266_Alarm_Clock
// about DST (Daylight saving time): https://en.wikipedia.org/wiki/Daylight_saving_time

// simple webcontrol for DST at NTP clock with ESP8266 by Nicu FLORICA (niq_ro)
// https://nicuflorica.blogspot.ro/
// http://arduinotehniq.blogspot.com/
// https://www.arduinotehniq.com/
// https://www.youtube.com/user/dj06ntm

#include <ESP8266WiFi.h> // To connect to a WiFi network
#include <DS3231.h> // For the DateTime object, originally was planning to use this RTC module: https://www.arduino.cc/reference/en/libraries/ds3231/
#include <NTPClient.h> // To get accurate time from a remote server
#include <WiFiUdp.h>  // Helper for NTPClient

#define WIFI_SSID "niq_ro"
#define WIFI_PASSWORD "beerforniq_ro"

#define TIMEZONE +2 // Define your timezone to have an accurate clock (hours with respect to GMT +2)
// "PST": -7 
// "MST": -6 
// "CST": -5 
// "EST": -4 
// "GMT": 0 

// WiFi
String ssid = WIFI_SSID;
String password = WIFI_PASSWORD;
WiFiServer server(80); // Set web server port number to 80

// Web Host
String header; // Variable to store the HTTP request
unsigned long currentTime = millis(); // Current time
unsigned long previousTime = 0; // Previous time
const long timeoutTime = 2000; // Define timeout time in milliseconds (example: 2000ms = 2s)

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Alarm State
// * DateTime objects cannot be modified after creation, so I get the current DateTime and use it to create the default 7AM alarm *
DateTime today = DateTime(timeClient.getEpochTime());
String oravara = "on"; // Default to having the alarm enabled


void setup() {
  Serial.begin(115200);
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Print local IP address
  Serial.println("\nWiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  timeClient.setTimeOffset(TIMEZONE*3600); // Offset time from the GMT standard
  server.begin(); // Start web server!
}

void loop(){
  timeClient.update(); // Update the latest time from the NTP server
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client is connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          if (currentLine.length() == 0) { // If the current line is blank, you got two newline characters in a row. That's the end of the client HTTP request, so send a response:
            client.println("HTTP/1.1 200 OK"); // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            client.println("Content-type:text/html"); // and a content-type so the client knows what's coming, then a blank line:
            client.println("Connection: close");
            client.println();
            
            if (header.indexOf("GET /vara/on") >= 0) { // If the user clicked the alarm's on button
              Serial.println("Daylight saving time (DST) was activated !");
              oravara = "on";
              timeClient.setTimeOffset((TIMEZONE+1)*3600); // Offset time from the GMT standard
            } 
            else if (header.indexOf("GET /vara/off") >= 0) { // If the user clicked the alarm's off button
              Serial.println("Daylight saving time (DST) was deactivated !");
              oravara = "off";
              timeClient.setTimeOffset((TIMEZONE+0)*3600); // Offset time from the GMT standard
            }

            else if (header.indexOf("GET /time") >= 0) { // If the user submitted the time input form
              // Strip the data from the GET request
              int index = header.indexOf("GET /time");
              String timeData = header.substring(index + 15, index + 22);
           
              Serial.println(timeData);
              // Update our alarm DateTime with the user selected time, using the current date.
              // Since we just compare the hours and minutes on each loop, I do not think the day or month matters.
              DateTime temp = DateTime(timeClient.getEpochTime()); 
            }
            
            // Display the HTML web page
            // Head
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<link rel=\"stylesheet\" href=\"//stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css\">"); // Bootstrap
            client.println("</head>");
            
            // Body
            client.println("<body>");
            client.println("<h1 class=\"text-center mt-3\"NTP / DSP Clock</h1>"); // Title

            // Current Time
            client.print("<h1 class=\"text-center\">"); 
            client.print(timeClient.getFormattedTime());
            client.println("</h1>");
            
            
            // Display current state, and ON/OFF buttons for Alarm  
            client.println("<h2 class=\"text-center\">Daylight Saving Time - " + oravara + "</h2>");
            if (oravara=="off") {
              client.println("<p class=\"text-center\"><a href=\"/vara/on\"><button class=\"btn btn-sm btn-danger\">ON</button></a></p>");
            }
            else {
              client.println("<p class=\"text-center\"><a href=\"/vara/off\"><button class=\"btn btn-success btn-sm\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            client.println(); // The HTTP response ends with another blank line
            break; // Break out of the while loop
            
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    
    header = ""; // Clear the header variable
    client.stop(); // Close the connection
    Serial.println("Client disconnected.");
    Serial.println("");
  }

if(oravara == "on")
timeClient.setTimeOffset((TIMEZONE+1)*3600); // Offset time from the GMT standard
else
timeClient.setTimeOffset(TIMEZONE*3600); // Offset time from the GMT standard

}  // end main loop
