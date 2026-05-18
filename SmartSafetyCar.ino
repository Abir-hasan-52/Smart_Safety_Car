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
            gasAlert(); // now non-blocking
            return;
        }
        else
        {
            if (gasPreviouslyDetected)
            {
                // Gas cleared → resume
                BT.println("Gas cleared! Resuming...");
                gasPreviouslyDetected = false;
                safeMode();
                btCommand = 'S'; // wait for next BT command safely
            }
        }
    }
}