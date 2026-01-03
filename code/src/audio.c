/* ---------------------------------------------------------------------------------
 * Ethan Berg
 * ECEN 5613 - Fall 2025 - Prof. McClure
 * University of Colorado Boulder
 * Created Wed Dec 03 2025
 * --------------------------------------------------------------------------------
 * Game audio driver
   ---------------------------------------------------------------------------------*/


#include <at89c51ed2.h>
#include <mcs51reg.h>
#include <stdint.h>
#include "audio.h"

#define SFX_QUEUE_LENGTH 16
#define SONG_SECTION_LENGTH 127

// Timer 2 values for the notes of the song. 1 represents sustain, and 0 represents no audio. 
const uint16_t ASection[127] = {57148, 1, 1, 1, 54340, 1, 54968, 1, 56121, 1, 57148, 56121, 54968, 1, 54340, 1, 52969, 1, 1, 1, 52969, 1, 54968, 1, 57148, 1, 1, 1, 56121, 1, 54968, 1, 54340, 1, 1, 1, 52221, 1, 54968, 1, 56121, 1, 1, 1, 57148, 1, 1, 1, 54968, 1, 1, 1, 52969, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 56121, 1, 1, 1, 57619, 1, 59252, 1, 1, 1, 58483, 1, 57619, 1, 57148, 1, 1, 1, 0, 0, 54968, 1, 57148, 1, 1, 1, 56121, 1, 54968, 1, 54340, 1, 1, 1, 54340, 1, 54968, 1, 56121, 1, 1, 1, 57148, 1, 1, 1, 54968, 1, 1, 1, 52969, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0};
const uint16_t BSection[127] = {48761, 1, 1, 1, 1, 1, 1, 1, 44400, 1, 1, 1, 1, 1, 1, 1, 46706, 1, 1, 1, 1, 1, 1, 1, 43144, 1, 1, 1, 1, 1, 1, 1, 44400, 1, 1, 1, 1, 1, 1, 1, 40401, 1, 1, 1, 1, 1, 1, 1, 38907, 1, 1, 1, 1, 1, 1, 1, 43144, 1, 1, 1, 0, 0, 0, 0, 48761, 1, 1, 1, 1, 1, 1, 1, 44400, 1, 1, 1, 1, 1, 1, 1, 46706, 1, 1, 1, 1, 1, 1, 1, 43144, 1, 1, 1, 1, 1, 1, 1, 44400, 1, 1, 1, 48761, 1, 1, 1, 52969, 1, 1, 1, 52969, 1, 1, 1, 52221, 1, 1, 1, 1, 1, 1, 1, 0};

//The form of the theme song; A A B A
const uint8_t form[4] = {0, 0, 1, 0};

// Track progress through a given section and through the form of the song
uint8_t sectionProgress;
uint8_t formProgress;

uint8_t audioFlag;

// Muted flag
uint8_t muted = 0;

void initAudio(void){
    sectionProgress = 0;
    formProgress = 0;
    audioFlag = 0;
    muted = 0;

    TMOD |= T0_M0; 
    TH0 = 0xFF;
    TL0 = 0xFF;
    ET0 = 1;
    EA = 1;
    TR0 = 1;

    T2MOD |= T2OE;
    C_T2 = 0;
}

void mute(void){
    if(muted){
        muted = 0;
        TR0 = 1;
        TR2 = 1;
        SFXQueueBack = 0;
        SFXQueueFront = 0;
        return;
    }
    muted = 1;
    TR0 = 0;
    TR2 = 0;
    SFXQueueBack = 0;
    SFXQueueFront = 0;
}

void playNote(uint16_t note){
    if(note == 0){
        TR2 = 0;
    } else if (note != 1){
        TR2 = 1;
        RCAP2H = note >> 8;
        RCAP2L = note & 0xFF;
    }
}

uint8_t advanceSong(void){
    if(sectionProgress == SONG_SECTION_LENGTH) {
        sectionProgress = 0;
        ++formProgress;
        formProgress %= 4;
    }
    uint16_t note;
    if(form[formProgress]) note = BSection[sectionProgress];
    else note = ASection[sectionProgress];

    playNote(note);

    ++sectionProgress;
    return 0;
}

uint16_t SFXQueue[SFX_QUEUE_LENGTH];
uint8_t SFXQueueBack = 0;
uint8_t SFXQueueFront = 0;

void advanceSFX(void){
    if(SFXQueueBack == SFXQueueFront){
        playNote(0);
        return;
    }
    playNote(SFXQueue[SFXQueueFront]);
    SFXQueueFront = (SFXQueueFront + 1) % SFX_QUEUE_LENGTH;
}

// Add note into SFX queue
void enqueue(uint16_t note){
    if((SFXQueueBack + 1) % SFX_QUEUE_LENGTH == SFXQueueFront) return;
    SFXQueue[SFXQueueBack] = note;
    SFXQueueBack = (SFXQueueBack + 1) % SFX_QUEUE_LENGTH;
}

void playBeep(uint16_t note){
    if(muted) return;
    enqueue(note);
}

void playLineClear(void){
    if(muted) return;
    enqueue(E3);
    enqueue(0);
    enqueue(E3);
}

void playTetrisClear(void){
    if(muted) return;
    enqueue(E3);
    enqueue(E4);
    enqueue(E3);
    enqueue(E4);
}

void playLevelUp(void){
    if(muted) return;
    enqueue(E5);
    enqueue(E5);
    enqueue(A5);
    enqueue(E5);
}

void playGameOver(void){
    if(muted) return;
    enqueue(E2);
    enqueue(1);
    enqueue(Ds2);
    enqueue(1);
    enqueue(D2);
    enqueue(1);
    enqueue(Cs2);
    enqueue(1);
}

void playPlacedTetromino(void){
    enqueue(E2);
    enqueue(1);
}
