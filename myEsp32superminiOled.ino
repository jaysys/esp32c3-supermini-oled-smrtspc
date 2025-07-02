#include <Arduino.h>
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

void initDisplay() {
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
}

enum TextAlign {
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT
};

void drawText(const char* text, int x, int y, TextAlign align = ALIGN_LEFT) {
  u8g2.clearBuffer();
  int xPos = x;
  if (align != ALIGN_LEFT) {
    int16_t textWidth = u8g2.getStrWidth(text);
    if (align == ALIGN_CENTER) {
      xPos = x - textWidth / 2;
    } else if (align == ALIGN_RIGHT) {
      xPos = x - textWidth;
    }
  }
  u8g2.drawStr(xPos, y, text);
  u8g2.sendBuffer();
}

void setup(void) {
  Serial.begin(115200);
  while (!Serial) {
    delay(1000);
  }
  Serial.println("\nSetup starting...");
  initDisplay();
}

void loop(void) {
  if (!displayInitialized) {
    delay(5000);
    return;
  }

  // 좌측 정렬 테스트
  drawText("Hello, Left!", xOffset, yOffset + 10);
  Serial.println("Display buffer updated. (Left)");
  delay(2000);

  // 중앙 정렬 테스트
  drawText("Yo, Cntr!", xOffset + screenWidth / 2, yOffset + 20, ALIGN_CENTER);
  Serial.println("Display buffer updated. (Center)");
  delay(2000);

  // 우측 정렬 테스트
  drawText("Hi, Rght!", xOffset + screenWidth, yOffset + 30, ALIGN_RIGHT);
  Serial.println("Display buffer updated. (Right)");
  delay(2000);

    // 좌측 정렬 테스트
  drawText("Ma, Left!", xOffset, yOffset + 40, ALIGN_LEFT);
  Serial.println("Display buffer updated. (Left)");
  delay(2000);
}
