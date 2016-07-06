/*
 *  GainSpan.h: Library to send/receive AT commands for Wifi interface to GainSpan GS1011 Module
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
 * 08-JUN-2014
 */

#ifndef GAINSPANWIFI_H
#define GAINSPANWIFI_H

#include "Arduino.h"

//#define GSW_DEBUG_TX   // View serial comms to GainSpan module
//#define GSW_DEBUG_RX   // View serial comms from GainSpan module

// Connection types:
#define GSW_INFRASTRUCTURE   0
#define GSW_ADHOC            1
#define GSW_ACCESSPOINT      2

// AT Commands:
#define GSW_RAW                 100    // Parameters: Raw ASCII command string
#define GSW_OEM                 101
#define GSW_HARDWARE            102
#define GSW_FIRMWARE            103
#define GSW_RXACTIVE            104    // Parameters: [GSW_DISABLE | GSW_ENABLE]
#define GSW_SECURITY            105    // Parameters: [GSW_AUTO | GSW_OPEN | GSW_WEP | GSW_WPAPSK | GSW_WPA2PSK]
#define GSW_MODE                106    // Parameters: [GSW_INFRASTRUCTURE | GSW_ADHOC | GSW_ACCESSPOINT]
#define GSW_DHCPSERVER          107    // Parameters: [GSW_DISABLE | GSW_ENABLE]
#define GSW_ASSOCIATE           108    // Parameters: SSID // TODO: Implement BSSID and Channel 
#define GSW_DISASSOCIATE        109
#define GSW_PASSPHRASE          110    // Parameters: Passphrase
#define GSW_UDPSERVER           111    // Parameters: Port

// AT Command Parameters
#define GSW_DISABLE             "0"
#define GSW_ENABLE              "1"
#define GSW_AUTO                "0"
#define GSW_OPEN                "1"
#define GSW_WEP                 "2"
#define GSW_WPAPSK              "4"
#define GSW_WPA2PSK             "8"
#define GSW_INFRASTRUCTURE      "0"
#define GSW_ADHOC               "1"
#define GSW_ACCESSPOINT         "2"


// Size of receive buffer from GainSpan Module
#define GSW_RESPONSE_SIZE   256


class GainSpanWifi
{
private:
  HardwareSerial _uart;
  int _power_pin;
  int _lastcommand;
  String _lastparameter;
  int _result;
  boolean _esc;
  boolean _connected;
  boolean _busy;
  boolean _failed;
  boolean _ok;
  boolean _error;
  String _lastip;
  String _lastcid;
  String _lastport;
  String _message;
  int _retry;
  char _response[GSW_RESPONSE_SIZE];
  String _dataresponse;
  int _index;
  int _initcommands;
  long _timeout;
  long _countdown;
  void _process();
  void _execute();
  void _processUDP();
public:
  GainSpanWifi(int power_pin, long timeout);
  void begin();
  void end();
  boolean busy();
  boolean error();
  boolean ok();
  boolean failed();
  boolean connected();
  void execute(int command);
  void execute(int command, String parameter);
  void update();
  boolean available();
  String read();
  String data();
  void write(String message);

};



#endif

