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

#define MATRIX_WIDTH 5

enum State {
  idle,
  typing,
  reading,
  receiving
};
State currentState;

// Initialize the color matrix
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(5, 5, LED_MATRIX_PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB + NEO_KHZ800);

const uint16_t red = matrix.Color(255, 0, 0);
const uint16_t green = matrix.Color(0, 255, 0);
const uint16_t blue = matrix.Color(0, 0, 255);
const uint16_t yellow = matrix.Color(255, 255, 0);

int textScrollPosition = 0;

// Initialize the servo
Servo servo;

int minFlagPosition;
int maxFlagPosition;
int servoDetachTimeout = 0;

// SocketIO
SocketIOclient socketIO;
bool socketIOisConnected = false;
String receivedText;

// Initialize morse parsing
bool isLongPress = false;
bool isTypingCharacter = false;
bool isSpaceCharacter = false;

// Use binary representation for encoding the morse character. For each "." we
// set the bit to 0 and each "-" is a 1. Then, each character is delimited by
// one final 1 bit.
byte characterCount = 0;
byte characterIdentifier = 0;

// Buffer for the currently typed morse message
String morseMessageBuffer = "";

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

  moveServoToAngle(0);
  delay(500);
  setCurrentState(idle);
}

void loop()
{
  socketIO.loop();
  resetServo();

  switch (currentState) {
    // 1. IDLE: flag is moved to 180? send msg "wait for message", change to TYPING
    // 2. IDLE: receive "wait for message", engage servo, change to RECEIVING
    case idle:
      loopIdle();
      break;
    // 3. TYPING: handle morse code like you know, on flag raised to 90, send text, lower flag and change to IDLE
    case typing:
      loopTyping();
      break;
    // 4. RECEIVING: engage servo and move flag to 180, on message with text, move flag to 90 and change to READING
    case receiving:
      loopReceiving();
      break;
    // 5. READING: display the message, detach servo. on flag lowered, change to IDLE
    case reading:
      loopReading();
      break;
  }

  matrix.show();
  delay(50);
  M5.update();
}

// 1. IDLE: flag is moved to 180? send msg "wait for message", change to TYPING
// 2. IDLE: receive "wait for message", engage servo, change to RECEIVING
void loopIdle() {
  matrix.fillScreen(0);

  if (isFlagInTypingPosition()) {
    setCurrentState(typing);
    sendTypingToServerIo();
    return;
  }
  if (M5.Btn.wasPressed()) {
    moveServoToAngle(180);
  }
}

// 3. TYPING: handle morse code like you know, on flag raised to 90, send text, lower flag and change to IDLE
void loopTyping() {
  if (isFlagRaised() || morseMessageBuffer.substring(morseMessageBuffer.length() - 2) == "  ") {
    if (!morseMessageBuffer.isEmpty()) {
      sendTextToServerIo(morseMessageBuffer);
      morseMessageBuffer = "";
      matrix.fillScreen(blue);
      delay(200);
    }

    moveServoToAngle(0);
    delay(500);
    setCurrentState(idle);
    return;
  }

  matrix.fillScreen(0);
  parseMorse();
}

// 4. RECEIVING: engage servo and move flag to 180, on message with text, move flag to 90 and change to READING
void loopReceiving() {
  moveServoToAngle(180); // Also ensures that the servo cannot be turned by hand
}

// 5. READING: display the message, detach servo. on flag lowered, change to IDLE
void loopReading() {
  matrix.fillScreen(0);

  if (isFlagLowered() || M5.Btn.wasPressed()) {
    textScrollPosition = 0;
    if(M5.Btn.wasPressed()) {
      moveServoToAngle(0);
    }
    setCurrentState(idle);
    return;
  }
  if (isFlagInTypingPosition()) {
    textScrollPosition = 0;
    setCurrentState(typing);
    sendTypingToServerIo();
    return;
  }

  matrix.setTextColor(blue);

  matrix.setCursor(textScrollPosition + MATRIX_WIDTH, 0);
  matrix.print(receivedText);

  --textScrollPosition;

  // The scroll length needs to be 5 steps per character plus a spacing between
  // the end and the beginning of the next scroll iteration
  if(textScrollPosition < -(receivedText.length() * MATRIX_WIDTH + 30)) {
    textScrollPosition = 0;
  }

  delay(30); // Make the text scrolling a bit slower
}

void setCurrentState(State nextState) {
  Serial.print("Setting state to: ");
  Serial.println(String(nextState));

  currentState = nextState;
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

  switch (type) {
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
    parseReceivedSocketIoPayload(payload);
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

void parseReceivedSocketIoPayload(uint8_t *payload) {
  String payloadString = String((char *) payload);

  if (payloadString.indexOf("[\"message\"") == 0) {
    receivedText = payloadString.substring(12, payloadString.length() - 2);
    Serial.print("-> received message: ");
    Serial.println(receivedText);

    if (currentState == receiving) {
      moveServoToAngle(90);
      delay(500);
      setCurrentState(reading);
    }

    return;
  }

  if (payloadString.indexOf("[\"wait_for_message\"") == 0 && currentState == idle) {
    setCurrentState(receiving);

    return;
  }
}

// Parses the bit we received into dot or dash
void parseMorse() {
  if (M5.Btn.wasPressed()) {
    matrix.fillScreen(green);
  } else if (M5.Btn.pressedFor(200)) {
    isLongPress = true;
    matrix.fillScreen(red);

    if (M5.Btn.pressedFor(1000)) {
      matrix.fillScreen(yellow);
      isLongPress = false;
      isSpaceCharacter = true;
    }
  } else if (M5.Btn.wasReleased()) {
    if (isSpaceCharacter) {
      Serial.print("\n\n / \n\n");
      morseMessageBuffer = morseMessageBuffer + " ";
      characterCount = 0;
      characterIdentifier = 0;
      isLongPress = false;
      isSpaceCharacter = false;
      isTypingCharacter = false;
      return;
    } else if (isLongPress) {
      Serial.print("-");
      bitWrite(characterIdentifier, characterCount, 1);
    } else {
      Serial.print(".");
      bitWrite(characterIdentifier, characterCount, 0);
    }

    isLongPress = false;
    isSpaceCharacter = false;
    isTypingCharacter = true;
    characterCount++;

    matrix.fillScreen(0);
  } else if (isTypingCharacter && M5.Btn.releasedFor(500)) {
    isTypingCharacter = false;
    Serial.print(" ");
    bitWrite(characterIdentifier, characterCount, 1);
    Serial.print(characterIdentifier);
    Serial.print(" ");
    Serial.print(morse[characterIdentifier]);
    Serial.print(" ");
    Serial.println();

    char character = morse[characterIdentifier];

    matrix.setCursor(0, 0);
    matrix.print(character);

    morseMessageBuffer = morseMessageBuffer + String(character);

    characterCount = 0;
    characterIdentifier = 0;
  }
}

void sendTextToServerIo(String message) {
  String payload = "[\"message\", \"" + String(message) + "\"]";
  Serial.println(payload);

  socketIO.send(sIOtype_EVENT, payload);
}

void sendTypingToServerIo() {
  String payload = "[\"typing\", \"" + String(MY_NAME) + "\"]";
  Serial.println(payload);

  socketIO.send(sIOtype_EVENT, payload);
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
  servoDetachTimeout = 10;
}

void resetServo() {
  if (servo.attached() && servoDetachTimeout <= 0) {
    Serial.println("detaching servo");
    servo.detach();
  } else {
    servoDetachTimeout--;
  }
}

int flagPosition() {
  return analogRead(SERVO_SENSOR_PIN);
}

int flagAngle() {
  return 180.0 * (flagPosition() - minFlagPosition) / maxFlagPosition;
}

bool isFlagLowered() {
  return SERVO_WITH_FEEDBACK && flagAngle() < 30;
}

bool isFlagRaised() {
  return SERVO_WITH_FEEDBACK && flagAngle() > 80 && flagAngle() < 130;
}

bool isFlagInTypingPosition() {
  return SERVO_WITH_FEEDBACK && flagAngle() > 150;
}
