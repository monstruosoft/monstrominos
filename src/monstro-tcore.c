/**
 * @file monstro-tcore.c
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
 * This file contains the core functions for a minimalistic 
 * implementation. The playfield, passed as an argument to each of the 
 * functions declared here, is expected to be represented as an 
 * array of 24 integers of type \c uint16_t:
 * 
 *      uint16_t playfield[24];
 * 
 * These values are hardcoded in order for the minimal approach used 
 * here to work reliably while maintaining a simple code strcuture. 
 * However, despite the values being hardcoded, these are enough to 
 * represent any typical playfield size.
 * 
 * Game pieces, passed as an argument to three of these functions, are 
 * expected to be represented as a single \c uint64_t where bits are 
 * set for each solid block in the piece and each row in the piece's  
 * shape starts at an offset 16 bits from the previous one, for example:
 * 
 *      uint64_t HorizontalI = 0xF;             // Horizontal I piece
 *                                              // uint64_t binary representation:
 *                                              // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
 *                                              // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
 *                                              // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
 *                                              // 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1
 *      uint64_t VerticalI = 0x1000100010001    // Vertical I piece
 *                                              // uint64_t binary representation:
 *                                              // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1
 *                                              // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1
 *                                              // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1
 *                                              // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1
 * 
 * @section SAMPLE_USAGE Sample usage
 * 
 * There are four function declarations providing the basic behavior 
 * for the game. A typical logic implementation would consist of the 
 * following steps:
 * 
 * - The playfield array is initialized to define the playfield walls. 
 * Also, the first piece in play is placed on the playfield by calling 
 * poner_pieza().
 * - At each cycle of the game loop the current piece is first removed 
 * from the playfield by calling borrar_pieza().
 * - The game logic finds the new potential position and rotation for 
 * the current piece and calls puede_mover() to verify if the piece can 
 * move to the new position/rotation.
 * - If puede_mover() returns \c true then the piece can be placed at 
 * the new position/rotation by calling poner_pieza() with the new 
 * coordinates and the corresponding \c uint64_t representing the new 
 * rotation, if any, and the game loop can continue to the next cycle.
 * 
 *   If puede_mover() returns \c false then the piece can't be 
 * placed at the new position/rotation and must instead remain at its 
 * original position/rotation by calling poner_pieza() with the original 
 * coordinates and the original piece \c uint64_t representation. At 
 * its most basic, this means that the piece will lock at its current 
 * position. However, the game logic might define ways to escape the 
 * most basic behavior by finding potential positions/rotations where 
 * the piece can move based on user input; all of this can be done by 
 * verifying the return value of puede_mover() at the possible new 
 * positions/rotations of the current piece.
 * 
 * - Each time a piece is locked into place, a call to 
 * borrar_completas() will remove the completed lines from the 
 * playfield.
 * 
 * So, at its most basic, a logic implementation would work as follows:
 * 
 *      // At each cycle of the game loop
 *      borrar_pieza(playfield, piece, x, y);
 *      int new_y = y - 1;                          // Try to move the piece down
 *      int new_x = x + user_input_x;               // Try to move the piece horizontally on user input
 *      uint64_t new_piece = piece;
 *      if (user_input_rotate)
 *          // Rotate on user input by:
 *          //      - Assigning to the new_piece variable a new uint64_t value
 *          //        corresponding to the next rotation of the current piece
 *          new_piece = piece_next_rotation;
 *      if (puede_mover(playfield, new_piece, new_x, new_y)) {
 *          x = new_x;
 *          y = new_y;
 *          piece = new_piece;
 *          poner_pieza(playfield, piece, x, y);
 *      }
 *      else {
 *          // Lock the piece in its current position
 *          poner_pieza(playfield, piece, x, y);
 *          borrar_completas();
 *          // Spawn a new piece by:
 *          //      - Assigning a new uint64_t value to the piece variable
 *          //      - Resetting the position to the top of playfield values
 *          spawn_new_piece();
 *      }
 * 
 * Typically, horizontal movement, vertical movement and rotation are 
 * handled as separated steps, each verifying if the piece puede_mover() 
 * separately. See the accompanying monstro-tlogic.c for a sample logic 
 * implementation.
 * 
 * @section PLAYFIELD_NOTE A note on the playfield
 * 
 * Although the actual playfield and pieces initalization is left to the 
 * logic, this code was written as a means to make it easy to visualize 
 * the playfield representation. As such, this code considers the 
 * playfield, an \c uint16_t[24], to have an inverted coordinate system 
 * where \c Y increases from the bottom to the top, as seen in 
 * borrar_completas(), and \c X increases from right to left, as seen 
 * in the use of <tt>left shifts</tt> for the pieces' \c X position. 
 * With this representation, \c playfield[0] is at the bottom of the 
 * playfield with its least significant bits representing the rightmost 
 * cells in the playfield and the most significant bits representing 
 * the leftmost cells in the playfield.
 * 
 * With this apparently odd coordinate system, the playfield maps to an 
 * easily visualisable version as a typical written representation of 
 * integers in their binary form. That is, the playfield would be 
 * represented like this:
 * 
 *          ...           ...                   ...
 *      playfield[20] = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[19] = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[18] = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[17] = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[16] = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[15] = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[14] = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[13] = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[12] = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[11] = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[10] = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[9]  = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[8]  = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[7]  = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[6]  = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[5]  = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[4]  = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[3]  = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[2]  = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[1]  = 0xE007   // 1 1 1 0 0 0 0 0 0 0 0 0 0 1 1 1
 *      playfield[0]  = 0xFFFF   // 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
 * 
 * Similarly, by using this coordinate system, an \c uint64_t can 
 * also be mapped to an easily visualisable representation of 
 * the pieces:
 * 
 *      // A typical T piece
 *      uint64_t T0 = 0x20007;      // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
 *                                  // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
 *                                  // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0
 *                                  // 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1
 * 
 *      uint64_t T1 = 0x200030002;  // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
 *                                  // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0
 *                                  // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1
 *                                  // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0
 * 
 *      uint64_t T2 = 0x70002;      // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
 *                                  // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
 *                                  // 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1
 *                                  // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0
 * 
 *      uint64_t T3 = 0x200060002;  // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
 *                                  // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0
 *                                  // 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0
 *                                  // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0
 * 
 * The code here relies on pointer arithmetic to place pieces on the 
 * playfield so, with the previous representation of the playfield and 
 * pieces, the result maps to an easy to visualize representation. Note, 
 * however, that such a representation is meant to be used in tutorials 
 * for easily visualizing the core behavior but, at least for \c X, the 
 * coordinates can be easily inverted by the logic to simplify the final 
 * drawing operations. See the accompanying examples for more details.
 * 
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>



/**
 * Pone una pieza en la posición (x, y) del tablero.
 * 
 * Esta función coloca una pieza en el tablero. La pieza a colocar 
 * debe encajar libremente en la posición especificada, de 
 * lo contrario se podría producir basura en el tablero.
 * 
 * @param area_de_juego Un apuntador a un arreglo de \c uint16_t 
 *                      representando el tablero del juego.
 * @param pieza         La pieza a colocar, representada como un entero 
 *                      de 64 bits \c uint64_t.
 * @param x             La posición \c x en la que se colocará la pieza.
 * @param y             La posición \c y en la que se colocará la pieza.
 */
void poner_pieza(uint16_t *area_de_juego, uint64_t pieza, int x, int y) {
    uint64_t *destino = (uint64_t *)&area_de_juego[y];
    uint64_t dato = pieza << x;
    *destino |= dato;
}



/**
 * Borra una pieza de la posición (x, y) del tablero.
 * 
 * Esta función borra del tablero una pieza previamente colocada con la 
 * función poner_pieza(). Llamar esta función sin haber llamado 
 * previamente poner_pieza(), o bien usando coordenadas distintas a las 
 * que se usaron para colocar la pieza, podría producir basura en 
 * el tablero.
 * 
 * @param area_de_juego Un apuntador a un arreglo de \c uint16_t 
 *                      representando el tablero del juego.
 * @param pieza         La pieza a colocar, representada como un entero 
 *                      de 64 bits \c uint64_t.
 * @param x             La posición \c x en la que se colocará la pieza.
 * @param y             La posición \c y en la que se colocará la pieza.
 */
void borrar_pieza(uint16_t *area_de_juego, uint64_t pieza, int x, int y) {
    uint64_t *destino = (uint64_t *)&area_de_juego[y];
    uint64_t dato = pieza << x;
    *destino ^= dato;
}



/**
 * Verifica si se puede colocar una pieza en el tablero en la 
 * posición (x, y).
 * 
 * Esta función verifica si se puede colocar una pieza en el tablero. 
 * Regresa \c true si la pieza puede encajar libremente en el tablero; 
 * de lo contrario regresa \c false.
 * 
 * @param area_de_juego Un apuntador a un arreglo de \c uint16_t 
 *                      representando el tablero del juego.
 * @param pieza         La pieza a colocar, representada como un entero 
 *                      de 64 bits \c uint64_t.
 * @param x             La posición \c x en la que se verificará si 
 *                      puede ser colocada la pieza.
 * @param y             La posición \c y en la que se verificará si 
 *                      puede ser colocada la pieza.
 * @return              \c true si la pieza puede ser colocada 
 *                      libremente en el tablero en la posición (x, y); 
 *                      de lo contrario, \c false.
 */
int puede_mover(uint16_t *area_de_juego, uint64_t pieza, int x, int y) {
    uint64_t *destino = (uint64_t *)&area_de_juego[y];
    uint64_t dato = pieza << x;
    uint64_t resultado = *destino | dato;
    resultado = resultado ^ dato;
    if (resultado == *destino) return true;
    return false;
}



/**
 * Borra las líneas completas del tablero.
 * 
 * @param area_de_juego Un apuntador a un arreglo de \c uint16_t 
 *                      representando el tablero del juego.
 * @param y             La posición \c Y en la que se ancló la última 
 *                      pieza. Este parámetro es importante para que 
 *                      esta función funcione correctamente ya que se 
 *                      basa en el hecho de que las líneas completas 
 *                      siempre estarán definidas por la posición en la 
 *                      que se colocó la última pieza.
 */
void borrar_completas(uint16_t *area_de_juego, int y) {
    uint16_t *origen = (uint16_t *)&area_de_juego[y];
    uint64_t destino = 0;
    int i = 0;
    
    // safeguard
    area_de_juego[0] = 0x7FFF;
    
    destino |= ((uint16_t)~*origen) ? (uint64_t)*origen << i++ * 16 : 0; origen++;
    destino |= ((uint16_t)~*origen) ? (uint64_t)*origen << i++ * 16 : 0; origen++;
    destino |= ((uint16_t)~*origen) ? (uint64_t)*origen << i++ * 16 : 0; origen++;
    destino |= ((uint16_t)~*origen) ? (uint64_t)*origen << i++ * 16 : 0; origen++;
    origen = &area_de_juego[y];
    *(uint64_t *)origen = destino;
    memmove(&area_de_juego[y + i], &area_de_juego[y + 4], (20 - y) * 2);  // 20 - y = 24 - 4 - y
    
    // safeguard
    area_de_juego[0] = 0xFFFF;
}
