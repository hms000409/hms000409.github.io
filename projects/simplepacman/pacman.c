/* Window는 아래 주석 5줄 해제 */
// #define SOKOL_IMPL
// #define SOKOL_GLCORE
// #define SOKOL_WIN32_FORCE_MAIN
// #include "sokol/sokol_gfx.h"
#include "sokol/sokol_app.h"
// #include "sokol/sokol_glue.h"

#include "sokol/sokol_log.h"

#include "graphic.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#define MAP_HEIGHT 14
#define MAP_WIDTH 20
#define NUM_GHOSTS 2
#define INITIAL_LIVES 3
#define INFO_OFFSET 4
#define POWER_DURATION 50

// Map elements
#define WALL '#'
#define EMPTY ' '
#define DOT '.'
#define POWERUP 'O'

typedef enum {
    RIGHT,
    LEFT,
    UP,
    DOWN
} Dir;

const int x_dir[4] = { 1, -1,  0,  0 };
const int y_dir[4] = { 0,  0, -1,  1 };

// Position
typedef struct
{
    int x, y;
} Position;

// Player
typedef struct
{
    Position pos;
    int lives;
    int score;
    int direction; // 0:right, 1:left, 2:up, 3:down
    int powered;
    int key;
} Player;

// Ghost
typedef struct
{
    Position pos;
    int scared;
    int color;
    int inside;    // 1: inside ghost house, 0: outside
    int returning; // 1: returning to ghost house as eyes
    int direction; // 0:right, 1:left, 2:up, 3:down
} Ghost;

// Ghost bounce direction for inside movement
static int ghost_bounce_dir[NUM_GHOSTS] = {1, -1};

// Global variables
char map[MAP_HEIGHT][MAP_WIDTH];          // Game logic: WALL, DOT, EMPTY, POWERUP
uint8_t video_ram[MAP_HEIGHT][MAP_WIDTH]; // Tile codes for rendering
Player player;
Ghost ghosts[NUM_GHOSTS];
int highscore = 0;
int total_dots = 0;
int eaten_dots = 0;
int frame_count = 0;
int level = 1;
int next_direction = -1; // Next direction player wants to go (-1: none)
int level_flash = 0;     // Flash timer for level up effect
#define num_fruit 3
#define num_egg 2
Position fruit_pos[num_fruit];
int fruit_kind[num_fruit];
int fruit_eaten[num_fruit];
Position key_pos;
int take_key;
Position secret_pos;
int egg;
Position egg_pos[num_egg];
int egg_eaten[num_egg];
int change;


// Game state
typedef enum
{
    STATE_INTRO,   // "Press any key to start!"
    STATE_READY,   // "Ready" 3-second countdown
    STATE_PLAYING, // Game in progress
    STATE_DYING,   // Death animation
    STATE_GAMEOVER // "Game Over"
} GameState;

GameState game_state = STATE_INTRO;
int state_timer = 0;
int death_frame = 0; // Death animation frame (0-11)

// Input state
struct
{
    int up, down, left, right;
    int quit;
} input;

// Pacman shapes for each direction
const uint8_t pacman_tiles[] = {48, 48, 48, 48}; // Closed mouth for now

typedef struct {
    /*
    Position player_pos;
    int player_lives;
    int player_score;
    int player_direction; // 0:right, 1:left, 2:up, 3:down
    int player_powered;

    Position ghost_pos[NUM_GHOSTS];
    int ghost_scared[NUM_GHOSTS];
    int ghost_color[NUM_GHOSTS];
    int ghost_inside[NUM_GHOSTS];    // 1: inside ghost house, 0: outside
    int ghost_returning[NUM_GHOSTS]; // 1: returning to ghost house as eyes
    int ghost_direction[NUM_GHOSTS];
    */

    char map_state[MAP_HEIGHT][MAP_WIDTH];
    uint8_t video_ram_state[MAP_HEIGHT][MAP_WIDTH];
    int eaten_dots;

} MAP;

MAP main_map, secret_map;
int is_secret;

// Level system helpers
int get_player_move_interval(void)
{
    // TODO: 레벨별 플레이어 이동 속도 반환
    // 입력: 전역변수 level 사용
    // 출력: 이동 간격 (프레임 수, 작을수록 빠름)
    // 힌트: 레벨 1은 5, 레벨 2는 4, 레벨 3+는 3
    if (is_secret) {
        return 5;
    }else if (level >= 3) {
        return 3;
    }else if (level==2) {
        return 4;
    }
    return 5;
}

int get_ghost_move_interval(void) {
    // TODO: 레벨별 유령 이동 속도 반환
    // 입력: 전역변수 level 사용
    // 출력: 이동 간격 (레벨 1은 6, 레벨 2는 5, 레벨 3+는 4)
    if (level>=3) {
        return 4;
    } else if (level==2) {
        return 5;
    }
    return 6;
}

int get_power_duration(void)
{
    // TODO: 레벨별 파워업 지속시간 반환
    // 출력: 프레임 수 (레벨 1은 50, 레벨 2는 40, 레벨 3은 30, 레벨 4+는 25)
    if (level >=4) {
        return 25;
    }else if (level ==3) {
        return 30;
    }else if(level==2){
        return 40;
    }
    return 50;
}

void load_highscore(void)
{
    FILE *f = fopen("highscore.txt", "r");
    if (f)
    {
        fscanf(f, "%d", &highscore);
        fclose(f);
    }
}

void save_highscore(void)
{
    if (player.score > highscore)
    {
        FILE *f = fopen("highscore.txt", "w");
        if (f)
        {
            fprintf(f, "%d", player.score);
            fclose(f);
        }
    }
}


void init_map(void)
{
    total_dots = 0;
    eaten_dots = 0;

    // Symmetric ASCII map (20x14)
    static const char *tiles =
        "0UUUUUUUUUUUUUUUUUU1"
        "L                  R"
        "L......      ......R"
        "LPebbf.      .ebbfPR"
        "L.guuh.      .guuh.R"
        "L......  --  ......R"
        "L.ebbf. l  r .ebbf.R"
        "L.guuh. l  r .guuh.R"
        "L...... guuh ......R"
        "LPebbf.      .ebbfPR"
        "L.guuh.      .guuh.R"
        "L......      ......R"
        "L                  R"
        "2BBBBBBBBBBBBBBBBBB3";

    // Create mapping table (ASCII char -> tile code)
    uint8_t t[128];
    for (int i = 0; i < 128; i++)
    {
        t[i] = TILE_DOT; // Default to dot
    }

    // Special tiles (from original pacman.c)
    t[' '] = TILE_SPACE; // 0x40
    t['.'] = TILE_DOT;   // 0x10
    t['0'] = 0xD1;       // Top-left corner
    t['1'] = 0xD0;       // Top-right corner
    t['2'] = 0xD5;       // Bottom-left corner
    t['3'] = 0xD4;       // Bottom-right corner
    t['4'] = 0xFB;       // Top center-left
    t['5'] = 0xFA;       // Top center-right
    t['U'] = 0xDB;       // Horizontal line (top/bottom)
    t['L'] = 0xD3;       // Left vertical
    t['R'] = 0xD2;       // Right vertical
    t['B'] = 0xDC;       // Horizontal line (bottom)
    t['b'] = 0xDF;       // Blob fill
    t['e'] = 0xE7;       // Corner piece
    t['f'] = 0xE6;       // Corner piece
    t['g'] = 0xEB;       // Corner piece
    t['h'] = 0xEA;       // Corner piece
    t['l'] = 0xE8;       // Small corner
    t['r'] = 0xE9;       // Small corner
    t['u'] = 0xE5;       // Horizontal piece
    t['-'] = 0xCF;       // Ghost house door (pink horizontal line)
    t['P'] = TILE_PILL;  // Power pill

    // Initialize both map and video_ram from ASCII map
    for (int y = 0, i = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++, i++)
        {
            char c = tiles[i];

            // Game logic map
            map[y][x] = (c == '.') ? DOT : (c == 'P')           ? POWERUP
                                       : (c == ' ' || c == '-') ? EMPTY
                                                                : WALL;

            // Tile code map for rendering
            video_ram[y][x] = t[(unsigned char)c];
        }
    }

    // Count dots
    for (int i = 0; i < MAP_HEIGHT; i++)
    {
        for (int j = 0; j < MAP_WIDTH; j++)
        {
            if (map[i][j] == DOT)
                total_dots++;
        }
    }

    is_secret=0;
}

int check_fruit_pos_wall(int x, int y) {
    int res = 0;

    if (map[y][x]==WALL) {
        res = 1;
    }else if (map[y+1][x]==WALL) {
        res = 1;
    }else if (map[y][x+1]==WALL) {
        res=1;
    }else if (map[y-1][x]==WALL) {
        res =1;
    }else if (map[y][x-1]==WALL) {
        res = 1;
    }

    return res;
}

void save_game(void) {
    for (int i=0;i<MAP_HEIGHT;i++) {
        for (int j=0;j<MAP_WIDTH;j++) {
            main_map.map_state[i][j] = map[i][j];
            main_map.video_ram_state[i][j] = video_ram[i][j];
        }
    }
    main_map.eaten_dots = eaten_dots;
}

void init_secret_map(void) {
    total_dots = 0;
    eaten_dots = 0;

    // Symmetric ASCII map (20x14)
    static const char *tiles =
        "0UUUUUUUUUUUUUUUUUU1"
        "L..................R"
        "L..................R"
        "L..................R"
        "L..................R"
        "L..................R"
        "L..................R"
        "L..................R"
        "L..................R"
        "L..................R"
        "L..................R"
        "L..................R"
        "L..................R"
        "2BBBBBBBBBBBBBBBBBB3";

    // Create mapping table (ASCII char -> tile code)
    uint8_t t[128];
    for (int i = 0; i < 128; i++)
    {
        t[i] = TILE_DOT; // Default to dot
    }

    // Special tiles (from original pacman.c)
    t[' '] = TILE_SPACE; // 0x40
    t['.'] = TILE_DOT;   // 0x10
    t['0'] = 0xD1;       // Top-left corner
    t['1'] = 0xD0;       // Top-right corner
    t['2'] = 0xD5;       // Bottom-left corner
    t['3'] = 0xD4;       // Bottom-right corner
    t['4'] = 0xFB;       // Top center-left
    t['5'] = 0xFA;       // Top center-right
    t['U'] = 0xDB;       // Horizontal line (top/bottom)
    t['L'] = 0xD3;       // Left vertical
    t['R'] = 0xD2;       // Right vertical
    t['B'] = 0xDC;       // Horizontal line (bottom)
    t['b'] = 0xDF;       // Blob fill
    t['e'] = 0xE7;       // Corner piece
    t['f'] = 0xE6;       // Corner piece
    t['g'] = 0xEB;       // Corner piece
    t['h'] = 0xEA;       // Corner piece
    t['l'] = 0xE8;       // Small corner
    t['r'] = 0xE9;       // Small corner
    t['u'] = 0xE5;       // Horizontal piece
    t['-'] = 0xCF;       // Ghost house door (pink horizontal line)
    t['P'] = TILE_PILL;  // Power pill

    // Initialize both map and video_ram from ASCII map
    for (int y = 0, i = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++, i++)
        {
            char c = tiles[i];

            // Game logic map
            map[y][x] = (c == '.') ? DOT : (c == 'P')           ? POWERUP
                                       : (c == ' ' || c == '-') ? EMPTY
                                                                : WALL;

            // Tile code map for rendering
            video_ram[y][x] = t[(unsigned char)c];
        }
    }

    // Count dots
    for (int i = 0; i < MAP_HEIGHT; i++)
    {
        for (int j = 0; j < MAP_WIDTH; j++)
        {
            if (map[i][j] == DOT)
                total_dots++;
        }
    }
    is_secret = 1;
}

void load_main_game(void) {
    for (int y = 0, i = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++, i++)
        {
            map[y][x] = main_map.map_state[y][x];
            video_ram[y][x] = main_map.video_ram_state[y][x];
        }
    }
    is_secret=0;
}

void init_game(void)
{
    srand(time(NULL));
    init_map();

    // Player init - start at middle area (row 1 is open)
    player.pos.x = 10;
    player.pos.y = 1;
    player.lives = INITIAL_LIVES;
    player.score = 0;
    player.direction = 0; // right
    player.powered = 0;
    player.key = 0;
    next_direction = -1;

    for (int i=0;i<num_fruit;i++) {
        int x = rand()%(MAP_WIDTH-2)+1;
        int y = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;

        while (check_fruit_pos_wall(x, y)) {
            x = rand()%(MAP_WIDTH-2)+1;
            y = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;
        }

        fruit_pos[i].x = x;
        fruit_pos[i].y = y;

        fruit_kind[i] = rand()%6;

        fruit_eaten[i] = 0;
    }

    for (int i=0;i<num_egg;i++) {
        int x = rand()%(MAP_WIDTH-2)+1;
        int y = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;

        while (check_fruit_pos_wall(x, y)) {
            x = rand()%(MAP_WIDTH-2)+1;
            y = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;
        }

        egg_pos[i].x = x;
        egg_pos[i].y = y;

        egg_eaten[i] = 0;
    }

    egg = 0;

    int kx = rand()%(MAP_WIDTH-2)+1;
    int ky = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;

    while (check_fruit_pos_wall(kx, ky)) {
        kx = rand()%(MAP_WIDTH-2)+1;
        ky = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;
    }

    key_pos.x = kx;
    key_pos.y = ky;

    take_key = 0;
    change = 0;

    int sx = rand()%(MAP_WIDTH-2)+1;
    int sy = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;

    while (check_fruit_pos_wall(sx, sy)) {
        sx = rand()%(MAP_WIDTH-2)+1;
        sy = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;
    }

    secret_pos.x = sx;
    secret_pos.y = sy;

    // Ghost init - ghost house is around row 6-7, columns 8-11
    Position ghost_starts[] = {
        {9, 6}, // First ghost starts outside (door position)
        {10, 6} // Second ghost inside ghost house
    };
    int ghost_colors[] = {COLOR_BLINKY, COLOR_PINKY};

    for (int i = 0; i < NUM_GHOSTS; i++)
    {
        ghosts[i].pos = ghost_starts[i];
        ghosts[i].scared = 0;
        ghosts[i].color = ghost_colors[i];
        ghosts[i].inside = (i == 0) ? 0 : 1; // First ghost starts outside
        ghosts[i].returning = 0;
        ghosts[i].direction = UP;
    }

    level = 1;
}

void reset_level(void)
{
    // Reset map and dots
    init_map();

    // Reset player position and state
    player.pos.x = 10;
    player.pos.y = 1;
    player.direction = 0;
    player.powered = 0;
    next_direction = -1;

    // Reset ghosts
    Position ghost_starts[] = {
        {9, 6}, // First ghost starts outside
        {10, 6} // Second ghost inside
    };

    for (int i = 0; i < NUM_GHOSTS; i++)
    {
        ghosts[i].pos = ghost_starts[i];
        ghosts[i].scared = 0;
        ghosts[i].inside = (i == 0) ? 0 : 1; // First ghost starts outside
        ghosts[i].returning = 0;
    }

    for (int i=0;i<num_fruit;i++) {
        int x = rand()%(MAP_WIDTH-2)+1;
        int y = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;

        while (check_fruit_pos_wall(x, y)) {
            x = rand()%(MAP_WIDTH-2)+1;
            y = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;
        }

        fruit_pos[i].x = x;
        fruit_pos[i].y = y;

        fruit_kind[i] = rand()%7;

        fruit_eaten[i] = 0;
    }

    for (int i=0;i<num_fruit;i++) {
        int x = rand()%(MAP_WIDTH-2)+1;
        int y = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;

        while (check_fruit_pos_wall(x, y)) {
            x = rand()%(MAP_WIDTH-2)+1;
            y = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;
        }

        fruit_pos[i].x = x;
        fruit_pos[i].y = y;

        fruit_kind[i] = rand()%6;

        fruit_eaten[i] = 0;
    }

    for (int i=0;i<num_egg;i++) {
        int x = rand()%(MAP_WIDTH-2)+1;
        int y = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;

        while (check_fruit_pos_wall(x, y)) {
            x = rand()%(MAP_WIDTH-2)+1;
            y = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;
        }

        egg_pos[i].x = x;
        egg_pos[i].y = y;

        egg_eaten[i] = 0;
    }

    egg = 0;

    int kx = rand()%(MAP_WIDTH-2)+1;
    int ky = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;

    while (check_fruit_pos_wall(kx, ky)) {
        kx = rand()%(MAP_WIDTH-2)+1;
        ky = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;
    }

    key_pos.x = kx;
    key_pos.y = ky;

    take_key = 0;
    change = 0;
    int sx = rand()%(MAP_WIDTH-2)+1;
    int sy = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;

    while (check_fruit_pos_wall(sx, sy)) {
        sx = rand()%(MAP_WIDTH-2)+1;
        sy = rand()%(MAP_HEIGHT-INFO_OFFSET-2)+1;
    }

    secret_pos.x = sx;
    secret_pos.y = sy;
}

int can_move(int x, int y)
{
    if (x < 1 || x >= MAP_WIDTH - 1 || y < 1 || y >= MAP_HEIGHT - 1)
        return 0;
    return map[y][x] != WALL;
}

void move_player(int dx, int dy)
{
    // TODO: 플레이어를 dx, dy만큼 이동
    // 입력: dx (x방향 이동량), dy (y방향 이동량)
    // 처리:
    //   1. 이동 가능 여부 확인 (can_move 사용)
    //   2. 위치 업데이트 및 방향 설정
    //   3. DOT 먹으면 점수 +10, eaten_dots++
    //   4. POWERUP 먹으면 점수 +50, 유령들 scared 상태로
    if (can_move(player.pos.x+dx, player.pos.y+dy)) {
        player.pos.x = player.pos.x+dx;
        player.pos.y = player.pos.y+dy;
    }

    if (dx == -1) {
        player.direction = 1;
    }else if (dx==1) {
        player.direction = 0;
    }else if (dy==-1) {
        player.direction = 2;
    }else{
        player.direction = 3;
    }

    if (map[player.pos.y][player.pos.x]==DOT) {
        map[player.pos.y][player.pos.x] = EMPTY;
        player.score += 10;
        eaten_dots++;
        video_ram[player.pos.y][player.pos.x] = TILE_SPACE;
    }

    if (map[player.pos.y][player.pos.x]==POWERUP) {
        map[player.pos.y][player.pos.x]=EMPTY;
        player.score+=50;
        player.powered = get_power_duration();
        video_ram[player.pos.y][player.pos.x] = TILE_SPACE;
        for (int i=0;i<NUM_GHOSTS;i++) {
            if (ghosts[i].inside == 0&&ghosts[i].returning == 0) ghosts[i].scared=1;
        }
    }
}

int can_move_in_direction(int dir)
{
    int dx = 0, dy = 0;
    switch (dir)
    {
    case 0:
        dx = 1;
        break; // right
    case 1:
        dx = -1;
        break; // left
    case 2:
        dy = -1;
        break; // up
    case 3:
        dy = 1;
        break; // down
    }
    return can_move(player.pos.x + dx, player.pos.y + dy);
}

void move_in_current_direction(void) {
    int dx = 0, dy = 0;
    switch (player.direction)
    {
        case 0:
            dx = 1;
            break; // right
        case 1:
            dx = -1;
            break; // left
        case 2:
            dy = -1;
            break; // up
        case 3:
            dy = 1;
            break; // down
    }
    move_player(dx, dy);
}

Dir opposite(Dir d) {
    switch (d) {
        case RIGHT:
            return LEFT;
            break;
        case LEFT:
            return RIGHT;
            break;
        case UP:
            return DOWN;
            break;
        case DOWN:
            return UP;
            break;
    }
}

void ghost_random_move(Ghost* g) {
    Dir dirs[4];
    int cnt = 0;

    for (Dir d = 0;d<4;d++) {
        if (d==opposite(g->direction)) continue;
        if (can_move(g->pos.x+x_dir[d], g->pos.y+y_dir[d])) {
            dirs[cnt++] = d;
        }
    }

    int e = rand()%10;

    if (can_move(g->pos.x+x_dir[g->direction], g->pos.y+y_dir[g->direction])&&e>=4) {
        g->pos.x += x_dir[g->direction];
        g->pos.y += y_dir[g->direction];
    }else if (cnt > 0) {
        Dir d = dirs[rand()%cnt];
        g->direction = d;
        g->pos.x += x_dir[d];
        g->pos.y += y_dir[d];
    }
}

void ghost_move_target(Ghost* g, int x, int y) {
    int min_dir;
    int min;

    Dir dirs[4];
    int cnt = 0;

    for (Dir d = 0;d<4;d++) {
        if (d==opposite(g->direction)) continue;
        if (can_move(g->pos.x+x_dir[d], g->pos.y+y_dir[d])) {
            dirs[cnt++] = d;
        }
    }

    min = abs(x - g->pos.x-x_dir[dirs[0]]) + abs(y-g->pos.y-y_dir[dirs[0]]);
    min_dir = dirs[0];
    for (Dir d = 0;d<cnt;d++) {
        if (abs(x - g->pos.x-x_dir[dirs[d]]) + abs(y-g->pos.y-y_dir[dirs[d]])<min) {
            min = abs(x - g->pos.x-x_dir[dirs[d]]) + abs(y-g->pos.y-y_dir[dirs[d]]);
            min_dir = dirs[d];
        }
    }

    g->pos.x += x_dir[min_dir];
    g->pos.y += y_dir[min_dir];
    g->direction = min_dir;
}

void ghost_move_away(Ghost* g, int x, int y) {
    int max_dir;
    int max;

    Dir dirs[4];
    int cnt = 0;

    for (Dir d = 0;d<4;d++) {
        if (d==opposite(g->direction)) continue;
        if (can_move(g->pos.x+x_dir[d], g->pos.y+y_dir[d])) {
            dirs[cnt++] = d;
        }
    }

    max = abs(x - g->pos.x-x_dir[dirs[0]]) + abs(y-g->pos.y-y_dir[dirs[0]]);
    max_dir = dirs[0];
    for (Dir d = 0;d<cnt;d++) {
        if (abs(x - g->pos.x-x_dir[dirs[d]]) + abs(y-g->pos.y-y_dir[dirs[d]])>max) {
            max = abs(x - g->pos.x-x_dir[dirs[d]]) + abs(y-g->pos.y-y_dir[dirs[d]]);
            max_dir = dirs[d];
        }
    }

    g->pos.x += x_dir[max_dir];
    g->pos.y += y_dir[max_dir];
    g->direction = max_dir;
}

void move_ghosts(void)
{
    // TODO: 모든 유령의 이동 처리
    // 각 유령마다:
    //   - returning 상태: 집(9,6)으로 복귀
    //   - inside 상태: 집 안에서 상하 이동, 점수 500+ 시 탈출
    //   - scared 상태: 플레이어 반대 방향으로 이동
    //   - 일반 상태: 랜덤 방향 이동
    for (int i=0;i<NUM_GHOSTS;i++) {
        Ghost *current_ghost = &ghosts[i];

        if (current_ghost->returning==1) {
            int home_x = 9;
            int home_y = 6;

            ghost_move_target(current_ghost, home_x, home_y);

            if (current_ghost->pos.x == home_x && current_ghost->pos.y==home_y) {
                current_ghost->returning = 0;
                current_ghost->inside = 1;
            }
        }else if (current_ghost->inside==1) {
            if (player.score >= 500) {
                current_ghost->inside = 0;
                current_ghost->pos.y=5;
                continue;
            }
            ghost_bounce_dir[i] *= -1;
            current_ghost->pos.y+= ghost_bounce_dir[i];
        }else if (current_ghost->scared==1) {
            int x = player.pos.x;
            int y = player.pos.y;

            int e = rand()%10;

            if (e<=1) {
                ghost_random_move(current_ghost);
            }else {
                ghost_move_away(current_ghost, x, y);
            }
        }else {
            if ((current_ghost->pos.y<=6&&current_ghost->pos.y>=4)&&((9==current_ghost->pos.x)||(current_ghost->pos.x==10))) {
                current_ghost->pos.y -= 1;
                current_ghost->direction = UP;
            }else {
                ghost_random_move(current_ghost);
            }
        }
    }
}


void check_collision(void)
{
    // TODO: 플레이어와 유령 충돌 확인
    // 각 유령마다:
    //   - inside/returning 상태는 무시
    //   - 위치가 같으면:
    //     * powered + scared: 유령 먹기 (점수 +200, returning 상태로)
    //     * 일반: 플레이어 사망 (STATE_DYING으로 전환)
    if (!is_secret) {
        for (int i=0;i<NUM_GHOSTS;i++) {
            Ghost* current_ghost = &ghosts[i];
            if (current_ghost->inside || current_ghost->returning) {
                continue;
            }
            if (player.pos.x == current_ghost->pos.x&&player.pos.y==current_ghost->pos.y) {
                if (current_ghost->scared==1) {
                    player.score += 200;
                    current_ghost->returning=1;
                    current_ghost->scared = 0;
                }else {
                    game_state=STATE_DYING;
                }
            }
        }
    }
}



void check_level_complete(void)
{
    // TODO: 레벨 클리어 확인 및 처리
    // 조건: eaten_dots >= total_dots
    // 처리: level++, level_flash 설정, reset_level() 호출
    if (eaten_dots>=total_dots&&is_secret==0) {
        level++;
        level_flash=36;
        reset_level();
    }
}

void check_secret_complete(void) {
    if (eaten_dots>=total_dots&&is_secret==1) {
        load_main_game();
    }
}

void render_game(void)
{
#define UI_OFFSET 4

    gfx_clear_tilemap();
    gfx_clear_sprites();

    // Render UI
    gfx_render_ui(player.score, highscore, player.lives, player.powered, level);

    // Render map tiles directly from video_ram
    for (int y = 0; y < MAP_HEIGHT; y++)
    {
        for (int x = 0; x < MAP_WIDTH; x++)
        {
            uint8_t tile = video_ram[y][x];
            uint8_t color = COLOR_DOT;

            // Ghost house door needs pink/magenta color
            if (tile == 0xCF)
            {
                color = COLOR_FRIGHTENED; // Bright blue/pink for visibility
            }

            // Flash white on level up
            if (level_flash > 0 && (level_flash / 3) % 2)
            {
                color = COLOR_WHITE_BORDER;
            }
            gfx_set_tile(x, y + UI_OFFSET, tile, color);
        }
    }

    // Render player (not in intro state)
    if (game_state != STATE_INTRO)
    {
        int df = (game_state == STATE_DYING) ? death_frame : 0;
        gfx_render_player(player.pos.x, player.pos.y, player.direction, UI_OFFSET, df);
    }

    // Render ghosts (not in intro or dying state)
    if ((game_state != STATE_INTRO && game_state != STATE_DYING)&&is_secret==0)
    {
        gfx_ghost_t gfx_ghosts[NUM_GHOSTS];
        for (int i = 0; i < NUM_GHOSTS; i++)
        {
            gfx_ghosts[i].pos.x = ghosts[i].pos.x;
            gfx_ghosts[i].pos.y = ghosts[i].pos.y;
            gfx_ghosts[i].scared = ghosts[i].returning ? 3 : ghosts[i].scared; // 3 = eyes state
            // Color: returning (eyes) or blinking or blue or normal
            if (ghosts[i].returning)
            {
                gfx_ghosts[i].color = COLOR_EYES;
            }
            else if (ghosts[i].scared == 2)
            {
                gfx_ghosts[i].color = (frame_count / 6) % 2 ? COLOR_WHITE_BORDER : COLOR_FRIGHTENED;
            }
            else if (ghosts[i].scared == 1)
            {
                gfx_ghosts[i].color = COLOR_FRIGHTENED;
            }
            else
            {
                gfx_ghosts[i].color = ghosts[i].color;
            }
        }
        gfx_render_ghosts(gfx_ghosts, NUM_GHOSTS, UI_OFFSET);
    }

    // 과일 스프라이트 예시 - gfx_set_sprite 사용법
    //
    // 사용 가능한 과일 스프라이트 (SPRITETILE):
    //   SPRITETILE_CHERRIES    = 0   (COLOR: 0x14)
    //   SPRITETILE_STRAWBERRY  = 1   (COLOR: 0x0F)
    //   SPRITETILE_PEACH       = 2   (COLOR: 0x15)
    //   SPRITETILE_BELL        = 3   (COLOR: 0x16)
    //   SPRITETILE_APPLE       = 4   (COLOR: 0x14)
    //   SPRITETILE_GRAPES      = 5   (COLOR: 0x17)
    //   SPRITETILE_GALAXIAN    = 6   (COLOR: 0x09)
    //   SPRITETILE_KEY         = 7   (COLOR: 0x16)
    //
    // gfx_set_sprite(index, enabled, pixel_x, pixel_y, tile, color, flipx, flipy)
    //   - index: 스프라이트 번호 (0=플레이어, 1-2=유령, 3-7=사용가능)
    //   - pixel_x/y: 픽셀 좌표 (타일좌표 * TILE_WIDTH/HEIGHT)
    //
    // 예시: 특정 위치에 과일 배치하기

    uint8_t color;

    for (int i=0;i<num_fruit;i++) {
        int kind = fruit_kind[i];
        int x = fruit_pos[i].x;
        int y = fruit_pos[i].y;

        switch (kind) {
            case 0: color = 0x14; break; // Cherries (빨강)
            case 1: color = 0x0F; break; // Strawberry (주황/노랑)
            case 2: color = 0x15; break; // Peach (주황)
            case 3: color = 0x16; break; // Bell (노랑)
            case 4: color = 0x14; break; // Apple (빨강)
            case 5: color = 0x17; break; // Grapes (보라)
            default: color = 0x14; break;
        }
        if (!fruit_eaten[i]&&is_secret==0) {
            gfx_set_sprite(3+i, true, x * TILE_WIDTH, (y + UI_OFFSET) * TILE_HEIGHT, kind, color, false, false);
        }

    }

    if (!take_key) {
        gfx_set_sprite(7, true, key_pos.x * TILE_WIDTH, (key_pos.y + UI_OFFSET) * TILE_HEIGHT, 7, 0x16, false, false);
    }else {
    }
    //gfx_set_sprite(3, true, fruit_pos[0].x * TILE_WIDTH, (fruit_pos[0].y + UI_OFFSET) * TILE_HEIGHT, 0, 0x14, false, false);   // Cherry
    //gfx_set_sprite(4, true, fruit_pos[1].x * TILE_WIDTH, (fruit_pos[1].y + UI_OFFSET) * TILE_HEIGHT, 1, 0x0F, false, false);  // Strawberry
    // gfx_set_sprite(5, true, 3 * TILE_WIDTH, (8 + UI_OFFSET) * TILE_HEIGHT, 2, 0x15, false, false);   // Peach
    // gfx_set_sprite(6, true, 16 * TILE_WIDTH, (8 + UI_OFFSET) * TILE_HEIGHT, 3, 0x16, false, false);  // Bell
    // gfx_set_sprite(7, true, 5 * TILE_WIDTH, (11 + UI_OFFSET) * TILE_HEIGHT, 4, 0x14, false, false);  // Apple

    // Render state-specific text
    if (game_state == STATE_INTRO)
    {
        gfx_render_text(MAP_WIDTH / 2 - 6, UI_OFFSET + MAP_HEIGHT / 2, "PRESS ANY KEY", COLOR_PACMAN);
        gfx_render_text(MAP_WIDTH / 2 - 4, UI_OFFSET + MAP_HEIGHT / 2 + 1, "TO START!", COLOR_PACMAN);
    }
    else if (game_state == STATE_READY)
    {
        gfx_render_text(MAP_WIDTH / 2 - 3, UI_OFFSET + MAP_HEIGHT / 2, "READY!", COLOR_PACMAN);
    }
    else if (game_state == STATE_GAMEOVER)
    {
        gfx_render_text(MAP_WIDTH / 2 - 5, UI_OFFSET + MAP_HEIGHT / 2, "GAME  OVER", COLOR_BLINKY);
    }

    gfx_draw();
}

void check_eat(void)
{
    // 과일 먹었는지 확인
    for (int i=0;i<num_fruit;i++) {
        if ((player.pos.x == fruit_pos[i].x&&player.pos.y==fruit_pos[i].y)&&is_secret==0) {
            if (!fruit_eaten[i]) {
                player.score += 50;
                fruit_eaten[i] =1;
            }

        }
    }
}



void check_key(void) {
    //열쇠 잡기
    if ((player.pos.x == key_pos.x&&player.pos.y==key_pos.y)&&is_secret==0) {
        if (!take_key) {
            player.key = 1;
            take_key = 1;
            change = 1;
        }

    }
}

void check_secret(void) {
    if (player.pos.x == secret_pos.x && (player.pos.y==secret_pos.y&&player.key==1)) {
        egg = 1;
    }
}

void game_update(void)
{
    // Handle game states
    if (game_state == STATE_INTRO)
    {
        return; // Wait for key press
    }

    if (game_state == STATE_READY)
    {
        state_timer++;
        if (state_timer >= 36)
        { // 3 seconds at ~12fps
            game_state = STATE_PLAYING;
        }
        return;
    }

    if (game_state == STATE_GAMEOVER)
    {
        return; // Game ended
    }

    if (game_state == STATE_DYING)
    {
        // TODO: 죽음 애니메이션 처리
        // 1. state_timer 증가, 3프레임마다 death_frame++
        // 2. death_frame >= 11 되면:
        //    - lives 감소
        //    - lives <= 0: 게임오버 처리
        //    - lives > 0: 플레이어/유령 위치 리셋, STATE_READY로
        if (death_frame>=11){
            player.lives--;
            if (player.lives<=0) {
                game_state = STATE_GAMEOVER;
            }else {
                state_timer = 0;
                death_frame = 0;

                game_state = STATE_READY;
                player.pos.x = 10;
                player.pos.y = 1;
                player.direction = 0;
                player.powered = 0;
                next_direction = -1;

                Position ghost_starts[] = {
                    {9, 6}, // First ghost starts outside
                    {10, 6} // Second ghost inside
                };

                for (int i = 0; i < NUM_GHOSTS; i++)
                {
                    ghosts[i].pos = ghost_starts[i];
                    ghosts[i].scared = 0;
                    ghosts[i].inside = (i == 0) ? 0 : 1; // First ghost starts outside
                    ghosts[i].returning = 0;
                }
            }
        }else {
            state_timer++;
            if (state_timer>0&&state_timer%3==0) {
                death_frame++;
            }
        }
        return;
    }

    // Handle input - store desired direction
    if (input.up)
        next_direction = 2;
    if (input.down)
        next_direction = 3;
    if (input.left)
        next_direction = 1;
    if (input.right)
        next_direction = 0;

    // Try to change direction if player wants to
    if (next_direction != -1 && next_direction != player.direction)
    {
        if (can_move_in_direction(next_direction))
        {
            player.direction = next_direction;
            next_direction = -1;
        }
    }

    // Move player based on level speed
    int player_interval = get_player_move_interval();
    if (frame_count % player_interval == 0)
    {
        move_in_current_direction();
    }

    // Update powerup timer
    if (player.powered > 0)
    {
        player.powered--;

        // Update scared state: 1=blue, 2=blinking
        int blink_threshold = get_power_duration() / 3;
        for (int i = 0; i < NUM_GHOSTS; i++)
        {
            if (ghosts[i].scared)
            {
                ghosts[i].scared = (player.powered <= blink_threshold) ? 2 : 1;
            }
        }

        if (player.powered == 0)
        {
            for (int i = 0; i < NUM_GHOSTS; i++)
            {
                ghosts[i].scared = 0;
            }
        }
    }

    // Move ghosts based on level speed
    int ghost_interval = get_ghost_move_interval();
    if (frame_count % ghost_interval == 0)
    {
        move_ghosts();
    }

    check_collision();
    check_eat();
    check_key();
    check_secret();

    // Check level complete
    check_level_complete();
    check_secret_complete();

    // Update level flash timer
    if (level_flash > 0)
        level_flash--;

    frame_count++;

    if (change) {
        if (!is_secret) {
            save_game();
            init_secret_map();
            change = 0;
        }else {
            load_main_game();
            change = 0;
        }
    }
}

static int render_frame_count = 0;
#define FRAMES_PER_UPDATE 5 // 60fps / 5 = 12fps

// Sokol callbacks
static void init(void)
{
    gfx_init();
    load_highscore();
    init_game();
    memset(&input, 0, sizeof(input));
}

static void frame(void)
{
    // Update game logic at reduced rate
    if (render_frame_count % FRAMES_PER_UPDATE == 0)
    {
        game_update();
    }
    render_frame_count++;

    render_game();

    if (input.quit)
    {
        sapp_quit();
    }
}

static void cleanup(void)
{
    save_highscore();
    gfx_shutdown();
}

static void event(const sapp_event *ev)
{
    if (ev->type == SAPP_EVENTTYPE_KEY_DOWN)
    {
        // Handle state transitions
        if (game_state == STATE_INTRO)
        {
            // Any key starts the game
            if (ev->key_code != SAPP_KEYCODE_Q && ev->key_code != SAPP_KEYCODE_ESCAPE)
            {
                game_state = STATE_READY;
                state_timer = 0;
                return;
            }
        }

        // Game over state - any key restarts
        if (game_state == STATE_GAMEOVER)
        {
            if (ev->key_code != SAPP_KEYCODE_Q && ev->key_code != SAPP_KEYCODE_ESCAPE)
            {
                init_game();
                game_state = STATE_INTRO;
                return;
            }
        }

        switch (ev->key_code)
        {
        case SAPP_KEYCODE_UP:
            input.up = 1;
            break;
        case SAPP_KEYCODE_DOWN:
            input.down = 1;
            break;
        case SAPP_KEYCODE_LEFT:
            input.left = 1;
            break;
        case SAPP_KEYCODE_RIGHT:
            input.right = 1;
            break;
        case SAPP_KEYCODE_Q:
        case SAPP_KEYCODE_ESCAPE:
            input.quit = 1;
            break;
        default:
            break;
        }
    }
    else if (ev->type == SAPP_EVENTTYPE_KEY_UP)
    {
        switch (ev->key_code)
        {
        case SAPP_KEYCODE_UP:
            input.up = 0;
            break;
        case SAPP_KEYCODE_DOWN:
            input.down = 0;
            break;
        case SAPP_KEYCODE_LEFT:
            input.left = 0;
            break;
        case SAPP_KEYCODE_RIGHT:
            input.right = 0;
            break;
        default:
            break;
        }
    }
}

sapp_desc sokol_main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = MAP_WIDTH * TILE_WIDTH * 2,
        .height = (MAP_HEIGHT + 3) * TILE_HEIGHT * 2, // +3 for UI panel
        .window_title = "Pacman",
        .icon.sokol_default = true,
        .logger.func = slog_func,
    };
}