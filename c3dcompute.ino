/*
 * This demostrates an application for CEDCOMPUTE-0
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Cedric Computer Engineering
 * www.cedric.com.au
 * 24-AUG-2014
 */


#include "GainSpanWifi.h"

// Create the GainSpan Wifi object
GainSpanWifi gswifi(2, 30000);

// Timer to regulate main loop
IntervalTimer myTimer;

char ch;
String message;
String result;
int val;
int dacout = 0;
int adc1in;
int adc2in;
long msec = 0;

int ok = false;

int autooff_sec = 300;

// Indicates connected
boolean connected = false; 

// Indicates timer done
volatile boolean timedone = false;


// Called by timer to progress main loop
void timer()
{
  timedone = true;
}


// Initialize before main loop
void setup()
{
  // Must set KILL high within 100ms of power on
  pinMode(13,  OUTPUT);
  pinMode(A14, OUTPUT);
  // Set KILL line high
  pinMode(17,  OUTPUT);
  digitalWrite(17, HIGH);
  // Set ADC inputs
  pinMode(A0, INPUT);
  pinMode(A2, INPUT);
  
  // Init serial port
  Serial.begin(9600);
  gswifi.begin();
  analogWriteResolution(12);
  analogReference(0); // 0=>3.3V //INTERNAL); // INTERNAL=>1.2V
  analogReadRes(16);
  
  digitalWrite(13, HIGH);
  delay(250);
  digitalWrite(13, LOW);
    
  // Do UDP server connection sequence for AP
  digitalWrite(13, HIGH);
  delay(750);
 /*
  execute(GSW_OEM);
  execute(GSW_RXACTIVE, GSW_ENABLE);
  execute(GSW_SECURITY, GSW_OPEN);
  execute(GSW_MODE, GSW_ACCESSPOINT);
  execute(GSW_DHCPSERVER, GSW_ENABLE);
  execute(GSW_ASSOCIATE, "Cedric", 40000);
  execute(GSW_UDPSERVER, "8888");
  // Check if connection worked
  if (ready() && gswifi.connected()) connected = true;
  digitalWrite(13, LOW);
 */
  
  myTimer.begin(timer, 1000);
}


// Main loop
void loop()
{
  gswifi.update();
  // Check for data
  if (gswifi.available()) 
  {
    message = gswifi.read();
    autooff_sec = 300;
  }

  if (Serial.available())
  {
    message = read();
  }
  
  // Parse any messages?
  if (message != "")
  {
    Serial.println(message);
    ok = false;
    
    if (message == "dacfff") 
    {
      analogWrite(A14, 0xFFF);
      ok = true;
    }
    else if (message == "dac111") 
    {
      analogWrite(A14,0x111);
      ok = true;
    }
    else if (message == "dacoff")
    {
      analogWrite(A14, 0x000);
      ok = true;
    }
    else if (message == "adc")
    {
      // Do ADC input
      adc1in = analogRead(A2);
      adc2in = analogRead(A0);
      Serial.print(adc1in,HEX);
      Serial.print("\t");
      Serial.println(adc2in,HEX);
      ok = true;
    }
    else if (message == "wifioff")
    {
      execute(GSW_DISASSOCIATE);
      gswifi.end();     
      connected = false;     
      ok = true;
    }
    if (ok) Serial.println("ok");
    else Serial.println("error");
    message = "";
  }
  
  // Flash LED to indicate connected or not
  msec++;
  if ((msec == 600) && (connected)) digitalWrite(13, HIGH);
  else if ((msec == 980) && (!connected)) digitalWrite(13, HIGH);    
  else if (msec >= 1000)
  {
    msec = 0;
    digitalWrite(13, LOW);   
    if (autooff_sec > 0)
    {
      autooff_sec--;
      if (autooff_sec == 0)
      {  // Turn off automatically
        execute(GSW_DISASSOCIATE);
        Serial.println("AUTO OFF");
        gswifi.end();     
        connected = false;     
      }
    }
  } 
  
  // Do DAC output
  
 
  // Wait until end of period
  while(!timedone) {};
  timedone = false;
}


// Execute GainSpan command after waiting for module to be ready
boolean execute(int command) 
{ 
  return execute(command, "", 5000); 
}
boolean execute(int command, String parameter)
{
  return execute(command, parameter, 5000);
}
boolean execute(int command, String parameter, int ms)
{
  while(ms > 0)
  {
    delay(1);
    gswifi.update();
    ms--;
    // Ready yet?
    if (!gswifi.busy()) 
    {  // Execute command and return
      gswifi.execute(command, parameter);
      return true;
    }
  }
  // Error, busy too long
  return false;
}


// Waits until GainSpan ready or times out
boolean ready()
{
  long ms = 40000;
  while(ms > 0)
  {
    delay(1);
    gswifi.update();
    ms--;
    // Ready yet?
    if (!gswifi.busy()) return true;
  }
  // Error, busy too long
  return false;
}


// Get command or message
String read()
{
  String command = "";
  char ch;
  long timeout = 20000;
  // Get full command
  while(timeout > 0)
  {
    if (Serial.available())
    {
      ch = Serial.read();
      // If <CR> pressed then process command without timeout
      if ((ch == 13) || (ch == 27)) timeout = 1;
      if (ch >= ' ') command += String(ch);
    }
    delay(1);
    gswifi.update();
    timeout--;
  }
  if (ch == 13) return command;
  return "";
}



