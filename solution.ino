//#define DEBUG

#include "funshield.h"
#include "string.h"

typedef unsigned long ulong;
typedef unsigned int uint;

uint millis_time = 0;
uint delta_millis = 0;

// map of letter glyphs
constexpr byte glyph_letter[]{
  0b10001000,  // A
  0b10000011,  // b
  0b11000110,  // C
  0b10100001,  // d
  0b10000110,  // E
  0b10001110,  // F
  0b10000010,  // G
  0b10001001,  // H
  0b11111001,  // I
  0b11100001,  // J
  0b10000101,  // K
  0b11000111,  // L
  0b11001000,  // M
  0b10101011,  // n
  0b10100011,  // o
  0b10001100,  // P
  0b10011000,  // q
  0b10101111,  // r
  0b10010010,  // S
  0b10000111,  // t
  0b11000001,  // U
  0b11100011,  // v
  0b10000001,  // W
  0b10110110,  // ksi
  0b10010001,  // Y
  0b10100100,  // Z
};

constexpr byte EMPTY_GLYPH = 0b11111111;

byte charToGlyph(char ch) {
  byte glyph = EMPTY_GLYPH;
  if (isAlpha(ch)) {
    glyph = glyph_letter[ch - (isUpperCase(ch) ? 'A' : 'a')];
  }

  return glyph;
}

constexpr char morse_char[]{ "abcdefghijklmnopqrstuvwqyz" };
constexpr const char* morse_dots[]{ ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.." };
constexpr unsigned long duration_dot = 200;
constexpr unsigned long duration_dash = 600;
constexpr unsigned long duration_space = 2000;
constexpr uint segmentDisplayCount = 4;

// a code fragment that converts a morse sequence to a char
char find_morse_char(const char* dots) {  // "-..." -> 'b'
  for (int i = 0; morse_char[i]; ++i) {   // for each char in the array
    if (!strcmp(dots, morse_dots[i])) {   // test if the detected morse sequence is equal to the character morse code
      return morse_char[i];
    }
  }
  return ' ';  // no valid sequence detected
}

class SegmentDisplay {
public:
  SegmentDisplay() {}
  void setup() {
    pinMode(data_pin, OUTPUT);
    pinMode(latch_pin, OUTPUT);
    pinMode(clock_pin, OUTPUT);
    digitalWrite(latch_pin, LOW);

    _init = true;
  }

  void showGlyph(byte glyph, int pos) {
    if (!_init)
      return;
    digitalWrite(latch_pin, HIGH);
    shiftOut(data_pin, clock_pin, MSBFIRST, glyph);
    shiftOut(data_pin, clock_pin, MSBFIRST, positions[pos]);
    digitalWrite(latch_pin, LOW);
  }

private:
  bool _init = false;
  const byte positions[segmentDisplayCount] = { 0x01, 0x02, 0x04, 0x08 };
};

constexpr int MSG_MAX = 4;

class DisplayRefresh {
public:
  DisplayRefresh(SegmentDisplay* pDisplay)
    : pSegmentDisplay(pDisplay) {}

  void start() {
    for (int i = 0; i < MSG_MAX; ++i)
      msg_[i] = ' ';
  }

  void loop() {
    byte glyph = charToGlyph(msg_[pos]);
    pSegmentDisplay->showGlyph(glyph, visiblePositions - pos);
    pos++;
    pos %= segmentDisplayCount;
  }

  void AddLetterToMsg(char letter) {
    char prev;
    for (int i = 0; i < MSG_MAX; i++) {
      if (i == 0) {
        prev = msg_[i];
        msg_[i] = letter;
      } else {
        char swap = msg_[i];
        msg_[i] = prev;
        prev = swap;
      }
    }
  }

private:
  SegmentDisplay* pSegmentDisplay = nullptr;
  char msg_[MSG_MAX];

  int pos = 0;
  int mPtr = 0;
  const int visiblePositions = 3;  // starting with 0 - its actualy 4,
};

constexpr int SEQUENCE_MAX = 5;

class App {
public:
  App(DisplayRefresh* pDisplay)
    : pDisplayRefresh(pDisplay) {}
  void setup() {
    ResetSequence();
  }

  void DecodeSequence() {
    char letter = find_morse_char(sequence);

#define DEBUG
#ifdef DEBUG
    Serial.println(' ');
    Serial.println("sequence: ");
    Serial.println(sequence);
    Serial.println("letter: ");
    Serial.println(letter);
#endif



    ResetSequence();
    pDisplayRefresh->AddLetterToMsg(letter);
  }

  void AddToSequence(char ch) {
    if (sPtr < SEQUENCE_MAX)
      sequence[sPtr++] = ch;
  }

private:
  char sequence[SEQUENCE_MAX];
  int sPtr = 0;
  DisplayRefresh* pDisplayRefresh = nullptr;


  void ResetSequence() {
    for (int i = 0; i < SEQUENCE_MAX; ++i) {
      sequence[i] = char();
    }
    sPtr = 0;
  }
};

//////////// Button ////////////////
class Button {
public:
  Button(int pin)
    : pin_(pin) {}
  virtual ~Button() {}

  void setup() {
    pinMode(pin_, INPUT);
    state_ = digitalRead(pin_);
  }

  void loop() {
    int st = digitalRead(pin_);
    auto ctime = millis();
    if (st != state_ && ctime - detectDelayTime_ > delayTime_) {
      detectDelayTime_ = ctime;
      state_ = st;
      onStateChanged();
    }
    buttonLoop();
  }

  bool isPressed() {
    return state_ == ON;
  }

private:
  void onStateChanged() {
    if (state_ == ON) {
      press();
    } else if (state_ == OFF) {
      depress();
    }
  }

  virtual void press() = 0;
  virtual void depress(){};
  virtual void buttonLoop(){};

  int pin_;
  int state_;
  uint detectDelayTime_;
  const uint delayTime_ = 30;
};

////////////////////////////////////////////

constexpr uint DOT_DELAY = 200;
constexpr uint DASH_DELAY = 600;
constexpr uint DECODE_DELAY = 2000;

class MorseButton : public Button {
public:
  MorseButton(int pin, App* pApp_)
    : Button(pin), pApp(pApp_) {}
  ~MorseButton() {}

private:
  void press() override {
    press_time = millis_time;
  }

  void depress() override {
    uint held_time = millis_time - press_time;
#ifdef DEBUG
    Serial.print(held_time);
    Serial.print(": ");
#endif
    if (held_time <= DOT_DELAY) {
      pApp->AddToSequence('.');
#ifdef DEBUG
      Serial.print(".");
#endif
    }

    if (held_time >= DASH_DELAY) {
      pApp->AddToSequence('-');
#ifdef DEBUG
      Serial.print("-");
#endif
    }
  }

private:
  void buttonLoop() override {
    if (!isPressed()) {
      idle_time += delta_millis;
    } else {
      decoded = false;
      idle_time = 0;
    }

    if (idle_time >= DECODE_DELAY && !decoded) {
      decoded = true;
      pApp->DecodeSequence();
    }
  }

  bool decoded = false;
  uint press_time = 0;
  uint idle_time = 0;
  App* pApp = nullptr;
};

SegmentDisplay display;
DisplayRefresh displayRefresh(&display);
App app(&displayRefresh);
MorseButton morseButton(button1_pin, &app);

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif
  app.setup();
  morseButton.setup();
  display.setup();
  displayRefresh.start();
}

void loop() {
  delta_millis = millis_time;
  millis_time = millis();
  delta_millis = millis_time - delta_millis;
  morseButton.loop();
  displayRefresh.loop();
}
