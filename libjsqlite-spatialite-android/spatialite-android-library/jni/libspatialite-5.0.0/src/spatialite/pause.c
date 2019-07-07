/*

 pause.c -- implementing PAUSE() for both Unix/Linux and Windows

 version 5.0, 2018 May 28

 Author: Sandro Furieri a.furieri@lqt.it

 ------------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2008-2018
the Initial Developer. All Rights Reserved.

Contributor(s):

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/

#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <unistd.h>
#endif

#include <spatialite_private.h>

#ifdef _WIN32

/* Windows implementation */

SPATIALITE_PRIVATE void
splite_pause_windows (void)
{
    HANDLE hStdin;
    DWORD fdwSaveOldMode;
    DWORD fdwMode;
    DWORD cNumRead;
    DWORD i;
    INPUT_RECORD irInBuf[128];
    int go = 1;

/* Get the standard input handle */
    hStdin = GetStdHandle (STD_INPUT_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE)
	return;

/* Save the current input mode, to be restored on exit */
    if (!GetConsoleMode (hStdin, &fdwSaveOldMode))
	return;

/* Enable window input events */
    fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
    if (!SetConsoleMode (hStdin, fdwMode))
	return;
/* flush to remove existing events */
    FlushConsoleInputBuffer (hStdin);

/* Wait for console events. */
    fprintf (stderr,
	     "***************  PAUSE  ***************  Hit any key to continue\n");
    while (go)
      {
	  if (!ReadConsoleInput (hStdin,	// input buffer handle 
				 irInBuf,	// buffer to read into 
				 128,	// size of read buffer 
				 &cNumRead))	// number of records read 
	      goto reset;

	  for (i = 0; i < cNumRead; i++)
	    {
		/* testing events */
		switch (irInBuf[i].EventType)
		  {
		  case KEY_EVENT:	/* keyboard input */
		      go = 0;
		      break;
		  }
	    }
      }
  reset:
    fprintf (stderr, "*************** resuming execution after PAUSE\n");

/* Restore input mode on exit. */
    SetConsoleMode (hStdin, fdwSaveOldMode);
}

/* end Windows implementation */

#else

/* Unix/Linux implementation */

void
sig_handler (int signum)
{
    if (signum == SIGCONT)
      {
	  fprintf (stderr,
		   "*************** SIGCONT: resuming execution after PAUSE\n");
	  fflush (stderr);
      }
}

SPATIALITE_PRIVATE void
splite_pause_signal (void)
{
/* installing the signal handlers */
    signal (SIGSTOP, sig_handler);
    signal (SIGCONT, sig_handler);

/* raising a SIGSTOP signal to the current process */
    fprintf (stderr, "***************  PAUSE  ***************\n");
    fprintf (stderr, "command for resuming execution is:\nkill -SIGCONT %d\n",
	     getpid ());
    fflush (stderr);
    raise (SIGSTOP);
}

/* end Unix/Linux implementation */

#endif
