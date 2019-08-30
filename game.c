#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <cab202_graphics.h>
#include <cab202_timers.h>

#define MAX 100
#define WALL_Char '*'
#define width (screen_width() - 1) //Usable gamplay width, componsating for starting at 0
#define height (screen_height() - 5) //Usable gameplay height (excluding the stats and top border)
#define MAX_cheeses 5
#define MAX_traps 5
#define MAX_weapons 5
#define M_PI 3.14159265358979323846
#define MAX_HEALTH 5


struct player {
    char character;
    double x;
    double y;
    double dx;
    double dy;
    int lives; //TODO: Assigne the lives to the exact player. Store the active player in the game_state. And using the activePlayer's value we update the corresponding player's attributes. ASK how to do this.
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
    int score;
    int lvl;
    char ActivePlayer;
    double start_time;
    timer_id cheeseTimer;
    bool gameOver;
};

struct object{
    int x;
    int y;
    bool visible;
};

struct time{
    int min;
    int sec;
};

// Global variables
struct wall Walls[MAX];
struct player Hero = { 'J', 0, 0, 0, 0, MAX_HEALTH};
struct player Chaser = { 'T', 1, 1, 0, 0, INT32_MAX}; //TODO: why can't  set the int values to NULL in the initialisation of the struct
struct game game_state;
struct object cheeses[MAX_cheeses];
struct object traps[MAX_traps];
struct time gameTime;
int wallc = 0;
int cheeseIndex;


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
    //int rand_max = RAND_MAX;
    double direction = rand() * M_PI * 2 / RAND_MAX;
    float speed = 0.15 + ((rand() % 100) / 100) * 0.8;

    chaser->dx = speed * cos(direction);
    chaser->dy = speed * sin(direction);
}

/*read_character() allocates character's starting position dynamically mapped to current screen. */
void read_character(double x1, double y1, struct player *player)
{
    player->x = x1 * width;
    player->y = 4 + (y1 * height);

    if(game_state.ActivePlayer == 'J')
    {
        if (player->character == 'J')
        {
            player->x = round(player->x); // Ensure the doubles are rounded for an even cordinated system
            player->y = round(player->y);
        }
        else if (player->character == 'T') initialise_chaser_movement(player);
    }

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

bool isValidLocation2(int next_x, int next_y)
{
    char character = scrape_char(next_x, next_y);

    if (character == '*') return false; // So there is wall at next coordinate location
    return true;
}

// THIS IS THE MATHEMATICAL WAYS OF CHECKING FOR WALLS BUT CONTAINS A SMALL BUG. DUE TO TIME CONSTRAINTS I HAVE RESORTED TO isValidLocation2() UNTIL ALL THE BUGS IN isValidLocation() CAN BE DEBUGGED
// /*Checks if a number is between provided two values (inlcusive). Returns true if its is else false.*/
// bool between(int y, int y1, int y2)
// {
//     // y1 <= y <= y2 OR   y2 <= y <= y1    check for both upward slope and downward slope
//     bool isBetween = ( y >= y1 && y <= y2) || (y<= y1 && y >=y2);
//     draw_formatted(1, (width * 0.8), "%d  %d  %d  %d", y, y1, y2, isBetween);
//     show_screen();
//     return isBetween;


//     //return (( y >= y1 && y <= y2) || (y<= y1 && y >=y2)); //FIXME: Check this logic
// }



// /*checks if the location of the placement is valid. i.e. not on walls.*/
// bool isValidLocation(int next_x, int next_y)
// {
//     for(int i = 0; i < wallc; i++)
//     {
//         // ONLY CHECK FOR HORIZONTAL WALLS CURRENTLY  //FIXME: Check the logic below

//         if (abs(Walls[i].x2 - Walls[i].x1) != 0) // True means there is no change in x and hence is a horizontal line
//         {
//             if (next_y == Walls[i].y1) // Making sure we are somewhere on the horizontal line
//             {
//                 draw_char(next_x, next_y, '!');
//                if(between(next_y, Walls[i].y1, Walls[i].y2)) return false; // Finally checking if the next step is witin the domains of the horizontal wall
//             }
//         }
//         else if (abs(Walls[i].y2 - Walls[i].y1) != 0) // True would mean x is constant all across and hence a vertical line wall
//         {
//             if (next_x == Walls[i].x1) // Making sure we are somewhere on the vertical line
//             {
//                 if(between(next_y, Walls[i].y1, Walls[i].y2)) return false; // Finally checking if the next step is within the domains of the vertical wall
//             }
//         }
//     }

//     return true;
// }

/* Defines the corrdinates of the cheese respawn location. */
void setup_cheese(int cheese_index)
{
     do
        {
            cheeses[cheese_index].x = (rand() % width);
            cheeses[cheese_index].y = 4 + (rand() % height);
        } while (!isValidLocation2(cheeses[cheese_index].x, cheeses[cheese_index].y));
}

/*Respawns the chaser at a new random location.*/
void setup_chaser()
{
    Chaser.x = (rand() % width);
    Chaser.y = 4 + (rand() % height);

    initialise_chaser_movement(&Chaser); //Set new velocity
}

/*Initialises the game global variables, default character and game start time.*/
void initalise_game_state()
{
    game_state.chesee = 0;
    game_state.score = 0;
    game_state.weapons = 0;
    game_state.traps = 0;
    game_state.paused = false;
    game_state.gameOver = false;
    game_state.lvl = 1;
    game_state.ActivePlayer = 'J'; //Default player Jerry
    game_state.start_time = get_current_time();
    game_state.cheeseTimer = create_timer(2000);

    for(int i = 0; i < 5; i++) 
    {
        setup_cheese(i);
        cheeses[i].visible = false;
    }
    cheeseIndex = 0; //TODO: REMOVE if not used
}

/* setup() initialises the game based on the map file provided. It also defines game's state. */
void setup (FILE * stream)
{
    clear_screen();

    initalise_game_state();

    bool read_successful = read_Map(stream);// Read the walls, Jerry and Tom loactions
    srand(get_current_time()); // Initilise a random seed

    //Setup cheeses, Traps and Weapons
    //setup_cheese(); //TODO: REMOVE not used currently

    if (!read_successful) // Initilise the game state
    {
        fprintf(stderr, "ERROR: Invalid format in Map File.");
    }
} // End setup()

/*Returns the lives of the active player.*/
int lives()
{
    return (game_state.ActivePlayer == 'J' ? Hero.lives : Chaser.lives);
}


void update_time()
{
    double currentTime = get_current_time();

    double difference = (currentTime - game_state.start_time); //Difference is in seconds

    gameTime.min = (int)difference/60;
    gameTime.sec = (int)round(difference - (gameTime.min * 60)); //Convert the remaining decimal minutes to seconds
}

// ----------------------------------------------------------------------------------

// ------------------------- DRAW FUNCTIONS -----------------------------------------

/*draw_game_stats() draws all the game stats in the game header area. List inludes: Score, Lives, Player, Time, Cheese, Traps, Fireworks and Level.*/
void draw_game_stats()
{
    draw_formatted(0, 0, "Student Number: N10235779");
    draw_formatted(round(width * 0.38), 0, "Score: %3d", game_state.score);
    draw_formatted(round(width * 0.55), 0, "Lives: %d", lives());
    draw_formatted(round(width * 0.7), 0, "Player: %c", game_state.ActivePlayer);
    update_time();
    draw_formatted(round(width * 0.85), 0, "Time: %02d:%02d", gameTime.min, gameTime.sec); //FIXME: Fix the print_time()

    draw_formatted(0, 2, "Cheese: %d", game_state.chesee);
    draw_formatted(round(width * 0.25), 2, "Traps: %d", game_state.traps);
    draw_formatted(round(width * 0.45), 2, "Fireworks: %d", game_state.weapons);
    draw_formatted(round(width * 0.65), 2, "Level: %d", game_state.lvl);
    draw_formatted(round(width * 0.8), 2, "x: %d", (int)round(Hero.x));
    draw_formatted(round(width * 0.9), 2, "y: %d", (int)round(Hero.y));

    // draw_formatted(round(width * 0.9), 4, "dy: %lf", Chaser.dy); //TODO: REMOVE

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
    for(int i = 0; i < MAX_cheeses; i++)
    {
        if(cheeses[i].visible) draw_char(cheeses[i].x, cheeses[i].y, 'C'); //TODO: updated this to perform the test
    }
}

/*draw_all() handles the drawing of all entities of the game.*/
void draw_all()
{
    clear_screen();

    draw_game_stats();
    draw_walls();
    draw_players();
    draw_cheese();

    show_screen();
}

//---------------------------------------------------------------------------------------------

// ----------------------------- COLLISION FUNCTIONS ------------------------------------------
/*Retruns true if the two coordiantes provided do collide.*/
bool collided(int x1, int y1, int x2, int y2)
{
   return  ((x1 == x2 && y1 == y2) ? true : false);
}

/*Simple linear equaltion function. Returns the y value given a greadient (m), y-intercent (c) and the input x value is provided.*/
int h(int m, int c, int x)
{
    double result = (m*x) + c;
    return round(result);
} //TODO: REMOVE: check if funciton is needed?

// ------------------------ UPDATE FUNCTIONS --------------------------------------------------

/*Displays the game over message and waits for a key input to exit the game.*/
void game_over()
{
    //do something
}

/* update_her0() updates the hero's position based on the keyboard input. Keryboard input: a => Left,  d => Right,  w => Up,  s => Down.*/
void update_hero(int key_code) //TODO: update update_hero() to incorporate the changes based upon the game_state.currerntPlayer value. Instead of Just sticking to the Hero character.
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

/*Moves the chaser to the next VALID location. */
void move_chaser()
{
    bool bounced = false;
    // Predict the next step of the chaser
    int next_x = round(Chaser.x + Chaser.dx);
    int next_y = round(Chaser.y + Chaser.dy);

    //Check for collisions
    if(next_x == 0 || next_x == width || !(isValidLocation2(next_x, next_y)) ) //If on the left or the right border switch direction horizontally
    {
        // Chaser.dx = -Chaser.dx;
        initialise_chaser_movement( &Chaser );
        bounced = true;
    }
    if (next_y == 3 || next_y == (height + 5) || !(isValidLocation2(next_x, next_y))) //if on the top or bottom border switch direction vertically
    {
        // Chaser.dy = -Chaser.dy;
        initialise_chaser_movement( &Chaser );
        bounced = true;
    }

    if(!bounced)
    {
        Chaser.x += Chaser.dx;
        Chaser.y += Chaser.dy;
    }
}

/*Handles the movement of the chaser player.*/
void update_chaser(int key_code)
{
    if (key_code < 0)
    {
        move_chaser();
    }

    if (collided(Hero.x, Hero.y, Chaser.x, Chaser.y))
        {
            Hero.lives--;

            if(Hero.lives <= 0)
            {
                game_over();
            }
            setup_chaser();
        }
}

/*update_cheese() checks if the player has captured the cheese (by colliding into it) and updates the score accordingly. It also respons new cheese upon successful collision.*/
void update_cheese()
{
    /*
        check if timer is expired & if there are less than 5 cheeses on screen
            make the next cheese available. 
        
        for each cheese that is visible check if the 
            
    */

   //Draw cheese if the timer has expired and there are less than 5 cheeses on screen
    if(timer_expired(game_state.cheeseTimer) && game_state.chesee < 5) //Drop cheese every 2 seconds
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
            game_state.score++;
            game_state.chesee--;
            setup_cheese(i); //Reset the cheese for next round
            cheeses[i].visible = false;
            break;
        }
    }

}

/*pause_game() changes the state of the game to pause mode. */
void pause_game()
{
    //do something
}

/* update_state() updates the game's state according to the keyboard input. It handles the player movement, chaser movement and game pause funcationaliy.*/
void update_state(int key_code)
{
    // Check Pause
    if (key_code == 'p') { pause_game(); }
    else
    {
        update_hero(key_code);
        update_chaser(key_code);
        update_cheese(key_code);
    }
}

void loop() {  //TODO: check if this is a redundant function with update_state()
    int key = get_char();
    update_state(key);

    show_screen();
} // End loop()


int main(int argc, char * args[]) {
    //Check if a map file has been provided
    if (argc > 1)
    {
        setup_screen();
        FILE * mapFile = fopen(args[1], "r");
        if(mapFile != NULL)
        {
            setup(mapFile); //Set the game based on mapFile
            while ( !game_state.gameOver )
            {
                draw_all();
                loop();
                timer_pause( 10 );
            }
        }
        else
        {
            fprintf(stderr, "ERROR: Provided map file is empty. Please check the map file.");
        }
    }
    else
    {
        fprintf(stderr, "ERROR: No Map File Provided.");
    }
    return 0;
}