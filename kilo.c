#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>

#define CTRL_KEY(k) ((k) & 0x1f)

void enableRawMode();
void disableRawMode();
void die(const char *s);
void editorProcessKeypress();
char editorReadKey();
void editorRefreshScreen();
void editorDrawRows();
int getWindowSize(int *rows, int *cols);
void initEditor();
int getCursorPosition(int *rows, int *cols);

struct editorConfig
{
	int screenrows, screencols;
	struct termios orig_termios;
};
struct editorConfig E;

int main()
{
	enableRawMode();
	initEditor();

	while (1)
	{
		editorRefreshScreen();
		editorProcessKeypress();
	}
	return 0;
}

// Stores the cursor position
int getCursorPosition(int *rows, int *cols)
{
	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
		return -1;
	printf("\r\n");
	char c;
	while (read(STDIN_FILENO, &c, 1) == 1)
	{
		if (iscntrl(c))
			printf("%d\r\n", c);
		else
			printf("%d ('%c')\r\n", c, c);
	}
	editorReadKey();
	return -1;
}

// Initializes the editor
void initEditor()
{
	if (getWindowSize(&E.screenrows, &E.screencols) == -1)
		die("getWindowSize");
}

// Stores the number of rows and columns
int getWindowSize(int *rows, int *cols)
{
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
	{
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999C", 12) != 12)
			return -1;
		return getCursorPosition(rows, cols);
	}
	else
	{
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

// Draws a tilde at the beginning of each line
void editorDrawRows()
{
	for (int y = 0; y < E.screenrows; y++)
		write(STDOUT_FILENO, "~\r\n", 3);
}

// Clears the screen
void editorRefreshScreen()
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	editorDrawRows();
	write(STDOUT_FILENO, "\x1b[H", 3);
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
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
	}
}

// Prints what the error was and terminates
void die(const char *s)
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	perror(s);
	exit(1);
}

// Returns the terminal to normal mode
void disableRawMode()
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) ==  -1)
		die("tcsetattr");
}

// Enables raw mode for character input
void enableRawMode()
{
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
	atexit(disableRawMode);

	struct termios raw = E.orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_oflag |= ~(CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}
