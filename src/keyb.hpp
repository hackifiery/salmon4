#warning not implemented yet...
#pragma once
#ifdef __unix__
#include <ncurses.h>
#elif defined(_WIN32) || defined(_WIN64)
// pd-curses
#include <curses.h>
#endif
