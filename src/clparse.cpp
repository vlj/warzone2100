/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

#include <gflags/gflags.h>
#include <sstream>

#include "lib/framework/frame.h"
#include "lib/framework/opengl.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"

#include "clparse.h"
#include "display3d.h"
#include "frontend.h"
#include "keybind.h"
#include "levels.h"
#include "loadsave.h"
#include "main.h"
#include "modding.h"
#include "multiplay.h"
#include "version.h"
#include "warzoneconfig.h"
#include "wrappers.h"

DEFINE_string(configdir, "", N_("Set configuration directory"));
DEFINE_string(datadir, "", N_("Add data directory"));
DEFINE_string(dbg, "", N_("Show debug for given level"));
DEFINE_string(debugfile, "", N_("Log debug output to file"));
DEFINE_bool(flush_debug_stderr, true, N_("Flush all debug output written to stderr"));
DEFINE_bool(fullscreen, false, N_("Play in fullscreen mode"));
DEFINE_string(game, "", N_("Load a specific game mode"));
DEFINE_string(mod, "", N_("Enable a global mod"));
DEFINE_string(mod_ca, "", N_("Enable a campaign only mod"));
DEFINE_string(mod_mp, "", N_("Enable a multiplay only mod"));
DEFINE_bool(noassert, false, N_("Disable asserts"));
DEFINE_bool(crash, false, N_("Causes a crash to test the crash handler"));
DEFINE_string(loadskirmish, "", N_("Load a saved skirmish game"));
DEFINE_string(loadcampaign, "", N_("Load a saved campaign game"));
DEFINE_bool(vers, false, N_("Show version information and exit"));
DEFINE_string(resolution, "", N_("Set the resolution (WIDTHxHEIGHT) to use"));
DEFINE_bool(shadows, true, N_("Enable shadows"));
DEFINE_bool(sound, true, N_("Enable sound"));
DEFINE_string(join, "", N_("Connect directly to IP/hostname"));
DEFINE_bool(host, false, N_("Go directly to host screen"));
DEFINE_bool(texturecompression, true, N_("Enable texture compression"));
DEFINE_bool(autogame, false, N_("Run games automatically for testing"));
DEFINE_string(saveandquit, "", N_("Immediately save game and quit"));
DEFINE_string(skirmish, "", N_("Start skirmish game with given settings file"));

//////
// Our fine replacement for the popt abomination follows

#define POPT_ERROR_BADOPT -1
#define POPT_SKIP_MAC_PSN 666


/// TODO: Find a way to use the real qFatal from Qt
#undef qFatal
#define qFatal(...)                                                                                                    \
	{                                                                                                                  \
		fprintf(stderr, __VA_ARGS__);                                                                                  \
		fprintf(stderr, "\n");                                                                                         \
		exit(1);                                                                                                       \
	}

/// Enable automatic test games
static bool wz_autogame = false;
static std::string wz_saveandquit;
static std::string wz_test;


//! Early parsing of the commandline
/**
 * First half of the command line parsing. Also see ParseCommandLine()
 * below. The parameters here are needed early in the boot process,
 * while the ones in ParseCommandLine can benefit from debugging being
 * set up first.
 * \param argc number of arguments given
 * \param argv string array of the arguments
 * \return Returns true on success, false on error */
bool ParseCommandLineEarly(int argc, const char **argv)
{
#if defined(WZ_OS_MAC) && defined(DEBUG)
	debug_enable_switch("all");
#endif /* WZ_OS_MAC && DEBUG */

	if (!FLAGS_dbg.empty())
	{
		if (!debug_enable_switch(FLAGS_dbg.c_str()))
		{
			qFatal("Debug flag \"%s\" not found!", FLAGS_dbg.c_str());
		}
	}

	if (!FLAGS_debugfile.empty())
	{
		debug_register_callback(debug_callback_file, debug_callback_file_init, debug_callback_file_exit,
								(void *)FLAGS_debugfile.c_str());
		customDebugfile = true;
	}

	if (FLAGS_flush_debug_stderr)
	{
		debugFlushStderr();
	}

	if (!FLAGS_configdir.empty())
	{
		sstrcpy(configdir, FLAGS_configdir.c_str());
	}

	if (FLAGS_vers)
	{
		printf("Warzone 2100 - %s\n", version_getFormattedVersionString());
		return false;
	}

	return true;
}

//! second half of parsing the commandline
/**
 * Second half of command line parsing. See ParseCommandLineEarly() for
 * the first half. Note that render mode must come before resolution flag.
 * \param argc number of arguments given
 * \param argv string array of the arguments
 * \return Returns true on success, false on error */
bool ParseCommandLine(int argc, const char **argv)
{
	if (FLAGS_noassert)
	{
		kf_NoAssert();
	}

	if (FLAGS_crash)
	{
		// NOTE: The sole purpose of this is to test the crash handler.
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
		sstrcpy(iptoconnect, FLAGS_join.c_str());
	}

	if (FLAGS_host)
	{
		// go directly to host screen, bypass all others.
		hostlaunch = 1;
	}

	if (!FLAGS_game.empty())
	{
		if (FLAGS_game != "CAM_1A" && FLAGS_game != "CAM_2A" && FLAGS_game != "CAM_3A" && FLAGS_game != "TUTORIAL3" &&
			FLAGS_game != "FASTPLAY")
		{
			qFatal("The game parameter requires one of the following keywords:"
				   "CAM_1A, CAM_2A, CAM_3A, TUTORIAL3, or FASTPLAY.");
		}
		NetPlay.bComms = false;
		bMultiPlayer = false;
		bMultiMessages = false;
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			NET_InitPlayer(i, true, false);
		}

		// NET_InitPlayer deallocates Player 0, who must be allocated so that a later invocation of
		// processDebugMappings does not trigger DEBUG mode
		NetPlay.players[0].allocated = true;

		if (FLAGS_game == "CAM_1A" || FLAGS_game == "CAM_2A" || FLAGS_game == "CAM_3A")
		{
			game.type = CAMPAIGN;
		}
		else
		{
			game.type = SKIRMISH; // tutorial is skirmish for some reason
		}
		sstrcpy(aLevelName, FLAGS_game.c_str());
		SetGameMode(GS_NORMAL);
	}

	if (!FLAGS_mod.empty())
	{
		global_mods.push_back(FLAGS_mod);
	}

	if (!FLAGS_mod_ca.empty())
	{
		campaign_mods.push_back(FLAGS_mod);
	}

	if (!FLAGS_mod_mp.empty())
	{
		multiplay_mods.push_back(FLAGS_mod);
	}

	if (!FLAGS_resolution.empty())
	{
		unsigned int width, height;
		auto is = std::istringstream(FLAGS_resolution);
		std::string str;
		if (is >> width >> str >> height)
		{
			qFatal("Invalid parameter specified (format is WIDTHxHEIGHT, e.g. 800x600)");
		}
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

	if (FLAGS_shadows)
	{
		setDrawShadows(true);
	}

	if (FLAGS_sound)
	{
		war_setSoundEnabled(false);
	}

	if (FLAGS_texturecompression)
	{
		wz_texture_compression = true;
	}

	if (FLAGS_autogame)
	{
		wz_autogame = true;
	}

	if (!FLAGS_saveandquit.empty())
	{
		if (!strchr(FLAGS_saveandquit.c_str(), '/'))
		{
			qFatal("Bad savegame name (needs to be a full path)");
		}
		wz_saveandquit = FLAGS_saveandquit;
	}

	if (!FLAGS_skirmish.empty())
	{
		wz_test = FLAGS_skirmish;
	}

	return true;
}

bool autogame_enabled() { return wz_autogame; }

const std::string &saveandquit_enabled() { return wz_saveandquit; }

const std::string &wz_skirmish_test() { return wz_test; }
