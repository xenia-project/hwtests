Xenia HW Test Suite
===================

This is a test suite intended to run against the emulator and the Xenon
processor to ensure accuracy.

## Requirements
* A PowerPC cross-compiler (g++-powerpc-linux-gnu)
  * Bash on Windows works fine.
* An xbox 360 capable of running linux (for hardware testing)

## Running on Xenia
Run the suite the same way you would run a xbox 360 game.

Output is sent through PowerPC debug prints (seen with --enable_debugprint_log=true).