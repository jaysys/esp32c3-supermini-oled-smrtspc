#ifndef MAQUEEN_WHEEL_MANAGER_H
#define MAQUEEN_WHEEL_MANAGER_H

#include <Arduino.h>
#include <Wire.h>

// 모터 상태 열거형
enum class MotorState {
    STOP,
    FORWARD,
    BACKWARD,
    TURN_LEFT,
    TURN_RIGHT,
    ROTATE_LEFT,
    ROTATE_RIGHT
};

class MaqueenWheelManager {
public:
    // 싱글톤 인스턴스 반환
    static MaqueenWheelManager& getInstance() {
        static MaqueenWheelManager instance;
        return instance;
    }

    // 초기화
    void begin(uint8_t i2cAddress = 0x10, uint8_t sdaPin = 6, uint8_t sclPin = 5);
    
    // 기본 제어
    void stop();
    void forward(uint8_t speed);
    void backward(uint8_t speed);
    void turnLeft(uint8_t speed);
    void turnRight(uint8_t speed);
    void rotateLeft(uint8_t speed);
    void rotateRight(uint8_t speed);
    
    // 상태 기반 제어
    void setState(MotorState state, uint8_t speed);
    MotorState getState() const;
    
    // 모터 속도 설정
    void setSpeed(uint8_t speed);
    uint8_t getSpeed() const;
    
    // I2C 주소 설정
    void setI2CAddress(uint8_t address);
    
private:
    // 생성자/소멸자 private로 선언하여 싱글톤 패턴 유지
    MaqueenWheelManager() = default;
    ~MaqueenWheelManager() = default;
    MaqueenWheelManager(const MaqueenWheelManager&) = delete;
    MaqueenWheelManager& operator=(const MaqueenWheelManager&) = delete;
    
    // I2C 명령 전송
    void sendMotorCommand(uint8_t motor, uint8_t dir, uint8_t speed);
    
    // 모터 상태
    MotorState currentState;
    uint8_t currentSpeed;
    uint8_t i2cAddress;
    bool isInitialized;
};

// 전역 인스턴스 참조
inline MaqueenWheelManager& Maqueen = MaqueenWheelManager::getInstance();

#endif
