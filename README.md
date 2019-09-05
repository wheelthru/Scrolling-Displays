# Scrolling-Displays
Arduino code to scroll messages on either a MAX7219-type LED display or an "RDU-350" that Ray found.
The RDU-350 can only scroll its 20 character buffer.  This code uses an arbitrarily long buffer
with 19 spaces at the front, a message, and 19 spaces at the end.  We repeatedly print a 20 char
window from that buffer, animating a scrolling appearance.
The Arduino I set up to drive the display uses an ADM483 5V TTL-RS485 driver.  The data direction
is controlled by pin 2.  Will test at the space soon.
