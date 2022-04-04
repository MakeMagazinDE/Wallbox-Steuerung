/*
 * Ausgabe des Zaehlers über die Infrarotdiode, ca. im Sekundenrythmus
/LOG5LK1XXXXXXXXX<\r><\n>
<\r><\n>
1-0:96.1.0*255(001LOG0XXXXXXXX)<\r><\n>
1-0:1.8.0*255(000471.6452*kWh)<\r><\n>
1-0:2.8.0*255(000528.9178*kWh)<\r><\n>
1-0:0.2.0*255(ver.03,432F,20170504)<\r><\n>
1-0:96.90.2*255(0F66)<\r><\n>
1-0:97.97.0*255(00000000)<\r><\n>
!<\r><\n>
*/
#define LOGAREX 4 //D2 = GPIO4
#include <SoftwareSerial.h>
SoftwareSerial SoS;
long zaehler_aufbereiten(char *puffer, int *x, int *y);    //nur Dekleration
unsigned long zaehler_als_integer(int pos, char *puffer);  //nur Dekleration
//******************************************************************
//void Zaehler(){
//int v, e;
//long n;
//  n=zaehler_LOGAREX(&v, &e);
//  if(n>=-200000) {
//    Serial.print(v); Serial.print("=v  e=");Serial.print(e); Serial.print("    n=");Serial.println(n);
//  }
//}
//******************************************************************
//Funktion kehrt mit dem aktuellem Energieverbrauch zurück. Ist der Rückkehrwert <=-200000 ist der Wert ungültig
long zaehler_LOGAREX(int *v, int *e){
//long zaehler_LOGAREX(){
  
  int v1= *v, e1= *e;
  long  r=-200002;
  static char puffer[227] ;
  static int m=0;
  static boolean flag=false, flag2=false;
  static unsigned long t=0;
    if(millis()-t > 10000) { //alle 10 Sekunden den Zaehlerstand lesen, kuerzere Zeit macht wenig Sinn, da dann ggf. kaum Aenderung
      if(flag==false) {
        //Serial.begin(9600);
        digitalWrite(LOGAREX, HIGH);
        //SoS.begin(9600, SWSERIAL_8N1, D2, D1) ; //D2 bzw GPIO 4 empfangen, D2 bzw GPIO5 senden
        flag=true;
      }
      if (Serial.available() > 0) {
        char c = Serial.read();
        if(c=='/' || m>225) { // '/' ist das erste Zeichen einer Ausgabe des Zaehlers
          m=0;      
          flag2=true;
        }
        if(flag2==true) {
          puffer[m++] = c; 
          if(c=='!') {    // '!' ist das letzte Zeichen einer Ausgabe des Zaehlers
            digitalWrite(LOGAREX, LOW);
            //Serial.end();    // SoftwareSeriell schließen, da nur alle 10 Sekunden gelesen werden soll
            t=millis();
            flag=false;
            flag2=false;
            r = zaehler_aufbereiten(puffer, &v1, &e1);
            *v=v1;
            *e=e1;
          }
        }
      }
    }
  return r; 
}
//******************************************************************
long zaehler_aufbereiten(char *puffer, int *x, int *y) {
  static unsigned long e=0, e_alt=0, v=0, v_alt=0, t=0; //v = Verbrauch, e = Einspeisung
  static int n=0, v_in_W, e_in_W;
  long returnwert;
  v=zaehler_als_integer(69, puffer); //ab Position 69 im Puffer aufbereiten
  e=zaehler_als_integer(101,puffer);
  if(v==999999999 || e==999999999) {
    //Serial.println("\n\n XXXXXXXXXXXXXXXXXXXX   F  E  H  L  E  R\n\n");
    return(-200000);
  }
  else {
    v_in_W = (v-v_alt)*36*10000/(millis()-t); //alle 10 sekunden,bei 60Sek.:(v-v_alt)*6*60000/(millis()-t), bei 30Sek.:(v-v_alt)*12*30000/(millis()-t)
    e_in_W = (-1)*int ((e-e_alt)*(36)*10000/(millis()-t)); // Einspeisung wird als negativ gerechnet
    //Serial.print("\n Zeit _____ ");Serial.print(millis()-t);Serial.print(", Verbrauch_ in_W: ");Serial.print(v_in_W); 
    //Serial.print(", Verbrauch: ");Serial.print(v-v_alt);Serial.print("   v_alt:  ");Serial.print(v_alt);Serial.print("   EINSPEISUNG in W: ");
    //Serial.print( e_in_W);Serial.print("   Einspeisung: ");Serial.println(e-e_alt);
    t=millis();
    returnwert=v_in_W+e_in_W; //Returnwert berechnen
    if(v_alt == 0) returnwert= -200001; //passiert beim allerersten Durchlauf und dann nie wieder
    v_alt=v;
    e_alt=e;
    *x=v_in_W;
    *y=e_in_W;
   }
   return (returnwert); 
}
//******************************************************************
unsigned long zaehler_als_integer(int pos, char *puffer) {
  unsigned long a=0, faktor=1000000000;
  for(int m=0; m<11;m++) {
    char c1=puffer[m+pos];
    if(c1!='.') {
      if( (c1<0x30 || c1>0x39) ) return(999999999); //einfache Plausibilitätsprüfung
      a=a+(c1 & 0x0F)*faktor;
      faktor = faktor/10;
    }
  }
  return(a);
}
