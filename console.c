/*
	CONSOLE I/O

	TM DOS application ported to Win32 Platform
	By Tan Hwee Tiong

	copytight (c) 2001 All Rights Reserved
	Version 1.0

	for screen size of 25 x 80
*/
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "console.h"

static HANDLE so, si;
static int vertSize, horzSize;
static DWORD oldMode, newMode;

/*
 *	Win32 API std error handler
 */
static void error_handler(char *s, DWORD err)
{
	con_print_CR();
	cprintf( "[%s. Error number: %d]" , s, err);
	con_print_CR();
	ExitProcess(err);
}
/*
	CTRL+C, CTRL-BRK etc character handler
*/
static BOOL con_ctrlhandler(DWORD ctrlchar)
{

	switch (ctrlchar)
	{
		case	CTRL_C_EVENT:
				Beep(500,100);
				return 1;	// don't quit
		case	CTRL_BREAK_EVENT:
				Beep(200,200);
				return 0;	// quit program
		default:
				Beep(300,300);
				return 0;	// quit program
	}
}
/*
 *	Get current console's info
 */
void con_getinfo(CONINFO *p)
{
	BOOL success;
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

	// get current console size (may change)
	success = GetConsoleScreenBufferInfo(so, &consoleInfo);
	if (!success)
    		error_handler(
				"con_getinfo(): In GetConsoleScreenBufferInfo",
				GetLastError());

	// Define the rectangle to scroll
	p->vertsize = consoleInfo.dwSize.Y;
	p->horzsize = consoleInfo.dwSize.X;
	p->ypos = consoleInfo.dwCursorPosition.Y;
	p->xpos = consoleInfo.dwCursorPosition.X;
}

/*
 *		Scroll screen up from line# 0 to 23 (reserve line 25 for status msg)
 */
void con_scrollup(void)
{
	BOOL success;
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	SMALL_RECT scrollRect;
	CHAR_INFO consoleFill;
	COORD coord;

	// get current console size (may change)
	success = GetConsoleScreenBufferInfo(so, &consoleInfo);
	if (!success)
    		error_handler(
				"con_scrollup(): In GetConsoleScreenBufferInfo",
				GetLastError());

	// Define the rectangle to scroll
	scrollRect.Top = 0;
	scrollRect.Left = 0;
	// scrollRect.Bottom = consoleInfo.dwSize.Y - 1;
	scrollRect.Bottom = 23;
	scrollRect.Right = consoleInfo.dwSize.X - 1;

	// Define destination of scrolled rectangle
	coord.X = 0;
	coord.Y = -1;

	// Define how to fill blank line
	consoleFill.Attributes =
		consoleInfo.wAttributes;
	consoleFill.Char.AsciiChar = ' ';

	// Perform the scroll
	success = ScrollConsoleScreenBuffer(so,	&scrollRect, 0,	coord, &consoleFill);
	if (!success)	
		error_handler("con_scrollup(): In ScrollConsoleScreenBuffer",
			GetLastError());

}
/*
 *	handle the <CR> character (may involve scrolling up)
 */
void con_print_CR(void)
{
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	BOOL success;

	success = GetConsoleScreenBufferInfo( so, &consoleInfo);
	if (!success)
		error_handler("con_print_CR(): In GetConsoleScreenBufferInfo",
			GetLastError());

	// Move cursor to far left
	consoleInfo.dwCursorPosition.X = 0;

	// If the cursor is on the last line of 
	// the console, then scroll the console,
	// else increment position
	// if (consoleInfo.dwSize.Y - 1 == consoleInfo.dwCursorPosition.Y)
	if ( 23 == consoleInfo.dwCursorPosition.Y)
		con_scrollup();
	else
		consoleInfo.dwCursorPosition.Y += 1;

	// Update the console
	success = SetConsoleCursorPosition( so, consoleInfo.dwCursorPosition);
	if (!success)
		error_handler("con_print_CR(): In SetConsoleCursorPosition",
			GetLastError());
}

/*
 *	Move cursor to (x, y) position
 */
void con_gotoxy(int x, int y)
{
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	BOOL success;

	if (x > 79) return;
	if (y > 25) return;

	success = GetConsoleScreenBufferInfo( so, &consoleInfo);
	if (!success)
		error_handler("con_gotoxy(): In GetConsoleScreenBufferInfo",
			GetLastError());

	// Move cursor to far left
	consoleInfo.dwCursorPosition.X = x;
	consoleInfo.dwCursorPosition.Y = y;

	// Update the console
	success = SetConsoleCursorPosition( so, consoleInfo.dwCursorPosition);
	if (!success)
		error_handler("con_gotoxy(): In SetConsoleCursorPosition",
			GetLastError());

}

void con_setcolor_normal(void)
{
	SetConsoleTextAttribute(so, FOREGROUND_INTENSITY);
}

void con_setcolor_red(void)
{
	SetConsoleTextAttribute(so, FOREGROUND_RED | FOREGROUND_INTENSITY);

}

void con_setcolor_blue(void)
{
	SetConsoleTextAttribute(so, FOREGROUND_BLUE | FOREGROUND_INTENSITY);

}
void con_setcolor_green(void)
{
	SetConsoleTextAttribute(so, FOREGROUND_GREEN | FOREGROUND_INTENSITY);

}

/*
 *	Erase line 'num' and home the cursor at that line
 */
void con_clrline(int y)
{
	con_gotoxy(0, y);
	cputs("                                                                               ");
	con_gotoxy(0, y);
}
/*
 *	Clear console's screen and Home cursor
 */
void con_clrscr(void)
{
	int y;

	con_gotoxy(0, 0);
	for (y=0; y < 25; y++)
		cputs("                                                                                ");
	con_gotoxy(0, 0);
}

/*
 *		startup and init console's handle
 */
void con_open(void)
{
	BOOL success;

	// Get handles for standard in and out
	si = GetStdHandle(STD_INPUT_HANDLE);
	so = GetStdHandle(STD_OUTPUT_HANDLE);
	if (si == so) // they must be invalid
		error_handler("con_open(): In GetStdHandle", GetLastError());
	success = SetConsoleCtrlHandler( (PHANDLER_ROUTINE)con_ctrlhandler, TRUE);
	if (!success)
		error_handler("con_open(): In SetConsoleCtrlHandler",	GetLastError());

	// Get current console mode so can modify it
	success = GetConsoleMode( si, &oldMode);
	if (!success)
		error_handler("con_open(): In GetConsoleMode",	GetLastError());

	newMode = oldMode & ~ENABLE_LINE_INPUT & ~ENABLE_ECHO_INPUT;
	success = SetConsoleMode(si, newMode);
   	if (!success)
		error_handler("con_open(): In SetConsoleMode", GetLastError());

}

/*
 *		resotre old/saved console's mode
 */
void con_close(void)
{
	BOOL success;

	// put the old mode back
	success = SetConsoleMode(si, oldMode);
	if (!success)
		error_handler("con_close(): In SetConsoleMode",	GetLastError());
}

/* EOF */