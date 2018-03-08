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
  {"PUT"},                  // 17
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
WiFiClient client;
String content, h, s, p, c, u, n;
char character;
char* ssid[32];
char* password[32];
char* host[128];
int port;

// Navigation /////////////////////////////////////////////////////////////////

void command()
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
        case 'U': httpPutRequest();  break;
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
} // command

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

void loadIrcProfile()
{
  entry = SD.open("ircprof.dat");
  while (entry.available()) {
    c = SD.readStringUntil('\t');
    u = SD.readStringUntil('\t');
    n = SD.readStringUntil('\n');
    Serial.println("Channel: " + c);
    Serial.println("User: " + u);
    Serial.println("Nick: " + n);
    Serial.println("[L] Load  [N] Next  [X] Cancel");
    
  }

} // loadIrcProfile

void createIrcProfile()
{
  Serial.print("Enter channel name: ");
  while (Serial.available()) {
    c = Serial.readStringUntil('\n');
  }
  Serial.print("Enter username: ");
  while (Serial.available()) {
    u = Serial.readStringUntil('\n');
  }
  Serial.print("Enter nick: ");
  while (Serial.available()) {
    n = Serial.readStringUntil('\n');
  }
  entry = SD.open("ircprof.dat", FILE_WRITE);
  content = c + '\t' + u + '\t' + n;
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
  _telnetConnect();
  entry = SD.open("irchist.dat", FILE_WRITE);
  content = h + "\t" + p;
  entry.println(content);
  entry.close();
  _telnetLink();
  goToPage(3);
} // connectToIrcServer

void ircBookmarks()
{
  if (_browseConnections("ircbkmk.dat")) {  
    _telnetConnect();
    _telnetLink();
    return;
  }
  goToPage(3); 
} // ircBookmarks

void ircHistory()
{
  if (_browseConnections("irchist.dat")) {  
    _telnetConnect();
    _telnetLink();
    return;
  }
  goToPage(3);
} // ircHistory

// World Wide Web /////////////////////////////////////////////////////////////

void httpHeadRequest()
  
} // httpHeadRequest

void httpGetRequest()
  
} // httpGetRequest

void httpPutRequest()
  
} // httpPutRequest

void httpPostRequest()
  
} // httpPostRequest

// FTP Transfer ///////////////////////////////////////////////////////////////

void ftpListCommand()
{
  
} // ftpListCommand

void ftpGetCommand()
{
  
} // ftpGetCommand

void ftpPutCommand()
{
  
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
  
} // setup

void loop()
{
} // loop


