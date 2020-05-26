# AqUART
simple UART project to send sample packets to Goldline Aquarite RS485 com port

compile with:

gcc aquart.c -o aquart


run with default port (serial0):

sudo ./aquart


run with your port (ie.ttyAMA0)

sudo ./aquart ttyAMA0

Packets you create do not need the leading "10 02 50" as the program will add this.
The program will also calculate and add the checksum, and trailing "10 03" to the sending packet.
Packets you create should be delimited with a 0xff for the last byte. See samples in program.

Program now does multiple reads from rx buffer to complete rx data.
