#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL.h>

#define TRUE 1
#define FALSE 0

typedef struct
{
	int running;
	int started;

	int WIDTH;
	int HEIGHT;
	int BPP;
	int TILE_ROWS;
	int TILE_COLS;
	int TILE_WIDTH;
	int TILE_HEIGHT;
	int SCRAM_NUM;

	SDL_Event event;
	SDL_Surface *picture;
	SDL_Surface *screen;

	int **position;
	int empty_row;
	int empty_col;

	Uint32 wspace;

	int tries;

	char trs[15];
} game_data;

void clean_up(game_data *gm);
void move(game_data *gm, Uint16 x, Uint16 y);
void set_title(game_data *gm);
void reflect(game_data *gm, int r, int c);
void clear_reflect(game_data *gm, int c);
void draw_tile(game_data *gm, int r, int c);
void draw_last(game_data *gm);
void draw_game(game_data *gm);
int check_win(game_data *gm);
int rand_int(int n);
void reset(game_data *gm);
void scramble(game_data *gm);
int init_game(game_data **gm, const char *img_name, int th, int tv);

/* Add animation. */

int main(int argc, char *argv[])
{
	int r, c;
	SDL_Surface *icon;
	game_data *gm;

	if(argc > 3)
	{
		r = atoi(argv[2]);
		c = atoi(argv[3]);
	}
	else
	{
		printf("Usage: puzzle image rows cols\n");
		return EXIT_FAILURE;
	}

	icon = NULL;
	gm = NULL;

	if(SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		printf("Couldn't initialize SDL: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	if(init_game(&gm, argv[1], r, c) == FALSE)
	{
		printf("Couldn't initialize the game.\n");
		clean_up(gm);
		return EXIT_FAILURE;
	}

	if((icon = SDL_LoadBMP("icon.bmp")) == NULL)
	{
		printf("Icon file not found. Running anyway.\n");
	}
	else
	{
		SDL_WM_SetIcon(icon, 0);
	}

	set_title(gm);
	SDL_Flip(gm->screen);
	gm->running = TRUE;

	while(gm->running == TRUE)
	{
		while(SDL_PollEvent(&(gm->event)))
		{
			if(gm->event.type == SDL_QUIT || (gm->event.type == SDL_KEYDOWN
			&& gm->event.key.keysym.sym == SDLK_ESCAPE))
			{
				gm->running = FALSE;
				break;
			}

			if(gm->event.type == SDL_KEYDOWN)
			{
				if(gm->event.key.keysym.sym == SDLK_s)
				{
					scramble(gm);
					set_title(gm);
					draw_game(gm);
					SDL_Flip(gm->screen);
				}
				else if(gm->started == TRUE &&
					gm->event.key.keysym.sym == SDLK_r)
				{
					reset(gm);
					set_title(gm);
					draw_game(gm);
					draw_last(gm);
					SDL_Flip(gm->screen);
					gm->started = FALSE;
				}
			}

			if(gm->started == TRUE &&
				gm->event.type == SDL_MOUSEBUTTONDOWN)
			{
				move(gm, gm->event.button.x, gm->event.button.y);

				if(check_win(gm) == TRUE && gm->started == TRUE)
				{
					draw_last(gm);
					SDL_Flip(gm->screen);
					sprintf(gm->trs, "Won: %d", gm->tries);
					SDL_WM_SetCaption(gm->trs, gm->trs);
					gm->started = FALSE;
				}
			}
		}

		SDL_Delay(34);
	}

	clean_up(gm);

	return EXIT_SUCCESS;
}

void clean_up(game_data *gm)
{
	if(gm != NULL)
	{
		int i;
		/* Get rid of position. */
		if(gm->position != NULL)
		{
			for(i = 0; i < gm->TILE_ROWS; i++)
				if(gm->position[i] != NULL)
					free(gm->position[i]);
			free(gm->position);
		}
		/* Get rid of picture. */
		if(gm->picture != NULL)
			SDL_FreeSurface(gm->picture);
		/* Get rid of screen. */
		if(gm->screen != NULL)
			SDL_FreeSurface(gm->screen);
		/* Quit SDL. */
		SDL_Quit();
		/* Get rid of gm. */
		free(gm);
	}
}

void move(game_data *gm, Uint16 x, Uint16 y)
{
	int re_r, re_c;

	re_r = y / gm->TILE_HEIGHT;
	re_c = x / gm->TILE_WIDTH;

	if(re_c < gm->TILE_COLS && re_r < gm->TILE_ROWS &&
	gm->position[re_r][re_c] != gm->TILE_ROWS * gm->TILE_COLS - 1)
	{
		if(re_r + 1 < gm->TILE_ROWS &&
		gm->position[re_r + 1][re_c] ==
		gm->TILE_ROWS * gm->TILE_COLS - 1)
		{
			gm->position[re_r + 1][re_c] = gm->position[re_r][re_c];
			gm->position[re_r][re_c] =
				gm->TILE_ROWS * gm->TILE_COLS - 1;
			draw_tile(gm, re_r, re_c);
			draw_tile(gm, re_r + 1, re_c);
			gm->empty_row++;
			goto set_all;
		}
		else if(re_r - 1 >= 0 &&
		gm->position[re_r - 1][re_c] ==
		gm->TILE_ROWS * gm->TILE_COLS - 1)
		{
			gm->position[re_r - 1][re_c] = gm->position[re_r][re_c];
			gm->position[re_r][re_c] =
				gm->TILE_ROWS * gm->TILE_COLS - 1;
			draw_tile(gm, re_r, re_c);
			draw_tile(gm, re_r - 1, re_c);
			gm->empty_row--;
			goto set_all;
		}
		else if(re_c + 1 < gm->TILE_COLS &&
		gm->position[re_r][re_c + 1] ==
		gm->TILE_ROWS * gm->TILE_COLS - 1)
		{
			gm->position[re_r][re_c + 1] = gm->position[re_r][re_c];
			gm->position[re_r][re_c] =
				gm->TILE_ROWS * gm->TILE_COLS - 1;
			draw_tile(gm, re_r, re_c);
			draw_tile(gm, re_r, re_c + 1);
			gm->empty_col++;
			goto set_all;
		}
		else if(re_c - 1 >= 0 &&
		gm->position[re_r][re_c - 1] ==
		gm->TILE_ROWS * gm->TILE_COLS - 1)
		{
			gm->position[re_r][re_c - 1] = gm->position[re_r][re_c];
			gm->position[re_r][re_c] =
				gm->TILE_ROWS * gm->TILE_COLS - 1;
			draw_tile(gm, re_r, re_c);
			draw_tile(gm, re_r, re_c - 1);
			gm->empty_col--;
			goto set_all;
		}
	}

	return;

	set_all:
		if(gm->started == TRUE)
			gm->tries++;
		set_title(gm);
		SDL_Flip(gm->screen);
}

void set_title(game_data *gm)
{
	sprintf(gm->trs, "Tries: %d", gm->tries);
	SDL_WM_SetCaption(gm->trs, gm->trs);
}

void draw_last(game_data *gm)
{
	SDL_Rect rct_pic, rct_scr;

	rct_scr.x = rct_pic.x = (gm->TILE_COLS - 1) * gm->TILE_WIDTH;
	rct_scr.y = rct_pic.y = (gm->TILE_ROWS - 1) * gm->TILE_HEIGHT;
	rct_scr.w = rct_pic.w = gm->TILE_WIDTH;
	rct_scr.h = rct_pic.h = gm->TILE_HEIGHT;

	SDL_BlitSurface(gm->picture, &rct_pic, gm->screen, &rct_scr);
	reflect(gm, gm->TILE_ROWS - 1, gm->TILE_COLS - 1);
}

void reflect(game_data *gm, int r, int c)
{
	SDL_Surface *opp;
	SDL_Surface *rf;
	Uint32 a;
	Uint8 *p;
	Uint8 rc;
	Uint8 g;
	Uint8 b;
	int i, j;
	SDL_Rect rct_pic, rct_rfl, rct_screen;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rf = SDL_CreateRGBSurface(SDL_SWSURFACE, gm->TILE_WIDTH,
		gm->TILE_HEIGHT, gm->BPP, 0xff000000, 0x00ff0000, 0x0000ff00,
		0x000000ff);
	opp = SDL_CreateRGBSurface(SDL_SWSURFACE, gm->TILE_WIDTH,
		gm->TILE_HEIGHT, gm->BPP, 0xff000000, 0x00ff0000, 0x0000ff00,
		0x000000ff);

#else
	rf = SDL_CreateRGBSurface(SDL_SWSURFACE, gm->TILE_WIDTH,
		gm->TILE_HEIGHT, gm->BPP, 0x000000ff, 0x0000ff00, 0x00ff0000,
		0xff000000);
	opp = SDL_CreateRGBSurface(SDL_SWSURFACE, gm->TILE_WIDTH,
		gm->TILE_HEIGHT, gm->BPP, 0x000000ff, 0x0000ff00, 0x00ff0000,
		0xff000000);
#endif

	if(SDL_MUSTLOCK(opp))
		SDL_LockSurface(opp);

	if(SDL_MUSTLOCK(gm->picture))
		SDL_LockSurface(gm->picture);

	/* Create the reflection surface (opp) here. */
	for(i = 0; i < gm->TILE_HEIGHT; i++)
	{
		for(j = 0; j < gm->TILE_WIDTH; j++)
		{
			p = (Uint8 *)gm->picture->pixels +
			(i + gm->position[r][c] / gm->TILE_COLS * gm->TILE_HEIGHT)
			* gm->picture->pitch
			+ (j + gm->position[r][c] % gm->TILE_COLS * gm->TILE_WIDTH)
			* 4;

			SDL_GetRGB(*(Uint32 *)p, gm->picture->format, &rc, &g, &b);

			p = (Uint8 *)opp->pixels + (gm->TILE_HEIGHT - i - 1) *
			opp->pitch + j * 4;
			*(Uint32 *)p = SDL_MapRGB(opp->format, rc, g, b);
		}
	}

	if(SDL_MUSTLOCK(gm->picture))
		SDL_UnlockSurface(gm->picture);

	if(SDL_MUSTLOCK(opp))
		SDL_UnlockSurface(opp);

	if(SDL_MUSTLOCK(rf))
		SDL_LockSurface(rf);

	/* Create the transparent overlay surface (rf) here. */
	for(i = 0; i < gm->TILE_HEIGHT; i++)
	{
		a = SDL_MapRGBA(rf->format, 0xd2, 0xd2, 0xd2,
			128 + (i + 1) * 0xff / (gm->TILE_HEIGHT * 2));
		for(j = 0; j < gm->TILE_WIDTH; j++)
		{
			p = (Uint8 *)rf->pixels + i * rf->pitch + j * 4;
			*(Uint32 *)p = a;
		}
	}

	if(SDL_MUSTLOCK(rf))
		SDL_UnlockSurface(rf);

	SDL_BlitSurface(rf, NULL, opp, NULL);

	rct_rfl.x = c * gm->WIDTH;
	rct_rfl.y = 0;

	rct_pic.x = gm->position[r][c] % gm->TILE_COLS * gm->TILE_WIDTH;
	rct_pic.y = gm->position[r][c] / gm->TILE_COLS * gm->TILE_HEIGHT;

	rct_screen.x = c * gm->TILE_WIDTH;
	rct_screen.y = gm->HEIGHT;

	rct_rfl.w = rct_screen.w = rct_pic.w = gm->TILE_WIDTH;
	rct_rfl.h = rct_screen.h = rct_pic.h = gm->TILE_HEIGHT;

	SDL_BlitSurface(opp, NULL, gm->screen, &rct_screen);

	/* We are done with the previous surfaces, so we free them. */
	SDL_FreeSurface(opp);
	SDL_FreeSurface(rf);
}

void clear_reflect(game_data *gm, int c)
{
	SDL_Rect rc;

	rc.x = gm->TILE_WIDTH * c;
	rc.y = gm->HEIGHT;
	rc.w = gm->TILE_WIDTH;
	rc.h = gm->TILE_HEIGHT;

	SDL_FillRect(gm->screen, &rc, gm->wspace);
}

void draw_tile(game_data *gm, int r, int c)
{

	if(gm->position[r][c] == gm->TILE_ROWS * gm->TILE_COLS - 1)
	{
		SDL_Rect blank;

		blank.x = c * gm->TILE_WIDTH;
		blank.y = r * gm->TILE_HEIGHT;
		blank.w = gm->TILE_WIDTH;
		blank.h = gm->TILE_HEIGHT;

		SDL_FillRect(gm->screen, &blank, gm->wspace);
	}
	else
	{
		SDL_Rect rct_screen, rct_pic;

		rct_pic.x = gm->position[r][c] % gm->TILE_COLS * gm->TILE_WIDTH;
		rct_pic.y = gm->position[r][c] / gm->TILE_COLS * gm->TILE_HEIGHT;

		rct_screen.x = c * gm->TILE_WIDTH;
		rct_screen.y = r * gm->TILE_HEIGHT;

		rct_screen.w = rct_pic.w = gm->TILE_WIDTH;
		rct_screen.h = rct_pic.h = gm->TILE_HEIGHT;

		SDL_BlitSurface(gm->picture, &rct_pic,
		gm->screen, &rct_screen);
	}

	if(r == gm->TILE_ROWS - 1)
	{
		if(gm->position[r][c] != gm->TILE_ROWS * gm->TILE_COLS - 1)
			reflect(gm, r, c);
		else
			clear_reflect(gm, c);
	}
}

void draw_game(game_data *gm)
{
	int i, j;

	for(i = 0; i < gm->TILE_ROWS; i++)
		for(j = 0; j < gm->TILE_COLS; j++)
			draw_tile(gm, i, j);
}

int check_win(game_data *gm)
{
	int i, j, k;

	k = 0;
	for(i = 0; i < gm->TILE_ROWS; i++)
		for(j = 0; j < gm->TILE_COLS; j++)
			if(gm->position[i][j] != k)
				return FALSE;
			else
				k++;

	return TRUE;
}

int rand_int(int n)
{
	static int init = 0;

	if(init == 0)
	{
		srand(time(NULL));
		init = 1;
	}

	return rand() % n;
}

void reset(game_data *gm)
{
	int i, j, k;

	k = 0;
	for(i = 0; i < gm->TILE_ROWS; i++)
		for(j = 0; j < gm->TILE_COLS; j++)
			gm->position[i][j] = k++;

	gm->tries = 0;
	gm->started = FALSE;
}

void scramble(game_data *gm)
{
	int i, j, k, tmp;

	k = 0;
	for(i = 0; i < gm->TILE_ROWS; i++)
		for(j = 0; j < gm->TILE_COLS; j++)
			gm->position[i][j] = k++;

	gm->empty_row = gm->TILE_ROWS - 1;
	gm->empty_col = gm->TILE_COLS - 1;

	for(i = 0; i < 2 * gm->SCRAM_NUM; i++)
	{
		switch(rand_int(4))
		{
			case 0:
				if(gm->empty_row - 1 >= 0)
				{
					gm->empty_row -= 1;
					tmp = gm->position[gm->empty_row][gm->empty_col];
					gm->position[gm->empty_row][gm->empty_col] =
						gm->TILE_ROWS * gm->TILE_COLS - 1;
					gm->position[gm->empty_row + 1][gm->empty_col] = tmp;
				}
				break;
			case 1:
				if(gm->empty_col + 1 < gm->TILE_COLS)
				{
					gm->empty_col += 1;
					tmp = gm->position[gm->empty_row][gm->empty_col];
					gm->position[gm->empty_row][gm->empty_col] =
						gm->TILE_ROWS * gm->TILE_COLS - 1;
					gm->position[gm->empty_row][gm->empty_col - 1] = tmp;
				}
				break;
			case 2:
				if(gm->empty_row + 1 < gm->TILE_ROWS)
				{
					gm->empty_row += 1;
					tmp = gm->position[gm->empty_row][gm->empty_col];
					gm->position[gm->empty_row][gm->empty_col] =
						gm->TILE_ROWS * gm->TILE_COLS - 1;
					gm->position[gm->empty_row - 1][gm->empty_col] = tmp;
				}
				break;
			case 3:
				if(gm->empty_col - 1 >= 0)
				{
					gm->empty_col -= 1;
					tmp = gm->position[gm->empty_row][gm->empty_col];
					gm->position[gm->empty_row][gm->empty_col] =
						gm->TILE_ROWS * gm->TILE_COLS - 1;
					gm->position[gm->empty_row][gm->empty_col + 1] = tmp;
				}
				break;
			default:
				break;
		}
	}

	gm->tries = 0;
	gm->started = TRUE;
}

int init_game(game_data **gm, const char *img_name, int tr, int tc)
{
	SDL_Surface *tmp = NULL;
	int i;

	if((*gm = (game_data *)malloc(sizeof(game_data))) == NULL)
	{
		printf("Couldn't allocate enough memory for the game.\n");
		return FALSE;
	}

	(*gm)->picture = NULL;
	(*gm)->screen = NULL;
	(*gm)->position = NULL;

	if((tmp = SDL_LoadBMP(img_name)) == NULL)
	{
		printf("Couldn't load image: %s\n", SDL_GetError());
		return FALSE;
	}

	(*gm)->picture = tmp;

	(*gm)->WIDTH = (*gm)->picture->w;
	(*gm)->HEIGHT = (*gm)->picture->h;
	(*gm)->BPP = 32;
	(*gm)->TILE_ROWS = tr;
	(*gm)->TILE_COLS = tc;
	(*gm)->TILE_WIDTH = (*gm)->WIDTH / (*gm)->TILE_COLS;
	(*gm)->TILE_HEIGHT = (*gm)->HEIGHT / (*gm)->TILE_ROWS;
	(*gm)->SCRAM_NUM =
		(*gm)->TILE_ROWS > (*gm)->TILE_COLS
		? (*gm)->TILE_ROWS * (*gm)->TILE_ROWS * (*gm)->TILE_COLS
		: (*gm)->TILE_COLS * (*gm)->TILE_COLS * (*gm)->TILE_ROWS;
	(*gm)->SCRAM_NUM += 100;

	if((*gm)->WIDTH % (*gm)->TILE_COLS != 0)
	{
		printf("The picture cannot be divided evenly horizontally.\n");
		return FALSE;
	}

	if((*gm)->HEIGHT % (*gm)->TILE_ROWS != 0)
	{
		printf("The picture cannot be divided evenly vertically.\n");
		return FALSE;
	}

	/* Allocate memory for the tiles representing the pieces. */
	if(((*gm)->position =
		(int **)malloc(sizeof(int *) * (*gm)->TILE_ROWS)) == NULL)
	{
		printf("Couldn't allocate enough memory.\n");
		return FALSE;
	}

	for(i = 0; i < (*gm)->TILE_ROWS; i++)
	{
		if(((*gm)->position[i] = (int *)malloc(sizeof(int) *
		(*gm)->TILE_COLS)) == NULL)
		{
			printf("Couldn't allocate enough memory.\n");
			return FALSE;
		}
	}

	if(((*gm)->screen = SDL_SetVideoMode((*gm)->WIDTH, (*gm)->HEIGHT +
	(*gm)->TILE_HEIGHT,	(*gm)->BPP, SDL_SWSURFACE | SDL_SRCALPHA)) == NULL)
	{
		printf("Couldn't set SDL video mode: %s\n", SDL_GetError());
		return FALSE;
	}

	(*gm)->picture = SDL_DisplayFormat(tmp);
	SDL_FreeSurface(tmp);

	(*gm)->wspace = SDL_MapRGB((*gm)->screen->format, 0xff, 0xff, 0xff);

	scramble(*gm);
	draw_game(*gm);

	(*gm)->tries = 0;

	return TRUE;
}
