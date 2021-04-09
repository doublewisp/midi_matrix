# midi_matrix
Midi button matrix based on Arduino Leonardo

######Library required: 
MIDIUSB.h
Keypad.h

####Features
######Expandable matrix to control multiple buttons
The main purpose of using the keypad library is to control more buttons that a simple pin-to-button relation. The current project include a 2x2 expandable matrix (4 pins - 4 buttons). To expand it, these line has to change: 
```
const byte ROWS = 2; //desired row number
const byte COLS = 2; //desired column number

byte rowPins[ROWS] = {13, 12}; //pin used for the rows
byte colPins[COLS] = {11, 10}; //pin used for the columns
```
If you want to consequently set a default configuration these are the lines to change:
```
char keys[ROWS][COLS] = { //add more values 
{'0','1'},
{'2','3'}
};

char modeInfo[ROWS*COLS] = {'N','N','C','N'};   //store C/N info - N for note, C for control
int valueInfo[ROWS*COLS] = {12,24,0,36};   //store value info
```

######No code writable configuration
The arduino can receive serial input from the outside in a predetermined format to change the stored configuration for the buttons. This may be useful to change notes and CC values "on the go" without accessing the .ino file and re-uploading the sketch (for example during a performance).

The software receives information as
```
<N2,C23,N20,N4>
```
The buttons are separated by a **,**. The single button configuration has a "type configuration" (**N** for note or **C** for control) and the actual int value of the midi command. 
The angular parenthesis **< >** are start/end markers to tell the arduino that a new configuration is about to arrive. 

The **parseData()** function checks and load the received data to extrapolate the button configuration. 

######Handshake protocol for connection
Upon connection, the Arduino continuously wait for a string **<hello>** and then responds with a string **<ready>**. Any software/hardware that has to connect with the board has to send an "hello" and then wait for an answer. This has a few uses:
1) Avoid spurious signals: a flag can be implemented to stop the board from reading incoming signals until the handshake porcedure is completed. The flag should avoid any call to the **parseData()** function untile the "ready" string is sent.
2) Find the board between the computer signal ports: any application that wants to connect with the board could blindtest one by one the serial ports sending the expected handshake request **<hello>**. The Arduino will be connected to the one port that responds with a **<ready>** string.

####Notes and development 
1) The last three functions in the .ino files are the three MIDIUSB functions used to send midi data. Potentiometers is not currently used in this project.
2) The Pitch file is not directly used in this project but it's a reference to choose the desired notes and to implement in the future a configuration based on note names instead of progressive numbers. 
3) The handshake procedure was implemented to work with a Processing java application that I plan to upload. The flag to stop the parsing is not implemented. 
