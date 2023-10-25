#define latchPin A2
#define clockPin A1
#define dataPin A0
#define ouchPin 2
#define winPin 3

volatile bool      youWin            = false;
volatile bool      youLose           = false;
volatile bool      playOuch          = false;
volatile int       strikeCount       = 0;

long long debounce_duration = 1000; // Debounce events for 1 second
long long last_debounce_at  = 0;
// Why didn't I use an enum for these? TODO make this better
bool      newGame           = true;
bool      startTheme        = false;
bool      waitForSong       = false;
int       waitSongTimeout   = false;
int       waitForSongSecs   = 10;
int       currentThemeSong  = 0;
long long blinking_duration = 200;
long long last_blink_at     = 0;

int now = 0;
bool firstRun = true;
bool testMode = false;
bool noseOn = false;
byte noseColor = 1;
byte dataToSend;
byte i = 0;
byte j = 0;

int themeSongsStartIdx = 1;
int themeSongsStopIdx = 12;
int winSongsStartIdx = 60;
int winSongsStopIdx = 64;
// Scout law from 70-82
int warnSongsStartIdx = 90;
int warnSongsStopIdx = 102;
int loseSongsStartIdx = 120;
int loseSongsStopIdx = 126;

// Table of values for MP3 index to the specific Scout Law word
// on disk corresponding to the LED that is lit up
//                  i    j
// trustworthy 70   
// loyal       71   
// helpful     72   
// friendly    73   
// courteous   74   
// kind        75   
// obedient    76   
// cheerful    77   
// thrifty     78   
// brave       79   
// clean       80   
// reverent    81         
int lawWords[ 4 ][ 3 ] = { { 75, 80, 72}, {77, 79, 78}, {73, 70, 74}, {76, 81, 71 } };

// j9 kind
// j5 clean
// j1 helpful
// j10 cheerful
// j6 brave
// j2 thrift
// j11 friendly
// j7 trustworthy
// j3 courteous
// j12 obedient
// j8 reverent
// j4 loyal


void setup() {
  Serial.begin(38400);

  randomSeed(analogRead(13));
  pinMode(ouchPin, INPUT_PULLUP);
  pinMode(winPin, INPUT_PULLUP);

  //set pins as output
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  // Set interrupt for edges
  attachInterrupt(digitalPinToInterrupt(ouchPin), ouchHandler, FALLING);
  attachInterrupt(digitalPinToInterrupt(winPin), winHandler, FALLING);

  // Random pick for the first song to play
  currentThemeSong = random(themeSongsStartIdx, themeSongsStopIdx + 1);
}

void ouchHandler() {
  int difference = now - last_debounce_at;
  if (difference > debounce_duration) {
    last_debounce_at = now;
    strikeCount++;
    if (strikeCount < 3) {
      playOuch = true;
    } else {
      youLose = true;
    }
  }
}

void winHandler() {
  int difference = now - last_debounce_at;
  if (difference > debounce_duration) {
    last_debounce_at = now;
    youWin = true;
  }
}

void cascadeLEDs(int del) {
  // Cycle through the LEDs and leave a random one on
  int x;
  int cycles = random(12, 75);
  for (x = 0; x < cycles; x++) {  // Drive LEDs
    i++;
    if (i > 2) {
      i = 0;
      j++;
      if (j > 3) {
        j = 0;
      }
    }

    dataToSend = (1 << (i+4)) | (15 & ~(1 << j));
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, LSBFIRST, dataToSend);
    digitalWrite(latchPin, HIGH);

    delay(del);
  }
}

void allOn() {
    byte data = (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (15 & ~15);
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, LSBFIRST, data);
    digitalWrite(latchPin, HIGH);
}

void allOff() {
    byte data = (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (15);
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, LSBFIRST, data);
    digitalWrite(latchPin, HIGH);
}

void doTests() {
  // Cycle bone box LEDs and play corresponding sound
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 4; j++) {
      // Play recording of the scout word corresponding to the lit up LED
      Serial.write("t");
      Serial.write(lawWords[ j ][ i ]);
  
      dataToSend = (1 << (i+4)) | (15 & ~(1 << j));
      digitalWrite(latchPin, LOW);
      shiftOut(dataPin, clockPin, LSBFIRST, dataToSend);
      digitalWrite(latchPin, HIGH);

      delay(3000);
    }
  }

  // Cycle Nose colors
  i = 3;
  for (j = 0; j < 4; j++) {
    dataToSend = (1 << (i+4)) | (15 & ~(1 << j));
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, LSBFIRST, dataToSend);
    digitalWrite(latchPin, HIGH);

    delay(1000 * (j + 1));
  }

}

void loop() {
  now = millis();

  if (firstRun || testMode) {
    allOn();
    delay(2000);
    allOff();
    firstRun = false;
  }

  if (testMode) {
    doTests();
    return;
  }

  if (newGame && !waitForSong) {
    newGame = false;
    cascadeLEDs(50);

    // Play recording of the scout word corresponding to the lit up LED
    Serial.write("t");
    Serial.write(lawWords[ j ][ i ]);
    waitForSong = true;
    waitSongTimeout = now + waitForSongSecs * 1000;
    currentThemeSong = random(themeSongsStartIdx, themeSongsStopIdx + 1);
    strikeCount = 0;
  }
  
  if (waitForSong) {
    if (now > waitSongTimeout) {
      waitForSong = false;
      return;
    }
    if (Serial.available() > 0) {
      // read the incoming byte:
      char data_rcvd = Serial.read();
      if (data_rcvd == 'X') {
        startTheme = true;
        waitForSong = false;
      }
    }
    return;
  }
 
  if (startTheme) {
    startTheme = false;
    // Game start, play a song
    Serial.write("t");
    Serial.write(currentThemeSong + (strikeCount * 20));
  }

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, dataToSend);
  digitalWrite(latchPin, HIGH);

  // Cycle through nose colors: green(0), orange(1), blinking orange(2)
  // Blink nose orange if we're on our last strike
  if (strikeCount < 2) {
    noseOn = true;
  } else if (now > last_blink_at + blinking_duration) {
    noseOn = !noseOn;
    last_blink_at = now;
  }

  if (strikeCount == 0) {
      noseColor = 1;
  } else {
      noseColor = 2;
  }

  if (noseOn) {
    //byte data = (1 << (noseColor + 4)) | (15 & ~(1 << 3));
    byte data = (1 << (7)) | (15 & ~(1 << noseColor));
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, LSBFIRST, data);
    digitalWrite(latchPin, HIGH);
  }

  if (youWin) {
    youWin = false;
    newGame = true;
    // Play a Win song
    Serial.write("t");
    Serial.write(random(winSongsStartIdx, winSongsStopIdx + 1));
    waitForSong = true;
    waitSongTimeout = now + waitForSongSecs * 1000;
    allOn();
  }

  if (playOuch) {
    playOuch = false;
    // Play a warning
    Serial.write("t");
    Serial.write(random(warnSongsStartIdx, warnSongsStopIdx + 1));
    waitForSong = true;
    waitSongTimeout = now + waitForSongSecs * 1000;
  }

  if (youLose) {
    youLose = false;
    newGame = true;
    // Turn the nose Red
    byte data = (1 << (3 + 4)) | (15 & ~(1 << 3));
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, LSBFIRST, data);
    digitalWrite(latchPin, HIGH);
    // Too many strikes, play game over song
    Serial.write("t");
    Serial.write(random(loseSongsStartIdx, loseSongsStopIdx + 1));
    waitForSong = true;
    waitSongTimeout = now + waitForSongSecs * 1000;
  }
}
