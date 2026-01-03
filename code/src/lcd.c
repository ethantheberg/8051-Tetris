/* ---------------------------------------------------------------------------------
 * Ethan Berg
 * ECEN 5613 - Fall 2025 - Prof. McClure
 * University of Colorado Boulder
 * Created Sat Nov 29 2025
 * --------------------------------------------------------------------------------
 * LCD Driver
   ---------------------------------------------------------------------------------*/

#include <at89c51ed2.h>       //also includes 8052.h and 8051.h
#include <mcs51reg.h>
#include <stdint.h>
#include <stdlib.h>
#include "lcd.h"

#define RST 0b11100010 // Reset
#define DSP_ON 0b10101111 // Display on
#define DSP_OFF 0b10101110 // Display off
#define ALL_ON 0b10100101 // All pixels on utility
#define ALL_OFF 0b10100100 // All pixels off utility
#define BIAS_LOW 0b10100010 // Contrast voltage bias low
#define BIAS_HIGH 0b10100011 // Contrast voltage bias low
#define V0_RATIO 0b00100000 // Set V0 ratio (set last 3 bits to ratio 0-7)
#define VOL_MODE 0b10000001 // Set volume; send value in next command
#define POWER_CONTROLLER 0b00101000 // Enable power circuits; last 3 bits control booster, voltage regulator, and voltage follower circuits, respectively
#define PAGE_ADDR 0b10110000 // Set page address, set last 4 bits to page 0-8
#define RMW 0b11100000 // Enable Read-Modify-Write mode
#define RMW_END 0b11101110 // Disable Read-Modify-Write mode 

// Bitpacked 4x6 font. Data for 2 characters is stored in each 8-bit integer. 
// 0123456789L TERIS!
const uint8_t symbols[5][9] = {
    {0b11100100, 0b11101110, 0b10101110, 0b11101110, 0b11101110, 0b10000000, 0b11101110, 0b11101110, 0b11101000},
    {0b10101100, 0b00100010, 0b10101000, 0b10000010, 0b10101010, 0b10000000, 0b01001000, 0b10100100, 0b10001000},
    {0b10100100, 0b11101110, 0b11101110, 0b11100100, 0b11101110, 0b10000000, 0b01001100, 0b11000100, 0b11101000},
    {0b10100100, 0b10000010, 0b00100010, 0b10100100, 0b10100010, 0b10000000, 0b01001000, 0b10100100, 0b00100000},
    {0b11101110, 0b11101110, 0b00101110, 0b11100100, 0b11101110, 0b11100000, 0b01001110, 0b10101110, 0b11101000},
};

// Extract data from bitpack
// c is character number, from 0 - 17
// " >> 4*((c+1)%2)" shifts number right by 4 if it's the left symbol in that 8-bit integer
uint8_t getSymbolData(uint8_t c, uint8_t row){ return 0b1111 & (symbols[row][c/2] >> 4*((c+1)%2)); }

// Delay function to add tolerance to LCD initialization
void delay_ms(uint8_t ms){
    for(uint8_t i = 0; i < ms; ++i){
        for(uint8_t j = 0; j < 128; ++j){} // trial and error value to get 1 ms delay
    }
}

void lcd_setPageAddr(uint8_t addr){
    if(addr >= 8) addr = 7;
    lcd_cmd = PAGE_ADDR | addr;
}

void lcd_setColumnAddr(uint8_t addr){
    lcd_cmd = 0b10000 | addr >> 4;
    lcd_cmd = addr & 0b1111;
}

void lcd_clearDisplay(void){
    for(uint8_t i = 0; i < 9; ++i){
        lcd_setPageAddr(i);
        lcd_setColumnAddr(0);
        for(uint8_t j = 0; j < 128; ++j){
            lcd_busyWait();
            lcd_data = 0;
        }
    }
}

void lcd_init(void){
    lcd_cmd = RST; //Reset display
    delay_ms(5);

    lcd_clearDisplay();

    // Initialize LCD contrast
    lcd_cmd = POWER_CONTROLLER | 0b111; //turn on all 3 power circuits
    lcd_cmd = V0_RATIO | 3; //V0 Ratio 3
    lcd_cmd = BIAS_HIGH;

    lcd_cmd = VOL_MODE; //set volume to 25
    lcd_cmd = 25;

    lcd_cmd = DSP_ON;
    delay_ms(1);
}

// Block until busy flag is cleared
void lcd_busyWait(void) {
    while(lcd_cmd & 0x80);
}

// Enable Read-Modify-Write mode
void inline lcd_setRMW(void){
    lcd_cmd = RMW;
}

// Disable Read-Modify-Write mode
void inline lcd_releaseRMW(void){
    lcd_cmd = RMW_END;
}

// Either fill, erase, or invert a rectangle on screen
void lcd_setBlock(uint8_t x, uint8_t y, uint8_t w, uint8_t h, setBlockMode mode){
    lcd_busyWait();
    if(w > SCREEN_WIDTH-x) w = SCREEN_WIDTH-x; // Clamp width to right side of screen
    
    // Calculate starting and ending pages overlapped by block
    // Column zero is on the right, so subtract from 7 to reverse
    int8_t pageAddrStart = 7-(x/8);
    int8_t pageAddrEnd = 7-((x+w)/8);
    
    // calculate the limited overlap into the first and last columns
    uint8_t columnStart = x%8;
    uint8_t columnEnd = (x+w)%8;
    
    lcd_setRMW(); // Enable Read-Modify-Write mode to 
    for(int8_t i = pageAddrStart; i >= pageAddrEnd; --i){
        lcd_setPageAddr(i);
        lcd_setColumnAddr(y);
        
        // For the first column, shift right to only mask overlap
        uint8_t v = 0xff >> columnStart;
        columnStart = 0;

        // Similarly for last column, mask off overlap
        if(i == pageAddrEnd){
            v &= (uint8_t)(0xff << (8-columnEnd));
        }
        
        for(uint8_t i = 0; i < h; ++i){
            lcd_busyWait();
            uint8_t temp = lcd_data; // Dummy read
            if (mode == FILL) lcd_data |= v; // Fill
            else if (mode == ERASE) lcd_data &= ~v; // Clear
            else if (mode == INVERT) lcd_data ^= v; // Invert
        }
    }
    lcd_releaseRMW();
}

// Draw a character from the font at the specified coordinates
void lcd_drawSymbol(uint8_t symbolIndex, uint8_t x, uint8_t y){
    lcd_busyWait();
    if(x > SCREEN_WIDTH - 4) return;
    int8_t pageAddr = 7-(x/8);
    uint8_t columnStart = x%8;
    lcd_setRMW();
    lcd_setPageAddr(pageAddr);
    lcd_setColumnAddr(y);
    
    for(uint8_t i = 0; i < 5; ++i){
        lcd_busyWait();
        // "<< 4" to align 4-bit data to left side of byte
        // then shift right to start of symbol
        uint8_t data = getSymbolData(symbolIndex, i) << 4 >> columnStart; 
        uint8_t mask = 0b1111 << 4 >> columnStart;
        uint8_t temp = lcd_data; // Dummy read
        lcd_data = (lcd_data & ~mask) | data; // "& ~mask" to clear the 4 bits before we write into them
    }

    if(columnStart > 4){ // If we've overflowed into the next page, we need to do another write in that page
        lcd_setPageAddr(pageAddr - 1);
        lcd_setColumnAddr(y);
        
        for(uint8_t i = 0; i < 5; ++i){
            lcd_busyWait();
            // "<< 12" to align 4-bit data to left side of previous page
            // "- columnStart" is equivalent to shifting right to start of symbol
            uint8_t data = getSymbolData(symbolIndex, i) << 12 - columnStart;
            uint8_t mask = 0b1111 << 12 - columnStart;
            uint8_t temp = lcd_data; // Dummy read
            lcd_data = (lcd_data & ~mask) | data; // "& ~mask" to clear the 4 bits before we write into them
        }
    }

    lcd_releaseRMW();
}