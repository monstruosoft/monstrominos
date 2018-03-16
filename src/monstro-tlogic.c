/**
 * @file monstro-tlogic.c
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
 * This file contains a sample logic implementation. The actual 
 * implementation for the logic, movement or piece definition 
 * is irrelevant as long as the playfield and piece definitions are 
 * consistent with the arguments expected by the core functions 
 * in monstro-tcore.c. That is, a different logic implementation is possible as 
 * long as, when calling the monstro-tcore.c funtions, the arguments are consistent.
 * 
 * This particular logic implementation contains simple versions of 
 * common mechanics as wall kick, floor kick and basic twists or spins.
 * 
 * @section LOGIC_NOTE A note on the logic
 * 
 * This particular logic implementation uses the definitions in 
 * monstro-tlogic.h for gameplay that is, hopefully, independent of the 
 * actual library used for the final product. That is, independently of 
 * the library used for the final input handling and drawing, the logic 
 * works based only on the structures and input flags defined in monstro-tlogic.h.
 * 
 * Currently, the logic also works based on hardcoded values defined for the 
 * horizontal, vertical and piece lock/snap movement. These values are 
 * expected to produce a consistent gameplay given that the game logic 
 * is called at a rate of ~30 times per second.
 * 
 * With this in mind, the final library-specific code must do the 
 * following:
 * 
 *      // Declare a MONSTRO_TGAME variable representing an independent game
 *      MONSTRO_TGAME game;
 * 
 *      ...
 *      // Within the game loop, update user inputs and call the main 
 *      // logic function mover_pieza() at a rate of ~30 times per second
 *      game.inputs = 0;
 *      if (user_input_down) game.inputs |= MONSTRO_TINPUT_DOWN;
 *      if (user_input_left) game.inputs |= MONSTRO_TINPUT_LEFT;
 *      if (user_input_right) game.inputs |= MONSTRO_TINPUT_RIGHT;
 *      if (user_input_rotate_left) game.inputs |= MONSTRO_TINPUT_ROTATE_LEFT;
 *      if (user_input_rotate_right) game.inputs |= MONSTRO_TINPUT_ROTATE_RIGHT;
 *      mover_pieza(&game);
 * 
 * The actual piece movement is internally handled by a set of variables 
 * defined in monstro-tlogic.h. The meaning of these variables is as 
 * follows:
 * 
 * - *snap_index, drop_index, move_index* define the increase amount to 
 * their respective counter; the greater the index, the faster the counter 
 * will increase.
 * - *snap_count, drop_count, move_count* define the movement or snap counter; 
 * this variable may increase on each game loop and, whenever it reaches the 
 * value of its corresponding <em>*_default</em> variable, the piece will 
 * move one block in the corresponding direction or, for the \c snap counter, 
 * it will lock in place.
 * - *snap_default, drop_default, move_default* define the default limit value 
 * that the corresponding counter variable must reach before a piece is moved. 
 * These variables are initially assigned to the corresponding constants 
 * \c MONSTRO_TSNAP_LIMIT, \c MONSTRO_TDROP_LIMIT and \c MONSTRO_TMOVE_LIMIT 
 * but as the game advances their value might change to increase the default 
 * snap, drop and movement speed at each game level.
 * 
 * There are two possible ways to increase a piece's movement speed. Either 
 * the <em>*_default</em> values are decreased so that the counter reaches 
 * the value faster, or the <em>*_index</em> values are increased to reach 
 * the limit value faster. In general, the <em>*_default</em> values should 
 * remain constant for the duration of a game's level and only modified each 
 * time the player advances to a new level, increasing de default speed. So, 
 * changes to movement speed based on player input should be handled by 
 * modifying the <em>*_index</em> values instead.
 */

#include <stdlib.h>                 // rand()
#include <stdint.h>
#include <stdbool.h>
#include <monstro-tlogic.h>



// Piezas:
static enum {_I_, _O_, _T_, _J_, _L_, _S_, _Z_};   // Named values for the pieces' index
// I,    O,      T,      J,    L,      S,    Z
// cyan, yellow, purple, blue, orange, lime, red
static const uint64_t I[] = {0xF00000000, 0x2000200020002, 0xF0000, 0x4000400040004};
static const uint64_t O[] = {0x600060000, 0x600060000, 0x600060000, 0x600060000};
static const uint64_t T[] = {0x200070000, 0x200030002, 0x70002, 0x200060002};
static const uint64_t J[] = {0x400070000, 0x300020002, 0x70001, 0x200020006};
static const uint64_t L[] = {0x100070000, 0x200020003, 0x70004, 0x600020002};
static const uint64_t S[] = {0x300060000, 0x200030001, 0x30006, 0x400060002};
static const uint64_t Z[] = {0x600030000, 0x100030002, 0x60003, 0x200060004};
// Use 0xF000700030001 to see the way the board maps to an uint64_t
static const uint64_t *piezas[] = {I, O, T, J, L, S, Z};



/**
 * Verifica si una pieza puede hacer <em>wall kick</em>.
 * 
 * @param area_de_juego Un apuntador a un arreglo de \c uint16_t 
 *                      representando el tablero del juego.
 * @param pieza         La pieza a colocar, representada como un entero 
 *                      de 64 bits \c uint64_t.
 * @param x             La posición \c x en la que se encuentra la pieza.
 * @param y             La posición \c y en la que se encuentra la pieza.
 * @return              La función regresa <tt>-1</tt> si la pieza puede 
 *                      hacer <em>wall kick</em> hacia una posición menor 
 *                      en el eje \c X, regresa \c 1 si la pieza puede 
 *                      hacer <em>wall kick</em> hacia una posición mayor 
 *                      en el eje \c X; finalmente, regresa \c 0 si la pieza
 *                      no puede hacer <em>wall kick</em>
 */
static int puede_wallkick(uint16_t *area_de_juego, uint64_t pieza, int x, int y) {
    if (puede_mover(area_de_juego, pieza, x - 1, y))
        return -1;
    else if (puede_mover(area_de_juego, pieza, x + 1, y))
        return 1;
    return 0;
}



/**
 * Verifica si una pieza puede hacer <em>floor kick</em>.
 * 
 * @param area_de_juego Un apuntador a un arreglo de \c uint16_t 
 *                      representando el tablero del juego.
 * @param pieza         La pieza a colocar, representada como un entero 
 *                      de 64 bits \c uint64_t.
 * @param x             La posición \c x en la que se encuentra la pieza.
 * @param y             La posición \c y en la que se encuentra la pieza.
 * @return              \c true si la pieza puede hacer <em>floor kick</em>; 
 *                      de lo contrario, \c false.
 */
static int puede_floorkick(uint16_t *area_de_juego, uint64_t pieza, int x, int y) {
    if (puede_mover(area_de_juego, pieza, x, y + 1))
        return true;
    return false;
}



/**
 * Verifica si una pieza puede hacer un \c spin.
 * 
 * @param area_de_juego Un apuntador a un arreglo de \c uint16_t 
 *                      representando el tablero del juego.
 * @param pieza         La pieza a colocar, representada como un entero 
 *                      de 64 bits \c uint64_t.
 * @param x             La posición \c x en la que se encuentra la pieza.
 * @param y             La posición \c y en la que se encuentra la pieza.
 * @return              \c true si la pieza puede hacer un \c spin; de 
 *                      lo contrario, \c false.
 */
static int puede_spin(uint16_t *area_de_juego, uint64_t pieza, int x, int y) {
    if (puede_mover(area_de_juego, pieza, x, y - 1))
        return true;
    return false;
}



/**
 * Sets the piece movement variables to the right values based on user input.
 * 
 * Pieces will fall/snap faster (soft drop) when the user is pressing \c DOWN.
 * Pieces will move left or right based on user input.
 * 
 * @param game  A \c MONSTRO_TGAME struct representing the current game.
 */
static void handle_inputs(MONSTRO_TGAME *game) {
    game->drop_index = 1;                           // Restaura el incremento del contador de velocidad de caída predeterminado
    game->snap_index = 1;                           // Restaura el incremento del contador de velocidad de anclaje predeterminado
    if (game->inputs & MONSTRO_TINPUT_DOWN && game->drop_default > 0) {
        game->drop_index = game->drop_default;      // Las piezas caerán más rápido cuando la tecla INPUT_DOWN esté presionada
        game->snap_index = game->snap_default;      // Las piezas se anclarán más rápido cuando la tecla INPUT_DOWN esté presionada
    }
    
    if (!(game->inputs & (MONSTRO_TINPUT_LEFT | MONSTRO_TINPUT_RIGHT))) {
        game->move_index = 0;                           // Restaura en incremento del contador de velocidad de movimiento horizontal predeterminado
        game->move_count = game->move_default;          // Asigna el contador de movimiento a MOVE_LIMIT de forma que el primer movmiento horizontal responda automáticamente
        // game->move_speed = MONSTRO_TMOVE_LIMIT;         // This line is no longer necessary as move_speed should never by modified for player input
    }
    else if (game->move_index == 0)
        game->move_index = 4;                           // Asigna un incremento para el contador de velocidad horizontal distinto de cero
}



/**
 * Handles horizontal movement of the current piece.
 * 
 * Horizontal, vertical and rotation movement are typically handled separately. 
 * The order in which these movements are evaluated and the actions performed 
 * for each one of the evaluations can affect gameplay.
 * 
 * @param game  A \c MONSTRO_TGAME struct representing the current game.
 */
static void horizontal_movement(MONSTRO_TGAME *game) {
    uint64_t piece = piezas[game->piece][game->rotation];
    int ox = game->x;                                               // Almacena la posición actual de la pieza
    
    game->move_count += game->move_index;                           // Incrementa el contador de velocidad
    if (game->move_count > game->move_default) {                    // Si el contador ha alcanzado el valor actual para la velocidad...
        game->x += (game->inputs & MONSTRO_TINPUT_LEFT) ? 1 : -1;   // incrementa el valor de x de acuerdo con la tecla que esté presionada y...
        game->move_index *= (game->move_index < 32) ? 2.5 : 1;        // reduce el límite para el contador para obtener un movimiento horizontal fluido mientras se mantenga presionada una dirección
        game->move_count = 0;                                       // Reinicia el contador
    }
    
    if (!puede_mover(game->playfield, piece, game->x, game->y))     // Si la pieza no se puede mover a la nueva posición...
        game->x = ox;                                               // mantener la posición original
}



/**
 * Handles vertical movement of the current piece.
 * 
 * Horizontal, vertical and rotation movement are typically handled separately. 
 * The order in which these movements are evaluated and the actions performed 
 * for each one of the evaluations can affect gameplay.
 * 
 * @param game  A \c MONSTRO_TGAME struct representing the current game.
 */
static void vertical_movement(MONSTRO_TGAME *game) {
    game->drop_count += game->drop_index;
    if (game->drop_count > game->drop_default) game->y--;
}



/**
 * Handles rotation movement of the current piece.
 * 
 * Horizontal, vertical and rotation movement are typically handled separately. 
 * The order in which these movements are evaluated and the actions performed 
 * for each one of the evaluations can affect gameplay.
 * 
 * In this particular implementation, the rotation logic is the most complex, 
 * partly due to the addition of the ability to perform wall kicks, 
 * floor kicks and basic spins.
 * 
 * @param game  A \c MONSTRO_TGAME struct representing the current game.
 */
static void rotation_movement(MONSTRO_TGAME *game) {
    int rotation_candidate = (game->inputs & MONSTRO_TINPUT_ROTATE_LEFT) ? game->rotation - 1 : game->rotation + 1;
    rotation_candidate = (rotation_candidate + 4) % 4;
    uint64_t piece_candidate = piezas[game->piece][rotation_candidate];
    
    if (puede_mover(game->playfield, piece_candidate, game->x, game->y)) { 
    // If the piece has started to snap (it hit the bottom), reset the snap and drop count, 
    // otherwise do nothing; this prevents from making the piece float in mid air if snap count and drop count 
    // are reset for a free-fall piece
        game->flags |= (game->inputs & MONSTRO_TINPUT_ROTATE_LEFT) ? MONSTRO_TACTION_ROTATE_LEFT : MONSTRO_TACTION_ROTATE_RIGHT;
        game->rotation = rotation_candidate;
        if (game->snap_count > 0) {
            game->snap_count = 0;
            game->drop_count = 0;
        }
        
        return;
    }
    
// If we got here, then the piece couldn't rotate freely so we might need to either wall kick, floor kick or spin
    if (game->piece == _O_) return;   // The O piece can't (doesn't need to) wall kick or floor kick
    
    int ajuste = puede_wallkick(game->playfield, piece_candidate, game->x, game->y);
    if (ajuste != 0) {                            // The piece can wall kick
        game->flags |= MONSTRO_TACTION_WALL_KICK;
        game->rotation = rotation_candidate;
        piece_candidate = piezas[game->piece][game->rotation];
        game->x += ajuste;
        game->snap_count = 0;                     // Reset snap count but not drop count
        if (puede_spin(game->playfield, piece_candidate, game->x, game->y) && game->snap_count > 0) {
            game->flags = MONSTRO_TACTION_SPIN;
            game->y--;
        }
        return;
    }
    
// La pieza no pudo rotar normalmente ni con 'wall kick', una última opción es intentar un 'floor kick'

// La pieza I requiere verificar una condición especial cuando está en Y = -1 antes de intentar hacer un 'floor kick'
    if (game->piece == _I_ && game->snap_count > 0 && game->rotation == 0) {
        rotation_candidate = (rotation_candidate + 2) % 4;
        game->rotation = 2;
        game->y += 2;
        game->snap_count = 0;
        piece_candidate = piezas[game->piece][rotation_candidate];
    }
    
    int i = (game->snap_count > 0) ? 1 : 0;
    if (puede_floorkick(game->playfield, piece_candidate, game->x, game->y + i)) {
        game->flags = MONSTRO_TACTION_FLOOR_KICK;
        game->rotation = rotation_candidate;
        game->y += (game->snap_count > 0) ? 2 : 1;
        game->snap_count = 0;
        game->drop_count = 0;
    }
}



/**
 * Performs the game logic.
 * 
 * This is the main logic function. Users of this particular logic 
 * implementaion should typically only call this function and not any 
 * of the other functions defined here, with the exception of 
 * spawn_piece() for game initalization.
 * 
 * @param game  A \c MONSTRO_TGAME struct representing the current game.
 */
void mover_pieza(MONSTRO_TGAME *game) {
    uint64_t piece = piezas[game->piece][game->rotation];
    int ox = game->x, oy = game->y;                     // Almacena la posición actual de la pieza
    
    game->flags = 0;
    borrar_pieza(game->playfield, piece, game->x, game->y);
    
    handle_inputs(game);
    horizontal_movement(game);
    vertical_movement(game);
    if (game->inputs & (MONSTRO_TINPUT_ROTATE_LEFT | MONSTRO_TINPUT_ROTATE_RIGHT))
        rotation_movement(game);
    game->inputs &= ~(MONSTRO_TINPUT_ROTATE_LEFT | MONSTRO_TINPUT_ROTATE_RIGHT);    // Reset inputs
    
// After all the logic is handled, all left is to verify the piece can be placed on 
// the playfield and then proceed to actually place it in its new position
    piece = piezas[game->piece][game->rotation];
#ifdef MONSTRO_TWANT_COLORS
    game->current_piece = piece;
#endif
    if (!puede_mover(game->playfield, piece, game->x, game->y)) {
        game->y = oy; 
        game->snap_count += game->snap_index;
    }
    poner_pieza(game->playfield, piece, game->x, game->y);
    
// If the piece moved vertically, snap and drop counters are reset
    if (game->y != oy) {
        game->drop_count = 0;
        game->snap_count = 0;
        game->flags |= MONSTRO_TACTION_DROP;
    }
// If the piece moved horizontally, move counter is reset
    if (game->x != ox) {
        game->move_count = 0;
        game->flags |= MONSTRO_TACTION_MOVE;
    }
// If snap counter reached its limit, the piece effectively has locked, 
// so proceed to clear completed lines and spawn a new piece
    if (game->snap_count > game->snap_default) {
        game->flags |= MONSTRO_TACTION_SNAP;
        game->flags |= MONSTRO_TACTION_SPAWN;
    // Flag completed lines
        int completed = 0;
        game->playfield[0] = 0x7FFF;      // safeguard
        completed |=     (game->playfield[game->y] == 0xFFFF) ? MONSTRO_TACTION_CLEARED0 : 0;
        completed |= (game->playfield[game->y + 1] == 0xFFFF) ? MONSTRO_TACTION_CLEARED1 : 0;
        completed |= (game->playfield[game->y + 2] == 0xFFFF) ? MONSTRO_TACTION_CLEARED2 : 0;
        completed |= (game->playfield[game->y + 3] == 0xFFFF) ? MONSTRO_TACTION_CLEARED3 : 0;
        game->playfield[0] = 0xFFFF;      // safeguard
        game->flags |= completed;
    // Clear completed lines from the playfield
    // TODO: Maybe borrar_completas() can be called from the main game loop in
    //       response to the flags, just like spawn_piece() in recent versions ???
        borrar_completas(game->playfield, game->y);
    }
}



/**
 * Spawns a new piece into the game.
 * 
 * @param game  A \c MONSTRO_TGAME struct representing the current game.
 */
int spawn_piece(MONSTRO_TGAME *game) {
    game->piece = rand() % 7;
    game->rotation = rand() % 4;
    game->x = 6;
    game->y = 20;
    game->drop_count = 0;
    game->snap_count = 0;
    uint64_t piece = piezas[game->piece][game->rotation];
#ifdef MONSTRO_TWANT_COLORS
    game->current_piece = piece;
#endif
    if (puede_mover(game->playfield, piece, game->x, game->y)) {
        poner_pieza(game->playfield, piece, game->x, game->y);
        return true;
    }
    
    return false;
}
