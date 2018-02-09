
## BLE MIDI convertor
This repository is for code that converts BLE midi into DIN midi.

**Contents**
* ble-test -- simple rx of BLE packets, prints sizes
* documentation -- Right now, only output of ble-test during Tsunami app transmission
* midi-test -- simple TX of midi messages over DIN
* parserUnitTest -- Exercise of BLE MIDI packet decoder
* serial-test -- Simple serial test
* standard-midi-ble -- The main application to create a BLE MIDI dongle.
* 
### Hardware requirements
* nRF52832 Breakout
* MIDI shield -- only populate MIDI headers.

Connections:
<table>
  <tr>
    <th>nRF Pin<br></th>
    <th>MIDI Shield Pin<br></th>
  </tr>
  <tr>
    <td>GND</td>
    <td>GND<br></td>
  </tr>
  <tr>
    <td>3.3V<br></td>
    <td>5V<br></td>
  </tr>
  <tr>
    <td>26(RX)<br></td>
    <td>RX<br></td>
  </tr>
  <tr>
    <td>27(TX)<br></td>
    <td>TX<br></td>
  </tr>
  <tr>
    <td>11</td>
    <td>7 (Red LED)<br></td>
  </tr>
  <tr>
    <td>12<br></td>
    <td>6 (Green LED)<br></td>
  </tr>
</table>

### Software
**Requirements**
* nRF52832 Arduino board package
* BLE library
* MIDI library

The sparkfun nRF52 comes programed with a bootloader and has arduino board files for it, see [nRF52832 Breakout Hookup Guide](https://learn.sparkfun.com/tutorials/nrf52832-breakout-board-hookup-guide)

The hookup guide recommends a library to do the BLE stuff, written by Sandeep [BLE library](https://github.com/sandeepmistry/arduino-BLEPeripheral/) which contains a document on api calls: [API doc](https://github.com/sandeepmistry/arduino-BLEPeripheral/blob/master/API.md)

[FortySevenEffects Midi Library](https://github.com/FortySevenEffects/arduino_midi_library) is used for some of the midi decoding.

**Status**
* BLE -> MIDI port: Fully decoding all packet sizes, TX all but system
* MIDI -> BLE: Only note on/off messages

**Bugs / Future Work**
* BLE name doesn't hold, reverts to "Arduino" for some reason.
* Serial port locks at boot sometimes, needs reset button press.

**Programming tips**
* Use the side of a pen to push reset and button for bootloader
* make sure switch is on "PROG" to connect MIDI IN to RX pin
