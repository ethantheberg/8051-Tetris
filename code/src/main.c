/* ---------------------------------------------------------------------------------
 * Ethan Berg
 * ECEN 5613 - Fall 2025 - Prof. McClure
 * University of Colorado Boulder
 * Created Tue Nov 25 2025
 *  --------------------------------------------------------------------------------
 * Handheld Tetris button polling and main game loop
   ---------------------------------------------------------------------------------*/
   
#include <at89c51ed2.h>
#include <mcs51reg.h>
#include <stdint.h>
#include <stdlib.h>
#include "lcd.h"
#include "tetris.h"
#include "menu.h"
#include "audio.h"

#define LEFT_BUTTON   1 << 0
#define RIGHT_BUTTON  1 << 1
#define ENTER_BUTTON  1 << 2
#define ROTATE_BUTTON 1 << 3
#define HOLD_BUTTON   1 << 4
#define DOWN_BUTTON   1 << 5

#define FRAMES_PER_TICK 600
#define FRAMES_PER_LEVEL 75
// Read the button state from P1
uint8_t inline getButtonState(void){
    return ~(P1 >> 1) & 0b111111; // Inverted since the buttons are active low
}

// Check if a certain button has been pressed between the current and last states
uint8_t inline buttonPressed(uint8_t prev, uint8_t current, uint8_t buttonMask){
    return !(prev & buttonMask) && (current & buttonMask);
}

unsigned char __sdcc_external_startup(void){
    AUXR |= XRS1 | XRS0; // unlock extended ram
    return 0;
}

uint8_t timer0Toggle = 0; // Toggle to reduce minimum ISR frequency by 1/2
uint16_t timer0Reload = MUSIC_T0_RELOAD; // 0x5000 yields the right tempo for music playback

void timer0_ISR(void) __interrupt(1) {
    TH0 = timer0Reload >> 8;       
    TL0 = timer0Reload & 0xFF;
    if(timer0Toggle == 1) audioFlag = 1;
    timer0Toggle = !timer0Toggle;
}

void main(void){
    lcd_init(); 
    delay_ms(20);
    initAudio();
    
    uint8_t buttonState = 0;
    uint8_t prevState = 0;
    uint8_t rseed = 0; // Count frames waited on main menu before pressing start to get a seed for the RNG
    
    drawMainMenu();
    
    while(1){
        prevState = buttonState;
        buttonState = getButtonState();
        rseed++;
        if(buttonPressed(prevState, buttonState, ENTER_BUTTON)){
            playNote(0);
            break;
        }

        if(buttonPressed(prevState, buttonState, HOLD_BUTTON)){
            mute();
        }
        
        if(audioFlag == 1){
            advanceSong();
            audioFlag = 0;
        }
    }

    lcd_clearDisplay();
    
    srand(rseed);
    
    uint16_t tickTimer = 0;
    uint16_t tickThreshold = FRAMES_PER_TICK;
    uint8_t dropped = 0; // Flag for hard drop
    timer0Reload = SFX_T0_RELOAD; // Change tempo of SFX notes
    
    newGame();

    while(1){
        prevState = buttonState;
        buttonState = getButtonState();

        if(audioFlag){
            advanceSFX();
            audioFlag = 0;
        }

        if(gameEnded){
            if(buttonPressed(prevState, buttonState, ENTER_BUTTON)){
                break; // Exit infinite loop to restart at main menu
            }
            continue;
        }
        
        if(dropped){
            if(!tickDown()) continue;
            dropped = 0;
        }

        if(buttonPressed(prevState, buttonState, ENTER_BUTTON)){
            pause();
            playBeep(A3);
        }

        if(paused){
            if(buttonPressed(prevState, buttonState, HOLD_BUTTON)){
                mute();
            }
            continue;
        }
        
        if(buttonPressed(prevState, buttonState, LEFT_BUTTON)){
            move(-1);
            playBeep(A3);
        }

        if(buttonPressed(prevState, buttonState, RIGHT_BUTTON)){
            move(1);
            playBeep(A3);
        }

        if(buttonPressed(prevState, buttonState, ROTATE_BUTTON)){
            rotateCW();
            playBeep(A3);
        }

        if(buttonPressed(prevState, buttonState, DOWN_BUTTON)){
            dropped = 1;
            playBeep(E3);
        }

        if(buttonPressed(prevState, buttonState, HOLD_BUTTON)){
            hold();
            playBeep(B3);
        }

        ++tickTimer;
        if(tickTimer >= tickThreshold - level*FRAMES_PER_LEVEL){ // Ticks get faster for higher levels
            tickDown();
            // Count down the tetris alert timer
            if(tetrisAlert != 0){
                --tetrisAlert;
                if(tetrisAlert == 0) eraseTetrisAlert();
                else drawTetrisAlert();
            }
            tickTimer = 0;
        }
    }
}