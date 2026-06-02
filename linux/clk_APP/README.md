# setclk appliction
This application is used to control the configuration of the clock chip. It can also specify whether the LMK uses an on-board crystal or an external input as a reference source.

1. Use the on-board crystal oscillator as a reference.
```
./setclk 
```

2. Use external input as a reference
```
./setclk -ext
```