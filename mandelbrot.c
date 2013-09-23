#include <termctl.h>
#include <string.h>	// for memset
#include <math.h>
#include <unistd.h>
#include <stdlib.h>

const double MOVC = 0.35;
int ITER_MAX = 100;
int wd, ht;
double ca = -0.5;
double cb = 0.0;
double sa = 1.8;
double ar, sb, la, lb, stepa, stepb;

bool helping = false;

void init (){
	wd = get_width();
	ht = get_height();
	ar = ((double) wd)/ht;
	sb = 2 * sa / ar;	// factor of two to account for stretching of terminal.
	la = ca - sa;
	lb = cb - sb;
	stepa = (2*sa)/wd;
	stepb = (2*sb)/ht;
}

void set_center (double a, double b){
	ca = a;
	cb = b;
	la = ca - sa;
	lb = cb - sb;
}

void set_zoom (double size_a){
	sa = size_a;
	sb = 2 * sa / ar;
	stepa = (2*sa)/wd;
	stepb = (2*sb)/ht;
	la = ca - sa;
	lb = cb - sb;
}

int iters (double a, double b){
	int ct = 0;
	double za = a;
	double zb = b;
	while (za*za + zb*zb < 4 && ct < ITER_MAX){
		// z = z^2 + c
		double tmp = za*za - zb*zb;
		zb = 2*za*zb + b;
		za = tmp + a;
		ct++;
	}
	if (ct == ITER_MAX){
		return -1;
	}
	return ct;
}

double linear = 1;
double exponent = 0.75;
double offset = 0;
const int ncolors = 6;
const int colors[] = {YELLOW, RED, MAGENTA, BLUE, CYAN, GREEN};

int get_color (int iters){
	if (iters == -1) return BLACK;
	double d = pow(iters, exponent);
	d *= linear;
	while (offset < 0){
		offset += ncolors;
	}
	d += offset;
	int idx = (int) d;
	idx %= ncolors;
	return colors[idx];
}

void render (){
	for (int i=0; i<ht; i++){
		double b = lb + i*stepb;
		for (int j=0; j<wd; j++){
			double a = la + j*stepa;
			int res = iters(a,b);
			color(BG, get_color(res));
			putchar(' ');
		}
		putchar('\n');
	}
	color(BG, BLACK);
	setpos(ht, 1);
	printf("Iters: %d\tSize: (%f, %f) \tCenter: (%f, %f)\n", ITER_MAX, sa, sb, ca, cb);
}

void help (){
	helping = true;
	int left = 10;
	int right = wd - 10;
	int top = 5;
	int bot = ht - 5;
	color(FG, WHITE);
	color(BG, BLACK);
	char blanks[right-left+1];
	memset(blanks, ' ', right-left);
	blanks[right-left] = 0;
	for (int i=top; i<bot; i++){
		setpos(i, left);
		printf("%s", blanks);
	}
	setpos(top+1, left+1);
	printf("MANDELBROT HELP");
	setpos(top+3, left+1);
	printf("= -        Increase or decrease zoom");
	setpos(top+5, left+1);
	printf("w a s d    Move up, down, left, right");
	setpos(top+7, left+1);
	printf("[ ]        Decrease or increase # of iterations");
	setpos(top+9, left+1);
	printf("u j        Increase or decrease coloring exponent");
	setpos(top+11, left+1);
	printf("i k        Increase or decrease coloring linear factor");
	setpos(top+13, left+1);
	printf("o l        Increase or decrease coloring offset");
	setpos(top+15, left+1);
	printf("c          Reset to defaults");
	setpos(top+17, left+1);
	printf("h          Show or hide this help");
	setpos(top+19, left+1);
	printf("q          Quit");
	fflush(stdout);
}


int main (){
	init();
//	printf("L = (%f, %f); S = (%f, %f); ar = %f; step = (%f, %f)\n", la, lb, sa, sb, ar, stepa, stepb);
	erase_all();
	render();
	keyboard_mode (KEY_INTERACTIVE);
	ikb_block(true);
	while (true){
		char inp = 'z';	// meaningless
		read(0, &inp, 1);
		if (inp != 0 && inp != -1){
			if (inp == '='){
				set_zoom(0.8 * sa);
			} else if (inp == '-'){
				set_zoom(1.25 * sa);
			} else if (inp == 'w'){
				set_center(ca, cb - MOVC * sb);
			} else if (inp == 's'){
				set_center(ca, cb + MOVC * sb);
			} else if (inp == 'a'){
				set_center(ca - MOVC * sa, cb);
			} else if (inp == 'd'){
				set_center(ca + MOVC * sa, cb);
			} else if (inp == '['){
				ITER_MAX = (int) (ITER_MAX * 0.6666666);
			} else if (inp == ']'){
				ITER_MAX = (int) (ITER_MAX * 1.5);
			} else if (inp == 'u'){
				exponent += 0.05;
			} else if (inp == 'j'){
				exponent -= 0.05;
			} else if (inp == 'i'){
				linear *= 1.5;
			} else if (inp == 'k'){
				linear *= 0.666666;
			} else if (inp == 'o'){
				offset ++;
			} else if (inp == 'l'){
				offset --;
			} else if (inp == 'h'){
				if (helping){
					helping = false;
					render();
				} else {
					help();
				}
			} else if (inp == 'q'){
				erase_all();
				color(BG, ORIGINAL);
				color(FG, ORIGINAL);
				setpos(1,1);
				keyboard_mode(KEY_NORMAL);
				ikb_block(false);
				exit(0);
			} else if (inp == 'c'){
				ITER_MAX = 100;
				set_center(-0.5, 0.0);
				set_zoom(1.8);
				exponent = 0.8;
				linear = 1;
				offset = 0;
			} else {
				continue;
			}
			setpos(1,1);
			if (!helping){
				render();
			}
		}
	}
}
