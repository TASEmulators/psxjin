## Header ##
```
000 4-byte signature: "PJM "
004 4-byte unsigned long: version number (currently "2")
008 4-byte unsigned long: version of the emulator used
00C 2-byte flags:
    bit 0: reserved, set to 0
    bit 1:
          if "0", movie begins from power-on
          if "1", movie begins from an embedded savestate
    bit 2:
          if "0", NTSC timing
          if "1", PAL timing
    bit 3:
          if "0", movie does not contain embedded memory cards
          if "1", movie does contain embedded memory cards
    bit 4:
          if "0", movie does not contain embedded cheat list
          if "1", movie does contain embedded cheat list
    bit 5:
          if "0", movie does not use hacks
          if "1", movie does use hacks such as "SPU/SIO IRQ always enabled"
    bit 6: 
          if "0", movie is in Binary format
          if "1", movie is in text format
    bit 7:
          Multitap toggle, player 1
    bit 8:
          Multitap toggle, player 2
    bit 9: 
          Analog Hack
    bit 10: 
          Parasite Eve/Vandal Hearts fix


00E 1-byte unsigned char: controller port 1 type
00F 1-byte unsigned char: controller port 2 type
010 4-byte unsigned long: number of frames
014 4-byte unsigned long: rerecord count
018 4-byte unsigned long: offset to the savestate inside file
01C 4-byte unsigned long: offset to the memory card 1 inside file
020 4-byte unsigned long: offset to the memory card 2 inside file
024 4-byte unsigned long: offset to the cheat list inside file
028 4-byte unsigned long: offset to the CD-ROM IDs inside file
02C 4-byte unsigned long: offset to the controller data inside file
030 4-byte unsigned long: string length of author name
034 string: name of the author
```

### Controller Types ###
```
01 Mouse
02 Negcon
03 Konami Gun
04 Standard
05 Analog Joystick
06 Namco Guncon
07 Analog Controller
```

## Savestate ##
After the header comes a compressed savestate. If the movie starts from power-on, the "savestate" is only 4 empty bytes.

## Memory Cards ##
After the savestate come two compressed memory cards. If the movie starts from a clean power-on, the "memory cards" are only 8 empty bytes (4 for each one).

## Cheat List ##
After the memory cards comes a compressed cheat list. If the movie doesn't use them, the "cheat list" is only 4 empty bytes.

## CD-ROM IDs ##
After the cheat list comes the CD-ROM ID chunk:
```
000 1-byte char: how many CDs does the movie use
001 9-byte string for each CD: CD-ROM ID of each CD used
```

## Controller Data ##
After the CD-ROM IDs, comes the controller data.

### Bytes per Frame ###
```
In Binary Format:
  04 Mouse
  02 Standard
  06 Analog Joystick/Controller
In Text Format:
  12 Mouse
  15 Standard
  33 Analog Joystick/Controller
```
<sup>(The other controller types are currently not supported by PCSX.)</sup>

There's also an extra byte for control functions like Reset, Open/Close CD case, etc.

So, if the movie uses 2 standard controllers, we know it will use 5 bytes per frame (2 bytes for player 1, 2 bytes for player 2 and 1 byte for control functions).

### Frame Data ###
The corresponding bytes indicate which buttons are pressed at each frame for each controller.

**Standard Controller**
```
000 2-byte unsigned short:
    01 00 Select
    02 00 unknown
    04 00 unknown
    08 00 Start
    10 00 Up
    20 00 Right
    40 00 Down
    80 00 Left
    00 01 L2
    00 02 R2
    00 04 L1
    00 08 R1
    00 10 Triangle
    00 20 Circle
    00 40 Cross
    00 80 Square
```

**Analog Joystick/Controller**
```
000 2-byte unsigned short: same as standard
002 1-byte unsigned char: left X
003 1-byte unsigned char: left Y
004 1-byte unsigned char: right X
005 1-byte unsigned char: right Y
```
<sup>(Values are in range 0-255, where 128 is center position.)</sup>

**Mouse**
```
000 2-byte unsigned short: same as standard
002 1-byte unsigned char: X movement
003 1-byte unsigned char: Y movement
Note: Values are in range 0-255, where 128 is center position
```
<sup>(Values are in range 0-255, where 128 is center position.)</sup>

**Control Byte**

The corresponding bits indicate which functions are used at each frame.
```
bit 1: Reset
bit 2: Open/Close CD case
bit 3: Enable/Disable "SIO IRQ Always Enabled" hack [4]
bit 4: Enable/Disable "SPU IRQ Always Enabled" hack [4]
bit 5: Enable/Disable cheats [5]
bit 6: Enable/Disable "Resident Evil 2/3 fix" hack [4]
bit 7: Enable/Disable "Parasite Eve 2 fix" hack[4]

[4]Note: These bits will be ignored at movie playback if the "hacks" header flag is not set.
[5]Note: These bits will be ignored at movie playback if the "cheats" header flag is not set.
```