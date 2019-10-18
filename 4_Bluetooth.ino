#include <Servo.h>
#include <SoftwareSerial.h>

int motor_1A = 2;
int motor_1B = 3;
int motor_2A = 4;
int motor_2B = 7;
int motor_1_PWM = 5;
int motor_2_PWM = 6;

#define pTrig 9
#define pEcho 10

SoftwareSerial bluetooth(11,12); //RX, TX
Servo servo;

int servoPin = 8;
int stredServa = 110;

int speed = 0;
int bezpecna_vzdalenost = 20;

long odezva, vzd_rovne, vzd_vpravo, vzd_vlevo;
bool jedu = false;
bool leva_prava = false; // kam se koukam, do leva nebo do prava
int pocet_prekazek = 0;
char c[100];

void setup() {
  pinMode(motor_1A, OUTPUT);
  pinMode(motor_1B, OUTPUT);
  pinMode(motor_2A, OUTPUT);
  pinMode(motor_2B, OUTPUT);
  pinMode(motor_1_PWM, OUTPUT);
  pinMode(motor_2_PWM, OUTPUT);

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
    
  bezpecna_vzdalenost = (speed / 10) + 25;
  int i = 0;

  if (bluetooth.available()) {
    while (bluetooth.available()) {
      c[i] = bluetooth.read();
      i += 1;
    }
      
    Serial.print("Prijato : ");
    Serial.print(c[0]);
    Serial.println();

    if (c[0] == 'F' && vzd_rovne < bezpecna_vzdalenost) { 
      Serial.println("Menim na S");
      c[0] = 'S'; 
    } //tvrdy stop, nemam kam jet*/
    
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
        break;
      case 'R':
        otoc_doprava();
        break;
      case 'L':
        otoc_doleva();
        break;
      case 'B':
        jed_dozadu();
        break;
     }
  }  
    // Pokud nemam na BT nic, pak jedu posledni moznosti
    else {
      if (c[0] == 'F' && vzd_rovne < bezpecna_vzdalenost) { 
        c[0] = 'S'; 
        Serial.println("Menim na S");
      } //tvrdy stop, nemam kam jet

    switch (c[0]) {
        case 'F':
          jed_dopredu();
          break;
        case 'S':
          zastav();
          break;
        case 'R':
          otoc_doprava();
          break;
        case 'L':
          otoc_doleva();
          break;
        case 'B':
          jed_dozadu();
          break;
      }
    }
  delay(200);
  //zkus snizit, mam pocit ze to pak blbne
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
      /*
       Serial.print("Vzdalenost je: ");
      Serial.print(vzd_rovne);
      Serial.println();
      */
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
  servo.write(130);
  delay(400);
  servo.write(50);
  delay(400);
  servo.write(130);
  delay(400);
  servo.write(50);
  delay(400);
  servo.write(130);
  delay(400);
  servo.write(90);
  delay(400);
}