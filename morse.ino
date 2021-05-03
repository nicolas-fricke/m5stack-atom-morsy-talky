#include <M5Atom.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define PIN 27

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(5, 5, PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB + NEO_KHZ800);

const uint16_t red = matrix.Color(255, 0, 0);
const uint16_t green = matrix.Color(0, 255, 0);
const uint16_t blue = matrix.Color(0, 0, 255);

void setup()
{
  M5.begin(true, false, false);
  delay(10);

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(40);
  matrix.setTextColor(blue);
}

bool isLongPress = false;
bool isTypingCharacter = false;
bool isTypingWord = false;

// Use binary representation for encoding the morse character. For each "." we
// set the bit to 0 and each "-" is a 1. Then, each character is delimited by
// one final 1 bit.
byte characterCount = 0;
byte characterIdentifier = 0;

// With that encoding, this lookup table can be used to translate the caracters
// to alphanumerics:
//                 0         1         2         3         4         5         6
//                 01234567890123456789012345678901234567890123456789012345678901234
char morse[256] = "  etinamsdrgukwohblzfcp vx q yj 56&7   8 /+  ( 94=      3   2 10";

void loop()
{
  if (M5.Btn.wasPressed())
  {
    matrix.fillScreen(red);
  }

  if (M5.Btn.pressedFor(200))
  {
    isLongPress = true;
    matrix.fillScreen(green);
  }

  if (M5.Btn.wasReleased())
  {
    if (isLongPress)
    {
      Serial.print("-");
      bitWrite(characterIdentifier, characterCount, 1);
    }
    else
    {
      Serial.print(".");
      bitWrite(characterIdentifier, characterCount, 0);
    }
    isLongPress = false;
    isTypingCharacter = true;
    isTypingWord = true;
    characterCount++;

    matrix.fillScreen(0);
  }

  if (isTypingCharacter && M5.Btn.releasedFor(500))
  {
    isTypingCharacter = false;
    Serial.print(" ");
    bitWrite(characterIdentifier, characterCount, 1);
    Serial.print(characterIdentifier);
    Serial.print(" ");
    Serial.print(morse[characterIdentifier]);
    Serial.print(" ");
    characterCount = 0;
    characterIdentifier = 0;
    Serial.println();
  }

  if (isTypingWord && M5.Btn.releasedFor(1500))
  {
    isTypingWord = false;
    Serial.print("\n\n / \n\n");
  }

  matrix.show();
  delay(50);
  M5.update();
}
