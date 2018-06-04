#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
typedef uint8_t cell_t;
typedef uint8_t cell_action_t;
typedef struct universe {
    struct ttysize ts;
    cell_action_t* cell_action;
    cell_t* cells;
} universe_t;


static universe_t universe;
unsigned int frameno = 1;


#define NRCOLORS (sizeof(color_codes) / sizeof(const char))

static const char color_codes[] = {
    31, 32, 33, 34, 35, 36, 37, 91, 92, 93, 94, 95, 96, 97
};

const short colors[] = 
{
    COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
    COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE
};
const int num_colors = sizeof(colors) / sizeof(*colors);

uint8_t valid_cell(int y, int x) {
    return ((y >= 0 && y < universe.ts.ts_lines && x >= 0 && x < universe.ts.ts_cols)) ? 1 : 0;
}

cell_t get_cell(int y, int x) {
    if(!valid_cell(y,x)) {
        return ' ';
    }
    return universe.cells[universe.ts.ts_cols*y + x];
}

void set_cell(int y, int x, const char ch) {
    if(!valid_cell(y,x)) {
        return;
    }
    universe.cells[universe.ts.ts_cols*y + x] = ch;
}


void bye() {
    free(universe.cells);
    standend();
    printf("\033[?1003l\n");
    endwin();

}

int count_live_neighbors(int y, int x) {
    
    int live = 0;
    int x_offset;
    int y_offset;
    int i = 0;
    for(y_offset = -1; y_offset <= 1; ++y_offset) {
        for(x_offset = -1; x_offset <= 1; ++x_offset) {
            if (x_offset == 0 && y_offset == 0) continue;
            char p = get_cell(y+y_offset,x+x_offset);
            live += (get_cell(y+y_offset, x+x_offset) != ' ') ? 1 : 0;
        }
    }
    return live;
}


void print_universe() {
    char color_code_str[12];
    unsigned short x, y;
    for(y = 1; y < universe.ts.ts_lines; y++)
        for(x = 0; x < universe.ts.ts_cols; x++)    
            mvaddch(y, x, get_cell(y,x));
    refresh();
}


void init_universe() {
    ioctl(0, TIOCGSIZE, &universe.ts);    
    universe.cells = (cell_t*)malloc(sizeof(cell_t)*universe.ts.ts_lines*universe.ts.ts_cols);
    universe.cell_action = (cell_action_t*)malloc(sizeof(cell_action_t)*universe.ts.ts_lines*universe.ts.ts_cols);
    memset(universe.cell_action, 'N', sizeof(cell_action_t)*universe.ts.ts_lines*universe.ts.ts_cols);    
    memset(universe.cells, ' ', sizeof(cell_t)*universe.ts.ts_lines*universe.ts.ts_cols);
}



void set_cell_action() {
    int x;
    int y;
    int alive_neighbors;
    for(y = 0; y < universe.ts.ts_lines; y++) {
        for(x = 0; x < universe.ts.ts_cols; x++) {
            alive_neighbors = count_live_neighbors(y, x);
            if (get_cell(y, x) == 'X') {
                if (alive_neighbors < 2) {
                    universe.cell_action[universe.ts.ts_cols*y + x] = 'K';
                } else if (alive_neighbors == 2 || alive_neighbors == 3 ) {
                    universe.cell_action[universe.ts.ts_cols*y + x] = 'N';
                } else if (alive_neighbors > 3) {
                    universe.cell_action[universe.ts.ts_cols*y + x] = 'K';
                }
            } else {
                if (alive_neighbors == 3) {
                    universe.cell_action[universe.ts.ts_cols*y + x] = 'B';
                }
            }
        }
    }
}

void update_cells() {
    set_cell_action();
    int x, y;
    for(y = 0; y < universe.ts.ts_lines; y++) {
        for(x = 0; x < universe.ts.ts_cols; x++) {
            switch(universe.cell_action[universe.ts.ts_cols*y + x]) {
                case 'K':
                    set_cell(y, x, ' '); 
                    break;
                case 'B':
                    set_cell(y, x, 'X');
                    break;
            } 
        }
    }
}


void init_conway() {
    atexit(bye); /* Register bye to run on program exit. */
    initscr();
    cbreak();
    noecho();
    printf("\033[?1003h\n");
    keypad (stdscr, TRUE);
    MEVENT event;
    char buffer[512];
    size_t max_size = sizeof(buffer);
    init_universe();
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    int c;
    
    printw("Conways Game of Life! Use The Mouse To Mark Cells. Press Space To Start. Control-C To Exit.");
    while(1) {    
        c = wgetch(stdscr);
        if (c == ' ') {
            break;
        }
        if (c == KEY_MOUSE) {
            
            if (getmouse(&event) == OK) {
                set_cell(event.y, event.x, 'X');
                print_universe();
            }
        }
    }
}
void play() {
    while(1) {
        mvprintw(0,0,"Frame Number: %d. Press Control-C To Exit!", frameno); 
        clrtoeol();
        print_universe();
        update_cells();
        usleep(8000);
        ++frameno;
    }
}



int main() {
     
    init_conway();
    play();
    return 0;
}
