#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "miniDisplayManager.h"

// Display update delay
const int RECT_UPDATE_DELAY_MS = 50;

// Forward declarations
void connectToWiFi();
void updateRectangleSize();
void drawDynamicRectangle();
void printDebug(int rectX, int rectY);
void blinkLED(int times = 1, int delayMs = 100);
bool sendDataToServer(float value1, float value2);

// Rectangle animation variables
int currentRectWidth = 10;
int currentRectHeight = 10;
int widthChangeDirection = 1;
int heightChangeDirection = 1;

// WiFi credentials
const char* ssid = "12345678";
const char* password = "12345678";

// Server details
const char* serverName = "http://192.168.123.111:5003/api/data";
const int serverPort = 5003;

// Global variables
float value1 = 0.0;
float value2 = 0.0;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 3000;  // Send data every 3 seconds

#undef LED_BUILTIN
#define LED_BUILTIN 8

// DisplayManager is now a singleton, no need to instantiate

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);

    // Initialize display
    Display.begin();
    Display.setFont(FONT_SMALL);
    
    // Show initial message
    Display.display2Lines("Initializing...", "Connecting to WiFi");
    
    // Connect to WiFi
    connectToWiFi();
    
    // Show connection status
    if (WiFi.status() == WL_CONNECTED) {
        Display.display2Lines("WiFi Connected", WiFi.localIP().toString().c_str());
    } else {
        Display.display2Lines("WiFi Failed!", "Check Credentials");
    }
    
    delay(2000);
    Serial.println("OLED Initialized. Adjusting Y-offset for centering...");
}

// Update rectangle size for animation
void updateRectangleSize() {
    // Update width
    currentRectWidth += 2 * widthChangeDirection;
    if (currentRectWidth > Display.width || currentRectWidth < 10) {
        widthChangeDirection *= -1;
        currentRectWidth += 2 * widthChangeDirection;
    }
    
    // Update height
    currentRectHeight += 2 * heightChangeDirection;
    if (currentRectHeight > Display.height || currentRectHeight < 10) {
        heightChangeDirection *= -1;
        currentRectHeight += 2 * heightChangeDirection;
    }
}

// Print actual display coordinates shown on OLED
void printDebug(int rectX, int rectY) {
    Serial.print("Display - X: ");
    Serial.print(rectX);
    Serial.print(", Y: ");
    Serial.print(rectY);
    Serial.print(", W: ");
    Serial.print(currentRectWidth);
    Serial.print(", H: ");
    Serial.println(currentRectHeight);
}

// Draw rectangle animation
void drawDynamicRectangle() {
    // Calculate rectangle position (centered relative to visible area)
    int rectX = (Display.width - currentRectWidth) / 2;
    int rectY = (Display.height - currentRectHeight) / 2;
    
    // Clear and prepare display
    Display.clear();
    
    // Prepare and display text
    char line1[16], line2[16], line3[16], line4[16];
    snprintf(line1, sizeof(line2), "X:%d Y:%d", rectX, rectY);
    snprintf(line2, sizeof(line1), "W:%d H:%d", currentRectWidth, currentRectHeight);
    snprintf(line3, sizeof(line3), "IP: %s", WiFi.localIP().toString().c_str());
    snprintf(line4, sizeof(line4), "V1:%.1f V2:%.1f", value1, value2);

    // Print debug info (convert to absolute coordinates for debug output)
    printDebug(rectX, rectY);    

    // Display text using display manager
    // Display.display4Lines(line1, line2, line3, line4);
    Display.display2Lines(line1, line2);
    
    // Draw rectangle (after text to ensure it's on top)
    Display.drawRect(rectX, rectY, currentRectWidth, currentRectHeight);
    
    // Update display
    Display.update();
    
    // Update rectangle size for next frame
    updateRectangleSize();

    
    // Small delay for animation
    delay(RECT_UPDATE_DELAY_MS);
}

// LED control function
void blinkLED(int times, int delayMs) {
    for(int i = 0; i < times; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_BUILTIN, LOW);
        if(i < times - 1) delay(delayMs);
    }
}

// Send data to server
bool sendDataToServer(float value1, float value2) {
    if(WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return false;
    }
    
    WiFiClient client;
    HTTPClient http;
    bool success = false;
    
    // Create JSON document
    StaticJsonDocument<200> doc;
    doc["sensor"] = "esp32";
    doc["value1"] = value1;
    doc["value2"] = value2;
    
    // Serialize JSON to string
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Configure HTTP client
    if (http.begin(client, serverName)) {
        http.addHeader("Content-Type", "application/json");
        
        // Send HTTP POST request
        int httpResponseCode = http.POST(jsonString);
        
        // Check response
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.print("Server response (");
            Serial.print(httpResponseCode);
            Serial.print("): ");
            Serial.println(response);
            blinkLED(2, 50);  // Double blink on success
            success = true;
        } else {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
            blinkLED(5, 50);  // Fast blink 5 times on failure
        }
        
        http.end();
    } else {
        Serial.println("Failed to connect to server");
        blinkLED(5, 50);  // Fast blink 5 times on failure
    }
    
    return success;
}

void loop() {
    drawDynamicRectangle();
    
    // Send data to server periodically
    if (millis() - lastTime > timerDelay) {
        if (WiFi.status() == WL_CONNECTED) {
            // Update values before sending
            value1 = random(100) / 10.0f;
            value2 = random(100) / 10.0f;
            
            if (sendDataToServer(value1, value2)) {
                lastTime = millis();
            }
        } else {
            // Try to reconnect if WiFi is down
            connectToWiFi();
        }
    }
}

void connectToWiFi() {
    Serial.print("Connecting to WiFi");
    
    // Show connection status on display
    Display.clear();
    Display.printCenter("Connecting to WiFi...", 15);
    Display.update();
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {  // Try for 10 seconds
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi");
        Serial.print("IP address: ");
        String ip = WiFi.localIP().toString();
        Serial.println(ip);
        
        // Show connection success on display
        Display.clear();
        Display.printCenter("WiFi Connected", 11);
        Display.printCenter(ip.c_str(), 22);
        Display.update();
        
        delay(2000);  // Show message for 2 seconds
    } else {
        Serial.println("\nFailed to connect to WiFi");
        
        // Show connection failure on display
        Display.clear();
        Display.printCenter("WiFi Failed!", 11);
        Display.printCenter("Check Credentials", 22);
        Display.update();
        
        delay(2000);  // Show message for 2 seconds
    }
}
