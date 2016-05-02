//#define DEBUG

#include <Arduino.h>
#include <Wire.h>
#include <U8glib.h>
#include "haloween2015.h"
#include "TQED.h"

#include "stc.h"

/************************
   Using my TQED extended library - https://github.com/DougBarry/TQED
 ************************/
#define I2C_TQED_LEFT_ADDR 0x39
#define I2C_TQED_RIGHT_ADDR 0x38

#define TILE_SIZE_RENDER_X (4)
#define TILE_SIZE_RENDER_Y (3)

#define UI_TITLECOUNT 3
#define UI_PAUSED_MESSAGE "PAUSED"
#define UI_GAMEOVER_MESSAGE "GAME OVER!"

const char *uiTitles[UI_TITLECOUNT] = { "Halloween Heart", "Rate Monitor", "Doug Barry 2015"};


/* ***********
   DEVICE STATE
  *********** */
uint8_t currentDeviceMode = MODE_STARTUP;    /* initial state is pulse sensor */

/* ***********
  TETRIS GAME STATE
  ************ */
StcGame tetris;
uint8_t tetris_highscore = 0;
unsigned long tetris_lastdroptime = 0;

bool restarted = false;
int error_code = 0;

/* ***********
  ROTARY INPUT
   *********** */
TQED rotaryLeft(I2C_TQED_LEFT_ADDR);
TQED rotaryRight(I2C_TQED_RIGHT_ADDR);

int rotaryLeftNew = 0;
int rotaryRightNew = 0;
int rotaryLeftOld = 0;
int rotaryRightOld = 0;

bool rotaryLeftPressed = false;
bool rotaryRightPressed = false;


/* ************
  GRAPHICAL LCD - Using hardware SPI
  ************* */
U8GLIB_LM6059 u8g(6, 5, 7);


/* ******
  GLOBALS
 ******* */
unsigned int currentHBRate = 0;


/* ************
  MULTI-TASKING
  ************* */
unsigned int msSinceLastScreenDraw = 0;
unsigned int msLastScreenDraw = 0;
unsigned int msSinceLastUpdate = 0;
unsigned int msLastUpdate = 0;

unsigned int pulseUpdateWaitms = 20;
unsigned int screenDrawWaitms = 200;



// storage for heat beat data used in display
int16_t beatBuffer[BEAT_BUFFER_SIZE];
// buffer size depends on lcd width, careful of datatype for cursor
uint8_t beatBufferPos = 0;


/* ****************
  PULSE SENSOR
  **************** */
volatile int dataBPM;                   // used to hold the pulse rate
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // holds the time between beats, the Inter-Beat Interval
volatile bool Pulse = false;     // true when pulse wave is high, false when it's low
volatile bool QS = false;        // becomes true when Arduoino finds a beat.
bool goodHB = false;

volatile int rate[10];                    // used to hold last ten IBI values
volatile unsigned long sampleCounter = 0;          // used to determine pulse timing
volatile unsigned long lastBeatTime = 0;           // used to find the inter beat interval
volatile int P = 512;                     // used to find peak in pulse wave
volatile int T = 512;                     // used to find trough in pulse wave
volatile int thresh = 768;                // used to find instant moment of heart beat
volatile int amp = 100;                   // used to hold amplitude of pulse waveform
volatile bool firstBeat = true;        // used to seed rate array so we startup with reasonable BPM
volatile bool secondBeat = true;       // used to seed rate array so we startup with reasonable BPM


// start up
void setup() {
  u8g.begin();
  u8g.setRot180();

  // enable serial for debugging
#ifdef DEBUG
  Serial.begin(57600);             // we agree to talk fast!
  DEBUG_PRINTLN("serial!");
  delay(500);
#endif

  // sets up to read Pulse Sensor signal every 2mS
  pulseInterruptSetup();

  for (int px = 0; px < BEAT_BUFFER_SIZE; px++)
  {
    beatBuffer[px] = 512;
  }

  // UN-COMMENT THE NEXT LINE IF YOU ARE POWERING The Pulse Sensor AT LOW VOLTAGE,
  // AND APPLY THAT VOLTAGE TO THE A-REF PIN
  //analogReference(EXTERNAL);

  // pin d0 and d1 are exclusive with serial debug!!
  // pin 13 has onboard LED problem
  pinMode(ROTARY_BUTTON_LEFT, INPUT);
  pinMode(ROTARY_BUTTON_RIGHT, INPUT);

#ifndef DEBUG
  DEBUG_PRINTLN("init tetris");
  stcGameInit(&tetris);

#ifdef DEBUG
  // test i2c
  DEBUG_PRINTLN("Attempting I2C test");
  long m = rotaryLeft.getCounterMin();
  DEBUG_PRINT("LeftMin: ");
  DEBUG_PRINTLN(m);
  m = rotaryLeft.getCounterMax();
  DEBUG_PRINT("LeftMax: ");
  DEBUG_PRINTLN(m);
#endif

  //zero rotary encoders as they may not have been reset if we are
  rotaryLeft.centerCount();
  rotaryRight.centerCount();

#endif
}

void loop(void)
{
  switch (currentDeviceMode)
  {
    case MODE_PULSE:
      modePulseExec();
      break;
    case MODE_TETRIS:
      modeTetrisExec();
      break;
    default:
    case MODE_STARTUP:
    case MODE_OTHER:
      currentDeviceMode = MODE_STARTUP;
      modeStartupExec();
      if (millis() > 5000)
      {
        // advance to modePulse
        currentDeviceMode = MODE_PULSE;
      }
      break;
  }
}

void moveToNextDeviceMode()
{
  stcGameOnKeyDown(&tetris, EVENT_RESTART);
  stcGameOnKeyUp(&tetris, EVENT_RESTART);

#ifdef GAME_WORM
  wormInit();
#endif

  currentDeviceMode++;
  u8g.begin();
  delay(500);
}

void modeStartupExec()
{
  u8g.firstPage();
  u8g.setFont(u8g_font_unifont);
  u8g.setFontPosTop();
  do {
    for (char i = 0; i < UI_TITLECOUNT; i++)
    {
      u8g_uint_t textWidth = u8g.getStrWidth(uiTitles[i]);
      u8g.drawStr((GLCD_WIDTH / 2) - (textWidth / 2), i * 20, uiTitles[i]);
    }
  } while (u8g.nextPage());
}

void inputUpdateStart(bool debounce = true)
{
  //  DEBUG_PRINTLN("Input>GetNewValues");
  rotaryLeftNew = (rotaryLeft.getCount() / 4);
  rotaryRightNew = (rotaryRight.getCount() / 4);
  rotaryLeftPressed = false;
  rotaryRightPressed = false;

  if (!debounce)
  {
    DEBUG_PRINTLN("Debounce off!");
    if (digitalRead(ROTARY_BUTTON_LEFT))
    {
      rotaryLeftPressed = true;
      DEBUG_PRINTLN("Left->Pressed");
    }
    if (digitalRead(ROTARY_BUTTON_RIGHT))
    {
      rotaryRightPressed = true;
      DEBUG_PRINTLN("Right->Pressed");
    }
  }
  else
  {
    static bool dL = false;
    static bool dR = false;
    if (digitalRead(ROTARY_BUTTON_LEFT))
    {
      DEBUG_PRINTLN("Left->PressedMaybe");
      dL = true;
    }
    if (digitalRead(ROTARY_BUTTON_RIGHT))
    {
      dR = true;
      DEBUG_PRINTLN("Right->PressedMaybe");
    }

    if (dL || dR)
    {
      // need to debounce
      delay(50);
      //test again
      if ((digitalRead(ROTARY_BUTTON_LEFT)) && dL)
      {
        // left defo down!
        rotaryLeftPressed = true;
        dL = false;
        DEBUG_PRINTLN("Left->PressedDefo");
      }
      if ((digitalRead(ROTARY_BUTTON_RIGHT)) && dR)
      {
        // left defo down!
        rotaryRightPressed = true;
        dR = false;
        DEBUG_PRINTLN("Right->PressedDefo");
      }
    }
    // reset tests
    dL = false;
    dR = false;

  }
  if (rotaryLeftPressed && rotaryRightPressed)
  {
    // MODE CHANGE!!
    moveToNextDeviceMode();
    DEBUG_PRINTLN("Device mode change!");
  }


#ifdef DEBUG
  if (rotaryLeftNew != rotaryLeftOld)
  {
    DEBUG_PRINTDEC(rotaryLeftNew);
    DEBUG_PRINTLN();
  }
  if (rotaryRightNew != rotaryRightOld)
  {
    DEBUG_PRINTDEC(rotaryRightNew);
    DEBUG_PRINTLN();
  }
#endif

}

void inputUpdateEnd()
{
  rotaryLeftOld = rotaryLeftNew;
  rotaryRightOld = rotaryRightNew;
}

#ifdef GAME_WORM
void modeWormExec()
{
  inputUpdateStart();

  wormUpdate();
  wormDraw();

  inputUpdateEnd();
}
#endif

void modePulseExec()
{
  msSinceLastUpdate = millis() - msLastUpdate;
  msSinceLastScreenDraw = millis() - msLastScreenDraw;

  // try to update pulse data every 500th second
  if (msSinceLastUpdate > pulseUpdateWaitms)
  {
    pulseUpdate();
    msLastUpdate = millis();
  }

  if (msSinceLastScreenDraw > screenDrawWaitms)
  {
    u8g.firstPage();
    do
    {
      pulseDraw(false);
    } while ( u8g.nextPage() );
    msLastScreenDraw = millis();
  }

  inputUpdateStart();

  if (rotaryLeftNew != rotaryLeftOld)
  {
    if (rotaryLeftNew < rotaryLeftOld)
    {
      // twiddled anti-clockwise
      screenDrawWaitms - 10;
      if (screenDrawWaitms < 50) screenDrawWaitms = 50;
    }
    else
    {
      // twiddled clockwise
      screenDrawWaitms += 25;
      if (screenDrawWaitms > 1000) screenDrawWaitms = 1000;
    }
  }

  if (rotaryRightNew != rotaryRightOld)
  {
    if (rotaryRightNew < rotaryRightOld)
    {
      // twiddled anti-clockwise
      if (pulseUpdateWaitms < 50)
      {
        pulseUpdateWaitms--;
      }
      else
      {
        pulseUpdateWaitms -= 100;
      }
      if (pulseUpdateWaitms < 2) pulseUpdateWaitms = 2;
    }
    else
    {
      // twiddled clockwise
      if (pulseUpdateWaitms < 50)
      {
        pulseUpdateWaitms++;
      }
      else
      {
        pulseUpdateWaitms += 100;
      }
      if (pulseUpdateWaitms > 5000) pulseUpdateWaitms = 5000;
    }
  }

  inputUpdateEnd();
}

void modeTetrisExec()
{
  DEBUG_PRINTLN("TetrisExec");
  DEBUG_PRINTLN("TetrisGameUpdate");
  stcGameUpdate(&tetris);

  DEBUG_PRINTLN("TetrisStartDraw");
  u8g.firstPage();
  do
  {
    DEBUG_PRINTLN("TetrisDrawing");
    stcGameRenderGame(&tetris);
    pulseUpdate();
    pulseDraw(true);
  } while ( u8g.nextPage() );

  DEBUG_PRINTLN("TetrisDrawFinished");


  DEBUG_PRINTLN("TetrisWait");
  //delay(250);  // FIXME GAMESPEED
  //delay(250 - (tetris.stats.score / 10));
}

void pulseDraw(bool overlay)
{

  static char beatText[4];

  if (goodHB)
  {
    itoa((int)currentHBRate, beatText, 10);
  }
  else
  {
    beatText[0] = 0x2D;
    beatText[1] = 0x2D;
    beatText[2] = 0x00;
    beatText[3] = 0x00;
  }

  uint8_t beatY = 0;

  if (overlay)
  {
    beatY =  ((GLCD_HEIGHT - 2) - 11);
  }

  // hb number
  u8g.setFont(u8g_font_unifont);
  u8g_uint_t textWidth = u8g.getStrWidth(beatText);
  u8g.setFontPosTop();
  u8g.drawStr((GLCD_WIDTH - 2) - textWidth, beatY, beatText);

  // heart icon
  u8g.setFont(u8g_font_unifont_76);
  u8g.setFontPosTop();
  beatText[0] = 0x85;
  beatText[1] = 0x00;
  u8g.drawStr((GLCD_WIDTH - 2 - 9) - textWidth, beatY, beatText);

  if (!overlay)
  {
    for (int px = 0; px < BEAT_BUFFER_SIZE; px++)
    {
      int py = map(beatBuffer[px], 0, 1023, 0, GLCD_HEIGHT);
      u8g.drawPixel(map(px, 0, BEAT_BUFFER_SIZE, 0, GLCD_WIDTH), py);
    }
  }
}

void pulseUpdate()
{

  if (QS == true) {                      // Quantified Self flag is true when arduino finds a heartbeat

    /*        fadeRate = 255;                  // Set 'fadeRate' Variable to 255 to fade LED with pulse
            sendDataToProcessing('B',dataBPM);   // send heart rate with a 'B' prefix
            sendDataToProcessing('Q',IBI);   // send time between beats with a 'Q' prefix
    */
    QS = false;                      // reset the Quantified Self flag for next time
    currentHBRate = dataBPM;
    //    DEBUG_PRINT("HBFound :");
    //    DEBUG_PRINTDEC(currentHBRate);
    //    DEBUG_PRINTLN();
  }

  beatBuffer[beatBufferPos] = Signal;
  beatBufferPos++;
  if (beatBufferPos > BEAT_BUFFER_SIZE)
  {
    beatBufferPos = 0;
  }

  goodHB = false;
  for (int px = 0; px < BEAT_BUFFER_SIZE; px += 8)
  {
    if ((beatBuffer[px] < 300) || (beatBuffer[px] > 700))
    {
      goodHB = true;
      //      DEBUG_PRINTLN("GOOD HB!");
      continue;
    }
  }

}

/***********
  TETRIS
  ********** */

void setCell(char x, char y, char bx, char by, char c, char f)
{
  if (!c) return;
  //  DEBUG_PRINTDEC(x);
  //  DEBUG_PRINTDEC(y);
  if (!f)
  {
    u8g.drawBox((x * TILE_SIZE_RENDER_X) + 2, (y * TILE_SIZE_RENDER_Y) + 2, TILE_SIZE_RENDER_X, TILE_SIZE_RENDER_Y);
  }
  else
  {
    u8g.drawFrame((x * TILE_SIZE_RENDER_X) + 2, (y * TILE_SIZE_RENDER_Y) + 2, TILE_SIZE_RENDER_X, TILE_SIZE_RENDER_Y);
  }
}

int platformInit(StcGame * gameInstance)
{
  //  printboard();
  return 0;
}

void platformEnd(StcGame * gameInstance)
{
}

void platformReadInput(StcGame * gameInstance)
{
  DEBUG_PRINTLN("TetrisReadInputEnter");

  delay(50);

  // reading this too often makes it a bit jumpy
  DEBUG_PRINTLN("InputUpdateStart");
  inputUpdateStart();

  DEBUG_PRINTLN("InputLeftTest");

  if (rotaryLeftNew < rotaryLeftOld)
  {
    // twiddled anti-clockwise
    // turns out the STC implimentation doesnt have CCW rotation!
    stcGameOnKeyDown(&tetris, EVENT_ROTATE_CW);
    stcGameOnKeyUp(&tetris, EVENT_ROTATE_CW);
  }
  else if (rotaryLeftNew > rotaryLeftOld)
  {
    // twiddled clockwise
    stcGameOnKeyDown(&tetris, EVENT_ROTATE_CW);
    stcGameOnKeyUp(&tetris, EVENT_ROTATE_CW);
  }


  DEBUG_PRINTLN("InputRightTest");
  if (rotaryRightNew < rotaryRightOld)
  {
    // twiddled anti-clockwise
    stcGameOnKeyDown(&tetris, EVENT_MOVE_LEFT);
    stcGameOnKeyUp(&tetris, EVENT_MOVE_LEFT);
  }
  else if (rotaryRightNew > rotaryRightOld)
  {
    // twiddled clockwise
    stcGameOnKeyDown(&tetris, EVENT_MOVE_RIGHT);
    stcGameOnKeyUp(&tetris, EVENT_MOVE_RIGHT);
  }

  DEBUG_PRINTLN("InputUpdateEnd");

  inputUpdateEnd();

  DEBUG_PRINTLN("Keyup");
  stcGameOnKeyUp(&tetris, EVENT_DROP);

  DEBUG_PRINTLN("InputLeftPressedTest");

  if (rotaryLeftPressed)
  {
    if ((millis() - tetris_lastdroptime) > 1000) // once a second
    {
      stcGameOnKeyDown(&tetris, EVENT_DROP);
      tetris_lastdroptime = millis();
    }
  }
  DEBUG_PRINTLN("TetrisReadInputLeave");

}

void stcGameRenderGame(StcGame * gameInstance)
{
  platformRenderGame(gameInstance);
}

void platformRenderGame(StcGame * gameInstance)
{

  // is paused? is over?
  if (!tetris.isPaused && !tetris.isOver) {

    // draw paused text
    // draw gameover text

    char i, j;

    // clear screen

    // draw titles

    // draw lables

    // draw score
    u8g.setFont(u8g_font_profont10);
    u8g.setFontPosTop();

    u8g.drawStr(BOARD_TILEMAP_WIDTH * TILE_SIZE_RENDER_X + 10, 0, TETRIS_LANG_LEVEL);
    u8g.drawStr(BOARD_TILEMAP_WIDTH * TILE_SIZE_RENDER_X + 10, 20, TETRIS_LANG_SCORE);
    u8g.drawStr(BOARD_TILEMAP_WIDTH * TILE_SIZE_RENDER_X + 10, 40, TETRIS_LANG_LINES);
    u8g_uint_t textwidth = u8g.getStrWidth(TETRIS_LANG_NEXT);
    u8g.drawStr((GLCD_WIDTH - 2 - textwidth), 0, TETRIS_LANG_NEXT);

    char buffer[7];

    itoa(tetris.stats.level, buffer, 10);
    u8g.drawStr(BOARD_TILEMAP_WIDTH * TILE_SIZE_RENDER_X + 10, 10, buffer);
    itoa(tetris.stats.score, buffer, 10);
    u8g.drawStr(BOARD_TILEMAP_WIDTH * TILE_SIZE_RENDER_X + 10, 30, buffer);
    itoa(tetris.stats.lines, buffer, 10);
    u8g.drawStr(BOARD_TILEMAP_WIDTH * TILE_SIZE_RENDER_X + 10, 50, buffer);

    // draw lines taken

    // draw level

    // draw borders
    u8g.drawFrame(0, 0, (BOARD_TILEMAP_WIDTH * TILE_SIZE_RENDER_X) + 4, (BOARD_TILEMAP_HEIGHT * TILE_SIZE_RENDER_Y) + 4);

    // draw current blocks

    for (i = 0; i < BOARD_TILEMAP_WIDTH; i++) {
      for (j = 0; j < BOARD_TILEMAP_HEIGHT; j++) {
#ifdef STC_SHOW_GHOST_PIECE
        setCell(i, j, TILE_SIZE_RENDER_X, TILE_SIZE_RENDER_Y, tetris.map[i][j], 0);
#else
        setCell(i, j, TILE_SIZE_RENDER_X, TILE_SIZE_RENDER_Y, tetris.map[i][j], 1);
#endif
      }
    }


    // draw "ghost" piece (drop target)
#ifdef STC_SHOW_GHOST_PIECE
    for (i = 0; i < TETROMINO_SIZE; i++) {
      for (j = 0; j < TETROMINO_SIZE; j++) {
        if (tetris.fallingBlock.cells[i][j] != EMPTY_CELL)
          setCell(tetris.fallingBlock.x + i, tetris.fallingBlock.y + tetris.shadowGap + j, TILE_SIZE_RENDER_X, TILE_SIZE_RENDER_Y, 0, 1);
      }
    }
#endif

    // draw falling block

    for (i = 0; i < TETROMINO_SIZE; i++) {
      for (j = 0; j < TETROMINO_SIZE; j++) {
        if (tetris.fallingBlock.cells[i][j]) {
          setCell(tetris.fallingBlock.x + i, tetris.fallingBlock.y + j, TILE_SIZE_RENDER_X, TILE_SIZE_RENDER_Y, 1, 1);
        }
      }
    }

    // draw nextblock block
    //    u8g.drawFrame(GLCD_WIDTH - 21, 0, (TETROMINO_SIZE * TILE_SIZE_RENDER_X) + 1, (TETROMINO_SIZE * TILE_SIZE_RENDER_Y) + 1);
    for (i = 0; i < TETROMINO_SIZE; i++) {
      for (j = 0; j < TETROMINO_SIZE; j++) {
        setCell (tetris.nextBlock.x + i + 27, tetris.nextBlock.y + j + 4, GLCD_WIDTH - 18, 2, tetris.nextBlock.cells[i][j], 0);
      }
    }
  }
  DEBUG_PRINTLN("TetrisGameOverTest");

  if (tetris.isOver)
  {
    u8g.setFont(u8g_font_unifont);
    u8g_uint_t textWidth = u8g.getStrWidth(UI_GAMEOVER_MESSAGE);
    u8g.drawStr((GLCD_WIDTH / 2) - (textWidth / 2), 26, UI_GAMEOVER_MESSAGE);
  }

  tetris.stateChanged = 0;

}

long platformGetSystemTime() {
  return millis();
}

void platformSeedRandom(long seed) {
  srand(analogRead(0)*seed);
}

int platformRandom() {
  return rand();
}


/* ****************
  PULSE SENSOR
  **************** */
void pulseInterruptSetup() {
  // Initializes Timer2 to throw an interrupt every 2mS.
  TCCR2A = 0x02;     // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
  TCCR2B = 0x06;     // DON'T FORCE COMPARE, 256 PRESCALER
  OCR2A = 0X7C;      // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
  TIMSK2 = 0x02;     // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
  sei();             // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED
}


// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE.
// Timer 2 makes sure that we take a reading every 2 miliseconds
ISR(TIMER2_COMPA_vect) {                        // triggered when Timer2 counts to 124
  cli();                                      // disable interrupts while we do this

  Signal = analogRead(PIN_PULSE);              // read the Pulse Sensor
  sampleCounter += 2;                         // keep track of the time in mS with this variable
  int N = sampleCounter - lastBeatTime;       // monitor the time since the last beat to avoid noise

  //  find the peak and trough of the pulse wave
  if (Signal < thresh && N > (IBI / 5) * 3) { // avoid dichrotic noise by waiting 3/5 of last IBI
    if (Signal < T) {                       // T is the trough
      T = Signal;                         // keep track of lowest point in pulse wave
    }
  }

  if ((Signal > thresh) && (Signal > P)) {        // thresh condition helps avoid noise
    P = Signal;                             // P is the peak
  }                                        // keep track of highest point in pulse wave

  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // signal surges up in value every time there is a pulse
  if (N > 250) {                                  // avoid high frequency noise
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI / 5) * 3) ) {
      Pulse = true;                               // set the Pulse flag when we think there is a pulse
      //    digitalWrite(blinkPin,HIGH);                // turn on pin 13 LED
      IBI = sampleCounter - lastBeatTime;         // measure time between beats in mS
      lastBeatTime = sampleCounter;               // keep track of time for next pulse

      if (firstBeat) {                       // if it's the first time we found a beat, if firstBeat == TRUE
        firstBeat = false;                 // clear firstBeat flag
        return;                            // IBI value is unreliable so discard it
      }
      if (secondBeat) {                      // if this is the second beat, if secondBeat == TRUE
        secondBeat = false;                 // clear secondBeat flag
        for (int i = 0; i <= 9; i++) {   // seed the running total to get a realisitic BPM at startup
          rate[i] = IBI;
        }
      }

      // keep a running total of the last 10 IBI values
      word runningTotal = 0;                   // clear the runningTotal variable

      for (int i = 0; i <= 8; i++) {          // shift data in the rate array
        rate[i] = rate[i + 1];            // and drop the oldest IBI value
        runningTotal += rate[i];          // add up the 9 oldest IBI values
      }

      rate[9] = IBI;                          // add the latest IBI to the rate array
      runningTotal += rate[9];                // add the latest IBI to runningTotal
      runningTotal /= 10;                     // average the last 10 IBI values
      dataBPM = 60000 / runningTotal;             // how many beats can fit into a minute? that's BPM!
      QS = true;                              // set Quantified Self flag
      // QS FLAG IS NOT CLEARED INSIDE THIS ISR
    }
  }

  if (Signal < thresh && Pulse == true) {    // when the values are going down, the beat is over
    //      digitalWrite(blinkPin,LOW);            // turn off pin 13 LED
    Pulse = false;                         // reset the Pulse flag so we can do it again
    amp = P - T;                           // get amplitude of the pulse wave
    thresh = amp / 2 + T;                  // set thresh at 50% of the amplitude
    P = thresh;                            // reset these for next time
    T = thresh;
  }

  if (N > 2500) {                            // if 2.5 seconds go by without a beat
    thresh = 512;                          // set thresh default
    P = 512;                               // set P default
    T = 512;                               // set T default
    lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date
    firstBeat = true;                      // set these to avoid noise
    secondBeat = true;                     // when we get the heartbeat back
  }

  sei();                                     // enable interrupts when youre done!
}// end isr


/* ***********
   MATRIX Screen saver type thing
 ************/

#define MATRIX_COLUMNS (20)
#define MATRIX_ROWS (20)
#define MATRIX_UPDATE_FREQ (500)

typedef struct {
  uint8_t row;
  char ch;
} MatrixCol;

MatrixCol matrixCol[MATRIX_COLUMNS];

void modeMatrixExec()
{
  matrixUpdate();

  static unsigned long lastUpdatems = 0;

  if ((millis() - lastUpdatems) > MATRIX_UPDATE_FREQ)
  {
    u8g.firstPage();
    do
    {
      DEBUG_PRINTLN("MatrixDrawing");
      matrixDraw();
    } while ( u8g.nextPage() );
    lastUpdatems = millis();
  }

}

void matrixDraw()
{
  static char ch[2];
  static uint8_t x, y;
  DEBUG_PRINTLN("Draw matrix");

  u8g.setFont(u8g_font_unifont_76);
  u8g.setFontPosTop();

  for (uint8_t i = 0; i < 24; i++)
  {
    x = random(0, MATRIX_COLUMNS);
    y = random(0, MATRIX_ROWS);

    ch[0] = random(0, 0x9f);;
    ch[1] = 0x00;

    u8g.drawStr(x * 16, y * 16, ch);
  }
}

void matrixUpdate()
{
  inputUpdateStart();
  inputUpdateEnd();
}

