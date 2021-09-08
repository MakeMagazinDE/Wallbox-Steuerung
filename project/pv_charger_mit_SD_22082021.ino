#define vers "Version: 22.08.2021"
/**Wallboxsteuerung in Kombination mit Photovoltaik 06.05.2021, <<<< 192.168.178.41 >>>>
*/
//#include <Arduino.h> wird bereits mit arduinoOTA.h (siehe ota.h)inkludiert
//OTA
//OTA *** in der Arduino IDE Board auf  "NodeMCU 1.0(ESP-12E Module)" und ***
//OTA *** Flash Size auf "4MB (FS:2MB OTA: 1019KB)" stellen               ***
//OTA
#include "ota.h" 
#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h> //#################################################################################
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h> //*********Server
WiFiServer server(80);        //*********Server
WiFiClient client1;           //*********Server
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h> //Mehrfarbled
#include <SD.h> //für SD Card, (CLK,MISO,MOSI,SS) 14,12,13,15oder16 bzw. D5, D6, D7, D8oder?
#include <TimeLib.h>
#define CS_PIN  16 //D8=15 ist bei MP3 Benutzung belegt, dann NICHT verwendenfür SD Card
File mySD; //für SD Card
#define taster 0 //Built in Taster auf Pin Gpio0
#define garagentorpin 15 // Ansteuerung Relais bzw. optoKoppler zum Garagentor öffnen, doppelt belegt mit RS485 Umschaltung
#define NEOPIXELPIN 2 //  Neopixel WS2811 (LED)   
#define RS485_Richtung 15 //Umschaltung MAX485 halbduplex zwischen senden und empfangen, dopplet belegt mit NEO-Pixel
#define relay_3ph 4 //Relay welches die Wallbox mit allen Phasen vom Netz trennt
#define relay_2ph 5 //Relay welches 2 Phasen dazu schaltet (von ein- auf dreiphasig)
#define mn 60 //Minimum
#define mx 61 //Maximum
#define mw 62 //Mittelwert
Adafruit_NeoPixel pixels(1, NEOPIXELPIN, NEO_GRB); // 1 steht für ein Neopixel (also die Anzahl)
#include "uwesserver.h"       // erledigt den Webseiteaufbau und Abfrage
#include "Telnet2Serial.h" ermöglicht die Ausgabe von Werten via Telnet auf Port 23
char ssid[18], password[18], ip_shelly[18], ip_goe[18]; // Werte werden aus der config.txt eingelsesen
int wb, st, eindrei, laden_aktiv=0;                                    // Werte werden aus der config.txt eingelsesen
char telnet_buf[200], uhrzeit[6];
float w_akt1[12];
boolean  ta_pin_ruhe, fl_ta_pin=false, fl_relay=false;
int minmaxmiw[63], lfd_nr_mmm=0; //Hier werden Minimum. Maximum und Mittelwerte der letzten 60 Messungen gebildet 19042021$$$$$$$$$$##############
int led_flag=1,led_puls=0,led_beat_ti[]={300,80,10}, fl_timer[]={0,0,1,2}, ladestrom_ist, spannung_L1, spannung_L2, car_old,ladestrom_old=0/*betriebsmodus=0 in uwesserver.h*/; 
int eingabe=0, betriebsmodus_old=0, ladestrom=0;
//betriebsmodus, blaue LED auf uC: 0=blinkt langsam,sofort laden mit max. Leistung, 
//                                 1=blinkt Sek.Rytm. min. 50% akt. Photovoltaik (PV), 
//                                 2=blinkt schnell min 100% PV
unsigned long led_previousmicros = 0, ti_taster, ti_timer[4], shelly3em_lfd_nr=0, led_interval = 10000, tme, t_unixtime;
//*********************************************************************************
void setup() {
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear(); // NEOPIXDEL LED: Set all pixel colors to 'off'
  if(!SD.begin(CS_PIN)) multi_led(4);//{Serial.println(F("Card Mount Failed"));while(1){yield;}}// (CLK,MISO,MOSI,SS)) 14,12,13,16
  if(config_von_sd()==false) multi_led(4);  
  if(wb==2) {
    Serial.begin(19200, SERIAL_8E1);  //8Bit + even Parity bei Heidelbergwallbox
    pinMode(RS485_Richtung, OUTPUT);
    digitalWrite(RS485_Richtung, LOW);
  }
  else Serial.begin(115200); //Serial.println("\nLadekurve der Wallboxsteuerung in Kombination mit Photovoltaik, Version 22.08.2021\n");
  for(int i=0; i<12;i++) w_akt1[i]=-10000.0; //Array zum Speichern des Minimalwertes der letzten 12 Abfragen initialisieren
  for(int i=0;i<63;i++) minmaxmiw[i]=0; //Array für min, max, mw Ermittlung auf Null setzen 
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); //Serial.println("\nConnecting to FRITZ!UWE");
  while (WiFi.status() != WL_CONNECTED) delay(500); 
  //Serial.print("\nconnected, address="); Serial.println(WiFi.localIP());
  //pinMode(LED_BUILTIN, OUTPUT);
  pinMode(taster, INPUT_PULLUP);
  pinMode(garagentorpin, OUTPUT);
  digitalWrite(garagentorpin, LOW);
  pinMode(relay_3ph, OUTPUT);
  pinMode(relay_2ph, OUTPUT);
  digitalWrite(relay_3ph, LOW);
  digitalWrite(relay_2ph, LOW);
  ta_pin_ruhe = digitalRead(taster);
  server.begin(); //*********Server
  multi_led(0);delay(500);multi_led(1);delay(500);multi_led(2);delay(500);multi_led(3);delay(500);multi_led(4);delay(500); //Farbenspiel
  multi_led(0);   // NEOPIXDEL LED auf rot = Betriebsmodus "nur Netz" 
  timer(2,1); //Timer2 initialisieren, 30-Minutentimer, wird fuer Umschaltung 3-phasig / 1-phasig verwendet
  uwes_ota_start();     //OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA

  sprintf(telnet_buf,"\nSSID %s, Password (1.Z.) %c..., IP Shelly %s, IP Go-e %s, wb: %d, st: %d, eindrei: %d\n\r", ssid,password[0], ip_shelly, ip_goe, wb, st, eindrei);
  telnet_write(telnet_buf);
  Serial.print(telnet_buf);

}
//*********************************************************************************
void loop() {
  int car;
  //while (Serial.available() > 0) {int e = Serial.parseInt();if(e!=0) {Serial.printf("Eingelesen: %d\n",e);eingabe=e;}}// zum Testen
  ArduinoOTA.handle(); //OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA
  if(tasterabfrage()==1) {
    betriebsmodus++;
    if(betriebsmodus>=3) betriebsmodus=0;
  }
  if(betriebsmodus != betriebsmodus_old) {
    betriebsmodus_old = betriebsmodus;
    multi_led(betriebsmodus); //LED NEOPIXEL WS2811 ansteuern
  }
  //led_beat(led_beat_ti[betriebsmodus]); //10=sehr schnell, 80=sekundenRythmus, 400=gemütlich
  if(timer(0,5000)==true) {             //alle 5 Sekunden go-echarger und shelly 3em abfragen  
    solar=int (shelly3em_status()); //Shelly3em abfragen, aktuelle Leistung als Integer in Watt
    //solar=eingabe; //zum Testen ###################################################################################
    car  =wallbox("egal", -1); // Parameter -1 fragt nur den Status ab, ohne Werte zu setzen 
    if(solar > -30000 && car > -30000) ladestrom=laden(car); // Werte von kleiner -30.000 sind Fehlermeldungen
    else {
      timer(3,2);
      multi_led(4); //LED ist für 2 Sekunden violett um gestörten Zugriff auf shelly3em oder go-echarger anzuzeigen
    }
    if( timer(3,2000)==true) multi_led(betriebsmodus); //... und wieder "Normalmodus
  }
  WiFI_Traffic(); //falls sich ein Client verbunden hat HTML ausgeben und Antwort einlesen
}
//*********************************************************************
int laden(int car) {
  if(car==1) car_old=1;
  ladestrom = solar *(-1)/230;                                        // Leistung der PV geteilt durch Spannung (230Volt) = Ampere
  if(eindrei==3) ladestrom=ladestrom/3;                               // es wird ausschließlich 3-phasig geladen (Parameter eindrei von SD)
  minmaxmiw[lfd_nr_mmm++%60]=ladestrom;                               // den aktuellen Ladestrom speichern um min, max, mittelw bilden zu können
  switch(betriebsmodus) {
    case 0: {                                                         // sofort laden, PV egal, LED=rot
      ladestrom=48;                                                   // Strom auf allen drei Phasen, 3x16=48
      break;
    }
    case 1: {                                                         // mindestens 50% PV, sonst kein Laden, LED=gelb
      if(ladestrom >=3&&ladestrom <6) ladestrom=6;                    // weniger als 6 AMpere geht nicht beim Laden 
      if(ladestrom <3) ladestrom = 0;                                 // bei weniger als 50% von der PV kein Laden 
      break;
    }
    case 2: {                                                         // mindestens 100% PV, sonst kein Laden, LED=grün 
      if(ladestrom<6) ladestrom=0;
      break;
    }
  } //ende Switch                                                     // switch-case Ende
  if(ladestrom !=0) {
    if(ladestrom <=16) dreiphasig(false);
    if(ladestrom > 20) dreiphasig(true);
    if(fl_relay==true) {
      ladestrom = ladestrom /3; //durch 3 weil dreiphasig
      if(ladestrom < 6) ladestrom=6;                                  // kann noetig werden, wenn noch auf 3-phasig geschaltet ist
    }
    if(ladestrom>16) ladestrom=16;                                    // maximal 16 Ampere Ladestrom
    if(ladestrom_old != ladestrom) {
      car=wallbox("amx",ladestrom);                                   // Ladestrom setzen
      ladestrom_old = ladestrom;
    }
    if(car_old==1&&car!=1) {                                          // findet statt, wenn das Ladekabel gesteckt wird
      car_old=car;
      car=wallbox("alw",1);                                           // Laden starten
      sprintf(telnet_buf,"Laden starten über car_old, alw=1 \n\r\0");  telnet_write(telnet_buf);
    }
    if(timer(1,600000) == true) {                                     // alle 10 Min.
      int mw1=mmm(mw); //Mittelwert der letzten 60 Werte 
      if( (mw1>=6&&betriebsmodus==2) || (mw1>=3&&betriebsmodus==1)) { // Wenn Mittelwert der letzten Messungen drüber waren das Laden starten 
        car=wallbox("alw",1);                                         // Laden starten
        sprintf(telnet_buf,"alw=1\n\r\0");  telnet_write(telnet_buf);
       } 
    }
  }
  else {
    dreiphasig(false);
    if(ladestrom_old >6) { //bis zum endgültigen beenden der Ladung Strom auf das Minimum von 6A stellen
      car=wallbox("amx",6); // Ladestrom setzen
      ladestrom_old = 6;
    }
    if(timer(1,600000)==true) {       // alle 10 Min.
      int mw1=mmm(mw); //Mittelwert der letzten 60 Werte 
      if( (mw1<6&&betriebsmodus==2) || (mw1<3&&betriebsmodus==1)) {  //Wenn Mittelwert der letzten Messungen drunter waren das Laden stoppen 
        car=wallbox("alw",0);       // Laden unterbrechen
        ladestrom_old=0;            // damit es beim Naechsten alw=1 dann auch wirklich gleich los geht
        sprintf(telnet_buf,"alw=0\n\r\0");  telnet_write(telnet_buf);
      } 
    }
  }
  sprintf(telnet_buf,"%04d.%02d.%02d;%02d:%02d:%02d;%2dAsoll;%5dPV;%1dcar;%2dLS;%2d=;%2d<;%2d>;%db;%4dWb;%2dAist;%dR;%4ds;%3dx\n\0",
               year(t_unixtime), month(t_unixtime), day(t_unixtime), hour(t_unixtime), minute(t_unixtime),second(t_unixtime),
               ladestrom,solar*(-1),car,minmaxmiw[(lfd_nr_mmm-1)%60],mmm(mw),mmm(mn),mmm(mx),betriebsmodus,leistung,
               ladestrom_ist,fl_relay,(ti_timer[2]+1800000-millis())/1000,(ti_timer[1]+600000-millis())/1000);
  sd(telnet_buf,t_unixtime); //auf SD Karte schreiben
  strcat(telnet_buf,"\r");telnet_write(telnet_buf);
  return(ladestrom);
}  
//*********************************************************************
boolean dreiphasig(boolean dreiph) {
  if(timer(2, 1800000)==true && eindrei==2) { //1'800.000 entspricht 30 Minuten, eindrei==2 bedeudet umschalten zwischen ein- und dreiphasig erlaubt
    if(ladestrom_ist >50 && dreiph==true && fl_relay==false) {  //Ladestrom_ist: Beispiel 50 = 5,0A;  89=8,9 Ampere
      relay_schalten(true);                                     //Ladestrom_ist wird genommen um zu prüfen, dass Auto tatsächlich lädt
      fl_relay=true;
      timer(2,1); //Timer neu starten
    }
    if(dreiph==false && fl_relay==true) {
      relay_schalten(false);
      fl_relay=false;
      timer(2,1); //Timer neu starten
    }
  }
}
//*********************************************************************
void relay_schalten(boolean an) {
  wallbox("alw",0);       // Laden unterbrechen MUSS noch getestet werden$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
  delay(3000);                  // Laden unterbrechen MUSS noch getestet werden$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
  sprintf(telnet_buf,"3ph-Rel: %d\n\r\0",HIGH);  telnet_write(telnet_buf);
  if(wb!=1) {
    digitalWrite(relay_3ph, HIGH); // go_echarger mit allen 3-Phasen stromlos schalten
    delay(500);
    if(an==true) {
      digitalWrite(relay_2ph, HIGH); //2 Phasen dazu schalten
      sprintf(telnet_buf,"2ph-Rel: %d\n\r\0",HIGH);  telnet_write(telnet_buf);
      //Serial.printf("2ph-Rel: %d\n",HIGH);
    }
    else {
      digitalWrite(relay_2ph,  LOW); //2 Phasen abschalten
      sprintf(telnet_buf,"2ph-Rel: %d\n\r\0",LOW);  telnet_write(telnet_buf);
    }
    delay(500);
    digitalWrite(relay_3ph,  LOW); // go_echarger mit allen 3-Phasen wieder anschalten
    sprintf(telnet_buf,"3ph-Rel: %d\n\r\0",LOW);  telnet_write(telnet_buf);
    delay(6000); // Go-echarger braucht Zeit für einen Neustart
  }
  else {
    if(an==true) wallbox("fsp",3); //der go-echarger schaltet mit dem Befehl fsp zwischen 1- und 3-phasig um
    else wallbox("fsp",0);         //ACHTUNG in der Abfrage mit /status ist fsp=1 1-phasig und fsp=0 3-phasig
    delay(500);
  }
  for(int n=0;n<15;n++) {         // Laden wieder starten, MUSS noch getestet werden$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    if(wallbox("alw",1)<0) delay(500); 
    else break;
  }
  //while(wallbox("alw",1)<0) delay(500);       // Laden wieder starten, MUSS noch getestet werden$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
  //wallbox("alw",1);
  wallbox("amx",ladestrom);                                   // Ladestrom setzen
  ladestrom_old = ladestrom;
}
//*********************************************************************
//mit dauer=0 wird timer initialisiert, mit dauer=1 wird Timer gestartet, nach Ablauf findet kein Rücksetzen bzw. neu "aufziehen" statt
//mit dauer=2 wie bei 1, kommt aber nur einmal mit true zurück
//wenn die Zeit "dauer" in ms um ist, return true, sonst false
boolean timer(int nr, unsigned long dauer) { 
  if(dauer<=2) {
    fl_timer[nr] = int (dauer);
    ti_timer[nr] = millis();
    return(false);
  }
  else {
    if((ti_timer[nr]+dauer) > millis()) return (false);
    else {
      if(fl_timer[nr]<=2) {
        if(fl_timer[nr]==2) fl_timer[nr]=3;
        if(fl_timer[nr]==0) ti_timer[nr]=millis();
        return(true);
      }
    return (false);
    }
  }
  //return(false); 
}
//*********************************************************************
int tasterabfrage() {
  int ta_pin=digitalRead(taster);
  if(ta_pin!=ta_pin_ruhe && fl_ta_pin==false) {
    fl_ta_pin=true;
    ti_taster=millis();
  }
  if(ta_pin==ta_pin_ruhe) {
    fl_ta_pin=false;
    return(-1);
    
  }
  if(ta_pin != ta_pin_ruhe && fl_ta_pin ==true) {
    unsigned long ti=millis()-ti_taster;
    if(ti<=5) return(0);                //entprellen
    if(ti>=5&&ti<2500) {
      ti_taster = ti_taster -3000;      //damit nur einmal return(1) kommt
      ti_timer[1]=millis()-600000;      //ti_timer läuft beim 1. Aufruf auf true, danach alle 10 Minuten
      //led_flag=1;led_puls=0;led_previousmicros = 0;led_interval = 10000;//LED-Beat neu initialisieren, da sporadisch Probleme nach dem Umschalten
      led_status = LOW; //da die Beatfrequenz neu gesetzt wird muss neu initialisiert werden
      return(1);
    }
    else return(2);
  }
}
//*********************************************************************************
void multi_led(int n) { //rot=sofort, gelb=min. 50% PV, gruen=100%PV, violett=Fehler
//                     rot,    gelb,   gruen,   blau,    violett
  byte neopix[5][3]={{0,10,0},{5,8,0},{10,0,0},{0,0,10},{0,5,8}}; // gedimmt, 255=maximum
//  byte neopix[5][3]={{0,64,0},{32,48,0},{64,0,0},{0,0,64},{0,32,48}}; //etwas hellere Variante
  pixels.setPixelColor(0, pixels.Color(neopix[n][0],neopix[n][1],neopix[n][2]));
  pixels.show(); 
}
//*********************************************************************************
// ein an/aus Zyklus der LED dauert 10000 us. Das Verhältnis an bzw. aus wird über 
// die Variable led_puls gesteuert. Die Dauer eines Zyklus in 10-facher MilliSekunden
// Zeit wird über n gesteuert (n=100  heisst ein Zyklus dauert 1000ms)
void led_beat(int n){
  unsigned long currentmicros = micros();
  if (currentmicros - led_previousmicros >= led_interval) {
    led_previousmicros = currentmicros;
    if (led_status == LOW) {
      led_status = HIGH;  // Note that this switches the LED *off*
      led_interval=10000;
    } 
    else {
      led_status = LOW; // Note that this switches the LED *on*
      if(led_flag==1) led_interval=led_puls *10000/n;
      else led_interval = 10000-(led_puls *10000/n);
    }
    //if(led_puls>n/4) led_puls=led_puls+4;
    led_puls++;
    if(led_puls>=n) {
      led_puls=0;
      led_flag=led_flag*(-1);
    }
    digitalWrite(LED_BUILTIN, led_status);
  }
}
//*********************************************************************************
int wallbox(char* param, int wert) {
  if(wb==1) return(go_echarger(param, wert)); //welche Wallbox ist auf der SD Karte aktiviert
  //else return(heidelberg(param, wert));
  else {
    int x=heidelberg(param, wert);
    if(x<-20000) {
      sprintf(telnet_buf,"                                                                                                                $$$$$$$$$$$$$$$FEHLER: %d\n\r",x );telnet_write(telnet_buf);
    }
    return(x);
  }
}
//*********************************************************************************
int heidelberg(char* param, int wert) { 
  int car=2;
  byte datacs[] = { 0x01, 0x04, 0x00, 0x05, 0x00, 0x01}; //chargingstate   
  byte datamc[] = { 0x01, 0x06, 0x01, 0x05, 0x00, 120}; //Strom setzen, 120 = 12A
  byte dataL1[] = { 0x01, 0x04, 0x00, 0x06, 0x00, 0x01}; //Strom_ist L1
  byte dataVA[] = { 0x01, 0x04, 0x00, 0x0E, 0x00, 0x01}; //Power in VA alle Phasen
  if((leistung=rs485_send_and_rec(dataVA,6))<=-30000) return(-30012);               // aktuelle Leistungsabgabe der Wallbox
  if((ladestrom_ist=rs485_send_and_rec(dataL1,6))<=-30000) return(-30011);          // immer den aktuellen Ladestrom abfragen, die Antwortlänge beträgt 6 Byte
  if(ladestrom_ist<1) {
    if((car=rs485_send_and_rec(datacs,6))<=-30000) return(-30009);                 // falls kein Ladestrom_ist dann abfragen ob Fahrzeug gesteckt, die Antwortlänge beträgt 6 Byte
    if(car==0x02) car=1;                                                           //entspricht dem car=1 beim go-eCharger
    else car=2;                                                                    //Wert egal, Hauptsache ungleich 1
  }
  if(param[1]=='l') {                                                              //'l' (l wie ludwig)kommt von "alw" Ladung starten wert=1 oder stoppen wert=0
    if(wert==1) {
      laden_aktiv=1;
      datamc[5]=ladestrom*10;
      if(rs485_send_and_rec(datamc,7)!=ladestrom*10) return(-30021);                //die Antwortlänge beträgt 7 Byte und enthält den gesetzten Ladestrom
    }
    else {
      laden_aktiv=0;
      datamc[5]=0;
      if(rs485_send_and_rec(datamc,7)!=0) return(-30010);                            //die Antwortlänge beträgt 7 Byte und enthält den gesetzten Ladestrom
     }
  }
  if(param[1]=='m') {                                                                //'m' kommt von "amx" = Ladestrom setzen,
    if(laden_aktiv==1 && wert >=6) {
      datamc[5]=wert*10; 
      if(rs485_send_and_rec(datamc,7)!=wert*10) return(-30010);                      //die Antwortlänge beträgt 7 Byte und enthält den gesetzten Ladestrom
    }
  }
  return(car);
}
//*********************************************************************************
int rs485_send_and_rec(byte* zu_senden, int anz)  { //in anz steht die länge der erwarteten Antwort
  unsigned int crc = 0xFFFF;                        //Wert ist bei RS485 Modbus zur CRC Prüfung definiert
  int wert=2; //auch wenn Box im Standby bzw.offline mit Fahrzeug nicht gesteckt zurück kommen
  for(int loop=0;loop<3;loop++) { //sporadisch wird keine Antwort empfangen, dann nocheinmal probieren
    //RS485 senden . . .
    int anz_temp=anz;
    for (unsigned int i = 0; i < 6; i++) crc = crc16(crc, zu_senden[i]); //es ist über sechs Byte CRC zu bilden
    digitalWrite(RS485_Richtung, HIGH);  
    Serial.flush();
    delay(100+loop*100); //bei jedem weiteren Versuch etwas länger warten
    sprintf(telnet_buf,"                                                                                                                Zeile344: ");telnet_write(telnet_buf);
    for( int j=0; j<6;j++) {                                            //es sind sechs Zeichen zu senden
      Serial.write( zu_senden[j]);
//      sprintf(telnet_buf,"0x%02x, ", zu_senden[j]);telnet_write(telnet_buf);
      sprintf(telnet_buf,"%02d, ", zu_senden[j]);telnet_write(telnet_buf);
    }
    Serial.write( crc%256);
    Serial.write( crc/256);
    Serial.flush(); 
    //delay(50); wenn dieses delay aktiv keine Antwort!!!
    digitalWrite(RS485_Richtung, LOW);
    //RS485 senden ist fertig, jetzt die Antwort auswerten ****************** 
    delay(20);// es ist alles gesendet, jetzt einen Moment warten bevor die Antwort gelesen wird
    crc = 0xFFFF; //crc neu initialisieren für die Antwortprüfung
    int anz_receive=Serial.available();
    sprintf(telnet_buf,"Empf:%d, loop:%d, ",anz_receive,loop); telnet_write(telnet_buf);
    while(Serial.available() > 0) {
      ArduinoOTA.handle(); //OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA OTA
      byte b = Serial.read();
      if(anz_temp>=2) {
        crc = crc16(crc, b);
        //sprintf(telnet_buf,"0x%02x, ", b);telnet_write(telnet_buf);
        sprintf(telnet_buf,"%02d, ", b);telnet_write(telnet_buf);
      }
      if(anz_temp==3) wert=b*256;   //Wert der Antwort in Integer wandeln 
      if(anz_temp==2) wert=wert+b;  //Wert der Antwort in Integer wandeln
      if(anz_temp==1&&b!=(crc%256)) return(-30000); //CRC Fehler Low Byte
      if(anz_temp==0) {                             
        if(b!=(crc/256)) return(-30001);       //CRC Fehler High Byte
        sprintf(telnet_buf,"CRC: 0x%02x,0x%02x ",crc%256, crc/256); telnet_write(telnet_buf);
      }
      anz_temp--;
    }
    telnet_write("\n\r");
    if(anz_receive>0) break;
  }
  return(wert);
}
//***************************************************************************************************************
unsigned int crc16(unsigned int crc, byte b) {
  crc ^= b;
  for(int n=0;n< 8;n++) {
    if((crc&1) ==1) crc=(crc >> 1) ^ 0xA001;  //Wert 0xA001 ist bei RS485 Modbus zur CRC Prüfung definiert
    else crc = crc >> 1;
  } 
  return crc;
}
//*********************************************************************************
int go_echarger(char* param, int wert) {               //Beispiel http://192.168.178.47:81/mqtt?payload=amx=8 bzw. alw=0
  int amp, alw, car;
  WiFiClient client;
  HTTPClient http;        //Serial.print("[HTTP] begin...\n");
  char buffer[100];
  if(wert== -1) sprintf(buffer, "http://%s/status", ip_goe); //bei -1 nur den Status abfragen
  else sprintf(buffer, "http://%s/mqtt?payload=%s=%d", ip_goe, param, wert);
  //Serial.printf("hier der Aufruf: %s\n",buffer);
  if (http.begin(client, buffer)) { //Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();      // start connection and send HTTP header
    if (httpCode > 0) {             // httpCode will be negative on error, // HTTP header has been send and Server response header has been handled
                                    // String payload = http.getString();Serial.println(payload); //so sthet es im Originalbeispiel
      const size_t capacity = /*JSON_OBJECT_SIZE(7) + 70*/2300; //ist vom JSON Assistenen eine Größenschätzung, hier mal willkürlich 2300 angenommen
      DynamicJsonDocument doc(capacity);
      // nur nötig wenn serialisiert werden soll: const char* json = "{\"power\":2406.2,\"pf\":1,\"current\":10.36,\"voltage\":232.72,\"is_valid\":true,\"total\":112838.1,\"total_returned\":433568.6}";
      deserializeJson(doc, http.getStream());
      tme = doc["tme"]; //Zeit und datum
      amp = doc["amp"]; // Ampere
      alw = doc["alw"]; // Laden gestartet = 1, gestoppt = 0
      car = doc["car"]; // Status Auto: 1=Ladekabel nicht gesteckt, 2=Fahrzeug lädt, 3=Warte auf Fahrzeug, 4=fertig mit laden
      JsonArray nrg=doc["nrg"];
      ladestrom_ist=nrg[4];         //Ladestrom auf L1, Beispiel: 51 = 5,1 Ampere
      leistung =nrg[11];        // *10=Leistung in W
      leistung = leistung*10;
      spannung_L1=nrg[0];           //Spannung Phase L1
      spannung_L2=nrg[1];           //Spannung Phase L2
      //Serial.printf("Spg=%d, Ladestrom_ist=%d\n", int(nrg[0]), ladestrom_ist);
      //Serial.printf("go-e:  Ampere= %2d,  Laden gestartet: %d, car= %d\n", amp, alw, car); //neue Zeile \n
    } 
    else {
      //Serial.printf("[HTTP] GET go-echarger... failed, error: %s\n", http.errorToString(httpCode).c_str());
      return(-30000);
    }
    http.end();
  } 
  else {
    //Serial.printf("[HTTP} Unable to connect to e-charger\n");
    return(-30001);
  }
  return(car);
}
//*********************************************************************************
int shelly3em_status() {
  float w_akt, w_akt2;
  char adr[36];
  WiFiClient client;
  HTTPClient http;
  //if (http.begin(client, "http://192.168.178.122/status")) {  // HTTP
  sprintf(adr,"http://%s/status",ip_shelly);
  if (http.begin(client, adr)) {  // HTTP
    //Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();  // start connection and send HTTP header
    if (httpCode > 0) {   // httpCode will be negative on error
      // HTTP header has been send and Server response header has been handled
      //Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      // file found at server
      //String payload = http.getString();Serial.println(payload); //so sthet es im Originalbeispiel
      //const size_t capacity = /*JSON_OBJECT_SIZE(7) + 70*/2000; //ist vom JSON Assistenen eine Größenschätzung, hier mal willkürlich 300 angenommen
      DynamicJsonDocument doc(2000);
      //nur nötig wenn serialisiert werden soll: const char* json = "{\"power\":2406.2,\"pf\":1,\"current\":10.36,\"voltage\":232.72,\"is_valid\":true,\"total\":112838.1,\"total_returned\":433568.6}";
      deserializeJson(doc, http.getStream());
      t_unixtime = doc["unixtime"];
      strcpy(uhrzeit,doc["time"]);
      //*uhrzeit = doc["time"]; //Zeitstempel von shelly
      JsonArray emeters = doc["emeters"];
      JsonObject emeters_0 = emeters[0];
      JsonObject emeters_1 = emeters[1];
      JsonObject emeters_2 = emeters[2];
      float emeters_0_power = emeters_0["power"];
      float emeters_1_power = emeters_1["power"];
      float emeters_2_power = emeters_2["power"];
      //float emeters_0_total = emeters_0["total"];float emeters_0_total_returned = emeters_0["total_returned"];
      //float emeters_1_total = emeters_1["total"];float emeters_1_total_returned = emeters_1["total_returned"];
      //float emeters_2_total = emeters_2["total"];float emeters_2_total_returned = emeters_2["total_returned"];
      w_akt2=emeters_0_power + emeters_1_power + emeters_2_power;
      //w_akt=emeters_0_power + emeters_1_power + emeters_2_power;
      //int x=12;
      //w_akt1[shelly3em_lfd_nr % x]=w_akt; // in der folgenden FOR-Schleife wird der niedrigste Wert der 
      //w_akt2 = w_akt;                     // letzten 12 Aufrufe (=1 Min) ermittelt um Peaks zu eleminieren
      //unsigned long j = 0;
      //for(int i=0;i<x;i++) {
      //  if(w_akt1[i]>w_akt2) {
      //    w_akt2 = w_akt1[i];
      //    if(shelly3em_lfd_nr % x > i) j =  shelly3em_lfd_nr - shelly3em_lfd_nr % x + i;
      //    else j = shelly3em_lfd_nr -x - shelly3em_lfd_nr % x + i;
      //  }
      //}
      //if (j==0) j = shelly3em_lfd_nr;
      //shelly3em_lfd_nr++;
      //Serial.printf("shelly3em: KW_min der letzten 12 Abfragen: %6.1f,       ", w_akt2);
      //Serial.printf("shelly3em:  mod: %2d, lfdNr:%5d, KW_akt: %6.1f, lfd_min:%6d, KW_min: %6.1f", shelly3em_lfd_nr % x, shelly3em_lfd_nr, w_akt, j, w_akt2); //neue Zeile \n
      //Serial.print(  ",   total_Energie_Return:    ");Serial.print(emeters_0_total_returned + emeters_1_total_returned + emeters_2_total_returned);
      //Serial.print(",   total_Energie:    ");Serial.println(emeters_0_total + emeters_1_total + emeters_2_total);
    }
    else {
      //Serial.printf("shelly3em [HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      return(-30000);
    }
  http.end();
  } 
  else {
    //Serial.printf("shelly3em [HTTP} Unable to connect\n");
    return(-30001);
  }
  return(int (w_akt2));
}
//*********************************************************************************
void sd(char* buf, unsigned long ut) {
  if(st==2) return;                                              //1=mit Statistik, 2=0hne Statistik, Wert st von config.txt der SD-Karte
  ut=ut-604800;                                                  //60*60*24*7 = 604800 = Sekunden einer Wochewird die t_unixtime reduziert
  char dateiname[14], dateiname_rem[14];
  sprintf(dateiname_rem,"%04d%02d%02d.csv",year(ut), month(ut), day(ut)); //Statistikdateien die aelter als eine Woche sind werden geloescht
  if (SD.exists(dateiname_rem)) SD.remove(dateiname_rem);
  int m=0;
  for(int n=0; n<10;n++) if(buf[n]!='.') dateiname[m++]=buf[n];  //Das Datum wird Dateiname und auf buf extrahiert
  strcpy(&dateiname[m],".csv\0");                                // und endet mit .csv Beispiel: 20210410.csv
  mySD = SD.open(dateiname, FILE_WRITE);  //mySD = SD.open("test1.txt", FILE_WRITE);
  if (mySD) {         // if the file opened okay, write to it:
    mySD.print(buf); ///Serial.print("SD: Writing to file");
    mySD.close();    // close the file //// 
  }
}
//*********************************************************************************
int mmm(int x) { //minimum, maximum oder Mittelwert der letzten 60 Werte ausrechnen
  int n, mw1=0, mx1=0, mn1=48;
  switch(x) {
    case mw: { //Mittelwert bilden
      for(n=0; n<60; n++) mw1=mw1+minmaxmiw[n];
      minmaxmiw[mw]=mw1/60;
      return(minmaxmiw[mw]);
      break;
    }
    case mx: { //Maximum bilden
      for(n=0; n<60;n++) if(mx1<minmaxmiw[n]) mx1=minmaxmiw[n];
      minmaxmiw[mx]=mx1;
      return(minmaxmiw[mx]);
      break;
    }
    case mn: { //Minimum bilden
      for(n=0; n<60;n++) if(mn1>minmaxmiw[n]) mn1=minmaxmiw[n];
      minmaxmiw[mn]=mn1;
      return(minmaxmiw[mn]);
      break;
    }
  }
}
//***********************************************************************************
// config.txt Beispiel:
// es gelten die Werte in spitzen Klammern, maximal 18 Stellen zwischen den spitzen Klammern
// SSID (Name des WLANs)                                            <FRITZ!UWE>           char ssid[18]
// Password                                                         <************>        char password[18]
// IP-Adresse Shelly 3em                                            <192.168.178.122>     char ip_shelly[18]
// IP-Adresse Go-echarger                                           <192.168.178.47:81>   char ip_goe[18]
// Wallboxfirma Go-echarger=1, Heidelber energy control=2           <1> int wb
// Statistik auf SD (Ja=1, Nein=2)                                  <1> int st
// Automatische Umschaltung nur einphasig=1, nur dreiphasig=3,
//                             mit Relais Umschaltautomatik=2       <2> int eindrei
boolean config_von_sd() { //liest die Konfiguration von der SD-Karte aus der Datei "config.txt" ein
  int j, flag1=0;
  char zeichen;
  File config_txt = SD.open("config.txt");
  if (config_txt) {
    while (config_txt.available()) {
      zeichen = config_txt.read();
      if (zeichen == '<' ) flag1++;
      if (flag1 % 2 != 0) {
        switch (flag1) {
          case  1: for(j=0; j<18; j++) if ((ssid[j]     =config_txt.read())== '>') break; ssid[j]     ='\0'; break; //+++++++++++++++++++++++++ SSID ++++++++++++++
          case  3: for(j=0; j<18; j++) if ((password[j] =config_txt.read())== '>') break; password[j] ='\0'; break; //+++++++++++++++++++++++++ Password ++++++++++
          case  5: for(j=0; j<18; j++) if ((ip_shelly[j]=config_txt.read())== '>') break; ip_shelly[j]='\0'; break; //+++++++++++++++++++++++++ IP-Shelly +++++++++
          case  7: for(j=0; j<18; j++) if ((ip_goe[j]   =config_txt.read())== '>') break; ip_goe[j]   ='\0'; break; //+++++++++++++++++++++++++ IP-Go-Echarger ++++
          case  9: wb      = config_txt.read() & 0x0F;                                                       break; //+++++++++++++++++++++++++ Wallboxhersteller +
          case 11: st      = config_txt.read() & 0x0F;                                                       break; //+++++++++++++++++++++++++ Statistik  ++++++++
          case 13: eindrei = config_txt.read() & 0x0F;                                                       break; //+++++++++++++++++++++++++ ein-/dreiphasig +++
        }
        flag1++;
      }
    }
    config_txt.close();
    return true;
  }
  else return false; //Serial.println("error opening config.txt");
}
//*****************************************************************************************************

