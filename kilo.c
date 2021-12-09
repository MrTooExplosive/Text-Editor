#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>

#define KILO_VERSION "0.0.1"
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}

typedef struct erow
{
	int size;
	char *chars;
} erow;

enum editorKey
{
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};

struct editorConfig
{
	int cx, cy;
	int screenrows, screencols;
	struct termios orig_termios;
	int numrows;
	erow *row;
	int rowoff;
};
struct editorConfig E;

struct abuf
{
	char *b;
	int len;
};

void editorOpen(char *filename);
void editorMoveCursor(int key);
void abFree(struct abuf *ab);
void abAppend(struct abuf *ab, const char *s, int len);
void enableRawMode();
void disableRawMode();
void die(const char *s);
void editorProcessKeypress();
int editorReadKey();
void editorRefreshScreen();
void editorDrawRows(struct abuf *ab);
int getWindowSize(int *rows, int *cols);
void initEditor();
int getCursorPosition(int *rows, int *cols);
void editorAppendRow(char *s, size_t len);
void editorScroll();

int main(int argc, char *argv[])
{
	enableRawMode();
	initEditor();
	if (argc >= 2)
		editorOpen(argv[1]);


	while (1)
	{
		editorRefreshScreen();
		editorProcessKeypress();
	}
	return 0;
}
// Checks if the cursor is not visible and if so, scrolls
void editorScroll()
{
	if (E.cy < E.rowoff)
		E.rowoff = E.cy;
	if (E.cy >= E.rowoff + E.screenrows)
		E.rowoff = E.cy - E.screenrows + 1;
}

// Appends a row to the printing buffer
void editorAppendRow(char *s, size_t len)
{
	E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
	int at = E.numrows;
	E.row[at].size = len;
	E.row[at].chars = malloc(len + 1);
	memcpy(E.row[at].chars, s, len);
	E.row[at].chars[len] = '\0';
	E.numrows++;
}

// Opens the editor for reading and writing to a file
void editorOpen(char *filename)
{
	FILE *fp = fopen(filename, "r");
	if (!fp)
		die("fopen");
	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while ((linelen = getline(&line, &linecap, fp)) != -1)
	{
		if (linelen != -1)
		{
			while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
				linelen--;
			editorAppendRow(line, linelen);
		}
	}
	free(line);
	fclose(fp);
}

// Moves the cursor
void editorMoveCursor(int key)
{
	switch (key)
	{
		case ARROW_LEFT:
			if (E.cx != 0)
				E.cx--;
			break;
		case ARROW_RIGHT:
			if (E.cx != E.screencols - 1)
				E.cx++;
			break;
		case ARROW_UP:
			if (E.cy != 0)
				E.cy--;
			break;
		case ARROW_DOWN:
			if (E.cy < E.numrows)
				E.cy++;
			break;
	}
}

// Appends to a dynamic string
void abAppend(struct abuf *ab, const char *s, int len)
{
	char *new = realloc(ab->b, ab->len + len);
	if (new == NULL)
		return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

// Frees a buffer
void abFree(struct abuf *ab)
{
	free(ab->b);
}

// Stores the cursor position
int getCursorPosition(int *rows, int *cols)
{
	char buf[32];
	unsigned  int i = 0;
	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
		return -1;
	while (i < sizeof(buf) - 1)
	{
		if (read(STDIN_FILENO, &buf[i], 1) != 1)
			break;
		if (buf[i] == 'R')
			break;
		i++;
	}
	buf[i] = '\0';
	if (buf[0] != '\x1b' || buf[1] != '[')
		return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
		return -1;
	return 0;
}

// Initializes the editor
void initEditor()
{
	E.cx = 0;
	E.cy = 0;
	E.numrows = 0;
	E.row = NULL;
	E.rowoff = 0;
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
void editorDrawRows(struct abuf *ab)
{
	for (int y = 0; y < E.screenrows; y++)
	{
		int filerow = y + E.rowoff;
		if (filerow >= E.numrows)
		{
			if (E.numrows == 0 && y == E.screenrows / 3)
			{
				char welcome[80];
				int welcomelen = snprintf(welcome, sizeof(welcome), "Kilo editor -- version %s", KILO_VERSION);
				if (welcomelen > E.screencols)
					welcomelen = E.screencols;
				int padding = (E.screencols - welcomelen) / 2;
				if (padding)
				{
					abAppend(ab, "~", 1);
					padding--;
				}
				while (padding--) abAppend(ab, " ", 1);
				abAppend(ab, welcome, welcomelen);
			}
			else
				abAppend(ab, "~", 1);
		}
		else
		{
			int len = E.row[y].size;
			if (len > E.screencols)
				len = E.screencols;
			abAppend(ab, E.row[y].chars, len);
		}
		abAppend(ab, "\x1b[K", 3);
		if (y < E.screenrows - 1)
			abAppend(ab, "\r\n", 2);
	}
}

// Clears the screen
void editorRefreshScreen()
{
	editorScroll();
	struct abuf ab = ABUF_INIT;
	abAppend(&ab, "\x1b[?25l", 6);
	abAppend(&ab, "\x1b[H", 3);
	editorDrawRows(&ab);
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
	abAppend(&ab, buf, strlen(buf));
	abAppend(&ab, "\x1b[?25h", 6);
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

// Reads a key from the terminal
int editorReadKey()
{
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
	{
		if (nread == -1 && errno != EAGAIN)
			die("read");
	}

	if (c == '\x1b')
	{
		char seq[3];
		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

		if (seq[0] == '[')
		{
			if (seq[1] >= '0' && seq[1] <= '9')
			{
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
				if (seq[2] == '~')
				{
					switch (seq[1])
					{
						case '1': return HOME_KEY;
						case '3': return DEL_KEY;
						case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
					}
				}
			}
			else
			{
				switch (seq[1])
				{
					case 'A': return ARROW_UP;
					case 'B': return ARROW_DOWN;
					case 'C': return ARROW_RIGHT;
					case 'D': return ARROW_LEFT;
					case 'H': return HOME_KEY;
					case 'F': return END_KEY;
				}
			}
		}
		else if (seq[0] == 'O')
		{
			switch (seq[1])
			{
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
			}
		}
		return '\x1b';
	}
	else
		return c;
}

// Process each key read
void editorProcessKeypress()
{
	int c = editorReadKey();
	switch (c)
	{
		case CTRL_KEY('q'):
			write(STDOUT_FILENO, "\x1b[2J", 4);
			write(STDOUT_FILENO, "\x1b[H", 3);
			exit(0);
			break;
		case HOME_KEY:
			E.cx = 0;
			break;
		case END_KEY:
			E.cx = E.screencols - 1;
			break;
		case PAGE_UP:
		case PAGE_DOWN:
		{
			int times = E.screenrows;
			while (times--)
				editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
		}
			break;
		case ARROW_UP:
		case ARROW_DOWN:
		case ARROW_LEFT:
		case ARROW_RIGHT:
			editorMoveCursor(c);
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
