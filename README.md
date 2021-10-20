# eigenbus
This repo contains a c++ implementation of an Eigenbus Backend library.

## What is Eigenbus? 
Eigenbus is a logical protocol that is designed to run over serial links between a strict tree (no loops!) of modules. It is commonly implemented over USB and/or RS-422.

It is designed to be "Human Readable/Writeable". This feature is not perfect due to checksums and other small quirks but it is mostly true.

[EigenBus cheatsheet](https://docs.google.com/document/d/10HxQWy6gR4vNm7ubD_OZE42J9Y9vgy9ribtj6P2n49I/edit#heading=h.vee9qi9uge19)

## Features
- Automatic Topology Mapping
- Automatic Polling for Module Status
- Bootloader Compatibility
- Group Commands and Value Comparing
- Packet Statistics
- Nonvolatile Parameter Management


## How it works
The best way to learn how this library works is to dive into the code. The main file is **eigen_comms.cpp**. 

**Use the following diagram as a guide to what is going on:
[Data Flow Diagram](https://lucid.app/lucidchart/6155035b-6640-4c53-bf28-3dc01c4c0836/edit?viewport_loc=-2128%2C-1144%2C4682%2C2690%2CDqhkRMBGXt7o&invitationId=inv_611f52eb-af0c-4adf-919f-11f953776f01)**

*Disclaimer: There are some small differences between the data flow diagram and the exact behavior in the code. The data flow diagram is provided as an overview of the logical processes going on in the library.*

This library is designed to be platform independent. It can work off of any serial port implementation as long as the interface functions correctly. The Eigenbus library is designed to take control of a serial port through a "read" and "write" function that you provide. This library will take care of all the messy error handling and data structure generation and provide a simple data model for use by a driver or front end GUI.

There are a few main components used to interact with the library:
- EigenCommand: This class is used to send an Eigenbus command through the library
- EigenUpdate: This class is used to signal that a module has been changed
- EigenModule: This is the class that actually holds all of the data that the library collects



