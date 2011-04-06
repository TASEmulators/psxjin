-- A library for regression-testing psxjin
-- (probably portable to other emulators, after implementing "test.checksum")

dofile("testlib.lua")

filename = arg[1]
out = io.open(filename, "w")

start = os.time()

-- We play through 10 seconds past the end of the movie (to check for strange
-- things happening at movie end)
while movie.framecount() < (0 + (arg[2] or (movie.length() + 600))) do
   -- In a simple test, it seems that checksumming the CPU has
   -- negligible cost, checksumming video memory every frame has about
   -- a 25% cost, checksumming main memory every frame has about a 50%
   -- cost, and checksumming the savestate every frame has about a
   -- 525% cost (!).  We keep the total checksumming cost down to
   -- about 25% by checking video memory every 5 frames (5%), checking
   -- main memory every 7 frames (7%), and checking the savestate
   -- every 41 frames (13%).  (All of these are odd primes, which
   -- means that we will check both even and odd frames, both frames
   -- which are and are not multiples of 3, etc.)  Since checksumming
   -- the CPU is cheap, we do that on every frame.

   fc = movie.framecount()
   results = {frame=fc}
   if fc%1 == 0 then results["cpu"] = test.checksum("cpu") end
   if fc%5 == 0 then results["videomem"] = test.checksum("videomem") end
   if fc%7 == 0 then results["mainmem"] = test.checksum("mainmem") end
   if fc%41 == 0 then results["savestate"] = test.checksum("savestate") end
   
   out:write(table.save(results), "\n")
   emu.frameadvance()
end

stop = os.time()
out:write(table.save({frame=movie.framecount(), quit=true, time=(stop-start)}), "\n")
out:close()

emu.exitemulator()
