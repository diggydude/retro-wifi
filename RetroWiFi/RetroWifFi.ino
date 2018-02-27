#include <Messenger.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <XModem.h>

#define MODE_PASSTHRU 0;
#define MODE_COMMAND  1;

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
  {"Save File to Card"}     // 22
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

byte mode = MODE_COMMAND;
byte i = 0;
byte currPage = -1;
Messenger message = Messenger();
File root, entry;
WiFiClient client;
String content, h, p;
char character;
char* ssid[32];
char* password[32];
char* host[128];
int port;

// Navigation /////////////////////////////////////////////////////////////////

void printMenu()
{
  Serial.println("--------------------");
  Serial.println(menuLabels[menus[currPage]].text);
  Serial.println("--------------------");
  for (i = 0; i < 35; i++) {
    if (menuItems[i].menuNumber == currPage) {
      Serial.print("[");
      Serial.print(menuItems[i].hotkey);
      Serial.print("] ");
      Serial.println(menuLabels[menuItems[i].label].text);
    }
  }
  Serial.println("--------------------");
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
  String s, p;
  content = "";
  entry = SD.open("wifiprev.dat");
  if (!entry) {
    Serial.println("No previous network connection found.");
    gotToPage(1);
    return;
  }
  while(entry.available()) {
    content += entry.read();
  }
  entry.close();
  content.trim();
  s = content.substring(0, content.indexOf("\t") - 1);
  p = content.substring(content.indexOf("\n") + 1);
  s.toCharArray(ssid, 32);
  p.toCharArray(password, 32);
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
    ssid = Serial.readUntil('\n');
  }
  Serial.println("");
  Serial.print("Enter password: ");
  while (Serial.available()) {
    password = Serial.readUntil('\n');
  }
  _wifiConnect();
  if (SD.exists("wifiprev.dat")) {
    SD.remove("wifiprev.dat");
  }
  entry = SD.open("wifiprev.dat", FILE_WRITE);
  content = ssid + "\t" + password;
  entry.println(content);
  entry.close();
  goToPage(1);
} // connectToNetwork

// Bookmarks and history

void _readLine()
{
  while (entry.available()) {
    character = entry.read();
    if (character == '\n') {
      break;
    }
    content += character;
  }
  h = content.substring(0, content.indexOf("\t") - 1);
  p = content.substring(content.indexOf("\t") + 1);
  h.toCharArray(host, 128);
  port = int(p);
} // _readLine

bool _browseConnections(const char filename)
{
  entry = SD.open(filename);
  while (entry,avaialble()) {
    _readLine();
    Serial.print("[C} Connect  [N] Next  [X] Cancel");
      while (Serial.available()) {
      command = Serial.readStringUntil('\n');
    }
    if (command == "C") {
      entry.close();
      return true;
    }
      else if (command == "N") {
      continue;
    }
    else if (command == "X") {
      entry.close();
      return false;
    }
    else {
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
  if (!client.connect(host, port)) {
    Serial.println("Connection failed.");
    goToPage(2);
  }
} // _telnetConnect

void _telnetLink()
{
  while (true) {
    while(client.available()) {
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
    host = Serial.readUntil('\n');
  }
  Serial.print("Enter port number: ");
  while (Serial.available()) {
    port = int(Serial.readUntil('\n'));
  }
  _telnetConnect();
  entry = SD.open("tnethist.dat", FILE_WRITE);
  content = host + "\t" + port;
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
  
} // loadIrcProfile

void createIrcProfile()
{
  
} // createIrcProfile

void connectToIrcServer()
{
  
} // connectToIrcServer

void ircBookmarks()
{
  
} // ircBookmarks

void ircHistory()
{
  
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

// Command interpreter ////////////////////////////////////////////////////////

void messageReceived()
{
  switch (currPage) {
    default:
    case 0: // Main Menu
      if (message.checkString("W")) goToPage(1);
      else if (message.checkString("T")) goToPage(2);
      else if (message.checkString("I")) goToPage(3);
      else if (message.checkString("W")) goToPage(4);
      else if (message.checkString("F")) goToPage(5);
      else if (message.checkString("S")) goToPage(6);
      else if (message.checkString("Q")) quit();
      else {
        Serial.println("Invalid command.");
        printMenu();
      }
      break;
    case 1: // WiFi Settings
      if (message.checkString("U")) usePrevious();
      else if (message.checkString("S")) scanForNetworks(); 
      else if (message.checkString("C")) connectToNetwork();
      else if (message.checkString("M")) goToPage(0);
      else {
        Serial.println("Invalid command.");
        printMenu();
      }
      break;
    case 2: // Telnet
      if (message.checkString("C")) connectToTelnetAddr();
      else if (message.checkString("B")) telnetBookmarks();
      else if (message.checkString("H")) telnetHistory();
      else if (message.checkString("M")) goToPage(0);
      else {
        Serial.println("Invalid command.");
        printMenu();
      }
      break;
    case 3: // Internet Relay Chat
      if (message.checkString("P")) loadIrcProfile();
      else if (message.checkString("N")) createIrcProfile();
      else if (message.checkString("C")) connectToIrcServer();
      else if (message.checkString("B")) ircBookmarks();
      else if (message.checkString("H")) ircHistory();
      else if (message.checkString("M")) goToPage(0);
      else {
        Serial.println("Invalid command.");
        printMenu();
      }
      break;
    case 4: // World Wide Web
      if (message.checkString("H")) httpHeadRequest();
      else if (message.checkString("G")) httpGetRequest();
      else if (message.checkString("U")) httpPutRequest();
      else if (message.checkString("P")) httpPostRequest();
      else if (message.checkString("M")) goToPage(0);
      else {
        Serial.println("Invalid command.");
        printMenu();
      }
      break;
    case 5: // FTP Transfer
      if (message.checkString("L")) ftpListCommand();
      else if (message.checkString("G")) ftpGetCommand();
      else if (message.checkString("P")) ftpPutCommand();
      else if (message.checkString("M")) goToPage(0);
      else {
       Serial.println("Invalid command.");
       printMenu();
      }
      break;
    case 6: // SD Card
      if (message.checkString("D")) listFiles();
      else if (message.checkString("L")) loadFile();
      else if (message.checkString("S")) saveFile();
      else if (message.checkString("M")) goToPage(0);
      else {
        Serial.println("Invalid command.");
        printMenu();
      }
      break;
} // messageReceived

// Application ////////////////////////////////////////////////////////////////

void setup()
{
} // setup

void loop()
{
} // loop


