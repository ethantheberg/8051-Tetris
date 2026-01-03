/* ---------------------------------------------------------------------------------
 * Ethan Berg
 * ECEN 5613 - Fall 2025 - Prof. McClure
 * University of Colorado Boulder
 * Created Sat Nov 29 2025
 * --------------------------------------------------------------------------------
 * LCD Driver
   ---------------------------------------------------------------------------------*/

#ifndef LCD_H
#define LCD_H

#include <stdint.h>

typedef enum{
    ERASE = 0, 
    FILL = 1, 
    INVERT = 2
} setBlockMode;

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 128

#define SYMBOL_WIDTH 4
#define SYMBOL_HEIGHT 6

// Memory-mapped addresses for communication with LCD. 
// Must be greater that 0x3FF to be outside of internal expanded ram. 
// Since A8 is connected the D/C pin on the LCD,
//  0x400 => A8 = 0 which signals command
//  0x500 => A8 = 1 which signals data
volatile __xdata uint8_t __at (0x500) lcd_data;
volatile __xdata uint8_t __at (0x400) lcd_cmd;


void delay_ms(uint8_t ms);
void lcd_init(void);

void lcd_clearDisplay(void);
void lcd_setPageAddr(uint8_t addr);
void lcd_setColumnAddr(uint8_t addr);
void lcd_busyWait(void);

// Enable and disable Read-Modify-Write mode
void inline lcd_setRMW(void);
void inline lcd_releaseRMW(void);

// Draw a rectangle on the screen
// mode = 0: clear
// mode = 1: fill
// mode = 2: invert
void lcd_setBlock(uint8_t x, uint8_t y, uint8_t w, uint8_t h, setBlockMode mode);

// Draw a character from the font at the specified coordinates
void lcd_drawSymbol(uint8_t symbolIndex, uint8_t x, uint8_t y);

#endif // LCD_H