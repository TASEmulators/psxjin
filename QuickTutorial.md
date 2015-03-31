# PSXjin Tutorial #

### BIOS ###

PSXjin needs an external BIOS>  By default it will look for SCPH1001.BIN in the .\bios folder.  The file & location can be modified in Configuration -> Plugins & Bios

These BIOS files should be used:

  * SCPHxx01.BIN for NTSC-U games,
  * SCPHxx00.BIN or SCPHxx03.BIN for NTSC-J games, or
  * SCPHxx02.BIN for PAL games.

where the xx can be any number. SCPH1001.BIN is recommended for NTSC-U games on [TASVideos](http://tasvideos.org). See [Wikipedia](http://en.wikipedia.org/wiki/PlayStation_%28console%29) for more information.

You must find the BIOS files yourself.  Please don't ask where you can find them.

### Graphics ###

PSXjin comes with an integrated GPU.  Default settings should be adequate for getting started.  Settings can be modified in Configuration -> Graphics

### Sound ###

PSXjin comes with its own integrated SPU.  Default settings should be adequate for getting started.  Settings may need to be modified for AVI capture.  Settings can be modified in Configuration -> Sound

### Input plugins ###

The Segu plugin is loaded by default.

PSXjin also comes with  the [NRage plugin](http://www.emulator-zone.com/doc.php/psx/psxplugins-tools.html), which has the advantage of saving controller profiles, although there is a config interface bug ("L2" is L1, "[R2](https://code.google.com/p/psxjin/source/detail?r=2)" is L2, "L1" is [R2](https://code.google.com/p/psxjin/source/detail?r=2)).