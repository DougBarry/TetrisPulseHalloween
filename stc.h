/* ========================================================================== */
/*                          STC - SIMPLE TETRIS CLONE                         */
/* -------------------------------------------------------------------------- */
/*   Game constants and definitions.                                          */
/*                                                                            */
/*   Copyright (c) 2010 Laurens Rodriguez Oscanoa.                            */
/*   This code is licensed under the MIT license:                             */
/*   http://www.opensource.org/licenses/mit-license.php                       */
/* -------------------------------------------------------------------------- */

#ifndef STC_SRC_GAME_H_
#define STC_SRC_GAME_H_

#include "platform.h"

/*
 * Game configuration.
 * Edit this section to change the sizes, scores or pace of the game.
 */

/* Application name */
#define STC_GAME_NAME "Tetris"
#define STC_SHOW_GHOST_PIECE

/* Playfield size (in tiles) */
#define BOARD_TILEMAP_WIDTH (10)
#define BOARD_TILEMAP_HEIGHT (20)

/* Initial time delay (in milliseconds) between falling moves */
enum { STC_INIT_DELAY_FALL = 750 };

/* Score points given by filled rows (we use the original NES * 10)
 * http://tetris.wikia.com/wiki/Scoring */
#define SCORE_1_FILLED_ROW (400)
#define SCORE_2_FILLED_ROW (1000)
#define SCORE_3_FILLED_ROW (3000)
#define SCORE_4_FILLED_ROW (12000)

/* The player gets points every time he accelerates downfall.
 * The added points are equal to SCORE_2_FILLED_ROW divided by this value */
#define SCORE_MOVE_DOWN_DIVISOR (1000)

/* The player gets points every time he does a hard drop.
 * The added points are equal to SCORE_2_FILLED_ROW divided by these
 * values. If the player is not using the shadow he gets more points */
#define SCORE_DROP_DIVISOR (20)
#define SCORE_DROP_WITH_SHADOW_DIVISOR (100)

/* Number of filled rows required to increase the game level */
#define FILLED_ROWS_FOR_LEVEL_UP (5)

/* The falling delay is multiplied and divided by
 * these factors with every level up */
#define DELAY_FACTOR_FOR_LEVEL_UP (7)
#define DELAY_DIVISOR_FOR_LEVEL_UP (5)

/* Delayed autoshift initial delay */
#define DAS_DELAY_TIMER (200)

/* Delayed autoshift timer for left and right moves */
#define DAS_MOVE_TIMER (50)

#ifdef STC_AUTO_ROTATION
/* Rotation auto-repeat delay */
#define ROTATION_AUTOREPEAT_DELAY (375)

/* Rotation autorepeat timer */
#define ROTATION_AUTOREPEAT_TIMER (200)
#endif /* STC_AUTO_ROTATION */

/*
 * Game constants.
 * You likely don't need to change this section unless you're changing the gameplay.
 */

/* Error codes */
#define ERROR_NONE (0)   /* Everything is OK, oh wonders!     */
#define ERROR_PLAYER_QUITS (1)   /* The player quits, our fail        */
#define ERROR_NO_MEMORY (2)   /* Not enough memory                 */
#define ERROR_NO_VIDEO (3)   /* Video system was not initialized  */
#define ERROR_NO_IMAGES (4)   /* Problem loading the image files   */
#define ERROR_ASSERT (5)  /* Something went very very wrong... */


/* Game events */
enum {
  EVENT_NONE        = 0,
  EVENT_MOVE_DOWN   = 1,
  EVENT_MOVE_LEFT   = 1 << 1,
  EVENT_MOVE_RIGHT  = 1 << 2,
  EVENT_ROTATE_CW   = 1 << 3,  /* rotate clockwise           */
  EVENT_ROTATE_CCW  = 1 << 4,  /* rotate counter-clockwise   */
  EVENT_DROP        = 1 << 5,
  EVENT_PAUSE       = 1 << 6,
  EVENT_RESTART     = 1 << 7,
  EVENT_SHOW_NEXT   = 1 << 8,  /* toggle show next tetromino */
  EVENT_SHOW_SHADOW = 1 << 9,  /* toggle show shadow         */
  EVENT_QUIT        = 1 << 10  /* finish the game            */
};

/* We are going to store the tetromino cells in a square matrix */
/* of this size (this is the size of the biggest tetromino)     */
enum { TETROMINO_SIZE  = 4 };

/* Number of tetromino types */
enum { TETROMINO_TYPES = 7 };

/* Tetromino definitions.
 * They are indexes and must be between: 0 - [TETROMINO_TYPES - 1]
 * http://tetris.wikia.com/wiki/Tetromino
 * Initial cell disposition is commented below.
 */
enum {
  /*
   *              ....
   *              ####
   *              ....
   *              ....
   */
  TETROMINO_I = 0,
  /*
   *              ##..
   *              ##..
   *              ....
   *              ....
   */
  TETROMINO_O = 1,
  /*
   *              .#..
   *              ###.
   *              ....
   *              ....
   */
  TETROMINO_T = 2,
  /*
   *              .##.
   *              ##..
   *              ....
   *              ....
   */
  TETROMINO_S = 3,
  /*
   *              ##..
   *              .##.
   *              ....
   *              ....
   */
  TETROMINO_Z = 4,
  /*
   *              #...
   *              ###.
   *              ....
   *              ....
   */
  TETROMINO_J = 5,
  /*
   *              ..#.
   *              ###.
   *              ....
   *              ....
   */
  TETROMINO_L = 6
};

/* Color indexes */
enum {
  COLOR_CYAN   = 1,
  COLOR_RED    = 1,
  COLOR_BLUE   = 1,
  COLOR_ORANGE = 1,
  COLOR_GREEN  = 1,
  COLOR_YELLOW = 1,
  COLOR_PURPLE = 1,
  COLOR_WHITE  = 0     /* Used for effects (if any) */
};

/* This value used for empty tiles */
enum { EMPTY_CELL = 0 };

/*
 * Data structure that holds information about our tetromino blocks.
 */
typedef struct StcTetromino {
  /*
   *  Tetromino buffer: [x][y]
   *  +---- x
   *  |
   *  |
   *  y
   */
  char cells[TETROMINO_SIZE][TETROMINO_SIZE];
  char x;
  char y;
  char size;
  char type;
} StcTetromino;

/* Game private data forward declaration */
typedef struct StcGamePrivate StcGamePrivate;

/*
 * Data structure that holds information about our game _object_.
 * With a little more of work we could create accessors for every
 * property this _object_ shares, thus avoiding the possibility
 * of external code messing with our _object_ internal state, (as the
 * C++ version shows using inline accessors and constant references).
 * For simplicity (and speed), I'm leaving it like this.
 */
typedef struct StcGame {

  /* Statistic data */
  struct {
    unsigned long score;         /* user score for current game      */
    uint16_t lines;          /* total number of lines cleared    */
    //int totalPieces;    /* total number of tetrominoes used */
    byte level;          /* current game level               */
    //int pieces[TETROMINO_TYPES]; /* number of tetrominoes per type */
  } stats;

  /* Matrix that holds the cells (tilemap) */
  char map[BOARD_TILEMAP_WIDTH][BOARD_TILEMAP_HEIGHT];

  StcTetromino nextBlock;     /* next tetromino                */
  StcTetromino fallingBlock;  /* current falling tetromino     */

  char stateChanged;    /* 1 if game state changed, 0 otherwise */
  byte errorCode;       /* game error code                      */
  char isPaused;        /* 1 if the game is paused, 0 otherwise */
  char isOver;
  char showPreview;     /* 1 if we must show preview tetromino  */
#ifdef STC_SHOW_GHOST_PIECE
  char showShadow; /* 1 if we must show ghost shadow            */
  char shadowGap;  /* distance between falling block and shadow */
#endif

  StcPlatform      *platform; /* platform hidden implementation */
  StcGamePrivate   *data;     /* hidden game properties         */

} StcGame;


/*
 * Main game functions
 */
int stcGameInit(StcGame *game);
void stcGameUpdate(StcGame *game);
void stcGameEnd(StcGame *game);
void stcGameOnKeyDown(StcGame *game, int command);
void stcGameOnKeyUp(StcGame *game, int command);

#endif /* STC_SRC_GAME_H_ */

