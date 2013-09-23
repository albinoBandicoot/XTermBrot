#ifndef TERMCTL_H
#define TERMCTL_H
#include <stdio.h>

#define bool unsigned char
#define true 1
#define false 0

extern void up (int);
extern void down (int);
extern void right (int);
extern void left (int);

extern void setpos (int, int);
extern void rsetpos (int, int);
extern void setline (int);
extern void setcol  (int);

extern void erase_all ();
extern void erase_above (int);
extern void erase_below (int);

extern void erase_line ();
extern void erase_line_left (int);
extern void erase_line_right (int);

extern void insert_lines (int);
extern void delete_lines (int);

extern void scroll (int);

extern int get_width ();
extern int get_height ();

#define KEY_NORMAL	0
#define KEY_INTERACTIVE	1
extern void keyboard_mode (int);
extern int  get_keyboard_mode ();
extern void ikb_block (bool);	// sets whether the read() command will block or not.

/* We will define a bit for each attribute that can be set:
 * bit 0: boldness
 * bit 1: underline
 * bit 2: inverse
 */

#define BOLD		1
#define UNDERLINE	2
#define INVERSE		4

#define CH_BOLD	"1"
#define CH_UNDERLINE "4"
#define CH_INVERSE "7"
#define CH_NO_BOLD "22"
#define CH_NO_UNDERLINE "24"
#define CH_NO_INVERSE "27"

// colors
#define FG	3
#define BG	4

#define BLACK	0
#define RED	1
#define GREEN	2
#define YELLOW	3
#define BLUE	4
#define MAGENTA	5
#define CYAN	6
#define WHITE	7
#define ORIGINAL	9

extern void attr (int attribs);
extern void color (int fg, int color);

extern void cleanup ();
extern void force_cleanup();

/* Mouse Tracking */

struct mouse_event_t {
	int x;
	int y;
	char button;
};


extern void enable_mouse ();
extern void disable_mouse ();
extern int query_mouse (struct mouse_event_t*);

#endif
