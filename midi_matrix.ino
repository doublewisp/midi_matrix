#include "MIDIUSB.h"
#include <Keypad.h>

//keypad variables
const byte ROWS = 2; //four rows
const byte COLS = 2; //three columns
char keys[ROWS][COLS] = {
{'0','1'},
{'2','3'}
};
byte rowPins[ROWS] = {13, 12}; //connect to the row pinouts of the kpd
byte colPins[COLS] = {11, 10}; //connect to the column pinouts of the kpd
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

/////midi variables
byte midiCh = 1;
byte note = 0; //lowest midi note 
byte cc = 0; //potentiometer control

//potentiometer variables
const int NPots = 1;      //number of potentiometers
int potPin[NPots] = {A0};   //pins for potentiometers
int potCState[NPots] = {0};
int potPState[NPots] = {0};
int potVar = 0;
int midiCState[NPots] = {0};
int midiPState[NPots] = {0};
int TIMEOUT = 300;  //readings after threshold
int varThreshold = 10; //pot variation threshold
boolean potMoving = true;
unsigned long PTime[NPots] = {0};
unsigned long timer[NPots] = {0};

//serial comm variables
const byte numChars = 32;       //max byte of single incoming string
char receivedChars[numChars];   //store incoming data
boolean newData = false;        //true if new data is present
char tempChars[numChars];        // temporary array for use when parsing
// variables to hold the parsed data
char modeInfo[ROWS*COLS] = {'N','N','C','N'};   //store C/N info
int valueInfo[ROWS*COLS] = {12,24,0,36};   //store value info
char handshakeRequest[] = "hello";
char handshakeAnswer[] = "<ready>";


void setup() {
    Serial.begin(9600);
}


void loop() {
    
    potentiometers();

    // Fills kpd.key[ ] array with up-to 10 active keys.
    // Returns true if there are ANY active keys.
    if (kpd.getKeys())
    {
        for (int i=0; i<LIST_MAX; i++)   // Scan the whole key list.
        {
            if ( kpd.key[i].stateChanged )   // Only find keys that have changed state.
            {
                switch (kpd.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
                    case PRESSED:
                   
                    if(modeInfo[kpd.key[i].kcode] == 'N'){   //if it's a note
                      noteOn(midiCh, note + valueInfo[kpd.key[i].kcode], 127);
                    }
                    if(modeInfo[kpd.key[i].kcode] == 'C'){   //if it's a CC
                      controlChange(midiCh, cc + valueInfo[kpd.key[i].kcode], 127);
                    }
                    MidiUSB.flush();
                break;
                    case HOLD:
                    
                break;
                    case RELEASED:
                    if(modeInfo[kpd.key[i].kcode] == 'N'){
                      noteOn(midiCh, note + valueInfo[kpd.key[i].kcode], 0);
                    }
                    if(modeInfo[kpd.key[i].kcode] == 'C'){
                      controlChange(midiCh, cc + valueInfo[kpd.key[i].kcode], 0);
                    }
                    MidiUSB.flush();
                break;
                
                }
            }
        }
    }

    recvWithStartEndMarkers();  //Check markers for incoming data
    if (newData == true) {
        checkData();  //check if received data is valid
        newData = false;
    }
    
}  // End loop

void recvWithStartEndMarkers() {    //sets markers for incoming data
    static boolean recvInProgress = false;
    static byte ndx = 0;  //index for data reading
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) { //store incoming char between markers
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) { //start marker detection
            recvInProgress = true;
        }
    }
}

void checkData() {
    //Serial.println(receivedChars);
    if(strcmp(receivedChars, "readConfig") == 0){ //if command to read is received, send stored configuration
      sendConfiguration();
    } else if(strcmp(receivedChars, handshakeRequest) == 0) { //check if received data is = handshake command 1
      Serial.println(handshakeAnswer); // send handshake command 2
    }
    else {
    strcpy(tempChars, receivedChars);  //temporary copy is necessary to protect the original data: strtok() commas with \0
    parseData(); //parse read data
    }
}

void parseData() {      // split the data into its parts and store them
    char * strtokIndx; // this is used by strtok() as an index

       strtokIndx = strtok(tempChars,",");      // get the first part - the string
       modeInfo[0] = strtokIndx[0];             //put first char of token into modeInfo
       valueInfo[0] = atoi(&strtokIndx[1]);     //put rest of token into integer in value info
       for(int i=1; i<ROWS*COLS; i++){
         strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
         modeInfo[i] = strtokIndx[0];     
         valueInfo[i] = atoi(&strtokIndx[1]);
       }
    //showParsedData();   
}

void showParsedData() {
    //sendConfiguration();  
}

void sendConfiguration(){ //send stored configuration
  String toSendChars = "<";
      for(int i=0; i<ROWS*COLS; i++){
        toSendChars += modeInfo[i];
        toSendChars += valueInfo[i];
        if(i!=ROWS*COLS-1){
        toSendChars += ',';
      }
    }
     toSendChars += '>';
     Serial.println(toSendChars);
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void potentiometers() {

  for (int i = 0; i<NPots; i++) {
    potCState[i] = analogRead(potPin[i]);
    midiCState[i] = map(potCState[i], 0, 1023, 127, 0);
    potVar = abs(potCState[i] - potPState[i]);
    if (potVar > varThreshold) {
      PTime[i] = millis();
    }
    timer[i] = millis() - PTime[i];
    if (timer[i] < TIMEOUT) {
      potMoving = true;
    }
    else {
      potMoving = false;
    }
    if (potMoving == true) {
      if (midiPState[i] != midiCState[i]) {
        controlChange(midiCh, cc+i, midiCState[i]);
        MidiUSB.flush();
        potPState[i] = potCState[i];
        midiPState[i] = midiCState[i];
}}}}
