// 
// 부팅하고 mini oled 화면에 맥주소의 끝 4자리를 출력해준다.
// MAX_DEVICES 에 등록해 놓은 4개의 디바이스에게 랜덤값은 esp-now로 보내준다.
// 

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <U8g2lib.h>
#include <Wire.h>

// U8g2 하드웨어 I2C 생성자 (reset 핀 없음)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// 실제 스크린 크기와 오프셋 (중앙 정렬용)
constexpr int screenWidth = 72;
constexpr int screenHeight = 40;
constexpr int xOffset = 28;
constexpr int yOffset = 24;

// 디스플레이 초기화 성공 여부를 저장할 변수
bool displayInitialized = false;

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  char a[32];
  int b;
  float c;
  bool d;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// List of known MAC addresses (excluding self)
const int MAX_DEVICES = 3;  // Maximum number of other devices
uint8_t knownDevices[MAX_DEVICES][6] = {  
  {0x19, 0x20, 0xBA, 0x93, 0x8D, 0xE4},  //  19/10
  {0x19, 0x20, 0xBA, 0x93, 0xF0, 0xA8},   //  19/10
  {0x19, 0x20, 0xBA, 0x93, 0xE1, 0x58}   //  19/10
};
//맥주소는 맞는 것으로 해야죠!!!!


// Self MAC address
uint8_t selfMac[6];
bool isInitialized = false;  // Flag to track if ESP-NOW is initialized

// Callback when data is sent
void printWithMac(const char* message) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           selfMac[0], selfMac[1], selfMac[2],
           selfMac[3], selfMac[4], selfMac[5]);
  Serial.print("[");
  Serial.print(macStr);
  Serial.print("] ");
  Serial.println(message);
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "Packet sent to: %s, Status: %s", 
           macStr, 
           status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
  printWithMac(buffer);
}

// Callback when data is received
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (!info || !info->src_addr) {
    printWithMac("Error: Invalid receive info");
    return;
  }
  
  const uint8_t *mac_addr = info->src_addr;
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2],
           mac_addr[3], mac_addr[4], mac_addr[5]);
  
  // Check if sender is in our known devices list
  bool isKnownDevice = false;
  for (int i = 0; i < MAX_DEVICES; i++) {
    if (memcmp(mac_addr, knownDevices[i], 6) == 0) {
      isKnownDevice = true;
      break;
    }
  }
  
  if (!isKnownDevice) {
    char msg[100];
    snprintf(msg, sizeof(msg), "Received from unknown device: %s", macStr);
    printWithMac(msg);
    return;
  }
  
  char buffer[200];
  snprintf(buffer, sizeof(buffer), "Packet received from: %s, Bytes: %d", macStr, len);
  printWithMac(buffer);
  
  memcpy(&myData, incomingData, sizeof(myData));
  
  snprintf(buffer, sizeof(buffer), "From: %s | Data - Char: %s, Int: %d, Float: %.2f, Bool: %s",
           macStr, myData.a, myData.b, myData.c, myData.d ? "true" : "false");
  printWithMac(buffer);
  printWithMac("");  // Empty line for better readability
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(1000);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initialize I2C and display with custom settings
  Wire.begin(5, 6); // SDA=5, SCL=6
  Serial.println("Initializing U8g2 display...");
  if (u8g2.begin()) {
    Serial.println("SUCCESS: U8g2 display initialized.");
    displayInitialized = true;
  } else {
    Serial.println("ERROR: U8g2 display initialization failed. Please check wiring and I2C pins.");
    displayInitialized = false;
    return;
  }
  delay(1000);
  u8g2.setContrast(150);
  u8g2.setBusClock(100000);
  u8g2.setFontMode(1);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setDrawColor(1);
  u8g2.setFontDirection(0);
  u8g2.setFontRefHeightExtendedText();
  Serial.println("Display setup complete.");
  
  if (displayInitialized) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 15, "Starting...");
    u8g2.sendBuffer();
  }


  // Get and store self MAC address
  WiFi.macAddress(selfMac);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           selfMac[0], selfMac[1], selfMac[2], selfMac[3], selfMac[4], selfMac[5]);
  
  // Display last two digits of MAC on OLED during setup
  if (displayInitialized) {
    char displayMac[10];
    snprintf(displayMac, sizeof(displayMac), "%02X%02X", selfMac[4], selfMac[5]);
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_10x20_tr);  // Smaller font
    
    // Center the text
    int16_t textWidth = u8g2.getStrWidth(displayMac);
    int16_t xPos = (128 - textWidth) / 2;  // Center horizontally
    
    u8g2.drawStr(xPos, 35+12, displayMac);
    u8g2.sendBuffer();
  }
  
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "ESP32-C3 starting up. MAC: %s", macStr);
  Serial.print("[");
  Serial.print(macStr);
  Serial.print("] ");
  Serial.println(buffer);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    printWithMac("Error initializing ESP-NOW");
    return;
  }

  // Register callbacks
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  
  // Initialize WiFi channel
  int32_t channel = 1;  // Use a fixed channel (1-13)
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  
  char msg[50];
  snprintf(msg, sizeof(msg), "WiFi channel set to %d", channel);
  printWithMac(msg);

  // Add known devices as peers
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  peerInfo.channel = channel;  // Use the same channel
  peerInfo.encrypt = false;
  
  for (int i = 0; i < MAX_DEVICES; i++) {
    // Skip adding self as peer
    if (memcmp(knownDevices[i], selfMac, 6) == 0) {
      continue;
    }
    
    memcpy(peerInfo.peer_addr, knownDevices[i], 6);
    
    // First try to modify existing peer
    esp_err_t result = esp_now_mod_peer(&peerInfo);
    if (result != ESP_OK) {
      // If modify fails, try to add new peer
      result = esp_now_add_peer(&peerInfo);
    }
    
    if (result == ESP_OK) {
      char msg[100];
      snprintf(msg, sizeof(msg), "Peer configured: %02X:%02X:%02X:%02X:%02X:%02X (Channel: %d)",
               knownDevices[i][0], knownDevices[i][1], knownDevices[i][2],
               knownDevices[i][3], knownDevices[i][4], knownDevices[i][5], channel);
      printWithMac(msg);
    } else {
      char errMsg[100];
      snprintf(errMsg, sizeof(errMsg), "Failed to configure peer %02X:%02X:%02X:%02X:%02X:%02X. Error: %d",
               knownDevices[i][0], knownDevices[i][1], knownDevices[i][2],
               knownDevices[i][3], knownDevices[i][4], knownDevices[i][5], result);
      printWithMac(errMsg);
    }
  }
  
  isInitialized = true;
  printWithMac("ESP-NOW initialized. Ready to send/receive messages.");
}

// Display update is now only done during setup

void loop() {
  static unsigned long lastMsgTime = 0;

  
  // Send a message every 5 seconds
  if (millis() - lastMsgTime > 5000 && isInitialized) {
    // Set values to send
    strcpy(myData.a, "Hello from ESP32-C3");
    myData.b = random(1, 100);
    myData.c = 1.2;
    myData.d = true;
    
    bool messageSent = false;
    
    // Send to all known devices (except self)
    for (int i = 0; i < MAX_DEVICES; i++) {
      if (memcmp(knownDevices[i], selfMac, 6) != 0) {  // Don't send to self
        // Add small delay between sends
        delay(10);
        
        // Print debug info before sending
        char debugMsg[150];
        snprintf(debugMsg, sizeof(debugMsg), "Attempting to send to %02X:%02X:%02X:%02X:%02X:%02X...",
                 knownDevices[i][0], knownDevices[i][1], knownDevices[i][2],
                 knownDevices[i][3], knownDevices[i][4], knownDevices[i][5]);
        printWithMac(debugMsg);
        
        // Send the message
        esp_err_t result = esp_now_send(knownDevices[i], (uint8_t *)&myData, sizeof(myData));
        
        // Print result
        if (result == ESP_OK) {
          messageSent = true;
          char msg[100];
          snprintf(msg, sizeof(msg), "Message sent to %02X:%02X:%02X:%02X:%02X:%02X | Status: Queued",
                   knownDevices[i][0], knownDevices[i][1], knownDevices[i][2],
                   knownDevices[i][3], knownDevices[i][4], knownDevices[i][5]);
          printWithMac(msg);
        } else {
          char errorMsg[150];
          snprintf(errorMsg, sizeof(errorMsg), "Error sending to %02X:%02X:%02X:%02X:%02X:%02X | Code: %d | %s",
                   knownDevices[i][0], knownDevices[i][1], knownDevices[i][2],
                   knownDevices[i][3], knownDevices[i][4], knownDevices[i][5], 
                   result, esp_err_to_name(result));
          printWithMac(errorMsg);
        }
      }
    }
    
    if (!messageSent) {
      printWithMac("No messages were sent. Check peer connections.");
    }
    
    lastMsgTime = millis();
  }
}

