/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/*
 * clParse.c
 *
 * Parse command line arguments
 *
 */
#include <QString>
#include "lib/gamelib/frame.h"
#include "lib/ivis_opengl/opengl.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"

#include "clparse.h"
#include "display3d.h"
#include "frontend.h"
#include "keybind.h"
#include "loadsave.h"
#include "main.h"
#include "modding.h"
#include "multiplay.h"
#include "version.h"
#include "warzoneconfig.h"
#include "wrappers.h"

//////
// Our fine replacement for the popt abomination follows

#define POPT_ARG_STRING true
#define POPT_ARG_NONE false
#define POPT_ERROR_BADOPT -1
#define POPT_SKIP_MAC_PSN 666

/// TODO: Find a way to use the real qFatal from Qt
#undef qFatal
#define qFatal(...) { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); }

/// Enable automatic test games
static bool wz_autogame = false;

DEFINE_string(configdir, "", "Set configuration directory");
DEFINE_string(datadir, "", "Set default data directory");
DEFINE_string(debug, "", "Show debug for given level");
DEFINE_string(debug_file, "", "Log debug output to file");
DEFINE_bool(flush_debug_stderr, true, "Flush all debug output written to stderr");
DEFINE_bool(fullscreen, false, "Play in fullscreen mode");
DEFINE_string(game, "", "Load a specific game");
DEFINE_string(mod, "", "Enable a global mod");
DEFINE_string(mod_ca, "", "Enable a campaign only mod");
DEFINE_string(mod_mp, "", "Enable a multiplay only mod");
DEFINE_bool(noassert, true, "Disable asserts");
DEFINE_bool(crash, false, "Causes a crash to test the crash handler");
DEFINE_string(loadskirmish, "", "Load a saved skirmish game");
DEFINE_string(loadcampaign, "", "Load a saved campaign game");
DEFINE_bool(window, true, "Play in windowed mode");
DEFINE_bool(ver, false, "Show version information and exit");
DEFINE_string(resolution, "", "Set the resolution to use");
DEFINE_bool(shadows, true, "Enable shadows");
DEFINE_bool(sound, true, "Enable sound");
DEFINE_string(join, "", "Connect directly to IP / hostname");
DEFINE_bool(host, false, "Go directly to host screen");
DEFINE_bool(texturecompression, true, "Enable texture compression");
DEFINE_bool(autogame, false, "Run games automatically for testing");


//! Early parsing of the commandline
/**
 * First half of the command line parsing. Also see ParseCommandLine()
 * below. The parameters here are needed early in the boot process,
 * while the ones in ParseCommandLine can benefit from debugging being
 * set up first.
 * \param argc number of arguments given
 * \param argv string array of the arguments
 * \return Returns true on success, false on error */
bool ParseCommandLineEarly(int argc, char **argv)
{
	gflags::ParseCommandLineFlags(&argc, &argv, false);

#if defined(WZ_OS_MAC) && defined(DEBUG)
	debug_enable_switch("all");
#endif /* WZ_OS_MAC && DEBUG */


	if (!FLAGS_debug_file.empty())
	{
		debug_register_callback(debug_callback_file, debug_callback_file_init, debug_callback_file_exit, (void *)FLAGS_debug_file.c_str());
		customDebugfile = true;
	}

	if (FLAGS_flush_debug_stderr)
	{
		debugFlushStderr();
	}

	if (!FLAGS_configdir.empty())
	{
		strcpy(configdir, FLAGS_configdir.c_str());
	}

	if (FLAGS_ver)
	{
		printf("Warzone 2100 - %s\n", version_getFormattedVersionString());
	}



	/*
		{
		case CLI_DEBUG:
			// retrieve the debug section name
			token = poptGetOptArg(poptCon);
			if (token == NULL)
			{
				qFatal("Usage: --debug=<flag>");
			}

			// Attempt to enable the given debug section
			if (!debug_enable_switch(token))
			{
				qFatal("Debug flag \"%s\" not found!", token);
			}
			break;
*/
	return true;
}

//! second half of parsing the commandline
/**
 * Second half of command line parsing. See ParseCommandLineEarly() for
 * the first half. Note that render mode must come before resolution flag.
 * \param argc number of arguments given
 * \param argv string array of the arguments
 * \return Returns true on success, false on error */
bool ParseCommandLine(int argc, char **argv)
{

	if (FLAGS_noassert)
	{
		kf_NoAssert();
	}

	// NOTE: The sole purpose of this is to test the crash handler.
	if (FLAGS_crash)
	{
		CauseCrash = true;
		NetPlay.bComms = false;
		sstrcpy(aLevelName, "CAM_3A");
		SetGameMode(GS_NORMAL);
	}

	if (!FLAGS_datadir.empty())
	{
		sstrcpy(datadir, FLAGS_datadir.c_str());
	}

	if (FLAGS_fullscreen)
	{
		war_setFullscreen(true);
	}

	if (!FLAGS_join.empty())
	{
		//get the ip we want to connect with, and go directly to join screen.
		sstrcpy(iptoconnect, FLAGS_join.c_str());
	}

	if (FLAGS_host)
	{
		hostlaunch = true;
	}

/*	if (!FLAGS_game.empty())
	{
		if (token == NULL
			|| (strcmp(token, "CAM_1A") && strcmp(token, "CAM_2A") && strcmp(token, "CAM_3A")
				&& strcmp(token, "TUTORIAL3") && strcmp(token, "FASTPLAY")))
		{
			qFatal("The game parameter requires one of the following keywords:"
				"CAM_1A, CAM_2A, CAM_3A, TUTORIAL3, or FASTPLAY.");
		}
		NetPlay.bComms = false;
		bMultiPlayer = false;
		bMultiMessages = false;
		NetPlay.players[0].allocated = true;
		if (!strcmp(token, "CAM_1A") || !strcmp(token, "CAM_2A") || !strcmp(token, "CAM_3A"))
		{
			game.type = CAMPAIGN;
		}
		else
		{
			game.type = SKIRMISH; // tutorial is skirmish for some reason
		}
		sstrcpy(aLevelName, FLAGS_game.c_str());
		SetGameMode(GS_NORMAL);
	}*/
	
	if (!FLAGS_mod.empty())
	{
		global_mods.push_back(FLAGS_mod);
	}

	if (!FLAGS_mod_ca.empty())
	{
		campaign_mods.push_back(FLAGS_mod_ca);
	}

	if (!FLAGS_mod_mp.empty())
	{
		multiplay_mods.push_back(FLAGS_mod_mp);
	}

	if (!FLAGS_resolution.empty())
	{
		unsigned int width, height;

		std::stringstream stream(FLAGS_resolution);
		stream >> width >> height;
		if (width < 640)
		{
			debug(LOG_ERROR, "Screen width < 640 unsupported, using 640");
			width = 640;
		}
		if (height < 480)
		{
			debug(LOG_ERROR, "Screen height < 480 unsupported, using 480");
			height = 480;
		}
		// tell the display system of the desired resolution
		pie_SetVideoBufferWidth(width);
		pie_SetVideoBufferHeight(height);
		// and update the configuration
		war_SetWidth(width);
		war_SetHeight(height);
	}
	if (!FLAGS_loadskirmish.empty())
	{
		snprintf(saveGameName, sizeof(saveGameName), "%s/skirmish/%s.gam", SaveGamePath, FLAGS_loadskirmish.c_str());
		SPinit();
		bMultiPlayer = true;
		game.type = SKIRMISH; // tutorial is skirmish for some reason
		SetGameMode(GS_SAVEGAMELOAD);
	}

	if (!FLAGS_loadcampaign.empty())
	{
		snprintf(saveGameName, sizeof(saveGameName), "%s/campaign/%s.gam", SaveGamePath, FLAGS_loadcampaign.c_str());
		SPinit();
		SetGameMode(GS_SAVEGAMELOAD);
	}

	if (FLAGS_fullscreen)
	{
		war_setFullscreen(false);
	}

	if (FLAGS_shadows)
	{
		setDrawShadows(true);
	}
	else
	{
		setDrawShadows(false);
	}

	if (FLAGS_sound)
	{
		war_setSoundEnabled(true);
	}
	else
	{
		war_setSoundEnabled(false);
	}

	if (FLAGS_texturecompression)
	{
		wz_texture_compression = GL_COMPRESSED_RGBA_ARB;
	}
	else
	{
		wz_texture_compression = GL_RGBA;
	}

	if (FLAGS_autogame)
	{
		wz_autogame = true;
	}

	return true;
}

bool autogame_enabled()
{
	return wz_autogame;
}
