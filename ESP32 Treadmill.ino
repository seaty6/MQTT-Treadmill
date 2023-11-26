//  * MAX 485: 5v to DE, D8 to DI. or the corresponding tx pin on your board.  A and B to A and B of the treadmill controller
//  * Short out the middle "safety key" pins 
//  * Take power from one of the treadmill controller pins (which one?)
//  * Treadmill header has 8 pins (left to right, latch at the bottom):
//  * 1 - 12v
//  * 2 - 12v
//  * 3 - A or B?
//  * 4 - Safety switch
//  * 5 - Safety switch
//  * 6 - A or B?
//  * 7 - Ground
//  * 8 - Ground

#include <WiFi.h>
#include <MQTT.h>
#include <HardwareSerial.h>
#include <SimpleTimer.h>

const char ssid[] = "SE_Wifi";
const char pass[] = "just-321";

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;

HardwareSerial ss(2); // Using Serial1 for communication
SimpleTimer timer;

bool halt = false;
int setSpeed = 0;
int curSpeed = 0; //ramp this up until it reaches the set speed
int savedSpeed = 0;
const int speedCount = 123;

const byte b1[] = {0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,13,13,13,13,13,13,13,14,14,14,14,14,14,14,14,15,15,15,15,15,15,15,16,16,16,16,16,16,16,16};
const byte b2[] = {0,161,195,230,8,42,76,111,145,179,213,248,26,60,94,129,163,197,231,10,44,78,112,147,181,215,249,28,62,96,130,165,199,233,11,46,80,114,148,183,217,251,29,64,98,132,166,200,235,13,47,81,116,150,184,218,253,31,65,99,134,164,202,236,15,49,83,117,152,186,220,254,33,67,101,135,170,204,238,16,51,85,119,153,188,222,0,34,101,103,137,171,206,240,18,52,87,121,155,173,224,2,36,70,105,139,173,207,242,20,54,88,123,157,191,225,4,20,54,88,123,157,191,225};
const byte b3[] = {221,16,201,186,144,116,105,188,33,197,216,18,156,188,101,79,171,182,82,43,11,210,8,154,186,99,250,96,132,229,70,87,142,23,64,51,212,48,87,130,38,194,136,186,94,57,221,121,172,63,219,60,79,236,117,172,189,135,230,2,54,175,118,86,112,234,51,19,158,122,103,131,132,93,125,222,20,9,237,132,81,76,168,118,5,220,105,141,161,69,139,127,49,235,188,156,116,237,78,110,92,210,242,43,131,32,0,217,80,195,39,131,86,49,213,180,119,195,39,131,86,49,213,180};
const int sfm[] = {0,262,314,366,418,471,523,575,628,680,732,785,837,889,941,994,1046,1098,1151,1203,1255,1308,1360,1412,1464,1517,1569,1621,1674,1726,1778,1831,1883,1935,1987,2040,2092,2144,2197,2249,2301,2355,2406,2458,2510,2563,2615,2667,2721,2772,2824,2878,2929,2981,3033,3087,3138,3190,3244,3295,3347,3401,3453,3504,3556,3610,3661,3713,3767,3818,3870,3924,3976,4027,4079,4133,4184,4236,4290,4342,4393,4447,4499,4550,4602,4656,4708,4759,4813,4865,4916,4970,5022,5074,5125,5179,5231,5282,5336,5388,5440,5493,5545,5597,5648,5702,5754,5805,5859,5911,5963,6016,6068,6120,6171,6225,6277,6329,6382,6434,6486,6539,6591,6643};


void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("treadmill", "mqttlogin", "mqttpassword")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  client.subscribe("treadmill/speed");
  // client.unsubscribe("/hello");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  // setSpeed = (payload.toFloat() * 10); // Read speed from Serial
  // setSpeed
  setSpeed = payload.toInt(); // Read speed from Serial
  setSpeed = constrain(setSpeed, 0, speedCount); // Constrain the speed value
  Serial.println("Set Speed: " + String(setSpeed)); // Print the set speed
  halt = false;


  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

void setup() {
  Serial.begin(9600);
  ss.begin(9600);
  timer.setInterval(100, sendSpeedPacket);


  WiFi.begin(ssid, pass);

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  client.begin("192.168.50.96", net);
  client.onMessage(messageReceived);

  connect();
}

void loop() {
  timer.run();
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }

  // publish a message roughly every second.
  if (millis() - lastMillis > 1000) {
    lastMillis = millis();
    // client.publish("/hello", "world");
  }
}

void sendSpeedPacket() {
  Serial.println("Sending Speed Packet");
  if(halt == true) { //send the stop packet
    ss.write((byte)0);
    ss.write((byte)255);
    ss.write((byte)241);
    ss.write((byte)2);
    ss.write((byte)0);
    ss.write((byte)0);
    ss.write((byte)221);
  }
  else { //send the current speed packet
    ss.write((byte)0);
    ss.write((byte)255);
    ss.write((byte)241);
    ss.write((byte)2);
    ss.write((byte)b1[curSpeed]);
    ss.write((byte)b2[curSpeed]);
    ss.write((byte)b3[curSpeed]);   
  }

  if (halt == true){
    curSpeed = 0;
  }

  if (curSpeed < setSpeed && halt == false) { //this ramps the sent speed up until it hits the set speed
    curSpeed++;
  }
  if (curSpeed > setSpeed && halt == false) { //this immediately reduces the sent speed to the set speed
    curSpeed = setSpeed;
  }

  Serial.print(halt);
  Serial.print("Current Speed: ");
  Serial.println(curSpeed); // Print the current speed
  
}
