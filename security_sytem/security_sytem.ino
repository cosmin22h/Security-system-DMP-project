#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

// pini
const int pinButton = 13; // Buton activare alarma manual
const int pinBuzzer = 12; // Buzzzer
const int pinTrig = 11;   // Pin-ul trig pentru senzorul ultrasonic
const int pinEcho = 10;   // Pin-ul echo pentru senzorul ultrasonic
const int ledR = 9;       // led-ul rosu 
const int ledY = 8;       // led-ul verde
const int ledG = 7;       // led-ul galben
const int laserPin = 4;   // laser

// flags
int bottomState;                // stare buton
bool systemOnline = false;      // sistem activ
bool alarmTriggered = false;    // alarma declansata
bool callPolice = false;        // apelare politie
int counterWrongPass = 3;       // counter incercari

String currPassword = "0000";   // parola
int frqAlarm = 1000;            // frecventa buzzer

int counterSystemOn = 5;        // nr. secunde pana la activarea sistemului
int currDistance;               // distanta masurata de senzorul ultrasonic

// keypad
char key; // tasta apasata de pe keypad
char keyMap[4][4] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
}; // butoane keypad
byte rowPins[4] = {3, 2, 14, 15}; // pinii de iesire corespunzatori randurilor keypad-ului
byte colPins[4] = {16, 17, 18, 19}; // pinii de iesire corespunzatori coloanelor keypad-ului

Keypad myKeypad = Keypad(makeKeymap(keyMap), rowPins, colPins, 4, 4); // keypad
LiquidCrystal_I2C lcd(0x27, 20, 2);                                   // LCD

void setup() {
  pinMode(pinBuzzer, OUTPUT); 
  pinMode(pinButton, INPUT); 
  pinMode(pinEcho, INPUT);
  pinMode(pinTrig, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(ledY, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(laserPin, OUTPUT);
 
  lcd.init();
  lcd.backlight();
}

void loop() {

  if (!systemOnline) {
    lcd.setCursor(0,0);
    lcd.print("A -  ALARM ON");
    lcd.setCursor(0,1);
    lcd.print("C -  SET PASS");
    digitalWrite(ledR, LOW);
    digitalWrite(ledY, HIGH);
    digitalWrite(ledG, LOW);
    digitalWrite(laserPin, LOW);
    key =  myKeypad.getKey();
    if (key == 'A') {
      lcd.clear();
      systemOnline = true;
      counterSystemOn = 5;
    } else if (key == 'C') {
      tone(pinBuzzer, frqAlarm); // alarma activata
      enterPassword();
      enterNewPassword();
    }
  }
  else {
    if (counterSystemOn > 0) {
      lcd.setCursor(0,0);
      lcd.print("Alarm ready in: ");
      lcd.setCursor(0,1);
      lcd.print(counterSystemOn);
      lcd.print(" seconds");
      counterSystemOn--;
      delay(1000);
    } else {
        if (counterSystemOn == 0) {
          lcd.clear();
          counterSystemOn = -1;
        }
        if (!alarmTriggered && !callPolice) {
        digitalWrite(ledR, LOW);
        digitalWrite(ledY, LOW);
        digitalWrite(ledG, HIGH);
        digitalWrite(laserPin, HIGH);
        lcd.setCursor(0,0);
        lcd.print("B -  ALARM OFF");
        lcd.setCursor(0,1);
        lcd.print("C -  SET PASS");
        // declansare alarma - buton
        bottomState = digitalRead(pinButton);
        if (bottomState == HIGH) {
          alarmTriggered = true;
        }
        // declansare alarma - senzor ultrasonic
        currDistance = getDistance();
        if (currDistance > 0 && currDistance < 10) {
          alarmTriggered = true;
        }
        // declansare alarma - senzor LDR
        if (analogRead(A0) > 500) {
          alarmTriggered = true;
        }
      }
      // alarma declansata
      if (alarmTriggered) {
        tone(pinBuzzer, frqAlarm); // alarma activata
        enterPassword();
      }
      // dezactivare sistem
      key =  myKeypad.getKey();
      if (key == 'B') {
        tone(pinBuzzer, frqAlarm); // alarma activata
        enterPassword();
      } else if (key == 'C') {
        tone(pinBuzzer, frqAlarm); // alarma activata
        enterPassword();
        enterNewPassword();
      }
    }
  }
  // apelare politie
  if (callPolice) {
    policeIncoming();
  }
}

// introducere parola
void enterPassword() {
  digitalWrite(ledR, HIGH);
  digitalWrite(ledY, LOW);
  digitalWrite(ledG, LOW);
  counterWrongPass = 3;
  bool pass = false; // flag parola introdusa corect
  bool again = true;
  String password = ""; // parola introdusa
  int i = 0; // contor caractere introduse
  while (!pass && !callPolice) {
    if (again) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("!!ALARM!!");
      lcd.setCursor(0,1);
      lcd.print("PASS>");
      again = false;
    }
    key =  myKeypad.getKey();
    if (key != NO_KEY) {
      if (key >= '0' && key <= '9') { // s-a apasat un key numeric
      password += key;
      lcd.setCursor(5 + i ,1);
      lcd.print(key);
      i++;
      }
      if (i > 4) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Pass is too");
        lcd.setCursor(0,1);
        lcd.print("long!");
        delay(3000);
        again = true;
        password = "";
        i = 0;
      }
      // resetare
      if (key == '*') {
        again = true;
        password = ""; 
        i = 0;
      }
      // enter
      if (key == '#') { 
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Pass - checking");
        lcd.setCursor(0,1);
        lcd.print("...");
        delay(2000);
        if (password == currPassword) {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("CORRECT");
          delay(1000);
          pass = true;
          alarmTriggered = false;
          noTone(pinBuzzer);
          systemOnline = false;
        }
        else if (password != currPassword) {
          counterWrongPass--;
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("WRONG");
          lcd.setCursor(0,1);
          lcd.print("Attempts: ");
          lcd.print(counterWrongPass);
          delay(1500);
          frqAlarm += 1000;
          tone(pinBuzzer, frqAlarm); 
          again = true;
          password = "";
          i = 0;
        }
      }
    }
    if (counterWrongPass == 0) {
      callPolice = true;
      alarmTriggered = false;
    }
    if (callPolice) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("call police...");
    }
  }
}

// introducere parola noua
void enterNewPassword() {
  String newPassword = "";
  bool confirmNewPass = false;
  bool again = true;
  int i = 0; 
  while (!confirmNewPass) {
    if (again) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("SET PASS");
      lcd.setCursor(0,1);
      lcd.print("NEW PASS>");
      again = false;
    }
    key =  myKeypad.getKey();
    if (key != NO_KEY) {
       if (key >= '0' && key <= '9') {
        newPassword += key;
        lcd.setCursor(9 + i,1);
        lcd.print(key);
        i++;
       }
       if (i > 4) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Pass is too");
        lcd.setCursor(0,1);
        lcd.print("long!");
        delay(3000);
        again = true;
        newPassword = "";
        i = 0;
       }
       if (key == '*') {
        again = true;
        newPassword = "";
        i = 0;
       }
       if (key == '#') {
		lcd.clear();
		lcd.setCursor(0,0);
    if (i < 4) {
			lcd.print("TOO SHORT");
			delay(1000);
      newPassword = "";
      i = 0;
			again = true;
		}
		else {
			lcd.print("NEW PASS");
			delay(1500);
			currPassword = newPassword;
			confirmNewPass = true;
		}
       }
    }
  }
}

// sunet sirena de politie
void policeIncoming() {
  for(int i = 700; i < 800; i++){
    tone(pinBuzzer, i);
    delay(15);
  }
  digitalWrite(ledR, LOW);
  for(int i = 800; i > 700; i--){
    tone(pinBuzzer, i);
    delay(15);
  }
  digitalWrite(ledR, HIGH);
}

// distanta masurata de senzorul ultrasonic
int getDistance(){
  digitalWrite(pinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrig, LOW);
  long duration = pulseIn(pinEcho, HIGH);
  int distance = (int) duration * 0.034 / 2;
  return distance;
}
