# Morsey-Talky

This is the result of a fun hackweek project at Xing.

We're using an [M5Atom Matrix](https://shop.m5stack.com/collections/m5-atom/products/atom-matrix-esp32-development-kit) development board to build a distributed Morse telegraph.
Each device connects to a central SocketIO server.
Then, one person can type a message using morse code and it will get displayed to all others.
If you connect a servo as described below and connect a little flag to it, it will also raise the flag to indicate the state you're currently in.

## Hardware

You will need:

* [M5-ATOM Matrix](https://shop.m5stack.com/collections/m5-atom/products/atom-matrix-esp32-development-kit)
* A servo motor with a feedback wire (feedback is optional)
* USB-C cable to connect the M5-ATOM to a power source
* A flag you can build yourself and glue/attach to the arm of the servo

### Wiring

* Servo GND to M5-ATOM GND pin
* Servo POWER to M5-ATOM 3V3 pin
* Servo DATA to M5-ATOM G22 pin
* Servo FEEDBACK to M5-ATOM G33 pin

## Libraries

### Board support

Install the required board manager following the instructions [here](https://docs.m5stack.com/en/arduino/arduino_development).

### Libraries to install

You will need to install the following libraries using your Arduino IDE:

* [M5Atom](https://github.com/m5stack/M5Atom)
* [WebSocketsClient](https://www.arduino.cc/reference/en/libraries/websockets/)
* [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)
* [Adafruit NeoMatrix](https://github.com/adafruit/Adafruit_NeoMatrix)
* [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
* [ESP32Servo](https://www.arduino.cc/reference/en/libraries/esp32servo/)

## Server application

A very basic server application written in Node.js is available at [this other repository](https://github.com/Marta83/socketio_server), with all the instructions to set it up and deploy it on the server of your choice. We tested it successfully deployed on a machine in our local network and on Heroku.

## Configuration

Before being able to compile the source code, copy the `config.h.sample` file to a new `config.h` file and edit the values to match your setup.

* `WIFI_SSID` and `WIFI_PASSWORD` will be used to connect to your WiFi router
* `SOCKET_IO_HOST` and `SOCKET_IO_PORT` should match the host and port wher the server application is available
* `MY_NAME` will be displayed before all the messages that you send
* `SERVO_WITH_FEEDBACK` should be set to `true` unless your servo motor doesn't have a working feedback wire

## How it works

(open a serial monitor if you need a verbose output)

### Starting up

1. Connect the M5-ATOM to a power source
2. The flag (i.e. the arm of the servo) will rotate to 180 degrees and the display will start a blue "breathing" animation, signalling that it is connecting to WiFi
3. The flag rotates back to 0 degrees, signaling that the device is ready to accept commands.

### Sending a message

1. Either press the button or move the flag 180 degrees to signal the other devices connected to the server that you are about to send a message. The flags of all the other devices will rotate to 180 degrees.
2. Type your message in morse code (you will see a preview of each character once you stop typing). Short presses turn the screen green and long presses turn the screen red. A longer press enters a space.
3. Either turn the flag 90 degrees or enter two spaces to send the message. The flags of all the other devices will also turn 90 degrees and start displaying the message.
4. Press the button or turn the flag to return to idle mode.

## Credits
- [Nicolas Fricke](https://github.com/nicolas-fricke)
- [Marta Ruiz](https://github.com/Marta83)
- [Jesús López](https://github.com/jesuslopezxing)
- [Brian Acosta](https://github.com/briandorian)
- [Aaron Ciaghi](https://github.com/aaronsama)
- [Sean Daryanani](https://github.com/seand52)
