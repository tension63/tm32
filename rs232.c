/*
	RS232.C

	WIN32 SERIAL COMMUNICATION SIMPLIFIED API

	copyright (c) 1999-2001 by HT TAN 
*/
#define WIN32_LEAN_AND_MEAN
#define SERIAL

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "rs232.h"
#include "console.h"


/* local */
static HANDLE		 comHandle=0;
static DCB			 comDcb;	
static COMMTIMEOUTS  comTimeouts;
static char			 comDebug=1;


static void error_handler(char *msg, DWORD err)
{
	if (comDebug)
	{
		con_print_CR();
		printf( "<'%s'. Error number: %d>" , msg, err);
		con_print_CR();
	}
	ExitProcess(err);
}

static void show_msg(char *msg)
{
	if (comDebug)
	{
		con_print_CR();
		printf("[");
		printf( msg );
		printf("]");
		con_print_CR();
	}
}
/*
 * Set debug Mode ON/OFF
*/	
void rs_debug(int mode)
{
	comDebug = mode;
}

/*
 * Open PC's COM port
 */
int rs_open( int portno, int baudrate )
{
	BOOL success;
	char msg[256];

	if (comHandle) return 0;

	if (portno < 0 || portno > 99) return COM_ERR;		// range check

	if (portno < 10)
		wsprintf( CommDevice, "com%d", portno);
	else
		wsprintf( CommDevice, "\\\\.\\com%d", portno);
	// Open COM port
	comHandle = CreateFile( CommDevice, 
							GENERIC_READ|GENERIC_WRITE,
							0,0,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL, 0);

	if (comHandle == INVALID_HANDLE_VALUE) 
	{
		wsprintf( msg, "RS232 DEBUG: it can not open '%s'.",CommDevice );
		show_msg(msg);
		return COM_ERR;
	}

	// Get Current DCB values
	success = GetCommState(comHandle, &comDcb);
	
	if (!success) 
	{
		error_handler("rs_open() in GetCommState", GetLastError());
		return COM_ERR;
	}
	// modify DCB values
	comDcb.BaudRate = baudrate;
	comDcb.ByteSize = 8;
	comDcb.Parity = NOPARITY;
	comDcb.StopBits = ONESTOPBIT;
	comDcb.fNull = TRUE;				// filter-out NULL
	comDcb.fOutxCtsFlow = FALSE;		// no CTS flow cotnrol
	comDcb.fOutxDsrFlow  = FALSE;		// no DSR flow control
	comDcb.fAbortOnError = FALSE;		// continue operation upon error
	
	// Apply new changes
	success = SetCommState(comHandle, &comDcb);
	if (!success) 
	{
		sprintf( msg, "RS232 DEBUG: fail to set state on '%s'.",CommDevice );
		show_msg( msg );
		return 1;
	}
	
	// Change Receive TimeOut Value
	comTimeouts.ReadIntervalTimeout = MAXDWORD;
	comTimeouts.ReadTotalTimeoutMultiplier = 0;
	comTimeouts.ReadTotalTimeoutConstant = 0;
	comTimeouts.WriteTotalTimeoutMultiplier = 0;
	comTimeouts.WriteTotalTimeoutConstant = 0;
	SetCommTimeouts( comHandle, &comTimeouts);

	return 0;	

}

/* 
 *  return number of charaters received,
 *  return 0 = if empty or error
 *  else buffer contains the string
 */
int rs_read( char *buffer, int buffsize )
{

	BOOL success;
	DWORD numread;

	if (!comHandle) return 0;  // port unopened error

	buffer[0]=0;

	success = ReadFile(comHandle, buffer, buffsize, &numread, 0);
	
	if (numread) buffer[numread] = 0; // null-termianted;
	
	if (!success) return 0; // error

	return numread;

}

/* 
 *  return one charater received,
 *  return < 0 if empty or error
 *  else return the character (from 0 to 255)
 */
int rs_getch( void )
{
unsigned char buffer[10];

	BOOL success;
	DWORD numread;

	if (!comHandle) return -3;  // port not open error

	buffer[0]=0;

	success = ReadFile(comHandle, buffer, 1, &numread, 0);
	
	if (numread) 
	{
			return (int)buffer[0]; 
	}
	
	if (!success) return -1; // error

	return -2;	// empty

}

/* 
 *  Write a 'num' of bytes from buffer to the com port
 *  return 0 if sucess
 *  return 0 = if error
 */
int rs_write( char *buffer, int numbytes )
{
	BOOL  success;
	DWORD numwrite;

	success = WriteFile(comHandle, buffer, numbytes,
		                &numwrite, 0);
	
	if (!success) 
	{
		error_handler("rs_write() in WriteFile", GetLastError());
		return COM_ERR;
	}
	return 0;
}


int rs_putc( char ch)
{
	char buffer;

	buffer = ch;
	return rs_write(&buffer, 1);
}

/* 
 *  Write a string to the com port
 *  return 0 if sucess
 *  return 0 = if error
 */
int rs_puts( char *str )
{
	BOOL  success;
	DWORD numwrite;

	success = WriteFile(comHandle, str, strlen(str),
		                &numwrite, 0);
	
	if (!success) 
	{
		error_handler("rs_puts() in WriteFile", GetLastError());
		return COM_ERR;
	}
	return 0;
}

/*
 *    Close down COM port
 */
void rs_close( void )
{
	if (comHandle) 
	{
		CloseHandle(comHandle);
		comHandle = 0;
	}

}

/*
 *	Serial port polling
 *	Please note: size of 'buffer' must be at least 1000 bytes (e.g. RT commnd)
 *	Return	0 : sucess
 *			1 : error
 */
int rs_poll(char *cmdstr, char *buffer, int timeoutsec)
{
	char resp[1024], msg[256];
	int tout, len;

	if (!comHandle) return COM_ERR;  // port unopened yet!

	// Send command out serial port
	if (rs_puts( cmdstr ))
	{
		sprintf( msg, "RS232 DEBUG: cannot send [%s] to '%s'.\n", cmdstr, CommDevice);
		show_msg( msg );
		return COM_ERR; // error found
	}
	
	// Wait for SLAVE DEVICE to respond
	*buffer=0; tout = timeoutsec * 100;	//  second timeout period
	while(1)
	{
			len = rs_read( resp, 1000 );		// ** MAX RETURN BYTES = 1000
			if (len)
			{
				strcat(buffer, resp);			// add characters to buffer		    

				if (strchr(resp,0x0D))			// search for ELID's terminating char = <ES>
					break;	
				
			}
			if (--tout==0) break;  /* Timeout error */
			Sleep(10);				
	}

	
	if (!tout) // timeout!		
	{
		sprintf( msg, "RS232 DEBUG: ** host reply timeout!" );
		show_msg( msg );
		return COM_ERR;			// Error found
	}
    else  
	{
		sprintf( msg, "RS232 DEBUG: reply '%s'.", buffer);
		show_msg( msg );
		
	}
		
	return 0;		// perfect, no error!
		
		
}

/*
 *	Get a string from the com port of 'num' char
 *  return:
 *		TIMEOUT ERR	=>	0
 *		OK			=>	actual numbytes received
 */	
int rs_gets(int num, int timeoutsec, char *buffer)
{
	char resp[1024];
	int tout, len, totallen;

	*buffer= totallen = 0; 
	tout = timeoutsec * 100;	//  second timeout period
	// Wait for SLAVE DEVICE to respond
	while(1)
	{
			len = rs_read( resp, num );			
			if (len)
			{
				strcat(buffer, resp);			// add characters to buffer		    
				return len;
			}
			if (--tout==0) break;  /* Timeout error */
			Sleep(10);				
	}
	
	return 0;		// TIMEOUT!
}
/* EOF */