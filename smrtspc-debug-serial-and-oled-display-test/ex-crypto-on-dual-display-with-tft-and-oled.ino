// freeRTOS 사용하여 2개의 독립적인 타스크로 동작
// 두개의 디스플레이 사용
// 크립토 가격을 조회해서 tft display에 표출하는 예제
// esp32-c3, tft and mini_oled display

#include <U8g2lib.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <WiFi.h>
#include <HTTPClient.h>

// WiFi 설정
const char* ssid = "1111111";
const char* password = "000000000";

// Upbit API 설정
const char* btcApiUrl = "https://api.upbit.com/v1/ticker?markets=KRW-BTC";
const char* ethApiUrl = "https://api.upbit.com/v1/ticker?markets=KRW-ETH";
const char* linkApiUrl = "https://api.upbit.com/v1/ticker?markets=KRW-LINK";
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 60000; // 1분마다 업데이트

// 암호화폐 가격 구조체
typedef struct {
  float btcPrice;
  float ethPrice;
  float linkPrice;
  char formattedDate[11];  // YYYY/MM/DD + null
  char formattedTime[9];   // HH:MM:SS + null
  bool dataValid;
} CryptoPrices_t;

CryptoPrices_t cryptoPrices = {0, 0, 0, false};

// 디스플레이 데이터 구조체
typedef struct {
  int potValue;
  int percentage;
} DisplayData_t;

// 디스플레이 업데이트 큐
QueueHandle_t displayQueue;

// ST7789 SPI 디스플레이 핀 설정
#define TFT_RST   1    // RST (Reset)
#define TFT_DC    2    // DC (Data/Command)
#define TFT_CS    7    // CS (Chip Select)
#define TFT_MOSI  8    // SDA (Software SPI MOSI)
#define TFT_SCLK  9    // SCL (Software SPI Clock)
#define TFT_BLK   10   // BLK (Backlight)

// SPI 클럭 속도 설정 (40MHz)
#define SPI_FREQ_HZ 40000000

// 태스크 핸들
TaskHandle_t oledTaskHandle = NULL;
TaskHandle_t tftTaskHandle = NULL;

// TFT 디스플레이 해상도 (90도 회전 고려)
#define TFT_WIDTH  284  // 실제 화면의 가로 길이 (90도 회전 시 세로가 됨)
#define TFT_HEIGHT 76   // 실제 화면의 세로 길이 (90도 회전 시 가로가 됨)

// ST7789 객체 생성 (소프트웨어 SPI 모드)
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// U8g2 하드웨어 I2C 생성자 (reset 핀 없음)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// OLED 디스플레이, 실제 스크린 크기와 오프셋 (중앙 정렬용)
constexpr int screenWidth = 72;
constexpr int screenHeight = 40;
constexpr int xOffset = 28;
constexpr int yOffset = 24;

// 디스플레이 초기화 성공 여부를 저장할 변수
bool displayInitialized = false;

void initI2C_OLEDDisplay() {
  // I2C 통신 시작 (SDA=5, SCL=6)
  Wire.begin(5, 6); 

  Serial.println("Initializing U8g2 display...");
  if (u8g2.begin()) {
    displayInitialized = true;
  } else {
    Serial.println("ERROR: U8g2 display initialization failed. Please check wiring and I2C pins.");
    while (1); 
  }
  
  // 디스플레이 설정
  u8g2.setContrast(150);
  u8g2.setBusClock(100000);
  u8g2.setFontMode(1);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setDrawColor(1);
  u8g2.setFontDirection(0);
  u8g2.setFontRefHeightExtendedText();
  Serial.println("OLED Display setup complete.");
}

// WiFi 연결 함수
void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
}

// 암호화폐 가격 가져오기 (Upbit API 사용)
bool fetchCryptoPrices() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, attempting to connect...");
    connectToWiFi();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Failed to connect to WiFi");
      return false;
    }
  }

  bool btcSuccess = false;
  bool ethSuccess = false;
  bool linkSuccess = false;
  
  // Helper function to parse price from JSON
  auto parsePriceAndTime = [](const String& json) -> float {
    float price = 0.0f;
    
    // 가격 파싱
    int tradePricePos = json.indexOf("\"trade_price\":");
    if (tradePricePos != -1) {
      int valueStart = json.indexOf(':', tradePricePos) + 1;
      int valueEnd = json.indexOf(',', valueStart);
      if (valueEnd == -1) valueEnd = json.indexOf('}', valueStart);
      
      if (valueEnd != -1) {
        String priceStr = json.substring(valueStart, valueEnd);
        priceStr.trim();
        price = priceStr.toFloat();
      }
    }
    
    return price;
  };

  // BTC 가격 가져오기
  Serial.println("\nFetching BTC price...");
  HTTPClient http;
  http.begin(btcApiUrl);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.print("BTC API Response: ");
    Serial.println(payload);
    
    // BTC API 응답에서 trade_date와 trade_time_kst 파싱
    String dateStr = "";
    String timeStr = "";
    
    // trade_date 파싱 (예: "20250815")
    int datePos = payload.indexOf("\"trade_date\":");
    if (datePos != -1) {
      int dateStart = payload.indexOf('"', datePos + 13) + 1;
      int dateEnd = payload.indexOf('"', dateStart);
      if (dateEnd != -1) {
        dateStr = payload.substring(dateStart, dateEnd);
      }
    }
    
    // trade_time_kst 파싱 (예: "164005")
    int timePos = payload.indexOf("\"trade_time_kst\":");
    if (timePos != -1) {
      int timeStart = payload.indexOf('"', timePos + 16) + 1;
      int timeEnd = payload.indexOf('"', timeStart);
      if (timeEnd != -1) {
        timeStr = payload.substring(timeStart, timeEnd);
      }
    }
    
    // 날짜와 시간을 조합하여 저장 (예: "2025/08/15"와 "16:40:05")
    if (dateStr.length() == 8 && timeStr.length() == 6) {
      // 날짜 포맷팅 (YYYY/MM/DD)
      snprintf(cryptoPrices.formattedDate, sizeof(cryptoPrices.formattedDate), 
              "%s/%s/%s", 
              dateStr.substring(0, 4).c_str(),
              dateStr.substring(4, 6).c_str(),
              dateStr.substring(6, 8).c_str());
      
      // 시간 포맷팅 (HH:MM:SS)
      snprintf(cryptoPrices.formattedTime, sizeof(cryptoPrices.formattedTime),
              "%s:%s:%s",
              timeStr.substring(0, 2).c_str(),
              timeStr.substring(2, 4).c_str(),
              timeStr.substring(4, 6).c_str());
      
      Serial.print("Using trade time (KST): ");
      Serial.print(cryptoPrices.formattedDate);
      Serial.print(" ");
      Serial.println(cryptoPrices.formattedTime);
    }
    
    cryptoPrices.btcPrice = parsePriceAndTime(payload);
    if (cryptoPrices.btcPrice > 0) {
      btcSuccess = true;
      Serial.print("Parsed BTC Price: ");
      Serial.print(cryptoPrices.btcPrice, 2);
      Serial.println(" KRW");
    } else {
      Serial.println("Failed to parse BTC price");
    }
  } else {
    Serial.print("BTC API request failed, error: ");
    Serial.println(httpCode);
  }
  http.end();
  
  // ETH 가격 가져오기
  Serial.println("\nFetching ETH price...");
  http.begin(ethApiUrl);
  httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.print("ETH API Response: ");
    Serial.println(payload);
    
    cryptoPrices.ethPrice = parsePriceAndTime(payload);
    if (cryptoPrices.ethPrice > 0) {
      ethSuccess = true;
      Serial.print("Parsed ETH Price: ");
      Serial.print(cryptoPrices.ethPrice, 2);
      Serial.println(" KRW");
    } else {
      Serial.println("Failed to parse ETH price");
    }
  } else {
    Serial.print("ETH API request failed, error: ");
    Serial.println(httpCode);
  }
  http.end();
  
  // LINK 가격 가져오기
  Serial.println("\nFetching LINK price...");
  http.begin(linkApiUrl);
  httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.print("LINK API Response: ");
    Serial.println(payload);
    
    cryptoPrices.linkPrice = parsePriceAndTime(payload);
    if (cryptoPrices.linkPrice > 0) {
      linkSuccess = true;
      Serial.print("Parsed LINK Price: ");
      Serial.print(cryptoPrices.linkPrice, 2);
      Serial.println(" KRW");
    } else {
      Serial.println("Failed to parse LINK price");
    }
  } else {
    Serial.print("LINK API request failed, error: ");
    Serial.println(httpCode);
  }
  http.end();
  
  cryptoPrices.dataValid = (btcSuccess && ethSuccess && linkSuccess);
  if (cryptoPrices.dataValid) {
    Serial.println("\nSuccessfully updated BTC, ETH, and LINK prices");
  } else {
    Serial.println("\nFailed to update one or both cryptocurrency prices");
  }
  
  return cryptoPrices.dataValid;
}

void initSPI_TFTDisplay() {
  Serial.println("Initializing SPI TFT display...");
  
  // 백라이트 핀 설정
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, LOW); // 백라이트 켜기
  
  // SPI 초기화 및 고속 설정
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);  // MISO는 사용하지 않음
  SPI.setFrequency(SPI_FREQ_HZ);
  
  // TFT 디스플레이 초기화 (90도 회전 적용)
  tft.init(TFT_HEIGHT, TFT_WIDTH);  // swap width/height for rotation
  tft.setRotation(1);  // 90도 회전 (0=0°, 1=90°, 2=180°, 3=270°)
  
  // SPI 트랜잭션 설정 (고속 모드)
  SPI.beginTransaction(SPISettings(SPI_FREQ_HZ, MSBFIRST, SPI_MODE0));
  
  // 화면 초기화
  tft.invertDisplay(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  
  Serial.println("SPI TFT Display setup complete.");
}

// OLED 디스플레이 태스크
void oledTask(void *parameter) {
  DisplayData_t displayData;
  
  while(1) {
    // 큐에서 데이터를 받아옴
    if (xQueueReceive(displayQueue, &displayData, portMAX_DELAY) == pdPASS) {
      // OLED 디스플레이 업데이트
      char valueStr[32];
      snprintf(valueStr, sizeof(valueStr), "Value: %d", displayData.potValue);
      
      char percentStr[32];
      snprintf(percentStr, sizeof(percentStr), "%d%%", displayData.percentage);
      
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(xOffset, yOffset + 10, "Potentiometer:");
      
      u8g2.setFont(u8g2_font_10x20_tr);
      u8g2.drawStr(xOffset, yOffset + 28, percentStr);
      
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(xOffset, yOffset + 40, valueStr);
      u8g2.sendBuffer();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // 짧은 딜레이
  }
}

// 숫자를 9자리 우측 정렬, 천 단위 구분자(,)가 있는 문자열로 변환하는 함수
String formatNumber(float number) {
  String numStr = String((long)number);
  String result = "";
  int len = numStr.length();
  
  // 천 단위 구분자(,) 추가
  for (int i = 0; i < len; i++) {
    if ((len - i) % 3 == 0 && i != 0) {
      result += ",";
    }
    result += numStr[i];
  }
  
  // 9자리로 맞추기 위해 앞에 공백 추가 (우측 정렬 효과)
  int totalLength = result.length();
  while (totalLength < 11) {  // 최대 9자리 숫자 + 2개의 콤마 = 11자리
    result = " " + result;
    totalLength++;
  }
  
  return result;
}

// TFT 디스플레이 태스크
void tftTask(void *parameter) {
  unsigned long lastUpdateAttempt = -updateInterval;  // 시작 즉시 조회하도록 설정
  
  // TFT 초기 화면 설정 (90도 회전 고려)
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 10);
  tft.setTextSize(1);
  tft.println("Crypto Prices");
  tft.drawLine(5, 20, TFT_WIDTH - 5, 20, ST77XX_WHITE);  // 가로선 길이 조정
  
  while(1) {
    unsigned long currentTime = millis();
    
    // 1분마다 가격 업데이트 시도
    if (currentTime - lastUpdateAttempt >= updateInterval) {
      lastUpdateAttempt = currentTime;
      
      if (fetchCryptoPrices()) {
        lastUpdateTime = currentTime;
      }
    }
    
    // 화면 업데이트 (90도 회전 고려)
    // 전체 화면을 지우지 않고 각 영역만 업데이트
    tft.fillRect(0, 25, TFT_WIDTH, 70, ST77XX_BLACK);  // 가격 표시 영역만 지우기 (LINK 추가로 높이 증가)
    
    if (cryptoPrices.dataValid) {
      // 비트코인 가격 표시 (노란색)
      tft.setTextColor(ST77XX_YELLOW);
      tft.setCursor(10, 25);
      tft.print("BTC: ");
      tft.print(formatNumber(cryptoPrices.btcPrice));
      tft.println(" KRW");

      tft.setTextColor(ST77XX_RED);  
      // 이더리움 가격 표시
      tft.setCursor(10, 35);
      tft.print("ETH: ");
      tft.print(formatNumber(cryptoPrices.ethPrice));
      tft.println(" KRW");
      
      tft.setTextColor(ST77XX_GREEN);  
      // 링크 가격 표시
      tft.setCursor(10, 45);
      tft.print("LINK:");
      tft.print(formatNumber(cryptoPrices.linkPrice));
      tft.println(" KRW");

      tft.setTextColor(ST77XX_WHITE);  // 다른 텍스트는 흰색으로 되돌리기
      // API 응답에서 파싱한 날짜와 시간을 그대로 사용
      // 이미 KST 시간이므로 추가 변환 없이 표시
      
      // 마지막 업데이트 시간 표시 (90도 회전 고려)
      tft.fillRect(0, TFT_HEIGHT - 10, TFT_WIDTH, TFT_HEIGHT, ST77XX_BLACK);  // 하단 영역 지우기
      tft.setCursor(10, TFT_HEIGHT - 10);  // 하단 여백 조정
      tft.print("Updated: ");
      tft.print(cryptoPrices.formattedDate);
      tft.print(" ");
      tft.print(cryptoPrices.formattedTime);
      tft.print("   ");  // 이전 텍스트 지우기
      
    } else {
      tft.setCursor(10, 35);
      tft.print("Connecting");
      // 연결 애니메이션을 위한 점 추가
      static int dotCount = 0;
      dotCount = (dotCount + 1) % 4;
      for (int i = 0; i < dotCount; i++) {
        tft.print(".");
      }
      tft.print("   ");  // 이전 텍스트 지우기
    }
    
    vTaskDelay(30000 / portTICK_PERIOD_MS); // 30초 대기
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting ESP32-C3 with dual displays");
  
  // WiFi 연결
  connectToWiFi();
  
  // 디스플레이 초기화
  initSPI_TFTDisplay();
  initI2C_OLEDDisplay();
  
  // 디스플레이 업데이트 큐 생성 (10개 항목 저장 가능)
  displayQueue = xQueueCreate(10, sizeof(DisplayData_t));
  
  if (displayQueue == NULL) {
    Serial.println("Error creating the queue");
    while(1); // 큐 생성 실패 시 정지
  }
  
  // 태스크 생성
  xTaskCreatePinnedToCore(
    oledTask,      // 태스크 함수
    "OLED_Task",   // 태스크 이름
    4096,          // 스택 크기
    NULL,          // 파라미터
    1,             // 우선순위
    &oledTaskHandle, // 태스크 핸들
    0              // 코어 0에서 실행
  );
  
  xTaskCreatePinnedToCore(
    tftTask,       // 태스크 함수
    "TFT_Task",    // 태스크 이름
    8192,          // 스택 크기 (네트워크 작업을 위해 더 큰 스택 필요)
    NULL,          // 파라미터
    1,             // 우선순위
    &tftTaskHandle, // 태스크 핸들
    0              // 코어 0에서 실행
  );
  
  Serial.println("FreeRTOS tasks started");
}

void loop() {
  static unsigned long lastReadTime = 0;
  static int lastPotValue = -1;
  
  // 100ms마다 가변 저항 값을 읽음
  if (millis() - lastReadTime > 100) {
    lastReadTime = millis();
    
    // 가변 저항 값을 읽음 (0~4095)
    int potValue = analogRead(0);
    
    // 값이 변경된 경우에만 처리
    if (abs(potValue - lastPotValue) > 2) { // 노이즈 필터링을 위한 임계값
      lastPotValue = potValue;
      
      // 가변 저항 값을 0~100%로 변환
      int percentage = map(potValue, 0, 4095, 0, 100);
      
      // 디스플레이 데이터 구조체 생성
      DisplayData_t displayData = {
        .potValue = potValue,
        .percentage = percentage
      };
      
      // 큐에 데이터 전송 (non-blocking)
      if (xQueueSend(displayQueue, &displayData, 10 / portTICK_PERIOD_MS) != pdPASS) {
        Serial.println("Queue full, dropped display update");
      }
    }
  }
  
  // 다른 태스크에 CPU 양보
  vTaskDelay(1);
}
