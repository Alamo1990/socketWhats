#include <unistd.h>
#include <errno.h>
#include "read_line.h"

int send_msg(int socket, char *message, int length)
{
	int r;
	int l = length;


	do {
		r = write(socket, message, l);
		l = l -r;
		message = message + r;
	} while ((l>0) && (r>=0));

	if (r < 0)
		return (-1);   /* fail */
	else
		return(0);	/* success */
}

int recv_msg(int socket, char message[4][256], int length)
{
	int r;
	int l = length;


	do {
		r = read(socket, *message, l);
		l = l -r ;
		message++;
	} while ((l>0) && (r>=0));

	if (r < 0)
		return (-1);   /* fail */
	else
		return(0);	/* success */
}



ssize_t readLine(int fd, void *buffer, size_t n)
{
	ssize_t numRead;  /* num of bytes fetched by last read() */
	size_t totRead;	  /* total bytes read so far */
	char *buf;
	char ch;


	if (n <= 0 || buffer == NULL) {
		errno = EINVAL;
		return -1;
	}

	buf = buffer;
	totRead = 0;

	for (;;) {
        	numRead = read(fd, &ch, 1);	/* read a byte */

        	if (numRead == -1) {
            		if (errno == EINTR)	/* interrupted -> restart read() */
                		continue;
            	else
			return -1;		/* some other error */
        	} else if (numRead == 0) {	/* EOF */
            		if (totRead == 0)	/* no byres read; return 0 */
                		return 0;
			else
                		break;
        	} else {			/* numRead must be 1 if we get here*/
            		if (ch == '\n')
                		break;
            		if (ch == '\0')
                		break;
            		if (totRead < n - 1) {		/* discard > (n-1) bytes */
				totRead++;
				*buf++ = ch;
			}
		}
	}

	*buf = '\0';
    	return totRead;
}
