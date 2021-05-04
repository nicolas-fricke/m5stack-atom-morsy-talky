#include <WiFi.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>


#include <M5Atom.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define LED_MATRIX_PIN 27

//WiFi wiFi;
SocketIOclient socketIO;

const char *ssid = "Galaxy S21";
const char *password = "123456789";

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(5, 5, LED_MATRIX_PIN,
                                               NEO_MATRIX_TOP + NEO_MATRIX_RIGHT +
                                                   NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
                                               NEO_GRB + NEO_KHZ800);

const uint16_t red = matrix.Color(255, 0, 0);
const uint16_t green = matrix.Color(0, 255, 0);
const uint16_t blue = matrix.Color(0, 0, 255);

void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
  Serial.println("type");
  Serial.println(type);
  switch (type)
  {
  case sIOtype_DISCONNECT:
    Serial.printf("[IOc] Disconnected!\n");
    break;
  case sIOtype_CONNECT:
    Serial.printf("[IOc] Connected to url: %s\n", payload);

    // join default namespace (no auto join in Socket.IO V3)
    socketIO.send(sIOtype_CONNECT, "/");
    break;
  case sIOtype_EVENT:
    Serial.printf("[IOc] get event: %s\n", payload);
    break;
  case sIOtype_ACK:
    Serial.printf("[IOc] get ack: %u\n", length);
    //hexdump(payload, length);
    break;
  case sIOtype_ERROR:
    Serial.printf("[IOc] get error: %u\n", length);
    //hexdump(payload, length);
    break;
  case sIOtype_BINARY_EVENT:
    Serial.printf("[IOc] get binary: %u\n", length);
    //hexdump(payload, length);
    break;
  case sIOtype_BINARY_ACK:
    Serial.printf("[IOc] get binary ack: %u\n", length);
    //hexdump(payload, length);
    break;
  }
}

void setup()
{
  M5.begin(true, false, false);
  delay(10);

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setTextColor(blue);

  int connectionBrightness = 1;
  bool brightnessDecreasing = false;
  matrix.fillScreen(blue);
  matrix.show();

  // disable AP
  // if (WiFi.getMode() & WIFI_AP)
  // {
  //   WiFi.softAPdisconnect(true);
  // }

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    if (connectionBrightness < 40 && !brightnessDecreasing)
    {
      connectionBrightness++;
    }
    else if (connectionBrightness == 40)
    {
      brightnessDecreasing = true;
      connectionBrightness--;
    }
    else if (connectionBrightness == 1)
    {
      brightnessDecreasing = false;
      connectionBrightness++;
    }
    else
    {
      connectionBrightness--;
    }

    matrix.setBrightness(connectionBrightness);
    matrix.show();

    delay(100);
  }

  String ip = WiFi.localIP().toString();
  // Serial.println("[SETUP] WiFi Connected " + ip.c_str());

  // server address, port and URL
  socketIO.begin("http://socketio-server-xing.herokuapp.com", 8080);

  // event handler
  socketIO.onEvent(socketIOEvent);

  matrix.clear();
  matrix.setBrightness(40);
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
char morse[256] = "  ETINAMSDRGUKWOHBLZFCP VX Q YJ 56&7   8 /+  ( 94=      3   2 10";

void loop()
{
  if (M5.Btn.wasPressed())
  {
    matrix.fillScreen(green);
  }
  else if (M5.Btn.pressedFor(200))
  {
    isLongPress = true;
    matrix.fillScreen(red);
  }
  else if (M5.Btn.wasReleased())
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
  else if (isTypingCharacter && M5.Btn.releasedFor(500))
  {
    isTypingCharacter = false;
    Serial.print(" ");
    bitWrite(characterIdentifier, characterCount, 1);
    Serial.print(characterIdentifier);
    Serial.print(" ");
    Serial.print(morse[characterIdentifier]);
    Serial.print(" ");
    Serial.println();

    matrix.setCursor(0, 0);
    matrix.print(morse[characterIdentifier]);

    characterCount = 0;
    characterIdentifier = 0;
  }
  else if (isTypingWord && M5.Btn.releasedFor(1500))
  {
    isTypingWord = false;
    Serial.print("\n\n / \n\n");
    matrix.fillScreen(0);
  }

  matrix.show();
  delay(50);
  M5.update();
}
