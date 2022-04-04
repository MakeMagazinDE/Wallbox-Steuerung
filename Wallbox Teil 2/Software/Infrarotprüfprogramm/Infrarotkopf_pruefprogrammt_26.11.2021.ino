

/*Infrarotkopfprüfprogramm
 * 
 * 
/LOG5LK13BE803039<\r><\n>
<\r><\n>
1-0:96.1.0*255(001LOG0065626430)<\r><\n>
1-0:1.8.0*255(000471.6452*kWh)<\r><\n>
1-0:2.8.0*255(000528.9178*kWh)<\r><\n>
1-0:0.2.0*255(ver.03,432F,20170504)<\r><\n>
1-0:96.90.2*255(0F66)<\r><\n>
1-0:97.97.0*255(00000000)<\r><\n>
!<\r><\n>
*/
#include <SoftwareSerial.h>  //Version 6.15.1
SoftwareSerial SoS;
#define LOGAREX 4 //D2 = GPIO4
//******************************************************************
void setup(){
  Serial.begin(9600);
  Serial.print("\n TEST TEST TEST   Zaehler LOGAREX über IR auslesen, alle 10 Sekunden, 13.11.2021\n");
  pinMode(LOGAREX, OUTPUT);
}
//******************************************************************
void loop(){
int v, e;
long n;
  n=zaehler_LOGAREX(&v, &e);
  if(n>=-200001) {
    Serial.print(v); Serial.print("=v  e=");Serial.print(e); Serial.print("    n=");Serial.println(n);
  }
}
//******************************************************************
long zaehler_LOGAREX(int *v, int *e){
  int v1= *v, e1= *e;
  long  r=-200002;
  static char puffer[227] ;
  static int m=0;
  static boolean flag=false, flag2=false;
  static unsigned long t=0;
    if(millis()-t > 10000) { //XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX   10000
      //Serial.println("hier Uwe . . .");Serial.flush();
      if(flag==false) {
       // Serial.print("SoS Beginn,  ");//Serial.flush();
      //  SoS.begin(9600, SWSERIAL_8N1, D5, D6) ; //D5, D6 urspünglich
        flag=true;
        digitalWrite(LOGAREX, HIGH);

      }
      //if (SoS.available() > 0) {
      //  char c = SoS.read();
      if (Serial.available() > 0) {
        char c = Serial.read();
        if(c=='/' || m>225) { // '/' ist das erste Zeichen einer Ausgabe des Zaehlers
          //Serial.print("/ erkannt   ");
          m=0;      
          flag2=true;
        }
        if(flag2==true) {
          puffer[m++] = c; 
          if(c=='!') {        // '!' ist das letzte Zeichen einer Ausgabe des Zaehlers
       //     SoS.end();
            digitalWrite(LOGAREX, LOW);
            t=millis();
            flag=false;
            flag2=false;
            puffer_ausgeben(puffer);
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
void puffer_ausgeben(char *puffer){
  int n=0;
  while(true) {
    Serial.print(puffer[n]);
    if(puffer[n++]=='!') break;
  }
  Serial.print("\n");
}
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
    v_in_W = (v-v_alt)*36*10000/(millis()-t); //alle 10 sekunden
    e_in_W = (-1)*int ((e-e_alt)*(36)*10000/(millis()-t)); // Einspeisung wird als negativ gerechnet
    Serial.print("\n Zeit _____ ");Serial.print(millis()-t);Serial.print(" ms, Verbrauch_ in_W: ");Serial.print(v_in_W); 
    Serial.print(", Verbrauch: ");Serial.print(v-v_alt);Serial.print("   v_alt:  ");Serial.print(v_alt);Serial.print("   EINSPEISUNG in W: ");
    Serial.print( e_in_W);Serial.print("   Einspeisung: ");Serial.println(e-e_alt);
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
