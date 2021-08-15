#ifndef SUUGU_KEYBINDS_H
#define SUUGU_KEYBINDS_H

#include "core/input.h"

struct Keybinds {
	Key::Key selectElement          = Key::MBLEFT;
	Key::Key selectMultipleElements = Key::MBLEFT | InputMod_Lshift | InputMod_Rshift;
	Key::Key deselectElement        = Key::MBRIGHT | InputMod_Lshift | InputMod_Rshift;

	Key::Key editElement = Key::MBLEFT; //double click
};



#endif