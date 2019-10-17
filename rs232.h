/*
	rs232.h

	Easy WIN32-based Serial Port Communication API

*/

int rs_open( int portno, int baudrate );
int rs_read(  char *buffer, int buffsize );
int rs_puts(  char *cmdstr );
int rs_putc( char ch);
void rs_close( void );
int rs_poll(char *cmdstr, char *buffer, int timeoutsec);
int rs_gets(int num, int timeoutsec, char *buffer);
int rs_write( char *buffer, int numbytes );
int rs_getch( void );
void rs_debug(int mode);

#define COM_ERR		1




#ifdef SERIAL

char CommDevice[20];

#else

extern char CommDevice[];

#endif
/* EOF */
