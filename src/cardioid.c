#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <argp.h>
#include "SDL.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// Default values for options
#define DEFAULT_FPS 60
#define DEFAULT_MULTIPLIER 0
#define DEFAULT_MULTIPLIER_INCREASE 0.005
#define DEFAULT_DOTS 200

// SDL poll events
#define USER_QUIT_EVENT 0
#define USER_PAUSE_EVENT 1
#define USER_UNKNOWN_EVENT 2

const char *argp_program_version =
"cardioid v1.0.0\n"
"Copyright (C) 2024 Sivefunc\n"
"License GPLv3+: GNU GPL version 3 or later"
    "<https://gnu.org/licenses/gpl.html>\n"
"This is free software: you are free to change and redistribute it.\n"
"There is NO WARRANTY, to the extent permitted by law.\n"
"\n"
"Written by a human\n";

static struct argp_option options[] =
{
    {"help", 'h', 0, 0, "display this help and exit", 0},
    {"version", 'v', 0, 0, "output version information and exit", 1},
    {"dots", 'd', "NUM", 0, "spaced dots at the circle: "
        STR(DEFAULT_DOTS) " by default", 2},
    {"multiplier", 'm', "NUM", 0, "initial multiplier: "
        STR(DEFAULT_MULTIPLIER) " by default", 3},
    {"mult_increase", 'i', "NUM", 0, "increase per frame: "
        STR(DEFAULT_MULTIPLIER_INCREASE) " by default", 4},
    {"fps", 'f', "NUM", 0, "frames per second: "
        STR(DEFAULT_FPS) " by default", 5},
    {0}
};

typedef struct Dot
{
    int32_t x;
    int32_t y;
} Dot;

typedef struct Arguments
{
    int32_t fps;
    int32_t dots;
    double multiplier;
    double multiplier_increase;
} Arguments;

static error_t parse_opt(int32_t key, char *arg, struct argp_state *state);

void draw_cardioid(
        SDL_Window *window, SDL_Renderer *renderer,
        int32_t dots_quantity,
        double multiplier
        );

Dot *create_spaced_dots(
        int32_t dots_quantity, int32_t cx, int32_t cy, int32_t radius);

void create_circle(
        int32_t *radius, int32_t *cx, int32_t *cy,
        int32_t window_width, int32_t window_height);

uint8_t get_key(void);
int32_t main(int32_t argc, char *argv[])
{
    struct argp argp =
    {
        options,
        parse_opt,
        0,
        "Simon plouffe and Mathologer times table\n"
        "Generator of beautiful patterns like cardioid.\n\n"
        "press [SPACE] or [ENTER] to pause the frame.\v"
        "Written by Sivefunc",
        0,
        0,
        0
    };

    Arguments args =
    {
        .fps = DEFAULT_FPS,
        .multiplier = DEFAULT_MULTIPLIER,
        .multiplier_increase = DEFAULT_MULTIPLIER_INCREASE,
        .dots = DEFAULT_DOTS
    };

    if (argp_parse(&argp, argc, argv, ARGP_NO_HELP, 0, &args) == 0)
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            SDL_Log("SDL_Init failed (%s)", SDL_GetError());
            return 1;
        }

        SDL_Window *window = NULL;
        SDL_Renderer *renderer = NULL;
        if (SDL_CreateWindowAndRenderer(
                    640, 480, // width, height
                    SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_RESIZABLE,
                    &window, &renderer) < 0)
        {
            SDL_Log("SDL_CreateWindowAndRenderer failed (%s)", SDL_GetError());
            SDL_Quit();
            return 1;
        }

        const int32_t ms_per_frame = 1000 / args.fps;
        int32_t last_frame_time;
        int32_t delay;
        int32_t time_to_wait;
        uint8_t key;
        draw_cardioid(window, renderer,
                args.dots, args.multiplier);

        bool pause = false;
        bool running = true; 
        while (running)
        {
            last_frame_time = SDL_GetTicks();
            key = get_key();
            if (key == USER_QUIT_EVENT)
            {
                running = false;
            }
            else if (key == USER_PAUSE_EVENT)
            {
                pause = !(pause);
            }

            if (pause == false)
            {
                args.multiplier += args.multiplier_increase;
            }
            
            // It needs to be render even if it's paused due to window resizes.
            draw_cardioid(window, renderer, args.dots, args.multiplier);

            delay = SDL_GetTicks() - last_frame_time;
            time_to_wait = ms_per_frame - delay;
            if (time_to_wait > 0 && time_to_wait <= ms_per_frame)
                SDL_Delay(time_to_wait);
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    return EXIT_SUCCESS;
}

static error_t parse_opt(int32_t key, char *arg, struct argp_state *state)
{
    Arguments *args = state -> input;
    errno = 0;
    char *endptr = NULL;
    switch (key) 
    {
        case 'h':
            argp_state_help(state, state -> out_stream, ARGP_HELP_STD_HELP);
            break;

        case 'v':
            fprintf(state -> out_stream, "%s", argp_program_version);
            exit(EXIT_SUCCESS);
            break;
        
        case 'f':
            args -> fps = strtol(arg, &endptr, 10);
            if (errno != 0 || endptr == arg || *endptr != '\0')
            {
                fprintf(state -> out_stream, "Error in conversion of "
                        "arg: |%s|\n", arg);
                exit(EXIT_FAILURE);
            }
            else if (args -> fps <= 0)
            {
                fprintf(state -> out_stream,
                        "fps can't be <= 0\n");
                exit(EXIT_FAILURE);
            }
            break;

        case 'm':
            args -> multiplier = strtod(arg, &endptr);
            if (errno != 0 || endptr == arg || *endptr != '\0')
            {
                fprintf(state -> out_stream, "Error in conversion of "
                        "arg: |%s|\n", arg);
                exit(EXIT_FAILURE);
            }
            else if (args -> multiplier < 0)
            {
                fprintf(state -> out_stream,
                        "multiplier can't be negative\n");
                exit(EXIT_FAILURE);
            }
            break;

        case 'i':
            args -> multiplier_increase = strtod(arg, &endptr);
            if (errno != 0 || endptr == arg || *endptr != '\0')
            {
                fprintf(state -> out_stream, "Error in conversion of "
                        "arg: |%s|\n", arg);
                exit(EXIT_FAILURE);
            }
            else if (args -> multiplier_increase < 0)
            {
                fprintf(state -> out_stream,
                        "multiplier increase can't be negative\n");
                exit(EXIT_FAILURE);
            }
            break;

        case 'd':
            args -> dots = strtol(arg, &endptr, 10);
            if (errno != 0 || endptr == arg || *endptr != '\0')
            {
                fprintf(state -> out_stream, "Error in conversion of "
                        "arg: |%s|\n", arg);
                exit(EXIT_FAILURE);
            }
            else if (args -> dots < 0)
            {
                fprintf(state -> out_stream,
                        "dots quantity can't be negative\n");
                exit(EXIT_FAILURE);
            }
            break;

        case ARGP_KEY_ARG:
            fprintf(state -> out_stream,
                "program doesn't accept arguments without optionn\n");
            exit(EXIT_FAILURE);

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}
void create_circle(
        int32_t *radius, int32_t *cx, int32_t *cy,
        int32_t window_width, int32_t window_height)
{
    int32_t _radius = fmin(window_width, window_height) / 2;
    *cx = _radius;
    *cy = _radius;
    *radius = _radius;

    if (window_width > window_height)
    {
        *cx += (window_width - 2 * (_radius)) / 2;
    }  
    else if (window_height > window_width)
    {
        *cy += (window_height - 2 * (_radius)) / 2;
    }

}

/*      
 * Trigonometry found on wikipedia
 *              #
 *             /|
 *            / |
 *           /  | sin(angle)
 *          /   |
 * (cx, cy) #----
 *          cos(angle)
 *
 */
Dot *create_spaced_dots(
        int32_t dots_quantity, int32_t cx, int32_t cy, int32_t radius)
{
    Dot * dots = malloc(sizeof(Dot) * dots_quantity);
    double angle = (2.0 * M_PI) / dots_quantity;
    for (int32_t i = 0; i < dots_quantity; i++)
    {
         dots[i] = (Dot)
         {
            cx + radius * cos(angle * i),
            cy + radius * sin(angle * i)
         };
     }

    return dots;
}

void draw_cardioid(
        SDL_Window *window, SDL_Renderer *renderer,
        int32_t dots_quantity,
        double multiplier
        )
{
    SDL_Color bg_color = {.r = 0, .g = 0, .b = 0, .a = 255};
    int32_t window_width, window_height;
    int32_t radius, cx, cy; // Circle

    SDL_GetWindowSize(window, &window_width, &window_height);
    create_circle(&radius, &cx, &cy, window_width, window_height);
    Dot *dots = create_spaced_dots(dots_quantity, cx, cy, radius);

    SDL_SetRenderDrawColor(
        renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    for (int32_t i = 0; i < dots_quantity; i++)
    {
        SDL_RenderDrawPoint(renderer, dots[i].x, dots[i].y);
        SDL_RenderDrawLine(renderer,
                dots[i].x, dots[i].y,
                dots[(int32_t)(multiplier * i) % dots_quantity].x,
                dots[(int32_t)(multiplier * i) % dots_quantity].y);
    }

    SDL_RenderPresent(renderer);
    free(dots);
}

/*
 * Function: get_key
 * ----------------------
 * Check for the events close window and key presses using SDL_PollEvent.
 *
 * Parameters:
 * -----------
 *  none
 *
 * returns: uint8_t constant indicating action of event
 * (QUIT, PAUSE or UNKNOWN)
 *
 */
uint8_t get_key(void)
{
    SDL_Event event;
    uint8_t result = USER_UNKNOWN_EVENT;
    while (SDL_PollEvent(&event)) 
    {
        switch (event.type)
        {
            case SDL_QUIT:
                result = USER_QUIT_EVENT;
                break;

            case SDL_KEYDOWN:
                {
                    SDL_Keycode key_code = event.key.keysym.sym;
                    if (key_code == SDLK_ESCAPE || key_code == SDLK_q)
                    {
                        result = USER_QUIT_EVENT;
                    }
                    
                    else if (key_code == SDLK_SPACE ||
                            key_code == SDLK_KP_ENTER ||
                            key_code == SDLK_RETURN)
                    {
                        result = USER_PAUSE_EVENT;
                    }
                    break;
                }
            default: {}
        }
    }
    return result;
}
