## QuakeED 3.2

### About
QuakeED 3.2 is a somewhat arbitrarily-numbered member of the family of Quake level editors that inherit from id software's original in-house tools. I took over a dead branch that I actually liked using despite its bugginess and missing functionality, and began fixing said things to have access to a customizable editor I could add new features to as soon as I needed them.

Currently, only Windows builds are available, and only Quake is supported.

### License & Authorship Notes
Some amount of work was done in ~2007 by Basil 'eerie' Nikityuk on [the source for QuakeEd 4](https://github.com/id-Software/Quake-2-Tools/tree/master/qe4) (id's Quake2 level editor that evolved from QE2 (their Quake level editor)) turning it into [QuakeEd 3.1](http://eerie.anarxi.st/qe3/build.2007.09.05/). I presume this is when the back-port from Quake2 support to Quake support took place. In 2008 a [func_msgboard poster named 'sikkpin'](https://www.celephais.net/board/view_thread.php?id=60225) took it up, folding in code borrowed from QERadiant and from M. 'Incubus' Milley's [QuakeEd 5](http://www.yossman.net/~mmilley/qe5/). I began rewriting it in C++ and modernizing it in 2018, tastefully incrementing the version number to QuakeEd 3.2.

The QE4 source was licensed under the GPL v2. SlabAllocator.h is borrowed from Trenchbroom, which is GPL3 and incompatible. This should not have been included for as long as it has, and will be corrected as soon as I write a suitable replacement allocator.

mru.cpp & mru.h have been in the codebase longer than I've had control of it, and their license is unknown. Being Windows-specific, they should disappear when the UI rewrite happens.

### Future Plans
- Support for the Valve220 map format and non-world-axial projection
- Much needed migration away from Win32 (!) to something portable, 64-bit capable, multithread friendly, and which isn't GTK - probably QT
- An even vaguely modern renderer (currently still relying on fixed function OpenGL1 in immediate mode)
- Embedded Python 3 interface for scriptable access to QuakeED functionality, inspired by Maya/Blender
- Compile and run from within the editor

