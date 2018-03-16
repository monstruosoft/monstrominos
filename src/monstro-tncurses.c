/**
 * @file monstro-tncurses.c
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
 * Sample ncurses implementation.
 */

#include <time.h>
#include <stdlib.h>
#include <ncurses.h> 
#include "monstro-tlogic.h"



#define KEY_SPACE    32
#define COLOR_ORANGE 16



MONSTRO_TGAME game = { .playfield = { 0xFFFF, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 
                                      0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 
                                      0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 
                                      0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007 }, 
                        .snap_default = MONSTRO_TSNAP_LIMIT, .snap_index = 1, 
                        .drop_default = MONSTRO_TDROP_LIMIT, .drop_index = 1, 
                        .move_default = MONSTRO_TMOVE_LIMIT, .move_index = 1};
int total_lines = 0;
int game_over = false;



/*
 * Draws the playfield using ncurses.
 */
void draw_playfield(MONSTRO_TGAME *game) {
    static uint16_t previous[24] = {0};
    int color;
    
    for (int y = 0; y < 20; y++)
        for (int x = 0; x < 16; x++)
#ifdef MONSTRO_TWANT_COLORS
            if (has_colors() && game->color_playfield[y][x] != -1) {
                attron(COLOR_PAIR(game->color_playfield[y][x] + 1));
                mvprintw(20 - y, x * 2, "[]");
            }
            
            attron(COLOR_PAIR(game->piece + 1));
            poner_pieza(previous, game->current_piece, game->x, game->y);
            for (int y = 0; y < 20; y++)
                for (int x = 0; x < 16; x++)
                    if (previous[y] & (1 << x))
                        mvprintw(20 - y, x * 2, "[]");
            borrar_pieza(previous, game->current_piece, game->x, game->y);
#else
            if (game->playfield[y] & (1 << x)) {
                if (has_colors() && (x < 3 || x > 12 || y == 0))
                    attroff(A_REVERSE);
                else attron(A_REVERSE);
                mvprintw(20 - y, x * 2, "[]");
            }
#endif
}



/*
 * Game initialization.
 */
void initialization() {
// ncurses initialization
    srand(time(NULL));
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    timeout(15);
    
// Init color playfield support
#ifdef MONSTRO_TWANT_COLORS
    // attron(A_BOLD);
    if (has_colors()) {
        start_color();
        
        init_pair(1, COLOR_BLACK, COLOR_CYAN);
        init_pair(2, COLOR_BLACK, COLOR_YELLOW);
        init_pair(3, COLOR_WHITE, COLOR_MAGENTA);
        init_pair(5, COLOR_WHITE, COLOR_BLUE);      // If using the inverted X coordiantes, 
        init_pair(4, COLOR_WHITE, COLOR_BLACK);     // also invert, the J/L
        init_pair(7, COLOR_BLACK, COLOR_GREEN);     // and S/Z pieces' colors
        init_pair(6, COLOR_WHITE, COLOR_RED);       // ... if you're obssesive, that is
        init_pair(8, COLOR_BLACK, COLOR_WHITE);
        
        if (can_change_color()) {
        // Manipulating the predefined colors will leave them in the modified state 
        // even after the program quits. Also, COLOR_ORANGE is not a predefined ncurses 
        // color, it was added here to complete the game color set.
            init_color(COLOR_CYAN,      0, 1000, 1000);
            init_color(COLOR_YELLOW, 1000, 1000,    0);
            init_color(COLOR_MAGENTA, 680,    0, 1000);
            init_color(COLOR_BLUE,      0,    0, 1000);
            init_color(COLOR_ORANGE, 1000,  660,    0);
            init_color(COLOR_GREEN,     0, 1000,    0);
            init_color(COLOR_RED,    1000,    0,    0);
            
            init_pair(4, COLOR_BLACK, COLOR_ORANGE);
        }
    }
    init_color_playfield(&game);
#endif
    spawn_piece(&game);
}



/*
 * Game logic.
 */
void logic() {
    int c = getch();
    
    if (c != ERR) {
        if (c == KEY_DOWN)  game.inputs |= MONSTRO_TINPUT_DOWN;
        if (c == KEY_LEFT)  game.inputs |= MONSTRO_TINPUT_RIGHT;
        if (c == KEY_RIGHT) game.inputs |= MONSTRO_TINPUT_LEFT;
    // If using the inverted X coordinates, also invert rotations
        if (c == KEY_SPACE) game.inputs |= MONSTRO_TINPUT_ROTATE_RIGHT;
        if (c == 'z')       game.inputs |= MONSTRO_TINPUT_ROTATE_RIGHT;
        if (c == 'x')       game.inputs |= MONSTRO_TINPUT_ROTATE_LEFT;
        
        if (c == 'q') game_over = 1;
    }
    mover_pieza(&game);
#ifdef MONSTRO_TWANT_COLORS
    update_color_playfield(&game);
#endif
    
    if (game.flags & MONSTRO_TACTION_SPAWN) {
        game_over = !spawn_piece(&game);
        if (game_over) printf("GAME OVER!\n");
    }
    if (game.flags & MONSTRO_TACTION_CLEARED) {
    // Count cleared lines and increase speed every few lines
        total_lines += (game.flags & MONSTRO_TACTION_CLEARED0) ? 1 : 0;
        total_lines += (game.flags & MONSTRO_TACTION_CLEARED1) ? 1 : 0;
        total_lines += (game.flags & MONSTRO_TACTION_CLEARED2) ? 1 : 0;
        total_lines += (game.flags & MONSTRO_TACTION_CLEARED3) ? 1 : 0;
        if (total_lines > 10) {
            total_lines -= 10;
            game.drop_default /= 2;
            game.snap_default -= game.snap_default / 8;
        }
    }
}



/*
 * Screen update.
 */
void update() {
    clear();
    draw_playfield(&game);
    
    attroff(COLOR_PAIR);
    // mvprintw(0, 0, "KEY %d", c);
    mvprintw(1, 33, "¡¡¡monstrominos by monstrochan!!!");
    mvprintw(2, 33, "---------------------------------");
    mvprintw(3, 33, "Version consola para monstros con");
    mvprintw(4, 33, "PCs cuanticas  peruanas porque el");
    mvprintw(5, 33, "monstro siempre al servicio de la");
    mvprintw(6, 33, "comunidad.");
    
    refresh();
}



/*
 * Game loop.
 */
int main() {
    initialization();
    
    while (!game_over) {
        logic();
        update();
        napms(15);
        game.inputs = 0;
    }
    endwin();

    return 0;
}
