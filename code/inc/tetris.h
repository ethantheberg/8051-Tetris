/* ---------------------------------------------------------------------------------
 * Ethan Berg
 * ECEN 5613 - Fall 2025 - Prof. McClure
 * University of Colorado Boulder
 * Created Tue Nov 25 2025
 * --------------------------------------------------------------------------------
 * Tetris Logic
   ---------------------------------------------------------------------------------*/

#ifndef TETRIS_H
#define TETRIS_H

#include <stdint.h>
#include <stdlib.h>

extern uint8_t paused;
extern uint16_t score;
extern uint8_t level;
extern uint8_t gameEnded;
extern uint8_t tetrisAlert; // Timer/flag for "Tetris!" popup

void drawBoard(void);
void drawTetrisAlert(void);
void eraseTetrisAlert(void);

uint8_t tickDown(void);
void newGame(void);
void pause(void);

void rotateCW(void);
void move(int8_t direction);
void hold(void);

#endif // TETRIS_H