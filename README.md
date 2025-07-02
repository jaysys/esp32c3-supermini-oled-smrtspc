# ESP32-C3 Super Mini OLED

<div align="left">
  <img src="./smrtspc-debug-serial-and-oled-display-test/esp32c3-super-mini-oled.png" alt="ESP32-C3 Super Mini OLED" width="50%">
</div>

```cpp
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, 6, 5, U8X8_PIN_NONE);
```

### 1. `U8G2_SSD1306_128X64_NONAME_F_SW_I2C`

- **U8g2 클래스 템플릿**입니다.
- 각 파트 의미:

  - `U8G2_`: U8g2 라이브러리의 객체
  - `SSD1306`: 드라이버 칩 이름
  - `128X64`: 해상도 (픽셀 기준)
  - `NONAME`: 제조사 없이 흔히 사용되는 SSD1306 보드 (아두이노용 0.96" OLED 등)
  - `F`: **풀 프레임 버퍼** 사용 (화면 전체 버퍼를 메모리에 저장 → 더 부드러운 렌더링)
  - `SW_I2C`: **소프트웨어 I2C** 사용 (Wire.h 대신, 직접 GPIO로 SCL/SDA 신호 제어)

> "SSD1306 128x64 디스플레이를 소프트웨어 I2C 방식으로 제어하는 객체"입니다.

### 2. `u8g2(U8G2_R0, 6, 5, U8X8_PIN_NONE)`

- 생성자 인수 설명:

| 인수            | 설명                                                        |
| --------------- | ----------------------------------------------------------- |
| `U8G2_R0`       | 화면 회전 없음 (기본 방향)                                  |
| `6`             | SCL 핀 번호 (GPIO 6 사용)                                   |
| `5`             | SDA 핀 번호 (GPIO 5 사용)                                   |
| `U8X8_PIN_NONE` | 리셋 핀 없음 (디스플레이 리셋 핀 연결 안 함 또는 자동 리셋) |

> 📌 일반적으로 `SCL`, `SDA`는 사용 중인 보드의 핀에 맞게 조정해야 합니다.

## 사용 예시

```cpp
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

```

## 참고

- **SW_I2C**는 하드웨어 I2C (`Wire`) 대신 소프트웨어로 I2C를 구현 → 어떤 핀에도 자유롭게 연결 가능 (단, 속도 느림).
- **F** 버전은 메모리를 많이 쓰지만 텍스트/그래픽/애니메이션에 유리합니다.

  - 메모리 부족할 경우 `U8G2_SSD1306_128X64_NONAME_1_SW_I2C` (1페이지 버퍼)로 변경 가능

| 요소                                  | 설명                                             |
| ------------------------------------- | ------------------------------------------------ |
| `U8G2_SSD1306_128X64_NONAME_F_SW_I2C` | SSD1306 128x64 OLED 제어용 소프트웨어 I2C 클래스 |
| `U8G2_R0`                             | 화면 회전 없음                                   |
| `6`                                   | SCL 핀                                           |
| `5`                                   | SDA 핀                                           |
| `U8X8_PIN_NONE`                       | 리셋 핀 사용 안 함                               |
| 장점                                  | 어떤 핀도 I2C로 사용 가능                        |
| 단점                                  | 속도가 느림 (HW I2C보다)                         |
