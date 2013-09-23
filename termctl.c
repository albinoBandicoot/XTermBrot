#include <termctl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <time.h>

unsigned int cattribs = 0;	// current attributes set.
unsigned int fg_col = 9;
unsigned int bg_col = 9;
unsigned int keymode = KEY_NORMAL;
unsigned int keymode_save = KEY_NORMAL;
bool mouse_enabled = false;
bool blocking = true;	// only for interactive keyboard input.

void csi (){
	putchar(0x1B);
	putchar('[');
}

void up (int x){
	csi();
	printf("%dA", x);
}

void down (int x){
	csi();
	printf("%dB", x);
}

void right (int x){
	csi();
	printf("%dC", x);
}

void left (int x){
	csi();
	printf("%dD", x);
}

void setpos (int row, int col){
	csi();
	printf("%d;%dH", row, col);
}

void rsetpos (int x, int y){
}

void setline (int x){
	csi();
	printf("%dd", x);
}

void setcol (int x){
	csi();
	printf("%dG", x);
}

/* Erasing functions */

void erase (int opt){
	csi();
	printf("%dJ", opt);
}

void erase_all (){
	erase(2);
}

void erase_above(int row){
	setline(row);
	erase(1);
}

void erase_below(int row){
	setline(row);
	erase(0);
}

void erasel (int opt){
	csi();
	printf("%dK", opt);
}

void erase_line (){
	erasel(2);
}

void erase_line_left (int col){
	setcol(col);
	erasel(1);
}

void erase_line_right(int col){
	setcol(col);
	erasel(0);
}

/* Insertion and deletion of lines */

void insert_lines (int x){
	csi();
	printf("%dL", x);
}

void delete_lines (int x){
	csi();
	printf("%dM", x);
}

/* Scrolling */

void scroll (int x){
	if (x == 0) return;
	csi();
	if (x < 0){	// scroll down
		printf("%dT", -x);
	} else {	// scroll up
		printf("%dS", x);
	}
}

/* Getting the terminal width and height */

int get_width (){
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	return w.ws_col;
}

int get_height (){
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	return w.ws_row;
}

/* Switching between canonical and non-canonical modes */

/* Note that the keyboard mode switch is deactivated when the mouse mode is
 * on, since we need to not have the terminal's responses barfed all over
 * the screen. 
 */
void keyboard_mode (int mode){
	if (mouse_enabled){
		return;
	}
	struct termios t;
	tcgetattr(0, &t);
	if (mode == KEY_NORMAL){
		t.c_lflag |= ICANON;
		t.c_lflag |= ECHO;
		t.c_cc[VMIN] = 1;
		tcsetattr(0, TCSADRAIN, &t);
	} else if (mode == KEY_INTERACTIVE){
		t.c_lflag &= ~ICANON;
		t.c_lflag &= ~ECHO;
		t.c_cc[VMIN] = 0;
		t.c_cc[VTIME] = 1;	// 1 decisecond (100 ms)
		tcsetattr(0, TCSANOW, &t);
	}
	keymode = mode;
}

int get_keyboard_mode (){
	return keymode;
}
/* read() has two possible modes, controlled by the VMIN property. If VMIN is 0, read() will not block,
 * but will return -1 if no data is available. If VMIN is positive (here we just use 1), it will block.
 *
 * read (int filedes, void *buf, size_t nbytes)
 *
 * For stdin, filedes = 0. 
 */
void ikb_block (bool block){
	if (block != blocking){
		struct termios t;
		tcgetattr(0, &t);
		t.c_cc[VMIN] = block ? 1 : 0;
		tcsetattr(0, TCSANOW, &t);
		blocking = block;
	}
}

/* Setting of attributes and colors */ 

static void setattr (const char *attr){
	csi();
	printf("%sm", attr);
}

void attr (int attribs){
	int changes = attribs ^ cattribs;
//	printf("((DEBUG: attribs = %hx; cattribs = %hx; changes = %hx))\n", attribs, cattribs, changes);
	if (changes & BOLD){
		if (attribs & BOLD) 
			setattr(CH_BOLD);
		else 
			setattr(CH_NO_BOLD);
	}
	if (changes & UNDERLINE){
		if (attribs & UNDERLINE)
			setattr(CH_UNDERLINE);
		else 
			setattr(CH_NO_UNDERLINE);
	}
	if (changes & INVERSE){
		if (attribs & INVERSE)
			setattr(CH_INVERSE);
		else 
			setattr(CH_NO_INVERSE);
	}
	cattribs = attribs;
}

void color (int where, int col){
	if (where == FG){
		if (fg_col == col) return;
		fg_col = col;
		csi();
		printf("3%dm", fg_col);
	} else if (where == BG){
		if (bg_col == col) return;
		bg_col = col;
		csi();
		printf("4%dm", bg_col);
	}
}

void cleanup (){
	keyboard_mode (KEY_NORMAL);
	attr(0);
	color(FG, ORIGINAL);
	color(BG, ORIGINAL);
}

void force_cleanup (){
	disable_mouse();
	keyboard_mode (KEY_NORMAL);
	setattr("0");
	color (FG, ORIGINAL);
	color (BG, ORIGINAL);
}

/* Mouse handling */

void enable_mouse (){
	if (mouse_enabled) return;
	csi();
	printf("?%dh", 1000);
	// do some somewhat non-portable crap to suppress it barfing the stuff onto the terminal.
	keymode_save = keymode;
	if (keymode == KEY_NORMAL){
		keyboard_mode(KEY_INTERACTIVE);	// suppress echoing etc.
	}
	mouse_enabled = true;
}

void disable_mouse (){
	if (!mouse_enabled) return;
	csi();
	printf("?%dh", 1000);
	mouse_enabled = false;
	if (keymode_save == KEY_NORMAL){
		keyboard_mode(KEY_NORMAL);
	}
}

int query_mouse (struct mouse_event_t *me){
	unsigned char c, b, x, y;
	if ((c = getchar()) != EOF){
		if (c == 0x1B){	// we have an escape sequence!
			c = getchar();
			if (c == '['){	// CSI has been found.
				if ((c = getchar()) == 'M'){
					b = getchar();
					x = getchar();
					y = getchar();
					me->x = x-32;
					me->y = y-32;
					me->button = b % 4;
					return 0;
				}
			}
		}
	} else {
		ungetc(c, stdin);
	}
	return -1;
}
