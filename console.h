/*
	CONSOLE.H

	CONSOLE I/O for WIN32

	copytight (c) 2001 All Rights Reserved
	Version 1.0

	(for a screen size of 25 x 80)
*/

typedef struct {
	int vertsize,
		horzsize,
		xpos,
		ypos;

} CONINFO;

void con_getinfo(CONINFO *p);
void con_scrollup(void);
void con_print_CR(void);
void con_gotoxy(int x, int y);
void con_clrscr(void);
void con_open(void);
void con_close(void);
void con_clrline(int y);
void con_setcolor_normal(void);
void con_setcolor_red(void);
void con_setcolor_blue(void);
void con_setcolor_green(void);
 
 
/* EOF */