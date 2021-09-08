/*  WiFiTelnetToSerial - Example Transparent UART to Telnet Server for esp8266 */
#define MAX_SRV_CLIENTS 2
boolean telnetflag=false;
WiFiServer Telnet_server(23);// 23 ist der Standard Telnet Port  //TELNET TELNET TELNET *********************
WiFiClient Telnet_serverClients[MAX_SRV_CLIENTS];                //TELNET TELNET TELNET *********************
char telnet_read();
int nr_des_letzten_gestoppten_clients=0; //weil MAX_SRV_CLIENTS erfüllt war
//*********************************************************************************************
void telnet_init() {
  if(telnetflag==false) { //beim ersten Telnetaufruf initialisieren
    telnetflag=true;
    Telnet_server.begin();  //start Telnet_server   //TELNET TELNET TELNET *********************
    Telnet_server.setNoDelay(true);                 //TELNET TELNET TELNET *********************
//    Serial.print("Ready! Use 'telnet (PUTTY)");
//    Serial.print(WiFi.localIP());
//    Serial.printf(":%d' to connect\n", 23); // Telnet Standardport ist 23
  }
}
//*********************************************************************************************
/*
 * Funktioniert nur bedingt, muss getestet werden, es fehlt zu prüfen ob ein Eingabewert vorliegt (available)
 */
  long telnet_read_zahl() { // wandelt eingelesenen Wert in Zahl im Format long um
  int vorzeichen=1;
  long c1=0;
  int c,n=0;
  while((c=int (telnet_read()))!=0x0a) {
    //if(c!=0) Serial.printf("hier while: c= %x,   c1=%d\n",c, c1);    delay(200);
    if(c>=0x30 && c<=0x39) {
      c1=c1*10 + long (c-0x30);
    }
    else {
      if(c==0x2d)vorzeichen= (-1); //0x2d = '-'
      else {
        if(c==0x0d);
        else return(-1); 
      }
    }
    if(c!=0)  delay(2); //*Serial.printf("hier while: c= %x,   c1=%d\n",c, c1);
  } 
  //delay(200);  Serial.printf("Hier Returnwert: %d, Das Vorzeichen lautet: %d\n", c1*vorzeichen, vorzeichen);
  return(c1*vorzeichen);
}
//*********************************************************************************************
char telnet_read() {
  telnet_init();
  if (Telnet_server.hasClient()) {               //check if there are any new clients //find free/disconnected spot
  int i;                               
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
      if (!Telnet_serverClients[i]) { // equivalent to !Telnet_serverClients[i].connected()
        Telnet_serverClients[i] = Telnet_server.available();
        //Serial.print("New client: index ");
        //Serial.print(i);
        break;
      }
    if (i == MAX_SRV_CLIENTS) {    //no free/disconnected spot so reject
      Telnet_server.available().println("busy"); // hints: server.available() is a WiFiClient with short-term scope, when out of scope, a WiFiClient will - flush() - all data will be sent - stop() - automatically too
      //Serial.printf("server is busy with %d active connections\n", MAX_SRV_CLIENTS);
    }
  }
  //check TCP clients for data
  for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
    while (Telnet_serverClients[i].available() && Serial.availableForWrite() > 0) {
      // working char by char is not very efficient
      return(Telnet_serverClients[i].read());
    }
  }
  return('\0');
}
//*********************************************************************************************
void telnet_write(char* buf) {
  telnet_init();
  if (Telnet_server.hasClient()) {                  //check if there are any new clients //find free/disconnected spot
    int i;                               
    for (i = 0; i <= MAX_SRV_CLIENTS; i++) {
      if (!Telnet_serverClients[i]) { // equivalent to !Telnet_serverClients[i].connected()
        Telnet_serverClients[i] = Telnet_server.available();
        //Serial.print("New client: index ");
        //Serial.print(i);
        break;
      }
      if (i == MAX_SRV_CLIENTS) {    //no free/disconnected spot so reject
        Telnet_server.available().println("busy, noch einmal versuchen"); // hints: server.available() is a WiFiClient with short-term scope, when out of scope, a WiFiClient will - flush() - all data will be sent - stop() - automatically too
        Telnet_serverClients[nr_des_letzten_gestoppten_clients].stop();
        nr_des_letzten_gestoppten_clients++;
        if(nr_des_letzten_gestoppten_clients>=MAX_SRV_CLIENTS) nr_des_letzten_gestoppten_clients=0;//uwe war da
        //Serial.printf("server is busy with %d active connections\n", MAX_SRV_CLIENTS);
      }
    }
  }
  unsigned int maxToTcp = 0;
  for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (Telnet_serverClients[i]) {
      unsigned int afw = Telnet_serverClients[i].availableForWrite();  // client.availableForWrite() returns 0 when !client.connected()
      if (afw!=0) {
        if (!maxToTcp) maxToTcp = afw;
        else if(maxToTcp>afw)  maxToTcp = afw; // 1.) #include <algorithm> // std::min, 2. else maxToTcp = std::min(maxToTcp, afw);
      } 
      //else Serial.printf("one client is congested, %d\n", afw);// warn but ignore congested clients
    }
  }
  unsigned int serial_got=0;
  for (int i = 0; i < MAX_SRV_CLIENTS; i++) {// push UART data to all connected telnet clients
    for(int j = 0; buf[j]!='\0'; j++) serial_got=j+1;
    int len = serial_got;
    if (Telnet_serverClients[i].availableForWrite() >= serial_got) { // if client.availableForWrite() was 0 (congested) and increased since then, ensure write space is sufficient:
      unsigned int tcp_sent = Telnet_serverClients[i].write(buf, serial_got);
      //Serial.printf("%s,  %s, serial_got: %d, i: %d\n","Hallo", buf, serial_got, i);
     if (tcp_sent != len) {
        //Serial.printf("len mismatch: available:%zd serial-read:%zd tcp-write:%zd\n", len, serial_got, tcp_sent);
      }
    }
  }
}
//*********************************************************************************************
/*loop() {  //Original aus Beispiele
  if (Telnet_server.hasClient()) {               //check if there are any new clients //find free/disconnected spot
    int i;                               
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
      if (!Telnet_serverClients[i]) { // equivalent to !Telnet_serverClients[i].connected()
        Telnet_serverClients[i] = Telnet_server.available();
        Serial.print("New client: index ");
        Serial.print(i);
        break;
      }
    if (i == MAX_SRV_CLIENTS) {    //no free/disconnected spot so reject
      Telnet_server.available().println("busy"); // hints: server.available() is a WiFiClient with short-term scope, when out of scope, a WiFiClient will - flush() - all data will be sent - stop() - automatically too
      Serial.printf("server is busy with %d active connections\n", MAX_SRV_CLIENTS);
    }
  }
  //check TCP clients for data
  for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    while (Telnet_serverClients[i].available() && Serial.availableForWrite() > 0) {
      // working char by char is not very efficient
      Serial.write(Telnet_serverClients[i].read());
    }
  // determine maximum output size "fair TCP use"
  // client.availableForWrite() returns 0 when !client.connected()
  unsigned int maxToTcp = 0;
  for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    if (Telnet_serverClients[i]) {
      unsigned int afw = Telnet_serverClients[i].availableForWrite();
      if (afw) {
        if (!maxToTcp) {
          maxToTcp = afw;
        } 
        else {
          maxToTcp = std::min(maxToTcp, afw);
        }
      } 
      else {
        // warn but ignore congested clients
        Serial.println("one client is congested");
      }
    }
    char sbuf[1000];
    unsigned int serial_got=0;
    sprintf(sbuf, "Hier ist Uwe %d\n\r\0", millis());
    for(int j = 0; sbuf[j]!='\0'; j++) serial_got=j+1;
    int len = serial_got;
    for (int i = 0; i < MAX_SRV_CLIENTS; i++) // push UART data to all connected telnet clients
      if (Telnet_serverClients[i].availableForWrite() >= serial_got) { // if client.availableForWrite() was 0 (congested) and increased since then, ensure write space is sufficient:
        unsigned int tcp_sent = Telnet_serverClients[i].write(sbuf, serial_got);
        Serial.print(sbuf);
        if (tcp_sent != len) {
          Serial.printf("len mismatch: available:%zd serial-read:%zd tcp-write:%zd\n", len, serial_got, tcp_sent);
        }
      }
}
*/


