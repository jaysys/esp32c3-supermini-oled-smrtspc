#include <U8g2lib.h>
#include <Wire.h>

#undef LED_BUILTIN
#define LED_BUILTIN 8
// U8g2 생성자: SH1106 드라이버, 128x64 내부 버퍼 사용.
// 핀 번호 6 (SCL), 5 (SDA)를 사용합니다.
// --> 이 핀들이 ESP32-C3 보드의 0.42인치 OLED에 실제 연결된 핀인지 확인해주세요.
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5); 

// 실제 OLED의 해상도
const int OLED_ACTUAL_WIDTH = 72;
const int OLED_ACTUAL_HEIGHT = 40;

// 화면 중앙에 매핑하기 위한 기본 오프셋 (U8g2 내부 128x64 버퍼 기준)
const int OLED_DISPLAY_X_OFFSET_BASE = (128 - OLED_ACTUAL_WIDTH) / 2; // 28
const int OLED_DISPLAY_Y_OFFSET_BASE = (64 - OLED_ACTUAL_HEIGHT) / 2; // 12

// ***** 중요: Y축 위치를 아래로 내리기 위한 추가 조정 값 *****
// 이 값을 조절하여 화면 중앙에 오도록 맞추세요.
// 0.42인치 OLED에서 15~20 정도의 값이 필요할 수 있습니다.
const int Y_OFFSET_ADJUSTMENT = 14; // <-- 이 값을 조정하세요!

// 사각형의 현재 너비와 높이
int currentRectWidth = 10;
int currentRectHeight = 10;

// 사각형 크기 변화 방향 (1: 증가, -1: 감소)
int widthChangeDirection = 1;
int heightChangeDirection = 1;

// 사각형 크기 변화 속도 조절 (딜레이 값, ms)
const int RECT_UPDATE_DELAY_MS = 50; 

void setup(void) {
    pinMode(LED_BUILTIN, OUTPUT);
     Serial.begin(115200); 
     Serial.println("--- 0.42 inch OLED Dynamic Frame Centering Test ---");
     Serial.print("OLED Connected on SDA=");
     Serial.print(5);
     Serial.print(", SCL=");
     Serial.println(6);

     delay(1000); // 보드 초기화 대기

     u8g2.begin();
     u8g2.setBusClock(400000); 
     u8g2.setContrast(255); 

     u8g2.setFont(u8g2_font_6x10_tf); 

     Serial.println("OLED Initialized. Adjusting Y-offset for centering...");
     Serial.println("------------------------------------");
}

void loop(void) {

  digitalWrite(LED_BUILTIN, LOW);
  delay(50);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(150);


    u8g2.clearBuffer(); 

    // 사각형 및 텍스트의 실제 시작 X, Y 좌표 계산
    // 베이스 오프셋에 추가 조정 값을 더합니다.
    int effectiveDisplayYOffset = OLED_DISPLAY_Y_OFFSET_BASE + Y_OFFSET_ADJUSTMENT;

    // 사각형의 시작 위치 계산 (OLED 실제 가시 영역 내에서 중앙에 가깝게)
    int rectX = OLED_DISPLAY_X_OFFSET_BASE + (OLED_ACTUAL_WIDTH - currentRectWidth) / 2;
    int rectY = effectiveDisplayYOffset + (OLED_ACTUAL_HEIGHT - currentRectHeight) / 2;

    // 사각형 프레임 그리기
    u8g2.drawFrame(rectX, rectY, currentRectWidth, currentRectHeight);

    // OLED 화면에 W, H 값 텍스트 출력
    // Y 좌표도 조정된 effectiveDisplayYOffset을 기준으로 합니다.
    u8g2.setCursor(OLED_DISPLAY_X_OFFSET_BASE + 5, effectiveDisplayYOffset + 5); // 텍스트를 프레임 내 상단에 위치시키기 위해 Y+5
    u8g2.printf("W: %02d", currentRectWidth); 

    u8g2.setCursor(OLED_DISPLAY_X_OFFSET_BASE + 5, effectiveDisplayYOffset + 18); // 다음 줄 텍스트 Y 위치
    u8g2.printf("H: %02d", currentRectHeight);
    
    u8g2.sendBuffer(); 

    // 시리얼 모니터로 현재 사각형 크기 출력
    Serial.printf("Frame: W=%02d, H=%02d (Eff Y Off=%d)\n", currentRectWidth, currentRectHeight, effectiveDisplayYOffset);

    // 사각형 너비 변경 로직
    currentRectWidth += widthChangeDirection;
    if (currentRectWidth >= OLED_ACTUAL_WIDTH - 2 || currentRectWidth <= 5) { 
        widthChangeDirection *= -1; 
    }

    // 사각형 높이 변경 로직
    currentRectHeight += heightChangeDirection;
    if (currentRectHeight >= OLED_ACTUAL_HEIGHT - 2 || currentRectHeight <= 5) { 
        heightChangeDirection *= -1; 
    }

    delay(RECT_UPDATE_DELAY_MS); 
}