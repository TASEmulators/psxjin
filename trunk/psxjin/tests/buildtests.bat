REM With the current PSXjin, the run desyncs before frame 11000.  We'll limit
REM our regression testing to the first 12000 frames, since the desynced
REM remainder of the run is not very interesting.
..\output\psxjin-release -lua buildtest.lua -runcd ..\..\isos\csotn\Castlevania.bin -play Any%%-Replay-v2.pjm -luaargs castlevania.expected 12000

..\output\psxjin-release -lua buildtest.lua -runcd ..\..\isos\ff8\ff8_disk1.bin -play BombTest.pjm -luaargs ff8.expected

