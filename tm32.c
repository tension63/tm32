/*
	TM32.C

	My popular TM.EXE DOS application ported to 32 bit Windows Platform
	
	By Tan Hwee Tiong	Copyright (c) 2001 All Rights Reserved
	
	Version 1.0.0

	History
	========================================================
	0.9.2		25/07/2001			Release for BETA testing
	1.0.0		26/07/2001			Bug fixed release for public
	1.1.0		24/09/2003			Improved F8 func = can now send HEX string
									New F7 cmd: repeat last F8 string
    1.1.1		7/2/2004			Fixed failed to open Com10, Com11.. etc bug
	1.1.2		7/4/2005			Fixed missing 38400 baudrate
	1.1.3		26-MAY-2005			Added color support
	1.2.0		08/03/2012			Added time stamping

*/
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "rs232.h"
#include "console.h"
#include <sys/timeb.h>
#include <time.h>



/* Windows Key code */
#define F1   0x3b00
#define F2   0x3c00
#define F3   0x3d00
#define F4   0x3e00
#define F5   0x3f00
#define F6   0x4000
#define F7   0x4100
#define F8   0x4200
#define F9   0x4300
#define F10  0x4400

/* ASCII table */
#define CTRL_C	 0x03
#define BS       0x08
#define TAB		 0x09
#define CR       0x0D
#define LF       0x0A
#define ESC		 0x1B

/* Globals */
FILE *Stream;				/* File Stream */
char MonitorMode=0;			/* start-up at Mon=zero */
char SaveYes=0;				/* A flag set for entering screen capturing mode */
char FileName[300];			/* screen capture to this 'filename' */
int	 Port, BaudRate;		/* Port = 1,2...8  BaudRate = 2400,4800...57600 */
int  Xcount=0;
char flg_timestamp = 0;		/* time stamping flag */

/* Version number */
char* Version = "1.2.0";

/* fwd reference */
void show_setting(void);

/*
 *	Get a string entered by the user (up to 255 characters)
 */
void get_str(char buff[])
{
int n;
unsigned char ch;

     n = 0;
     for(;;)
     {
			while (!kbhit()) Sleep(20);
			ch = getch();
			switch (ch)
			{
			   case 	BS   :
						if (n)
						{
							n--;
							putch( BS );
							putch( ' ');
							putch( BS );
						}
						break;

			   case		CR   :
						buff[n] = 0;
						return;

			   case		ESC	:
						buff[0]=0;
						return;
						
			   default  :
						putch ( ch );
						buff[n++] = ch;
			}
			if (n>=256) { buff[n]=0; return; }
     }
}

/*
 *	Display a string on the next line in the console.
 *	Scroll up if necessary.
 */
void show_msg(char *msg)
{
CONINFO cinfo;

	con_getinfo( &cinfo );
	// if (cinfo.xpos) con_print_CR();
	con_clrline(24); 
	con_setcolor_blue();
	cprintf(msg);
	con_gotoxy(cinfo.xpos, cinfo.ypos);
	con_setcolor_normal();
}


/* 
 *		Get filename from user 
 */
void get_filename( char * prompt )
{
CONINFO cinfo;

	 con_getinfo( &cinfo );
	 con_clrline( 24 );
	 con_setcolor_red();
	 cprintf( prompt );
     get_str( FileName );		
	 
	 con_gotoxy(cinfo.xpos, cinfo.ypos);
	 con_setcolor_normal();
}


/*
 *		Show current settings
 */
void show_setting(void)
{
	char temp[100], txt[100], finaltxt[300];
	int n;

	if (SaveYes) strcpy(temp, FileName);
	else strcpy( temp, "n/a");

	sprintf( txt,"TM32 V%s (PORT=COM%d BAUD=%d MODE=%d FILE='%s')", 
		     Version, Port, BaudRate, MonitorMode, temp);
	n = (80-strlen(txt))/2;
	if (n)
	{
		memset(temp, '-', n-1);	temp[n-1]=0;
		finaltxt[0]=0;
		strcpy(finaltxt, temp);
		strcat(finaltxt, txt);
		strcat(finaltxt, temp);
		show_msg( finaltxt);
	}
	else show_msg( txt );
}

/*
 *		Convert 2 ASCII HEX codes to one unsigned 8 bit binary
 */
int hex2char(char *hex)
{
   int bin,i;

   for (i=0, bin=0; i < 2; i++, hex++)  
   {
       if (*hex !=' ' && isxdigit(*hex))
       {
		bin =(*hex > '9') ? bin*16+(*hex - '7') : bin*16+(*hex - '0');

       } else return 0;  /* non HEX code, return 0 */


   }
   return bin;
}



/*
 *		Check User's Key Press
 */
void check_key(void)
{
int		re, ch, xcnt;
int		chread, tmp;
char	txt[100], *p;

	if (kbhit()) {
		chread = getch();
		if (chread==0 || chread==0xE0)	// function key ?
		{
			chread = getch();
			chread = chread << 8;		// shift right
		}

	    switch (chread)   {

		case	F10   :  
				rs_close();
			    con_clrscr();
				cprintf("Send any bug report, feedback etc. to me (tanhweetiong@yahoo.com).\n");
				cprintf("Thank you for using TM32...see you soon!\n");
				
			    _fcloseall();
				con_close();
			    exit(0);
			    break;

		case	F1    :  
			    MonitorMode = MonitorMode++;
			    if (MonitorMode==4) MonitorMode=0;
				show_setting();			    
			    break;

		case	F2    :  
				_fcloseall();
				if (SaveYes) 
				{
					sprintf( txt, "TM32: Screen captured to '%s'.", FileName);
					show_msg( txt );
					Sleep(500);
				}
			    SaveYes = !SaveYes;
			    if (SaveYes)
			    {
					get_filename( "TM32: Screen capture file name: " );
					if (FileName[0])
					{
						Stream = fopen( FileName, "wb");
						if (!Stream) SaveYes = 0;
					}
					else
					{
						SaveYes = 0;
					}
			    }
			    show_setting();
			    break;

		case	F3    :  
				if (SaveYes) break;

				_fcloseall();
   				get_filename("TM32: Text file to upload : " );
			    Stream = fopen( FileName,"rb");
				if (Stream)		// file opened OK
				{
					con_clrscr();
					sprintf( txt, "TM32: Uploading '%s'...<ESC> to abort", FileName);
					show_msg( txt );
					con_gotoxy(0,0);

					xcnt = 0;			// column counter
					while ( (re=fgetc( Stream )) != EOF)
					{
						Sleep(5);
						rs_putc((char)(re & 0xff));
					
						ch = rs_getch();	// get char from COM port
						if (ch> 0) 
						{
							if (ch==LF)	// LF ?
							{ 
								xcnt = 0; con_print_CR();
							}
							else if (ch==CR)
							{
								xcnt = 0;
							}
							else 
							{ 
								if (++xcnt >= 79) 
								{ 
									xcnt = 0; con_print_CR(); 
								}
								if (ch==TAB) putch(' ');	// replace all TAB with ' '
								else putch(ch);
							}
						}
						if (kbhit()) { if (getch()==ESC) break; }		// user abort
					}
				}
			    show_setting();
			    break;

		case	F8    :  
				get_filename("TM32: HEX STRING (e.g. '$D0VN$0D') = " );
		case	F7		:
				p = FileName;
				for (;*p;p++)
				{
					if (*p=='$' && *(p+1)=='$')
					{
						rs_putc('$');
						p++;
					}
					else if (*p=='$')
					{
						tmp = *(p+3);	/* save it */
						*(p+3) = 0;		/* null-termianted */
						p++;
						rs_putc( hex2char(p) );	 /* convert to char */
						*(p+2) = tmp;			/* restore saved value */
						p++;
					}
					else rs_putc(*p);
				}
						
				show_setting();
				break;

		case	F5  :  
				con_clrscr();
				show_setting();
			    break;

		default  :  
				rs_putc((char)(chread & 0x00ff));
	    }
	}
}

/*
 *		HELP screen
 */
void show_usage(void)
{

		printf("TM32 Version ");
		printf(Version);
		printf(", by TAN HWEE TIONG copyright (c) 2001-2005\n\n");
		printf("  Usage:\n");
		printf("       TM32 <CommPort> <Baud>\n");
		printf("  e.g.:\n");
		printf("       C:\\>TM32 1 2400\n\n");
		printf("       Port = 1 - 16 ,  (COM1 to COM16)\n");
		printf("       Baud = 1200,2400,4800,9600,19200,38400,57600 or 115200\n");
		exit(1);
}

/*
 *		Handle different charceter display, <LF>,<CR> and the rest
 */
void display_ch(int ch)
{
		switch (ch)
		{
			case	LF:
					Xcount = 0;
					con_print_CR();
					break;
			case	CR:
					putch((char)ch);
					Xcount = 0;
					break;
			default:	
					putch((char)ch);
					if (++Xcount >= 79) { Xcount = 0; con_print_CR(); }
					break;
		}
		if (SaveYes) fputc(ch, Stream); 
}


void main(int argc,char *argv[])
{
int ch, re; 
unsigned int n;
char ok =0, temp[100];
struct _timeb timebuffer;


     switch(argc)
     {
		case  	1 :
				Port= 1;
				BaudRate = 9600;
				ok = 1;
				break;
		case    2 :
				if (!strcmp(argv[1], "/?"))
				{
					show_usage();
					break;
				}
				sscanf( argv[1], "%d", &Port);
				BaudRate = 9600;
				ok = 1;
				break;
		case    3 :
				sscanf( argv[1], "%d", &Port);
				sscanf( argv[2], "%d", &BaudRate);
				switch(BaudRate)
				{
					case 600	: 
				    case 1200	: 
					case 2400	: 
				    case 4800	: 
					case 9600	: 
				    case 19200	: 
					case 38400	:
					case 57600	: 
					case 115200 :
						 ok = 1;		   
						 break;
					default:
						 ok = 0;
				}
	}

	if (argc > 3 || !ok)
	{
		show_usage();	// show how-to
	}

	con_open();		// init console 
	re = rs_open(Port, BaudRate);	// init COM port
	if (re==COM_ERR)	// v 1.0.1
	{
		printf("TM32 Error: cannot open port 'com%d'!\n", Port);
		exit(1);
	}
	/* Welcome */
	con_clrscr();
	con_gotoxy( 10,7 ); 
	cputs("WELCOME TO TM32 (32 BIT VERSION FOR WINDOWS) VER ");
	cputs(Version);
	con_gotoxy( 15,9); 
	cputs("WRITTEN BY TAN HWEE TIONG (c) COPYRIGHT 2001-2003");

	con_gotoxy( 15,11 ); 
	cputs("F1  = Toggle Monitor Mode Between 0,1 and 2");
	con_gotoxy( 15,12 ); 
	cputs("F2  = Screen Capture to a Text File");
	con_gotoxy( 15,13 ); 
	cputs("F3  = Upload from a Text File");
	con_gotoxy( 15,14 ); 
	cputs("F5  = Clear Screen");
	con_gotoxy( 15,15 ); 
	cputs("F7  = Repeat F8 string");
	con_gotoxy( 15,16 ); 
	cputs("F8  = Send HEX coded string");
	con_gotoxy( 15,17 ); 
	cputs("F10 = Quit");

	con_gotoxy( 15,19 ); 
	cputs("** Press any key..");

	getch();

	con_clrscr();		// clr sreen
	show_setting();		// show bottom line current setting

	FileName[0]= 0;		// clear File name field

     /* MAIN PROGRAM LOOP */
     for(;;) 
	 {
		do
		{
			check_key();				// poll user key press
			ch = rs_getch();			// poll char from COM port
			if (ch < 0 ) Sleep(20);		// without this, CPU usage will raise to 70%-90%

		} while (ch < 0 );
		
		if (MonitorMode)       /* monitor mode 1 or 2 is ON */
	    {
			if (flg_timestamp)
			{
				  _ftime( &timebuffer );
				  sprintf(  temp, "%u.%03u", timebuffer.time ,timebuffer.millitm);
				  con_setcolor_red();
				  for (n=0; n < strlen(temp); n++) display_ch( temp[n]);
				  con_setcolor_normal();
				  flg_timestamp = 0;
			}

			if (ch >= 0x20  && ch < 0x7F)	// displayable characters
			{
				display_ch((char)ch);
			}
			else
			{
				sprintf( temp, "<%02X>", ch & 0xFF);	// show as HEX digits
				con_setcolor_green();
				for (n=0; n < strlen(temp); n++) display_ch( temp[n]);
				con_setcolor_normal();
				if (ch==CR && MonitorMode==2)		// in MON=2, CR will scroll the page
				{ 
					display_ch(LF); 
				}
				if (ch==CR && MonitorMode==3)		// in MON=3, CR will scroll the page, and display TimeStamp on next char
				{ 
					display_ch(LF); flg_timestamp = 1;
				}
			}
	    }
	    else	// MON=0
	    {
			display_ch( ch);
			check_key();
	    }
	}
}

/* EOF */