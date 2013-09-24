#include <termctl.h>
#include <string.h>	// for memset
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

const double MOVC = 0.35;
int ITER_MAX = 100;
int wd, ht;
double ca = -0.5;
double cb = 0.0;
double sa = 1.8;
double ar, sb, la, lb, stepa, stepb;

bool helping = false;

bool  use_dither = true;
bool  use_overlays = true;
bool  use_ramp = false;
short *error;
float (*iters)(double, double);	// what function to use to get the iteration count. 
int power = 3;

typedef struct {
	short r;
	short g;
	short b;
} color_t;

color_t *cerror;	// color error.
color_t *ramp;	// color ramp
color_t *palette;
int palette_n = 8;
int ramp_n;	// how many colors in the ramp

color_t rgb (short r, short g, short b){
	color_t res;
	res.r = r;
	res.g = g;
	res.b = b;
	return res;
}

int col_dist (color_t a, color_t b){
	return (((int) a.r - b.r) * ((int) a.r - b.r) + ((int) a.g - b.g) * ((int) a.g - b.g) + ((int) a.b - b.b) * ((int) a.b - b.b));
}

void col_add_mul (color_t *res, color_t *a, float d){
	res->r += (short) (a->r * d);
	res->g += (short) (a->g * d);
	res->b += (short) (a->b * d);
}

void init (const char *ramp_fname){
	wd = get_width();
	ht = get_height();
	error = (short *)(calloc(wd * ht, sizeof(short)));
	ar = ((double) wd)/ht;
	sb = 2 * sa / ar;	// factor of two to account for stretching of terminal.
	la = ca - sa;
	lb = cb - sb;
	stepa = (2*sa)/wd;
	stepb = (2*sb)/ht;
	if (use_ramp){
		cerror = (color_t *)(calloc(wd * ht, sizeof(color_t)));
		palette = (color_t *)(malloc(8 * sizeof(color_t)));
		palette[0] = rgb(0,0,0);
		palette[1] = rgb(256,0,0);
		palette[2] = rgb(0,256,0);
		palette[3] = rgb(256,256,0);
		palette[4] = rgb(0,0,256);
		palette[5] = rgb(256,0,256);
		palette[6] = rgb(0,256,256);
		palette[7] = rgb(256,256,256);	// this numbering corresponds to the terminal codes.

		/* Read the ramp in from the file */
		FILE *rampfile = fopen(ramp_fname, "r");
		fscanf(rampfile, "%d\n", &ramp_n);
		ramp = (color_t *)(malloc(ramp_n * sizeof(color_t)));
		for (int i=0; i < ramp_n; i++){
			fscanf(rampfile, "%hd %hd %hd\n", &(ramp[i].r), &(ramp[i].g), &(ramp[i].b));
			printf("Found ramp color: %d, %d, %d\n", ramp[i].r, ramp[i].g, ramp[i].b);
		}
		fclose(rampfile);
	}
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

const double BAILOUT = 1e20;	// this is the value squared
const double BANDING = 10;
double LOGP = 0.6931471805;	// ln(2)

float mandelbrot (double a, double b){
	int ct = 0;
	double za = a;
	double zb = b;
	while (za*za + zb*zb < BAILOUT && ct < ITER_MAX){
		// z = z^2 + c
		double tmp = za*za - zb*zb;
		zb = 2*za*zb + b;
		za = tmp + a;
		ct++;
	}
	if (ct == ITER_MAX){
		return -1;
	}
	return (float) (ct - (log(log((za*za + zb*zb)/BANDING))/LOGP));
}

float multibrot (double a, double b){
	int ct = 0;
	double za = a;
	double zb = b;
	while (za * za + zb * zb < BAILOUT && ct < ITER_MAX){
		// z = z^n + c.
		int n = 1;
		double xa = za;
		double xb = zb;
		while (n < power){
			double tmp = za*xa - zb*xb;
			zb = za * xb + xa * zb;
			za = tmp;
			n++;
		}
		za += a;
		zb += b;
		ct ++;
	}
	if (ct == ITER_MAX) return -1;
	return (float) (ct - (log(log((za*za + zb*zb)/BANDING))/LOGP));
}

float burning_ship (double a, double b){
	int ct = 0;
	double za = a;
	double zb = -b;	// turn it upside down
	while (za * za + zb * zb < BAILOUT && ct < ITER_MAX){
		// z = |z^2| + c
		za = fabs(za);
		zb = fabs(zb);
		double tmp = za*za - zb*zb;
		zb = 2*za*zb + b;
		za = tmp + a;
		ct ++;
	}
	if (ct == ITER_MAX) return -1;
//	return ct;
	return (float) (ct - (log(log((za*za+zb*zb)/BANDING))/LOGP));
}

void set_fractal (int i){
	power = i;
	LOGP = log(i);
	if (i == 1){
		iters = burning_ship;
		power = 2;
		LOGP = log(2.0);
	} else if (i == 2){
		iters = mandelbrot;
	} else {
		iters = multibrot;
	}
}

double linear = 1;
double exponent = 0.75;
double offset = 0;
const int ncolors = 6;
const int colors[] = {YELLOW, RED, MAGENTA, BLUE, CYAN, GREEN};

float map (float iters){
	if (iters == -1) return -1;
	if (iters < 0) iters = 0;
	double d = pow(iters, exponent);
	d *= linear;
	while (offset < 0){
		offset += ncolors;
	}
	d += offset;
	return d;
}

const int 	novchars = 3;
const int 	npieces = 2 * novchars - 1;
const char 	ovchars[] = {' ', 'o','@'};

int mod (int a, int m){
	if (a >= 0) return a%m;
	return m + (a%m);
}

void draw_with_ramp (float iters, int x, int y){
	if (iters == -1){
		color (BG, BLACK);
		putchar(' ');
		return;
	}
	float mapped = map(iters);
	float d = mapped - (int) mapped;
	int idx = ((int) mapped) % ramp_n;
	int nidx = (idx + 1) % ramp_n;
	color_t col;
	col.r = (short) (ramp[idx].r * (1-d) + ramp[nidx].r * d) + cerror[y*wd+x].r;
	col.g = (short) (ramp[idx].g * (1-d) + ramp[nidx].g * d) + cerror[y*wd+x].g;
	col.b = (short) (ramp[idx].b * (1-d) + ramp[nidx].b * d) + cerror[y*wd+x].b;
//	printf("COL: %d, %d, %d\n", col.r, col.g, col.b);
	// col now holds the target color.
	
	int best_idx = 1;
	int badness = 0x5fffffff;
	for (int i=0; i < palette_n; i++){
		int bad = col_dist(col, palette[i]);
		if (bad < badness){
			badness = bad;
			best_idx = i;
		}
	}
	color_t err;
	err.r = col.r - palette[best_idx].r;
	err.g = col.g - palette[best_idx].g;
	err.b = col.b - palette[best_idx].b;
	// now we diffuse the error to the other pixels.
	if (y+1 < ht && x > 0 && x+1 < wd){
		col_add_mul(&cerror[y*wd + x + 1], 	&err, 7/16.0);
		col_add_mul(&cerror[(y+1)*wd + x-1], 	&err, 3/16.0);
		col_add_mul(&cerror[(y+1)*wd + x], 	&err, 5/16.0);
		col_add_mul(&cerror[(y+1)*wd + x+1], 	&err, 1/16.0);
	}
	color(BG, best_idx);
	putchar(' ');
}

/* Draws a pixel. This may involve dithering, setting the FG and BG colors, and printing spaces or potentially other characters. */
void draw_pixel(float iters, int x, int y){	
	if (iters == -1){
		color(BG, BLACK);
		putchar(' ');
		return;
	}
	if (use_ramp){
		draw_with_ramp (iters, x, y);
		return;
	}
	float mapped = map(iters);
	if (use_dither) mapped += error[y*wd + x]/8192.0f;
	short newerror = 0;
	if (use_overlays){
		int bg = (int) (mapped + 0.5f);
		float err = mapped - bg;
		int fg = (err > 0) ? (bg + 1) : (bg - 1);
		err += 0.5;	// now on [0..1]
		err *= npieces;	// now on [0..npieces]
		int ch = (int) err;
		ch -= (novchars - 1);
		float newerr= mapped - bg - (ch * 1.0f/npieces);	// ha!
		newerror = (short) (8192.0f * newerr / 16.0f);
		char todraw = ovchars[abs(ch)];

		color(FG, colors[mod(fg,ncolors)]);
		color(BG, colors[mod(bg,ncolors)]);
		putchar(todraw);

	} else {
		int idx = (int) (mapped + 0.5f);	
		newerror = (short) (8192.0f * (mapped - idx)/16.0f);
		idx %= ncolors;
		color(BG, colors[idx]);
		putchar(' ');
	}
	if (use_dither && y+1 < ht && x > 0 && x+1 < wd){
		/* propogate the error according to the Floyd-Steinberg dithering algorithm. The kernel is:
		 * 
		 * 1/16 * | 0 * 7 |
		 *        | 3 5 1 |	Where * is the current pixel.
		 */
		//		printf("X = %d Y = %d; newerror = %d\n", x, y, newerror);
		error[y*wd + x+1] += newerror * 7;
		error[(y+1)*wd + (x-1)] += newerror * 3;
		error[(y+1)*wd + x] += newerror * 5;
		error[(y+1)*wd + (x+1)] += newerror;
	}
}

void render (){
	if (use_ramp){
		memset(cerror, 0, wd*ht*sizeof(color_t));
	} else {
		if (use_dither){
			memset(error, 0, wd*ht*sizeof(short));	// clear the error matrix.
		}
	}
	for (int i=0; i<ht-1; i++){
		double b = lb + i*stepb;
		for (int j=0; j<wd; j++){
			double a = la + j*stepa;
			draw_pixel(iters(a,b), j, i);
		}
		putchar('\n');
	}
	color(BG, BLACK);
	color(FG, WHITE);
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
	printf("z          Toggle dithering");
	setpos(top+19, left+1);
	printf("x          Toggle overlays (characters printed on top)");
	setpos(top+21, left+1);
	printf("h          Show or hide this help");
	setpos(top+23, left+1);
	printf("q          Quit");
	fflush(stdout);
}


int main (int argc, const char *argv[]){
	if (argc > 1){
		use_ramp = true;
		init(argv[1]);
	} else {
		use_ramp = false;
		init("");
	}
	set_fractal(2);
//	printf("L = (%f, %f); S = (%f, %f); ar = %f; step = (%f, %f)\n", la, lb, sa, sb, ar, stepa, stepb);
	erase_all();
	render();
	keyboard_mode (KEY_INTERACTIVE);
	ikb_block(true);
	while (true){
		char inp = 2;	// meaningless
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
				use_dither = true;
			} else if (inp == 'z'){
				use_dither = !use_dither;
			} else if (inp == 'x'){
				use_overlays = !use_overlays;
			} else if (isdigit(inp)){
				set_fractal(inp - '0');
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
