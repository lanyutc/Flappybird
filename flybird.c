/**
 * Viclan(lanyutc@163.com) 2014-11-17
 *
 * File flybird.c 
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>

/* Configure */
const static uint16_t sl = 80; /* scene length */
const static uint16_t sw = 25; /* scene width */

const static uint16_t bl = 5; /* barrier length */
const static uint16_t bgw = 5; /* barrier gap width*/

const static uint16_t gbi = 6; /* gap begin idx, must less sw - bgw */
const static uint16_t gei = 14; /* gap end idx, must less sw - bgw and great gbi */

const static uint16_t bm = 4; /* The distance bird moved upward when press 'j' */



struct FlyBird {
	uint32_t bpos; /* bird pos */

	uint16_t left; /* left distance bird moved upward */

	uint16_t spi; /* scene print idx */

	char *ms; /* main scene */
};
static struct FlyBird Game;



#define pos(x,y) ((y)*(sl*2)+(x))
#define right(x) (x >= (sl*2) ? (x-sl*2):(x))


static struct termios oterm;
int set_term_mode(int fd)
{   
	struct termios term; 
	if (tcgetattr(fd, &term) == -1 || tcgetattr(fd, &oterm) == -1){   
		printf("Cannot get the attribution of the terminal");   
		return -1;   
	}  

	term.c_lflag &= ~(ECHO | ICANON);

	if (tcsetattr(fd, TCSAFLUSH, &term) == -1) {
		printf("Cannot set the attribution of the terminal");
		return -1;   
	}   

	/* set stdin nonblock */
	int flag = fcntl(STDIN_FILENO, F_GETFL);
	flag |= O_NONBLOCK;
	if (fcntl(STDIN_FILENO, F_SETFL, flag) == -1) {
		printf("Cannot set the O_NONBLOCK of the STDIN_FILENO");
		return -1;
	}
	return 0;   
}   

int resume_term_mode(int fd)
{
	if (tcsetattr(fd, TCSAFLUSH, &oterm) == -1) {
		printf("Cannot resume the attribution of the terminal");
		return -1;   
	}   
	return 0;
}

void inc_spi()
{
	Game.spi = Game.spi + 1 >= 2 * sl ? 0 : Game.spi + 1;
}

void pnc(const char c, uint16_t cnt)
{
	for (uint16_t i = 0; i < cnt; i++) {
		printf("%c",c);
	}
	fflush(stdout);
}

void bird_move()
{
	/* clear old pos */
	Game.ms[Game.bpos] = ' ';
	
	/* calc new pos */
	if (Game.left > 0) {
		Game.bpos = Game.bpos < 2 * sl ? 0 : Game.bpos - 2 * sl + 1;	
		Game.left --;
	} else {
		Game.bpos = Game.bpos + 2 * sl >= 2 * sl * sw ? 0 : Game.bpos + 2 * sl + 1;
	}

	/* check collision */
	if (Game.ms[Game.bpos] == '+') {
		Game.ms[Game.bpos] = 'X';
	} else {
		Game.ms[Game.bpos] = 'O';
	}
}

void ps()
{
	for (uint16_t i = 0; i < sw; i++) {
		for (uint16_t j = 0; j < sl; j++) {
			uint16_t p = right(j + Game.spi); 
			printf("%c", Game.ms[pos(p,i)]);
		}
		pnc('\n', 1);
	}
	printf("\033[25A");
}

uint16_t rand_range(uint16_t min, uint32_t max)
{
	if (min >= max) {
		return min;
	}

	return (rand()%(max - min) + min);
}

void clear_barrier(uint16_t begin_idx, uint16_t end_idx)
{
	for (uint16_t i = begin_idx; i <= end_idx; i++) {
		for (uint16_t j = 0; j < sw; j++) {
			Game.ms[pos(i,j)] = ' ';
		}
	}
}

void create_barrier(char *s, uint16_t begin_idx, uint16_t end_idx)
{
	uint16_t shift_idx = rand_range(7, 10) + begin_idx;
	if (shift_idx + bl >= end_idx) {
		return;
	}

	uint16_t gap_idx = rand_range(gbi, gei); 
	for (uint16_t i = 0; i < bl; i++) {
		for (uint16_t j = 0; j < sw; j++) {
			if (j < gap_idx || j > gap_idx + bgw) {
				s[pos(shift_idx + i, j)] = '+';
			}
		}
	}

	create_barrier(s, shift_idx + bl, end_idx);
}

bool check_collision()
{
	/* Touching the upper and lower walls */
	if (Game.bpos == 0) {
		return true;
	}

	/* Touching the barrier */
	if (Game.ms[Game.bpos] == 'X') {
		return true;
	}
	return false;
}

void check_config()
{
	if (gei <= gbi) {
		printf("gap idx error\n");
		exit(-1);
	}
}

void flush_barrier()
{
	if (Game.spi == sl - 1) {
		clear_barrier(0, Game.spi);
		create_barrier(Game.ms, 0, Game.spi);
	}

	if (Game.spi == 2 * sl - 1) {
		clear_barrier(sl, Game.spi);
		create_barrier(Game.ms, sl, Game.spi);
	}
}

void game_ready()
{
	if (!Game.ms) {
		Game.ms = (char *)calloc(sl * sw * 2, sizeof(char));
		if (!Game.ms) {
			printf("Game alloc memory error\n");
			exit(-1);
		}
	} else {
		memset(Game.ms, ' ', sl * sw * 2);
	}
	Game.left = 0;
	Game.spi = 0;
	Game.bpos = (sl - 2)*sw/2; /* init bird begin pos */

	srand(time(NULL));

	create_barrier(Game.ms, sl / 3, sl * 2);
}

void game_init()
{
	set_term_mode(STDIN_FILENO);
	pnc('*', sl);
	pnc('\n', sw + 1);
	pnc('*', sl);

	//move cursor 
	printf("\033[15A\033[50D\033[32m Press j To Fly\n\n \
			\033[5C Press g To Start Or Restart\n\033[13A\033[?25l");
	fflush(stdout);
}

void game_over()
{
	free(Game.ms);
	Game.ms = NULL;
	//fix cursor 
	printf("\033[%dB\33[?25h\n", sw);
	fflush(stdout);
	resume_term_mode(STDIN_FILENO);
}


int main()
{
	bool is_over = false;
	bool is_start = false;
	char input = 0;

	check_config();
	game_init();
	game_ready();

	while (!is_over) {
		input = getchar();
		if (input == 'g') {
			game_ready();
			is_start = true;
		} else if (input == 'j') {
			Game.left = bm; 
		}

		if (is_start) {
			bird_move();
			ps();
			flush_barrier();
		}

		if (check_collision()) {
			is_over = true;
		}

		usleep(200000);
		inc_spi();
	}

	game_over();
	return 0;
}
