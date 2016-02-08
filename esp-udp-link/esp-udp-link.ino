#include <ESP8266WiFi.h>
#include <WiFiUdp.h>


#define PPRZ_STX 0x99

/* PPRZ message parser states */
enum normal_parser_states {
  SearchingPPRZ_STX,
  ParsingLength,
  ParsingSenderId,
  ParsingMsgId,
  ParsingMsgPayload,
  CheckingCRCA,
  CheckingCRCB
};

struct normal_parser_t {
  enum normal_parser_states state;
  unsigned char length;
  int counter;
  unsigned char sender_id;
  unsigned char msg_id;
  unsigned char payload[100];
  unsigned char crc_a;
  unsigned char crc_b;
};

struct normal_parser_t parser;



/* Set these to your desired credentials. */
const char *ssid = "e540";
const char *password = "KwNiyzac";

unsigned int localPort = 4243; // port to listen on

char packetBuffer[255]; //buffer to hold incoming packet
char outBuffer[255];    //buffer to hold outgoing data
uint8_t out_idx = 0;

WiFiUDP udp;


/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */
IPAddress myIP;
IPAddress broadcastIP(10,42,0,255);

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  digitalWrite(BUILTIN_LED, HIGH);
	delay(1000);
	Serial.begin(115200);
	Serial.println();
	Serial.print("Connnecting to ");
  Serial.println(ssid);
	/* You can remove the password parameter if you want the AP to be open. */
	WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  udp.begin(localPort);
}

void loop() {
  /* Check for UDP data from host */
  int packetSize = udp.parsePacket();
  if(packetSize) { /* data received */
    //Serial.print("Received packet of size ");
    //Serial.println(packetSize);
    //Serial.print("From ");
    //IPAddress remoteIp = udp.remoteIP();
    //Serial.print(remoteIp);
    //Serial.print(", port ");
    //Serial.println(udp.remotePort());
    
    // read the packet into packetBufffer
    int len = udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len] = 0;
    //Serial.println("Contents:");
    Serial.print(packetBuffer);
    //udp.beginPacketMulticast(broadcastIP, 4242, myIP);
    //udp.write(packetBuffer);
    //udp.endPacket();
  }
  //udp.beginPacket("192.168.4.255", 4242);
  //udp.write(ReplyBuffer);
  //udp.endPacket();

  /* Check for Serial data from drone */
  /* Put all serial in_bytes in a buffer */
  while(Serial.available() > 0) {
    unsigned char inbyte = Serial.read();
    if (parse_single_byte(inbyte)) { // if complete message detected
      digitalWrite(BUILTIN_LED, LOW);
      udp.beginPacketMulticast(broadcastIP, 4242, myIP);
      udp.write(outBuffer, out_idx);
      udp.endPacket();
      delay(10);
      digitalWrite(BUILTIN_LED, HIGH);
    }
  }
}

/*
 * PPRZ-message: ABCxxxxxxxDE
    A PPRZ_STX (0x99)
    B LENGTH (A->E)
    C PPRZ_DATA
      0 SENDER_ID
      1 MSG_ID
      2 MSG_PAYLOAD
      . DATA (messages.xml)
    D PPRZ_CHECKSUM_A (sum[B->C])
    E PPRZ_CHECKSUM_B (sum[ck_a])

    Returns 0 if not ready, return 1 if complete message was detected
*/
uint8_t parse_single_byte(unsigned char in_byte)
{
  switch (parser.state) {

    case SearchingPPRZ_STX:
      out_idx = 0;
      if (in_byte == PPRZ_STX) {
        //printf("Got PPRZ_STX\n");
        parser.crc_a = 0;
        parser.crc_b = 0;
        parser.counter = 1;
        parser.state = ParsingLength;
      }
      break;

    case ParsingLength:
      parser.length = in_byte;
      parser.crc_a += in_byte;
      parser.crc_b += parser.crc_a;
      parser.counter++;
      parser.state = ParsingSenderId;
      break;

    case ParsingSenderId:
      parser.sender_id = in_byte;
      parser.crc_a += in_byte;
      parser.crc_b += parser.crc_a;
      parser.counter++;
      parser.state = ParsingMsgId;
      break;

    case ParsingMsgId:
      parser.msg_id = in_byte;
      parser.crc_a += in_byte;
      parser.crc_b += parser.crc_a;
      parser.counter++;
      parser.state = ParsingMsgPayload;
      break;

    case ParsingMsgPayload:
      parser.payload[parser.counter-4] = in_byte;
      parser.crc_a += in_byte;
      parser.crc_b += parser.crc_a;
      parser.counter++;
      if (parser.counter == parser.length - 2) {
        parser.state = CheckingCRCA;
      }
      break;

    case CheckingCRCA:
      //printf("CRCA: %d vs %d\n", in_byte, parser.crc_a);
      if (in_byte == parser.crc_a) {
        parser.state = CheckingCRCB;
      }
      else {
        parser.state = SearchingPPRZ_STX;
      }
      break;

    case CheckingCRCB:
      //printf("CRCB: %d vs %d\n", in_byte, parser.crc_b);
      if (in_byte == parser.crc_b) {
        /*printf("MSG ID: %d \t"
               "SENDER_ID: %d\t"
               "LEN: %d\t"
               "SETTING: %d\n",
               parser.msg_id,
               parser.sender_id,
               parser.length,
               parser.payload[0]);*/
        //printf("Request confirmed\n");

        /* Check what to do next if the command was received */
        outBuffer[out_idx++] = in_byte; // final byte
        parser.state = SearchingPPRZ_STX;
        return 1;
      }
      parser.state = SearchingPPRZ_STX;
      break;

    default:
      /* Should never get here */
      break;
  }
  
  outBuffer[out_idx++] = in_byte;
  return 0;
}
