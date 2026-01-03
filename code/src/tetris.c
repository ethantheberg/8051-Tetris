/* ---------------------------------------------------------------------------------
 * Ethan Berg
 * ECEN 5613 - Fall 2025 - Prof. McClure
 * University of Colorado Boulder
 * Created Tue Nov 25 2025
 * --------------------------------------------------------------------------------
 * Tetris Logic
   ---------------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdlib.h>
#include "lcd.h"
#include "tetris.h"
#include "audio.h"
#include "menu.h"

// Return codes for collision check
enum CollisionType {
    NO_COLLISION,
    LEFT_WALL_COLLISION,
    RIGHT_WALL_COLLISION,
    BOTTOM_COLLISION
};

// Board size in minos
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 19

// Mino size in pixels
#define MINO_SIZE 6

// Pixel offset for board
#define BOARD_OFFSET_X (SCREEN_WIDTH - BOARD_WIDTH * MINO_SIZE) / 2
#define BOARD_OFFSET_Y (SCREEN_HEIGHT - BOARD_HEIGHT * MINO_SIZE) - 1

// Pixel dimensions for the hold box and internal drawing
#define HELD_MINO_SIZE 5
#define HELD_BOX_WIDTH HELD_MINO_SIZE * 4 + 1
#define HELD_BOX_HEIGHT HELD_MINO_SIZE * 2 + 1

// Pixel offsets for level and score readout, and Game End message
#define LEVEL_SCORE_OFFSET_Y 3
#define GAME_END_X 18
#define GAME_END_Y 40

// Pixel offsets for pause symbol
#define PAUSE_OFFSET_Y 33
#define PAUSE_WIDTH 8
#define PAUSE_HEIGHT 35
#define PAUSE_SPACING 8

// Score for clearing 0, 1, 2, 3, or 4 lines in one place
const uint8_t scorePerLine[5] = {0, 1, 3, 5, 8};

uint8_t p; // Tetromino type 
int8_t x;
uint8_t y;
uint8_t r; // Tetromino rotation

uint8_t hp = 8; // Held tetromino type
uint8_t hFlag = 0; // Hold tetromino flag, to prevent multiple holds per place

uint8_t paused = 0;
uint16_t score = 0;
uint8_t clearedLines = 0;
uint8_t level = 1;
uint8_t gameEnded = 0;
uint8_t tetrisAlert = 0; // Timer/flag for "Tetris!" popup

// Bag array and size for tetromino randomization system
uint8_t bag[7];
uint8_t bagSize;

uint16_t board[BOARD_HEIGHT];

// Character indicies in bitpacked font for the tetris message, i.e. "TETRIS!"
const uint8_t tetrisMessage[7] = {12, 13, 12, 14, 15, 16, 17};

// Bitpacked tetromino sizes. Dimensions are the same for alternating rotations, so only 4 2-bit values need to be stored
// Format: wwhhwwhh (actually max x index and max y index i.e. width-1 and height-1)
const uint8_t sizes[7] = {
    0b00111100, // I
    0b01101001, // J
    0b01101001, // L
    0b01010101, // O
    0b01101001, // S
    0b01101001, // T
    0b01101001, // Z
};

// Bitpacked mino locations within each tetromino, for each rotation of each piece. Each of 4 minos needs 2 bits for x and 2 bits for y
// Format: xxyyxxyyxxyyxxyy
const uint16_t tetrominos[7][4] = {
    {0b0000010010001100, 0b0000000100100011, 0b0000010010001100, 0b0000000100100011}, // I
    {0b0000000101011001, 0b0000010000010010, 0b0000010010001001, 0b0100010100100110}, // J
    {0b1000000101011001, 0b0000000100100110, 0b0000010010000001, 0b0000010001010110}, // L
    {0b0000010000010101, 0b0000010000010101, 0b0000010000010101, 0b0000010000010101}, // O
    {0b0100100000010101, 0b0000000101010110, 0b0100100000010101, 0b0000000101010110}, // S
    {0b0100000101011001, 0b0000000101010010, 0b0000010010000101, 0b0100000101010110}, // T
    {0b0000010001011001, 0b0100000101010010, 0b0000010001011001, 0b0100000101010010}, // Z
};

/* -------------------------------------------------------------------------- */
/*                                Unbitpacking                                */
/* -------------------------------------------------------------------------- */

// Extract bitpacked information
uint8_t inline width(uint8_t p, uint8_t r){ return (0b11 & (sizes[p] >> (2 + 4*(r%2)))) + 1; }
uint8_t inline height(uint8_t p, uint8_t r){ return (0b11 & (sizes[p] >> 4*(r%2))) + 1; }
uint8_t inline minoX(uint8_t p, uint8_t r, uint8_t n){ return 0b11 & (tetrominos[p][r] >> (2+4*n)); }
uint8_t inline minoY(uint8_t p, uint8_t r, uint8_t n){ return 0b11 & (tetrominos[p][r] >> (4*n)); }

/* -------------------------------------------------------------------------- */
/*                                   Drawing                                  */
/* -------------------------------------------------------------------------- */

void inline drawMino(uint8_t x, uint8_t y){
    lcd_setBlock(BOARD_OFFSET_X + x*MINO_SIZE, BOARD_OFFSET_Y + y*MINO_SIZE, MINO_SIZE - 1, MINO_SIZE - 1, FILL);
}

void inline eraseMino(uint8_t x, uint8_t y){
    lcd_setBlock(BOARD_OFFSET_X + x*MINO_SIZE, BOARD_OFFSET_Y + y*MINO_SIZE, MINO_SIZE, MINO_SIZE, ERASE);
}

void inline eraseRows(uint8_t y, uint8_t n){
    lcd_setBlock(BOARD_OFFSET_X, BOARD_OFFSET_Y + y*MINO_SIZE, MINO_SIZE * BOARD_WIDTH, MINO_SIZE*n, ERASE);
}

void inline drawHeldMino(uint8_t x, uint8_t y, uint8_t offsetX, uint8_t offsetY){
    lcd_setBlock(x*HELD_MINO_SIZE + offsetX, y* HELD_MINO_SIZE + offsetY, HELD_MINO_SIZE - 1, HELD_MINO_SIZE - 1, FILL);
}

void drawHeldBox(void){
    // Draw outline and clear inside of box
    lcd_setBlock(0, 0, HELD_BOX_WIDTH, HELD_BOX_HEIGHT, FILL);
    lcd_setBlock(0, 0, HELD_BOX_WIDTH - 1, HELD_BOX_HEIGHT - 1, ERASE);
    // 8 is the initialization value i.e. no held piece yet
    if(hp != 8){
        // loop through minos and draw them
        for(uint8_t i = 0; i < 4; ++i){
            drawHeldMino(
                minoX(hp, 0, i), 
                minoY(hp, 0, i), 
                HELD_MINO_SIZE*2 - (HELD_MINO_SIZE*width(hp, 0))/2, 
                HELD_MINO_SIZE - (HELD_MINO_SIZE*height(hp, 0))/2
            );
        }
    }
}

void drawLevelScore(void){
    // clear out interior of region and draw bottom line
    lcd_setBlock(HELD_BOX_WIDTH, 0, 64-HELD_BOX_WIDTH, SYMBOL_HEIGHT, ERASE);
    lcd_setBlock(HELD_BOX_WIDTH, HELD_BOX_HEIGHT-1, 64-HELD_BOX_WIDTH-3, 1, FILL);

    // Draw 'L' and level
    lcd_drawSymbol(10, HELD_BOX_WIDTH + 1, LEVEL_SCORE_OFFSET_Y);
    lcd_drawSymbol(level%10, HELD_BOX_WIDTH + 5, LEVEL_SCORE_OFFSET_Y);

    // Draw score
    uint16_t temp = score;
    for(uint8_t place = 0; place < 4; ++place){
        lcd_drawSymbol(temp%10, 60 - place*4 - 1, LEVEL_SCORE_OFFSET_Y);
        temp /= 10;
    }
}

void drawTetrisAlert(void){
    // draw "TETRIS!" right under score
    for(uint8_t i = 0; i < 7; ++i){
        lcd_drawSymbol(tetrisMessage[i], 60 - 3 - 4*(7 - i), HELD_BOX_HEIGHT + 1);
    }
}

void eraseTetrisAlert(void){
    lcd_setBlock(60 - 3 - 4*7, HELD_BOX_HEIGHT + 1, 4*7, 5, ERASE);
}

void drawBoard(void){
    lcd_setBlock(0, HELD_BOX_HEIGHT, 1, SCREEN_HEIGHT - HELD_BOX_HEIGHT, FILL); // left wall
    lcd_setBlock(1, HELD_BOX_HEIGHT, 1, SCREEN_HEIGHT - HELD_BOX_HEIGHT, ERASE); // blank strip to the right
    lcd_setBlock(SCREEN_WIDTH - 2, HELD_BOX_HEIGHT, 1, SCREEN_HEIGHT - HELD_BOX_HEIGHT, FILL); // right wall
    lcd_setBlock(SCREEN_WIDTH - 1, HELD_BOX_HEIGHT, 1, SCREEN_HEIGHT - HELD_BOX_HEIGHT, ERASE); // blank strip
    
    lcd_setBlock(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1, 1, FILL); // bottom
    drawHeldBox();
    drawLevelScore();
}

// Draw the currently falling tetromino
void drawTetromino(void){
    for(uint8_t i = 0; i < 4; ++i){
        drawMino(x+minoX(p, r, i), y+minoY(p, r, i));
    }
}

// Erase the currently falling tetromino
void eraseTetromino(void){
    for(uint8_t i = 0; i < 4; ++i){
        eraseMino(x+minoX(p, r, i), y+minoY(p, r, i));
    }
}

/* -------------------------------------------------------------------------- */
/*                                 Game Logic                                 */
/* -------------------------------------------------------------------------- */

// Write currently falling tetrimino into board
void putTetromino(void){
    playPlacedTetromino();
    for(uint8_t i = 0; i < 4; ++i){
        board[y+minoY(p, r, i)] |= 1 << (x+minoX(p, r, i));
    }
}

void endGame(void){
    playGameOver();
    gameEnded = 1;
    lcd_clearDisplay();
    
    // Draw level and score in the middle of the screen
    lcd_drawSymbol(10, GAME_END_X, GAME_END_Y);
    lcd_drawSymbol(level, GAME_END_X + 4, GAME_END_Y);

    uint16_t temp = score;
    for(uint8_t place = 0; place < 4; ++place){
        lcd_drawSymbol(temp%10, GAME_END_X + 4*(6 - place), GAME_END_Y);
        temp /= 10;
    }

    drawPressStart(GAME_END_Y + 30);
}

// Check for full lines below place location and clear them, update score and level
void clearLines(void){
    uint8_t lowestAffectedRow = 0;
    uint8_t clearCount = 0;
    for(; y < BOARD_HEIGHT; ++y){ // y can be the index since it is not used until it is set to zero again for the new tetromino
        if(board[y] == 0b1111111111){
            clearCount++;
            if(y > lowestAffectedRow) lowestAffectedRow = y;
            for(uint8_t i = y; i > 0; --i){
                board[i] = board[i-1];
            }
        }
    }
    score += scorePerLine[clearCount] * level;
    clearedLines += clearCount;
    if(clearedLines >= 10){
        clearedLines -= 10;
        ++level;
        playLevelUp();
    }
    // Level and score are re-rendered in drawBoard

    // Redraw everything above the lowest affected row
    eraseRows(0, lowestAffectedRow+1);
    for(y = 0; y <= lowestAffectedRow; ++y){
        for(uint8_t x = 0; x < BOARD_WIDTH; ++x){
            if((board[y] >> x) & 1){
                drawMino(x, y);
            }
        }
    }
    drawBoard();
    
    if(clearCount == 4){
        tetrisAlert = 4;
        playTetrisClear();
        drawTetrisAlert();
    } else if (clearCount != 0){
        playLineClear();
    }
}

// Check if a tentative location is valid
uint8_t checkInterference(int8_t x, uint8_t y, uint8_t p, uint8_t r){
    if(x < 0) return LEFT_WALL_COLLISION;
    if(x+width(p, r) > BOARD_WIDTH) return RIGHT_WALL_COLLISION;
    if(y+height(p, r) > BOARD_HEIGHT) return BOTTOM_COLLISION;

    for(uint8_t i = 0; i < 4; ++i){
        if(board[y + minoY(p, r, i)] >> (x+minoX(p, r, i)) & 1){
            return BOTTOM_COLLISION;
        }
    }
    return 0;
}

void newTetromino(void){
    // Bag system for generating tetrominos -- guarantees every type before seeing repeats
    uint8_t p_n = rand() % bagSize;
    p = bag[p_n];
    bagSize--;
    if(bagSize == 0){
        bagSize = 7;
        for(uint8_t i = 0; i < 7; ++i){
            bag[i] = i;
        }
    } else {
        for(uint8_t i = p_n; i < bagSize; ++i){
            bag[i] = bag[i+1];
        }
    }

    r = 0;
    x = BOARD_WIDTH/2 - width(p, r)/2;
    y = 0; 

    // If the tetromino clips other pieces on spawn, the game is lost
    if(checkInterference(x, y, p, r)){
        return endGame();
    }

    drawTetromino();
}

// Move tetromino down
uint8_t tickDown(void){
    if(checkInterference(x, y+1, p, r) == BOTTOM_COLLISION){
        // Place tetromino
        putTetromino();
        clearLines();
        hFlag = 0;
        newTetromino();
        return 1;
    }
    eraseTetromino();
    y++;
    drawTetromino();
    return 0;
}

void inline clearBoard(void){
    for(uint8_t i = 0; i < BOARD_HEIGHT; ++i){
        board[i] = 0;
    }
}

void newGame(void){
    clearBoard();
    eraseRows(0, BOARD_HEIGHT);

    hp = 8;
    hFlag = 0;
    paused = 0;
    score = 0;
    clearedLines = 0;
    level = 1;
    gameEnded = 0;
    tetrisAlert = 0;

    for (uint8_t i = 0; i < 7; ++i) {
        bag[i] = i;
    }
    bagSize = 7;

    drawBoard();

    newTetromino();
}

void pause(void){
    // Draw pause symbol by inverting blocks
    lcd_setBlock((SCREEN_WIDTH - PAUSE_SPACING)/2 - PAUSE_WIDTH, PAUSE_OFFSET_Y, PAUSE_WIDTH, PAUSE_HEIGHT, INVERT);
    lcd_setBlock((SCREEN_WIDTH + PAUSE_SPACING)/2, PAUSE_OFFSET_Y, PAUSE_WIDTH, PAUSE_HEIGHT, INVERT);
    paused = !paused;
}

void rotateCW(void){
    uint8_t xOffset = 0;
    uint8_t yOffset = 0;
    uint8_t interferenceType = checkInterference(x, y, p, (r+1)%4);
    while(interferenceType != NO_COLLISION){
        if(interferenceType == LEFT_WALL_COLLISION) xOffset++;
        if(interferenceType == RIGHT_WALL_COLLISION) xOffset--;
        if(interferenceType == BOTTOM_COLLISION) {
            if(y+yOffset == 0) return endGame();
            else yOffset--;
        }
        interferenceType = checkInterference(x+xOffset, y+yOffset, p, (r+1)%4);
    }

    eraseTetromino();
    x += xOffset;
    y += yOffset;
    r = (r + 1)%4;
    drawTetromino();
}

void move(int8_t direction){
    if(checkInterference(x+direction, y, p, r) != NO_COLLISION){
        return;
    }
    eraseTetromino();
    x += direction;
    drawTetromino();
}

void hold(void){
    if(hFlag) return;
    hFlag = 1;
    eraseTetromino();
    uint8_t temp = hp;
    hp = p;
    drawHeldBox();
    if(temp == 8){
        newTetromino();
        return;
    }
    p = temp;
    r = 0;
    y = 0;
    x = BOARD_WIDTH/2 - width(p, r)/2;
    drawTetromino();
}