/**
 * @file monstro-tcolor.c
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
 * This file contains a sample implementation for handling a color 
 * playfield. Note that the core functions in monstro-tcore.c use 1 bit to 
 * represent each cell in the playfield but that doesn't mean it's not 
 * possible to create a color playfield. This file is an example of 
 * how to do just that by using the core information in the \c MONSTRO_TGAME 
 * struct to generate a color representation of the playfield.
 */

#include <stdint.h>
#include <string.h>
#include "monstro-tlogic.h"



/**
 * Initializes the color playfield representation.
 * 
 * This function initializes de color playfield representation, where 
 * each cell is represented as a color index corresponding to the 
 * \c piece field in the \c MONSTRO_TGAME structure. Since \c 0 is a 
 * valid index, empty cells are initialized with <tt>-1</tt> and 
 * playfield walls for this particular implementation are initialized 
 * with index value 7 (indices 0 through 6 corresponding to game pieces' colors).
 * 
 * TODO: Replace hardcoded values ???
 * TODO: Also, hardcoded initialization is for a 10-cell width playfield, 
 *       should allow for other possible initialization values ???
 */
void init_color_playfield(MONSTRO_TGAME *game) {
    memset(game->color_playfield, 7, MONSTRO_TFIELD_SIZE * 16);
    for (int y = 1; y < 24; y++)
        memset(game->color_playfield[y] + 3, -1, 10);
}



/**
 * Updates the color playfield according to the current state of the playfield.
 * 
 * This function must be called *only if* \c MONSTRO_TWANT_COLORS is defined. 
 * It will update the color representation of the playfield. Note that, unlike 
 * the core representation of the playfield, this does not include the current 
 * piece as that would complicate the code unnecessarily; instead, the current 
 * piece can be drawn in place by the final drawing function (see the accompanying 
 * examples). Also, keep in mind that this function is not mandatory for the core 
 * functions to work and is linked to the particular logic implementation in 
 * monstro-tlogic.h and monstro-tlogic.c and only when \c MONSTRO_TWANT_COLORS 
 * is defined. Other logic implementations can use this function or write
 * their own versions for handling color playfields as needed.
 * 
 * @param game  A \c MONSTRO_TGAME struct representing the current game.
 */
void update_color_playfield(MONSTRO_TGAME *game) {
    uint64_t t = game->current_piece << game->x;
    
// This places the piece permanently in the color playfield
    if (game->flags & MONSTRO_TACTION_SNAP)
        for (int i = 0; i < 64; i++)
            if (t & ((uint64_t)1 << i))
                game->color_playfield[game->y + i / 16][i % 16] = game->piece;

// This clears completed lines from the color playfield
    if ((game->flags & MONSTRO_TACTION_SNAP) && (game->flags & MONSTRO_TACTION_CLEARED)) {
        int y = game->y;
        for (int i = 0; i < 4; i++)
            if (!(game->flags & (MONSTRO_TACTION_CLEARED0 << i)))
                memmove(game->color_playfield[y++], game->color_playfield[game->y + i], sizeof(char) * 16);
        memmove(game->color_playfield[y], game->color_playfield[game->y + 4], sizeof(char) * 16 * (20 - y));
    }
}
