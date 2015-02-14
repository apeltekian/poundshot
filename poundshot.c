/*
 * Poundshot - simple top-down shoot-em-up game
 * Copyright (C) 2015 Robert Cochran
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License in the LICENSE file for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

int row, col;
int py, px; /* Player's XY */
int lives = 3;
bool gotlife = 0;
int score = 0;

int bx, by; /* Bullet's XY */
int direction; /* of the bullet */
bool bullet; /* Bullet existance variable */

int limit = 15; /* Used to limit the monster spawning */
int left = 15; /* Keep track of how many left before the next cycle */
static int newscore = 1000; /* When to decrease the spawn cycle */

int maxmon;
int monnum;

struct Monster_t {
	int x;
	int y;
	int status;
};

struct Monster_t *mon_list;

static char livestxt[] = "Lives - ";
static char scoretxt[] = "Score - ";
static char title[] = "Poundshot";
static char version[] = "1.0.3";

int key;

bool quit = 0;

void init(void);
void i_getkey(void);
void l_char(void);
void l_bullet(void);
void l_enemy(void);
void d_draw(void);
void l_hit(void);
void d_explosion(void); /* Seperating the explosion from l_hit() */

int main(void)
{
	init();
	
	while (!quit) {
		i_getkey();
		l_char();
		l_bullet();
		l_enemy();
		d_draw();
		refresh();
	}

	free(mon_list);
	endwin();
	return 0;
}

void init(void)
{
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	getmaxyx(stdscr,row,col);
	curs_set(0); /* Disable cursor */

	py = row / 2;
	px = col / 2;

	maxmon = ((row - 1) * col) / 2;

	mon_list = (struct Monster_t *)
		malloc(maxmon * sizeof(struct Monster_t));

	if (mon_list == NULL) {
		endwin();
		perror("Memory allocation error ");
		exit(1);
	}

	int i; /* Initialize default values */
	for (i = 0; i < maxmon; i++) {
		mon_list[i].x = -1;
		mon_list[i].y = -1;
		mon_list[i].status = 0;
	}
}

void i_getkey(void)
{
	halfdelay(1);
	key = getch();
	/* Key 27 is escape */
	if(key == 27) quit = 1;
}

/* l_char - handles input and movement for the player */
void l_char(void)
{
	if (score == 25000 && gotlife == 0) {lives++; gotlife = 1;}

	switch (key) {
		case 119 : /* W */
			if(py == 1) break;
			py -= 1;
			break;
		case 115 : /* S */
			if(py == row - 1) break;
			py += 1;
			break;
		case 97 : /* A */
			if(px == 0) break;
			px -= 1;
			break;
		case 100 : /* D */
			if(px == col - 1) break;
			px += 1;
			break;
	}
}

/* l_bullet - handles the input and logic for the bullet */
void l_bullet(void)
{
	if (bullet == 0 && (key >= 105 && key <= 108)) {	
	
		bullet = 1;
		mvaddch(py,px,'+');	
		by = py; bx = px;

		switch (key) {
			case 105 : /* I */
				direction = 0; /* Up */
				break;
			case 107 : /* K */
				direction = 2; /* Down */
				break;
			case 106 : /* J */
				direction = 3; /* Left */
				break;
			case 108 : /* L */
				direction = 1; /* Right */
				break;
		}
	} else if (bullet) {
		switch (direction) {
			case 0 : /* Up */
				by -= 1;
				if (by == 0) bullet = 0;
				break;
			case 1 : /* Right */
				bx += 1;
				if (bx > col - 1) bullet = 0;
				break;
			case 2 : /* Left */
				by += 1;
				if (by > row - 1) bullet = 0;
				break;
			case 3 : /* Down */
				bx -= 1;
				if (bx < 0) bullet = 0;
				break;
		}
	}
	
	return;
}

void l_enemy(void)
{
	if (monnum < maxmon && left == 0) {
		int curmon; /* Keep track of our number */
	
		monnum++;

		for (curmon = 0; curmon < maxmon; curmon++)
			if (mon_list[curmon].status == 0) break;

		/* Make sure this monster doesn't get picked again */
		mon_list[curmon].status = (rand() % 2) + 1; 		
		/* Seed the random number generator for our monster spawning */
		srand(time(NULL));

		bool goodlocation = 0;
		
		while (!goodlocation) {
			bool badlocation = 0;
			
			mon_list[curmon].x = rand() % col;

			mon_list[curmon].y = 1 + (rand() % row - 1);
			
			if (mon_list[curmon].y == 0) 
				mon_list[curmon].y = 1;			

			/* Test 1 - Is our location the player's? */
			if (mon_list[curmon].x == px
					&& mon_list[curmon].y == py)
				continue;
			

			/* Test 2 - Is our location another monster's? */
			
			int i;
			for (i = 0; i < maxmon; i++) {
				if(i == curmon) continue;
				
				if (mon_list[curmon].y == mon_list[i].y
				&& mon_list[curmon].x == mon_list[i].x) {
					badlocation = 1;
					break;
				}
			}			

			if (badlocation) continue;

			/* If we've gotten here, our location must be ok */
			goodlocation = 1;

			left = limit;
		}
		
	} else {left -= 1;}

	if (monnum > 0) {
		int i;
		for (i = 0; i < maxmon; i++) {
			if (bullet) {
				/* Variables to store offsets for bullet
				 * detection */
				int tempx, tempy;
 
				switch (direction) {
					case 0 : /* Up */
						tempx = mon_list[i].x;
						tempy = (mon_list[i].y - 1);
						break;
					case 1 : /* Right */
						tempx = (mon_list[i].x + 1);
						tempy = mon_list[i].y;
						break;
					case 2 : /* Down */
						tempx = mon_list[i].x;
						tempy = (mon_list[i].y + 1);
						break;
					case 3 : /* Left */
						tempx = (mon_list[i].x - 1);
						tempy = mon_list[i].y;
						break;
				}
				

				if ((tempx == bx) && (tempy == by)) {
					score += 65;
					bullet = 0;
					mon_list[i].x = -1;
					mon_list[i].y = -1;
					mon_list[i].status = 0;
					monnum -= 1;
				}
			}

			if ((py == mon_list[i].y && px == mon_list[i].x)
					&& mon_list[i].status != 0) l_hit();

			if (left == 0) {
				/* Stands for 'up, down, left, or right?'
 				 * Up/Left will be -1, Down/Right will be 1 */

				int udlr;
				if (mon_list[i].status == 0) continue;
					
				if (mon_list[i].status == 1) {
				
					mon_list[i].status = 2;

					/* Don't even bother */
					if (py == mon_list[i].y) continue;

					if (py - mon_list[i].y >
							mon_list[i].y - py)
						udlr = 1;
					else udlr = -1;
				
					mon_list[i].y += udlr;		
			
				} else if (mon_list[i].status == 2) {
				
					mon_list[i].status = 1;
					if (px == mon_list[i].x) continue;	
				
					if (px - mon_list[i].x >
							mon_list[i].x - px)
						udlr = 1;
					else udlr = -1;
				
					mon_list[i].x += udlr;
				}
			
			}

		}
	}	
	
	if ((score % 1000) == 0 && score == newscore && score != 0) {
		limit--;
		newscore += 1000;
	}

	
}

void d_draw(void)
{
	/* Clear screen */
	int a, b;

	for (a = 0; a < row; a++) {
		move(a, 0);

		for (b = 0; b < col; b++)
			addch(' ');
	}

	/* Bullet - note : rendered first to prevent problems with top bar */
	if (bullet)
		mvaddch(by, bx, '+');

	/* Top bar */
	
	for (a = 0; a < row; a++)
		mvaddch(a, 0, ' ');

	mvprintw(0,0, "%s", livestxt);
	
	int tmplives = lives;
	int xpos = 8;
	for (; tmplives != 0; tmplives--, xpos++)
		mvaddch(0, xpos, '#');
	
	mvprintw(0,( strlen(livestxt) + 5),"%s%d", scoretxt, score);
	mvprintw(0, ((col - strlen(title)) - strlen(version)) - 1,"%s %s",
		title, version);
	move(0,0);
	chgat(-1, A_REVERSE, 0, NULL);

	/* Monsters */
	
	for (a = 0; a < maxmon; a++) {
		if (mon_list[a].status == 0) continue;
		mvaddch(mon_list[a].y, mon_list[a].x, '@');
	}

	/* Player */
	mvaddch(py, px, '#');
}

void l_hit (void)
{
	lives--;

	int i;
	for (i = 0; i < maxmon; i++)
		mon_list[i].status = 0;

	monnum = 0;

	d_explosion();

	py = row / 2;
	px = col / 2;

	if (lives == 0) {
		mvprintw(row / 2, (col / 2) - 4, "GAME OVER");
		refresh();
		sleep(5);
		quit = 1;
	}
}

void d_explosion(void)
{
	usleep(600000);
	// Drawing of explosion
	mvaddch(py, px, '*');
	refresh();
	usleep(600000);
	mvaddch(py + 1, px, '*');
	mvaddch(py - 1, px, '*');
	mvaddch(py, px + 1, '*');
	mvaddch(py, px - 1, '*');
	refresh();
	usleep(600000);
	mvaddch(py + 2, px, '*');
	mvaddch(py - 2, px, '*');
	mvaddch(py, px + 2, '*');
	mvaddch(py, px - 2, '*');
	mvaddch(py + 1, px + 1, '*');
	mvaddch(py + 1, px - 1, '*');
	mvaddch(py - 1, px + 1, '*');
	mvaddch(py - 1, px - 1, '*');
	refresh();
	usleep(600000);

	// Un-drawing
	mvaddch(py, px, ' ');
	refresh();
	usleep(600000);
	mvaddch(py + 1, px, ' ');
	mvaddch(py - 1, px, ' ');
	mvaddch(py, px + 1, ' ');
	mvaddch(py, px - 1, ' ');
	refresh();
	usleep(600000);
	mvaddch(py + 2, px, ' ');
	mvaddch(py - 2, px, ' ');
	mvaddch(py, px + 2, ' ');
	mvaddch(py, px - 2, ' ');
	mvaddch(py + 1, px + 1, ' ');
	mvaddch(py + 1, px - 1, ' ');
	mvaddch(py - 1, px + 1, ' ');
	mvaddch(py - 1, px - 1, ' ');
	refresh();
	usleep(700000);
}
