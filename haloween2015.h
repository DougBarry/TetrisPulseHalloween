#ifndef HWEEN2015_H
#define HWEEN2015_H

#ifdef DEBUG
#define DEBUG_PRINT(x)     Serial.print (x)
#define DEBUG_PRINTDEC(x)     Serial.print (x, DEC)
#define DEBUG_PRINTLN(x)  Serial.println (x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTDEC(x)
#define DEBUG_PRINTLN(x)
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define NOOP() __asm__("nop\n\t")

#define GLCD_WIDTH (128)              // graphical lcd width in pixels
#define GLCD_HEIGHT (64)              // graphical lcd height in pixels

#define BEAT_BUFFER_SIZE GLCD_WIDTH
#define PIN_PULSE 0                 // Pulse Sensor purple wire connected to analog pin 0

#define ROTARY_BUTTON_LEFT 2
#define ROTARY_BUTTON_RIGHT 3

#define MODE_STARTUP  0
#define MODE_PULSE 1
#define MODE_TETRIS 2
#define MODE_WORM 3
#define MODE_OTHER 255

#define TETRIS_LANG_LEVEL "LEVEL"
#define TETRIS_LANG_LINES "LINES"
#define TETRIS_LANG_SCORE "SCORE"
#define TETRIS_LANG_NEXT "NEXT"


#endif /* HWEEN2015_H */
