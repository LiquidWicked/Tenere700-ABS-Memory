# Tenere700-ABS-Memory-Dongle
This project is inspired by [DKey96's project](https://github.com/DKey96/T7-ABS-Dongle).
Rewritten for [CANduino board](https://www.tindie.com/products/massivegroup/canduino-v4-atmega328p-with-can-bus/) as I had one laying around.

## Getting started

Please make sure that you really use the `v4`, because on `v3` the CS_Pin was `CS_Pin 8`!

Before we can start with the program it is important to check if the jumper `CAN_CS` (on top next to the USB-C port) is connected to a soldering point. Only if this is connected, it is possible to address the mcp2515.

The following library must be installed before use: https://github.com/autowp/arduino-mcp2515/archive/master.zip

If your CANduino is at the end of the CANbus chain, it is mandatory to connect the CAN_T jumper (next to the CAN connector)!

## Important parameters

So that the mcp2515 can be addressed it is important to set the following parameters:
```
MCP2515 mcp2515(10);
mcp2515.setBitrate("Your Bitrate", MCP_8MHZ);
```

Please note that the CAN controller uses the pins D2, D10, D11, D12 and D13 and that these can accordingly not be used or only to a limited extent when the CAN bus is active!

## Troubleshooting

If the CANduino is not displayed, please download the driver for the CP2102 which can be found here: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads


### License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.