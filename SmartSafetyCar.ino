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
