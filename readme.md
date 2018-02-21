## BLE MIDI Converter
This repository is for code that converts BLE midi into DIN midi.  It is discussed in the SparkFun [MIDI BLE Tutorial](https://learn.sparkfun.com/tutorials/midi-ble-tutorial)

**Contents**
* standard-midi-ble -- The main application to create a BLE MIDI dongle.
* ble-to-din -- Just the BLE packet parser to serial MIDI out
* din-to-ble -- Just the serial MIDI in to BLE packet builder
* midi-lib-starter -- an empty shell with MIDI and BLE configured
* ble-starter -- an empty shell with just BLE configured
* test-programs
  * ble-test -- simple rx of BLE packets, prints sizes
  * midi-test -- simple TX of midi messages over DIN
  * parserUnitTest -- Exercise of BLE MIDI packet decoder
  * serial-test -- Simple serial test
* documentation -- Right now, only output of ble-test during Tsunami app transmission

### Hardware requirements
* nRF52832 Breakout
* MIDI shield -- only populate MIDI jacks.

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
* nRF52832 Arduino board package -- see [nRF52832 Breakout Hookup Guide](https://learn.sparkfun.com/tutorials/nrf52832-breakout-board-hookup-guide)
* [BLE library](https://github.com/sandeepmistry/arduino-BLEPeripheral/) -- [API doc](https://github.com/sandeepmistry/arduino-BLEPeripheral/blob/master/API.md)
* [FortySevenEffects Midi Library](https://github.com/FortySevenEffects/arduino_midi_library) 

**Status**
* BLE to Serial MIDI
  * Decodes all forms of BLE MIDI data
  * SysEx ignored
  * BLE timestamps ignored
* Serial MIDI to BLE
  * Converts all MIDI messages to Full type
  * Applies timestamp
  * SysEx ignored

**Bugs / Future Work**
* Serial port locks at boot sometimes, needs reset button press.
* When uploading a lot of tests, sometimes connection won't hold - connecting other devices sometimes resolves this, or try renaming the device in firmware.

