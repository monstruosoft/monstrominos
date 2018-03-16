/**
 * @file monstro-tallegro5.c
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
 * Sample Allegro 5 implementation.
 */

#include <stdint.h>
#include <string.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#ifdef MONSTRO_TWANT_OPENGL
#include <GL/gl.h>
#endif
#include "monstro-tlogic.h"



#define BLOCK_SIZE      32



// Allegro global variables
ALLEGRO_EVENT_QUEUE *events = NULL;
ALLEGRO_DISPLAY *display = NULL;
ALLEGRO_TIMER *timer = NULL;
ALLEGRO_EVENT event;

// Game global variables
MONSTRO_TGAME game = { .playfield = { 0xFFFF, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 
                                      0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 
                                      0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 
                                      0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007 }, 
                      .snap_default = MONSTRO_TSNAP_LIMIT, .snap_index = 1, 
                      .drop_default = MONSTRO_TDROP_LIMIT, .drop_index = 1, 
                      .move_default = MONSTRO_TMOVE_LIMIT, .move_index = 1};
ALLEGRO_COLOR colors[8];
bool game_over = false;
int total_lines = 0;
bool redraw = true;



/**
 * Draws a single block.
 * 
 * @param x     The X coordinate where the block will be drawn.
 * @param y     The Y coordinate where the block will be drawn.
 * @param color The color de block will be drawn with.
 */
void draw_block(int x, int y, ALLEGRO_COLOR color) {
    al_draw_filled_rectangle(x, y, x + BLOCK_SIZE, y + BLOCK_SIZE, color);
    al_draw_rectangle(x, y, x + BLOCK_SIZE, y + BLOCK_SIZE, al_map_rgb(0, 0, 0), 2);
    al_draw_rectangle(x + 5, y + 5, x + BLOCK_SIZE - 8, y + BLOCK_SIZE - 8, al_map_rgb(0, 0, 0), 1);
    al_draw_filled_rectangle(x + 5, y + 5, x + BLOCK_SIZE - 8, y + BLOCK_SIZE - 8, al_map_rgba(192, 192, 192, 192));
}



/**
 * Draws the playfield when using the 1bpp version of the game, that is
 * when \c MONSTRO_TWANT_COLORS is not defined at compile time.
 * 
 * @param game A \c MONSTRO_TGAME struct representing the current game.
 */
void draw_playfield(MONSTRO_TGAME *game) {
    for (int y = 0; y < MONSTRO_TFIELD_SIZE - 4; y++)
        for (int x = 0; x < 16; x++)
            if (game->playfield[y] & (1 << x))
            // Dibuja de un color distinto las celdas fijas del tablero, este es el tipo de coloreado que se puede utilizar 
            // para darle variedad de color al juego usando simplemente la informacion básica proporcionada por la variable 
            // playfield[]
                if (x < 3 || x > 12 || y == 0)
                    draw_block((15 - x) * BLOCK_SIZE, (19 - y) * BLOCK_SIZE, colors[7]);
                else draw_block((15 - x) * BLOCK_SIZE, (19 - y) * BLOCK_SIZE, colors[0]);
}



/**
 * Draws the color version of the playfield; \c MONSTRO_TWANT_COLORS must 
 * be defined at compile time, otherwise this function does nothing.
 * 
 * @param game A \c MONSTRO_TGAME struct representing the current game.
 */
void draw_color_playfield(MONSTRO_TGAME *game) {
    int color;

#ifdef MONSTRO_TWANT_COLORS    
    for (int y = 0; y < MONSTRO_TFIELD_SIZE - 4; y++)
        for (int x = 0; x < 16; x++) {
            color = game->color_playfield[y][x];
            if (color != -1)
                draw_block((15 - x) * BLOCK_SIZE, (19 - y) * BLOCK_SIZE, colors[color]);
        }
    
// Use a dummy playfield to place and draw the current piece.
    static uint16_t dummy[24] = {0};
    color = game->piece;
    poner_pieza(dummy, game->current_piece, game->x, game->y);
    for (int y = 0; y < MONSTRO_TFIELD_SIZE; y++)
        for (int x = 0; x < 16; x++)
            if (dummy[y] & (1 << x))
                draw_block((15 - x) * BLOCK_SIZE, (19 - y) * BLOCK_SIZE, colors[color]);
    borrar_pieza(dummy, game->current_piece, game->x, game->y);
#endif
}



/**
 * Draws the 1bpp OpenGL version of the playfield.
 *
 * @param game A \c MONSTRO_TGAME struct representing the current game.
 */
static void draw_opengl(MONSTRO_TGAME *game) {
    int width = 16 * BLOCK_SIZE, height = 20 * BLOCK_SIZE;

#ifdef MONSTRO_TWANT_OPENGL
    glEnable(GL_TEXTURE_2D);    
    GLuint textures[2];
    glGenTextures(2, textures);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    float index[] = {0.0, 1.0};

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 2, index);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 2, index);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 2, index);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 2, index);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 24, 0, GL_COLOR_INDEX, GL_BITMAP, game->playfield);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    int y_offset = 4 * BLOCK_SIZE;   // Top 4 rows are not visible
    
    glBegin(GL_QUADS);
        glTexCoord2f(0.5, 0); glVertex2f(0, -y_offset); 
        glTexCoord2f(1.5, 0); glVertex2f(width, -y_offset); 
        glTexCoord2f(1.5, -1); glVertex2f(width, height); 
        glTexCoord2f(0.5, -1); glVertex2f(0, height);
    glEnd();

    glFlush();
    glActiveTexture(GL_TEXTURE0);
#endif
}



/*
 * Game logic.
 */
void logic(MONSTRO_TGAME *game, ALLEGRO_EVENT *event) {
    if (event->type == ALLEGRO_EVENT_TIMER) {
        mover_pieza(game);
#ifdef MONSTRO_TWANT_COLORS
        update_color_playfield(game);
#endif
        if (game->flags & MONSTRO_TACTION_SPAWN) {
            game_over = !spawn_piece(game);
            if (game_over) printf("GAME OVER!\n");
        }
        if (game->flags & MONSTRO_TACTION_CLEARED) {
        // Count cleared lines and increase speed every few lines
            total_lines += (game->flags & MONSTRO_TACTION_CLEARED0) ? 1 : 0;
            total_lines += (game->flags & MONSTRO_TACTION_CLEARED1) ? 1 : 0;
            total_lines += (game->flags & MONSTRO_TACTION_CLEARED2) ? 1 : 0;
            total_lines += (game->flags & MONSTRO_TACTION_CLEARED3) ? 1 : 0;
            if (total_lines > 10) {
                total_lines -= 10;
                game->drop_default /= 2;
                game->snap_default -= game->snap_default / 8;
            }
        }
        redraw = true;
    }
    else if (event->any.source == al_get_keyboard_event_source()) {
        if (event->type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event->keyboard.keycode == ALLEGRO_KEY_DOWN)
                game->inputs |= MONSTRO_TINPUT_DOWN;
            if (event->keyboard.keycode == ALLEGRO_KEY_LEFT)
                game->inputs |= MONSTRO_TINPUT_LEFT;
            if (event->keyboard.keycode == ALLEGRO_KEY_RIGHT)
                game->inputs |= MONSTRO_TINPUT_RIGHT;
            if (event->keyboard.keycode == ALLEGRO_KEY_Z || event->keyboard.keycode == ALLEGRO_KEY_SPACE)
                game->inputs |= MONSTRO_TINPUT_ROTATE_LEFT;
            if (event->keyboard.keycode == ALLEGRO_KEY_X)
                game->inputs |= MONSTRO_TINPUT_ROTATE_RIGHT;
        }
        if (event->type == ALLEGRO_EVENT_KEY_UP) {
            if (event->keyboard.keycode == ALLEGRO_KEY_DOWN)
                game->inputs &= ~MONSTRO_TINPUT_DOWN;
            if (event->keyboard.keycode == ALLEGRO_KEY_LEFT)
                game->inputs &= ~MONSTRO_TINPUT_LEFT;
            if (event->keyboard.keycode == ALLEGRO_KEY_RIGHT)
                game->inputs &= ~MONSTRO_TINPUT_RIGHT;
            if (event->keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                game_over = true;
        }
    }
}



/*
 * Screen update.
 */
void update() {
    al_clear_to_color(al_map_rgb(64, 64, 128));
#ifdef MONSTRO_TWANT_COLORS
    draw_color_playfield(&game);
#elif MONSTRO_TWANT_OPENGL
    draw_opengl(&game);
#else
    draw_playfield(&game);
#endif
}


/*
 * Game initialization.
 */
void initialization() {
// Allegro initialization
    assert(al_init());
    assert(al_install_keyboard());
    al_set_new_display_flags(ALLEGRO_WINDOWED);
    al_set_new_window_title("monstrominos by monstrochan");
    display = al_create_display(16 * BLOCK_SIZE, 20 * BLOCK_SIZE);
    assert(display);
    assert(al_init_primitives_addon());

    events = al_create_event_queue();
    assert(events);
    timer = al_create_timer(ALLEGRO_BPS_TO_SECS(30));
    assert(timer);
    al_start_timer(timer);
    al_register_event_source(events, al_get_keyboard_event_source());
    al_register_event_source(events, al_get_timer_event_source(timer));
    
    srand(time(NULL));
    
// Inicialización del tablero
    colors[0] = al_map_rgb(255, 0, 0);
    colors[7] = al_map_rgb(0, 128, 0);      // Playfield walls' color
#ifdef MONSTRO_TWANT_COLORS
    colors[0] = al_map_rgb(  0, 255, 255);
    colors[1] = al_map_rgb(255, 255,   0);
    colors[2] = al_map_rgb(170,   0, 255);
    colors[3] = al_map_rgb(  0,   0, 255);
    colors[4] = al_map_rgb(255, 165,   0);
    colors[5] = al_map_rgb(  0, 255,   0);
    colors[6] = al_map_rgb(255,   0,   0);
    colors[7] = al_map_rgb(255, 128, 192);  // Playfield walls' color
    init_color_playfield(&game);
#endif
    spawn_piece(&game);
}



/*
 * Game loop.
 */
int main() {
    initialization();
    
    while (!game_over) {
        al_wait_for_event(events, &event);
        logic(&game, &event);
        if (redraw && al_is_event_queue_empty(events)) {
            update();
            al_flip_display();
            redraw = false;
        }  
    }
} 
