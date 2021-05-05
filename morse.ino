#include <M5Atom.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <Servo.h>
#include "config.h"

#define LED_MATRIX_PIN 27
#define SERVO_MOTOR_PIN 22
#define SERVO_SENSOR_PIN 33
// Connect the Servo's brown wire to ground (G) and the red wire to 3.3V (3V3)

// Initialize the color matrix
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(5, 5, LED_MATRIX_PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB + NEO_KHZ800);

const uint16_t red = matrix.Color(255, 0, 0);
const uint16_t green = matrix.Color(0, 255, 0);
const uint16_t blue = matrix.Color(0, 0, 255);

// Initialize the servo
Servo servo;

int minFlagPosition;
int maxFlagPosition;

// SocketIO
SocketIOclient socketIO;
bool socketIOisConnected = false;

// Initialize morse parsing
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

// TODO: Just for debugging. TO REMOVE LATER.
int i = 0;

void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length);

void setup()
{
  Serial.println();

  M5.begin(true, false, false);
  delay(10);

  initializeServo();

  pinMode(SERVO_SENSOR_PIN, INPUT);

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setTextColor(blue);

  setupWifi();

  matrix.clear();
  matrix.setBrightness(40);
}

void loop()
{
  socketIO.loop();

  // TODO: Just for debugging. TO REMOVE LATER.
  if (isFlagRaised()) {
    matrix.drawPixel(0, 0, blue);
  } else {
    matrix.drawPixel(0, 0, 0);
  }

  resetServo();
  parseMorse();

  // TODO: Just for debugging. TO REMOVE LATER.
  if (i < 90) {
    moveServoToAngle(i);
    i += 2;
  }

  matrix.show();
  delay(50);
  M5.update();
}

void setupWifi() {
  int connectionBrightness = 1;
  bool brightnessDecreasing = false;
  matrix.fillScreen(blue);
  matrix.show();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("[SETUP] Connecting to SSID " + String(WIFI_SSID));

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
  Serial.println("[SETUP] WiFi Connected " + String(ip));

  // server address, port and URL
  socketIO.begin(SOCKET_IO_HOST, SOCKET_IO_PORT, "/socket.io/?EIO=4");

  Serial.println("[SETUP] Connecting to server " + String(SOCKET_IO_HOST));

  // event handler
  socketIO.onEvent(socketIOEvent);
}

void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
  Serial.println("type: " + String(type));

  switch (type)
  {
  case sIOtype_DISCONNECT:
    Serial.printf("[IOc] Disconnected!\n");
    socketIOisConnected = false;
    break;
  case sIOtype_CONNECT:
    Serial.printf("[IOc] Connected to url: %s\n", payload);

    // join default namespace (no auto join in Socket.IO V3)
    socketIO.send(sIOtype_CONNECT, "/");
    socketIOisConnected = true;
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

// Parses the bit we received into dot or dash
void parseMorse(){
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

    String payload = "[\"input\", \"" + String(morse[characterIdentifier]) + "\"]";
    Serial.println(payload);

    socketIO.send(sIOtype_EVENT, payload);

    characterCount = 0;
    characterIdentifier = 0;
  }
  else if (isTypingWord && M5.Btn.releasedFor(1500))
  {
    isTypingWord = false;
    Serial.print("\n\n / \n\n");
    matrix.fillScreen(0);
  }
}

// Detect the maximum and minimum values measured by the sero feedback
void initializeServo() {
  Serial.println("Initializing Servo stops...");

  moveServoToAngle(0);
  delay(1000);
  minFlagPosition = flagPosition();
  Serial.print("Minimum: ");
  Serial.print(minFlagPosition);

  moveServoToAngle(180);
  delay(1000);
  maxFlagPosition = flagPosition();

  Serial.print(" Maximum: ");
  Serial.println(maxFlagPosition);

  servo.detach();
}

void moveServoToAngle(int angle) {
  if (!servo.attached()) {
    servo.attach(SERVO_MOTOR_PIN);
  }
  servo.write(angle);
}

void resetServo() {
  Serial.print("flagAngle(): ");
  Serial.print(flagAngle());
  Serial.print(" servo.read(): ");
  Serial.println(servo.read());

  if (abs(servo.read() - flagAngle()) < 5) {
    Serial.println("detaching servo");
    servo.detach();
  }
}

int flagPosition() {
  return analogRead(SERVO_SENSOR_PIN);
}

int flagAngle() {
  return 180.0 * (flagPosition() - minFlagPosition) / maxFlagPosition;
}

bool isFlagRaised() {
  return flagPosition() > (maxFlagPosition + minFlagPosition) / 2;
}
