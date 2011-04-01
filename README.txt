This is Hasenpfeffer OS, a toy/educational OS built
as a set of components on top of L4 microkernel.

The system was built as a practical part of Senko's diploma
thesis at the Faculty of Electrical Engineering and
Computing in Zagreb, Croatia.

As the original documentation is in Croatian, the most important
(for following and playing with the code) parts have been
translated using Google Translate, until some poor human goes
through that manually. Sorry about that.

Trying it out
-------------

You don't have to compile anything to see how this looks like.
You can download the boot floppy image, as well as a small
IDE disk image from:
  http://vault.senko.net/hasenpfeffer/bootfloppy.img (1.4MB)
  http://vault.senko.net/hasenpfeffer/idedisk.img (64MB)

(note that the IDE disk image is a raw image file, and that it
doesn't contain any partitions - just the ext2 filesystem; mount
it using loop device to see the internals)

Boot from the boot floppy and see how it looks. The boot floppy
is enough for Hello World program, but Tiny Basic and Tiny Scheme
require the disk as they attempt to read files from the fs.

Building it
-----------

You'll need a L4Ka::Pistachio ia32 (x86) SDK . The doc/overview.txt
file has requirements specified in more detail.

Updating, modifying, improving and reusing it
---------------------------------------------

Please do! Hasenpfeffer is released in the hope that it will
be useful, instructive or just plain fun to people. If it
succeeds in that task, the author is happy.

While you don't need to submit your changes or improvements
back upstream (depending on the exact license of a particular
component, see Licensing), they are very welcome if you
choose to do so.

Licences
--------

All of the new code written for Hasenpfeffer is under MIT license.
The 3rd party components used in Hasenpfeffer are under their
own open source licenses, listed on each file.

Author
------

The original author of the Hasenpfeffer operating system is
Senko Rasic <senko@senko.net>.

Hasenpfeffer uses numerous code written by a huge number of
other people, and couldn't happen without them - so thanks to
all of them!

