#include <SoftwareSerial.h>

// ── Bluetooth
SoftwareSerial BT(10, 11);

// ── Motor Pins
#define ENA 5
#define IN1 2
#define IN2 3
#define IN3 4
#define IN4 7
#define ENB 6

// ── Ultrasonic
#define TRIG 8
#define ECHO 9

// ── IR Sensor
#define IR_FRONT A0

// ── Gas Sensor
#define GAS_PIN A2
#define GAS_THRESHOLD 400

// ── Buzzer & LEDs
#define BUZZER 12
#define LED_RED A3
#define LED_GREEN A4

// ── Variables
int motorSpeed = 150;
bool manualOverride = false;
char btCommand = 'S';

unsigned long lastWarningTime = 0;
bool warningBuzzerState = false;

bool gasPreviouslyDetected = false;

void setup()
{
    pinMode(ENA, OUTPUT);
    pinMode(ENB, OUTPUT);
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);
    pinMode(IR_FRONT, INPUT);

    pinMode(BUZZER, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);

    Serial.begin(9600);
    BT.begin(9600);

    // Startup blink
    digitalWrite(LED_GREEN, HIGH);
    delay(300);
    digitalWrite(LED_GREEN, LOW);
    BT.println("Smart Safety Car Ready!");
}

void loop()
{

    // Bluetooth read
    if (BT.available())
    {
        btCommand = BT.read();
        handleBluetooth(btCommand);
    }

    long distance = getUltrasonicDistance();
    int gasValue = analogRead(GAS_PIN);
    bool irDetect = digitalRead(IR_FRONT);

    Serial.print("Dist: ");
    Serial.print(distance);
    Serial.print(" | Gas: ");
    Serial.print(gasValue);
    Serial.print(" | IR: ");
    Serial.println(irDetect);

    if (!manualOverride)
    {
        if (gasValue > GAS_THRESHOLD)
        {
            if (!gasPreviouslyDetected)
            {
                BT.println("GAS DETECTED! Car Stopped.");
                gasPreviouslyDetected = true;
            }
            stopMotors();
            gasAlert();
            return;
        }
        else
        {
            if (gasPreviouslyDetected)
            {
                // Gas cleared - resume
                BT.println("Gas cleared! Resuming...");
                gasPreviouslyDetected = false;
                safeMode();
                btCommand = 'S'; // wait for next BT command safely
            }
        }
        if (irDetect == LOW)
        {
            BT.println("IR Obstacle!");
            autoEscape();
            return;
        }

        //   skip -1 (invalid) readings 
        if (distance != -1 && distance < 15)
        {
            autoEscape();
            return;
        }

        //  Warning zone 
        if (distance != -1 && distance < 30)
        {
            warningAlert();   non-blocking
        }
        else
        {
            safeMode();
        }
    }
    executeMovement(btCommand);
}

void autoEscape()
{
    BT.println("OBSTACLE! Escaping...");

    stopMotors();
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    escapeBeep();
    delay(300);

    // Reverse
    BT.println("Reversing...");
    moveBackward();
    delay(800);
    stopMotors();
    delay(300);

    // Try Left
    BT.println("Trying Left...");
    turnLeft();
    delay(600);
    stopMotors();
    delay(300);

    long frontDist = getUltrasonicDistance();
    bool irClear = digitalRead(IR_FRONT);

    if ((frontDist > 30 || frontDist == -1) && irClear == HIGH)
    {
        BT.println("Left clear! Moving forward.");
        safeMode();
        //   only set btCommand if manual override is OFF
        if (!manualOverride)
        {
            btCommand = 'F';
        }
        return;
    }

    // Try Right
    BT.println("Left blocked! Trying Right...");
    moveBackward();
    delay(500);
    stopMotors();
    delay(200);

    turnRight();
    delay(1200);
    stopMotors();
    delay(300);

    frontDist = getUltrasonicDistance();
    irClear = digitalRead(IR_FRONT);

    if ((frontDist > 30 || frontDist == -1) && irClear == HIGH)
    {
        BT.println("Right clear! Moving forward.");
        safeMode();
        //  only set btCommand if manual override is OFF
        if (!manualOverride)
        {
            btCommand = 'F';
        }
        return;
    }

    // Completely stuck
    BT.println("Completely stuck! Manual needed.");
    stopMotors();
    stuckAlert();
    btCommand = 'S';
}

//  BLUETOOTH HANDLER

void handleBluetooth(char cmd)
{
    switch (cmd)
    {
    case 'H':
        motorSpeed = 220;
        BT.println("Speed: HIGH");
        break;
    case 'M':
        motorSpeed = 150;
        BT.println("Speed: MEDIUM");
        break;
    case 'L':
        motorSpeed = 100;
        BT.println("Speed: LOW");
        break;
    case 'X':
        manualOverride = !manualOverride;
        BT.println(manualOverride ? "Override: ON" : "Override: OFF");
        if (!manualOverride)
            safeMode();
        break;
    }
}


 
//  MOVEMENT
 
void executeMovement(char cmd) {
  switch (cmd) {
    case 'F': moveForward();  break;
    case 'B': moveBackward(); break;
    case 'L': turnLeft();     break;
    case 'R': turnRight();    break;
    case 'S': stopMotors();   break;
  }
}

void moveForward() {
  analogWrite(ENA, motorSpeed); analogWrite(ENB, motorSpeed);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void moveBackward() {
  analogWrite(ENA, motorSpeed); analogWrite(ENB, motorSpeed);
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void turnLeft() {
  analogWrite(ENA, motorSpeed); analogWrite(ENB, motorSpeed);
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void turnRight() {
  analogWrite(ENA, motorSpeed); analogWrite(ENB, motorSpeed);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void stopMotors() {
  analogWrite(ENA, 0); analogWrite(ENB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}


 
//   IMPROVED ULTRASONIC
//  returns -1 if reading is invalid
 
long getUltrasonicDistance() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);

  // duration == 0 means no echo received
  if (duration == 0) return -1;

  long distance = duration * 0.034 / 2;

  // Sanity check: HC-SR04 valid range is 2cm – 400cm
  if (distance < 2 || distance > 400) return -1;

  return distance;
}


 
//  ALERTS
 
//  Non-blocking warningAlert using millis()
void warningAlert() {
  unsigned long now = millis();
  digitalWrite(LED_GREEN, LOW);

  if (now - lastWarningTime >= 100) {   // toggle every 100ms
    lastWarningTime    = now;
    warningBuzzerState = !warningBuzzerState;
    digitalWrite(BUZZER,  warningBuzzerState ? HIGH : LOW);
    digitalWrite(LED_RED, warningBuzzerState ? HIGH : LOW);
  }
}

//  Non-blocking gasAlert using millis()
void gasAlert() {
  static unsigned long lastGasBuzz = 0;
  static bool gasBuzzState         = false;
  unsigned long now                = millis();

  digitalWrite(LED_RED,   HIGH);
  digitalWrite(LED_GREEN, LOW);

  if (now - lastGasBuzz >= 80) {
    lastGasBuzz  = now;
    gasBuzzState = !gasBuzzState;
    digitalWrite(BUZZER, gasBuzzState ? HIGH : LOW);
  }
}

void escapeBeep() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER, HIGH); delay(120);
    digitalWrite(BUZZER, LOW);  delay(80);
  }
}

void stuckAlert() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER, HIGH); delay(400);
    digitalWrite(BUZZER, LOW);  delay(200);
  }
  digitalWrite(LED_RED, HIGH);
}

void safeMode() {
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED,   LOW);
  digitalWrite(BUZZER,    LOW);
}

