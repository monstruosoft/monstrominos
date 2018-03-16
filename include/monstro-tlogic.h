/**
 * @file monstro-tlogic.h
 * 
 * @section LICENSE License
 * 
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 * 
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 * For more information, please refer to <https://unlicense.org>
 * 
 * @section DESCRIPTION Description
 * 
 * This file contains function prototypes, struct definitions and 
 * defines for the logic implementation in monstro-tlogic.c.
 */

#define MONSTRO_TFIELD_SIZE                24

#define MONSTRO_TINPUT_UP                   1      // Game inputs
#define MONSTRO_TINPUT_DOWN                 2
#define MONSTRO_TINPUT_LEFT                 4
#define MONSTRO_TINPUT_RIGHT                8
#define MONSTRO_TINPUT_ROTATE_LEFT         16
#define MONSTRO_TINPUT_ROTATE_RIGHT        32

#define MONSTRO_TMOVE_LIMIT                64      // Default horizontal movement counter limit
#define MONSTRO_TDROP_LIMIT                64      // Default vertical movement counter limit
#define MONSTRO_TSNAP_LIMIT                65      // Default lock/snap counter limit

// Game action flags
#define MONSTRO_TACTION_MOVE              0x1
#define MONSTRO_TACTION_DROP              0x2
#define MONSTRO_TACTION_ROTATE_LEFT       0x4
#define MONSTRO_TACTION_ROTATE_RIGHT      0x8
#define MONSTRO_TACTION_WALL_KICK        0x10
#define MONSTRO_TACTION_FLOOR_KICK       0x20
#define MONSTRO_TACTION_SPIN             0x40
#define MONSTRO_TACTION_SNAP             0x80
#define MONSTRO_TACTION_CLEARED         0xF00       // 4 bit flags for cleared lines
#define MONSTRO_TACTION_CLEARED0        0x100
#define MONSTRO_TACTION_CLEARED1        0x200
#define MONSTRO_TACTION_CLEARED2        0x400
#define MONSTRO_TACTION_CLEARED3        0x800
#define MONSTRO_TACTION_SPAWN          0x1000



typedef struct {
    uint16_t playfield[MONSTRO_TFIELD_SIZE];
    int piece;          // The index of the current piece
    int rotation;       // The index of the current piece rotation
    int x, y;           // The current piece position
    int inputs;
// Game action flags for each logic call; these flags are returned 
// from mover_pieza() in order to let the calling code respond to the 
// game actions at each call of the game logic.
    int flags;
    int snap_default;
    int snap_count;
    int snap_index;
    int drop_default;
    int drop_count;
    int drop_index;
    int move_default;
    int move_count;
    int move_index;
#ifdef MONSTRO_TWANT_COLORS
    int8_t color_playfield[MONSTRO_TFIELD_SIZE][16];
// Currently, the only places where knowing the piece uint64_t representation 
// outside of the logic implementation is needed is when drawing the color version of 
// the playfield, thus this variable is defined here, only when building 
// with MONSTRO_TWANT_COLORS; otherwise it would add 8 bytes to the size 
// of the structure that wouldn't be used for minimal implementations.
// The downside, every use of current piece must be enclosed in the 
// corresponding conditional compilation blocks. This, however, allows to 
// remove the extern reference to piezas[]. So, it's a matter of choosing 
// between using current_piece in the MONSTRO_TGAME definition or using 
// extern references to piezas[] throughout the code.
    uint64_t current_piece;
#endif
} MONSTRO_TGAME;



// Public function prototypes
void mover_pieza(MONSTRO_TGAME *game);
int spawn_piece(MONSTRO_TGAME *game);
#ifdef MONSTRO_TWANT_COLORS
void init_color_playfield(MONSTRO_TGAME *game);
void update_color_playfield(MONSTRO_TGAME *game);
#endif
