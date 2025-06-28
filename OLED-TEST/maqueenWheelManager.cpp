#include "maqueenWheelManager.h"

// 모터 제어 명령어
#define MOTOR_STOP 0x02
#define MOTOR_MOVE 0x00
#define MOTOR_LEFT 0x00
#define MOTOR_RIGHT 0x02

void MaqueenWheelManager::begin(uint8_t i2cAddress, uint8_t sdaPin, uint8_t sclPin) {
    this->i2cAddress = i2cAddress;
    Wire.begin(sdaPin, sclPin);
    currentState = MotorState::STOP;
    currentSpeed = 0;
    isInitialized = true;
    stop();  // 초기 정지
}

void MaqueenWheelManager::stop() {
    if (!isInitialized) return;
    
    sendMotorCommand(MOTOR_LEFT, MOTOR_STOP, 0);
    sendMotorCommand(MOTOR_RIGHT, MOTOR_STOP, 0);
    currentState = MotorState::STOP;
    currentSpeed = 0;
}

void MaqueenWheelManager::forward(uint8_t speed) {
    if (!isInitialized) return;
    
    sendMotorCommand(MOTOR_LEFT, MOTOR_MOVE, speed);
    sendMotorCommand(MOTOR_RIGHT, MOTOR_MOVE, speed);
    currentState = MotorState::FORWARD;
    currentSpeed = speed;
}

void MaqueenWheelManager::backward(uint8_t speed) {
    if (!isInitialized) return;
    
    sendMotorCommand(MOTOR_LEFT, MOTOR_MOVE | 0x01, speed);
    sendMotorCommand(MOTOR_RIGHT, MOTOR_MOVE | 0x01, speed);
    currentState = MotorState::BACKWARD;
    currentSpeed = speed;
}

void MaqueenWheelManager::turnLeft(uint8_t speed) {
    if (!isInitialized) return;
    
    sendMotorCommand(MOTOR_LEFT, MOTOR_MOVE, speed / 2);
    sendMotorCommand(MOTOR_RIGHT, MOTOR_MOVE, speed);
    currentState = MotorState::TURN_LEFT;
    currentSpeed = speed;
}

void MaqueenWheelManager::turnRight(uint8_t speed) {
    if (!isInitialized) return;
    
    sendMotorCommand(MOTOR_LEFT, MOTOR_MOVE, speed);
    sendMotorCommand(MOTOR_RIGHT, MOTOR_MOVE, speed / 2);
    currentState = MotorState::TURN_RIGHT;
    currentSpeed = speed;
}

void MaqueenWheelManager::rotateLeft(uint8_t speed) {
    if (!isInitialized) return;
    
    sendMotorCommand(MOTOR_LEFT, MOTOR_MOVE | 0x01, speed);
    sendMotorCommand(MOTOR_RIGHT, MOTOR_MOVE, speed);
    currentState = MotorState::ROTATE_LEFT;
    currentSpeed = speed;
}

void MaqueenWheelManager::rotateRight(uint8_t speed) {
    if (!isInitialized) return;
    
    sendMotorCommand(MOTOR_LEFT, MOTOR_MOVE, speed);
    sendMotorCommand(MOTOR_RIGHT, MOTOR_MOVE | 0x01, speed);
    currentState = MotorState::ROTATE_RIGHT;
    currentSpeed = speed;
}

void MaqueenWheelManager::setState(MotorState state, uint8_t speed) {
    if (!isInitialized) return;
    
    switch (state) {
        case MotorState::STOP:
            stop();
            break;
        case MotorState::FORWARD:
            forward(speed);
            break;
        case MotorState::BACKWARD:
            backward(speed);
            break;
        case MotorState::TURN_LEFT:
            turnLeft(speed);
            break;
        case MotorState::TURN_RIGHT:
            turnRight(speed);
            break;
        case MotorState::ROTATE_LEFT:
            rotateLeft(speed);
            break;
        case MotorState::ROTATE_RIGHT:
            rotateRight(speed);
            break;
    }
}

MotorState MaqueenWheelManager::getState() const {
    return currentState;
}

void MaqueenWheelManager::setSpeed(uint8_t speed) {
    if (!isInitialized || currentState == MotorState::STOP) return;
    
    currentSpeed = speed;
    // 현재 상태 유지하면서 속도만 업데이트
    setState(currentState, speed);
}

uint8_t MaqueenWheelManager::getSpeed() const {
    return currentSpeed;
}

void MaqueenWheelManager::setI2CAddress(uint8_t address) {
    if (address != i2cAddress) {
        i2cAddress = address;
        // 주소가 변경되면 모터 정지
        stop();
    }
}

void MaqueenWheelManager::sendMotorCommand(uint8_t motor, uint8_t dir, uint8_t speed) {
    Wire.beginTransmission(i2cAddress);
    Wire.write(motor);
    Wire.write(dir);
    Wire.write(speed);
    Wire.endTransmission();
}
