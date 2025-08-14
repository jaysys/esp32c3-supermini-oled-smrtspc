#include <U8g2lib.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// 가변 저항이 연결된 아날로그 핀 번호
const int POT_PIN = 0; // GPIO0 (ADC1_CH0)에 연결

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

// 디스플레이 데이터 구조체
typedef struct {
  int potValue;
  int percentage;
} DisplayData_t;

// 디스플레이 업데이트 큐
QueueHandle_t displayQueue;

// 디스플레이 해상도
#define TFT_WIDTH  76
#define TFT_HEIGHT 284

// ST7789 객체 생성 (소프트웨어 SPI 모드)
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// U8g2 하드웨어 I2C 생성자 (reset 핀 없음)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// 미니 디스플레이, 실제 스크린 크기와 오프셋 (중앙 정렬용)
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

void initSPI_TFTDisplay() {
  Serial.println("Initializing SPI TFT display...");
  
  // 백라이트 핀 설정
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, LOW); // 백라이트 켜기
  
  // SPI 초기화 및 고속 설정
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);  // MISO는 사용하지 않음
  SPI.setFrequency(SPI_FREQ_HZ);
  
  // TFT 디스플레이 초기화
  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setRotation(1); // 가로 방향
  
  // SPI 트랜잭션 설정 (고속 모드)
  SPI.beginTransaction(SPISettings(SPI_FREQ_HZ, MSBFIRST, SPI_MODE0));
  
  // 화면 초기화
  tft.invertDisplay(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  
  // 초기 메시지 표시
  tft.setCursor(10, 10);
  tft.println("SPI Display");
  tft.setCursor(10, 20);
  tft.println("Initialized!");
  
  Serial.println("SPI TFT Display setup complete.");
  delay(2000);
  
  // 스크린을 클리어하는 함수
  tft.fillScreen(ST77XX_BLACK);
  delay(1000);
}

void updateSPIDisplay(int potValue, int percentage) {
  static int lastBarWidth = -1;
  static int lastPotValue = -1;
  static int lastPercentage = -1;
  
  // 이전과 값이 같으면 업데이트 생략
  if (potValue == lastPotValue && percentage == lastPercentage) {
    return;
  }

  // 제목 표시 (처음 한 번만)
  if (lastPotValue == -1) {
    tft.setCursor(10, 10);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.println("Potentiometer:");
  }
  
  // 막대 그래프 업데이트
  int barWidth = map(percentage, 0, 100, 0, TFT_WIDTH - 20);
  if (barWidth != lastBarWidth) {
    // 이전 막대 지우기
    if (lastBarWidth > 0) {
      tft.fillRect(10, 30, TFT_WIDTH - 20, 20, ST77XX_BLACK);
    }
    // 새 막대 그리기
    if (barWidth > 0) {
      tft.fillRect(10, 30, barWidth, 20, ST77XX_BLUE);
    }
    tft.drawRect(10, 30, TFT_WIDTH - 20, 20, ST77XX_WHITE);
    lastBarWidth = barWidth;
  }

  // 값 텍스트 업데이트
  if (lastPotValue != potValue) {
    tft.fillRect(50, 55, 50, 8, ST77XX_BLACK); // 이전 값 지우기
    tft.setCursor(10, 55);
    tft.print("Value: ");
    tft.print(potValue);
    lastPotValue = potValue;
  }

  // 퍼센트 텍스트 업데이트
  if (lastPercentage != percentage) {
    tft.fillRect(60, 65, 40, 8, ST77XX_BLACK); // 이전 퍼센트 지우기
    tft.setCursor(10, 65);
    tft.print("Percent: ");
    tft.print(percentage);
    tft.print("%");
    lastPercentage = percentage;
  }
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

// TFT 디스플레이 태스크
void tftTask(void *parameter) {
  DisplayData_t displayData;
  static int lastPotValue = -1;
  static int lastPercentage = -1;
  
  while(1) {
    // 큐에서 데이터를 받아옴
    if (xQueueReceive(displayQueue, &displayData, portMAX_DELAY) == pdPASS) {
      // 값이 변경된 경우에만 TFT 업데이트
      if (displayData.potValue != lastPotValue || displayData.percentage != lastPercentage) {
        lastPotValue = displayData.potValue;
        lastPercentage = displayData.percentage;
        updateSPIDisplay(displayData.potValue, displayData.percentage);
      }
    }
    vTaskDelay(1 / portTICK_PERIOD_MS); // 매우 짧은 딜레이
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting ESP32-C3 with FreeRTOS and dual displays");
  
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
    4096,          // 스택 크기
    NULL,          // 파라미터
    1,             // 우선순위
    &tftTaskHandle, // 태스크 핸들
    0              // 코어 0에서 실행
  );
  
  // 메인 루프는 가변 저항 값을 읽어서 큐에 전달
  Serial.println("FreeRTOS tasks started");
}

void loop() {
  static unsigned long lastReadTime = 0;
  static int lastPotValue = -1;
  
  // 100ms마다 가변 저항 값을 읽음
  if (millis() - lastReadTime > 100) {
    lastReadTime = millis();
    
    // 가변 저항 값을 읽음 (0~4095)
    int potValue = analogRead(POT_PIN);
    
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
