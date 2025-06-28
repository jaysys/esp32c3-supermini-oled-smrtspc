#include "miniDisplayManager.h"
#include <Wire.h>

// Initialize static member variables
constexpr int DisplayManager::width;
constexpr int DisplayManager::height;
constexpr int DisplayManager::xOffset;
constexpr int DisplayManager::yOffset;

// Font definitions
static const uint8_t* FONT_SMALL_FONT = u8g2_font_6x10_tf;       // 6x10 픽셀 폰트 (더 작음)
static const uint8_t* FONT_MEDIUM_FONT = u8g2_font_ncenB08_tr;   // 8px 폰트
static const uint8_t* FONT_LARGE_FONT = u8g2_font_ncenB10_tr;    // 10px 폰트

// Constructor
DisplayManager::DisplayManager() 
    : u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5),  // SCL=6, SDA=5
      currentFontSize(FONT_MEDIUM) {
}

void DisplayManager::begin() {
    // Initialize display
    u8g2.begin();
    
    // Configure display
    u8g2.setContrast(150);
    u8g2.setBusClock(100000);  // 100kHz I2C clock
    u8g2.setFontMode(1);       // Transparent mode
    
    // Apply default font
    applyFont();
    
    // Clear display
    clear();
    
    // Small delay for stability
    delay(50);
}

void DisplayManager::clear() {
    u8g2.clearBuffer();
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 128, 64);
    u8g2.setDrawColor(0);
    u8g2.sendBuffer();
}

void DisplayManager::update() {
    u8g2.sendBuffer();
}

void DisplayManager::setFont(FontSize size) {
    if (currentFontSize != size) {
        currentFontSize = size;
        applyFont();
    }
}

void DisplayManager::print(const char* text, int x, int y, TextAlign align) {
    if (!text) return;
    
    int drawX = xOffset + x;
    if (align != ALIGN_LEFT) {
        int textWidth = getTextWidth(text);
        if (align == ALIGN_CENTER) {
            drawX -= textWidth / 2;
        } else { // ALIGN_RIGHT
            drawX -= textWidth;
        }
    }
    
    u8g2.setCursor(drawX, yOffset + y + getFontHeight() - 2);
    u8g2.print(text);
}

void DisplayManager::printCenter(const char* text, int y) {
    print(text, width / 2, y, ALIGN_CENTER);
}

void DisplayManager::printRight(const char* text, int y) {
    print(text, width, y, ALIGN_RIGHT);
}

void DisplayManager::displayText(const std::vector<std::string>& lines, TextAlign align) {
    if (lines.empty()) return;
    
    clear();
    
    int fontHeight = getFontHeight();
    int lineSpacing = 1;  // Reduced from 2px to 1px for better fit
    int totalTextHeight = (fontHeight * lines.size()) + (lineSpacing * (lines.size() - 1));
    int startY = (height - totalTextHeight) / 2 + fontHeight - 2;  // Adjusted for better vertical centering
    
    // For 4 lines, adjust the starting position to prevent top/bottom cutoff
    if (lines.size() == 4) {
        // 더 작은 폰트를 사용하므로 여백을 더 줄일 수 있음
        startY = 1;
    }
    
    for (size_t i = 0; i < lines.size(); i++) {
        int y = startY + (i * (fontHeight + lineSpacing));
        print(lines[i].c_str(), 0, y, align);
    }
    
    update();
}

void DisplayManager::display2Lines(const char* line1, const char* line2, TextAlign align) {
    if (!line1 && !line2) return;
    
    clear();
    
    int fontHeight = getFontHeight();
    int lineSpacing = 1;
    int startY = 0;  // Start from the very top
    
    // Print first line
    if (line1) {
        print(line1, 0, startY, align);
    }
    
    // Print second line with spacing
    if (line2) {
        print(line2, 0, startY + fontHeight + lineSpacing, align);
    }
    
    update();
}

void DisplayManager::display4Lines(const char* line1, const char* line2, 
                                 const char* line3, const char* line4, TextAlign align) {
    std::vector<std::string> lines;
    if (line1) lines.push_back(line1);
    if (line2) lines.push_back(line2);
    if (line3) lines.push_back(line3);
    if (line4) lines.push_back(line4);
    displayText(lines, align);
}

// Graphics functions
void DisplayManager::drawFrame(int x, int y, int w, int h) {
    u8g2.drawFrame(xOffset + x, yOffset + y, w, h);
}

void DisplayManager::drawRect(int x, int y, int w, int h, bool filled) {
    if (filled) {
        u8g2.drawBox(xOffset + x, yOffset + y, w, h);
    } else {
        u8g2.drawFrame(xOffset + x, yOffset + y, w, h);
    }
}

void DisplayManager::drawHLine(int x, int y, int w) {
    u8g2.drawHLine(xOffset + x, yOffset + y, w);
}

void DisplayManager::drawVLine(int x, int y, int h) {
    u8g2.drawVLine(xOffset + x, yOffset + y, h);
}

// Helper functions
void DisplayManager::applyFont() {
    switch (currentFontSize) {
        case FONT_SMALL:  u8g2.setFont(FONT_SMALL_FONT); break;
        case FONT_MEDIUM: u8g2.setFont(FONT_MEDIUM_FONT); break;
        case FONT_LARGE:  u8g2.setFont(FONT_LARGE_FONT); break;
    }
}

int DisplayManager::getTextWidth(const char* text) {
    return text ? u8g2.getStrWidth(text) : 0;
}

int DisplayManager::getFontHeight() {
    return u8g2.getAscent() - u8g2.getDescent();
}

int DisplayManager::calculateAlignedX(const char* text, int x, int maxWidth, TextAlign align) {
    if (!text) return x;
    
    int textWidth = getTextWidth(text);
    switch (align) {
        case ALIGN_CENTER: return x + (maxWidth - textWidth) / 2;
        case ALIGN_RIGHT:  return x + maxWidth - textWidth;
        default:           return x;  // ALIGN_LEFT
    }
}
