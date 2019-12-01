#include <Servo.h>
#include <SoftwareSerial.h>

/**
 * S - Stop
 * X - Set Speed
 * R - Turn Right
 * L - Turn Left
 * B - Go Back
 * F - Go Forward
 * W - Front Leds
 * H - Front Leds Long (high power)
 * A - Left-turn Leds
 * D - Right-turn Leds
 **/

int motor_2A = 4;
int motor_2B = 7;
int motor_1_PWM = 5;
int motor_2_PWM = 6;

#define pTrig 9
#define pEcho 10
#define LED_LEFT A0
#define LED_RIGHT A1
#define LED_FRONT 3
#define LED_REVERSE_LIGHT A3
#define LED_BACK 11
#define motor_1A A5
#define motor_1B A4

#define LED_BACK_LOW_POWER 30

//Rychlost blikani
const long interval = 500;

boolean LedRightRequest = false;
unsigned long previousMillisRight = 0;
int LED_RIGHT_STATE = LOW;

boolean LedLeftRequest = false;
unsigned long previousMillisLeft = 0;
int LED_LEFT_STATE = LOW;

boolean LedFrontState = false;
boolean LedFrontHIGHStare = false;

boolean stopState = false;
unsigned long previousMillisFront = 0;

boolean reverseState = false;

SoftwareSerial bluetooth(12,A7); //RX, TX
Servo servo;

int servoPin = 8;
int stredServa = 110;

int speed = 0;
int bezpecna_vzdalenost = 20;

long odezva, vzd_rovne, vzd_vpravo, vzd_vlevo;
bool jedu = false;
bool leva_prava = false; // kam se koukam, do leva nebo do prava
bool stopped = false;
int pocet_prekazek = 0;
char c[100];

void setup() {
  pinMode(motor_1A, OUTPUT);
  pinMode(motor_1B, OUTPUT);
  pinMode(motor_2A, OUTPUT);
  pinMode(motor_2B, OUTPUT);

  pinMode(LED_RIGHT, OUTPUT);
  pinMode(LED_LEFT, OUTPUT);
  pinMode(LED_REVERSE_LIGHT, OUTPUT);

  pinMode(pTrig, OUTPUT);
  pinMode(pEcho, INPUT);

  Serial.begin(9600);
  bluetooth.begin(9600);

  servo.attach(servoPin);
  kalibruj_servo();
  servo.detach();

  analogWrite(motor_1_PWM, speed);
  analogWrite(motor_2_PWM, speed);

  c[0] = 'S'; //Stop as default value
}

void loop() {
  vzd_rovne = getVzdalenost();

  //resetuji stopState a reserveState a nastavim si ho v kodu nize
  stopState = false;
  reverseState = false;
    
  bezpecna_vzdalenost = (speed / 10) + 25;
  int i = 0;

  if (bluetooth.available()) {
    while (bluetooth.available()) {
      c[i] = bluetooth.read();
      i += 1;
      if (c[0] != 'X') {
        break;
      }
    }
      
    Serial.print("Prijato : ");
    Serial.print(c[0]);
    Serial.println();

    //reset FrontLimis abych ziskal vice jak 2sec od currentMillis()
    previousMillisFront = 0;

    //tvrdy stop, nemam kam jet
    if (c[0] == 'F' && vzd_rovne < bezpecna_vzdalenost) { 
      Serial.println("Menim na S");
      c[0] = 'S';    
    } 
    
    switch (c[0]) {
      case 'X':
        speed = (String(c[1]).toInt() * 100) + (String(c[2]).toInt() * 10) + (String(c[2]).toInt()); 
        Serial.print("Speed : ");
        Serial.println(speed);
        analogWrite(motor_1_PWM, speed);
        analogWrite(motor_2_PWM, speed);
        break;
      case 'F':
        jed_dopredu();
        break;
      case 'S':
        zastav();
        stopState = true;
        break;
      case 'R':
        otoc_doprava();
        break;
      case 'L':
        otoc_doleva();
        break;
      case 'B':
        jed_dozadu();
        reverseState = true;
        break;
      case 'A':
        if (LedLeftRequest) {
          LedLeftRequest = false;
        }
        else {
          LedLeftRequest = true;
          LedRightRequest = false;
        }
        break;
      case 'D':
        if (LedRightRequest) {
          LedRightRequest = false;
        }
        else {
          LedRightRequest= true;
          LedLeftRequest = false;
        }
        break;
      case 'W':
        if (LedFrontState) {
          analogWrite(LED_FRONT, 0);
          LedFrontState = false;
        }
        else {
          analogWrite(LED_FRONT, 60);
          LedFrontState = true;
        }
        LedFrontHIGHStare = false;
        break;
      case 'H':
        if (LedFrontHIGHStare) {
          analogWrite(LED_FRONT, 0);
          LedFrontHIGHStare = false;
        }
        else {
          analogWrite(LED_FRONT, 255);
          LedFrontHIGHStare = true;
        }
        LedFrontState = false;
        break;
     }
  }  
  // Pokud nemam na BT nic, pak jedu posledni moznosti
  else {
    //tvrdy stop, nemam kam jet
    if (c[0] == 'F' && vzd_rovne < bezpecna_vzdalenost) 
    { 
        c[0] = 'S'; 
      Serial.println("Menim na S");
    }

    switch (c[0]) 
    {
      case 'F':
        jed_dopredu();
        break;
      case 'S':
        zastav();
        stopState = true;
        break;
      case 'R':
        otoc_doprava();
        break;
      case 'L':
        otoc_doleva();
        break;
      case 'B':
        jed_dozadu();
        reverseState = true;
        break;
    }
  }

  //Nastaveni vykonu brzdovych svetel
  if (stopState) {
    // pokud trva dlouho, tak jdu na nizsi vykon LED
    unsigned long currentMillis = millis();
    //set previousMillisFront if more than 5sec.
    if (currentMillis - previousMillisFront > 5000) {
      previousMillisFront = currentMillis;
    }
    //if more than 1sec go to low power, simulate that braking is gone
    else if (currentMillis - previousMillisFront > 1500 && currentMillis - previousMillisFront < 5000)  
    {
      if (LedFrontState) {
        analogWrite(LED_BACK, LED_BACK_LOW_POWER);
      }
      else {
        analogWrite(LED_BACK, 0);
      }
      previousMillisFront = currentMillis - 1500;
    }
    //If less than 1 sec go full brake-power
    else {
      analogWrite(LED_BACK, 255);
    }
  }
  else if (LedFrontState) {
    analogWrite(LED_BACK, LED_BACK_LOW_POWER);
  }
  else {
    analogWrite(LED_BACK, 0);
  }

  //Nastaveni svetel zpratecky
  if (reverseState) {
    digitalWrite(LED_REVERSE_LIGHT, HIGH);
  }
  else {
    digitalWrite(LED_REVERSE_LIGHT, LOW);
  }

  // Blikani levou
  if (LedLeftRequest) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisLeft > interval) {
      previousMillisLeft = currentMillis;
      if (LED_LEFT_STATE == LOW) {
        LED_LEFT_STATE = HIGH;
      }
      else {
        LED_LEFT_STATE = LOW;
      }
      digitalWrite(LED_LEFT, LED_LEFT_STATE);
      // neblikal jsem pravou?
      digitalWrite(LED_RIGHT, LOW);
    }
  }
  // pokud nechci blikat, tak tam navtrdo poslu LOW
  else {
    digitalWrite(LED_LEFT, LOW);
  }

  //Blikani pravou
  if (LedRightRequest) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisRight > interval) {
      previousMillisRight = currentMillis;
      if (LED_RIGHT_STATE == LOW) {
        LED_RIGHT_STATE = HIGH;
      }
      else {
        LED_RIGHT_STATE = LOW;
      }
      digitalWrite(LED_RIGHT, LED_RIGHT_STATE);
    }
  }
  // pokud nechci blikat, tak tam navtrdo poslu LOW
  else {
    digitalWrite(LED_RIGHT, LOW);
  }
}

/*
void jedAutomaticky() {
  if ((jedu) || (vzd_rovne < 7) && (vzd_rovne > 0)) {

    if (!jedu) {
      delay(1000); //davam cas na odstraneni nohy pred autem
      vzd_rovne = getVzdalenost(); // nactu si realnou vzd_rovne "bez nohy"
    }
    
    jedu = true;
    analogWrite(motor_1_PWM, speed);
    analogWrite(motor_2_PWM, speed);
    
    if (vzd_rovne > 30) { //Pokud je vzd_rovne > 25 tak jedu dopredu
      jed_dopredu();
      if (leva_prava) { // podle leva_prava se podivam bud do leva nebo do prava, oboje nestiham
        koukni_Vlevo();
        vzd_vlevo = getVzdalenost();
        if (vzd_vlevo < vzd_rovne + 10 || vzd_vlevo < 30) {
          zastav();
          otoc_doprava();
        }
        leva_prava = false;;
      }
      else {
        koukni_Vpravo();
        vzd_vpravo = getVzdalenost();
        if (vzd_vpravo < vzd_rovne + 10 || vzd_vpravo < 30) {
          zastav();
          otoc_doleva();
        }
        leva_prava = true;
      }
      servoStred(); // dorovnam Servo na stred
    } 
    else {
      //Pokude je vzd_rovne < 25 pak zastavim a trochu couvnu
      stopAndBack();
      pocet_prekazek = pocet_prekazek + 1;
      
      if (pocet_prekazek > 5) {
        jedu = false;
        pocet_prekazek = 0;
        rekniNE();
      }
      else {
        koukni_Vlevo();
        vzd_vlevo = getVzdalenost();

        koukni_Vpravo();
        vzd_vpravo = getVzdalenost();
        
        servoStred();

        if (vzd_vlevo >= vzd_vpravo) {
          otoc_doprava();
          delay(300);
        }
        else {
          otoc_doleva();
          delay(300);
        }
      }
    }
  }
}*/

long getVzdalenost() {
      digitalWrite(pTrig, HIGH);
      delayMicroseconds(2);
      digitalWrite(pTrig,LOW);
      delayMicroseconds(5);
      odezva = pulseIn(pEcho, HIGH);
      vzd_rovne = odezva / 58.31;
      return vzd_rovne;
}

void stopAndBack() {
      zastav();
      analogWrite(motor_1_PWM, 100);
      analogWrite(motor_2_PWM, 100);
      jed_dozadu();
      delay(300);  
      analogWrite(motor_1_PWM, speed);
      analogWrite(motor_2_PWM, speed); 
      zastav();
}

void jed_dopredu() {
    digitalWrite(motor_1A, HIGH);
    digitalWrite(motor_1B, LOW);
    digitalWrite(motor_2A, HIGH);
    digitalWrite(motor_2B, LOW);
}

void jed_dozadu() {
    digitalWrite(motor_1A, LOW);
    digitalWrite(motor_1B, HIGH);
    digitalWrite(motor_2A, LOW);
    digitalWrite(motor_2B, HIGH);
}

void zastav() {
    digitalWrite(motor_1A, LOW);
    digitalWrite(motor_1B, LOW);
    digitalWrite(motor_2A, LOW);
    digitalWrite(motor_2B, LOW);
}

void otoc_doleva() {
    digitalWrite(motor_1A, HIGH);
    digitalWrite(motor_1B, LOW);
    digitalWrite(motor_2A, LOW);
    digitalWrite(motor_2B, HIGH);
}

void otoc_doprava() {
    digitalWrite(motor_1A, LOW);
    digitalWrite(motor_1B, HIGH);
    digitalWrite(motor_2A, HIGH);
    digitalWrite(motor_2B, LOW);
}

void servoStred() {
  servo.write(stredServa);
  delay(300);
}

void koukni_Vpravo(){
  servo.write(stredServa + 60);
  delay(400);
}

void mrkni_Vpravo() {
  servo.write(stredServa + 40);
  delay(300);
}

void mrkni_Vlevo() {
  servo.write(stredServa - 40);
  delay(300);
}

void koukni_Vlevo() {
  servo.write(stredServa - 60);
  delay(400);
}

void kalibruj_servo() {
  servo.write(0);
  delay(1000);
  servo.write(180);
  delay(1000);
  servoStred();
}

void rekniNE() {
  servo.attach(servoPin);
  servo.write(130);
  delay(300);
  servo.write(50);
  delay(300);
  servo.write(130);
  delay(300);
  servo.write(50);
  delay(300);
  servo.write(90);
  delay(300);
  servo.detach();
}
