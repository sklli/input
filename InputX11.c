/**********************************************************************
 *
 * PROJECT:		Input library
 * FILE:		InputX11.c
 * LICENCE:		See Licence.txt
 * PURPOSE:		A portable input hooker library.
 *				Functions to query X11 input systems.
 *
 *				(c) Tuomo Jauhiainen 2012-13
 *
 **********************************************************************/

#ifndef _WIN32

#include "Input.h"
#include "InputSys.h"
#include "Platform/Window.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

static syswindow_t* window = NULL;
static uint32 modifier_flags = 0;

void input_platform_initialize( void* wnd )
{
	window = wnd;
}

void input_platform_shutdown( void )
{
	window = NULL;
}

bool input_process( void* data )
{
	XEvent* event = (XEvent*)data;
	XKeyEvent* key;
	XButtonEvent* button;
	XMotionEvent* motion;
	int16 x, y;
	char buf[20];
	KeySym sym;
	uint32 code;
	bool ret = true;

	switch ( event->type )
	{
	case KeyPress:
		{
			key = (XKeyEvent*)event;
			modifier_flags = key->state;

			XLookupString( key, buf, sizeof(buf), &sym, NULL );
			code = (uint32)sym;

			// A dodgy fix to make windows and linux hooks/binds compatible:
			// Convert lowercase characters to upper case before processing hooks.
			if ( code >= 'a' && code <= 'z' ) code -= ( 'a' - 'A' );

			ret = input_handle_keyboard_event( INPUT_KEY_DOWN, code );
			if ( ret )
			{
				ret = input_handle_key_down_bind( code );
			}

			if ( !ret ) return false;
			if ( !*buf ) return ret;

			ret = input_handle_keyboard_event( INPUT_CHARACTER, buf[0] );
			if ( ret )
			{
				ret = input_handle_char_bind( buf[0] );
			}

			return ret;
		}

	case KeyRelease:
		{
			modifier_flags = 0;
			key = (XKeyEvent*)event;
			sym = (uint32)XkbKeycodeToKeysym( window->display, key->keycode, 0, 0 );

			ret = input_handle_keyboard_event( INPUT_KEY_UP, (uint32)sym );
			if ( ret )
			{
				ret = input_handle_key_up_bind( (uint32)sym );
			}

			return ret;
		}

	case ButtonPress:
		{
			button = (XButtonEvent*)event;

			x = (int16)button->x;
			y = (int16)button->y;

			switch ( button->button )
			{
			case Button1:
				// Left mouse button
				XGrabPointer( button->display, button->window, False, ButtonPressMask|ButtonReleaseMask|
								PointerMotionMask|FocusChangeMask|EnterWindowMask|LeaveWindowMask,
								GrabModeAsync, GrabModeAsync, button->window, None, CurrentTime );

				ret = input_handle_mouse_event( INPUT_LBUTTON_DOWN, x, y, MOUSE_LBUTTON, MWHEEL_STATIONARY );
				if ( ret ) input_handle_mouse_down_bind( MOUSE_LBUTTON, x, y );

				break;

			case Button3:
				// Right mouse button
				ret = input_handle_mouse_event( INPUT_RBUTTON_DOWN, x, y, MOUSE_RBUTTON, MWHEEL_STATIONARY );
				if ( ret ) input_handle_mouse_down_bind( MOUSE_RBUTTON, x, y );

				break;

			case Button2:
				// Middle mouse button (wheel)
				ret = input_handle_mouse_event( INPUT_MBUTTON_DOWN, x, y, MOUSE_MBUTTON, MWHEEL_STATIONARY );
				if ( ret ) input_handle_mouse_down_bind( MOUSE_MBUTTON, x, y );

				break;

			case Button4:
				// Mouse wheel scroll up
				ret = input_handle_mouse_event( INPUT_MOUSE_WHEEL, x, y, MOUSE_NONE, MWHEEL_UP );
				break;

			case Button5:
				// Mouse wheel scroll down
				ret = input_handle_mouse_event( INPUT_MOUSE_WHEEL,x, y, MOUSE_NONE, MWHEEL_DOWN );
				break;
			}

			return ret;
		}

	case ButtonRelease:
		{
			button = (XButtonEvent*)event;

			x = (int16)button->x;
			y = (int16)button->y;

			switch ( button->button )
			{
			case Button1:
				// Left mouse button
				XUngrabPointer( button->display, CurrentTime );

				ret = input_handle_mouse_event( INPUT_LBUTTON_UP, x, y, MOUSE_LBUTTON, MWHEEL_STATIONARY );
				if ( ret ) input_handle_mouse_up_bind( MOUSE_LBUTTON, x, y );

				break;

			case Button2:
				// Right mouse button
				ret = input_handle_mouse_event( INPUT_RBUTTON_UP, x, y, MOUSE_RBUTTON, MWHEEL_STATIONARY );
				if ( ret ) input_handle_mouse_up_bind( MOUSE_RBUTTON, x, y );

				break;

			case Button3:
				// Middle mouse button (wheel)
				ret = input_handle_mouse_event( INPUT_MBUTTON_UP, x, y, MOUSE_MBUTTON, MWHEEL_STATIONARY );
				if ( ret ) input_handle_mouse_up_bind( MOUSE_MBUTTON, x, y );

				break;
			}

			return ret;
		}

	case MotionNotify:
		{
			motion = (XMotionEvent*)event;
			x = (int16)motion->x;
			y = (int16)motion->y;

			ret = input_handle_mouse_event( INPUT_MOUSE_MOVE, x, y, MOUSE_NONE, MWHEEL_STATIONARY );
			if ( ret )
			{
				ret = input_handle_mouse_move_bind( x, y );
			}

			return ret;
		}
	}

	return true;
}

bool input_get_key_state( uint32 key )
{
	char keys[32];
	uint32 code;
	uint32 idx;

	switch ( key )
	{
	case MKEY_SHIFT:
		return ( modifier_flags & ShiftMask );

	case MKEY_CONTROL:
		return ( modifier_flags & ControlMask );

	case MKEY_ALT:
		return ( modifier_flags & Mod1Mask );

	case MKEY_RALT:
		return ( modifier_flags & Mod5Mask );

	default:
		code = XKeysymToKeycode( window->display, key );

		XQueryKeymap( window->display, keys );
		idx = code >> 3;

		return keys[idx] & ( (code-idx) << 3 );
	}
}

#endif /* _WIN32 */
