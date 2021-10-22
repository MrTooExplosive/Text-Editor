#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

#define CTRL_KEY(k) ((k) & 0x1f)

void enableRawMode();
void disableRawMode();
void die(const char *s);
void editorProcessKeyPress();
char editorReadKey();

struct termios orig_termios;

int main()
{
	enableRawMode();

	char c;
	while (1)
		editorProcessKeypress();
	return 0;
}

// Reads a key from the terminal
char editorReadKey()
{
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
	{
		if (nread == -1 && errno != EAGAIN)
			die("read");
	}
	return c;
}

// Process each key read
void editorProcessKeypress()
{
	char c = editorReadKey();
	switch (c)
	{
		case CTRL_KEY('q'):
			exit(0);
			break;
	}
}

// Prints what the error was and terminates
void die(const char *s)
{
	perror(s);
	exit(1);
}

// Returns the terminal to normal mode
void disableRawMode()
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) ==  -1)
		die("tcsetattr");
}

// Enables raw mode for character input
void enableRawMode()
{
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
	atexit(disableRawMode);

	struct termios raw = orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_oflag |= ~(CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_CC[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}
