#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>

// Replace with your network credentials
const char* ssid = "Ak";
const char* password = "12345678";

// Your server details
const char* serverName = "http://13.201.118.241/sensor.php";

// DHT Sensor settings
#define DHTPIN D5
#define DHTTYPE DHT22  // Change to DHT22

DHT dht(DHTPIN, DHTTYPE);

// Onboard LED pin (usually D4 or D0 depending on the board)
#define LED_PIN LED_BUILTIN

// LDR pin
const int ldrPin = A0;  // Analog pin A0

// Room ID and additional details
const char* roomId = "room-204";  // Add your room ID
const char* designation = "Engineer";  // Replace with your designation
const char* gender = "Male";  // Replace with gender (Male/Female/Other)
int age = 25;  // Replace with your age
const char* module = "Module-A";  // Replace with your module name

void setup() {
  Serial.begin(115200);
  delay(10);

  // Initialize DHT sensor
  dht.begin();

  // Set LED pin as output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Turn off LED

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi");
}

void loop() {
  // Read temperature and humidity
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(2000); // Wait for sensor to recover
    return;
  }

  int ldrValue = analogRead(ldrPin);
  int brightnessPercent = map(ldrValue, 0, 1023, 100, 0);

  // Print the values to the Serial Monitor
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" *C\t");
  Serial.print("Brightness Level: ");
  Serial.print(brightnessPercent);
  Serial.println(" %");

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    // Increase the timeout duration
    http.setTimeout(35000); // 35 seconds

    // Prepare the URL for the HTTP request
    Serial.println("Connecting to server...");
    if (http.begin(client, serverName)) {
      Serial.println("Connected to server");
      
      // Specify content type as form data
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Prepare the data to be sent in the form body
      String postData = "temperature=" + String(temperature) + 
                        "&humidity=" + String(humidity) + 
                        "&brightness=" + String(brightnessPercent) + 
                        "&roomid=" + String(roomId) +
                        "&designation=" + String(designation) +
                        "&gender=" + String(gender) + 
                        "&age=" + String(age) + 
                        "&module=" + String(module);

      // Send the HTTP POST request with the data in the form body
      Serial.println("Sending data...");
      int httpResponseCode = http.POST(postData);

      if (httpResponseCode == 301) {
        // Get the new URL from the response headers
        String newUrl = http.header("Location");
        Serial.print("Redirected to: ");
        Serial.println(newUrl);
        // Send a new request to the new URL
        http.begin(client, newUrl);
        httpResponseCode = http.POST(postData);
      }

      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        // Turn on the LED if the response code is positive
        digitalWrite(LED_PIN, HIGH);
      } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        Serial.print("Error occurred: ");
        Serial.println(http.errorToString(httpResponseCode).c_str());

        // Turn off the LED if the response code is not positive
        digitalWrite(LED_PIN, LOW);
      }

      // Free resources
      http.end();
    } else {
      Serial.println("Connection to server failed");
      digitalWrite(LED_PIN, LOW);
    }
  } else {
    Serial.println("WiFi Disconnected");
    // Turn off the LED if WiFi is disconnected
    digitalWrite(LED_PIN, LOW);
  }

  // Break the long delay into smaller intervals to prevent watchdog resets
  for (int i = 0; i < 240; i++) { // 240 * 500ms = 120000ms (2 minutes)
    delay(500);
    // Feed the watchdog timer
    yield();
  }
}
