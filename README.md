# Kickshaw - A Menu Editor for Openbox

This is my git mirror of [Kickshaw](https://savannah.nongnu.org/projects/obladi) used for primarily for Arch packaging

![alt text](https://cdn.scrot.moe/images/2017/08/04/kickshaw.png)

## INSTALLATION

If on Arch you can get the package in the AUR with

    yaourt -S kickshaw


A simple script is provided to build and install from source if you prefer

    git clone https://github.com/natemaia/kickshaw

    cd kickshaw/

    ./install && exit


Running `make` then `sudo make install`

inside `../kickshaw/source/` is all that's required for installation

---

## REQUIREMENTS

Dependencies: GTK version 3; the code makes use of GNU extensions.
Kickshaw is not dependent on Openbox; it can be used inside all window
managers/desktop environments that support GTK applications to create and
edit menu files. A makefile is included in the source directory. "make"
and "make install" are sufficient to compile and install this program;
there is no configure script file which has to be started first.

## IMPORTANT NOTES

From version 0.5 on this program has all essential plus some additional
features. This is the first release considered by the author to be stable
and bug-free enough, as to his knowledge achieved by own reviewing and
testing.
Additional input to enhance the program further is of course
appreciated, since one author alone has always his own "limited view".
There are some additional features planned for the future (see the TODO
file for these), but they rather fall into the "luxury" category;
priority from now on is a stable and as bug-free as possible program.

Even though the program can cope with whitespace and several tags that
are nested into each other, the menu xml file has to be syntactically
correct, or it won't be loaded properly or at all. In this case the
program gives a detailed error message, if possible.

## FEATURES

- Dynamic, context sensitive interface
- The tree view can be customised (show/hide columns, grid etc.)
- Context menus
- Editable cells (dependent on the type of the cell)
- Support for icons. Changes done outside the program to the image files,
  for example a replacement of the image file with a new one under the
  same name, are taken over within a second, and the program will show the
  new image.
- (Multirow) Drag and drop
- The options of an "Execute" action and "startupnotify" option block can
  be auto-sorted to obtain a constant menu structure
- Search functionality (incl. processing of regular expressions)
- Multiple selections (hold ctrl/shift key) can be used for mass deletions
  or multirow drag and drop
- Entering an existing Menu ID is prevented
- The program informs the user if there are missing menu/item labels,
  invalid icon paths and menus defined outside the root menu that are not
  included inside the latter. By request the program creates labels for
  these invisible menus/items, integrates the orphaned menus into the root
  menu and opens a file chooser dialog so invalid icon paths can be
  corrected.
- Deprecated "execute" options are converted to "command" options.
- Fast and compact, avoidance of overhead (programmed natively in C,
  only one additional settings file created by the program itself,
  no use of Glade)

## SPECIAL NOTE ABOUT DRAG AND DROP

GTK does not support multirow drag and drop, that's why only one row is
shown as dragged if mulitple rows have been selected for drag and drop.
There is a workaround for this, but as good as it works for list views
it doesn't for tree views, so it wasn't implemented here.

## GTK 2 > 3 > 4

The program was ported relatively early to GTK 3. I am aware of complaints
about GTK 3 regarding RAM usage, but in some respects working with GTK 2
makes things quite awkward for a developer, and GTK 3 eased things a bit.
Besides that, at the time of the release of 0.5.0, the first mature subversion
release based on GTK 3, GTK 2 had been already abandoned for six years.
Obviously I won't do a parallel version for GTK 2, since the effort would be
too much.
I won't do a GTK 4 version until GTK 4 is at a mature stage, since one can
expect quite a lot of changes coming up during the updates for GTK 4. As long
there aren't improvements that make it worth porting Kickshaw to GTK 4, it's
simply not worth the effort as long as GTK 3 is still in use.

## License

Copyright (c) 2010-2017        Marcus Schaetzle

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with Kickshaw. If not, see http://www.gnu.org/licenses/.
