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
#include "config.h"
#include "file.h"
#include "joystick.h"
#include "keyboard.h"
#include "nortsong.h"
#include "opentyr.h"
#include "params.h"
#include "varz.h"
#include "video.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/*
 * picosdl has at most one input device worth the name, and it reaches the game
 * through two APIs at once.
 *
 * The board's analog stick and the I2C pad both feed one SDL_GameController,
 * which is the one to read when there is one: it has named buttons, a D-pad and
 * the merged stick.  With no pad attached there is no controller, and the stick
 * is a bare two-axis joystick.  The stick's click is joystick button 0 in either
 * case and never a controller button, so it is read through the joystick API
 * alongside the controller rather than instead of it.  picowolf found three
 * bugs in exactly that arrangement; this is the shape that came out of them.
 *
 * So an assignment's AXIS and BUTTON name controller axes and buttons when
 * there is a controller and joystick ones when there is not, and the stick
 * click is a button of its own, STICK_CLICK, past the controller's range.
 */
#define STICK_CLICK SDL_CONTROLLER_BUTTON_MAX

int joystick_axis_threshold( int j, int value );
int check_assigned( int j, const Joystick_assignment assignment[2] );
const char *assignment_to_code( const Joystick_assignment *assignment );

int joystick_repeat_delay = 300; // milliseconds, repeat delay for buttons
bool joydown = false;            // any joystick buttons down, updated by poll_joysticks()

bool ignore_joystick = false;
int joysticks = 0;

static Joystick the_joystick;
Joystick *joystick = &the_joystick;

static SDL_GameController *controller;

static const int joystick_analog_max = 32767;

// eliminates axis movement below the threshold
int joystick_axis_threshold( int j, int value )
{
	assert(j < joysticks);

	bool negative = value < 0;
	if (negative)
		value = -value;

	if (value <= joystick[j].threshold * 1000)
		return 0;

	value -= joystick[j].threshold * 1000;

	return negative ? -value : value;
}

// converts joystick axis to sane Tyrian-usable value (based on sensitivity)
int joystick_axis_reduce( int j, int value )
{
	assert(j < joysticks);

	value = joystick_axis_threshold(j, value);

	if (value == 0)
		return 0;

	return value / (3000 - 200 * joystick[j].sensitivity);
}

// converts analog joystick axes to an angle
// returns false if axes are centered (there is no angle)
bool joystick_analog_angle( int j, float *angle )
{
	assert(j < joysticks);

	float x = joystick_axis_threshold(j, joystick[j].x), y = joystick_axis_threshold(j, joystick[j].y);

	if (x != 0)
	{
		*angle += atanf(-y / x);
		*angle += (x < 0) ? -M_PI_2 : M_PI_2;
		return true;
	}
	else if (y != 0)
	{
		*angle += y < 0 ? M_PI : 0;
		return true;
	}

	return false;
}

static int axis_count( void )
{
	return controller != NULL ? SDL_CONTROLLER_AXIS_MAX : 2;
}

static int button_count( void )
{
	return controller != NULL ? SDL_CONTROLLER_BUTTON_MAX + 1 : 1;
}

static int read_axis( int num )
{
	return controller != NULL ? SDL_GameControllerGetAxis(controller, num)
	                          : SDL_JoystickGetAxis(joystick[0].handle, num);
}

static bool read_button( int num )
{
	if (controller == NULL || num == STICK_CLICK)
		return SDL_JoystickGetButton(joystick[0].handle, controller != NULL ? 0 : num) != 0;
	return SDL_GameControllerGetButton(controller, num) != 0;
}

/* gives back value 0..joystick_analog_max indicating that one of the assigned
 * buttons has been pressed or that one of the assigned axes has been moved
 * in the assigned direction
 */
int check_assigned( int j, const Joystick_assignment assignment[2] )
{
	(void)j;

	int result = 0;

	for (int i = 0; i < 2; i++)
	{
		int temp = 0;

		switch (assignment[i].type)
		{
		case NONE:
			continue;

		case AXIS:
			temp = read_axis(assignment[i].num);

			if (assignment[i].negative_axis)
				temp = -temp;
			break;

		case BUTTON:
			temp = read_button(assignment[i].num) ? joystick_analog_max : 0;
			break;

		case HAT:
			break;  // there are none
		}

		if (temp > result)
			result = temp;
	}

	return result;
}

// updates joystick state
void poll_joystick( int j )
{
	assert(j < joysticks);

	if (joystick[j].handle == NULL)
		return;

	SDL_PumpEvents();

	// indicates that a direction/action was pressed since last poll
	joystick[j].input_pressed = false;

	// indicates that an direction/action has been held long enough to fake a repeat press
	bool repeat = joystick[j].joystick_delay < SDL_GetTicks();

	// update direction state
	for (uint d = 0; d < COUNTOF(joystick[j].direction); d++)
	{
		bool old = joystick[j].direction[d];

		joystick[j].analog_direction[d] = check_assigned(j, joystick[j].assignment[d]);
		joystick[j].direction[d] = joystick[j].analog_direction[d] > (joystick_analog_max / 2);
		joydown |= joystick[j].direction[d];

		joystick[j].direction_pressed[d] = joystick[j].direction[d] && (!old || repeat);
		joystick[j].input_pressed |= joystick[j].direction_pressed[d];
	}

	joystick[j].x = -joystick[j].analog_direction[3] + joystick[j].analog_direction[1];
	joystick[j].y = -joystick[j].analog_direction[0] + joystick[j].analog_direction[2];

	// update action state
	for (uint d = 0; d < COUNTOF(joystick[j].action); d++)
	{
		bool old = joystick[j].action[d];

		joystick[j].action[d] = check_assigned(j, joystick[j].assignment[d + COUNTOF(joystick[j].direction)]);
		joydown |= joystick[j].action[d];

		/* Once per press, where directions repeat.  Held across a screen
		 * change - whose fade polls nothing for longer than the repeat delay -
		 * a button counted as held long enough to repeat, and its first
		 * poll on the new screen confirmed that one too: a single click
		 * went two or three screens deep.  Firing is read from action[],
		 * the held state, so play is unaffected. */
		joystick[j].action_pressed[d] = joystick[j].action[d] && !old;
		joystick[j].input_pressed |= joystick[j].action_pressed[d];
	}

	joystick[j].confirm = joystick[j].action[0] || joystick[j].action[4];
	joystick[j].cancel = joystick[j].action[1] || joystick[j].action[5];

	// if new input, reset press-repeat delay
	if (joystick[j].input_pressed)
		joystick[j].joystick_delay = SDL_GetTicks() + joystick_repeat_delay;
}

// updates all joystick states
void poll_joysticks( void )
{
	joydown = false;

	for (int j = 0; j < joysticks; j++)
		poll_joystick(j);
}

// sends SDL KEYDOWN and KEYUP events for a key
void push_key( SDL_Scancode key )
{
	SDL_Event e;

	memset(&e, 0, sizeof(e));

	e.key.keysym.scancode = key;
	e.key.keysym.sym = key;
	e.key.state = SDL_RELEASED;

	e.type = SDL_KEYDOWN;
	SDL_PushEvent(&e);

	e.type = SDL_KEYUP;
	SDL_PushEvent(&e);
}

// helps us be lazy by pretending joysticks are a keyboard (useful for menus)
void push_joysticks_as_keyboard( void )
{
	const SDL_Scancode confirm = SDL_SCANCODE_RETURN, cancel = SDL_SCANCODE_ESCAPE;
	const SDL_Scancode direction[4] = { SDL_SCANCODE_UP, SDL_SCANCODE_RIGHT, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT };

	poll_joysticks();

	for (int j = 0; j < joysticks; j++)
	{
		if (!joystick[j].input_pressed)
			continue;

		/* On the button's own press, not on whatever else was pressed while
		 * it happened to be held - a repeating direction would repeat it. */
		if (joystick[j].action_pressed[0] || joystick[j].action_pressed[4])
			push_key(confirm);
		if (joystick[j].action_pressed[1] || joystick[j].action_pressed[5])
			push_key(cancel);

		for (uint d = 0; d < COUNTOF(joystick[j].direction_pressed); d++)
		{
			if (joystick[j].direction_pressed[d])
				push_key(direction[d]);
		}
	}
}

// opens the one input device picosdl can have, as a controller if it is one
void init_joysticks( void )
{
	if (ignore_joystick)
		return;

	if (SDL_NumJoysticks() < 1)
	{
		printf("no joysticks detected\n");
		return;
	}

	memset(&joystick[0], 0, sizeof(joystick[0]));

	joystick[0].handle = SDL_JoystickOpen(0);
	if (joystick[0].handle == NULL)
		return;

	if (SDL_IsGameController(0))
		controller = SDL_GameControllerOpen(0);

	joysticks = 1;

	printf("joystick detected: %s\n", controller != NULL ? "game controller" : "analog stick");

	reset_joystick_assignments(0);
}

void deinit_joysticks( void )
{
	if (joysticks == 0)
		return;

	if (controller != NULL)
		SDL_GameControllerClose(controller);
	controller = NULL;

	SDL_JoystickClose(joystick[0].handle);
	joystick[0].handle = NULL;
	joysticks = 0;
}

static void assign( Joystick_assignment *a, Joystick_assignment_types type, int num, bool negative_axis )
{
	a->type = type;
	a->num = num;
	a->x_axis = false;
	a->negative_axis = negative_axis;
}

void reset_joystick_assignments( int j )
{
	assert(j < joysticks);

	for (uint a = 0; a < COUNTOF(joystick[j].assignment); a++)
		for (uint i = 0; i < COUNTOF(joystick[j].assignment[a]); i++)
			joystick[j].assignment[a][i].type = NONE;

	Joystick_assignment (*as)[2] = joystick[j].assignment;

	// up, right, down, left: the stick, and the D-pad if there is one
	assign(&as[0][0], AXIS, 1, true);
	assign(&as[1][0], AXIS, 0, false);
	assign(&as[2][0], AXIS, 1, false);
	assign(&as[3][0], AXIS, 0, true);

	if (controller != NULL)
	{
		assign(&as[0][1], BUTTON, SDL_CONTROLLER_BUTTON_DPAD_UP, false);
		assign(&as[1][1], BUTTON, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, false);
		assign(&as[2][1], BUTTON, SDL_CONTROLLER_BUTTON_DPAD_DOWN, false);
		assign(&as[3][1], BUTTON, SDL_CONTROLLER_BUTTON_DPAD_LEFT, false);

		// fire (also confirm), change fire (also cancel), left and right
		// sidekick, menu, pause - and the stick click fires too
		assign(&as[4][0], BUTTON, SDL_CONTROLLER_BUTTON_A, false);
		assign(&as[4][1], BUTTON, STICK_CLICK, false);
		assign(&as[5][0], BUTTON, SDL_CONTROLLER_BUTTON_B, false);
		assign(&as[6][0], BUTTON, SDL_CONTROLLER_BUTTON_X, false);
		assign(&as[7][0], BUTTON, SDL_CONTROLLER_BUTTON_Y, false);
		assign(&as[8][0], BUTTON, SDL_CONTROLLER_BUTTON_START, false);
		assign(&as[9][0], BUTTON, SDL_CONTROLLER_BUTTON_BACK, false);
	}
	else
	{
		// the bare stick has one button, and firing is what it is for
		assign(&as[4][0], BUTTON, 0, false);
	}

	joystick[j].analog = false;
	joystick[j].sensitivity = 5;
	joystick[j].threshold = 5;
}

// fills buffer with comma separated list of assigned joystick functions
void joystick_assignments_to_string( char *buffer, size_t buffer_len, const Joystick_assignment *assignments )
{
	strncpy(buffer, "", buffer_len);

	bool comma = false;
	for (uint i = 0; i < COUNTOF(*joystick->assignment); ++i)
	{
		if (assignments[i].type == NONE)
			continue;

		size_t len = snprintf(buffer, buffer_len, "%s%s",
		                      comma ? ", " : "",
		                      assignment_to_code(&assignments[i]));
		buffer += len;
		buffer_len -= len;

		comma = true;
	}
}

/* gives the short (6 or less characters) identifier for a joystick assignment
 *
 * two of these per direction/action is all that can fit on the joystick config screen
 */
const char *assignment_to_code( const Joystick_assignment *assignment )
{
	static const char *const button_names[SDL_CONTROLLER_BUTTON_MAX + 1] =
	{
		"A", "B", "X", "Y", "BACK", "GUIDE", "START", "LSTICK", "RSTICK",
		"LB", "RB", "UP", "DOWN", "LEFT", "RIGHT", "CLICK",
	};
	static char name[7];

	switch (assignment->type)
	{
	case NONE:
	case HAT:
		strcpy(name, "");
		break;

	case AXIS:
		snprintf(name, sizeof(name), "AX %d%c",
		         assignment->num + 1,
		         assignment->negative_axis ? '-' : '+');
		break;

	case BUTTON:
		if (controller != NULL && assignment->num >= 0 && assignment->num <= STICK_CLICK)
			snprintf(name, sizeof(name), "%s", button_names[assignment->num]);
		else
			snprintf(name, sizeof(name), "BTN %d", assignment->num + 1);
		break;
	}

	return name;
}

// captures joystick input for configuring assignments
// returns false if non-joystick input was detected
bool detect_joystick_assignment( int j, Joystick_assignment *assignment )
{
	// get initial joystick state to compare against to see if anything was pressed

	Sint16 axis[SDL_CONTROLLER_AXIS_MAX];
	const int axes = axis_count();
	for (int i = 0; i < axes; i++)
		axis[i] = read_axis(i);

	Uint8 button[SDL_CONTROLLER_BUTTON_MAX + 1];
	const int buttons = button_count();
	for (int i = 0; i < buttons; i++)
		button[i] = read_button(i);

	bool detected = false;

	do
	{
		setjasondelay(1);

		SDL_PumpEvents();

		for (int i = 0; i < axes; ++i)
		{
			Sint16 temp = read_axis(i);

			if (abs(temp - axis[i]) > joystick_analog_max * 2 / 3)
			{
				assignment->type = AXIS;
				assignment->num = i;
				assignment->negative_axis = temp < axis[i];
				detected = true;
				break;
			}
		}

		for (int i = 0; i < buttons; ++i)
		{
			Uint8 new_button = read_button(i),
			      changed = button[i] ^ new_button;

			if (!changed)
				continue;

			if (new_button == 0) // button was released
			{
				button[i] = new_button;
			}
			else                 // button was pressed
			{
				assignment->type = BUTTON;
				assignment->num = i;
				detected = true;
				break;
			}
		}

		service_SDL_events(true);
		JE_showVGA();

		wait_delay();
	}
	while (!detected && !newkey && !newmouse);

	(void)j;
	return detected;
}

// compares relevant parts of joystick assignments for equality
bool joystick_assignment_cmp( const Joystick_assignment *a, const Joystick_assignment *b )
{
	if (a->type == b->type)
	{
		switch (a->type)
		{
		case NONE:
		case HAT:
			return true;
		case AXIS:
			return (a->num == b->num) &&
			       (a->negative_axis == b->negative_axis);
		case BUTTON:
			return (a->num == b->num);
		}
	}
	return false;
}
