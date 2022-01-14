# els - Extended LS
## History
This is a fork of the work found at:

- els-1.54a1.tar.gz from http://els-software.org

I have modified it for my own person use.

*No claims are made to the reliability or safety of those changes.*

Extended LS is a very extended version of 'ls' by the original author.  I have only made a few minor tweaks, such as adding color and glyph support.

README-ORIG is the original README from els-software.org.
All of the other text files and documentation are unmodified from the original source.

## Compiling
This has compiled on OS X, Big Sur, not tested on other platforms.

- make config
- make all

binaries will be in:  Build/els/bin

## Features
- add commas to file sizes
- colorize file size column
- colorize date/time by age
- add glyphs to show file types

## Using
These are zsh functions I use to call it.

Note: You will need to remove the "| mc" or also install it.
mc turns "line" output into multi-columns

- l  () { els +T^NY-M-DT +G~Atp~ugsmNL $@ }
![l]<img/l.png>

- ll () { els +T^NY-M-DT +G~At~smN $@ | mc }
![ll]<img/ll.png>

- ls () { els +G~t~N $@ | mc }
![ls]<img/ls.png>

- lt () { els +T^NY-M-DT +G~At~smN -rt $@ | mc }
![lt]<img/lt.png>
