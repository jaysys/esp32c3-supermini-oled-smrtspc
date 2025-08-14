// 하나는 72*40 supermini i2c oled display 
// 그리고, 한개 더! 76*284 와이드 spi tft display 붙여놓고
// 가변저항 1개를 읽어들여서 supermini에 출력해보는 예제소스입니다.
// esp32 c3 supermini oled display 기본 붙어있던것에 spi tft display를 하나 더 연결했슴.


#include <U8g2lib.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// 가변 저항이 연결된 아날로그 핀 번호
const int POT_PIN = 0; // GPIO0 (ADC1_CH0)에 연결

// ST7789 SPI 디스플레이 핀 설정
#define TFT_RST   1    // RST (Reset)
#define TFT_DC    2    // DC (Data/Command)
#define TFT_CS    7    // CS (Chip Select)
#define TFT_MOSI  8    // SDA (Software SPI MOSI)
#define TFT_SCLK  9    // SCL (Software SPI Clock)
#define TFT_BLK   10   // BLK (Backlight)

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

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting ESP32-C3 with I2C OLED and SPI TFT Displays");
  
  // SPI 디스플레이 초기화
  initSPI_TFTDisplay();
  
  // I2C OLED 디스플레이 초기화
  initI2C_OLEDDisplay();
}

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
  
  // 백라이트 핀 설정 (LOW일 때 켜지는 타입)
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, LOW); // 백라이트 켜기, 내가 가진 oled supermini 보드에서는 LOW로 설정해야 백라이트 켜짐
  
  // 소프트웨어 SPI로 TFT 디스플레이 초기화
  tft.init(TFT_WIDTH, TFT_HEIGHT);
  tft.setRotation(1); // 가로 방향
  
  // 화면 초기화
  tft.invertDisplay(false);  // 색상 반전 비활성화
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
}

void updateSPIDisplay(int potValue, int percentage) {
  // SPI 디스플레이 업데이트
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 10);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Potentiometer:");
  
  // 백분율을 막대 그래프로 표시
  int barWidth = map(percentage, 0, 100, 0, TFT_WIDTH - 20);
  tft.fillRect(10, 30, barWidth, 20, ST77XX_BLUE);
  tft.drawRect(10, 30, TFT_WIDTH - 20, 20, ST77XX_WHITE);
  
  // 텍스트로 값 표시
  tft.setCursor(10, 60);
  tft.print("Value: ");
  tft.println(potValue);
  tft.print("Percent: ");
  tft.print(percentage);
  tft.println("%");
}

void loop() {
  static unsigned long lastDisplayUpdate = 0;
  static int lastPotValue = -1;  // 값이 변경되었는지 확인하기 위한 변수
  
  // 디스플레이를 100ms마다 업데이트합니다.
  if (millis() - lastDisplayUpdate > 100) {
    lastDisplayUpdate = millis();
    
    // 가변 저항 값을 읽습니다 (0~4095).
    int potValue = analogRead(POT_PIN);
    
    // 가변 저항 값을 0~100%로 변환합니다.
    int percentage = map(potValue, 0, 4095, 0, 100);
    
    // 값이 변경된 경우에만 디스플레이 업데이트
    if (potValue != lastPotValue) {
      lastPotValue = potValue;
      
      // 출력 문자열을 만듭니다.
      char valueStr[32];
      snprintf(valueStr, sizeof(valueStr), "Value: %d", potValue);
      
      char percentStr[32];
      snprintf(percentStr, sizeof(percentStr), "%d%%", percentage);
      
      // OLED 디스플레이 업데이트
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(xOffset, yOffset + 10, "Potentiometer:");
      
      u8g2.setFont(u8g2_font_10x20_tr);
      u8g2.drawStr(xOffset, yOffset + 28, percentStr);
      
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(xOffset, yOffset + 40, valueStr);
      u8g2.sendBuffer();
      
      // SPI 디스플레이 업데이트
      // updateSPIDisplay(potValue, percentage);
    }
  }
}
