 PSXjin - Pc Psx Emulator
 ----------------------

Contents
--------

1) General
2) How it works
3) Supported games
4) TroubleShoot
5) Credits

--------------------------------------------------------------------------------

1) General
   -------

PSXjin is a PSX emulator. What does that really mean? It means that it emulates the
way that a PSX works and tries to translate PSX machine language to PC language.
That is very hard to do and we can't speak for 100% success in any time.

PSXjin is an overhaul of the PCSX-rr fork of the PCSX emulator (confused yet?)  Compared to previous forks, PSXjin uses mimial plugins (only an input plugin is needed), a simple to use GUI, and a new SPU core.


--------------------------------------------------------------------------------

2) How it works
   ------------

Steps:
------
 1) Put a bios on bios directory (recommended: scph1001.bin) (optional)
 2) Neccessary Plugins should already be in the plugins directory
 3) Open the emulator 
 4) Select Open CD and pick a .iso (or orther valid file type)


Cpu Options:
-----------
 * Disable Xa Decoding:
    Disables xa sound, if you don't want sound this might
    speed up a little movies (mostly) and the game itself.

 * Sio Irq Always Enabled:
    Enable this only when pads or memcards doesn't works,
    but usually brokes compatibility with them.

 * Spu Irq Always Enabled:
 	May help your game to work, but usually doesn't helps.
 
 * Black & White Movies:
    Speed up for slow machines on movies.

 * Disable CD-DA:
    Will disable CD audio.

 * Enable Console Output:
    Displays the psx text output.

 * Enable Interpreter Cpu:
    Enables interpretive emulation (recomiler by default),
	it may be more compatible, but it's slower.

 * Psx System Type:
    Autodetect: Try to autodetect the system.
    NSTC/PAL: Specify it for yourself.

--------------------------------------------------------------------------------

3) Supported Games
   ---------------

Here is a small list with games that pcsx support. Notice that it might have
some glitches on sound or gfx but it is considered playable. :)

 Crash Bandicoot 1
 Time crisis
 Mickey Wild adventure
 Coolboarders 3
 Street fighter EX+a
 Street fighter EX2 plus
 Breath of fire 3
 Breath of fire 4
 Quake II
 Alone in the Dark 4
 Tekken 3

and probably lots more.
Check www.emufanatics.com or www.ngemu.com for compatibility lists.

------------------------------------------------------------------------------------------------

4) Troubleshoot
   ------------

 -QUE: My favourite game doesn't work
 -AN:  Wait for the next release, or get another emu ;)
 -QUE: Can I have a bios image?
 -AN : No
 -QUE: CD audio is crappy...
 -AN : If it doesn't work then disable it :)

------------------------------------------------------------------------------------------------

5) Credits
   -------

The PSXjin Team:
zeromus
adelikat
Darkkobold

PCSX-rr:
mz

the original PCSX is the work of the following people:

main coder:
 Linuzappz   , e-mail: linuzappz@pcsx.net
co-coders:
 Shadow      , e-mail: shadow@pcsx.net
ex-coders:
 Pete Bernett, e-mail: psswitch@online.de
 NoComp      , e-mail: NoComp@mailcity.com
 Nik3d       , e-mail:
webmaster:
 Akumax      , e-mail: akumax@pcsx.net

Team would like to thanks:
--------------------------
Ancient   :Shadow's small bother for beta testing pcsx and bothering me to correct it :P
Roor      :for his help on cd-rom decoding routine :)
Duddie,
Tratax,
Kazzuya   :for the great work on psemu,and for the XA decoder :)
Calb      :for nice chats and coding hits ;)  We love you calb ;)
Twin      :for the bot to #pcsx on efnet, for coding hints and and :)
Lewpy     :Man this plugin is quite fast! 
Psychojak :For beta testing PSXjin and donating a great amount of games to Shadow ;)
JNS       :For adding PEC support to PSXjin
Antiloop  :Great coder :)
Segu      :He knows what for :)
Null      :He also knows :)
Bobbi
cdburnout :For putting PSXjin news to their site and for hosting..
D[j]      :For hosting us ;)
Now3d     :For nice chats,info and blabla :)
Ricardo Diaz:Linux version may never have been released without his help
Nuno felicio:Beta testing, suggestions and a portugues readme
Shunt     :Coding hints and nice chats :)
Taka      :Many fixes to PSXjin
jang2k    :Lots of fixes to PSXjin :)

and last but not least:
-----------------------
James   : for all the friendly support
Dimitris: for being good friend to me..
Joan: The woman that make me nuts the last couple months...

------------------------------------------------------------------------------------------------

PSXjin: http://code.google.com/p/psxjin/