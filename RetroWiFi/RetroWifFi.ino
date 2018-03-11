#include <ESP8266WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <XModem.h>

struct MenuLabel 
{
  char text[20];
};

struct MenuItem {
  char hotkey;
  byte menuNumber;
  byte label;
};

MenuLabel menuLabels[23] = {
  {"Wifi Settings"},        // 0
  {"Telnet"},               // 1
  {"Internet Relay Chat"},  // 2
  {"World Wide Web"},       // 3
  {"FTP Transfer"},         // 4
  {"SD Card"},              // 5
  {"Quit"},                 // 6
  {"Use Previous"},         // 7
  {"Scan for Networks"},    // 8
  {"Connect to..."},        // 9
  {"Main Menu"},            // 10
  {"Bookmarks"},            // 11
  {"History"},              // 12
  {"Load User Profile"},    // 13
  {"Create New Profile"},   // 14
  {"HEAD"},                 // 15
  {"GET"},                  // 16
  {"Upload"},               // 17
  {"POST"},                 // 18
  {"LIST"},                 // 19
  {"List Files on Card"},   // 20
  {"Load File from Card"},  // 21
  {"Save File to Card"},    // 22
  {"--------------------"}  // 23
};

byte menus[7] = {10, 0, 1, 2, 3, 4, 5};

MenuItem menuItems[34] = {
  // Main Menu
  {'W', 0, 0},
  {'T', 0, 1},
  {'I', 0, 2},
  {'B', 0, 3},
  {'F', 0, 4},
  {'S', 0, 5},
  {'Q', 0, 6},
  // WiFi Settings
  {'U', 1, 7},
  {'S', 1, 8},
  {'C', 1, 9},
  {'M', 1, 10},
  // Telnet
  {'C', 2, 9},
  {'B', 2, 11},
  {'H', 2, 12},
  {'M', 2, 10},
  // Internet Relay Chat
  {'P', 3, 13},
  {'N', 3, 14},
  {'C', 3, 9},
  {'B', 3, 11},
  {'H', 3, 12},
  {'M', 3, 10},
  // World Wide Web
  {'H', 4, 15},
  {'G', 4, 16},
  {'U', 4, 17},
  {'P', 4, 18},
  {'M', 4, 10},
  // FTP Transfer
  {'L', 5, 19},
  {'G', 5, 16},
  {'U', 5, 17},
  {'M', 5, 10},
  // SD Card
  {'D', 6, 20},
  {'L', 6, 21},
  {'S', 6, 22},
  {'M', 6, 10}
};

byte i = 0;
byte currPage = -1;
File root, entry;
WiFiClient client, ftp;
String content, h, s, p, channel, user, nick, uri, filename;
char character;
char* ssid[32];
char* password[32];
char* host[128];
int port;
unsigned long timeout;

// Navigation /////////////////////////////////////////////////////////////////

void getCommand()
{
  character = char(Serial.readStringUntil('\n'));
  switch (currPage) {
    default:
    case 0: // Main Menu
      switch (character) {
        case 'W': goToPage(1); break;
        case 'T': goToPage(2); break;
        case 'I': goToPage(3); break;
        case 'W': goToPage(4); break;
        case 'F': goToPage(5); break;
        case 'S': goToPage(6); break;
        case 'Q': quit();
        default:
          Serial.println("Invalid command.");
          printMenu();
          break;
      }
    case 1: // WiFi Settings
      switch (character) {
        case 'U': usePrevious();      break;
        case 'S': scanForNetworks();  break; 
        case 'C': connectToNetwork(); break;
        case 'M': goToPage(0);        break;
        default:
          Serial.println("Invalid command.");
          printMenu();
          break;
      }
    case 2: // Telnet
      switch (character) {
        case 'C': connectToTelnetAddr(); break;
        case 'B': telnetBookmarks();     break;
        case 'H': telnetHistory();       break;
        case 'M': goToPage(0);           break;
        default:
          Serial.println("Invalid command.");
          printMenu();
          break;
      }
    case 3: // Internet Relay Chat
      switch (character) {
        case 'P': loadIrcProfile();     break;
        case 'N': createIrcProfile();   break;
        case 'C': connectToIrcServer(); break;
        case 'B': ircBookmarks();       break;
        case 'H': ircHistory();         break;
        case 'M': goToPage(0);          break;
        default:
          Serial.println("Invalid command.");
          printMenu();
          break;
      }
    case 4: // World Wide Web
	  switch (character) {
        case 'H': httpHeadRequest(); break;
        case 'G': httpGetRequest();  break;
        case 'U': httpPostUpload();  break;
        case 'P': httpPostRequest(); break;
        case 'M': goToPage(0);       break;
        default:
          Serial.println("Invalid command.");
          printMenu();
          break;
      }
    case 5: // FTP Transfer
      switch (character) {
        case 'L': ftpListCommand(); break;
        case 'G': ftpGetCommand();  break;
        case 'P': ftpPutCommand();  break;
        case 'M': goToPage(0);      break;
        default:
          Serial.println("Invalid command.");
          printMenu();
          break;
      }
    case 6: // SD Card
      switch (character) {
        case 'D': listFiles(); break;
        case 'L': loadFile();  break;
        case 'S': saveFile();  break;
        case 'M': goToPage(0); break;
        default:
          Serial.println("Invalid command.");
          printMenu();
          break;
      }
  }
} // getCommand

void printMenu()
{
  Serial.println(menuLabels[23]);
  Serial.println(menuLabels[menus[currPage]].text);
  Serial.println(menuLabels[23]);
  for (i = 0; i < 35; i++) {
    if (menuItems[i].menuNumber == currPage) {
      Serial.print("[");
      Serial.print(menuItems[i].hotkey);
      Serial.print("] ");
      Serial.println(menuLabels[menuItems[i].label].text);
    }
  }
  Serial.println(menuLabels[23]);
  Serial.print("Enter selection: ");
} // printMenu

void goToPage(byte pageNum)
{
  currPage = pageNum;
  printMenu();
} // goToPage

// WiFi Settings //////////////////////////////////////////////////////////////

void _wifiConnect()
{
  s.toCharArray(ssid, 32);
  p.toCharArray(password, 32);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);  
  Serial.print("Connecting to ");
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected! IP address: ");
  Serial.println(WiFi.localIP());
} // _wifiConnect

void usePrevious()
{
  content = "";
  entry = SD.open("wifiprev.dat");
  if (!entry) {
    Serial.println("No previous network connection found.");
    gotToPage(1);
    return;
  }
  if (entry.available()) {
    s = entry.readStringUntil('\t');
    p = entry.readStringUntil('\n');
  }
  entry.close();
  _wifiConnect();
  goToPage(1);
} // usePrevious

void scanForNetworks()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("No networks found.");
    goToPage(1);
    return;
  }
  Serial.print("Found ");
  Serial.print(n);
  Serial.println(" network(s).");
  for (i = 0; i < n; ++i) {
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.print(")");
    Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
    delay(10);
  }
  Serial.println("");
  goToPage(1);
} // scanForNetworks

void connectToNetwork()
{
  Serial.print("Enter SSID: ");
  while (Serial.available()) {
    s = Serial.readStringUntil('\n');
  }
  Serial.println("");
  Serial.print("Enter password: ");
  while (Serial.available()) {
    p = Serial.readStringUntil('\n');
  }
  _wifiConnect();
  if (SD.exists("wifiprev.dat")) {
    SD.remove("wifiprev.dat");
  }
  entry = SD.open("wifiprev.dat", FILE_WRITE);
  content = s + "\t" + p;
  entry.println(content);
  entry.close();
  goToPage(1);
} // connectToNetwork

// Bookmarks and history //////////////////////////////////////////////////////

void _readLine()
{
  if (entry.available()) {
    h = entry.readStringUntil('\t');
    p = entry.readStringUntil('\n');
  }
} // _readLine

bool _browseConnections(const char filename)
{
  entry = SD.open(filename);
  while (entry.avaialble()) {
    _readLine();
    Serial.println(h + ':' + p);
    Serial.println("[C] Connect  [N] Next  [X] Cancel");
    while (Serial.available()) {
      command = char(Serial.readStringUntil('\n'));
    }
    switch (command) {
      case 'C':
        entry.close();
        return true;
      case 'N':
        break;
      case 'X':
        entry.close();
        return false;
      default:
        Serial.println("Invalid command.");
        entry.close();
        return false;
    }
  }
  entry.close();
  return false;
} // _browseConnections

// Telnet /////////////////////////////////////////////////////////////////////

void _telnetConnect()
{
  h.toCharArray(host, 128);
  port = int(p);
  if (!client.connect(host, port)) {
    Serial.println("Connection failed.");
    goToPage(2);
  }
} // _telnetConnect

void _telnetLink()
{
  while (true) {
    while (client.available()) {
      Serial.println(client.readStringUntil('\n'));
    }
    delay(100);
    while (Serial.available()) {
      content = Serial.readStringUntil('\n');
      if (content.equalsIgnoreCase("quit")) {
        client.println(content);
        client.stop();
        goToPage(2);
        return;
      }
      client.println(content);
    }
    delay(100);
  }
} // _telnetLink

void connectToTelnetAddr()
{
  Serial.print("Enter host: ");
  while (Serial.available()) {
    h = Serial.readStringUntil('\n');
  }
  Serial.print("Enter port number: ");
  while (Serial.available()) {
    p = Serial.readStringUntil('\n');
  }
  _telnetConnect();
  entry = SD.open("tnethist.dat", FILE_WRITE);
  content = h + "\t" + p;
  entry.println(content);
  entry.close();
  _telnetLink();
  goToPage(2);
} // connectToTelnetAddr

void telnetBookmarks()
{
  if (_browseConnections("tnetbkmk.dat")) {  
    _telnetConnect();
    _telnetLink();
    return;
  }
  goToPage(2);
} // telnetBookmarks

void telnetHistory()
{
  if (_browseConnections("tnethist.dat")) {
    _telnetConnect();
    _telnetLink();
    return;
  }
  goToPage(2);
} // telnetHistory

// Internet Relay Chat ////////////////////////////////////////////////////////

void _ircChat()
{
  _telnetConnect();
  entry = SD.open("irchist.dat", FILE_WRITE);
  content = h + "\t" + p;
  entry.println(content);
  entry.close();
  while (client,available()) {
    Serial.println(client.readStringUntil('\n'));
  }
  client.print("NICK ");
  client.println(nick);
  client.print("USER ");
  client.print(user);
  client.println(" 8 *");
  while (client.available()) {
    Serial.println(client.readStringUntil('\n'));
  }
  client.print("JOIN #");
  client.println(channel);
  while (client.available()) {
    Serial.println(client.readStringUntil('\n'));
  }
  while (true) {
    while (client.available()) {
      Serial.println(client.readStringUntil('\n'));
    }
    delay(100);
    while (Serial.available()) {
      content = Serial.readStringUntil('\n');
      if (content.equalsIgnoreCase("quit")) {
        content.toUpperCase();
        client.println(content);
        while (client.available()) {
          Serial.println(client.readStringUntil('\n'));
        }
        client.stop();
        goToPage(3);
        return;
      }
      client.print("PRIVMSG #");
      client.print(channel);
      client.print(":");
      client.println(content);
    }
    delay(100);
  }
  client.stop();
  goToPage(3);
} // _ircChat

void loadIrcProfile()
{
  entry = SD.open("ircprof.dat");
  while (entry.available()) {
    channel = SD.readStringUntil('\t');
    user    = SD.readStringUntil('\t');
    nick    = SD.readStringUntil('\n');
    Serial.println("Channel: " + channel);
    Serial.println("User: " + uusr);
    Serial.println("Nick: " + nick);
    Serial.println("[L] Load  [N] Next  [X] Cancel");
    while (Serial.available()) {
      command = char(Serial.readStringUntil('\n'));
    }
    switch (command) {
      case 'L':
      case 'X':
        break;
    }
  }
  entry.close();
} // loadIrcProfile

void createIrcProfile()
{
  Serial.print("Enter channel name: ");
  while (Serial.available()) {
    channel = Serial.readStringUntil('\n');
  }
  Serial.print("Enter username: ");
  while (Serial.available()) {
    user = Serial.readStringUntil('\n');
  }
  Serial.print("Enter nick: ");
  while (Serial.available()) {
    nick = Serial.readStringUntil('\n');
  }
  entry = SD.open("ircprof.dat", FILE_WRITE);
  content = channel + '\t' + user + '\t' + nick;
  entry.println(content);
  entry.close();
  goToPage(3);
} // createIrcProfile

void connectToIrcServer()
{
  Serial.print("Enter host: ");
  while (Serial.available()) {
    h = Serial.readStringUntil('\n');
  }
  Serial.print("Enter port number: ");
  while (Serial.available()) {
    p = Serial.readStringUntil('\n');
  }
  _ircChat();
} // connectToIrcServer

void ircBookmarks()
{
  if (_browseConnections("ircbkmk.dat")) {  
    _ircChat();
    return;
  }
  goToPage(3); 
} // ircBookmarks

void ircHistory()
{
  if (_browseConnections("irchist.dat")) {  
    _ircChat();
    return;
  }
  goToPage(3);
} // ircHistory

// World Wide Web /////////////////////////////////////////////////////////////

void _httpConnect()
{
  Serial.print("HTTP host: ");
  while (Serial.available()) {
    h = Serial.readStringUntil('\n');
  }
  h.toCharArray(host, 128);
  Serial.print("URI: ");
  while (Serial.available()) {
    uri = Serial.readStringUntil('\n');
  }
  if (!client.connect(host, 80)) {
    Serial.println("Connection failed.");
    goToPage(4);
  }
} // _httpConnect

void httpHeadRequest()
{
  _httpConnect();
  client.print("HEAD ");
  client.print(uri);
  client.print(" HTTP/1.1\r\n");
  client.print("Host: ");
  client.print(host);
  client.print("\r\n");
  client.print("Connection: close\r\n\r\n");
  timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("Connection timed out.");
      client.stop();
      gotToPage(4);
      return;
    }
  }
  while (client.available()) {
    Serial.println(client.readStringUntil('\r'));
  }
  Serial.println();
  Serial.println("Closing connection.");
  client.stop();
  goToPage(4);
} // httpHeadRequest

void httpGetRequest()
{
  Serial.print("Save to file: ");
  while (Serial.available()) {
    filename = Serial.readStringUntil('\n');
  }
  entry = SD.open(filename, FILE_WRITE);
  _httpConnect();
  client.print("GET ");
  client.print(uri);
  client.print(" HTTP/1.1\r\n");
  client.print("Host: ");
  client.print(host);
  client.print("\r\n");
  client.print("Connection: close\r\n\r\n");
  timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("Connection timed out.");
      client.stop();
      gotToPage(4);
      return;
    }
  }
  while (client.available()) {
    entry.println(client.readStringUntil('\n'));
  }
  entry.close();
  Serial.println("File downloaded.");
  client.stop();
  goToPage(4);
} // httpGetRequest

void httpPostRequest()
{
  Serial.print("Request body: ");
  while (Serial.available()) {
    body = Serial.readStringUntil('\n');
  }
  _httpConnect();
  client.print("POST ");
  client.print(uri);
  client.print(" HTTP/1.1\r\n");
  client.print("Host: ");
  client.print(host);
  client.print("\r\n");
  client.print("Connection: close\r\n\r\n");
  timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("Connection timed out.");
      client.stop();
      gotToPage(4);
      return;
    }
  }
  while (client.available()) {
    entry.println(client.readStringUntil('\n'));
  }
  Serial.println();
  Serial.println("Closing connection.");
  client.stop();
  goToPage(4);
} // httpPostRequest

void httpPostUpload()
{
  Serial.print("Local file: ");
  while (Serial.available()) {
    filename = Serial.readStringUntil('\n');
  }
  entry  = SD.open(filename);
  length = sizeof(filename) + entry.size() + 233;
  _httpConnect();
  client.print("POST /upload.php HTTP/1.0\r\n");
  client.print("Host: ");
  client.print(host);
  client.print("\r\n");
  client.print("Content-Type: multipart/form-data, boundary=AaB03x\r\n");
  client.print("Content-Length: ");
  client.print(length);
  client.print("\r\n\r\n");
  client.print("--AaB03x\r\n");
  client.print("Content-Disposition: form-data; name=\"MAX_FILE_SIZE\"\r\n\r\n");
  client.print("1024\r\n");
  client.print("--AaB03x\r\n");
  client.print("Content-Disposition: form-data; name=\"upload\"; filename=\"");
  client.print(filename);
  client.print("\"\r\n");
  client.print("Content-Type: application/octet-stream\r\n");
  client.print("Content-Transfer-Encoding: binary\r\n\r\n");
  while (entry.available()) {
    client.write(entry.read());
  }
  client.print("\r\n--AaB03x--\r\n");
  entry.close();
  client.stop();
  Serial.println("File uploaded.");
  goToPage(4);
} // httpPostUpload

// FTP Transfer ///////////////////////////////////////////////////////////////

void _ftpConnect()
{
  content = "";
  Serial.print("Host: ");
  while (Serial.available()) {
    h = Serial.readStringUntil('\n');
  }
  Serial.print("Username: ");
  while (Serial.available()) {
    user = Serial.readStringUntil('\n');
  }
  Serial.print("Password: ");
  while (Serial.available()) {
    p = Serial.readStringUntil('\n');
  }
  _telnetConnect();
  while (client.available()) {
    Serial.println(client.readStringUntil('\n'));
  }
  client.print("USER ");
  client.println(user);
  while (client.available()) {
    Serial.println(client.readStringUntil('\n'));
  }
  client.print("PASS ");
  client.println(password);
  while (client.available()) {
    Serial.println(client.readStringUntil('\n'));
  }
  client.println("PASV");
  while (client.available()) {
    i = byte(client.readStringUntil(' '));
    if (i != 227) {
      client.stop();
      Serial.println("Transfer connection failed.");
      goToPage(5);
      return;
    }
    character = client.read();
    if (!isDigit(character)) {
      continue;
    }
    h += client.readStringUntil(',');
    h += ".";
    h += client.readStringUntil(',');
    h += ".";
    h += client.readStringUntil(',');
    h += ".";
    h += client.readStringUntil(',');
    h.toCharArray(host, 128);
    port = int(client.readStringUntil(',')) * 256;
  }
  p = "";
  while (client.available()) {
    character = client.read();
    if (isDigit(character)) {
      p += character;
    }
  }
  port += int(p);
  ftp.connect(host, port);
} // _ftpConnect

void ftpListCommand()
{
  _ftpConnect();
  client.println("LIST");
  while (ftp.available()) {
    Serial.println(ftp.readStringUntil('\n'));
  }
} // ftpListCommand

void ftpGetCommand()
{
  Serial.print("Local file: ");
  while (Serial.available()) {
    filename = Serial.readStringUntil('\n');
  }
  _ftpConnect();
  Serial.println("Mode: [A] ASCII  [B] Binary");
  while (Serial.available()) {
    command = Serial.readStringUntil('\n');
  }
  switch (command) {
    case 'A':
    default:
      client.println("TYPE A");
    case 'B':
      client.println("TYPE I");
  }
  SD.remove(filename);
  entry = SD.open(filename, FiLE_WRITE);
  client.print("RETR ");
  client.println(filename);
  while (ftp.available()) {
    entry.write(ftp.read());
  }
  ftp.stop();
  client.stop();
  entry.close();
  Serial.println("File downloaded.");
  goToPage(5);
} // ftpGetCommand

void ftpPutCommand()
{
  Serial.print("Local file: ");
  while (Serial.available()) {
    filename = Serial.readStringUntil('\n');
  }
  _ftpConnect();
  Serial.println("Mode: [A] ASCII  [B] Binary");
  while (Serial.available()) {
    command = Serial.readStringUntil('\n');
  }
  switch (command) {
    case 'A':
    default:
      client.println("TYPE A");
    case 'B':
      client.println("TYPE I");
  }
  entry = SD.open(filename);
  client.print("STOR ");
  client.println(filename);
  while (entry.available()) {
    ftp.write(entry.read());
  }
  ftp.stop();
  client.stop();
  entry.close();
  Serial.println("File uploaded.");
  goToPage(5); 
} // ftpPutCommand

// SD Card ////////////////////////////////////////////////////////////////////

void listFiles()
{
  root = SD.open("/");
  while (true) {
    entry = root.openNextFile();
    if (!entry) {
      break;
    }
    Serial.print(entry.name());
    Serial.print("\t\t");
    if (entry.isDirectory()) {
      Serial.println("<DIR>");
    }
    else {
      Serial.print(entry.size(), DEC);
      Serial.println(" bytes");
    }
    entry.close();
  }
  goToPage(6);
} // listFiles

void loadFile()
{
  
} // loadFile

void saveFile()
{
  
} // saveFile

// Application ////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(1200);
  SD.begin(4);
  goToPage(0);
} // setup

void loop()
{
  getCommand();
} // loop


