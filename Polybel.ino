#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>

//Pin de liason serie
#define SS_PIN 10
#define RST_PIN 9

//Define moteurs
#define MOT_AV 110
#define MOT_AR 70
#define MOT_STOP 90
#define PORT_MOT1 6
#define PORT_MOT2 7
#define PORT_MOT3 9

//Define RFID
#define TAILLE_DATAB 9

//Leds
#define LED_ROUGE 2
#define LED_VERTE 3
#define LED_JAUNE 4

//Bouton
#define BOUTON_1 A0
#define BOUTON_2 A1
#define BOUTON_3 A2

//Variables globales
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
Servo servoTrappe;

int SensMoteur;
long cmptMot;
long cmptLed;
int etat;
int onPressBouton;

//Variables global RFID
String tagID;
String database[TAILLE_DATAB]= {
//Plastique
"B1 62 CA 20",
"B1 2E 90 20",
"B1 D6 98 20",
//Carton
"C1 0C 1B 20",
"03 AD 57 21",
"BE 40 1F 14",
//Aluminium
"B1 9A 66 20",
"B1 03 35 20",
"B1 1D 70 20"
};


void setupCompteur()
{
  //Set up timer2
  cli(); //close global interruption
  bitClear(TCCR2A, WGM20);
  bitClear(TCCR2A, WGM21);
  TCCR2B = 0b00000110; //prescaller 256 de 16Mhz = 1.6us par incrementation du registre
  TIMSK2 = 0b00000001; //Interruption de debodement de T2 enable
  sei();//set global interruption 

  //Set up compteurs
  cmptMot=0;
  cmptLed=0;
}

void setupMoteur()
{
 SensMoteur = MOT_STOP;
 servoTrappe.attach(2);
 servoTrappe.write(SensMoteur);
 servoTrappe.detach();
 servoTrappe.attach(7);
 servoTrappe.write(SensMoteur);
 servoTrappe.detach();
 servoTrappe.attach(8);
 servoTrappe.write(SensMoteur);
 servoTrappe.detach();
}

void setupRFID()
{
  SPI.begin();
  mfrc522.PCD_Init();   // Initiate MFRC522
  mfrc522.PCD_DumpVersionToSerial(); //Affiche les info de la biblio
}

void setupBouton(){
    pinMode(BOUTON_1,INPUT);
    pinMode(BOUTON_2,INPUT);
    pinMode(BOUTON_3,INPUT);
}

void setup()
{
 Serial.begin(9600);

 setupCompteur();
 setupMoteur();
 setupRFID();
 setupBouton();
 
 //Leds
 pinMode(LED_ROUGE,OUTPUT);
 pinMode(LED_VERTE,OUTPUT);
 pinMode(LED_JAUNE,OUTPUT);
 
 //Bouton
 onPressBouton = 0;
 
 //Machine d'etat
 etat = 0;

 
}

//routine d'intteruption du timer 2
ISR(TIMER2_OVF_vect)
{
  TCNT2 = 256 - 250;// 250 * 16us = 4ms , a chaque interruption 4ms se seront ecoulées
  cmptMot++;
  cmptLed++;
  
  if(cmptMot > 2000)
  {
    cmptMot = 0;
  }
  if(cmptLed > 250)
  {
    cmptLed = 0;
  }
}

String LireTagID()
{
  //Show UID on serial monitor
  Serial.print("UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  Serial.println("");
  return content.substring(1);
}



void loop() 
{
  switch (etat)
  {
    case 0:
      if(cmptLed <= 125)
      {
        digitalWrite(LED_JAUNE,HIGH);
      }
      else
      {
        digitalWrite(LED_JAUNE,LOW);
      }

      //Si un dechet est presenter au lecteur de la Poly'bel 
      if(mfrc522.PICC_IsNewCardPresent())
      {
        if (mfrc522.PICC_ReadCardSerial()) 
        {
          tagID = LireTagID();
          etat = 1;
          break;
        }
      }
      
      //Appuie sur un bouton
      if(digitalRead(BOUTON_1) == HIGH)
      {
        onPressBouton = 1;
        etat = 1;
        break;
      }
      else if(digitalRead(BOUTON_2) == HIGH)
      {
        onPressBouton = 2;
        etat = 1;
        break;
      }
       else if(digitalRead(BOUTON_3) == HIGH)
      {
        onPressBouton = 3;
        etat = 1;
        break;
      }
      
      break;

      
    case 1 :
      etat = 0; //Si on ne trouve pas le tag on retournera a l'etat 0
      digitalWrite(LED_JAUNE,LOW);
      for(int i = 0; i < TAILLE_DATAB; i++)
      {
        Serial.println(database[i]);
        if(tagID == database[i] || onPressBouton != 0)//On cherche la valeur dans la base de données pour si savoir si le tag existe ou si un bouton a été appuyer
        {
          if( (i>= 0 && i < 3) || onPressBouton == 1)
          {
            //Plasitque -> Bac 1 
             servoTrappe.detach();
             servoTrappe.attach(PORT_MOT1);  
          }
          else if(i>= 3 && i < 6 || onPressBouton == 2)
          {
            //Carton -> Bac 2
             servoTrappe.detach();
             servoTrappe.attach(PORT_MOT2);
          }
          else if(i>= 6 && i < 9 || onPressBouton == 3)
          {
            //Aluminium -> Bac 3
            servoTrappe.detach();
            servoTrappe.attach(PORT_MOT3);
          }
        
          SensMoteur = MOT_AV;//On activer le sens d'ouverture des moteurs
          cmptMot = 0;//On reinitialise le compteur pour le temps d'ouverture
          onPressBouton = 0; //On remet le flag de bouton au cas ou il a "t" appuyer
          
          servoTrappe.write(SensMoteur);
          digitalWrite(LED_ROUGE,HIGH);
          tagID = "";
          etat = 3;
        }
      }
      break;


    case 3 :
      //Serial.println(cmptMot);
      if(cmptMot >= 750)
      {
        digitalWrite(LED_ROUGE,LOW);
        digitalWrite(LED_VERTE,HIGH);
        cmptMot = 0;
        SensMoteur = MOT_STOP;
        servoTrappe.write(SensMoteur);
        etat = 4;
      }
      break;


    case 4 :
      //Serial.println(cmptMot);
      if(cmptMot >= 1250)
        {
          digitalWrite(LED_ROUGE,HIGH);
          digitalWrite(LED_VERTE,LOW);
          cmptMot = 0;
          SensMoteur = MOT_AR;
          servoTrappe.write(SensMoteur);
          etat = 5;
        }
      break;

    case 5 :
      //Serial.println(cmptMot);
      if(cmptMot >= 750)
      {
        cmptMot = 0;
        digitalWrite(LED_ROUGE,LOW);
        SensMoteur = MOT_STOP;
        servoTrappe.write(SensMoteur);
        etat = 0;
      }
      break;s
    
    default : 
      Serial.println("ERROR ETAT NON DEFINI");
      etat = 0;
      break;     
  }
}
