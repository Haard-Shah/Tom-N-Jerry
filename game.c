#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <cab202_graphics.h>
#include <cab202_timers.h>

// Define constants
#define MAX 100
#define WALL_Char '*'
#define width (screen_width() - 1) //Usable gamplay width, componsating for starting at 0
#define height (screen_height() - 5) //Usable gameplay height (excluding the stats and top border)
#define MAX_cheeses 5
#define MAX_traps 5
#define MAX_weapons 5
#define M_PI 3.14159265358979323846
#define MAX_HEALTH 5
#define TIMER 2000
#define CHEESE_IMG 'C'
#define TRAP_IMG 'M'
#define FIREWORK_IMG '#'
#define MAX_FIREWORKS 50
#define WEAPON_SPEED 0.5

// Define functions
#define len(x) (int)( sizeof(x) / sizeof((x)[0]) )

// Define colours
#define COLOR_BLACK	0
#define COLOR_RED	1
#define COLOR_GREEN	2
#define COLOR_YELLOW 3
#define COLOR_BLUE	4
#define COLOR_MAGENTA 5
#define COLOR_CYAN	6
#define COLOR_WHITE	7

// Define Game object's 
struct player {
    char character;
    double x;
    double y;
    double dx;
    double dy;
    int lives; //TODO: Assigne the lives to the exact player. Store the active player in the game_state. And using the activePlayer's value we update the corresponding player's attributes. ASK how to do this.
    double reset_x;
    double reset_y;
};

struct wall {
    int x1;
    int y1;
    int x2;
    int y2;
};

struct game{
    int chesee;
    int weapons;
    int traps;
    bool paused;
    int cheeseEaten;
    //int fireworksHit; //TODO: REMOVE not being used
    double start_time;
    timer_id cheeseTimer;
    timer_id trapTimer;
    timer_id fireworkTimer;
    double pause_time;
    double unpause_time;
    double lvl_start_time;
    bool gameOver;
    bool resetLvl;
    int finalScore;
    bool lvlUp;
    char activePlayer;
    int lives_at_swtich;
};

struct object{
    int x;
    int y;
    bool visible;
};

struct weapon{
    double x;
    double y;
    bool visible;
}; //TODO: combine weapon with object struct as they are same currently using weapon due to truncation issues 

struct time{
    int min;
    int sec;
};

// Global variables
struct wall Walls[MAX];
struct player Hero = { 'J', 0, 0, 0, 0, MAX_HEALTH};
struct player Chaser = { 'T', 1, 1, 0, 0, INT32_MAX}; 
//TODO: why can't  set the int values to NULL in the initialisation of the struct
struct game game_state;
struct game stored_state;
struct object cheeses[MAX_cheeses];
struct object traps[MAX_traps];
struct weapon fireworks[MAX_FIREWORKS];
struct object door;
struct time gameTime;
int wallc = 0;
timer_id cheeseTimerTemp;
timer_id trapTimerTemp;
int Num_rooms;
int lvl;
char ActivePlayer = 'J'; //Default player Jerry
bool switched = false; 

// ------------------------------------ SUPPORTING FUNCTIONS ----------------------------------------------
// /* Returns the total score of the current game state.  */
// int score()
// {
//     return (game_state.cheeseEaten + game_state.fireworksHit);
// } //TODO: REMOVE not being used

/*Reset's the game to level (room) 1.*/
void reset_game()
{
    lvl = 1; //Reset to first room
    clear_screen();
    if(game_state.gameOver) game_state.gameOver = false; // Begin play //TODO: CHECK: need to check the logic as it currently will not reset the game
}

/* Wraps the x and y values to values within game boundary  */
void map_values(double *x, double *y)
{
    //map the x axis
    if (*x < 0 )*x = 0;
    else if (*x > width) *x = width;

    //map the y axis
    if (*y < 4) *y = 4;
    else if (*y > (4 + height)) *y = (4 + height);
}

// ---------------------------------- READ FUNCTIONS ----------------------------------------------


/*read_wall() reads in the parsed variables as a wall of type struct wall. It managed dynamic scaling of wall based on current screen width.*/
void read_wall(double X1, double Y1, double X2, double Y2, struct wall *wall)
{
    wall->x1 = round(X1 * width);
    wall->y1 = round(Y1 * height) + 4; // +4 is the offset for status bar
    wall->x2 = round(X2 * width);
    wall->y2 = round(Y2 * height) + 4;
}

/*Calculated the step size dx and dy for the chaser.*/
void initialise_chaser_movement(struct player *chaser)
{
    double direction = rand() * M_PI * 2 / RAND_MAX;
    float speed = 0.15 + ((rand() % 100) / 100) * 0.8; //Mapping the speed to range of 0.15 to 0.8 pixels per step

    chaser->dx = speed * cos(direction);
    chaser->dy = speed * sin(direction);
}

/*read_character() allocates character's starting position dynamically mapped to current screen. */
void read_character(double x1, double y1, struct player *player)
{
    player->x = x1 * width;
    player->y = 4 + (y1 * height);

    if(ActivePlayer == 'J')
    {
        if (player->character == 'J')
        {
            player->x = round(player->x); // Ensure the doubles are rounded for an even cordinated system
            player->y = round(player->y);
        }
        else if (player->character == 'T') initialise_chaser_movement(player);
    }

    //Store each player's reset_x and reset_y location 
    player->reset_x = player->x;
    player->reset_y = player->y;
}

/* read_Map() function takes in a stream of File pointer and read's all its values, until EOF. read_Map() initialises the characters, walls, chesee, traps and Weapons. It return a bool as a form of quick validation check. */
bool read_Map(FILE * stream)
{
    char command;
    double arg1, arg2, arg3, arg4;
    wallc = 0; //reset wall counter

    while(!feof(stream))
    {
        int items_scanned = fscanf(stream, "%c %lf %lf %lf %lf", &command, &arg1, &arg2, &arg3, &arg4);

        if (items_scanned == 3)
        {
            if(command == 'J') { read_character(arg1, arg2, &Hero); }
            else if (command == 'T') { read_character(arg1, arg2, &Chaser); }
            else {
                fprintf(stderr, "Error: Invalid Character defnition");
                return false;
            }
        }
        else if (items_scanned == 5 )
        {
            if (command == 'W'){
                read_wall(arg1, arg2, arg3, arg4, &Walls[wallc]);
                wallc++;
            }
        }
        else if (command != '\n')
        {
            fprintf(stderr, "Error: Invalid Command found in the file");
            return false;
        }
    } // End While

    return true;
}

// ------------------------- SETUP FUNCTIONS -------------------------------------------

/*
    Check if the supplied coordiantes are a valid location to move on to. 
    Returns true if and only if its is a valid location.
*/
bool isValidLocation2(int next_x, int next_y ) //char validLocationDeliminator //TODO: updated this so more than one non-valid location can be deliminated using a char[] argv and argc style 
{
    char character = scrape_char(next_x, next_y);

    if (character == '*') return false; // So there is wall at next coordinate location
    return true;
}

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
THIS IS THE MATHEMATICAL WAYS OF CHECKING FOR WALLS BUT CONTAINS A SMALL BUG. DUE TO TIME CONSTRAINTS I HAVE RESORTED TO isValidLocation2() UNTIL ALL THE BUGS IN isValidLocation() CAN BE DEBUGGED

//Checks if a number is between provided two values (inlcusive). Returns true if its is else false.
bool between(int y, int y1, int y2)
{
    // y1 <= y <= y2 OR   y2 <= y <= y1    check for both upward slope and downward slope
    bool isBetween = ( y >= y1 && y <= y2) || (y<= y1 && y >=y2);
    draw_formatted(1, (width * 0.8), "%d  %d  %d  %d", y, y1, y2, isBetween);
    show_screen();
    return isBetween;


    //return (( y >= y1 && y <= y2) || (y<= y1 && y >=y2)); //FIXME: Check this logic
}



//checks if the location of the placement is valid. i.e. not on walls.
bool isValidLocation(int next_x, int next_y)
{
    for(int i = 0; i < wallc; i++)
    {
        // ONLY CHECK FOR HORIZONTAL WALLS CURRENTLY  //FIXME: Check the logic below

        if (abs(Walls[i].x2 - Walls[i].x1) != 0) // True means there is no change in x and hence is a horizontal line
        {
            if (next_y == Walls[i].y1) // Making sure we are somewhere on the horizontal line
            {
                draw_char(next_x, next_y, '!');
               if(between(next_y, Walls[i].y1, Walls[i].y2)) return false; // Finally checking if the next step is witin the domains of the horizontal wall
            }
        }
        else if (abs(Walls[i].y2 - Walls[i].y1) != 0) // True would mean x is constant all across and hence a vertical line wall
        {
            if (next_x == Walls[i].x1) // Making sure we are somewhere on the vertical line
            {
                if(between(next_y, Walls[i].y1, Walls[i].y2)) return false; // Finally checking if the next step is within the domains of the vertical wall
            }
        }
    }

    return true;
} 
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/



/* Defines the corrdinates of the cheese respawn location. */
void setup_cheese(int cheese_index)
{
    // TODO: FIX: there is a bug that allow it to be placed on a wall. Debug it
     do
        {
            cheeses[cheese_index].x = (rand() % width);
            cheeses[cheese_index].y = 4 + (rand() % height);
        } while (!isValidLocation2(cheeses[cheese_index].x, cheeses[cheese_index].y)); 
}

/* Defines the coordinates of the trap respawn location. Checks if the spaw location is valid */
void setup_trap(int trap_index)
{
    int x = (int)Chaser.x;
    if(x < 0) x = 0;
    else if (x > width) x = width;

    int y = (int)Chaser.y;
    if (y <  4) y = 4;
    else if (y > (4 + height)) y = (4 + height);

    int counter = 0;

    do
    {
        //Check if it is a valid location
        if (isValidLocation2(x, y))
        {
            traps[trap_index].x = x;
            traps[trap_index].y = y;
            break;
        }
        else
        {
            (counter%2 ? x++ : y++); //TODO: update so if the x > width then x-- else x++ and if y > 4 + height then y-- else y++
            counter++;
        }
    } while (true);
}

/* Updates current trajectery of the weapon to remain locked on to Chaser. Firework moves in the recalcuated trajectory to target Chaser. */
void move_weapon(double current_x, double current_y, int weapon_index)
{
    //(NOTE: wrt == with respect to)

    //Get a unit vector of Tom's displacement wrt current location 
    double theta;
    // Set a gaurd statement so if dx does == 0 then it is set to infinity which will equate to near 0 when divided
    if(abs(Chaser.x - current_x) == 0)
    {
        // Weapon is perpendicular to Tom so (dy/dx) == NaN as (dx == 0) therefore setting the theta manually to avoid segmentation fault
        if(Chaser.y <= current_y) theta = M_PI / 2;  // 90 degrees Tom is perpendicularly above the weapon
        else theta = M_PI * (3/2); // 270 degrees Tom is perpendiculary below the weapon
    }
    else
    {
        double dy = abs(Chaser.y - current_y);
        double dx = abs(Chaser.x - current_x);
        theta = atan( dy / dx); //absolute unit vector. In this case the magnitute of the unit vector is just 1 as it moves 1 pixel per step so absolute unit vector should be fine for x and y calculations
    } 

    //Find out where Tom is in regards to current location and fire weapon accordingly    
    // dx = speed * cos(theta);  ->  dx = (1) * cos(theta)   ->   dx = cos(theta)  
    // SO, x = current_x + dx -> x = current_x + cos(theta)
     if (Chaser.x < current_x ) fireworks[weapon_index].x = current_x - (WEAPON_SPEED * (width/height) * cos(theta)); // So Tom is to the left of current location 
     else fireworks[weapon_index].x = current_x + (WEAPON_SPEED * (width/height ) * cos(theta)); // Tom is to the right of current location
    
    // SIMILARLY dy = speed * sin(theta);  ->  dy = (1) * sin(theta)   ->   dy = sin(theta)      
    // SO, y = current_y + dx -> y = current_y + sin(theta)
     if (Chaser.y < current_y) fireworks[weapon_index].y = current_y - (WEAPON_SPEED * sin(theta)); // Tom is above current location
     else fireworks[weapon_index].y = current_y + (WEAPON_SPEED *  sin(theta)); // Tom is below current location
}

void setup_firework(int firework_index)
{    
    move_weapon(Hero.x, Hero.y, firework_index); // Initialise the weapon based on the Hero's location it travels away from Hero
}
void fire()
{
    //setup the firework 
    for (int i = 0; i < MAX_FIREWORKS; i++)
    {
        if(!fireworks[i].visible)
        {
            //Fire one weapon at a time 
            setup_firework(i);
            game_state.weapons++;
            fireworks[i].visible = true; // Make it visible on the screen
            break;
        }
    }    
}

/* Reset's Hers's position back to the original position. */
void reset_Hero()
{
    //Reset Jerry
    Hero.x = Hero.reset_x;
    Hero.y = Hero.reset_y;
}

/* Reset's chaser back to the original position and sets a new random velocity */
void reset_chaser()
{
    //Reset Tom
    Chaser.x = Chaser.reset_x;
    Chaser.y = Chaser.reset_y;
    initialise_chaser_movement(&Chaser); //Set new velocity
}

/*Respawns the chaser at a new random location.*/
void reset_players()
{
    reset_Hero();
    reset_chaser();    
}

/*Sets up door's location at a valid door location.*/
void setup_door()
{
    do
    {
        door.x = round(rand() % width);
        door.y = 4 + (rand() % height);
    } while (!isValidLocation2(door.x, door.y));

    door.visible = true;
}

/*Initialises the game global variables, default character and game start time.*/
void initalise_game_state()
{
    if (game_state.resetLvl || lvl == 1) //Reset below values if resetting to level 1
    {   
        game_state.finalScore = 0;
        game_state.start_time = get_current_time();
        game_state.resetLvl = false;
        Hero.lives = MAX_HEALTH;
        Chaser.lives = MAX_HEALTH;
        game_state.lvl_start_time = get_current_time();
    }
    game_state.cheeseEaten = 0;
    //game_state.fireworksHit = 0; //TODO: REMOVE not being used
    game_state.traps = 0; 
    game_state.chesee = 0;
    game_state.weapons = 0;
    game_state.paused = false;
    game_state.gameOver = false;
    
    //TODO: break it down into initialise_timers()
    if(ActivePlayer == 'J')
    {
        game_state.cheeseTimer = create_timer(TIMER);
        game_state.trapTimer = create_timer(TIMER);
    }
    else game_state.fireworkTimer = create_timer(2.5 * TIMER);
    
    game_state.pause_time = 0;
    door.visible = false;
    game_state.resetLvl = false;
    game_state.activePlayer = ActivePlayer;

    //TODO: BREAK it down into initialise_game_objects()
    //initialise cheeses and make them invisible  AND make the traps inviisible 
    for(int i = 0; i < 5; i++) 
    {
        setup_cheese(i);
        cheeses[i].visible = false;
        traps[i].visible = false;
    }

    for (int i = 0; i < MAX_FIREWORKS; i++)
    {
        fireworks[i].visible = false; //Make all the fireworks invisible at the start
    }
}

/* setup() initialises the game based on the map file provided. It also defines game's state. */
void setup (FILE * stream)
{
    clear_screen();

    initalise_game_state();

    bool read_successful = read_Map(stream);// Read the walls, Jerry and Tom loactions
    srand(get_current_time()); // Initilise a random seed

    if (!read_successful)
    {
        fprintf(stderr, "ERROR: Invalid format in Map File.");
        game_state.gameOver = true;
    }
} // End setup()

/*Returns the lives of the active player.*/
int lives()
{
    return (ActivePlayer == 'J' ? Hero.lives : Chaser.lives);
}


void update_time()
{
    if(!game_state.paused)
    {
        double currentTime = get_current_time();
        double difference;
        if((game_state.unpause_time - game_state.pause_time) > 0)
        {
            double offset = (game_state.unpause_time - game_state.pause_time);
            game_state.start_time += offset;
            game_state.pause_time = 0;
            game_state.unpause_time = 0;
        }
        difference = (currentTime - game_state.start_time);
        // else difference = (currentTime - game_state.start_time); //Difference is in seconds

        gameTime.min = (int)difference/60; // cast and truncate to get the minutes part 
        gameTime.sec = (int)round(difference - (gameTime.min * 60)); //Convert the remaining decimal minutes to seconds
        //TODO: floor the sec to get it to go upto 59 but not 60
    }    
}

// ------------------------- DRAW FUNCTIONS -----------------------------------------

/*draw_game_stats() draws all the game stats in the game header area. List inludes: Score, Lives, Player, Time, Cheese, Traps, Fireworks and Level.*/
void draw_game_stats()
{
    draw_formatted(0, 0, "Student Number: N10235779");
    draw_formatted(round(width * 0.38), 0, "Score: %3d", game_state.finalScore);
    draw_formatted(round(width * 0.55), 0, "Lives: %d", lives());
    draw_formatted(round(width * 0.7), 0, "Player: %c", ActivePlayer);
    update_time();
    draw_formatted(round(width * 0.85), 0, "Time: %02d:%02d", gameTime.min, gameTime.sec);

    draw_formatted(0, 2, "Cheese: %d", game_state.chesee);
    draw_formatted(round(width * 0.25), 2, "Traps: %d", game_state.traps);
    draw_formatted(round(width * 0.45), 2, "Fireworks: %d", game_state.weapons);
    draw_formatted(round(width * 0.65), 2, "Level: %d", lvl);
    draw_formatted(round(width * 0.8), 2, "x: %d", (int)round(Hero.x)); //TODO: REMOVE
    draw_formatted(round(width * 0.9), 2, "y: %lf", Chaser.dy); //TODO: REMOVE

    draw_line(0, 3, width, 3, '-');
}

/*draw_walls() draw wallc number of walls on the screen using the Walls[] of type struct Wall. */
void draw_walls()
{
    for (int i = 0; i < wallc; i++) { draw_line(Walls[i].x1, Walls[i].y1, Walls[i].x2, Walls[i].y2, WALL_Char); }
}

/*draw_players() draw's players on the the screen play.*/
void draw_players()
{
    draw_char(round(Hero.x), round(Hero.y), Hero.character);
    draw_char(round(Chaser.x), round(Chaser.y), Chaser.character);
}

/*draw_cheese() draw cheese on screen at random(x,y) coordinates. It draw at max only 5 cheeses on screen at rate of 1 cheese per 2 seconds.*/
void draw_cheese()
{
    set_colours(COLOR_YELLOW, COLOR_BLACK); //Set color forground color to yellow
    for(int i = 0; i < len(cheeses); i++)
    {
        if(cheeses[i].visible) draw_char(cheeses[i].x, cheeses[i].y, CHEESE_IMG); //TODO: updated this to perform the test
    }
    set_colours(COLOR_WHITE, COLOR_BLACK); // reset colors to default
}

/*Draw traps on screen at Tom's previous locations. It draw at max only 5 traps on screen at rate of 1 trap per 2 seconds.*/
void draw_traps()
{
    set_colours(COLOR_CYAN, COLOR_BLACK); //Set color forground color to cyan
    for(int i = 0; i < len(traps); i++)
    {
        if(traps[i].visible) draw_char(traps[i].x, traps[i].y, TRAP_IMG);
    }
    set_colours(COLOR_YELLOW, COLOR_BLACK); // reset colors to default
}

//TODO: combine all of above functions into one
// /*  */
// void draw_objects(struct object objects[][])
// {
//     for (int i = 0; i < len(objects[i]); i++)
//     {
//         /* code */
//     }

//     /*
//     pesudocode 
//     for array in objects:
//         for element in array: 
//             draw(element) 
//     */

    
// }

void draw_fireworks()
{
    // set_colours(COLOR_RED, COLOR_YELLOW);
    set_colours(COLOR_YELLOW, COLOR_RED);
    for (int i = 0; i < MAX_FIREWORKS; i++)
    {
        if(fireworks[i].visible) draw_char(round(fireworks[i].x), round(fireworks[i].y), FIREWORK_IMG);
    }
    set_colours(COLOR_WHITE, COLOR_BLACK);
}

/*Draws the door of it's property is set to visible.*/
void draw_door()
{
    set_colours(COLOR_GREEN, COLOR_BLUE);
    if(door.visible) draw_char(door.x, door.y, 'X');
    set_colours(COLOR_WHITE, COLOR_BLACK);
}

/*draw_all() handles the drawing of all entities of the game.*/
void draw_all()
{
    clear_screen();
    if(game_state.lvlUp)
    {
        show_screen(); //TODO: REMOVE 
        game_state.lvlUp = false;
    }

    draw_game_stats();
    draw_walls();
    draw_players();
    draw_cheese();
    draw_traps();
    draw_fireworks();
    draw_door();

    show_screen();
}

//---------------------------------------------------------------------------------------------

// ----------------------------- COLLISION FUNCTIONS ------------------------------------------
/*Retruns true if the two coordiantes provided do collide.*/
bool collided(int x1, int y1, int x2, int y2)
{
   return  ((x1 == x2 && y1 == y2) ? true : false);
}

// /*Simple linear equaltion function. Returns the y value given a greadient (m), y-intercent (c) and the input x value is provided.*/
// int h(int m, int c, int x)
// {
//     double result = (m*x) + c;
//     return round(result);
// } //TODO: REMOVE: check if funciton is needed?

// ------------------------ UPDATE FUNCTIONS --------------------------------------------------

/*Displays the game over message and waits for a key input to exit the game.*/
void game_over(bool *exitToTerminal)
{
    if (game_state.lvlUp)
    {
        clear_screen();
        game_state.gameOver = false;
        return; //level up to the next level in current state
    }
    if(lvl < Num_rooms && Hero.lives > 0)
    {
        lvl++;
        clear_screen();
        game_state.gameOver = false;
        return; // Proceed to the next room
    }

    clear_screen();

    char *message[] = {
        (Hero.lives <= 0? "Game Over!" : "YOU WIN!"),
        "You Lost all Your lives.", 
        "Press 'r' to restart the game or Press 'q' to exit..."
    };    

    for (int i = 0; i < len(message); i++) {
        // Draw message in middle of screen.
        int len = strlen(message[i]);
        int x = (screen_width() - len) / 2;
        int y = (screen_height() - len(message)) / 2 + i;
        if(i == 1) set_foreground(COLOR_RED);
        draw_formatted(x, y, message[i]);
        if(i == 1) set_foreground(COLOR_WHITE);
    }

    if (Hero.lives > 0 )
    {
        draw_line(0, (screen_height() - len(message)) / 2 + 1, width, (screen_height() - len(message)) / 2 + 1, ' ');
        set_foreground(COLOR_GREEN);
        draw_formatted((screen_width() - 9) / 2, (screen_height() - len(message)) / 2 + 1, "Score: %2d", game_state.finalScore);
        set_foreground(COLOR_WHITE);
    }
    show_screen();

    while (true)
    {
        char key_code = get_char();
        if (key_code == 'r')
        {
            reset_game();
            break;
        }
        else if (key_code == 'q')
        {
            // Exit to the terminal gracefully
            cleanup_screen();
            *exitToTerminal = true;
            break;
        }
    } 
}

/* update_her0() updates the hero's position based on the keyboard input. Keryboard input: a => Left,  d => Right,  w => Up,  s => Down.*/
void update_hero(int key_code) //TODO: update update_hero() to incorporate the changes based upon the game_state.currerntPlayer value. Instead of Just sticking to the Jerry character.
{   
    if ((key_code == 'a' && ((int)Hero.x) > 0) && \
        ( isValidLocation2((int)round(Hero.x - 1), (int)round(Hero.y))) )
        {
            Hero.x--;
        }  // Left

    else if ((key_code == 'd' && ((int)Hero.x) < width) && \
            ( isValidLocation2((int)round(Hero.x + 1), (int)round(Hero.y)))) { Hero.x++; } // Right

    else if ((key_code == 'w' && ((int)Hero.y) > 4) && \
            ( isValidLocation2((int)round(Hero.x), round(Hero.y - 1))) ) { Hero.y--; } // Up

    else if ((key_code == 's' && ((int)Hero.y) < (height + 4)) && \
            ( isValidLocation2((int)round(Hero.x), (int)round(Hero.y + 1))) ) { Hero.y++; } // Down
}

/* Performs the randomised movement of Chaser. If chaser collides with walls or bondaries of the game a new randomised velocity is generated to continue moving. */
void randomised_movement()
{
    bool bounced = false;
    // Predict the next step of the chaser
    int next_x = round(Chaser.x + Chaser.dx);
    int next_y = round(Chaser.y + Chaser.dy);

    //Check for collisions
    if(next_x == 0 || next_x == width || !(isValidLocation2(next_x, next_y)) ) //If on the left or the right border  or colliding with a vertical wall switch direction horizontally
    {
        initialise_chaser_movement( &Chaser );  
        bounced = true;
    }
    if (next_y == 3 || next_y == (height + 5) || !(isValidLocation2(next_x, next_y))) //if on the top or bottom border or colliding with a horizontal wall switch direction vertically
    {
        initialise_chaser_movement( &Chaser );
        bounced = true;
    }

    if(!bounced)
    {
        Chaser.x += Chaser.dx;
        Chaser.y += Chaser.dy;
    }
}

/* Moves chaser intelligently to chase Hero rather than just moving randomly in hope of one day catching the Hero. */
void intelligent_movement()
{
    //Get a unit vector of Tom's displacement with respect to Jerry
    double theta;

    if(abs(Hero.x - Chaser.x) == 0)
    {
        // Weapon is perpendicular to Tom so (dy/dx) == NaN as (dx == 0) therefore setting the theta manually to avoid segmentation fault
        if(Hero.y <= Chaser.y) theta = M_PI / 2;  // 90 degrees Jerry is perpendicularly above Tom
        else theta = M_PI * (3/2); // 270 degrees Jerry is perpendiculary below Tom
    }
    else
    {
        double dy = abs(Hero.y - Chaser.y);
        double dx = abs(Hero.x - Chaser.x);
        theta = atan( dy / dx); //angle of the unit vector. 
    } 

    //float speed = 0.3;
    
    //Speed will grow expotentially as time passes in the level
    float speed = (1/6000) * exp(sqrt(get_current_time() - game_state.lvl_start_time)) - 1; // magnitute of the unit vector

    Chaser.dy = speed;

    // dx = speed * cos(theta);  to avoid constantly writing and reading the data I have choosen to just use the speed*cos(theta) definition of dx and dy. for efficiency.
     if (Hero.x < Chaser.x ) Chaser.x -= speed * cos(theta); // So Jerry is to the left of current location 
     else Chaser.x += speed * cos(theta); // Jerry is to the right of current location

     if (Hero.y < Chaser.y) Chaser.y -=  speed * sin(theta); // Jerry is above current location
     else Chaser.y += speed *  sin(theta); // Jerry is below current location

     //NEXT
        // check for wall collisions and bondary collisions
            // store prevous step values and using a threshold see if last two values are exactly the same if yes than move in direction of greater change so if dx>dy than than a step in dx and vice verse 
            // if both are distances are equal than randomly chose a direction and take a step in that direction 
}

/*Moves the chaser to the next VALID location. */ //TODO: UPDATE the description
void move_chaser()
{
    if (lvl < 2)
    {
        randomised_movement();
    }
    else intelligent_movement();
}
/* Updates the traps of the game according the game rules. */
void update_traps(int key)
{
     //Ebable next trap if the timer has expired and there are less than 5 traps on screen and only if playing as Jerry and game is not in pause mode 
    if(timer_expired(game_state.trapTimer) && game_state.traps < 5 && !game_state.paused && ActivePlayer == 'J') //Drop traps every 2 seconds
    {
        // Find the next disable trap and enable it
        for (int i = 0; i < len(traps); i++)
        {
            if(!traps[i].visible) 
            {
                setup_trap(i);
                traps[i].visible = true;
                game_state.traps++;
                timer_reset(game_state.trapTimer); // Reset the timer
                break;
            }
        }
    } 

    // Check for trap collisions between Hero and Traps
    for(int i = 0; i < len(traps); i++)
    {
        if (traps[i].visible && collided(round(Hero.x), round(Hero.y), traps[i].x, traps[i].y))
        {
            Hero.lives--;
            game_state.traps--;
            traps[i].visible = false;
            break;
        }
    }

    if(Hero.lives <= 0) 
    {
        clear_screen();
        game_state.gameOver = true; // Finally check Hero's health 
    }
}

/*Handles the movement of the chaser player and automatic trap deployments.*/
void update_chaser(int key_code)
{
    if (key_code < 0 && !game_state.paused)
    {
        move_chaser();
        // update_traps();
    }

    if (collided(Hero.x, Hero.y, Chaser.x, Chaser.y))
        {
            Hero.lives--;

            if(Hero.lives <= 0)
            {
                clear_screen();
                game_state.gameOver = true;
            }
            else reset_players();
        }
}

/*update_cheese() checks if the player has captured the cheese (by colliding into it) and updates the score accordingly. It also respons new cheese upon successful collision.*/
void update_cheese()
{
    //Draw cheese if the timer has expired and there are less than 5 cheeses on screen
    if(timer_expired(game_state.cheeseTimer) && game_state.chesee < 5 && !game_state.paused) //Drop cheese every 2 seconds
    {
        for (int i = 0; i < MAX_cheeses; i++)
        {
            if(!cheeses[i].visible) 
            {
                cheeses[i].visible = true;
                game_state.chesee++;
                timer_reset(game_state.cheeseTimer); // Reset the timer
                break;
            }
        }
    }

    //Check if the Hero collides with any of the cheeses
    for(int i = 0; i < MAX_cheeses; i++)
    {
        if (cheeses[i].visible && collided(round(Hero.x), round(Hero.y), cheeses[i].x, cheeses[i].y))
        {
            game_state.cheeseEaten++;
            game_state.finalScore++;
            game_state.chesee--;
            setup_cheese(i); //reinitialise the cheese for next round
            cheeses[i].visible = false;
            break;
        }
    }
}

void update_fireworks(int key_code)
{
    for (int i = 0; i < MAX_FIREWORKS; i++)
    {
        if (fireworks[i].visible)
        {
            // check for weapon's collision with Tom
            if ((int)Chaser.x == (int)fireworks[i].x && (int)Chaser.y == (int)fireworks[i].y)
            {
                game_state.finalScore++;
                game_state.weapons--;
                fireworks[i].visible = false;
                reset_chaser();
                break; // No point checking others as Tom no longer is in the same posoition
            }
        }
    }
    
    //update all the visible firework's coordinates 
    for (int i = 0; i < MAX_FIREWORKS; i++)
    {
        if (fireworks[i].visible) 
        {
            move_weapon(fireworks[i].x, fireworks[i].y, i); // if they are on the screen than move them
            if ( !isValidLocation2(round(fireworks[i].x), round(fireworks[i].y)) || \
                (round(fireworks[i].x) < 0 || round(fireworks[i].x) > width ) || \
                (round(fireworks[i].y) < 4 || round(fireworks[i].y) > (4 + height)) )
            {
                fireworks[i].visible = false;
                game_state.weapons--;
            } 
        }
    }
}

/*Checks if the Jerry has eaten >=5 cheese then opens the door to next level.*/
void update_door()
{
    if(game_state.cheeseEaten > 0 && !door.visible) //FIXME: REMOVE: fixme tag 
    {
        setup_door();
    }
    else
    {
        if(collided((int)Hero.x, (int)Hero.y, door.x, door.y))
        {
            clear_screen();
            game_state.gameOver = true; // Let the game_over() function handle the next step accordinly
        }
    }
}

/* Drops a new cheese on the current location of Tom if there are less than 5 cheeses on screen. */
void drop_cheese()
{
    //if there are less than 5 cheese on screen 
        // Make the next cheese in the array visible 
        // Set the new cheese's coordinates to current Tom's coordinates (remeber to use round as x and y are int types and draw_player uses round to draw the charactes)

}

/* Drops a new trap on current location of Tom if there are less than 5 traps on screen.  */
void drop_trap()
{
    //if there are less than 5 traps on screen 
        // Make the next trap in the array visible 
        // Set the new trap's coordinates to current Tom's coordinates (remeber to use round as x and y are int types and draw_player uses round to draw the charactes)
}

void reset_walls()
{
    for (int i = 0; i < MAX; i++)
    {
        Walls[i].x1 = 0;
        Walls[i].y1 = 0;
        Walls[i].x2 = 0;
        Walls[i].y2 = 0;
    }
    wallc = 0;    
}

/* Switchs to the next level in the game until last room has been reached. */
void level_up()
{
    if (lvl < Num_rooms)
    {
        game_state.lvlUp = true;
        game_state.gameOver = true;
        reset_walls();
        lvl++;
    }   
}

void switch_players()
{
    //change the active player to Tom 
    ActivePlayer = (ActivePlayer == 'J' ? 'T' : 'J');

    if (!switched) // if switching first time 
    {
        //save the state of the game 
        stored_state = game_state;// Store current state of the game for Jerry 
        stored_state.lives_at_swtich = Hero.lives;
        initalise_game_state();// Initialised new game state
        switched = true;
    }
    else 
    {
        struct game Temp = game_state;
        game_state = stored_state;
        stored_state = Temp;

        if (game_state.activePlayer == 'J')
        {
            Hero.lives = game_state.lives_at_swtich;
            stored_state.lives_at_swtich = Chaser.lives;
        }
        else
        {
            Chaser.lives = game_state.lives_at_swtich;
            stored_state.lives_at_swtich = Hero.lives;
        } 
    } 
}

/*pause_game() changes the state of the game to pause mode. */
void pause_game()
{
    if(!game_state.paused)
    {
        game_state.paused = true;
        game_state.pause_time = get_current_time();
        cheeseTimerTemp = game_state.cheeseTimer; //Hold the cheese timer value at pause time for later timer resuming
        trapTimerTemp = game_state.trapTimer; // Hold the trap timer value at pause time for later timer resuming 
    }
    else
    {
        game_state.paused = false;
        game_state.unpause_time = get_current_time();
        game_state.cheeseTimer = cheeseTimerTemp; //Resume the cheese timer
        game_state.trapTimer = trapTimerTemp; // Resume the trap timer
    }
}

/* update_state() updates the game's state according to the keyboard input. It handles the player movement, chaser movement and game pause funcationaliy.*/
void update_state(int key_code)
{
    // Check Pause
    if (key_code == 'p') { pause_game(); }
    else if (key_code == 'r') 
    { 
        game_state.resetLvl = true;
        reset_game(); 
    }
    else if (key_code == 'f' && lvl > 1 && ActivePlayer == 'J') fire();
    else if (key_code == 'l' && Num_rooms > 1) level_up();
    else if (key_code == 'z') switch_players();
    else if (key_code == 'c' && ActivePlayer == 'T') drop_cheese();
    else if (key_code == 'm' && ActivePlayer == 'T') drop_trap();
    else
    {
        update_hero(key_code);
        update_chaser(key_code);
        if(!game_state.gameOver) update_traps(key_code);
        if(!game_state.gameOver) update_cheese(key_code);
        if(!game_state.gameOver) update_fireworks(key_code);
        update_door();
    }
}

void loop() {  //TODO: check if this is a redundant function with update_state()
    int key = get_char();
    update_state(key);

    show_screen();
} // End loop()


int main(int argc, char * args[]) {
    Num_rooms = argc - 1;

    bool exit = false;
    //Check if a map file has been provided
    if (argc > 1)
    {
        lvl = 1;
        while(!exit)
        {
            setup_screen();
            FILE * mapFile = fopen(args[lvl], "r");
            if(mapFile != NULL)
            {
                setup(mapFile); //Set the game based on mapFile
                while (!game_state.gameOver)
                {
                    draw_all();
                    loop();
                    timer_pause( 10 );
                }
                game_over(&exit);
            }
            else
            {
                fprintf(stderr, "ERROR: Provided map file is empty. Please check the map file.");
                exit = true;
            }
        }
    }
    else
    {
        fprintf(stderr, "ERROR: No Map File Provided.");
    }
    return 0;
}