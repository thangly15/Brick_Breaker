#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <SDL.h>
#include <SDL_image.h>

// First, we define constants for the game
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define FRAME_RATE 60

// Paddle for reflecting the ball constants
#define PADDLE_WIDTH 100
#define PADDLE_HEIGHT 15
#define PADDLE_SPEED 500

//Ball constants
#define BALL_SIZE 15
#define BALL_SPEED_X 300
#define BALL_SPEED_Y 300

// Brick wall constants
#define BRICK_ROWS 6
#define BRICK_COLS 10
#define BRICK_WIDTH 75
#define BRICK_HEIGHT 30
#define BRICK_OFFSET_X 10
#define BRICK_OFFSET_Y 50

typedef enum brick_state {// Bricks can either be empty or active
  CELL_EMPTY,
  CELL_ACTIVE
} brick_state_t;

typedef struct vec {//We define a vector struct to represent positions and velocities in 2D space
  float x;
  float y;
} vec_t;

typedef struct paddle {//The paddle has a position and a velocity
  vec_t pos;
  float vel;
} paddle_t;

typedef struct ball {//The ball has a position and a velocity
  vec_t pos;
  vec_t vel;
} ball_t;

int main(int argc, char** argv) {
    // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    //This ios to check if something went wrong. Log an error and exit.
    fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }

    // Create an SDL window for the game
  SDL_Window* window = SDL_CreateWindow(
    "Breakout - Final Project",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    SDL_WINDOW_SHOWN
  );

  if (window == NULL) {
    // Something went wrong. Log an error, shut down SDL, and then exit
    fprintf(stderr, "Failed to create SDL window: %s\n", SDL_GetError());
    SDL_Quit();
    exit(EXIT_FAILURE);
  }
  // Create an SDL renderer for we can draw to the window
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  // Check if the renderer was created successfully
  if (renderer == NULL) {
    SDL_DestroyWindow(window);
    SDL_Quit();
    exit(EXIT_FAILURE);
  }

  IMG_Init(IMG_INIT_PNG); // Initialize SDL_image for loading PNG images

  SDL_Surface* bg_surface = IMG_Load("background.png");// load our image

  SDL_Texture* bg_texture = NULL;// we don't need the surface after creating the texture, so we can free it

  if (bg_surface != NULL) {// Check if the image was loaded successfully
    bg_texture = SDL_CreateTextureFromSurface(renderer, bg_surface);
    SDL_FreeSurface(bg_surface);
  } else {
    printf("Could not load background image\n");
  }
  // Initialize brick states
  brick_state_t* bricks = malloc(sizeof(brick_state_t) * BRICK_ROWS * BRICK_COLS);
  // Check if memory allocation was successful
  if (bricks == NULL) {
    SDL_DestroyRenderer(renderer);// Clean up the renderer
    SDL_DestroyWindow(window);//clean up the window
    SDL_Quit();
    return 1;
  }
  // Set all bricks to active
  for (int i = 0; i < BRICK_ROWS * BRICK_COLS; i++) {// This is a loop to initialize all the bricks in the game. We set them all to active at the start of the game.
    bricks[i] = CELL_ACTIVE;
  }

  paddle_t player = {// The player paddle starts in the middle of the screen near the bottom, and is stationary at the start of the game
    .pos = { .x = SCREEN_WIDTH / 2 - PADDLE_WIDTH / 2, .y = SCREEN_HEIGHT - 50 },
    .vel = 0
  };

  ball_t ball = {// The ball starts in the middle of the screen and moves diagonally downwards at the start of the game
    .pos = { .x = SCREEN_WIDTH / 2, .y = SCREEN_HEIGHT / 2 },
    .vel = { .x = BALL_SPEED_X, .y = BALL_SPEED_Y }
  };

  bool left_pressed = false;//These variables are used to track whether the left or right movement keys are currently pressed. This allows for smooth movement of the paddle when the keys are held down.
  bool right_pressed = false;

  int lives = 3;

  bool game_over = false;// Initialize game state variables to track whether the game is over or won. This will be used to control the game flow and display appropriate messages.
  bool game_won = false;
  bool running = true;
  // Start the game loop
  while (running) {
    // Handle events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {// Loop while there are events to process. This is where we handle user input and other events like quitting the game.
        if (event.type == SDL_QUIT) {// If the user tries to close the window, we set running to false to exit the game loop and end the game.
            running = false;
        } else if (event.key.keysym.sym == SDLK_ESCAPE) {// If the user presses the Escape key, we also exit game
            running = false;
        } else if (event.type == SDL_KEYDOWN) {// If a key is pressed down, we check if it's the left or right movement keys and set the corresponding variables to true. This allows the paddle to start moving in the next update cycle.
            if (event.key.keysym.sym == SDLK_LEFT ||
                event.key.keysym.sym == SDLK_a) {// We check for both the left arrow key and the 'A' key for left movement, and the right arrow key and 'D' key for right movement to provide multiple options for the player to choose to control.
            left_pressed = true;
            }

            if (event.key.keysym.sym == SDLK_RIGHT ||
                event.key.keysym.sym == SDLK_d) {// This is the same as above but for right movement keys.
            right_pressed = true;
            }
        } else if (event.type == SDL_KEYUP) {// If a key is released, this allows the paddle to stop moving left when the keys are released.
            if (event.key.keysym.sym == SDLK_LEFT ||
                event.key.keysym.sym == SDLK_a) {
            left_pressed = false;
            }

            if (event.key.keysym.sym == SDLK_RIGHT ||
                event.key.keysym.sym == SDLK_d) {// This is the same as above but for right movement keys.
            right_pressed = false;
            }
        }
    }

    // Update paddle
    if (!game_over && !game_won) {// We only update the game state if the game is not over or won. This prevents the game from continuing to update and move the ball and paddle after the game has ended.
        if (left_pressed) {//this is to move the paddle left if the left movement keys are pressed. 
            //We also divide the speed by the frame rate to ensure consistent movement of the frame rate of the game.
            player.pos.x -= PADDLE_SPEED / (float)FRAME_RATE;
        }

        if (right_pressed) {// This is the same as above but for right movement keys.
            player.pos.x += PADDLE_SPEED / (float)FRAME_RATE;
        }

        if (player.pos.x < 0) {// Preventing paddle from moving off the left edge of the game screen
            player.pos.x = 0;
        }

        if (player.pos.x > SCREEN_WIDTH - PADDLE_WIDTH) {// Preventing paddle to move off screen in right direction
            player.pos.x = SCREEN_WIDTH - PADDLE_WIDTH;
        }

        // Update ball
        ball.pos.x += ball.vel.x / (float)FRAME_RATE;// Update the ball's position based on its velocity

        // Check for collisions with walls
        if (ball.pos.x <= 0 || ball.pos.x >= SCREEN_WIDTH - BALL_SIZE) {// If the ball hits the left or right wall, we reverse its x velocity to make it bounce back in the opposite direction.
            ball.vel.x *= -1;
        }

        if (ball.pos.y <= 0) {// If the ball hits the top wall, we reverse its y velocity to make it bounce back downwards towards the brick and paddle
            ball.vel.y *= -1;
        }

        if (ball.pos.y >= SCREEN_HEIGHT) {// If the ball goes below the bottom of the screen, it means the player missed it
            lives--;

            if (lives <= 0) {// If the player has no lives left, we set the game over state to true to end the game and display the game over message
                game_over = true;
            } else {// If the player still has lives left, we reset the ball and paddle to their starting positions to continue playing
                ball.pos.x = SCREEN_WIDTH / 2;
                ball.pos.y = SCREEN_HEIGHT / 2;

                ball.vel.x = BALL_SPEED_X;
                ball.vel.y = BALL_SPEED_Y;

                player.pos.x = SCREEN_WIDTH / 2 - PADDLE_WIDTH / 2;
                player.pos.y = SCREEN_HEIGHT - 50;
            }
        }

        // Paddle collision
        if (ball.pos.y + BALL_SIZE >= player.pos.y &&
            ball.pos.y <= player.pos.y + PADDLE_HEIGHT &&
            ball.pos.x + BALL_SIZE >= player.pos.x &&
            ball.pos.x <= player.pos.x + PADDLE_WIDTH) {// If the ball collides with the paddle, we reverse its y velocity to make it bounce back upwards towards the bricks
                //We also check if the ball is moving downwards before reversing the velocity to prevent it from getting stuck inside the paddle as well

            if (ball.vel.y > 0) {// This check is to ensure that the ball is only bounced back if it is moving downwards towards the paddle
                ball.vel.y *= -1;
            }
        }

        bool hit_brick = false; // initialize variable to track if a brick was hit during this cycle

        for (int r = 0; r < BRICK_ROWS && !hit_brick; r++) {// loop thru each row of bricks and check for collision of brick and ball
            for (int c = 0; c < BRICK_COLS && !hit_brick; c++) {// loop thru each column of bricks and check for collision of brick and ball
                int index = r * BRICK_COLS + c;// 1D index for the brick array based on the current row and column

                if (bricks[index] == CELL_ACTIVE) {// Only check for collision if the brick is active
                    // and iff the brick is empty, we skip the collision check for that brick and move on to the next one

                    // Calculate the x and y position of the current brick based on its row and column
                    float brick_x = BRICK_OFFSET_X + c * 78;
                    float brick_y = BRICK_OFFSET_Y + r * 35;

                    if (ball.pos.x + BALL_SIZE >= brick_x &&
                        ball.pos.x <= brick_x + BRICK_WIDTH &&
                        ball.pos.y + BALL_SIZE >= brick_y &&
                        ball.pos.y <= brick_y + BRICK_HEIGHT) {

                            bricks[index] = CELL_EMPTY;

                            ball.vel.y *= -1;

                            hit_brick = true;
                    }
                }
            }
        }

        int remaining_bricks = 0;

        for (int i = 0; i < BRICK_ROWS * BRICK_COLS; i++) {
            if (bricks[i] == CELL_ACTIVE) {
                remaining_bricks++;
            }
        }

        if (remaining_bricks == 0) {
            game_won = true;
        }
    }

    // Clear the screen
    SDL_RenderClear(renderer);

    if (bg_texture != NULL) {
        SDL_RenderCopy(renderer, bg_texture, NULL, NULL);
    }

    // Draw bricks
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    // Loop through bricks and draw active ones
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            // Calculate the index in the brocks array
            int index = r * BRICK_COLS + c;
            // If the brick is active then it is drawn
            if (bricks[index] == CELL_ACTIVE) {
                SDL_Rect brick_rect = {
                    .x = BRICK_OFFSET_X + c * 78,
                    .y = BRICK_OFFSET_Y + r * 35,
                    .w = BRICK_WIDTH,
                    .h = BRICK_HEIGHT
                };

                SDL_RenderFillRect(renderer, &brick_rect);
            }
        }
    }

    // Draw paddle
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_Rect paddle_rect = {
        .x = player.pos.x,
        .y = player.pos.y,
        .w = PADDLE_WIDTH,
        .h = PADDLE_HEIGHT
    };

    SDL_RenderFillRect(renderer, &paddle_rect);

    // Draw ball
    SDL_Rect ball_rect = {
        .x = ball.pos.x,
        .y = ball.pos.y,
        .w = BALL_SIZE,
        .h = BALL_SIZE
    };

    SDL_RenderFillRect(renderer, &ball_rect);

    if (game_won) {
        SDL_SetWindowTitle(window, "Breakout - YOU WIN!");
    } else if (game_over) {
        SDL_SetWindowTitle(window, "Breakout - GAME OVER");
    } else if (lives == 3) {
        SDL_SetWindowTitle(window, "Breakout - Lives: 3");
    } else if (lives == 2) {
        SDL_SetWindowTitle(window, "Breakout - Lives: 2");
    } else if (lives == 1) {
        SDL_SetWindowTitle(window, "Breakout - Lives: 1");
    }

    // Show everything
    SDL_RenderPresent(renderer);

    SDL_Delay(1000 / FRAME_RATE);
  }

  // Clean up 
  free(bricks);

  if (bg_texture != NULL) {
    SDL_DestroyTexture(bg_texture);
  }

  IMG_Quit();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}