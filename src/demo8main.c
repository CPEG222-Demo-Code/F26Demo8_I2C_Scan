// ****************************************************************
// * Authors: T. LUM and R. MARTIN
// * CPEG222 Demo8 I2C Scan, 10/15/26
// * NucleoF466RE CMSIS STM32F4xx example
// * Scan the I2C bus for connected devices and display their addresses
// * on the OLED.
// ****************************************************************
#include "stm32f4xx.h" // CMSIS STM32F4xx header file
#include "I2C.h" // Include the I2C header file for I2C functions
#include "OLED.h"// Include the OLED header file for display functions
#include "SysClock180MHz.h"// Include the system clock configuration header file for 180 MHz clock setup
#include <stdio.h> // For snprintf

char buffer[40];  // Buffer for formatted strings

void delay_ms(uint32_t ms) { // Simple delay function that loops for the specified number of milliseconds w/180 MHz clock
    for(uint32_t i = 0; i < ms * 22500; i++){
            __NOP();
    }
}

int main(void) {
    SystemClock_Config(); // Set system clock to 180 MHz
    OLED_Init();// Initialize the OLED display
    OLED_Clear();// Clear the OLED display
    OLED_Print(0, 0, "CPEG222-F26 I2C Scan Demo");// Print the title on the first line of the OLED display
    OLED_Print(1, 0, "  I2C Initialization");// Print the initialization message on the second line of the OLED display
    I2C1_Init(); // Initialize I2C1 peripheral  
    uint8_t devices_found = 0;
    OLED_Print(2, 0, "  Scanning I2C bus...");// Print the scanning message on the third line of the OLED display
    for (uint8_t addr = 1; addr < 127; addr++) {// Iterate through all possible 7-bit I2C addresses
        if (I2C1_IsDeviceReady(addr)) {// Check if a device responds at the current address
            snprintf(buffer, sizeof(buffer), "  Device at 0x%02X  ", addr); // Format the device address into the buffer
            OLED_Print(3 + devices_found, 0, buffer);// Print the device address on the OLED display
            devices_found++;// Increment the count of devices found 
        }
        delay_ms(50);
    }
    OLED_Print(3+devices_found, 0, "  Scan complete.");// Print the scan complete message on the OLED display    
    if (devices_found == 0) {
        OLED_Print(4+devices_found, 0, "No I2C devices found.");
    } else {
        snprintf(buffer, sizeof(buffer), "Found %d I2C devices.  ", devices_found);
        OLED_Print(4+devices_found, 0, buffer);
    }  
    while (1) {// Infinite loop to keep the program running
        // Main loop
    }
}