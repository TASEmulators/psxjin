This directory holds a regression testing framework for PSXjin.

To run the tests:

Put a copy of Castlevania: Symphony of the Night in:
   ..\..\isos\csotn\Castlevania.bin

Put a copy of Final Fantasy 8 in:
   ..\..\isos\ff8\ff8_disk1.bin
   ..\..\isos\ff8\ff8_disk2.bin
   ..\..\isos\ff8\ff8_disk3.bin
   ..\..\isos\ff8\ff8_disk4.bin
   
Type "runtests"

This will run for a very long time; near the end, you'll have to
select ff8_disk2.bin from the file open dialog box.  After
you've selected the file, to continue the test, press Pause.

At the end, if all goes well, it should print 4 lines of text, saying that 
tests passed for castlevania.expected (and how long it took) and for
ff8.expected (and how long it took).

If you need to deliberately break backward-compatibility, you can run
"buildtests" to regenerate the new ".expected" files.  Again, you'll
need to select ff8_disk2.bin from the file open dialog box and
then press Pause to continue.