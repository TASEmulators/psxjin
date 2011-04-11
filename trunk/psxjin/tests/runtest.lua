dofile("testlib.lua")

exp_filename = arg[1]
expected = io.open(exp_filename, "r")

out_filename = arg[2]
out = io.open(out_filename, "a+")

start = os.time()

pass = true

function check(results, key)
   if key == "frame" then
      result = results.frame
      actual = movie.framecount()
   else
      result = results[key]
      if not result then return end
      actual = test.checksum(key)
   end

   if result ~= actual then
      return string.format("ERROR!  On frame %d, expected %s but got %s for '%s'\n",
			   movie.framecount(), result, actual, key)
   end
end

for results_str in expected:lines() do
   results, err = table.load(results_str)
   if err then
      out:write("table.load error: ", err)
      pass = false
      break
   end
   if results.quit then break end
   for _, key in ipairs({ "frame", "cpu", "mainmen", "videomem", "savestate" }) do
      fail = check(results, key)
      if fail then 
	 out:write(fail)
	 pass = false
	 break
      end
   end
   if not pass then break end
   emu.frameadvance()
end

if pass then
   out:write("tests passed for ", exp_filename, "\n")
end

stop = os.time()

out:write("test run for ", exp_filename, " took ", (stop - start), " seconds\n")

out:close()

emu.exitemulator()
