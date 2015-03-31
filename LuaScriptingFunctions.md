See also [Lua manual](http://www.lua.org/manual/5.1/).

**Important: nil is not 0. nil and 0 are two different entities.**

# Emu #

(Note: All these functions can be called through psxjin. or emu.)

### emu.speedmode(string mode) ###

Changes the speed of emulation depending on mode. If "normal", emulator runs at normal speed. If "nothrottle", emulator runs at max speed without frameskip. If "turbo", emulator drops some frames. If "maximum", screen rendering is disabled.

### emu.frameadvance() ###

Pauses script until a frame is emulated. Cannot be called by a coroutine or registered function.

### emu.pause() ###

Pauses emulator when the current frame has finished emulating.

### emu.unpause() ###

Unpauses emulator.

### int emu.framecount() ###

Returns the frame count for the movie, or the number of frames from last reset otherwise.

### int emu.lagcount() ###

Returns the lag count.

### boolean emu.lagged() ###

Returns true if the current frame is a lag frame, false otherwise.

### emu.registerbefore(function func) ###
### emu.registerafter(function func) ###
### emu.registerexit(function func) ###

(?)

### emu.message(string msg) ###

Displays the message on the screen.

# Memory #

### int memory.readbyte(int addr) ###
### int memory.readbytesigned(int addr) ###
### int memory.readword(int addr) ###
### int memory.readwordsigned(int addr) ###
### int memory.readdword(int addr) ###
### int memory.readdwordsigned(int addr) ###

Reads value from memory address. Word=2 bytes, Dword=4 bytes.

### string memory.readbyterange(int startaddr, int length) ###

Returns a chunk of memory from the given address with the given length as a string. To access, use _string.byte(str,offset)_.

### memory.writebyte(int addr, int value) ###
### memory.writeword(int addr, int value) ###
### memory.writedword(int addr, int vlaue) ###

Writes value to memory address.

### memory.register(int addr, function func) ###

Calls the function whenever the given address is written to. Function can be nil.

# joypad #

Before the next frame is emulated, one may set keys to be pressed. The buffer is cleared each frame.

### table joypad.get(int port) ###

Returns a table of buttons which have been pressed. Does not read movie input. Key values are 1 for pressed, nil for not pressed. Keys for joypad table: (select, l3, [r3](https://code.google.com/p/psxjin/source/detail?r=3), start, up, right, down, left, l2, [r2](https://code.google.com/p/psxjin/source/detail?r=2), l1, [r1](https://code.google.com/p/psxjin/source/detail?r=1), triangle, circle, x, square).

### joypad.set(int port, table buttons) ###

Sets the buttons to be pressed next frame. 1 for pressed, nil for not pressed.

# savestate #

### object savestate.create(int slot=nil) ###

Creates a savestate object. If any argument is given, it must be from 1 to 12, and it corresponds with the numbered savestate slots. If no argument is given, the savestate can only be accessed by Lua.

### savestate.save(object savestate) ###

Saves the current state to the savestate object.

### savestate.load(object savestate) ###

Loads the state of the savestate object as the current state.

# movie #

### string movie.mode() ###

Returns "record" if movie is recording, "playback" if movie is replaying input, or nil if there is no movie.

### movie.rerecordcounting(boolean skipcounting) ###

If set to true, no rerecords done by Lua are counted in the rerecord total. If set to false, rerecords done by Lua count. By default, rerecords count.

### movie.stop() ###

Stops the movie. Cannot be used if there is no movie.

# gui #

Before the next frame is drawn, one may draw to the buffer. The buffer is cleared each frame.

All functions assume that the height of the image is 256 and the width is 239.

Color can be given as "0xrrggbbaa" or as a name (e.g. "red").

### gui.pixel(int x, int y, type color) ###

Draws a pixel at (x,y) with the given color.

### gui.line(int x1, int y1, int x2, int y2, type color) ###

Draws a line from (x1,y1) to (x2,y2) with the given color.

### gui.box(int x1, int y1, int x2, int y2, type color) ###
### gui.fillbox(int x1, int y1, int x2, int y2, type color) ###

Draws a box with (x1,y1) and (x2,y2) as opposite corners with the given color. "box" is hollow, while "fillbox" is filled.

### gui.circle(int x, int y, int radius, type color) ###
### gui.fillcircle(int x, int y, int radius, type color) ###

Draws a circle centered at (x,y) with given radius and color. "circle" is hollow, while "fillcircle" is filled.

### string gui.gdscreenshot() ###

Takes a screenshot and returns it as a string that can be used by the [gd library](http://lua-gd.luaforge.net/).

For direct access, use _string.byte(str,offset)_. The gd image consists of a 11-byte header and each pixel is alpha,red,green,blue (1 byte each, alpha is 0 in this case) left to right then top to bottom.

### gui.opacity(float alpha) ###

Sets the opacity of drawings depending on alpha. 0.0 is invisible, 1.0 is drawn over. Values less than 0.0 or greater than 1.0 work by extrapolation.

### gui.transparency(float strength) ###

4.0 is invisible, 0.0 is drawn over. Values less than 0.0 or greater than 4.0 work by extrapolation.

### gui.text(int x, int y, string msg) ###

Draws the given text at (x,y). Not to be confused with _emu.message(string msg)_.

### gui.gdoverlay(int x=0, int y=0, string gdimage, float alpha=1.0) ###

Overlays the given gd image with top-left corner at (x,y) and given opacity.

### function gui.register(function func) ###

Registers a function to be called when the screen is updated. Function can be nil. The previous function is returned, possibly nil.

### string gui.popup(string msg, string type = "ok") ###

Creates a pop-up dialog box with the given text and some dialog buttons. There are three types: "ok", "yesno", and "yesnocancel". If "yesno" or "yesnocancel", it returns either "yes", "no", or "cancel", depending on the button pressed. If "ok", it returns nil.

### gui.clearuncommitted() ###

Clears drawing buffer. Only works if it hasn't been drawn on screen yet.

# input #

### table input.get() ###

Returns a table of which keyboard buttons are pressed as well as mouse status. Key values for keyboard buttons and mouse clicks are true for pressed, nil for not pressed. Mouse position is returned in terms of game screen pixel coordinates. Coordinates assume that the game screen is 256 by 224. Keys for mouse are (xmouse, ymouse, leftclick, rightclick, middleclick). Keys for keyboard buttons:
(backspace, tab, enter, shift, control, alt, pause, capslock, escape, space, pageup, pagedown, end, home, left, up, right, down, insert, delete,
0, 1, ..., 9, A, B, ..., Z, numpad0, numpad1, ..., numpad9, numpad`*`, numpad+, numpad-, numpad., numpad/, F1, F2, ..., F24,
numlock, scrolllock, semicolon, plus, comma, minus, period, slash, tilde, leftbracket, backslash, rightbracket, quote)

Keys are case-sensitive. Keys for keyboard buttons are for buttons, **not** ASCII characters, so there is no need to hold down shift. Key names may differ depending on keyboard layout. On US keyboard layouts, "slash" is /?, "tilde" is `````~, "leftbracket" is `[``{`, "backslash" is \|, "rightbracket" is `]``}`, "quote" is '".