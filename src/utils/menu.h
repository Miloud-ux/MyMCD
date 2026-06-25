// == menu.h ==
//
// PURPOSE:
//   Declares the startup splash / menu shown once before the main editing
//   loop begins.  The menu is a centred, box-drawn ncurses window with three
//   options:
//
//     [1] New Diagram
//     [2] Load Diagram
//     [3] Exit
//
//   Navigation:
//     • Arrow keys UP / DOWN   – move the highlighted (blinking) cursor
//     • Number keys 1 / 2 / 3  – jump directly to that option
//     • ENTER                  – confirm the currently highlighted option
//
//   Visual style:
//     • The window is drawn with box() so it has a full border.
//     • A title bar "[ MCD Tool ]" is centred on the top edge.
//     • The currently selected option is rendered with A_BLINK | A_REVERSE
//       so it pulses on supported terminals (most modern terminal emulators
//       respect A_BLINK; on those that do not the reverse-video highlight is
//       still clearly visible).
//     • The other two options are rendered in plain text.
//
// RETURN VALUE:
//   show_startup_menu() returns a MenuChoice enum value so main() can branch
//   without checking magic integers.
//
// INTERACTION WITH LOAD:
//   When the user selects MENU_LOAD the caller (main.c) is responsible for
//   prompting for the filename and calling load_diagram().  The menu itself
//   only returns MENU_LOAD; it does NOT open files.  This keeps the menu
//   code free of Arena / AST dependencies.

#pragma once

#include <ncurses.h>
#include <string.h>

typedef enum { MENU_NEW = 0, MENU_LOAD = 1, MENU_EXIT = 2 } MenuChoice;

// Draw the startup menu, block until the user makes a choice, return it.
// Must be called after initscr() and initColors().
MenuChoice show_startup_menu(void);
