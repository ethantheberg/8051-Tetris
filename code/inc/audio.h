/* ---------------------------------------------------------------------------------
 * Ethan Berg
 * ECEN 5613 - Fall 2025 - Prof. McClure
 * University of Colorado Boulder
 * Created Wed Dec 03 2025
 * --------------------------------------------------------------------------------
 * Game audio driver
   ---------------------------------------------------------------------------------*/

#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>


// Note definitions. Values are Timer 2 reload values that yield certain frequencies 
// Reload value r for frequency f is: 
// r = 65536  - F_per / (4 * f)
#define A3 59252
#define B3 59938
#define E4 61342
#define E3 56993
#define E5 63439
#define A5 63965
#define E2 48759
#define Ds2 47783
#define D2 46707
#define Cs2 44402

#define MUSIC_T0_RELOAD 0x5000
#define SFX_T0_RELOAD 0x8000

// Audio processing flag raised by Timer 0 ISR
extern uint8_t audioFlag;

void initAudio(void);
void mute(void);
// Configure Timer 2 to modulate a certain frequency
void playNote(uint16_t note);

// Advance through note queue for playing SFX
void advanceSFX(void);

// Enqueue various sound effects for certain game events
void playBeep(uint16_t note);
void playLineClear(void);
void playTetrisClear(void);
void playLevelUp(void);
void playGameOver(void);
void playPlacedTetromino(void);

//Advance through Tetris theme song on main menu
uint8_t advanceSong(void);

#endif // AUDIO_H