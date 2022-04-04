//uwesserver.h
//Weboberfläche um das Garagentor zu öffnen, ein Druck auf die Schaltfläche erzeugt einen 1-Sekundenimpuls
#define MAX_PACKAGE_SIZE 2048
char HTML_String[5000];
char HTTP_Header[150];
int Aufruf_Zaehler = 0, switch_color=0;
#define ACTION_Tor 1
#define ACTION_Wallbox 2
#define ACTION_Refresh 3
int action, leistung, solar;
char betriebsmodus_tab[3][20] = {"100% Netz (rt)", "min. 50%PV (ge)", "100% PV (gn)"}; // Radiobutton
byte betriebsmodus = 0;
char tmp_string[20];
boolean fl_garagentor=false,fl_garagentorcolor=false, led_status = LOW;
unsigned long ti_timer1=0;
boolean fl_ti_timer=false;
//*********************************************************************************************************************
void WiFI_Traffic() ;
int Pick_Parameter_Zahl(const char*, char*);
void make_HTML01() ;
void send_bin(const unsigned char * , int, const char * , const char * ) ;
void send_not_found() ;
void send_HTML() ;
void set_colgroup(int, int, int, int, int) ;
void set_colgroup1(int ) ;
void strcati(char* , int ) ;
void strcati2(char*, int) ;
int Find_End(const char *, const char *) ;
int Find_Start(const char *, const char *) ;
int Pick_Dec(const char *, int ) ;
int Pick_N_Zahl(const char *, char, byte) ;
int Pick_Hex(const char * , int) ;
void Pick_Text(char *, char  *, int) ;
char HexChar_to_NumChar( char) ;
void exhibit(const char *, int) ;
void exhibit(const char *, unsigned int) ;
void exhibit(const char *, unsigned long) ;
void exhibit(const char *, const char *) ;
//*********************************************************************************************************************
void garagentor() {         //Gragentor öffnen
  if(fl_garagentor==true) {
    if(fl_ti_timer==false) {
      fl_ti_timer=true;
      ti_timer1=millis();
      digitalWrite(garagentorpin, HIGH);
    }
    if((ti_timer1+1000)<millis()) {
      fl_ti_timer=false;  
      fl_garagentor=false;
      digitalWrite(garagentorpin, LOW);
    }
  }
}
//*********************************************************************************************************************
void WiFI_Traffic() {
  char my_char;
  int htmlPtr = 0,myIdx, myIndex;
  unsigned long my_timeout;
  garagentor();
  client1 = server.available();  // Check if a client1 has connected
  if (!client1) return;
  my_timeout = millis() + 250L;
  while (!client1.available() && (millis() < my_timeout) ) delay(10);
  delay(10);
  if (millis() > my_timeout) return;
  htmlPtr = 0;
  my_char = 0;
  while (client1.available() && my_char != '\r') {
    my_char = client1.read();
    HTML_String[htmlPtr++] = my_char;
  }
  client1.flush();
  HTML_String[htmlPtr] = 0;
  if (Find_Start ("/?", HTML_String) < 0 && Find_Start ("GET / HTTP", HTML_String) < 0 ) {
    send_not_found();
    return;
  }
  action = Pick_Parameter_Zahl("ACTION=", HTML_String);         // Benutzereingaben einlesen und verarbeiten
  if ( action == ACTION_Tor) fl_garagentor=true;
  if ( action == ACTION_Wallbox) {
    betriebsmodus = Pick_Parameter_Zahl("BETRIEBSMODUS=", HTML_String);
    led_status = LOW; //da die Beatfrequenz neu gesetzt wird muss neu initialisiert werden
  }
  if ( action == ACTION_Refresh); //tue nichts außer HTML neu versenden
  make_HTML01();  //Antwortseite aufbauen
  strcpy(HTTP_Header , "HTTP/1.1 200 OK\r\n"); // Header aufbauen
  strcat(HTTP_Header, "Content-Length: ");
  strcati(HTTP_Header, strlen(HTML_String));
  strcat(HTTP_Header, "\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  client1.print(HTTP_Header);
  delay(20);
  send_HTML();
}
//*********************************************************************************************************************
void make_HTML01() {                         // HTML Seite 01 aufbauen
  char buf[100];
  strcpy( HTML_String, "<!DOCTYPE html>");
  strcat( HTML_String, "<html>");
  strcat( HTML_String, "<meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"); //für Schriftgroesse Smartphone
  strcat( HTML_String, "<head><title>Garage</title></head>");
  strcat( HTML_String, "<font color=\"#000000\" face=\"VERDANA,ARIAL,HELVETICA\">");
  if(fl_garagentorcolor==true) strcat( HTML_String, "<body bgcolor=\"#adcede\">"); //Farbe hin und her schalten, als Quittung fürs absenden
  else strcat( HTML_String, "<body bgcolor=\"#decead\">");                         //Farbe hin und her schalten, als Quittung fürs absenden
  fl_garagentorcolor = !fl_garagentorcolor; //negieren
  sprintf(buf,"<font color=\"7f7f7f\">%s</font>",vers);  //Versionsdatum ausgeben
  strcat( HTML_String, buf);
  strcat( HTML_String, "<h2>Garagentor / Wallbox</h2>");
  strcat( HTML_String, "<form><td><button style= \"width:250px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_Tor);
  strcat( HTML_String, "\"><h1><font color=\"000000\">GARAGENTOR</button></font></h1></td></form>");
  strcat( HTML_String, "<h2><font color=\"2f7f38\"><br><br>Ladeautomatik</font></h2><form><table>");
  sprintf(buf,"<h5><font color=\"2f387f\"><br>PV-Ueberschuss: %5dW, WallBox: %5dW</font></h5><form><table>",solar*-1,leistung);
  strcat( HTML_String, buf);
  set_colgroup(80, 270, 150, 0, 0);
  strcat( HTML_String, "<form>");
  // Radiobuttons
  for (int i = 0; i < 3; i++) {            
    strcat( HTML_String, "<tr>");
    if (i == 0)  strcat( HTML_String, "<td><b>Betriebsmodus</b></td>");
    else strcat( HTML_String, "<td> </td>");
    strcat( HTML_String, "<td><input type = \"radio\" name=\"BETRIEBSMODUS\" id=\"LD");
    strcati( HTML_String, i);
    strcat( HTML_String, "\" value=\"");
    strcati( HTML_String, i);
    strcat( HTML_String, "\"");
    if (betriebsmodus == i)strcat( HTML_String, " CHECKED"); //kennzeichnet den bisher ausgewählten button
    strcat( HTML_String, "><label for=\"LD");
    strcati( HTML_String, i);
    strcat( HTML_String, "\">");
    strcat( HTML_String, betriebsmodus_tab[i]);
    strcat( HTML_String, "</label></td></tr>");
  }
  // OK-Knopf
  strcat( HTML_String, "<td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_Wallbox);
  strcat( HTML_String, "\"><font color=\"000000\"><h4>OK</h4></button></font></td>"); //strcat( HTML_String, "</tr>");
  strcat( HTML_String, "</form>");
  // Refresh Knopf
  strcat( HTML_String, "<form><td><button style= \"width:100px\" name=\"ACTION\" value=\"");
  strcati(HTML_String, ACTION_Refresh);
  strcat( HTML_String, "\"><font color=\"000000\">Refresh</button></font></td>"); //strcat( HTML_String, "</tr>");
  strcat( HTML_String, "</form>");
}
//*********************************************************************************************************************
void send_not_found() {
  client1.print("HTTP/1.1 404 Not Found\r\n\r\n");
  delay(20);
  client1.stop();
}
//*********************************************************************************************************************
void send_HTML() {
  char my_char;
  int  my_len = strlen(HTML_String);
  int  my_ptr = 0;
  int  my_send = 0;
  while ((my_len - my_send) > 0) {            // in Portionen senden
    my_send = my_ptr + MAX_PACKAGE_SIZE;
    if (my_send > my_len) {
      client1.print(&HTML_String[my_ptr]);
      delay(20);
      my_send = my_len;
    } else {
      my_char = HTML_String[my_send];
      // Auf Anfang eines Tags positionieren
      while ( my_char != '<') my_char = HTML_String[--my_send];
      HTML_String[my_send] = 0;
      client1.print(&HTML_String[my_ptr]);
      delay(20);
      HTML_String[my_send] =  my_char;
      my_ptr = my_send;
    }
  }
  client1.stop();
}
//*********************************************************************************************************************
void set_colgroup(int w1, int w2, int w3, int w4, int w5) {
  strcat( HTML_String, "<colgroup>");
  set_colgroup1(w1);
  set_colgroup1(w2);
  set_colgroup1(w3);
  set_colgroup1(w4);
  set_colgroup1(w5);
  strcat( HTML_String, "</colgroup>");
}
//*********************************************************************************************************************
void set_colgroup1(int ww) {
  if (ww == 0) return;
  strcat( HTML_String, "<col width=\"");
  strcati( HTML_String, ww);
  strcat( HTML_String, "\">");
}
//*********************************************************************************************************************
void strcati(char* tx, int i) {
  char tmp[8];
  itoa(i, tmp, 10);
  strcat (tx, tmp);
}
//*********************************************************************************************************************
void strcati2(char* tx, int i) {
  char tmp[8];
  itoa(i, tmp, 10);
  if (strlen(tmp) < 2) strcat (tx, "0");
  strcat (tx, tmp);
}
//*********************************************************************************************************************
int Pick_Parameter_Zahl(const char * par, char * str) {
  int myIdx = Find_End(par, str);
  if (myIdx >= 0) return  Pick_Dec(str, myIdx);
  else return -1;
}
//*********************************************************************************************************************
int Find_End(const char * such, const char * str) {
  int tmp = Find_Start(such, str);
  if (tmp >= 0)tmp += strlen(such);
  return tmp;
}
//*********************************************************************************************************************
int Find_Start(const char * such, const char * str) {
  int tmp = -1;
  int ww = strlen(str) - strlen(such);
  int ll = strlen(such);
  for (int i = 0; i <= ww && tmp == -1; i++) {
    if (strncmp(such, &str[i], ll) == 0) tmp = i;
  }
  return tmp;
}
//*********************************************************************************************************************
int Pick_Dec(const char * tx, int idx ) {
  int tmp = 0;
  for (int p = idx; p < idx + 5 && (tx[p] >= '0' && tx[p] <= '9') ; p++) {
    tmp = 10 * tmp + tx[p] - '0';
  }
  return tmp;
}
//*********************************************************************************************************************
int Pick_N_Zahl(const char * tx, char separator, byte n) {
  int ll = strlen(tx), tmp = -1;
  byte anz = 1, i = 0;
  while (i < ll && anz < n) {
    if (tx[i] == separator)anz++;
    i++;
  }
  if (i < ll) return Pick_Dec(tx, i);
  else return -1;
}
//*********************************************************************************************************************
int Pick_Hex(const char * tx, int idx ) {
  int tmp = 0;
  for (int p = idx; p < idx + 5 && ( (tx[p] >= '0' && tx[p] <= '9') || (tx[p] >= 'A' && tx[p] <= 'F')) ; p++) {
    if (tx[p] <= '9')tmp = 16 * tmp + tx[p] - '0';
    else tmp = 16 * tmp + tx[p] - 55;
  }
  return tmp;
}
//*********************************************************************************************************************
void Pick_Text(char * tx_ziel, char  * tx_quelle, int max_ziel) {
  int p_ziel = 0;
  int p_quelle = 0;
  int len_quelle = strlen(tx_quelle);
  while (p_ziel < max_ziel && p_quelle < len_quelle && tx_quelle[p_quelle] && tx_quelle[p_quelle] != ' ' && tx_quelle[p_quelle] !=  '&') {
    if (tx_quelle[p_quelle] == '%') {
      tx_ziel[p_ziel] = (HexChar_to_NumChar( tx_quelle[p_quelle + 1]) << 4) + HexChar_to_NumChar(tx_quelle[p_quelle + 2]);
      p_quelle += 2;
    } else if (tx_quelle[p_quelle] == '+') {
      tx_ziel[p_ziel] = ' ';
    }
    else {
      tx_ziel[p_ziel] = tx_quelle[p_quelle];
    }
    p_ziel++;
    p_quelle++;
  }
  tx_ziel[p_ziel] = 0;
}
//*********************************************************************************************************************
char HexChar_to_NumChar( char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 55;
  return 0;
}
