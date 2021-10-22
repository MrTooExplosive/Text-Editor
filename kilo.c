#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

void enableRawMode();
void disableRawMode();

struct termios orig_termios;

int main()
{
	enableRawMode();
	char c;
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q')
	{
		if (iscntrl(c))
			printf("%d\n", c);
		else
			printf("%d ('%c')\r\n", c, c);
	}

	return 0;
}

void disableRawMode()
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode()
{
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);

	struct termios raw = orig_termios;
	raw.c_iflag &= ~(ICRNL | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
