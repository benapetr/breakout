#include <stdio.h>
#include <time.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>

#define clear() printf("\033[H\033[J")
#define gotoxy(x,y) printf("\033[%d;%dH", (y+1), (x+1))
#define MAX_SIZE   800
#define MAX_WIDTH  300
#define MAX_HEIGHT 300

struct Position
{
    int x;
    int y;
};

int pad_size = 15;
int score = 0;
int speed = 60;
char s_ball = '@';
char s_brick = 'X';
char s_pad = '=';
int screen_width = -30;
int screen_height = -30;
int direction_x;
int direction_y;
struct Position pad_position;
struct Position ball_position;
struct termios orig_term_attr;
struct termios new_term_attr;
int turbo = 0;
int paused = 0;
int quit = 0;
int is_idle = 0;
char arena[MAX_HEIGHT][MAX_WIDTH];

void move_ball();
void new_game();
void game_over();

int rand_lim(int limit)
{
    int divisor = RAND_MAX/(limit+1);
    int retval;

    do { 
        retval = rand() / divisor;
    } while (retval > limit);

    return retval;
}

int fetch_key()
{
    /* read a character from the stdin stream without blocking */
    /*   returns EOF (-1) if no character is available */
    return fgetc(stdin);
}

void fatal(char *reason)
{
    printf("FATAL: %s\n", reason);
    exit(1);
}

void draw_map()
{
    int x,y;
    x = 0;
    while (x-1 < screen_width)
    {
        y = 0;
        do {
            if (x == 0 || y == 0 || x >= screen_width-1 || y >= screen_height-1)
            {
                arena[x][y] = '*';
                gotoxy(x,y);
                printf("*");
            } else if (arena[x][y] == s_brick)
            {
                gotoxy(x,y);
                printf("%c", s_brick);
            }
        } while (y++ < screen_height-1);
       x++;
    }
}

void hud()
{
    gotoxy(0, screen_height + 1);
    printf("Score: %i | t - turbo | x - Quit the game | p - pause the game - https://github.com/benapetr/breakout   ", score);
}

int check_colision(int *x, int *y)
{
    *x = direction_x;
    *y = direction_y;
    int return_value = 0;
    if (ball_position.x + direction_x >= screen_width - 1)
    {
        *x = -1;
        return_value = 1;
    }
    if (ball_position.x + direction_x <= 0)
    {
        *x = 1;
        return_value = 1;
    }
    if (ball_position.y + direction_y >= screen_height - 1)
    {
        game_over();
        return 1;
    }
    if (ball_position.y + direction_y <= 0)
    {
        *y = 1;
        return_value = 1;
    }
    if (direction_y > 0 && (ball_position.y == pad_position.y - 1))
    {
        if (ball_position.x >= pad_position.x && ball_position.x <= pad_position.x + pad_size)
        {
            *y = -1;
            if (ball_position.x == pad_position.x || ball_position.x == pad_position.x + 1)
                *x = -1;
            else if (ball_position.x == pad_position.x + pad_size -1 || ball_position.x == pad_position.x + pad_size)
                *x = 1;
            return_value = 1;
        }
    }
    if (arena[ball_position.x + direction_x][ball_position.y + direction_y] == s_brick)
    {
        score++;
        arena[ball_position.x + direction_x][ball_position.y + direction_y] = ' ';
        gotoxy(ball_position.x + direction_x, ball_position.y + direction_y);
        printf(" ");
        if (arena[ball_position.x + direction_x * 2][ball_position.y] == s_brick)
            *x = direction_x * -1;
        if (arena[ball_position.x][ball_position.y + direction_y * 2] == s_brick)
            *y = direction_y * -1;
        return_value = 1;
        hud();
    }
    return return_value;
}

void game_over()
{
    gotoxy(screen_width / 2, screen_height / 2);
    printf("* GAME OVER *");
    gotoxy((screen_width / 2) - 10, (screen_height / 2) + 2);
    printf("Press x to exit or n for new game");
    int key = -1;
    while (key != 110 && key != 120)
    {
        key = fetch_key();
        usleep(200);
    }
    if (key == 120)
        quit = 2;
    else
        quit = 1;
}

void draw_pad()
{
    gotoxy(pad_position.x, pad_position.y);
    printf("<<===========>>");
    fflush(stdout);
}

void move_pad(int direction)
{
    if (direction < 0)
    {
        if (pad_position.x <= 1)
            return;
        pad_position.x--;
        gotoxy(pad_position.x, pad_position.y);
        printf("<<===========>> ");
    } else
    {
        if (pad_position.x + pad_size > screen_width - 2)
            return;
        pad_position.x++;
        gotoxy(pad_position.x - 1, pad_position.y);
        printf(" <<===========>>");
    }
    if (is_idle)
    {
        gotoxy(ball_position.x, ball_position.y);
        printf(" ");
        ball_position.x += direction;
        gotoxy(ball_position.x, ball_position.y);
        printf("%c", s_ball);
    }
}

void move_ball()
{
    // Clear the existing ball
    gotoxy(ball_position.x, ball_position.y);
    printf(" ");
    if (!is_idle)
    {
        int nx, ny;
        if (check_colision(&nx, &ny))
        {
            direction_x = nx;
            direction_y = ny;
        }
        ball_position.x += direction_x;
        ball_position.y += direction_y;
    }
    gotoxy(ball_position.x, ball_position.y);
    printf("%c", s_ball);
}

void play()
{
    int overlap = 0;
    while(!quit)
    {
        if (!turbo)
            usleep(speed * 1000);
        else
            usleep(speed * 100);
        int key = fetch_key();
        while (key != -1)
        {
            switch (key)
            {
                case 116:
                    turbo = !turbo;
                    break;
                case 32:
                    is_idle = 0;
                    break;
                case 67:
                    move_pad(1);
                    break;
                case 68:
                    move_pad(-1);
                    break;
                case 112:
                    if (paused)
                        paused = 0;
                    else
                        paused = 1;
                    break;
                case 120:
                    quit = 2;
                    break;
            }
            key = fetch_key();
        }
        if (quit)
            return;
        if (paused)
            continue;
        if (overlap)
        {
            overlap = 0;
            move_ball();
        } else
        {
            overlap = 1;
        }
    }
}

void params(int argc, char **argv)
{
}

void new_game()
{
    int ax = 0;
    int ay = 0;
    do
    {
        ay = 0;
        do
        {
            if ((ax > 6 && ax < screen_width - 6) && (ay > 4 && ay < screen_height / 2))
            {
                arena[ax][ay] = s_brick;
            } else
            {
                arena[ax][ay] = ' ';
            }
        } while (++ay < MAX_WIDTH);
    } while (++ax < MAX_HEIGHT);
    direction_x = 1;
    direction_y = -1;
    score = 0;
    clear();
    pad_position.x = screen_width / 2;
    pad_position.y = screen_height - 2;
    ball_position.x = screen_width / 2 + 3;
    ball_position.y = screen_height - 3;
    draw_map();
    hud();
    draw_pad();
}

int main(int argc, char **argv)
{
    params(argc, argv);
    struct winsize w;
    srand(time(NULL));
    /* set the terminal to raw mode */
    tcgetattr(fileno(stdin), &orig_term_attr);
    memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
    new_term_attr.c_lflag &= ~(ECHO|ICANON);
    new_term_attr.c_cc[VTIME] = 0;
    new_term_attr.c_cc[VMIN] = 0;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if (screen_width < 0 || screen_height < 0)
    {
        screen_height = w.ws_row - 1;
        screen_width = w.ws_col;
    }
    if (screen_width >= MAX_WIDTH)
        screen_width = MAX_WIDTH - 1;
    if (screen_height >= MAX_HEIGHT)
        screen_height = MAX_HEIGHT - 1; 
    tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);
    fputs("\e[?25l", stdout);
    while (quit != 2)
    {
        new_game();
        quit = 0;
        play();
    }
    /* restore the original terminal attributes */
    clear();
    tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);
    fputs("\e[?25h", stdout);
    return 0;
}
