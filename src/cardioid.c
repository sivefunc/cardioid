// Libraries needed, here's a quick summary:
#include <stdbool.h>    // bool, true and false macros.
#include <stdlib.h>     // malloc, free and others.
#include <stdint.h>     // intN_t and uintN_t.
#include <errno.h>      // global error variable "errno" and error macros.
#include <math.h>       // fmin, fmax, M_PI, cos and sin.
#include <argp.h>       // parsing of arguments on command line.
#include "SDL.h"        // graphics.

// Concatenate string with number, e.g "Pi is: " STR(3.14159)
// on preprocessing.
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// Default argument values for the options on command line.
#define DEFAULT_FPS 60
#define DEFAULT_MULTIPLIER 0
#define DEFAULT_MULTIPLIER_INCREASE 0.005
#define DEFAULT_DOTS 200
#define DEFAULT_LIGHTNESS 0.5
#define DEFAULT_SATURATION 1.0
#define DEFAULT_RAINBOW false
// End of default values for options

// SDL poll events
#define USER_QUIT_EVENT 0
#define USER_PAUSE_EVENT 1
#define USER_UNKNOWN_EVENT 2
// End of SDL poll events

// General message containing prog_name, author, license and year.
const char *argp_program_version =
"cardioid v2.0.0\n"
"Copyright (C) 2024 Sivefunc\n"
"License GPLv3+: GNU GPL version 3 or later"
    "<https://gnu.org/licenses/gpl.html>\n"
"This is free software: you are free to change and redistribute it.\n"
"There is NO WARRANTY, to the extent permitted by law.\n"
"\n"
"Written by a human\n";

// CLI options.
static struct argp_option options[] =
{
    // Quick summary
    {
        "help",                         // Long name of option
        'h',                            // Short name of option
        0,                              // Name of the arg, e.g NUM.
        0,                              // Flags
        "display this help and exit",   // Doc. of option useful for help.
        -1                              // Order of appearance in help
                                        // -1 means last, pos. value get first.
    },

    {"version", 'v', 0, 0, "output version information and exit", -1},
    {"dots", 'd', "NUM", 0,
        "spaced dots at the circle: " STR(DEFAULT_DOTS) " by default", 2},

    {"multiplier", 'm', "NUM", 0,
        "initial multiplier: " STR(DEFAULT_MULTIPLIER) " by default", 3},

    {"mult_increase", 'i', "NUM", 0,
        "increase per frame: " STR(DEFAULT_MULTIPLIER_INCREASE) " by default",
        4},

    {"fps", 'f', "NUM", 0,
        "frames per second: " STR(DEFAULT_FPS) " by default", 5},


    {"lightness", 'l', "NUM", 0,
        "lightness (HSL) [0, 1], higher is brigther: " STR(DEFAULT_LIGHTNESS)
            " by default", 6},

    {"saturation", 's', "NUM", 0,
        "saturation (HSL) [0, 1], higher greater fidelity to color: "
            STR(DEFAULT_SATURATION) " by default", 7},

    {"rainbow", 'r', 0, 0,
        "color it using rainbow colors instead of using single color ", 8},
   
    {0}
};

// type containing location on screen of the given pixel
// useful to create Dot array instead of array[N][2]; or two separate arrays.
typedef struct Dot
{
    int32_t x;
    int32_t y;
} Dot;

// type containing the arguments given to options
// like fps=240.
typedef struct Arguments
{
    int32_t fps;
    int32_t dots;
    double multiplier;
    double multiplier_increase;
    double lightness;
    double saturation;
    bool rainbow;
} Arguments;

// Prototypes
// parse arguments given to options or alone.
static error_t parse_opt(int32_t key, char *arg, struct argp_state *state);

// split circle into (dots_quantity) equally spaced parts.
// --------> using trigonometry (sin and cos).
// start going from dot 1 to final dot (dots_quantity) and connect with a line
// dot at position (i) to dot at (i * multiplier) % dots_quantity position.
//
// note: dots will be clockwise because SDL doesn't have coordinate plane
// so it start at top left corner (0,0) and upto bottom right corner.
// only positive coords.
//
// (0, 0)
//       ----------------x +inf
//       |
//       |
//       |
//       |
//       |
//       y
//       +inf
//
void draw_cardioid(SDL_Window *window, SDL_Renderer *renderer,
        int32_t dots_quantity,
        double multiplier,
        double lightness,
        double saturation,
        bool rainbow
        );

// array of spaced dots on the circle (clockwise)
Dot *create_spaced_dots(int32_t dots_quantity,
        int32_t cx, int32_t cy, int32_t radius);

// according to window size create the biggest circle that doesn't exit from
// the boundaries.
void create_circle(int32_t *radius, int32_t *cx, int32_t *cy,
        int32_t window_width, int32_t window_height);

// https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB
void hsl_to_rgb(double hue, double saturation, double lightness,
        int *r, int *g, int *b);

// Poll events from keyboard using SDL.
uint8_t get_key(void);

/////////////////
// Entry point //
/////////////////
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
        .dots = DEFAULT_DOTS,
        .lightness = DEFAULT_LIGHTNESS,
        .saturation = DEFAULT_SATURATION,
        .rainbow = DEFAULT_RAINBOW
    };

    // Succesfull parsing
    if (argp_parse(&argp, argc, argv, ARGP_NO_HELP, 0, &args) == 0)
    {
        // Unsuccessfull creation of video.
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            SDL_Log("SDL_Init failed (%s)", SDL_GetError());
            return 1;
        }

        SDL_Window *window = NULL;
        SDL_Renderer *renderer = NULL;
        
        // Unsuccessfull creation of window and renderer.
        if (SDL_CreateWindowAndRenderer(
                    640, 480, // width, height
                    SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_RESIZABLE,
                    &window, &renderer) < 0)
        {
            SDL_Log("SDL_CreateWindowAndRenderer failed (%s)", SDL_GetError());
            SDL_Quit();
            return 1;
        }

        // FPS calculation (on miliseconds)
        const int32_t ms_per_frame = 1000 / args.fps;
        int32_t last_frame_time;    
        int32_t delay;
        int32_t time_to_wait;
        uint8_t key;
        bool pause = false;
        bool running = true; 
        draw_cardioid(window, renderer, args.dots, args.multiplier,
                args.lightness, args.saturation, args.rainbow);

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
            draw_cardioid(window, renderer, args.dots, args.multiplier,
                args.lightness, args.saturation, args.rainbow);

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

// Draw the clockwise cardioid with spaced dots.
void draw_cardioid(
        SDL_Window *window, SDL_Renderer *renderer,
        int32_t dots_quantity,
        double multiplier,
        double lightness,
        double saturation,
        bool rainbow
        )
{
    int32_t window_width, window_height;
    int32_t radius, cx, cy; // Circle

    SDL_GetWindowSize(window, &window_width, &window_height);
    create_circle(&radius, &cx, &cy, window_width, window_height);
    Dot *dots = create_spaced_dots(dots_quantity, cx, cy, radius);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);


    // Color of the line according to dot at position i.
    int r, g, b;
    const double hue = rainbow ? 360.0 / dots_quantity : 120;
    if (rainbow == false)
    {
        // 120 is green (0, 255, 0) rgb tuple.
        hsl_to_rgb(120, saturation, lightness, &r, &g, &b);
    }
    
    // Connect dots through lines and colorize it.
    for (int32_t i = 0; i < dots_quantity; i++)
    {
        if (rainbow)
        {
            hsl_to_rgb(hue * i, saturation, lightness, &r, &g, &b);
        }

        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderDrawLine(renderer,
                dots[i].x, dots[i].y,
                dots[(int32_t)(multiplier * i) % dots_quantity].x,
                dots[(int32_t)(multiplier * i) % dots_quantity].y);
    }
    SDL_RenderPresent(renderer);
    free(dots);
}

// Create a centered circle of radius fmin(window_width, window_height) this
// maximizes the radius, while being inside the screen.
void create_circle(
        int32_t *radius, int32_t *cx, int32_t *cy,
        int32_t window_width, int32_t window_height)
{
    int32_t _radius = fmin(window_width, window_height) / 2;
    *cx = _radius;
    *cy = _radius;
    *radius = _radius;

    // Add padding to center the circle.
    // This is in case window_width > window_height, rotate the diagram
    // to see window_height > window_width.
    //
    //  _______width______________
    // |                          |
    // |__________________________|___.
    // | /  \ #  divide this by 2 |    |
    // ||    |#  and add it to    |    height
    // |\    /#  the beginning    |    |
    // |_\__/_#___________________|____|
    // |     |
    // |-----|
    //  height
    //  
    if (window_width > window_height)
    {
        *cx += (window_width - window_height) / 2;
    }  

    else if (window_height > window_width)
    {
        *cy += (window_height - window_width) / 2;
    }
    else
    {
        // Actually this is unnecesary, because if height == width then
        // width - height == 0, nothing get's added.
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
 * Normally this would be counter-strike 1.6 clockwise, but due to SDL
 * not having a coordinate plane it only has a positive direction
 * (if u want to show things on screen) so increasing y direction is actually
 * going down on the screen.
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

// https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB
// Recommendations for good rainbows
// saturation = 1
// lightness = 0.5
void hsl_to_rgb(double hue, double saturation, double lightness,
        int *r, int *g, int *b)
{
    double a = saturation * fmin(lightness, 1 - lightness);
    double k = (int)(0 + hue / 30.0) % 12;
    *r = 255 * (lightness - a * fmax(-1, fmin(fmin(k - 3, 9 - k), 1)));

    k = (int)(8 + hue / 30.0) % 12;
    *g = 255 * (lightness - a * fmax(-1, fmin(fmin(k - 3, 9 - k), 1)));

    k = (int)(4 + hue / 30.0) % 12;
    *b = 255 * (lightness - a * fmax(-1, fmin(fmin(k - 3, 9 - k), 1)));

    // Multiply r, g, b by 255 because these were on [0, 1] interval.
}

/*
 * Function: get_key
 * ----------------------
 * Check for the events close window and key presses using SDL_PollEvent.
 *
 * Parameters:
 * -----------
 *  none aka void.
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

// parses each argument, check if that argument was placed with a option
// identified by int32_t key.
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

        case 'l':
            args -> lightness = strtod(arg, &endptr);
            if (errno != 0 || endptr == arg || *endptr != '\0')
            {
                fprintf(state -> out_stream, "Error in conversion of "
                        "arg: |%s|\n", arg);
                exit(EXIT_FAILURE);
            }
            else if (args -> lightness < 0)
            {
                fprintf(state -> out_stream,
                        "lightness can't be negative\n");
                exit(EXIT_FAILURE);
            }
            break;

        case 's':
            args -> saturation = strtod(arg, &endptr);
            if (errno != 0 || endptr == arg || *endptr != '\0')
            {
                fprintf(state -> out_stream, "Error in conversion of "
                        "arg: |%s|\n", arg);
                exit(EXIT_FAILURE);
            }
            else if (args -> saturation < 0)
            {
                fprintf(state -> out_stream,
                        "saturation can't be negative\n");
                exit(EXIT_FAILURE);
            }
            break;

        case 'r':
            args -> rainbow = true;
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
