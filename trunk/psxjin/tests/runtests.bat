copy NUL results.txt

..\output\psxjin-release -lua runtest.lua -runcd ..\..\isos\csotn\Castlevania.bin -play Any%%-Replay-v2.pjm -luaargs castlevania.expected results.txt

..\output\psxjin-release -lua runtest.lua -runcd ..\..\isos\ff8\ff8_disk1.bin -play BombTest.pjm -luaargs ff8.expected results.txt

type results.txt