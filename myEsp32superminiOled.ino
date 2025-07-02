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

'''
1. 전체 OLED 해상도와 실제 사용 영역
 전체 OLED 해상도: 128 × 64 (U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);)
 실제 사용 영역: 72 × 40 (부착되어 있는 실제 oled 72*40 픽셀 제품)
2. 중앙 정렬을 위한 오프셋 계산 공식
 중앙 정렬을 위해서는 오프셋 = (전체 크기 - 실제 사용 크기) / 2 로 계산합니다.
 xOffset 계산
  (128 - 72) / 2 = 56 / 2 = 28
 yOffset 계산
  (64 - 40) / 2 = 24 / 2 = 12
3. 결론
 수학적으로 yOffset은 12가 맞습니다. 만약 yOffset이 24로 되어 있다면, 실제 사용 영역이 아래쪽으로 치우쳐서 표시됩니다. 중앙에 맞추려면 yOffset = 12로 해야 합니다.
4. 요약
 yOffset = 12가 중앙 정렬에 맞는 값입니다.
 yOffset = 24는 중앙보다 아래로 치우친 값입니다.
---
그런데, 실제로 yOffset이 24로 설정되어 있는데도 화면이 “잘 출력된다면”, 다음과 같은 여러 가지 이유가 있을 수 있습니다:
1. 커스텀 보드의 OLED 실장 위치
 보드 설계상 OLED가 PCB의 중앙이 아니라, 위쪽이나 아래쪽으로 치우쳐 실장되어 있을 수 있습니다. 이 경우, 논리적 중앙(12)보다 실제로는 24 정도로 내려야 화면이 물리적으로 “중앙”에 보입니다.
2. OLED 패널의 표시 영역이 실제 해상도와 다를 수 있음
 일부 저가형 OLED 패널은 128×64 전체를 다 표시하지 않고, 위/아래에 보이지 않는 영역이 있을 수 있습니다. 이 경우, 중앙 정렬 공식이 실제 표시 영역과 맞지 않을 수 있습니다.
3. 사용자 경험(UX) 또는 디자인상의 이유
 텍스트나 그래픽이 화면의 “체감상 중앙”에 오도록, 의도적으로 yOffset을 조정할 수 있습니다. 폰트 크기, 아이콘, 기타 UI 요소와의 균형을 맞추기 위해서입니다.
4. 펌웨어/라이브러리의 내부 오프셋
 일부 라이브러리(특히 U8g2)는 디스플레이 초기화 시 내부적으로 오프셋을 적용할 수 있습니다. 이로 인해 실제로는 (0,0)이 화면의 맨 위가 아닐 수 있습니다.
'''
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
