#ifndef MINI_DISPLAY_MANAGER_H
#define MINI_DISPLAY_MANAGER_H

#include <U8g2lib.h>
#include <vector>
#include <string>

// Text alignment options
enum TextAlign { 
    ALIGN_LEFT, 
    ALIGN_CENTER, 
    ALIGN_RIGHT 
};

// Font size options
enum FontSize {
    FONT_SMALL,
    FONT_MEDIUM,
    FONT_LARGE
};

class DisplayManager {
public:
    // Singleton instance
    static DisplayManager& getInstance() {
        static DisplayManager instance;
        return instance;
    }

    // Display control
    void begin();
    void clear();
    void update();
    
    // Text display functions
    void setFont(FontSize size);
    void print(const char* text, int x, int y, TextAlign align = ALIGN_LEFT);
    void printCenter(const char* text, int y);
    void printRight(const char* text, int y);
    
    // Multi-line display
    void displayText(const std::vector<std::string>& lines, TextAlign align = ALIGN_LEFT);
    void display2Lines(const char* line1, const char* line2, TextAlign align = ALIGN_LEFT);
    void display4Lines(const char* line1, const char* line2 = "", 
                      const char* line3 = "", const char* line4 = "", 
                      TextAlign align = ALIGN_LEFT);
    
    // Graphics functions
    void drawFrame(int x, int y, int w, int h);
    void drawRect(int x, int y, int w, int h, bool filled = false);
    void drawHLine(int x, int y, int w);
    void drawVLine(int x, int y, int h);
    
    // Display properties
    static constexpr int width = 72;
    static constexpr int height = 40;
    static constexpr int xOffset = 28;
    static constexpr int yOffset = 24;
    
    // Get text width for the current font
    int getTextWidth(const char* text);
    int getFontHeight();

private:
    DisplayManager();
    ~DisplayManager() = default;
    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;
    
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
    FontSize currentFontSize;
    
    void applyFont();
    int calculateAlignedX(const char* text, int x, int maxWidth, TextAlign align);
};

// Global instance for easier access
inline DisplayManager& Display = DisplayManager::getInstance();

#endif
