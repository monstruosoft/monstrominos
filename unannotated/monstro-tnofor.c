#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <GL/gl.h>

#define BLOCK_SIZE                         32
#define MONSTRO_TFIELD_SIZE                24

#define MONSTRO_TINPUT_UP                   1
#define MONSTRO_TINPUT_DOWN                 2
#define MONSTRO_TINPUT_LEFT                 4
#define MONSTRO_TINPUT_RIGHT                8
#define MONSTRO_TINPUT_ROTATE_LEFT         16
#define MONSTRO_TINPUT_ROTATE_RIGHT        32

#define MONSTRO_TMOVE_LIMIT                64
#define MONSTRO_TDROP_LIMIT                64
#define MONSTRO_TSNAP_LIMIT                65

#define MONSTRO_TACTION_MOVE              0x1
#define MONSTRO_TACTION_DROP              0x2
#define MONSTRO_TACTION_ROTATE_LEFT       0x4
#define MONSTRO_TACTION_ROTATE_RIGHT      0x8
#define MONSTRO_TACTION_WALL_KICK        0x10
#define MONSTRO_TACTION_FLOOR_KICK       0x20
#define MONSTRO_TACTION_SPIN             0x40
#define MONSTRO_TACTION_SNAP             0x80
#define MONSTRO_TACTION_CLEARED         0xF00
#define MONSTRO_TACTION_CLEARED0        0x100
#define MONSTRO_TACTION_CLEARED1        0x200
#define MONSTRO_TACTION_CLEARED2        0x400
#define MONSTRO_TACTION_CLEARED3        0x800
#define MONSTRO_TACTION_SPAWN          0x1000

typedef struct {
    uint16_t playfield[MONSTRO_TFIELD_SIZE];
    int piece;
    int rotation;
    int x, y;
    int inputs;
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
} MONSTRO_TGAME;

// Piezas:
static enum {_I_, _O_, _T_, _J_, _L_, _S_, _Z_};
static const uint64_t I[] = {0xF00000000, 0x2000200020002, 0xF0000, 0x4000400040004};
static const uint64_t O[] = {0x600060000, 0x600060000, 0x600060000, 0x600060000};
static const uint64_t T[] = {0x200070000, 0x200030002, 0x70002, 0x200060002};
static const uint64_t J[] = {0x400070000, 0x300020002, 0x70001, 0x200020006};
static const uint64_t L[] = {0x100070000, 0x200020003, 0x70004, 0x600020002};
static const uint64_t S[] = {0x300060000, 0x200030001, 0x30006, 0x400060002};
static const uint64_t Z[] = {0x600030000, 0x100030002, 0x60003, 0x200060004};
static const uint64_t *piezas[] = {I, O, T, J, L, S, Z};

ALLEGRO_EVENT_QUEUE *events = NULL;
ALLEGRO_DISPLAY *display = NULL;
ALLEGRO_TIMER *timer = NULL;
ALLEGRO_EVENT event;

MONSTRO_TGAME game = { .playfield = { 0xFFFF, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 
                                      0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 
                                      0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 
                                      0xE007, 0xE007, 0xE007, 0xE007, 0xE007, 0xE007 }, 
                      .snap_default = MONSTRO_TSNAP_LIMIT, .snap_index = 1, 
                      .drop_default = MONSTRO_TDROP_LIMIT, .drop_index = 1, 
                      .move_default = MONSTRO_TMOVE_LIMIT, .move_index = 1};
bool game_over = false;
int total_lines = 0;
bool redraw = true;

void poner_pieza(uint16_t *area_de_juego, uint64_t pieza, int x, int y) {
    uint64_t *destino = (uint64_t *)&area_de_juego[y];
    uint64_t dato = pieza << x;
    *destino |= dato;
}

void borrar_pieza(uint16_t *area_de_juego, uint64_t pieza, int x, int y) {
    uint64_t *destino = (uint64_t *)&area_de_juego[y];
    uint64_t dato = pieza << x;
    *destino ^= dato;
}

int puede_mover(uint16_t *area_de_juego, uint64_t pieza, int x, int y) {
    uint64_t *destino = (uint64_t *)&area_de_juego[y];
    uint64_t dato = pieza << x;
    uint64_t resultado = *destino | dato;
    resultado = resultado ^ dato;
    if (resultado == *destino) return true;
    return false;
}

void borrar_completas(uint16_t *area_de_juego, int y) {
    uint16_t *origen = (uint16_t *)&area_de_juego[y];
    uint64_t destino = 0;
    int i = 0;
    
    area_de_juego[0] = 0x7FFF;
    
    destino |= ((uint16_t)~*origen) ? (uint64_t)*origen << i++ * 16 : 0; origen++;
    destino |= ((uint16_t)~*origen) ? (uint64_t)*origen << i++ * 16 : 0; origen++;
    destino |= ((uint16_t)~*origen) ? (uint64_t)*origen << i++ * 16 : 0; origen++;
    destino |= ((uint16_t)~*origen) ? (uint64_t)*origen << i++ * 16 : 0; origen++;
    origen = &area_de_juego[y];
    *(uint64_t *)origen = destino;
    memmove(&area_de_juego[y + i], &area_de_juego[y + 4], (20 - y) * 2);
    
    area_de_juego[0] = 0xFFFF;
}

static int puede_wallkick(uint16_t *area_de_juego, uint64_t pieza, int x, int y) {
    if (puede_mover(area_de_juego, pieza, x - 1, y))
        return -1;
    else if (puede_mover(area_de_juego, pieza, x + 1, y))
        return 1;
    return 0;
}

static int puede_floorkick(uint16_t *area_de_juego, uint64_t pieza, int x, int y) {
    if (puede_mover(area_de_juego, pieza, x, y + 1))
        return true;
    return false;
}

static int puede_spin(uint16_t *area_de_juego, uint64_t pieza, int x, int y) {
    if (puede_mover(area_de_juego, pieza, x, y - 1))
        return true;
    return false;
}

static void handle_inputs(MONSTRO_TGAME *game) {
    game->drop_index = 1;
    game->snap_index = 1;
    if (game->inputs & MONSTRO_TINPUT_DOWN && game->drop_default > 0) {
        game->drop_index = game->drop_default;
        game->snap_index = game->snap_default;
    }
    
    if (!(game->inputs & (MONSTRO_TINPUT_LEFT | MONSTRO_TINPUT_RIGHT))) {
        game->move_index = 0;
        game->move_count = game->move_default;
    }
    else if (game->move_index == 0)
        game->move_index = 4;
}

static void horizontal_movement(MONSTRO_TGAME *game) {
    uint64_t piece = piezas[game->piece][game->rotation];
    int ox = game->x;
    
    game->move_count += game->move_index;
    if (game->move_count > game->move_default) {
        game->x += (game->inputs & MONSTRO_TINPUT_LEFT) ? 1 : -1;
        game->move_index *= (game->move_index < 32) ? 2.5 : 1;
        game->move_count = 0;
    }
    
    if (!puede_mover(game->playfield, piece, game->x, game->y))
        game->x = ox;
}

static void vertical_movement(MONSTRO_TGAME *game) {
    game->drop_count += game->drop_index;
    if (game->drop_count > game->drop_default) game->y--;
}

static void rotation_movement(MONSTRO_TGAME *game) {
    int rotation_candidate = (game->inputs & MONSTRO_TINPUT_ROTATE_LEFT) ? game->rotation - 1 : game->rotation + 1;
    rotation_candidate = (rotation_candidate + 4) % 4;
    uint64_t piece_candidate = piezas[game->piece][rotation_candidate];
    
    if (puede_mover(game->playfield, piece_candidate, game->x, game->y)) { 
       game->flags |= (game->inputs & MONSTRO_TINPUT_ROTATE_LEFT) ? MONSTRO_TACTION_ROTATE_LEFT : MONSTRO_TACTION_ROTATE_RIGHT;
        game->rotation = rotation_candidate;
        if (game->snap_count > 0) {
            game->snap_count = 0;
            game->drop_count = 0;
        }
        
        return;
    }
    
    if (game->piece == _O_) return;
    
    int ajuste = puede_wallkick(game->playfield, piece_candidate, game->x, game->y);
    if (ajuste != 0) {
        game->flags |= MONSTRO_TACTION_WALL_KICK;
        game->rotation = rotation_candidate;
        piece_candidate = piezas[game->piece][game->rotation];
        game->x += ajuste;
        game->snap_count = 0;
        if (puede_spin(game->playfield, piece_candidate, game->x, game->y) && game->snap_count > 0) {
            game->flags = MONSTRO_TACTION_SPIN;
            game->y--;
        }
        return;
    }
    
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

void mover_pieza(MONSTRO_TGAME *game) {
    uint64_t piece = piezas[game->piece][game->rotation];
    int ox = game->x, oy = game->y;
    
    game->flags = 0;
    borrar_pieza(game->playfield, piece, game->x, game->y);
    
    handle_inputs(game);
    horizontal_movement(game);
    vertical_movement(game);
    if (game->inputs & (MONSTRO_TINPUT_ROTATE_LEFT | MONSTRO_TINPUT_ROTATE_RIGHT))
        rotation_movement(game);
    game->inputs &= ~(MONSTRO_TINPUT_ROTATE_LEFT | MONSTRO_TINPUT_ROTATE_RIGHT);
    
    piece = piezas[game->piece][game->rotation];
    if (!puede_mover(game->playfield, piece, game->x, game->y)) {
        game->y = oy; 
        game->snap_count += game->snap_index;
    }
    poner_pieza(game->playfield, piece, game->x, game->y);
    
    if (game->y != oy) {
        game->drop_count = 0;
        game->snap_count = 0;
        game->flags |= MONSTRO_TACTION_DROP;
    }
    if (game->x != ox) {
        game->move_count = 0;
        game->flags |= MONSTRO_TACTION_MOVE;
    }
    if (game->snap_count > game->snap_default) {
        game->flags |= MONSTRO_TACTION_SNAP;
        game->flags |= MONSTRO_TACTION_SPAWN;
        int completed = 0;
        game->playfield[0] = 0x7FFF;
        completed |=     (game->playfield[game->y] == 0xFFFF) ? MONSTRO_TACTION_CLEARED0 : 0;
        completed |= (game->playfield[game->y + 1] == 0xFFFF) ? MONSTRO_TACTION_CLEARED1 : 0;
        completed |= (game->playfield[game->y + 2] == 0xFFFF) ? MONSTRO_TACTION_CLEARED2 : 0;
        completed |= (game->playfield[game->y + 3] == 0xFFFF) ? MONSTRO_TACTION_CLEARED3 : 0;
        game->playfield[0] = 0xFFFF;
        game->flags |= completed;
        borrar_completas(game->playfield, game->y);
    }
}

int spawn_piece(MONSTRO_TGAME *game) {
    game->piece = rand() % 7;
    game->rotation = rand() % 4;
    game->x = 6;
    game->y = 20;
    game->drop_count = 0;
    game->snap_count = 0;
    uint64_t piece = piezas[game->piece][game->rotation];
    if (puede_mover(game->playfield, piece, game->x, game->y)) {
        poner_pieza(game->playfield, piece, game->x, game->y);
        return true;
    }
    
    return false;
}

static void draw_opengl(MONSTRO_TGAME *game) {
    int width = 16 * BLOCK_SIZE, height = 20 * BLOCK_SIZE;

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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    int y_offset = 4 * BLOCK_SIZE;
    
    glBegin(GL_QUADS);
        glTexCoord2f(0.5, 0); glVertex2f(0, -y_offset); 
        glTexCoord2f(1.5, 0); glVertex2f(width, -y_offset); 
        glTexCoord2f(1.5, -1); glVertex2f(width, height); 
        glTexCoord2f(0.5, -1); glVertex2f(0, height);
    glEnd();

    glFlush();
    glActiveTexture(GL_TEXTURE0);
}

void logic(MONSTRO_TGAME *game, ALLEGRO_EVENT *event) {
    if (event->type == ALLEGRO_EVENT_TIMER) {
        mover_pieza(game);
        if (game->flags & MONSTRO_TACTION_SPAWN) {
            game_over = !spawn_piece(game);
            if (game_over) printf("GAME OVER!\n");
        }
        if (game->flags & MONSTRO_TACTION_CLEARED) {
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

void update() {
    al_clear_to_color(al_map_rgb(64, 64, 128));
    draw_opengl(&game);
}

void initialization() {
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
    
    spawn_piece(&game);
}

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

