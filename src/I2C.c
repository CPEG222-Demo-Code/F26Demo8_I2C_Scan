#include "I2C.h"
#include "stm32f446xx.h"

#define PIN_WIRE_SDA          9
#define PIN_WIRE_SCL          8
#define PORT_WIRE           GPIOB
#define BMI270_ADDR          (0x68 << 1) // Shifted for I2C write/read bit operations

void I2C1_Init(void) {
    // Enable GPIOB and I2C1 clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    // Set PB8/PB9 to AF4 (I2C1)
    PORT_WIRE->MODER &= ~((0x3 << (PIN_WIRE_SCL*2)) | (0x3 << (PIN_WIRE_SDA*2)));
    PORT_WIRE->MODER |=  (0x2 << (PIN_WIRE_SCL*2)) | (0x2 << (PIN_WIRE_SDA*2));
    PORT_WIRE->AFR[1] &= ~((0xF << ((PIN_WIRE_SCL-8)*4)) | (0xF << ((PIN_WIRE_SDA-8)*4)));
    PORT_WIRE->AFR[1] |=  (0x4 << ((PIN_WIRE_SCL -8)*4)) | (0x4 << ((PIN_WIRE_SDA-8)*4));
    PORT_WIRE->OTYPER |= (1 << PIN_WIRE_SCL) | (1 << PIN_WIRE_SDA); // Open-drain
    PORT_WIRE->OSPEEDR |= (0x3 << (PIN_WIRE_SCL*2)) | (0x3 << (PIN_WIRE_SDA*2)); // High speed

    // I2C1 software reset
    I2C1->CR1 |= I2C_CR1_SWRST;
    for (volatile int i = 0; i < 1000; i++);
    I2C1->CR1 &= ~I2C_CR1_SWRST;
    for (volatile int i = 0; i < 1000; i++);

    // I2C1 config for 100kHz, 16MHz PCLK1
    I2C1->CR1 = 0;
    I2C1->CR2 = 16; // 16 MHz peripheral clock
    I2C1->CCR = 80; // 100kHz standard mode
    I2C1->TRISE = 17; // Maximum rise time
    I2C1->CR1 |= I2C_CR1_PE; //enable I2C1
}
uint8_t I2C1_IsDeviceReady(uint8_t addr) {
    // Generate start condition
    I2C1->CR1 |= I2C_CR1_START | I2C_CR1_ACK; // must include ACK for single byte read
    
    // Wait for start bit to be sent
    uint32_t timeout = 10000;
    while (!(I2C1->SR1 & I2C_SR1_SB) && timeout--);
    if (timeout == 0) return 0; // Timeout
    
    // Send device address
    I2C1->DR = addr << 1; // Write address
    
    // Wait for address to be sent
    timeout = 10000;
    while (!(I2C1->SR1 & I2C_SR1_ADDR) && !(I2C1->SR1 & I2C_SR1_AF) && timeout--);
    
    if (I2C1->SR1 & I2C_SR1_AF) {
        // NACK received - device not present
        I2C1->SR1 &= ~I2C_SR1_AF; // Clear AF flag
        I2C1->CR1 |= I2C_CR1_STOP;
        return 0;
    }
    
    if (I2C1->SR1 & I2C_SR1_ADDR) {
        // ACK received - device present
        (void)I2C1->SR1; (void)I2C1->SR2; // Clear ADDR flag
        I2C1->CR1 |= I2C_CR1_STOP;
        return 1;
    }
    
    // Timeout
    I2C1->CR1 |= I2C_CR1_STOP;
    return 0;
}
void I2C_Start(void) {
    I2C1->CR1 |= I2C_CR1_START;
    while (!(I2C1->SR1 & I2C_SR1_SB)); // Wait for Start condition generated
}

void I2C_Write(uint8_t data) {
    I2C1->DR = data;
    while (!(I2C1->SR1 & I2C_SR1_TXE)); // Wait for byte transmission buffer empty
}

void I2C_Address(uint8_t addr, uint8_t direction) {
    I2C1->DR = addr | direction;
    while (!(I2C1->SR1 & I2C_SR1_ADDR)); // Wait for address acknowledgment match
    (void)I2C1->SR2; // Clear ADDR flag by reading SR2
}

void I2C_Stop(void) {
    I2C1->CR1 |= I2C_CR1_STOP;
}

void I2C_WriteRegister(uint8_t reg, uint8_t data) {
    I2C_Start();
    I2C_Address(BMI270_ADDR, 0); // Write Mode
    I2C_Write(reg);
    I2C_Write(data);
    while (!(I2C1->SR1 & I2C_SR1_BTF)); // Ensure last byte fully shifted out
    I2C_Stop();
}

void I2C_WriteBurstRegister(uint8_t reg, const uint8_t *data, uint8_t length) {
    I2C_Start();
    I2C_Address(BMI270_ADDR, 0); // Write Mode
    I2C_Write(reg);
    for (uint8_t i = 0; i < length; i++) {
        I2C_Write(data[i]);
    }
    while (!(I2C1->SR1 & I2C_SR1_BTF)); // Ensure final byte transmission completes
    I2C_Stop();
}

uint8_t I2C_ReadRegister(uint8_t reg) {
    uint8_t data;

    I2C_Start();
    I2C_Address(BMI270_ADDR, 0); // Write Mode to select register
    I2C_Write(reg);

    I2C_Start(); // Repeated Start condition

    // For a single-byte read, clear ACK before ADDR is cleared.
    I2C1->CR1 &= ~I2C_CR1_ACK;
    I2C1->DR = BMI270_ADDR | 1U;
    while (!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2;

    I2C_Stop();
    while (!(I2C1->SR1 & I2C_SR1_RXNE));
    data = I2C1->DR;

    // Restore ACK for subsequent multi-byte transactions.
    I2C1->CR1 |= I2C_CR1_ACK;

    return data;
}

void I2C_ReadBurst(uint8_t start_reg, uint8_t *buffer, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
        buffer[i] = I2C_ReadRegister((uint8_t)(start_reg + i));
    }
}
