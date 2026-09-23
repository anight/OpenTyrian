/*
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "joystick.h"
#include "keyboard.h"
#include "network.h"
#include "opentyr.h"
#include "video.h"

#include "SDL.h"
#include <stdio.h>
#include <stdlib.h>


JE_boolean ESCPressed;

JE_boolean newkey, newmouse, keydown, mousedown;
SDL_Scancode lastkey_sym;
Uint16 lastkey_mod;
unsigned char lastkey_char;
Uint8 lastmouse_but;
Uint16 lastmouse_x, lastmouse_y;
JE_boolean mouse_pressed[3] = {false, false, false};
Uint16 mouse_x, mouse_y;

Uint8 keysactive[SDL_NUM_SCANCODES];

/* There is no mouse to grab; the game still asks. */
bool input_grab_enabled = false;


void flush_events_buffer( void )
{
	SDL_Event ev;

	while (SDL_PollEvent(&ev));
}

void wait_input( JE_boolean keyboard, JE_boolean mouse, JE_boolean joystick )
{
	service_SDL_events(false);
	while (!((keyboard && keydown) || (mouse && mousedown) || (joystick && joydown)))
	{
		SDL_Delay(SDL_POLL_INTERVAL);
		push_joysticks_as_keyboard();
		service_SDL_events(false);
	}
}

void wait_noinput( JE_boolean keyboard, JE_boolean mouse, JE_boolean joystick )
{
	service_SDL_events(false);
	while ((keyboard && keydown) || (mouse && mousedown) || (joystick && joydown))
	{
		SDL_Delay(SDL_POLL_INTERVAL);
		poll_joysticks();
		service_SDL_events(false);
	}
}

void init_keyboard( void )
{
	newkey = newmouse = false;
	keydown = mousedown = false;
}

void input_grab( bool enable )
{
	input_grab_enabled = enable;
}

JE_word JE_mousePosition( JE_word *mouseX, JE_word *mouseY )
{
	service_SDL_events(false);
	*mouseX = mouse_x;
	*mouseY = mouse_y;
	return mousedown ? lastmouse_but : 0;
}

void set_mouse_position( int x, int y )
{
	mouse_x = x;
	mouse_y = y;
}

/*
 * The character a key types, which the game wants for name entry and for the
 * codes typed at the title screen.  SDL's keycodes would carry it, but
 * picosdl's keysym.sym is only the scancode again - it has no keyboard layout
 * to consult - so this is a US layout, which is what the Bluetooth keyboard
 * reports its usages in anyway.
 */
static unsigned char scancode_char( SDL_Scancode sc, bool shift )
{
	static const char digits[]         = "1234567890";
	static const char digits_shifted[] = "!@#$%^&*()";

	if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
		return (shift ? 'A' : 'a') + (sc - SDL_SCANCODE_A);
	if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_0)
		return (shift ? digits_shifted : digits)[sc - SDL_SCANCODE_1];

	switch (sc)
	{
	case SDL_SCANCODE_RETURN:       return '\r';
	case SDL_SCANCODE_ESCAPE:       return 27;
	case SDL_SCANCODE_BACKSPACE:    return '\b';
	case SDL_SCANCODE_TAB:          return '\t';
	case SDL_SCANCODE_SPACE:        return ' ';
	case SDL_SCANCODE_MINUS:        return shift ? '_' : '-';
	case SDL_SCANCODE_EQUALS:       return shift ? '+' : '=';
	case SDL_SCANCODE_LEFTBRACKET:  return shift ? '{' : '[';
	case SDL_SCANCODE_RIGHTBRACKET: return shift ? '}' : ']';
	case SDL_SCANCODE_BACKSLASH:    return shift ? '|' : '\\';
	case SDL_SCANCODE_SEMICOLON:    return shift ? ':' : ';';
	case SDL_SCANCODE_APOSTROPHE:   return shift ? '"' : '\'';
	case SDL_SCANCODE_GRAVE:        return shift ? '~' : '`';
	case SDL_SCANCODE_COMMA:        return shift ? '<' : ',';
	case SDL_SCANCODE_PERIOD:       return shift ? '>' : '.';
	case SDL_SCANCODE_SLASH:        return shift ? '?' : '/';
	default:                        return 0;
	}
}

void service_SDL_events( JE_boolean clear_new )
{
	SDL_Event ev;

	if (clear_new)
		newkey = newmouse = false;

	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
			case SDL_KEYDOWN:
				keysactive[ev.key.keysym.scancode] = 1;

				newkey = true;
				lastkey_sym = ev.key.keysym.scancode;
				lastkey_mod = ev.key.keysym.mod;
				lastkey_char = scancode_char(ev.key.keysym.scancode, (ev.key.keysym.mod & KMOD_SHIFT) != 0);
				keydown = true;
				return;
			case SDL_KEYUP:
				keysactive[ev.key.keysym.scancode] = 0;
				keydown = false;
				return;
			case SDL_QUIT:
				/* TODO: Call the cleanup code here. */
				exit(0);
				break;
		}
	}
}

void JE_clearKeyboard( void )
{
	// /!\ Doesn't seems important. I think. D:
}
