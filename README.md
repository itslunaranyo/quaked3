# QuakeED 3.2
QuakeED 3.2 is a somewhat arbitrarily-numbered member of the family of Quake level editors that inherit from id software's original in-house tool. I took over a dead branch that I actually liked using despite its bugginess and missing functionality, and began fixing said things to have access to a customizable tool I could add new features to as soon as I needed them.

Currently, only Windows builds are available, and only Quake is supported.

# Future Plans
- Support for the Valve220 map format and non-world-axial projection
- Much needed migration away from Win32 (!) to something portable, 64-bit capable, multithread friendly, and which isn't GTK
- Implement an even vaguely modern renderer (currently still relying on fixed function OpenGL1 in immediate mode)
- Embedded Python 3 interface for scriptable access to QuakeED functionality, inspired by Maya/Blender
- Compile and run from within the editor
