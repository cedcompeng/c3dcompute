/*
 *  GainSpan.cpp: Library to send/receive AT commands for Wifi inerface to GainSpan GS1011 Module
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
 * 30-MAR-2014
 */

#include "Arduino.h"
#include "GainSpanWifi.h"

// Constructor
GainSpanWifi::GainSpanWifi(int power_pin, long timeout)
{
  // This line defines a "Uart" object to access the serial port
  _uart = HardwareSerial();
  _power_pin = power_pin;
  _timeout = timeout;
  _countdown = 0;
  _busy = false;
  _error = false;
  _ok = false;
  _failed = false;
  _lastcommand = 0;
  _result = 0;
  pinMode(_power_pin, OUTPUT);
}


// Turns module power supply on and sets serial connection
void GainSpanWifi::begin() 
{
  // Turn power on to voltage regulator supplying GainSpan module
  digitalWrite(_power_pin, HIGH);  
  // Setup serial connection
  _uart.begin(9600);
  // Will execute init commands when not busy
  _busy = false;
  _initcommands = 3;
  _connected = false; 
  _esc = false; 
}


// Turns module off and releases any class resources
void GainSpanWifi::end()
{
  // Turn power off to voltage regulator supply GainSpan module
  digitalWrite(_power_pin, LOW);
  // End serial connection
  _uart.end();
  _connected = false;  
}


// Returns true if command result not received yet
boolean GainSpanWifi::busy()
{
  return _busy;
}


// Returns true if last command completed and succeeded
boolean GainSpanWifi::ok()
{
  return _ok;
}


// Returns true if last command completed and failed
boolean GainSpanWifi::failed()
{
  return _failed;
}


// Returns true if last command completed with error
boolean GainSpanWifi::error()
{
  return _error;
}


// Returns true if connection has been established
boolean GainSpanWifi::connected()
{
  return _connected;
}


// Executes command by sending to GainSpan module
void GainSpanWifi::execute(int command)
{
  _lastcommand = command;
  _retry = 3;
  _execute();
}


// Executes command with parameter 
void GainSpanWifi::execute(int command, String parameter)
{
  _lastcommand = command;
  _lastparameter = parameter;
  _retry = 3;
  _execute();
}

// Wait for data from GainSpan module
void GainSpanWifi::update()
{
  char ch;
  while(_uart.available()) 
  {
    ch = _uart.read();
    _response[_index] = ch;
    if (_index < GSW_RESPONSE_SIZE) _index++;
    if (_connected) 
    { // End of UDP or TCP packet?
      if (_esc) 
      { // <ESC> signals start or end of packet
        _countdown = 0;
        if (ch == 'u') _index = 0;
        else if (ch == 'E') 
        {
          _processUDP();
          _index = 0;
        }
      }
      if (ch == 27) _esc = true;
      else _esc = false;
    }
    if (ch == 13) _process();
  }
  if (_countdown > 0)
  {
    // Timeout?
    _countdown--;
    // Retry last command?
    if (_countdown == 0) _execute();
  }

}


// Process response
void GainSpanWifi::_process()
{
  _response[_index] = 0;
  String response = String(_response);
  response.trim();
  // Look for command responses
  if (response.indexOf("OK") >= 0) 
  {  // Got "OK" response
    _ok = true;
    if (_initcommands > 0) 
    {
      _initcommands--;
      _retry = 3;
      _execute();
    }
    else
    {
#ifdef GSW_DEBUG_RX
      if (response.length() > 1) Serial.println("[RX]"+response);
#endif
      _countdown = 0;
      _error = false;
      _failed = false;
      _ok = true;
      _busy = false;
    }
  }
  else if (response.indexOf("ERROR") >= 0) 
  {  // Had "ERROR" with command
    _error = true;
#ifdef GSW_DEBUG_RX
    Serial.println("[RX]"+response);
#endif
    _execute();
  }
  else if (response.indexOf("CONNECT") >= 0) 
  {  // Had "CONNECTED" message
    _error = true;
#ifdef GSW_DEBUG_RX
    Serial.println("[RX]"+response);
#endif
    // Setup for receiving UDP/TCP packets
    _connected = true;
    _esc = false;
    _index = 0;
    // Clear list of previously received data
    _message = "";
  }
  else
  {  // Otherwise this must be data
#ifdef GSW_DEBUG_RX
    if (response.length() > 1) Serial.println("[RX]"+response);
#endif
    _dataresponse += response;
    _index = 0;
  }
}


// Send response
void GainSpanWifi::_execute()
{
  String command;
  // Enough retries?
  if (_retry <= 0)
  {  // Failed all retries
    _failed = true;
    _busy = false;
    // Completely done if no more init commands
    if (_initcommands == 0) return;
    _initcommands--;
    _retry = 3;
    _failed = false;
  }
  _retry--;
  _error = false;
  _ok = false;
  _busy = true;
  _index = 0;
  _dataresponse = "";
  // Restart timeout countdown
  _countdown = _timeout;
  // Any init command to complete first?
  if (_initcommands > 0)
  {
    switch(_initcommands)
    {
    case 3:
      // Does nothing but generate an "OK" response
      command = "AT";
      break;
    case 2:
      // Enable ASCII codes
      command = "ATV1";
      break;
    case 1:
      // Disable echo codes
      command = "ATE0";
      break;
    }
    // Send to GainSpan module
#ifdef GSW_DEBUG_TX
    Serial.println("[TX]"+command);
#endif
    _uart.println(command);
  }
  else
  { // Execute last/current command
    switch(_lastcommand)
    {
    case GSW_RAW:
      // Send raw command
      command = _lastparameter;
      break;
    case GSW_OEM:
      // Requests OEM which should be "GainSpan"
      command = "ATI0";
      break;
    case GSW_HARDWARE:
      // Requests hardware version which should be "GS1011"
      command = "ATI1";
      break;
    case GSW_FIRMWARE:
      // Requests firmware version which must be "2.3.1" or greater
      command = "ATI2";
      break;
    case GSW_RXACTIVE:
      // Enables or disables the 802.11 radio (Enable=1, Disable=0)
      command = "AT+WRXACTIVE="+_lastparameter;
      break;
    case GSW_SECURITY:
      // Sets security mode (Auto=0, Open=1,WEP=2, WPA-PSK=4, WPA2-PSK=8)
      command = "AT+WSEC="+_lastparameter;
      break;
    case GSW_MODE:
      // Sets operating mode (Infrastructure=0, Adhoc=1, AccessPoint=2)
      command = "AT+WM="+_lastparameter;
      break;
    case GSW_DHCPSERVER:
      // Sets DHCP server when in Accesspoint mode
      command = "AT+DHCPSRVR="+_lastparameter;
      break;
    case GSW_ASSOCIATE:
      // Associate with network
      command = "AT+WA="+_lastparameter;
      break;
    case GSW_DISASSOCIATE:
      // Disassociate from network
      command = "AT+WD";
      _connected = false;        
      break;
    case GSW_PASSPHRASE:
      // Set passphrase
      command = "AT+WWPA="+_lastparameter;
      break;
    case GSW_UDPSERVER:
      // Start listening for UDP connections
      command = "AT+NSUDP="+_lastparameter;
      break;
    default:
      // Not a valid command
      _busy = false;
      _ok = false;
      _error = false;
      _failed = true;
      // Bad command, do not send string
      return;
      break;
    }
    // Send command to serial
#ifdef GSW_DEBUG_TX
    Serial.println("[TX]"+command);
#endif
    _uart.println(command);
  }
}


// Process UDP packet
void GainSpanWifi::_processUDP()
{
  // Remove trailing <ESC> command
  if (_index > 2) _index -= 2;
  _response[_index] = 0;
  String response = String(_response);
  // Get IP address
  _lastip = response.substring(1,response.indexOf(" "));
  _lastcid = String(_response[0]);
  _lastport = response.substring(response.indexOf(" ")+1, response.indexOf(String("\t")));
  String message = response.substring(response.indexOf(String("\t"))+1);
#ifdef GSW_DEBUG_RX
    Serial.println("[RXUDP]"+_lastcid+":"+_lastip+":"+message);
#endif  
  _index = 0;
  _message += message+"\n";
}


// Send UDP packet to last received IP address and port
void GainSpanWifi::write(String message)
{
  // Create packet
  String packet = _lastcid+_lastip+":"+_lastport+":"+message;
#ifdef GSW_DEBUG_RX
    Serial.println("[TXUDP]"+packet);
#endif  
  // Send UDP packet enclosed between header and footer <ESC> codes
  _uart.write(27);
  _uart.write('U');
  _uart.print(packet);
  _uart.write(27);
  _uart.write('E');
}


// Return true if data ready to be read
boolean GainSpanWifi::available()
{
  if (_message != "") return true;
  return false;
}


// Return next message from queue
String GainSpanWifi::read()
{
  String message = _message.trim();
  _message = "";
  return message;
}


// Return last data response
String GainSpanWifi::data()
{
  String data = _dataresponse.trim();
  return data;
}
