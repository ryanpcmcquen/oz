/*** Includes: ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/*** Defines: ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** Data: ***/
struct editorConfig {
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

struct editorConfig E;

/*** Output: ***/
void editorDrawRows() {
    int y;
    for (y = 0; y < E.screenrows; y++) {
        write(STDOUT_FILENO, "\r\n", 3);
    }
}
void clearScreen() {
    write(STDOUT_FILENO, "\x1b[2j", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}
void editorRefreshScreen() {
    clearScreen();

    editorDrawRows();

    write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** Terminal: ***/
void die(const char *s) {
    clearScreen();

    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
        die("tcsetattr");
    }
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) {
        die("tcgetattr");
    }
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;
    tcgetattr(STDIN_FILENO, &raw);

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("tcsetattr");
    }
}

char editorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            die("read");
        }
    }
    return c;
}

int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
        return -1;
    }

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) {
            break;
        }
        if (buf[i] == 'R') {
            break;
        }
        i++;
    }
    buf[i] = '\0';

    printf("\r\n&buf[1]: '%s'\r\n", &buf[1]);

    editorReadKey();

    return -1;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;

    if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}


/*** Input: ***/
void editorProcessKeypress() {
    char c = editorReadKey();

    switch (c) {
    case CTRL_KEY('q'):
        clearScreen();
        exit(0);
        break;
    }
}

/*** Init: ***/
void initEditor() {
    if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
        die("getWindowsize");
    }
}

int main() {
    enableRawMode();
    initEditor();

    while (1) {

        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
