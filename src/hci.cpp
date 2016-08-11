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
/**
 * @file hci.c
 *
 * Functions for the in game interface.
 * (Human Computer Interface - thanks to Alex for the file name).
 *
 * Obviously HCI should mean "Hellish Code Incoming" -- Toksyuryel.
 *
 */

#include <string.h>
#include <algorithm>
#include <functional>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/stdio_ext.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/piestate.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/screen.h"
#include "lib/script/script.h"
#include "lib/netplay/netplay.h"

#include "action.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
#include "lib/widget/label.h"
#include "lib/widget/bar.h"
#include "lib/widget/button.h"
#include "lib/widget/editbox.h"
#include "cheat.h"
#include "console.h"
#include "design.h"
#include "display.h"
#include "display3d.h"
#include "drive.h"
#include "edit3d.h"
#include "effects.h"
#include "game.h"
#include "hci.h"
#include "ingameop.h"
#include "intdisplay.h"
#include "intelmap.h"
#include "intorder.h"
#include "keymap.h"
#include "loadsave.h"
#include "loop.h"
#include "mapdisplay.h"
#include "mission.h"
#include "multimenu.h"
#include "multiplay.h"
#include "multigifts.h"
#include "radar.h"
#include "research.h"
#include "scriptcb.h"
#include "scriptextern.h"
#include "scripttabs.h"
#include "transporter.h"
#include "warcam.h"
#include "main.h"
#include "template.h"
#include "wrappers.h"
#include "keybind.h"
#include "qtscript.h"
#include "frend.h"

// Is a button widget highlighted, either because the cursor is over it or it is flashing.
//
#define buttonIsHilite(p)  ((p->getState() & WBUT_HIGHLIGHT) != 0)

#define RETXOFFSET (0)// Reticule button offset
#define RETYOFFSET (0)
#define NUMRETBUTS	7 // Number of reticule buttons.

enum  				  // Reticule button indecies.
{
	RETBUT_CANCEL,
	RETBUT_FACTORY,
	RETBUT_RESEARCH,
	RETBUT_BUILD,
	RETBUT_DESIGN,
	RETBUT_INTELMAP,
	RETBUT_COMMAND,
};

struct BUTSTATE
{
	uint32_t id;
	bool Enabled;
	bool Hidden;
};

struct BUTOFFSET
{
	SWORD x;
	SWORD y;
};

/***************************************************************************************/
/*                  Widget ID numbers                                                  */

#define IDPOW_FORM			100		// power bar form

/* Option screen IDs */
#define IDOPT_FORM			1000		// The option form
#define IDOPT_CLOSE			1006		// The close button
#define IDOPT_LABEL			1007		// The Option screen label
#define IDOPT_PLAYERFORM	1008		// The player button form
#define IDOPT_PLAYERLABEL	1009		// The player form label
#define IDOPT_PLAYERSTART	1010		// The first player button
#define IDOPT_PLAYEREND		1030		// The last possible player button
#define IDOPT_QUIT			1031		// Quit the game
#define IDOPT_DROID			1037		// The place droid button
#define IDOPT_STRUCT		1038		// The place struct button
#define IDOPT_FEATURE		1039		// The place feature button
#define IDOPT_IVISFORM		1043		// iViS engine form
#define IDOPT_IVISLABEL		1044		// iViS form label

#define	IDPROX_START		120000		// The first proximity button
#define	IDPROX_END		129999		// The last proximity button

#define PROX_BUTWIDTH		9
#define PROX_BUTHEIGHT		9
/***************************************************************************************/
/*                  Widget Positions                                                   */

/* Reticule positions */
#define RET_BUTWIDTH		25
#define RET_BUTHEIGHT		28

/* Option positions */
#define OPT_X			(640-300)
#define OPT_Y			20
#define OPT_WIDTH		275
#define OPT_HEIGHT		350
#define OPT_BUTWIDTH	60
#define OPT_BUTHEIGHT	20
#define OPT_EDITY		50
#define OPT_PLAYERY		150

#define STAT_BUTX				4
#define STAT_BUTY				2

/* Structure type screen positions */
#define STAT_GAP			2
#define STAT_BUTWIDTH		60
#define STAT_BUTHEIGHT		46

namespace
{
// Reticule button form relative positions.
constexpr std::array<BUTOFFSET, NUMRETBUTS> ReticuleOffsets =
{
	BUTOFFSET{ 48, 47 },	// RETBUT_CANCEL,
	BUTOFFSET{ 53, 15 },	// RETBUT_FACTORY,
	BUTOFFSET{ 87, 33 },	// RETBUT_RESEARCH,
	BUTOFFSET{ 87, 68 },	// RETBUT_BUILD,
	BUTOFFSET{ 53, 86 },	// RETBUT_DESIGN,
	BUTOFFSET{ 19, 68 },	// RETBUT_INTELMAP,
	BUTOFFSET{ 19, 33 },	// RETBUT_COMMAND,
};

/* Close strings */
const std::string pCloseText = "X";

/* Player button strings */
const std::array<std::string, 16> apPlayerText =
{
	"0", "1", "2", "3", "4", "5", "6", "7",
	"8", "9", "10", "11", "12", "13", "14", "15",
};

const std::array<std::string, 16> apPlayerTip =
{
	"Select Player 0",
	"Select Player 1",
	"Select Player 2",
	"Select Player 3",
	"Select Player 4",
	"Select Player 5",
	"Select Player 6",
	"Select Player 7",
	"Select Player 8",
	"Select Player 9",
	"Select Player 10",
	"Select Player 11",
	"Select Player 12",
	"Select Player 13",
	"Select Player 14",
	"Select Player 15",
};
}

/* The widget screen */
W_SCREEN		*psWScreen;

// The last widget ID from widgRunScreen
UDWORD				intLastWidget;

INTMODE intMode;

/* Status of the positioning for the object placement */
static enum _edit_pos_mode
{
	IED_NOPOS,
	IED_POS,
} editPosMode;

/* Which type of object screen is being displayed. Starting value is where the intMode left off*/
static enum _obj_mode
{
	IOBJ_NONE = INT_MAXMODE,	// Nothing doing.
	IOBJ_BUILD,			        // The build screen
	IOBJ_BUILDSEL,		        // Selecting a position for a new structure
	IOBJ_DEMOLISHSEL,	        // Selecting a structure to demolish
	IOBJ_MANUFACTURE,	        // The manufacture screen
	IOBJ_RESEARCH,		        // The research screen
	IOBJ_COMMAND,		        // the command droid screen

	IOBJ_MAX,			        // maximum object mode
} objMode;

/* The current stats list being used by the stats screen */
static BASE_STATS		**ppsStatsList;
static UDWORD			numStatsListEntries;

/* The button ID of the objects stat when the stat screen is displayed */
UDWORD			objStatID;

/* The stats for the current getStructPos */
static BASE_STATS		*psPositionStats;

/* Store a list of Template pointers for Droids that can be built */
std::vector<DROID_TEMPLATE *>   apsTemplateList;
std::list<DROID_TEMPLATE>       localTemplates;

/* Store a list of component stats pointers for the design screen */
UDWORD			numComponent;
COMPONENT_STATS	**apsComponentList;
UDWORD			numExtraSys;
COMPONENT_STATS	**apsExtraSysList;

/***************************************************************************************/
/*              Function Prototypes                                                    */
/* Remove the object widgets from the widget screen */
void intRemoveObject(void);
static void intRemoveObjectNoAnim(void);

/* Start looking for a structure location */
static void intStartStructPosition(BASE_STATS *psStats);
/* Stop looking for a structure location */
static void intStopStructPosition(void);

/******************Power Bar Stuff!**************/

static void intRunStats(void);

//proximity display stuff
static void processProximityButtons(UDWORD id);

static DROID *intCheckForDroid(UDWORD droidType);
static STRUCTURE *intCheckForStructure(UDWORD structType);

// count the number of selected droids of a type
static SDWORD intNumSelectedDroids(UDWORD droidType);


/***************************GAME CODE ****************************/

struct RETBUTSTATS
{
	int downTime;
	QString filename;
	QString filenameDown;
	QString tip;
	int flashing;
	int flashTime;
};
static RETBUTSTATS retbutstats[NUMRETBUTS];

void setReticuleStats(int ButId, QString tip, QString filename, QString filenameDown)
{
	retbutstats[ButId].tip = tip;
	retbutstats[ButId].filename = filename;
	retbutstats[ButId].filenameDown = filenameDown;
	retbutstats[ButId].downTime = 0;
	retbutstats[ButId].flashing = 0;
	retbutstats[ButId].flashTime = 0;
}

static void intDisplayReticuleButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset)
{
	int     x = xOffset + psWidget->x();
	int     y = yOffset + psWidget->y();
	bool	Hilight = false;
	bool	Down = false;
	UBYTE	DownTime = retbutstats[psWidget->UserData].downTime;
	UBYTE	flashing = retbutstats[psWidget->UserData].flashing;
	UBYTE	flashTime = retbutstats[psWidget->UserData].flashTime;
	ASSERT_OR_RETURN(, psWidget->type == WIDG_BUTTON, "Not a button");
	W_BUTTON *psButton = (W_BUTTON *)psWidget;

	if (psButton->state & WBUT_DISABLE)
	{
		iV_DrawImage(IntImages, IMAGE_RETICULE_GREY, x, y);
		return;
	}

	Down = psButton->state & (WBUT_DOWN | WBUT_CLICKLOCK);
	Hilight = buttonIsHilite(psButton);

	if (Down)
	{
		if ((DownTime < 1) && (psWidget->UserData != RETBUT_CANCEL))
		{
			iV_DrawImage(IntImages, IMAGE_RETICULE_BUTDOWN, x, y);
		}
		else
		{
			iV_DrawImage2(retbutstats[psWidget->UserData].filenameDown, x, y);
		}
		DownTime++;
		flashing = false;	// stop the reticule from flashing if it was
	}
	else
	{
		if (flashing)
		{
			if (((realTime / 250) % 2) != 0)
			{
				iV_DrawImage2(retbutstats[psWidget->UserData].filenameDown, x, y);
				flashTime = 0;
			}
			else
			{
				iV_DrawImage2(retbutstats[psWidget->UserData].filename, x, y);
			}
			flashTime++;
		}
		else
		{
			iV_DrawImage2(retbutstats[psWidget->UserData].filename, x, y);
			DownTime = 0;
		}
	}
	if (Hilight)
	{
		if (psWidget->UserData == RETBUT_CANCEL)
		{
			iV_DrawImage(IntImages, IMAGE_CANCEL_HILIGHT, x, y);
		}
		else
		{
			iV_DrawImage(IntImages, IMAGE_RETICULE_HILIGHT, x, y);
		}
	}
	retbutstats[psWidget->UserData].flashTime = flashTime;
	retbutstats[psWidget->UserData].flashing = flashing;
	retbutstats[psWidget->UserData].downTime = DownTime;
}

struct human_computer_interface
{
	human_computer_interface();
	~human_computer_interface();

	void update();
	void displayWidgets();
	INT_RETVAL display();
	bool addReticule();
	bool addPower();
	void removeReticule();

	void setMapPos(uint32_t x, uint32_t y);
	void notifyNewObject(BASE_OBJECT *);
	void notifyBuildFinished(DROID *psDroid);
	void notifyBuildStarted(DROID *psDroid);
	void notifyResearchFinished(STRUCTURE *psBuilding);
	void notifyManufactureFinished(STRUCTURE *psBuilding);
	void updateManufacture(STRUCTURE *psBuilding);
	bool coordInBuild(int x, int y);

	void objectSelected(BASE_OBJECT *psObj);
	void constructorSelected(DROID *psDroid);
	bool buildSelectMode();
	bool demolishSelectMode();
	bool buildMode();
	void commanderSelected(DROID *psDroid);
	void addIntelScreen();
	void resetScreen(bool NoAnim);
	void refreshScreen();
	bool addOptions();
	void removeStats();
	void removeStatsNoAnim();
	STRUCTURE *interfaceStructList();
	void addTransporterInterface(DROID *psSelected, bool onMission);
	void flashReticuleButton(uint32_t buttonID);
	void stopReticuleButtonFlash(uint32_t buttonID);

	void togglePowerBar();
	void showPowerBar();
	void hidePowerBar();

	void forceHidePowerBar();
	bool addProximityButton(PROXIMITY_DISPLAY *psProxDisp, uint32_t inc);
	void removeProximityButton(PROXIMITY_DISPLAY *psProxDisp);

	void setKeyButtonMapping(uint32_t val);

	STRUCTURE *findAStructure();
	STRUCTURE *gotoNextStructureType(uint32_t structType, bool JumpTo, bool CancelDrive);
	DROID *gotoNextDroidType(DROID *CurrDroid, DROID_TYPE droidType, bool AllowGroup);

	int getResearchState();
	void notifyResearchButton(int prevState);

	bool checkReticuleButEnabled(uint32_t id);
	BASE_OBJECT *getCurrentSelected();
	void resetPreviousObj();
	bool isRefreshing();
	void demolishCancel();

	StateButton *makeObsoleteButton(WIDGET *parent);  ///< Makes a button to toggle showing obsolete items.

	void chatDialog(int mode);
	bool isChatUp();
	bool isSecondaryWindowUp();

protected:
	/* Store a list of stats pointers from the main structure stats */
	std::array<STRUCTURE_STATS *, MAXSTRUCTURES> apsStructStatsList;
	/* Store a list of research pointers for topics that can be performed*/
	std::array<RESEARCH *, MAXRESEARCH> ppResearchList;
	/*Store a list of research indices which can be performed*/
	std::array<UWORD, MAXRESEARCH> pList;
	std::array<UWORD, MAXRESEARCH> pSList;
	/* Store a list of Feature pointers for features to be placed on the map */
	std::array<FEATURE_STATS *, MAXFEATURES> apsFeatureList;
	// store the objects that are being used for the object bar
	std::vector<BASE_OBJECT *> apsObjectList;
	/* The selected object on the object screen when the stats screen is displayed */
	BASE_OBJECT *psObjSelected;
	/* Whether the objects that are on the object screen have changed this frame */
	bool objectsChanged;
	/* The jump position for each object on the base bar */
	std::vector<Vector2i> asJumpPos;
	bool ReticuleUp = false;
	// Reticule button enable states.
	std::array<BUTSTATE, NUMRETBUTS> ReticuleEnabled;
	bool refreshPending = false;
	bool refreshing = false;
	/* Flags to check whether the power bars are currently on the screen */
	bool powerBarUp = false;
	bool statsUp = false;
	BASE_OBJECT *psStatsScreenOwner = nullptr;
	/* The previous object for each object bar */
	std::array<BASE_OBJECT *, IOBJ_MAX> apsPreviousObj;
	// Empty edit window
	bool secondaryWindowUp = false;
	// Chat dialog
	bool ChatDialogUp = false;
	STRUCTURE *CurrentStruct = nullptr;
	int32_t CurrentStructType = 0;
	DROID *CurrentDroid = nullptr;
	DROID_TYPE CurrentDroidType = DROID_ANY;
	/* The button ID of an objects stat on the stat screen if it is locked down */
	uint32_t statID;
	uint32_t keyButtonMapping = 0;

	/* functions to select and get stats from the current object list */
	std::function<bool(BASE_OBJECT *)> objSelectFunc; /* Function for selecting a base object while building the object screen */
	std::function<BASE_STATS*(BASE_OBJECT*)> objGetStatsFunc; /* Function type for getting the appropriate stats for an object */
	std::function<bool(BASE_OBJECT *, BASE_STATS *)> objSetStatsFunc; /* Function type for setting the appropriate stats for an object */

	/* Process return codes from the Options screen */
	void processOptions(uint32_t id);
	/* Set the shadow for the PowerBar */
	void runPower();
	/* Add the stats screen for a given object */
	void addObjectStats(BASE_OBJECT *psObj, uint32_t id);
	/* Process return codes from the object screen */
	void processObject(uint32_t id);
	/* Add the object screen widgets to the widget screen.
	* select is a pointer to a function that returns true when the object is
	* to be added to the screen.
	* getStats is a pointer to a function that returns the appropriate stats
	* for the object.
	* If psSelected != NULL it specifies which object should be hilited.
	*/
	bool addObjectWindow(BASE_OBJECT *psObjects, BASE_OBJECT *psSelected, bool bForceStats);

	bool updateObject(BASE_OBJECT *psObjects, BASE_OBJECT *psSelected, bool bForceStats);
	/* Add the build widgets to the widget screen */
	/* If psSelected != NULL it specifies which droid should be hilited */
	bool addBuild(DROID *psSelected);
	/* Add the manufacture widgets to the widget screen */
	/* If psSelected != NULL it specifies which factory should be hilited */
	bool addManufacture(STRUCTURE *psSelected);
	/* Add the research widgets to the widget screen */
	/* If psSelected != NULL it specifies which droid should be hilited */
	bool addResearch(STRUCTURE *psSelected);
	/* Add the command droid widgets to the widget screen */
	/* If psSelected != NULL it specifies which droid should be hilited */
	bool addCommand(DROID *psSelected);
	/*puts the selected players factories in order - Standard factories 1-5, then
	cyborg factories 1-5 and then Vtol factories 1-5*/
	void orderFactories();

	void resetWindows(BASE_OBJECT *psObj);
	/*Deals with the RMB click for the Object Stats buttons */
	void objStatRMBPressed(uint32_t id);
	// Refresh widgets once per game cycle if pending flag is set.
	//
	void doScreenRefresh();
	/* Process return codes from the stats screen */
	void processStats(uint32_t id);
	//reorder the research facilities so that first built is first in the list
	void orderResearch();
	// reorder the commanders
	void orderDroids();
	/** Order the objects in the bottom bar according to their type. */
	void orderObjectInterface();
	// Rebuilds apsObjectList, and returns the index of psBuilding in apsObjectList, or returns apsObjectList.size() if not present (not sure whether that's supposed to be possible).
	unsigned rebuildFactoryListAndFindIndex(STRUCTURE *psBuilding);
	/**
	* Get the object refered to by a button ID on the object screen. This works for object or stats buttons.
	*/
	BASE_OBJECT *getObject(uint32_t id);
	/*Deals with the RMB click for the Object screen */
	void objectRMBPressed(uint32_t id);
	/*Deals with the RMB click for the stats screen */
	void statsRMBPressed(uint32_t id);
	/* Reset the stats button for an object */
	void setStats(uint32_t id, BASE_STATS *psStats);
	// clean up when an object dies
	void objectDied(uint32_t objID);
	// Set the x,y members of a button widget initialiser given a reticule button index.
	//
	void setReticuleBut(int ButId);
	// Check that each reticule button has the structure or droid required for it and
	// enable/disable accordingly.
	//
	void checkReticuleButtons();
	/* Add the stats widgets to the widget screen */
	/* If psSelected != NULL it specifies which stat should be hilited
	psOwner specifies which object is hilighted on the object bar for this stat*/
	bool addStats(BASE_STATS **ppsStatsList, uint32_t numStats, BASE_STATS *psSelected, BASE_OBJECT *psOwner);
};

human_computer_interface::human_computer_interface()
{
	ReticuleEnabled = {
		BUTSTATE{ IDRET_CANCEL, false, false },
		BUTSTATE{ IDRET_MANUFACTURE, false, false },
		BUTSTATE{ IDRET_RESEARCH, false, false },
		BUTSTATE{ IDRET_BUILD, false, false },
		BUTSTATE{ IDRET_DESIGN, false, false },
		BUTSTATE{ IDRET_INTEL_MAP, false, false },
		BUTSTATE{ IDRET_COMMAND, false, false },
	};
	widgSetTipColour(WZCOL_TOOLTIP_TEXT);
	WidgSetAudio(WidgetAudioCallback, ID_SOUND_BUTTON_CLICK_3, ID_SOUND_SELECT, ID_SOUND_BUILD_FAIL);

	/* Create storage for Templates that can be built */
	apsTemplateList.clear();

	/* Create storage for the component list */
	apsComponentList = (COMPONENT_STATS **)malloc(sizeof(COMPONENT_STATS *) * MAXCOMPONENT);

	/* Create storage for the extra systems list */
	apsExtraSysList = (COMPONENT_STATS **)malloc(sizeof(COMPONENT_STATS *) * MAXEXTRASYS);

	psObjSelected = nullptr;

	intInitialiseGraphics();

	psWScreen = new W_SCREEN;

	if (GetGameMode() == GS_NORMAL)
	{
		if (!addPower())
		{
			debug(LOG_ERROR, "Couldn't create power Bar widget(Out of memory ?)");
		}
	}

	/* Note the current screen state */
	intMode = INT_NORMAL;

	objectsChanged = false;

	// reset the previous objects
	resetPreviousObj();

	/* make demolish stat always available */
	if (!bInTutorial)
	{
		for (int comp = 0; comp < numStructureStats; comp++)
		{
			if (asStructureStats[comp].type == REF_DEMOLISH)
			{
				for (int inc = 0; inc < MAX_PLAYERS; inc++)
				{
					apStructTypeLists[inc][comp] = AVAILABLE;
				}
			}
		}
	}
}

human_computer_interface::~human_computer_interface()
{
	delete psWScreen;
	psWScreen = NULL;

	apsTemplateList.clear();
	free(apsComponentList);
	free(apsExtraSysList);
	apsObjectList.clear();
	psObjSelected = nullptr;

	psWScreen = NULL;
	apsComponentList = NULL;
	apsExtraSysList = NULL;
}

namespace
{
	std::unique_ptr<human_computer_interface> default_hci;
}


void human_computer_interface::setReticuleBut(int ButId)
{
	/* Default button data */
	W_BUTINIT sButInit;
	sButInit.formID = IDRET_FORM;
	sButInit.id = ReticuleEnabled[ButId].id;
	sButInit.width = RET_BUTWIDTH;
	sButInit.height = RET_BUTHEIGHT;
	sButInit.pDisplay = intDisplayReticuleButton;
	sButInit.x = ReticuleOffsets[ButId].x + RETXOFFSET;
	sButInit.y = ReticuleOffsets[ButId].y + RETYOFFSET;
	sButInit.pTip = retbutstats[ButId].tip;
	sButInit.style = WBUT_SECONDARY;
	sButInit.UserData = ButId;
	retbutstats[ButId].downTime = 0;
	retbutstats[ButId].flashing = 0;
	retbutstats[ButId].flashTime = 0;
	if (!widgAddButton(psWScreen, &sButInit))
	{
		debug(LOG_ERROR, "Failed to add reticule button");
	}
}

//initialise all the previous obj - particularly useful for when go Off world!
void human_computer_interface::resetPreviousObj()
{
	//make sure stats screen doesn't think it should be up
	statsUp = false;
	// reset the previous objects
	memset(apsPreviousObj.data(), 0, sizeof(apsPreviousObj));
}

// Set widget refresh pending flag.
//
void human_computer_interface::refreshScreen()
{
	refreshPending = true;
}

bool human_computer_interface::isRefreshing()
{
	return refreshing;
}

// see if a delivery point is selected
static FLAG_POSITION *intFindSelectedDelivPoint(void)
{
	FLAG_POSITION *psFlagPos;

	for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos;
	     psFlagPos = psFlagPos->psNext)
	{
		if (psFlagPos->selected && (psFlagPos->type == POS_DELIVERY))
		{
			return psFlagPos;
		}
	}

	return NULL;
}

// Refresh widgets once per game cycle if pending flag is set.
//
void human_computer_interface::doScreenRefresh()
{
	UWORD           objMajor = 0, statMajor = 0;
	FLAG_POSITION	*psFlag;

	if (refreshPending)
	{
		refreshing = true;

		if ((intMode == INT_OBJECT ||
		     intMode == INT_STAT ||
		     intMode == INT_CMDORDER ||
		     intMode == INT_ORDER ||
		     intMode == INT_TRANSPORTER) &&
		    widgGetFromID(psWScreen, IDOBJ_FORM) != NULL &&
		    widgGetFromID(psWScreen, IDOBJ_FORM)->visible())
		{
			bool StatsWasUp = false;
			bool OrderWasUp = false;

			// If the stats form is up then remove it, but remember that it was up.
			if ((intMode == INT_STAT) && widgGetFromID(psWScreen, IDSTAT_FORM) != NULL)
			{
				StatsWasUp = true;
			}

			// store the current tab position
			if (widgGetFromID(psWScreen, IDOBJ_TABFORM) != NULL)
			{
				objMajor = ((ListTabWidget *)widgGetFromID(psWScreen, IDOBJ_TABFORM))->currentPage();
			}
			if (StatsWasUp)
			{
				statMajor = ((ListTabWidget *)widgGetFromID(psWScreen, IDSTAT_TABFORM))->currentPage();
			}
			// now make sure the stats screen isn't up
			if (widgGetFromID(psWScreen, IDSTAT_FORM) != NULL)
			{
				intRemoveStatsNoAnim();
			}

			// see if there was a delivery point being positioned
			psFlag = intFindSelectedDelivPoint();

			// see if the commander order screen is up
			if ((intMode == INT_CMDORDER) &&
			    (widgGetFromID(psWScreen, IDORDER_FORM) != NULL))
			{
				OrderWasUp = true;
			}

			switch (objMode)
			{
			case IOBJ_MANUFACTURE:	// The manufacture screen (factorys on bottom bar)
			case IOBJ_RESEARCH:		// The research screen
				//pass in the currently selected object
				updateObject((BASE_OBJECT *)interfaceStructList(), psObjSelected, StatsWasUp);
				break;

			case IOBJ_BUILD:
			case IOBJ_COMMAND:		// the command droid screen
			case IOBJ_BUILDSEL:		// Selecting a position for a new structure
			case IOBJ_DEMOLISHSEL:	// Selecting a structure to demolish
				//pass in the currently selected object
				updateObject((BASE_OBJECT *)apsDroidLists[selectedPlayer], psObjSelected, StatsWasUp);
				break;

			default:
				// generic refresh (trouble at the moment, cant just always pass in a null to addobject
				// if object screen is up, refresh it if stats screen is up, refresh that.
				break;
			}

			// set the tabs again
			if (widgGetFromID(psWScreen, IDOBJ_TABFORM) != NULL)
			{
				((ListTabWidget *)widgGetFromID(psWScreen, IDOBJ_TABFORM))->setCurrentPage(objMajor);
			}

			if (widgGetFromID(psWScreen, IDSTAT_TABFORM) != NULL)
			{
				((ListTabWidget *)widgGetFromID(psWScreen, IDSTAT_TABFORM))->setCurrentPage(statMajor);
			}

			if (psFlag != NULL)
			{
				// need to restart the delivery point position
				startDeliveryPosition(psFlag);
			}

			// make sure the commander order screen is in the right state
			if ((intMode == INT_CMDORDER) &&
			    !OrderWasUp &&
			    (widgGetFromID(psWScreen, IDORDER_FORM) != NULL))
			{
				intRemoveOrderNoAnim();
				if (statID)
				{
					widgSetButtonState(psWScreen, statID, 0);
				}
			}
		}

		// Refresh the transporter interface.
		intRefreshTransporter();

		// Refresh the order interface.
		intRefreshOrder();

		refreshing = false;
	}

	refreshPending = false;
}


//hides the power bar from the display
void human_computer_interface::hidePowerBar()
{
	//only hides the power bar if the player has requested no power bar
	if (!powerBarUp)
	{
		if (widgGetFromID(psWScreen, IDPOW_POWERBAR_T))
		{
			widgHide(psWScreen, IDPOW_POWERBAR_T);
		}
	}
}

/* Remove the options widgets from the widget screen */
static void intRemoveOptions(void)
{
	widgDelete(psWScreen, IDOPT_FORM);
}

/* Reset the widget screen to just the reticule */
void human_computer_interface::resetScreen(bool NoAnim)
{
	// Ensure driver mode is turned off.
	StopDriverMode();

	if (getWidgetsStatus() == false)
	{
		NoAnim = true;
	}

	if (ReticuleUp)
	{
		/* Reset the reticule buttons */
		widgSetButtonState(psWScreen, IDRET_COMMAND, 0);
		widgSetButtonState(psWScreen, IDRET_BUILD, 0);
		widgSetButtonState(psWScreen, IDRET_MANUFACTURE, 0);
		widgSetButtonState(psWScreen, IDRET_INTEL_MAP, 0);
		widgSetButtonState(psWScreen, IDRET_RESEARCH, 0);
		widgSetButtonState(psWScreen, IDRET_DESIGN, 0);
	}

	/* Remove whatever extra screen was displayed */
	switch (intMode)
	{
	case INT_OPTION:
		intRemoveOptions();
		break;
	case INT_EDITSTAT:
		intStopStructPosition();
		if (NoAnim)
		{
			intRemoveStatsNoAnim();
		}
		else
		{
			intRemoveStats();
		}
		break;
	case INT_OBJECT:
		intStopStructPosition();
		if (NoAnim)
		{
			intRemoveObjectNoAnim();
		}
		else
		{
			intRemoveObject();
		}
		break;
	case INT_STAT:
		if (NoAnim)
		{
			intRemoveStatsNoAnim();
			intRemoveObjectNoAnim();
		}
		else
		{
			intRemoveStats();
			intRemoveObject();
		}
		break;

	case INT_CMDORDER:
		if (NoAnim)
		{
			intRemoveOrderNoAnim();
			intRemoveObjectNoAnim();
		}
		else
		{
			intRemoveOrder();
			intRemoveObject();
		}
		break;
	case INT_ORDER:
		if (NoAnim)
		{
			intRemoveOrderNoAnim();
		}
		else
		{
			intRemoveOrder();
		}
		break;
	case INT_INGAMEOP:
		if (NoAnim)
		{
			intCloseInGameOptionsNoAnim(true);
		}
		else
		{
			intCloseInGameOptions(false, true);
		}
		break;
	case INT_MISSIONRES:
		intRemoveMissionResultNoAnim();
		break;
	case INT_MULTIMENU:
		if (NoAnim)
		{
			intCloseMultiMenuNoAnim();
		}
		else
		{
			intCloseMultiMenu();
		}
		break;
	case INT_DESIGN:
		intRemoveDesign();
		intHidePowerBar();
		if (bInTutorial)
		{
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_QUIT);
		}
		break;
	case INT_INTELMAP:
		if (NoAnim)
		{
			intRemoveIntelMapNoAnim();
		}
		else
		{
			intRemoveIntelMap();
		}
		intHidePowerBar();
		if (!bMultiPlayer)
		{
			gameTimeStart();
		}
		break;
	case INT_TRANSPORTER:
		if (NoAnim)
		{
			intRemoveTransNoAnim();
		}
		else
		{
			intRemoveTrans();
		}
		break;
	default:
		break;
	}
	secondaryWindowUp = false;
	intMode = INT_NORMAL;
	//clearSel() sets IntRefreshPending = true by calling intRefreshScreen() but if we're doing this then we won't need to refresh - hopefully!
	refreshPending = false;
}

// calulate the center world coords for a structure stat given
// top left tile coords
static void intCalcStructCenter(STRUCTURE_STATS *psStats, UDWORD tilex, UDWORD tiley, uint16_t direction, UDWORD *pcx, UDWORD *pcy)
{
	unsigned width  = getStructureStatsWidth(psStats, direction) * TILE_UNITS;
	unsigned height = getStructureStatsBreadth(psStats, direction) * TILE_UNITS;

	*pcx = tilex * TILE_UNITS + width / 2;
	*pcy = tiley * TILE_UNITS + height / 2;
}

void human_computer_interface::processOptions(uint32_t id)
{
	if (id >= IDOPT_PLAYERSTART && id <= IDOPT_PLAYEREND)
	{
		int oldSelectedPlayer = selectedPlayer;

		widgSetButtonState(psWScreen, IDOPT_PLAYERSTART + selectedPlayer, 0);
		selectedPlayer = id - IDOPT_PLAYERSTART;
		ASSERT_OR_RETURN(, selectedPlayer < MAX_PLAYERS, "Invalid player number");
		NetPlay.players[selectedPlayer].allocated = !NetPlay.players[selectedPlayer].allocated;
		NetPlay.players[oldSelectedPlayer].allocated = !NetPlay.players[oldSelectedPlayer].allocated;
		// Do not change realSelectedPlayer here, so game doesn't pause.
		widgSetButtonState(psWScreen, IDOPT_PLAYERSTART + selectedPlayer, WBUT_LOCK);
	}
	else
	{
		switch (id)
		{
		/* The add object buttons */
		case IDOPT_DROID:
			intRemoveOptions();
			apsTemplateList.clear();
			for (std::list<DROID_TEMPLATE>::iterator i = localTemplates.begin(); i != localTemplates.end(); ++i)
			{
				apsTemplateList.push_back(&*i);
			}
			ppsStatsList = (BASE_STATS **)&apsTemplateList[0]; // FIXME Ugly cast, and is undefined behaviour (strict-aliasing violation) in C/C++.
			objMode = IOBJ_MANUFACTURE;
			addStats(ppsStatsList, apsTemplateList.size(), NULL, NULL);
			intMode = INT_EDITSTAT;
			editPosMode = IED_NOPOS;
			break;
		case IDOPT_STRUCT:
			intRemoveOptions();
			for (unsigned i = 0; i < std::min<unsigned>(numStructureStats, MAXSTRUCTURES); ++i)
			{
				apsStructStatsList[i] = asStructureStats + i;
			}
			ppsStatsList = (BASE_STATS **)apsStructStatsList.data();
			objMode = IOBJ_BUILD;
			addStats(ppsStatsList, std::min<unsigned>(numStructureStats, MAXSTRUCTURES), NULL, NULL);
			intMode = INT_EDITSTAT;
			editPosMode = IED_NOPOS;
			break;
		case IDOPT_FEATURE:
			intRemoveOptions();
			for (unsigned i = 0; i < std::min<unsigned>(numFeatureStats, MAXFEATURES); ++i)
			{
				apsFeatureList[i] = asFeatureStats + i;
			}
			ppsStatsList = (BASE_STATS **)apsFeatureList.data();
			addStats(ppsStatsList, std::min<unsigned>(numFeatureStats, MAXFEATURES), NULL, NULL);
			intMode = INT_EDITSTAT;
			editPosMode = IED_NOPOS;
			break;
		/* Close window buttons */
		case IDOPT_CLOSE:
			intRemoveOptions();
			intMode = INT_NORMAL;
			break;
		/* Ignore these */
		case IDOPT_FORM:
		case IDOPT_LABEL:
		case IDOPT_PLAYERFORM:
		case IDOPT_PLAYERLABEL:
		case IDOPT_IVISFORM:
		case IDOPT_IVISLABEL:
			break;
		default:
			ASSERT(false, "Unknown return code");
			break;
		}
	}
}


/* Process return codes from the object placement stats screen */
static void intProcessEditStats(UDWORD id)
{
	if (id >= IDSTAT_START && id <= IDSTAT_END)
	{
		/* Clicked on a stat button - need to look for a location for it */
		psPositionStats = ppsStatsList[id - IDSTAT_START];
		if (psPositionStats->ref >= REF_TEMPLATE_START &&
		    psPositionStats->ref < REF_TEMPLATE_START + REF_RANGE)
		{
			FLAG_POSITION debugMenuDroidDeliveryPoint;
			// Placing a droid from the debug menu, set up the flag. (This would probably be safe to do, even if we're placing something else.)
			debugMenuDroidDeliveryPoint.factoryType = REPAIR_FLAG;
			debugMenuDroidDeliveryPoint.factoryInc = 0;
			debugMenuDroidDeliveryPoint.player = selectedPlayer;
			debugMenuDroidDeliveryPoint.selected = false;
			debugMenuDroidDeliveryPoint.psNext = NULL;
			startDeliveryPosition(&debugMenuDroidDeliveryPoint);
		}
		else
		{
			intStartStructPosition(psPositionStats);
		}
		editPosMode = IED_POS;
	}
	else if (id == IDSTAT_CLOSE)
	{
		intRemoveStats();
		intStopStructPosition();
		intMode = INT_NORMAL;
		objMode = IOBJ_NONE;
	}
}

void human_computer_interface::update()
{
	// Update the object list if necessary, prune dead objects.
	if (intMode == INT_OBJECT || intMode == INT_STAT || intMode == INT_CMDORDER)
	{
		// see if there is a dead object in the list
		for (unsigned i = 0; i < apsObjectList.size(); ++i)
		{
			if (apsObjectList[i] && apsObjectList[i]->died)
			{
				objectDied(i + IDOBJ_OBJSTART);
				apsObjectList[i] = nullptr;
			}
		}
	}

	// Update the previous object array, prune dead objects.
	for (int i = 0; i < IOBJ_MAX; ++i)
	{
		if (apsPreviousObj[i] && apsPreviousObj[i]->died)
		{
			apsPreviousObj[i] = NULL;
		}
	}

	if (psObjSelected && psObjSelected->died)
	{
		// refresh when unit dies
		psObjSelected = nullptr;
	}
}

/* Run the widgets for the in game interface */
INT_RETVAL human_computer_interface::display()
{
	bool			quitting = false;
	UDWORD			structX, structY, structX2, structY2;

	doScreenRefresh();

	/* if objects in the world have changed, may have to update the interface */
	if (objectsChanged)
	{
		/* The objects on the object screen have changed */
		if (intMode == INT_OBJECT)
		{
			ASSERT_OR_RETURN(INT_NONE, widgGetFromID(psWScreen, IDOBJ_TABFORM) != NULL, "No object form");

			/* Remove the old screen */
			int objMajor = ((ListTabWidget *)widgGetFromID(psWScreen, IDOBJ_TABFORM))->currentPage();
			intRemoveObject();

			/* Add the new screen */
			switch (objMode)
			{
			case IOBJ_BUILD:
			case IOBJ_BUILDSEL:
				addBuild(NULL);
				break;
			case IOBJ_MANUFACTURE:
				addManufacture(NULL);
				break;
			case IOBJ_RESEARCH:
				addResearch(NULL);
				break;
			default:
				break;
			}

			/* Reset the tabs on the object screen */
			((ListTabWidget *)widgGetFromID(psWScreen, IDOBJ_TABFORM))->setCurrentPage(objMajor);
		}
		else if (intMode == INT_STAT)
		{
			/* Need to get the stats screen to update as well */
		}
	}
	objectsChanged = false;

	if (bLoadSaveUp && runLoadSave(true) && strlen(sRequestResult) > 0)
	{
		if (bRequestLoad)
		{
			NET_InitPlayers();	// reinitialize starting positions
			loopMissionState = LMS_LOADGAME;
			sstrcpy(saveGameName, sRequestResult);
		}
		else
		{
			if (saveGame(sRequestResult, GTYPE_SAVE_START))
			{
				char msg[256] = {'\0'};

				sstrcpy(msg, _("GAME SAVED: "));
				sstrcat(msg, saveGameName);
				addConsoleMessage(msg, LEFT_JUSTIFY, NOTIFY_MESSAGE);

				if (widgGetFromID(psWScreen, IDMISSIONRES_SAVE))
				{
					widgDelete(psWScreen, IDMISSIONRES_SAVE);
				}
			}
			else
			{
				ASSERT(false, "intRunWidgets: saveGame Failed");
				deleteSaveGame(sRequestResult);
			}
		}
	}

	if (MissionResUp)
	{
		intRunMissionResult();
	}

	/* Run the current set of widgets */
	std::vector<unsigned> retIDs;
	if (!bLoadSaveUp)
	{
		WidgetTriggers const &triggers = widgRunScreen(psWScreen);
		for (WidgetTriggers::const_iterator trigger = triggers.begin(); trigger != triggers.end(); ++trigger)
		{
			retIDs.push_back(trigger->widget->id);
		}
	}

	/* We may need to trigger widgets with a key press */
	if (keyButtonMapping)
	{
		/* Set the appropriate id */
		retIDs.push_back(keyButtonMapping);

		/* Clear it so it doesn't trigger next time around */
		keyButtonMapping = 0;
	}

	intLastWidget = retIDs.empty() ? 0 : retIDs.back();
	if (bInTutorial && !retIDs.empty())
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_BUTTON_PRESSED);
	}

	/* Extra code for the power bars to deal with the shadow */
	if (powerBarUp)
	{
		runPower();
	}

	if (statsUp)
	{
		intRunStats();
	}

	if (OrderUp)
	{
		intRunOrder();
	}

	if (MultiMenuUp)
	{
		intRunMultiMenu();
	}

	/* Extra code for the design screen to deal with the shadow bar graphs */
	if (intMode == INT_DESIGN)
	{
		secondaryWindowUp = true;
		intRunDesign();
	}

	// Deal with any clicks.
	for (std::vector<unsigned>::const_iterator rit = retIDs.begin(); rit != retIDs.end(); ++rit)
	{
		unsigned retID = *rit;

		if (retID >= IDPROX_START && retID <= IDPROX_END)
		{
			processProximityButtons(retID);
			return INT_NONE;
		}

		switch (retID)
		{
		/*****************  Reticule buttons  *****************/

		case IDRET_OPTIONS:
			intResetScreen(false);
			(void)intAddOptions();
			intMode = INT_OPTION;
			break;

		case IDRET_COMMAND:
			intResetScreen(false);
			widgSetButtonState(psWScreen, IDRET_COMMAND, WBUT_CLICKLOCK);
			addCommand(NULL);
			break;

		case IDRET_BUILD:
			intResetScreen(true);
			widgSetButtonState(psWScreen, IDRET_BUILD, WBUT_CLICKLOCK);
			addBuild(NULL);
			break;

		case IDRET_MANUFACTURE:
			intResetScreen(true);
			widgSetButtonState(psWScreen, IDRET_MANUFACTURE, WBUT_CLICKLOCK);
			addManufacture(NULL);
			break;

		case IDRET_RESEARCH:
			intResetScreen(true);
			widgSetButtonState(psWScreen, IDRET_RESEARCH, WBUT_CLICKLOCK);
			(void)addResearch(NULL);
			break;

		case IDRET_INTEL_MAP:
			// check if RMB was clicked
			if (widgGetButtonKey_DEPRECATED(psWScreen) == WKEY_SECONDARY)
			{
				//set the current message to be the last non-proximity message added
				setCurrentMsg();
				setMessageImmediate(true);
			}
			else
			{
				psCurrentMsg = NULL;
			}
			addIntelScreen();
			break;

		case IDRET_DESIGN:
			intResetScreen(true);
			widgSetButtonState(psWScreen, IDRET_DESIGN, WBUT_CLICKLOCK);
			/*add the power bar - for looks! */
			intShowPowerBar();
			intAddDesign(false);
			intMode = INT_DESIGN;
			break;

		case IDRET_CANCEL:
			intResetScreen(false);
			psCurrentMsg = NULL;
			break;

		/*Transporter button pressed - OFFWORLD Mission Maps ONLY *********/
		case IDTRANTIMER_BUTTON:
			addTransporterInterface(NULL, true);
			break;

		case IDTRANS_LAUNCH:
			processLaunchTransporter();
			break;

		/* Catch the quit button here */
		case INTINGAMEOP_POPUP_QUIT:
		case IDMISSIONRES_QUIT:			// mission quit
		case INTINGAMEOP_QUIT_CONFIRM:			// esc quit confrim
		case IDOPT_QUIT:						// options screen quit
			intResetScreen(false);
			quitting = true;
			break;

		// process our chatbox
		case CHAT_EDITBOX:
			{
				const char *msg2 = widgGetString(psWScreen, CHAT_EDITBOX);
				int mode = (int) widgGetUserData2(psWScreen, CHAT_EDITBOX);
				if (strlen(msg2))
				{
					sstrcpy(ConsoleMsg, msg2);
					ConsolePlayer = selectedPlayer;
					eventFireCallbackTrigger((TRIGGER_TYPE)CALL_CONSOLE);
					attemptCheatCode(msg2);		// parse the message
					if (mode == CHAT_TEAM)
					{
						sendTeamMessage(msg2);
					}
					else
					{
						sendTextMessage(msg2, false);
					}
				}
				inputLoseFocus();
				bAllowOtherKeyPresses = true;
				widgDelete(psWScreen, CHAT_CONSOLEBOX);
				ChatDialogUp = false;
				break;
			}

		/* Default case passes remaining IDs to appropriate function */
		default:
			switch (intMode)
			{
			case INT_OPTION:
				processOptions(retID);
				break;
			case INT_EDITSTAT:
				intProcessEditStats(retID);
				break;
			case INT_STAT:
			case INT_CMDORDER:
			/* In stat mode ids get passed to processObject
			* and then through to processStats
			*/
			// NO BREAK HERE! THIS IS CORRECT;
			case INT_OBJECT:
				processObject(retID);
				break;
			case INT_ORDER:
				intProcessOrder(retID);
				break;
			case INT_MISSIONRES:
				intProcessMissionResult(retID);
				break;
			case INT_INGAMEOP:
				intProcessInGameOptions(retID);
				break;
			case INT_MULTIMENU:
				intProcessMultiMenu(retID);
				break;
			case INT_DESIGN:
				intProcessDesign(retID);
				break;
			case INT_INTELMAP:
				intProcessIntelMap(retID);
				break;
			case INT_TRANSPORTER:
				intProcessTransporter(retID);
				break;
			case INT_NORMAL:
				break;
			default:
				ASSERT(false, "intRunWidgets: unknown interface mode");
				break;
			}
			break;
		}
	}

	if (!quitting && retIDs.empty())
	{
		if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_BUILDSEL)
		{
			// See if a position for the structure has been found
			if (found3DBuildLocTwo(&structX, &structY, &structX2, &structY2))
			{
				// check if it's a straight line.
				if ((structX == structX2) || (structY == structY2))
				{
					// Send the droid off to build the structure assuming the droid
					// can get to the location chosen
					structX = world_coord(structX) + TILE_UNITS / 2;
					structY = world_coord(structY) + TILE_UNITS / 2;
					structX2 = world_coord(structX2) + TILE_UNITS / 2;
					structY2 = world_coord(structY2) + TILE_UNITS / 2;

					// Set the droid order
					if (intNumSelectedDroids(DROID_CONSTRUCT) == 0
					    && intNumSelectedDroids(DROID_CYBORG_CONSTRUCT) == 0
					    && psObjSelected != NULL && isConstructionDroid(psObjSelected))
					{
						orderDroidStatsTwoLocDir((DROID *)psObjSelected, DORDER_LINEBUILD, (STRUCTURE_STATS *)psPositionStats, structX, structY, structX2, structY2, player.r.y, ModeQueue);
					}
					else
					{
						orderSelectedStatsTwoLocDir(selectedPlayer, DORDER_LINEBUILD, (STRUCTURE_STATS *)psPositionStats, structX, structY, structX2, structY2, player.r.y, ctrlShiftDown());
					}
				}
				if (!quickQueueMode)
				{
					// Clear the object screen, only if we aren't immediately building something else
					intResetScreen(false);
				}

			}
			else if (found3DBuilding(&structX, &structY))	//found building
			{
				//check droid hasn't died
				if ((psObjSelected == NULL) ||
				    !psObjSelected->died)
				{
					bool CanBuild = true;

					// Send the droid off to build the structure assuming the droid
					// can get to the location chosen
					intCalcStructCenter((STRUCTURE_STATS *)psPositionStats, structX, structY, player.r.y, &structX, &structY);

					// Don't allow derrick to be built on burning ground.
					if (((STRUCTURE_STATS *)psPositionStats)->type == REF_RESOURCE_EXTRACTOR)
					{
						if (fireOnLocation(structX, structY))
						{
							AddDerrickBurningMessage();
						}
					}
					if (CanBuild)
					{
						// Set the droid order
						if (intNumSelectedDroids(DROID_CONSTRUCT) == 0
						    && intNumSelectedDroids(DROID_CYBORG_CONSTRUCT) == 0
						    && psObjSelected != NULL)
						{
							orderDroidStatsLocDir((DROID *)psObjSelected, DORDER_BUILD, (STRUCTURE_STATS *)psPositionStats, structX, structY, player.r.y, ModeQueue);
						}
						else
						{
							orderSelectedStatsLocDir(selectedPlayer, DORDER_BUILD, (STRUCTURE_STATS *)psPositionStats, structX, structY, player.r.y, ctrlShiftDown());
						}
					}
				}

				if (!quickQueueMode)
				{
					// Clear the object screen, only if we aren't immediately building something else
					intResetScreen(false);
				}
			}
			if (buildState == BUILD3D_NONE)
			{
				intResetScreen(false);
			}
		}
		else if (intMode == INT_EDITSTAT && editPosMode == IED_POS)
		{
			/* Directly positioning some type of object */
			unsigned structX1 = INT32_MAX;
			unsigned structY1 = INT32_MAX;
			FLAG_POSITION flag;
			structX2 = INT32_MAX - 1;
			structY2 = INT32_MAX - 1;
			if (sBuildDetails.psStats && (found3DBuilding(&structX1, &structY1) || found3DBuildLocTwo(&structX1, &structY1, &structX2, &structY2)))
			{
				if (structX2 == INT32_MAX - 1)
				{
					structX2 = structX1;
					structY2 = structY1;
				}
				if (structX1 > structX2)
				{
					std::swap(structX1, structX2);
				}
				if (structY1 > structY2)
				{
					std::swap(structY1, structY2);
				}
			}
			else if (deliveryReposFinished(&flag))
			{
				structX2 = structX1 = map_coord(flag.coords.x);
				structY2 = structY1 = map_coord(flag.coords.y);
			}

			for (unsigned j = structY1; j <= structY2; ++j)
				for (unsigned i = structX1; i <= structX2; ++i)
				{
					structX = i;
					structY = j;
					/* See what type of thing is being put down */
					if (psPositionStats->ref >= REF_STRUCTURE_START
					    && psPositionStats->ref < REF_STRUCTURE_START + REF_RANGE)
					{
						STRUCTURE_STATS *psBuilding = (STRUCTURE_STATS *)psPositionStats;
						STRUCTURE tmp(0, selectedPlayer);

						intCalcStructCenter(psBuilding, structX, structY, player.r.y, &structX, &structY);
						if (psBuilding->type == REF_DEMOLISH)
						{
							MAPTILE *psTile = mapTile(map_coord(structX), map_coord(structY));
							FEATURE *psFeature = (FEATURE *)psTile->psObject;
							STRUCTURE *psStructure = (STRUCTURE *)psTile->psObject;

							if (psStructure && psTile->psObject->type == OBJ_STRUCTURE)
							{
								//removeStruct(psStructure, true);
								SendDestroyStructure(psStructure);
							}
							else if (psFeature && psTile->psObject->type == OBJ_FEATURE)
							{
								removeFeature(psFeature);
							}
						}
						else
						{
							STRUCTURE *psStructure = &tmp;
							tmp.id = generateNewObjectId();
							tmp.pStructureType = (STRUCTURE_STATS *)psPositionStats;
							tmp.pos.x = structX;
							tmp.pos.y = structY;
							tmp.pos.z = map_Height(structX, structY) + world_coord(1) / 10;
							const char *msg;

							// In multiplayer games be sure to send a message to the
							// other players, telling them a new structure has been
							// placed.
							SendBuildFinished(psStructure);
							// Send a text message to all players, notifying them of
							// the fact that we're cheating ourselves a new
							// structure.
							sasprintf((char **)&msg, _("Player %u is cheating (debug menu) him/herself a new structure: %s."),
							          selectedPlayer, getName(psStructure->pStructureType));
							sendTextMessage(msg, true);
							Cheated = true;
						}
					}
					else if (psPositionStats->ref >= REF_FEATURE_START && psPositionStats->ref < REF_FEATURE_START + REF_RANGE)
					{
						const char *msg;

						// Send a text message to all players, notifying them of the fact that we're cheating ourselves a new feature.
						sasprintf((char **)&msg, _("Player %u is cheating (debug menu) him/herself a new feature: %s."),
						          selectedPlayer, getName(psPositionStats));
						sendTextMessage(msg, true);
						Cheated = true;
						// Notify the other hosts that we've just built ourselves a feature
						//sendMultiPlayerFeature(result->psStats->subType, result->pos.x, result->pos.y, result->id);
						sendMultiPlayerFeature(((FEATURE_STATS *)psPositionStats)->ref, world_coord(structX), world_coord(structY), generateNewObjectId());
					}
					else if (psPositionStats->ref >= REF_TEMPLATE_START &&
					         psPositionStats->ref < REF_TEMPLATE_START + REF_RANGE)
					{
						const char *msg;
						DROID *psDroid = buildDroid((DROID_TEMPLATE *)psPositionStats,
						                     world_coord(structX) + TILE_UNITS / 2, world_coord(structY) + TILE_UNITS / 2,
						                     selectedPlayer, false, NULL);
						cancelDeliveryRepos();
						if (psDroid)
						{
							addDroid(psDroid, apsDroidLists);

							// Send a text message to all players, notifying them of
							// the fact that we're cheating ourselves a new droid.
							sasprintf((char **)&msg, _("Player %u is cheating (debug menu) him/herself a new droid: %s."), selectedPlayer, psDroid->aName);

							psScrCBNewDroid = psDroid;
							psScrCBNewDroidFact = NULL;
							eventFireCallbackTrigger((TRIGGER_TYPE)CALL_NEWDROID);	// notify scripts so it will get assigned jobs
							psScrCBNewDroid = NULL;

							triggerEventDroidBuilt(psDroid, NULL);
						}
						else
						{
							// Send a text message to all players, notifying them of
							// the fact that we're cheating ourselves a new droid.
							sasprintf((char **)&msg, _("Player %u is cheating (debug menu) him/herself a new droid."), selectedPlayer);
						}
						sendTextMessage(msg, true);
						Cheated = true;
					}
					if (!quickQueueMode)
					{
						editPosMode = IED_NOPOS;
					}
				}
		}
	}

	unsigned widgOverID = widgGetMouseOver(psWScreen);

	INT_RETVAL retCode = INT_NONE;
	if (quitting)
	{
		retCode = INT_QUIT;
	}
	else if (!retIDs.empty() || intMode == INT_EDIT || intMode == INT_MISSIONRES || widgOverID != 0)
	{
		retCode = INT_INTERCEPT;
	}

	if ((testPlayerHasLost() || testPlayerHasWon()) && !bMultiPlayer && intMode != INT_MISSIONRES && !getDebugMappingStatus())
	{
		debug(LOG_ERROR, "PlayerHasLost Or Won");
		intResetScreen(true);
		retCode = INT_QUIT;
	}
	return retCode;
}

void human_computer_interface::runPower()
{
	UDWORD				statID;
	BASE_STATS			*psStat;
	UDWORD				quantity = 0;
	RESEARCH			*psResearch;

	/* Find out which button was hilited */
	statID = widgGetMouseOver(psWScreen);
	if (statID >= IDSTAT_START && statID <= IDSTAT_END)
	{
		psStat = ppsStatsList[statID - IDSTAT_START];
		if (psStat->ref >= REF_STRUCTURE_START && psStat->ref <
		    REF_STRUCTURE_START + REF_RANGE)
		{
			//get the structure build points
			quantity = ((STRUCTURE_STATS *)apsStructStatsList[statID -
			            IDSTAT_START])->powerToBuild;
		}
		else if (psStat->ref >= REF_TEMPLATE_START &&
		         psStat->ref < REF_TEMPLATE_START + REF_RANGE)
		{
			//get the template build points
			quantity = calcTemplatePower(apsTemplateList[statID - IDSTAT_START]);
		}
		else if (psStat->ref >= REF_RESEARCH_START &&
		         psStat->ref < REF_RESEARCH_START + REF_RANGE)
		{
			//get the research points
			psResearch = (RESEARCH *)ppResearchList[statID - IDSTAT_START];

			// has research been not been canceled
			int rindex = psResearch->index;
			if (IsResearchCancelled(&asPlayerResList[selectedPlayer][rindex]) == 0)
			{
				quantity = ((RESEARCH *)ppResearchList[statID - IDSTAT_START])->researchPower;
			}
		}

		//update the power bars
		intSetShadowPower(quantity);
	}
	else
	{
		intSetShadowPower(0);
	}
}


// Process stats screen.
static void intRunStats(void)
{
	BASE_OBJECT			*psOwner;
	STRUCTURE			*psStruct;
	FACTORY				*psFactory;

	if (intMode != INT_EDITSTAT && objMode == IOBJ_MANUFACTURE)
	{
		psOwner = (BASE_OBJECT *)widgGetUserData(psWScreen, IDSTAT_LOOP_LABEL);
		ASSERT_OR_RETURN(, psOwner->type == OBJ_STRUCTURE, "Invalid object type");

		psStruct = (STRUCTURE *)psOwner;
		ASSERT_OR_RETURN(, StructIsFactory(psStruct), "Invalid Structure type");

		psFactory = (FACTORY *)psStruct->pFunctionality;
		//adjust the loop button if necessary
		if (psFactory->psSubject != NULL && psFactory->productionLoops != 0)
		{
			widgSetButtonState(psWScreen, IDSTAT_LOOP_BUTTON, WBUT_CLICKLOCK);
		}
	}
}

void human_computer_interface::addObjectStats(BASE_OBJECT *psObj, uint32_t id)
{
	BASE_STATS		*psStats;
	UWORD               statMajor = 0;
	UDWORD			i, j, index;
	UDWORD			count;
	SDWORD			iconNumber, entryIN;

	/* Clear a previous structure pos if there is one */
	intStopStructPosition();

	/* Get the current tab pos */
	int objMajor = ((ListTabWidget *)widgGetFromID(psWScreen, IDOBJ_TABFORM))->currentPage();

	// Store the tab positions.
	if (intMode == INT_STAT)
	{
		if (widgGetFromID(psWScreen, IDSTAT_FORM) != NULL)
		{
			statMajor = ((ListTabWidget *)widgGetFromID(psWScreen, IDSTAT_TABFORM))->currentPage();
		}
		intRemoveStatsNoAnim();
	}

	/* Display the stats window
	 *  - restore the tab position if there is no stats selected
	 */
	psStats = objGetStatsFunc(psObj);

	// note the object for the screen
	apsPreviousObj[objMode] = psObj;

	// NOTE! The below functions populate our list (building/units...)
	// up to MAX____.  We have unlimited potential, but it is capped at 200 now.
	//determine the Structures that can be built
	if (objMode == IOBJ_BUILD)
	{
		numStatsListEntries = fillStructureList(apsStructStatsList.data(),
		                                        selectedPlayer, MAXSTRUCTURES - 1);

		ppsStatsList = (BASE_STATS **)apsStructStatsList.data();
	}

	//have to determine the Template list once the factory has been chosen
	if (objMode == IOBJ_MANUFACTURE)
	{
		fillTemplateList(apsTemplateList, (STRUCTURE *)psObj);
		numStatsListEntries = apsTemplateList.size();
		ppsStatsList = (BASE_STATS **)&apsTemplateList[0];  // FIXME Ugly cast, and is undefined behaviour (strict-aliasing violation) in C/C++.
	}

	/*have to calculate the list each time the Topic button is pressed
	  so that only one topic can be researched at a time*/
	if (objMode == IOBJ_RESEARCH)
	{
		//set to value that won't be reached in fillResearchList
		index = asResearch.size() + 1;
		if (psStats)
		{
			index = ((RESEARCH *)psStats)->index;
		}
		//recalculate the list
		numStatsListEntries = fillResearchList(pList.data(), selectedPlayer, (UWORD)index, MAXRESEARCH);

		//	-- Alex's reordering of the list
		// NOTE!  Do we even want this anymore, since we can have basically
		// unlimted tabs? Future enhancement assign T1/2/3 button on form
		// so we can pick what level of tech we want to build instead of
		// Alex picking for us?
		count = 0;
		for (i = 0; i < RID_MAXRID; i++)
		{
			iconNumber = mapRIDToIcon(i);
			for (j = 0; j < numStatsListEntries; j++)
			{
				entryIN = asResearch[pList[j]].iconID;
				if (entryIN == iconNumber)
				{
					pSList[count++] = pList[j];
				}

			}
		}
		// Tag on the ones at the end that have no BASTARD icon IDs - why is this?!!?!?!?
		// NOTE! more pruning [future ref]
		for (j = 0; j < numStatsListEntries; j++)
		{
			iconNumber = mapIconToRID(asResearch[pList[j]].iconID);
			if (iconNumber < 0)
			{
				pSList[count++] = pList[j];
			}
		}

		//fill up the list with topics
		for (i = 0; i < numStatsListEntries; i++)
		{
			ppResearchList[i] = &asResearch[pSList[i]];	  // note change from pList
		}
	}

	addStats(ppsStatsList, numStatsListEntries, psStats, psObj);

	// get the tab positions for the new stat form
	// Restore the tab positions.
	// only restore if we've still got at least that many tabs
	if (psStats == nullptr && widgGetFromID(psWScreen, IDSTAT_TABFORM) != nullptr)
	{
		((ListTabWidget *)widgGetFromID(psWScreen, IDSTAT_TABFORM))->setCurrentPage(statMajor);
	}

	intMode = INT_STAT;
	/* Note the object */
	psObjSelected = psObj;
	objStatID = id;

	/* Reset the tabs and lock the button */
	((ListTabWidget *)widgGetFromID(psWScreen, IDOBJ_TABFORM))->setCurrentPage(objMajor);
	if (id != 0)
	{
		widgSetButtonState(psWScreen, id, WBUT_CLICKLOCK);
	}
}


static void intSelectDroid(BASE_OBJECT *psObj)
{
	if (driveModeActive())
	{
		clearSel();
		((DROID *)psObj)->selected = true;
		driveSelectionChanged();
		driveDisableControl();
	}
	else
	{
		clearSelection();
		((DROID *)psObj)->selected = true;
	}
	triggerEventSelected();
}

void human_computer_interface::resetWindows(BASE_OBJECT *psObj)
{
	if (psObj)
	{
		// reset the object screen with the new object
		switch (objMode)
		{
		case IOBJ_BUILD:
		case IOBJ_BUILDSEL:
		case IOBJ_DEMOLISHSEL:
			addBuild((DROID *)psObj);
			break;
		case IOBJ_RESEARCH:
			addResearch((STRUCTURE *)psObj);
			break;
		case IOBJ_MANUFACTURE:
			addManufacture((STRUCTURE *)psObj);
			break;
		case IOBJ_COMMAND:

			addCommand((DROID *)psObj);
			break;
		default:
			break;
		}
	}
}

void human_computer_interface::processObject(uint32_t id)
{
	BASE_OBJECT		*psObj;
	STRUCTURE		*psStruct;
	SDWORD			butIndex;
	UDWORD			statButID;

	ASSERT_OR_RETURN(, widgGetFromID(psWScreen, IDOBJ_TABFORM) != NULL, "intProcessObject, missing form");

	// deal with CRTL clicks
	if (objMode == IOBJ_BUILD &&	// What..................?
	    (keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT)) &&
	    ((id >= IDOBJ_OBJSTART && id <= IDOBJ_OBJEND) ||
	     (id >= IDOBJ_STATSTART && id <= IDOBJ_STATEND)))
	{
		/* Find the object that the ID refers to */
		psObj = getObject(id);
		if (id >= IDOBJ_OBJSTART && id <= IDOBJ_OBJEND)
		{
			statButID = IDOBJ_STATSTART + id - IDOBJ_OBJSTART;
		}
		else
		{
			statButID = id;
		}
		if (psObj && psObj->selected)
		{
			psObj->selected = false;
			widgSetButtonState(psWScreen, statButID, 0);
			if (intNumSelectedDroids(DROID_CONSTRUCT) == 0 && intNumSelectedDroids(DROID_CYBORG_CONSTRUCT) == 0)
			{
				intRemoveStats();
			}
			if (psObjSelected == psObj)
			{
				psObjSelected = (BASE_OBJECT *)intCheckForDroid(DROID_CONSTRUCT);
				if (!psObjSelected)
				{
					psObjSelected = (BASE_OBJECT *)intCheckForDroid(DROID_CYBORG_CONSTRUCT);
				}
			}
		}
		else if (psObj)
		{
			if (psObjSelected)
			{
				psObjSelected->selected = true;
			}
			psObj->selected = true;
			widgSetButtonState(psWScreen, statButID, WBUT_CLICKLOCK);
			addObjectStats(psObj, statButID);
		}
		triggerEventSelected();
	}
	else if (id >= IDOBJ_OBJSTART && id <= IDOBJ_OBJEND)
	{
		/* deal with RMB clicks */
		if (widgGetButtonKey_DEPRECATED(psWScreen) == WKEY_SECONDARY)
		{
			objectRMBPressed(id);
		}
		/* deal with LMB clicks */
		else
		{
			/* An object button has been pressed */
			/* Find the object that the ID refers to */
			psObj = getObject(id);
			if (!psObj)
			{
				return;
			}
			if (psObj->type == OBJ_STRUCTURE && !offWorldKeepLists)
			{
				/* Deselect old buildings */
				for (psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
				{
					psStruct->selected = false;
				}

				/* Select new one */
				((STRUCTURE *)psObj)->selected = true;
				triggerEventSelected();
			}

			if (!driveModeActive())
			{
				// don't do this if offWorld and a structure object has been selected
				if (!(psObj->type == OBJ_STRUCTURE && offWorldKeepLists))
				{
					// set the map position - either the object position, or the position jumped from
					butIndex = id - IDOBJ_OBJSTART;
					if (butIndex >= 0 && butIndex <= IDOBJ_OBJEND - IDOBJ_OBJSTART)
					{
						asJumpPos.resize(IDOBJ_OBJEND - IDOBJ_OBJSTART, Vector2i(0, 0));
						if (((asJumpPos[butIndex].x == 0) && (asJumpPos[butIndex].y == 0)) ||
						    !DrawnInLastFrame((SDWORD)psObj->sDisplay.frameNumber) ||
						    ((psObj->sDisplay.screenX > pie_GetVideoBufferWidth()) ||
						     (psObj->sDisplay.screenY > pie_GetVideoBufferHeight())))
						{
							asJumpPos[butIndex] = getPlayerPos();
							setPlayerPos(psObj->pos.x, psObj->pos.y);
							if (getWarCamStatus())
							{
								camToggleStatus();
							}
						}
						else
						{
							setPlayerPos(asJumpPos[butIndex].x, asJumpPos[butIndex].y);
							if (getWarCamStatus())
							{
								camToggleStatus();
							}
							asJumpPos[butIndex].x = 0;
							asJumpPos[butIndex].y = 0;
						}
					}
				}
			}

			resetWindows(psObj);

			// If a construction droid button was clicked then
			// clear all other selections and select it.
			if (psObj->type == OBJ_DROID)
			{
				intSelectDroid(psObj);
				psObjSelected = psObj;

			}
		}
	}
	/* A object stat button has been pressed */
	else if (id >= IDOBJ_STATSTART &&
	         id <= IDOBJ_STATEND)
	{
		/* deal with RMB clicks */
		if (widgGetButtonKey_DEPRECATED(psWScreen) == WKEY_SECONDARY)
		{
			objStatRMBPressed(id);
		}
		else
		{
			/* Find the object that the stats ID refers to */
			psObj = getObject(id);
			ASSERT_OR_RETURN(, psObj, "Missing referred to object id %u", id);

			resetWindows(psObj);

			// If a droid button was clicked then clear all other selections and select it.
			if (psObj->type == OBJ_DROID)
			{
				// Select the droid when the stat button (in the object window) is pressed.
				intSelectDroid(psObj);
				psObjSelected = psObj;
			}
			else if (psObj->type == OBJ_STRUCTURE)
			{
				if (StructIsFactory((STRUCTURE *)psObj))
				{
					//might need to cancel the hold on production
					releaseProduction((STRUCTURE *)psObj, ModeQueue);
				}
				else if (((STRUCTURE *)psObj)->pStructureType->type == REF_RESEARCH)
				{
					//might need to cancel the hold on research facilty
					releaseResearch((STRUCTURE *)psObj, ModeQueue);
				}

				for (STRUCTURE *psCurr = apsStructLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
				{
					psCurr->selected = false;
				}
				psObj->selected = true;
				triggerEventSelected();
			}
		}
	}
	else if (id == IDOBJ_CLOSE)
	{
		intResetScreen(false);
		intMode = INT_NORMAL;
	}
	else
	{
		if (objMode != IOBJ_COMMAND && id != IDOBJ_TABFORM)
		{
			/* Not a button on the build form, must be on the stats form */
			processStats(id);
		}
		else  if (id != IDOBJ_TABFORM)
		{
			intProcessOrder(id);
		}
	}
}

void human_computer_interface::processStats(uint32_t id)
{
	BASE_STATS		*psStats;
	STRUCTURE		*psStruct;
	FLAG_POSITION	*psFlag;
	int compIndex;

	ASSERT_OR_RETURN(, widgGetFromID(psWScreen, IDOBJ_TABFORM) != NULL, "missing form");

	if (id >= IDSTAT_START &&
	    id <= IDSTAT_END)
	{
		ASSERT_OR_RETURN(, id - IDSTAT_START < numStatsListEntries, "Invalid structure stats id");

		/* deal with RMB clicks */
		if (widgGetButtonKey_DEPRECATED(psWScreen) == WKEY_SECONDARY)
		{
			statsRMBPressed(id);
		}
		/* deal with LMB clicks */
		else
		{
			//manufacture works differently!
			if (objMode == IOBJ_MANUFACTURE)
			{
				compIndex = id - IDSTAT_START;
				//get the stats
				ASSERT_OR_RETURN(, compIndex < numStatsListEntries, "Invalid range referenced for numStatsListEntries, %d > %d", compIndex, numStatsListEntries);
				psStats = ppsStatsList[compIndex];
				ASSERT_OR_RETURN(, psObjSelected != NULL, "Invalid structure pointer");
				ASSERT_OR_RETURN(, psStats != NULL, "Invalid template pointer");
				if (productionPlayer == (SBYTE)selectedPlayer)
				{
					STRUCTURE *psStructure = (STRUCTURE *)psObjSelected;
					FACTORY  *psFactory = &psStructure->pFunctionality->factory;
					DROID_TEMPLATE *psNext = (DROID_TEMPLATE *)psStats;

					//increase the production
					factoryProdAdjust(psStructure, psNext, true);

					//need to check if this was the template that was mid-production
					if (getProduction(psStructure, FactoryGetTemplate(psFactory)).numRemaining() == 0)
					{
						doNextProduction(psStructure, FactoryGetTemplate(psFactory), ModeQueue);
						psNext = FactoryGetTemplate(psFactory);
					}
					else if (!StructureIsManufacturingPending(psStructure))
					{
						structSetManufacture(psStructure, psNext, ModeQueue);
					}

					if (StructureIsOnHoldPending(psStructure))
					{
						releaseProduction(psStructure, ModeQueue);
					}

					// Reset the button on the object form
					setStats(objStatID, (BASE_STATS *)psNext);
				}
			}
			else
			{
				/* See if this was a click on an already selected stat */
				psStats = objGetStatsFunc(psObjSelected);
				// only do the cancel operation if not trying to add to the build list
				if (psStats == ppsStatsList[id - IDSTAT_START] && objMode != IOBJ_BUILD)
				{
					// this needs to be done before the topic is cancelled from the structure
					/* If Research then need to set topic to be cancelled */
					if (objMode == IOBJ_RESEARCH)
					{
						if (psObjSelected->type == OBJ_STRUCTURE)
						{
							// TODO This call seems to be redundant, since cancelResearch is called from objSetStatsFunc==setResearchStats.
							cancelResearch((STRUCTURE *)psObjSelected, ModeQueue);
						}
					}

					/* Clear the object stats */
					objSetStatsFunc(psObjSelected, NULL);

					/* Reset the button on the object form */
					setStats(objStatID, NULL);

					/* Unlock the button on the stats form */
					widgSetButtonState(psWScreen, id, 0);
				}
				else
				{
					//If Research then need to set the topic - if one, to be cancelled
					if (objMode == IOBJ_RESEARCH)
					{
						if (psObjSelected->type == OBJ_STRUCTURE && ((STRUCTURE *)
						        psObjSelected)->pStructureType->type == REF_RESEARCH)
						{
							//if there was a topic currently being researched - cancel it
							if (((RESEARCH_FACILITY *)((STRUCTURE *)psObjSelected)->
							     pFunctionality)->psSubject)
							{
								cancelResearch((STRUCTURE *)psObjSelected, ModeQueue);
							}
						}
					}

					// call the tutorial callback if necessary
					if (bInTutorial && objMode == IOBJ_BUILD)
					{

						eventFireCallbackTrigger((TRIGGER_TYPE)CALL_BUILDGRID);
					}

					// Set the object stats
					compIndex = id - IDSTAT_START;
					ASSERT_OR_RETURN(, compIndex < numStatsListEntries, "Invalid range referenced for numStatsListEntries, %d > %d", compIndex, numStatsListEntries);
					psStats = ppsStatsList[compIndex];

					// Reset the button on the object form
					//if this returns false, there's a problem so set the button to NULL
					if (!objSetStatsFunc(psObjSelected, psStats))
					{
						setStats(objStatID, NULL);
					}
					else
					{
						// Reset the button on the object form
						setStats(objStatID, psStats);
					}
				}

				// Get the tabs on the object form
				int objMajor = ((ListTabWidget *)widgGetFromID(psWScreen, IDOBJ_TABFORM))->currentPage();

				// Close the stats box
				intRemoveStats();
				intMode = INT_OBJECT;

				// Reset the tabs on the object form
				((ListTabWidget *)widgGetFromID(psWScreen, IDOBJ_TABFORM))->setCurrentPage(objMajor);

				// Close the object box as well if selecting a location to build- no longer hide/reveal
				// or if selecting a structure to demolish
				if (objMode == IOBJ_BUILDSEL || objMode == IOBJ_DEMOLISHSEL)
				{
					if (driveModeActive())
					{
						// Make sure weve got a construction droid selected.
						if (driveGetDriven()->droidType != DROID_CONSTRUCT
						    && driveGetDriven()->droidType != DROID_CYBORG_CONSTRUCT)
						{
							driveDisableControl();
						}
						driveDisableTactical();
						driveStartBuild();
						intRemoveObject();
					}

					intRemoveObject();
					// hack to stop the stats window re-opening in demolish mode
					if (objMode == IOBJ_DEMOLISHSEL)
					{
						refreshPending = false;
					}
				}
			}
		}
	}
	else if (id == IDSTAT_CLOSE)
	{
		/* Get the tabs on the object form */
		int objMajor = ((ListTabWidget *)widgGetFromID(psWScreen, IDOBJ_TABFORM))->currentPage();

		/* Close the structure box without doing anything */
		intRemoveStats();
		intMode = INT_OBJECT;

		/* Reset the tabs on the build form */
		((ListTabWidget *)widgGetFromID(psWScreen, IDOBJ_TABFORM))->setCurrentPage(objMajor);

		/* Unlock the stats button */
		widgSetButtonState(psWScreen, objStatID, 0);
	}
	else if (id == IDSTAT_SLIDER)
	{
		// Process the quantity slider.
	}
	else if (id >= IDPROX_START && id <= IDPROX_END)
	{
		// process the proximity blip buttons.
	}
	else if (id == IDSTAT_LOOP_BUTTON)
	{
		// Process the loop button.
		psStruct = (STRUCTURE *)widgGetUserData(psWScreen, IDSTAT_LOOP_LABEL);
		if (psStruct)
		{
			//LMB pressed
			if (widgGetButtonKey_DEPRECATED(psWScreen) == WKEY_PRIMARY)
			{
				factoryLoopAdjust(psStruct, true);
			}
			//RMB pressed
			else if (widgGetButtonKey_DEPRECATED(psWScreen) == WKEY_SECONDARY)
			{
				factoryLoopAdjust(psStruct, false);
			}
			if (((FACTORY *)psStruct->pFunctionality)->psSubject != NULL && ((FACTORY *)psStruct->pFunctionality)->productionLoops != 0)
			{
				//lock the button
				widgSetButtonState(psWScreen, IDSTAT_LOOP_BUTTON, WBUT_CLICKLOCK);
			}
			else
			{
				//unlock
				widgSetButtonState(psWScreen, IDSTAT_LOOP_BUTTON, 0);
			}
		}
	}
	else if (id == IDSTAT_DP_BUTTON)
	{
		// Process the DP button
		psStruct = (STRUCTURE *)widgGetUserData(psWScreen, IDSTAT_DP_BUTTON);
		if (psStruct)
		{
			// make sure that the factory isn't assigned to a commander
			assignFactoryCommandDroid(psStruct, NULL);
			psFlag = FindFactoryDelivery(psStruct);
			if (psFlag)
			{
				startDeliveryPosition(psFlag);
			}
		}
	}
	else if (id == IDSTAT_OBSOLETE_BUTTON)
	{
		includeRedundantDesigns = !includeRedundantDesigns;
		StateButton *obsoleteButton = (StateButton *)widgGetFromID(psWScreen, IDSTAT_OBSOLETE_BUTTON);
		obsoleteButton->setState(includeRedundantDesigns);
		intRefreshScreen();
	}
}


/* Set the map view point to the world coordinates x,y */
void human_computer_interface::setMapPos(uint32_t x, uint32_t y)
{
	if (!driveModeActive())
	{
		setViewPos(map_coord(x), map_coord(y), true);
	}
}


/* Sync the interface to an object */
// If psObj is NULL then reset interface displays.
//
// There should be two version of this function, one for left clicking and one got right.
//
void human_computer_interface::objectSelected(BASE_OBJECT *psObj)
{
	if (psObj)
	{
		setWidgetsStatus(true);
		switch (psObj->type)
		{
		case OBJ_DROID:
			if (!OrderUp)
			{
				intResetScreen(false);
				// changed to a BASE_OBJECT to accomodate the factories - AB 21/04/99
				intAddOrder(psObj);
				intMode = INT_ORDER;
			}
			else
			{
				// changed to a BASE_OBJECT to accomodate the factories - AB 21/04/99
				intAddOrder(psObj);
			}
			break;

		case OBJ_STRUCTURE:
			//don't do anything if structure is only partially built
			intResetScreen(false);

			if (objMode == IOBJ_DEMOLISHSEL)
			{
				/* do nothing here */
				break;
			}

			if (((STRUCTURE *)psObj)->status == SS_BUILT)
			{
				if (((STRUCTURE *)psObj)->pStructureType->type == REF_FACTORY ||
				    ((STRUCTURE *)psObj)->pStructureType->type == REF_CYBORG_FACTORY ||
				    ((STRUCTURE *)psObj)->pStructureType->type == REF_VTOL_FACTORY)
				{
					addManufacture((STRUCTURE *)psObj);
				}
				else if (((STRUCTURE *)psObj)->pStructureType->type == REF_RESEARCH)
				{
					addResearch((STRUCTURE *)psObj);
				}
			}
			break;
		default:
			break;
		}
	}
	else
	{
		intResetScreen(false);
	}
}

// add the construction interface if a constructor droid is selected
void human_computer_interface::constructorSelected(DROID *psDroid)
{
	setWidgetsStatus(true);
	addBuild(psDroid);
	widgHide(psWScreen, IDOBJ_FORM);
}

// add the construction interface if a constructor droid is selected
void human_computer_interface::commanderSelected(DROID *psDroid)
{
	setWidgetsStatus(true);
	addCommand(psDroid);
	widgHide(psWScreen, IDOBJ_FORM);
}

/* Start looking for a structure location */
static void intStartStructPosition(BASE_STATS *psStats)
{
	init3DBuilding(psStats, NULL, NULL);
}


/* Stop looking for a structure location */
static void intStopStructPosition(void)
{
	/* Check there is still a struct position running */
	if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_BUILDSEL)
	{
		// Reset the stats button
		objMode = IOBJ_BUILD;
	}
	kill3DBuilding();
}


/* Display the widgets for the in game interface */
void human_computer_interface::displayWidgets()
{
	if (ReticuleUp && !bInTutorial)
	{
		checkReticuleButtons();
	}

	/*draw the background for the design screen and the Intelligence screen*/
	if (intMode == INT_DESIGN || intMode == INT_INTELMAP)
	{
		// When will they ever learn!!!!
		if (!bMultiPlayer)
		{
			screen_RestartBackDrop();

			// We need to add the console messages to the intelmap for the tutorial so that it can display messages
			if ((intMode == INT_DESIGN) || (bInTutorial && intMode == INT_INTELMAP))
			{
				displayConsoleMessages();
			}
		}
	}

	widgDisplayScreen(psWScreen);

	if (bLoadSaveUp)
	{
		displayLoadSave();
	}
}

void intDisplayWidgets(void)
{
	default_hci->displayWidgets();
}


/* Tell the interface when an object is created - it may have to be added to a screen */
void human_computer_interface::notifyNewObject(BASE_OBJECT *psObj)
{
	if (intMode == INT_OBJECT || intMode == INT_STAT)
	{
		if ((objMode == IOBJ_BUILD || objMode == IOBJ_BUILDSEL) &&
		    psObj->type == OBJ_DROID && objSelectFunc(psObj))
		{
			objectsChanged = true;
		}
		else if ((objMode == IOBJ_RESEARCH || objMode == IOBJ_MANUFACTURE) &&
		         psObj->type == OBJ_STRUCTURE && objSelectFunc(psObj))
		{
			objectsChanged = true;
		}
	}
}

void human_computer_interface::objectDied(uint32_t objID)
{
	UDWORD				statsID, gubbinsID;

	// clear the object button
	IntObjectButton *psBut = (IntObjectButton *)widgGetFromID(psWScreen, objID);
	if (psBut != nullptr && psBut->clearData())
	{
		// and its gubbins
		gubbinsID = IDOBJ_FACTORYSTART + objID - IDOBJ_OBJSTART;
		widgSetUserData(psWScreen, gubbinsID, NULL);
		gubbinsID = IDOBJ_COUNTSTART + objID - IDOBJ_OBJSTART;
		widgSetUserData(psWScreen, gubbinsID, NULL);
		gubbinsID = IDOBJ_PROGBARSTART + objID - IDOBJ_OBJSTART;
		widgSetUserData(psWScreen, gubbinsID, NULL);

		// clear the stats button
		statsID = IDOBJ_STATSTART + objID - IDOBJ_OBJSTART;
		setStats(statsID, NULL);
		// and disable it
		widgSetButtonState(psWScreen, statsID, WBUT_DISABLE);

		// remove the stat screen if necessary
		if ((intMode == INT_STAT) && statsID == objStatID)
		{
			intRemoveStatsNoAnim();
			intMode = INT_OBJECT;
		}
	}
}


/* Tell the interface a construction droid has finished building */
void human_computer_interface::notifyBuildFinished(DROID *psDroid)
{
	ASSERT_OR_RETURN(, psDroid != NULL, "Invalid droid pointer");

	if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_BUILD)
	{
		// Find which button the droid is on and clear its stats
		unsigned droidID = 0;
		for (DROID *psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
		{
			if (objSelectFunc((BASE_OBJECT *)psCurr))
			{
				if (psCurr == psDroid)
				{
					setStats(droidID + IDOBJ_STATSTART, NULL);
					break;
				}
				droidID++;
			}
		}
	}
}

/* Tell the interface a construction droid has started building*/
void human_computer_interface::notifyBuildStarted(DROID *psDroid)
{
	ASSERT_OR_RETURN(, psDroid != NULL, "Invalid droid pointer");

	if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_BUILD)
	{
		// Find which button the droid is on and clear its stats
		unsigned droidID = 0;
		for (DROID *psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
		{
			if (objSelectFunc(psCurr))
			{
				if (psCurr == psDroid)
				{
					STRUCTURE *target = castStructure(psCurr->order.psObj);
					setStats(droidID + IDOBJ_STATSTART, target != nullptr? target->pStructureType : nullptr);
					break;
				}
				droidID++;
			}
		}
	}
}

/* Are we in build select mode*/
bool human_computer_interface::buildSelectMode()
{
	return (objMode == IOBJ_BUILDSEL);
}

/* Are we in demolish select mode*/
bool human_computer_interface::demolishSelectMode()
{
	return (objMode == IOBJ_DEMOLISHSEL);
}

//is the build interface up?
bool human_computer_interface::buildMode()
{
	return (objMode == IOBJ_BUILD);
}

//Written to allow demolish order to be added to the queuing system
void human_computer_interface::demolishCancel()
{
	if (objMode == IOBJ_DEMOLISHSEL)
	{
		objMode = IOBJ_NONE;
	}
}

void human_computer_interface::orderResearch()
{
	std::reverse(apsObjectList.begin(), apsObjectList.end());  // Why reverse this list, instead of sorting it?
}


static inline bool sortObjectByIdFunction(BASE_OBJECT *a, BASE_OBJECT *b)
{
	return (a == NULL ? 0 : a->id) < (b == NULL ? 0 : b->id);
}

// reorder the commanders
void human_computer_interface::orderDroids()
{
	// bubble sort on the ID - first built will always be first in the list
	std::sort(apsObjectList.begin(), apsObjectList.end(), sortObjectByIdFunction);  // Why sort this list, instead of reversing it?
}

static inline bool sortFactoryByTypeFunction(BASE_OBJECT *a, BASE_OBJECT *b)
{
	if (a == NULL || b == NULL)
	{
		return (a == NULL) < (b == NULL);
	}
	STRUCTURE *s = castStructure(a), *t = castStructure(b);
	ASSERT_OR_RETURN(false, s != NULL && StructIsFactory(s) && t != NULL && StructIsFactory(t), "object is not a factory");
	FACTORY *x = (FACTORY *)s->pFunctionality, *y = (FACTORY *)t->pFunctionality;
	if (x->psAssemblyPoint->factoryType != y->psAssemblyPoint->factoryType)
	{
		return x->psAssemblyPoint->factoryType < y->psAssemblyPoint->factoryType;
	}
	return x->psAssemblyPoint->factoryInc < y->psAssemblyPoint->factoryInc;
}

void human_computer_interface::orderFactories()
{
	std::sort(apsObjectList.begin(), apsObjectList.end(), sortFactoryByTypeFunction);
}

void human_computer_interface::orderObjectInterface()
{
	if (apsObjectList.empty())
	{
		//no objects so nothing to order!
		return;
	}

	switch (apsObjectList[0]->type)
	{
	case OBJ_STRUCTURE:
		if (StructIsFactory((STRUCTURE *)apsObjectList[0]))
		{
			orderFactories();
		}
		else if (((STRUCTURE *)apsObjectList[0])->pStructureType->type == REF_RESEARCH)
		{
			orderResearch();
		}
		break;
	case OBJ_DROID:
		orderDroids();
	default:
		//nothing to do as yet!
		break;
	}
}

unsigned human_computer_interface::rebuildFactoryListAndFindIndex(STRUCTURE *psBuilding)
{
	apsObjectList.clear();
	for (STRUCTURE *psCurr = interfaceStructList(); psCurr; psCurr = psCurr->psNext)
	{
		if (objSelectFunc(psCurr))
		{
			// The list is ordered now so we have to get all possible entries and sort it before checking if this is the one!
			apsObjectList.push_back(psCurr);
		}
	}
	// order the list
	orderFactories();
	// now look thru the list to see which one corresponds to the factory that has just finished
	return std::find(apsObjectList.begin(), apsObjectList.end(), psBuilding) - apsObjectList.begin();
}

/* Tell the interface a factory has completed building ALL droids */
void human_computer_interface::notifyManufactureFinished(STRUCTURE *psBuilding)
{
	ASSERT_OR_RETURN(, psBuilding != NULL, "Invalid structure pointer");

	if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_MANUFACTURE)
	{
		/* Find which button the structure is on and clear it's stats */
		unsigned structureIndex = rebuildFactoryListAndFindIndex(psBuilding);
		if (structureIndex != apsObjectList.size())
		{
			setStats(structureIndex + IDOBJ_STATSTART, NULL);
			// clear the loop button if interface is up
			if (widgGetFromID(psWScreen, IDSTAT_LOOP_BUTTON))
			{
				widgSetButtonState(psWScreen, IDSTAT_LOOP_BUTTON, 0);
			}
		}
	}
}


void human_computer_interface::updateManufacture(STRUCTURE *psBuilding)
{
	ASSERT_OR_RETURN(, psBuilding != NULL, "Invalid structure pointer");

	if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_MANUFACTURE)
	{
		/* Find which button the structure is on and update its stats */
		unsigned structureIndex = rebuildFactoryListAndFindIndex(psBuilding);
		if (structureIndex != apsObjectList.size())
		{
			setStats(structureIndex + IDOBJ_STATSTART, psBuilding->pFunctionality->factory.psSubject);
		}
	}
}

/* Tell the interface a research facility has completed a topic */
void human_computer_interface::notifyResearchFinished(STRUCTURE *psBuilding)
{
	ASSERT_OR_RETURN(, psBuilding != NULL, "Invalid structure pointer");

	// just do a screen refresh
	intRefreshScreen();
}

void intAlliedResearchChanged()
{
	if ((intMode == INT_OBJECT || intMode == INT_STAT) && objMode == IOBJ_RESEARCH)
	{
		intRefreshScreen();
	}
}

/* Add the reticule widgets to the widget screen */
bool human_computer_interface::addReticule()
{
	if (ReticuleUp)
	{
		return true; // all fine
	}
	WIDGET *parent = psWScreen->psForm;
	IntFormAnimated *retForm = new IntFormAnimated(parent, false);
	retForm->id = IDRET_FORM;
	retForm->setGeometry(RET_X, RET_Y, RET_FORMWIDTH, RET_FORMHEIGHT);
	for (int i = 0; i < NUMRETBUTS; i++)
	{
		setReticuleBut(i);
	}
	ReticuleUp = true;
	return true;
}

void human_computer_interface::removeReticule(void)
{
	if (ReticuleUp == true)
	{
		widgDelete(psWScreen, IDRET_FORM);		// remove reticule
		ReticuleUp = false;
	}
}

//toggles the Power Bar display on and off
void human_computer_interface::togglePowerBar()
{
	//toggle the flag
	powerBarUp = !powerBarUp;

	if (powerBarUp)
	{
		intShowPowerBar();
	}
	else
	{
		intHidePowerBar();
	}
}

/* Add the power bars to the screen */
bool human_computer_interface::addPower()
{
	W_BARINIT sBarInit;

	/* Add the trough bar */
	sBarInit.formID = 0;	//IDPOW_FORM;
	sBarInit.id = IDPOW_POWERBAR_T;
	//start the power bar off in view (default)
	sBarInit.style = WBAR_TROUGH;
	sBarInit.x = (SWORD)POW_X;
	sBarInit.y = (SWORD)POW_Y;
	sBarInit.width = POW_BARWIDTH;
	sBarInit.height = iV_GetImageHeight(IntImages, IMAGE_PBAR_EMPTY);
	sBarInit.sCol = WZCOL_POWER_BAR;
	sBarInit.pDisplay = intDisplayPowerBar;
	sBarInit.iRange = POWERBAR_SCALE;

	sBarInit.pTip = _("Power");

	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return false;
	}

	powerBarUp = true;
	return true;
}

/* Add the options widgets to the widget screen */
bool human_computer_interface::addOptions()
{
	W_FORMINIT	sFormInit;
	W_BUTINIT	sButInit;
	W_LABINIT	sLabInit;
	UDWORD		player;

	/* Add the option form */

	sFormInit.formID = 0;
	sFormInit.id = IDOPT_FORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = OPT_X;
	sFormInit.y = OPT_Y;
	sFormInit.width = OPT_WIDTH;
	sFormInit.height = OPT_HEIGHT;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	// set the interface mode
	intMode = INT_OPTION;

	/* Add the Option screen label */
	sLabInit.formID = IDOPT_FORM;
	sLabInit.id = IDOPT_LABEL;
	sLabInit.x = OPT_GAP;
	sLabInit.y = OPT_GAP;
	sLabInit.width = OPT_BUTWIDTH;
	sLabInit.height = OPT_BUTHEIGHT;
	sLabInit.pText = _("Options");
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return false;
	}

	/* Add the close box */
	sButInit.formID = IDOPT_FORM;
	sButInit.id = IDOPT_CLOSE;
	sButInit.x = OPT_WIDTH - OPT_GAP - CLOSE_SIZE;
	sButInit.y = OPT_GAP;
	sButInit.width = CLOSE_SIZE;
	sButInit.height = CLOSE_SIZE;
	sButInit.pText = pCloseText.data();
	sButInit.pTip = _("Close");
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	/* Add the add object buttons */
	sButInit.id = IDOPT_DROID;
	sButInit.width = OPT_BUTWIDTH;
	sButInit.height = OPT_BUTHEIGHT;
	sButInit.x = OPT_GAP;
	sButInit.y = OPT_EDITY;
	sButInit.pText = _("Unit");
	sButInit.pTip = _("Place Unit on map");
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	sButInit.id = IDOPT_STRUCT;
	sButInit.x += OPT_GAP + OPT_BUTWIDTH;
	sButInit.pText = _("Struct");
	sButInit.pTip = _("Place Structures on map");
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	sButInit.id = IDOPT_FEATURE;
	sButInit.x += OPT_GAP + OPT_BUTWIDTH;
	sButInit.pText = _("Feat");
	sButInit.pTip = _("Place Features on map");
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}
	if (NetPlay.bComms)
	{
		widgSetButtonState(psWScreen, sButInit.id, WBUT_DISABLE);
	}

	/* Add the quit button */
	sButInit.formID = IDOPT_FORM;
	sButInit.id = IDOPT_QUIT;
	sButInit.x = OPT_GAP;
	sButInit.y = OPT_HEIGHT - OPT_GAP - OPT_BUTHEIGHT;
	sButInit.width = OPT_WIDTH - OPT_GAP * 2;
	sButInit.height = OPT_BUTHEIGHT;
	sButInit.pText = _("Quit");
	sButInit.pTip = _("Exit Game");
	int quitButtonY = sButInit.y - OPT_GAP;
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	/* Add the player form */
	sFormInit.formID = IDOPT_FORM;
	sFormInit.id = IDOPT_PLAYERFORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = OPT_GAP;
	sFormInit.y = OPT_PLAYERY;
	sFormInit.width = OPT_WIDTH - OPT_GAP * 2;
	sFormInit.height = (OPT_BUTHEIGHT + OPT_GAP) * (1 + (MAX_PLAYERS + 3) / 4) + OPT_GAP;
	int nextFormY = sFormInit.y + sFormInit.height + OPT_GAP;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Add the player label */
	sLabInit.formID = IDOPT_PLAYERFORM;
	sLabInit.id = IDOPT_PLAYERLABEL;
	sLabInit.x = OPT_GAP;
	sLabInit.y = OPT_GAP;
	sLabInit.pText = _("Current Player:");
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return false;
	}

	/* Add the player buttons */
	sButInit.formID = IDOPT_PLAYERFORM;
	sButInit.id = IDOPT_PLAYERSTART;
	sButInit.x = OPT_GAP;
	sButInit.y = OPT_BUTHEIGHT + OPT_GAP * 2;
	sButInit.width = OPT_BUTWIDTH;
	sButInit.height = OPT_BUTHEIGHT;
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		sButInit.pText = apPlayerText[player].data();
		sButInit.pTip = apPlayerTip[player].data();
		if (!widgAddButton(psWScreen, &sButInit))
		{
			return false;
		}
		if (NetPlay.bComms)
		{
			widgSetButtonState(psWScreen, sButInit.id, WBUT_DISABLE);
		}
		/* Update the initialisation structure for the next button */
		sButInit.id += 1;
		sButInit.x += OPT_BUTWIDTH + OPT_GAP;
		if (sButInit.x + OPT_BUTWIDTH + OPT_GAP > OPT_WIDTH - OPT_GAP * 2)
		{
			sButInit.x = OPT_GAP;
			sButInit.y += OPT_BUTHEIGHT + OPT_GAP;
		}
	}

	/* Add iViS form */
	sFormInit.formID = IDOPT_FORM;
	sFormInit.id = IDOPT_IVISFORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = OPT_GAP;
	sFormInit.y = nextFormY;  //OPT_PLAYERY + OPT_BUTHEIGHT * 3 + OPT_GAP * 5;
	sFormInit.width = OPT_WIDTH - OPT_GAP * 2;
	sFormInit.height = quitButtonY - nextFormY;  //OPT_BUTHEIGHT * 3 + OPT_GAP * 4;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	widgSetButtonState(psWScreen, IDOPT_PLAYERSTART + selectedPlayer, WBUT_LOCK);

	return true;
}

bool human_computer_interface::addObjectWindow(BASE_OBJECT *psObjects, BASE_OBJECT *psSelected, bool bForceStats)
{
	UDWORD                  statID = 0;
	BASE_OBJECT            *psFirst;
	BASE_STATS		*psStats;
	DROID			*Droid;
	STRUCTURE		*Structure;
	int				compIndex;

	// Is the form already up?
	if (widgGetFromID(psWScreen, IDOBJ_FORM) != NULL)
	{
		intRemoveObjectNoAnim();
	}
	else
	{
		// reset the object position array
		asJumpPos.clear();
	}

	/* See how many objects the player has */
	apsObjectList.clear();
	for (BASE_OBJECT *psObj = psObjects; psObj; psObj = psObj->psNext)
	{
		if (objSelectFunc(psObj))
		{
			apsObjectList.push_back(psObj);
		}
	}

	if (apsObjectList.empty())
	{
		// No objects so close the stats window if it's up...
		if (widgGetFromID(psWScreen, IDSTAT_FORM) != NULL)
		{
			intRemoveStatsNoAnim();
		}
		// and return.
		return false;
	}
	psFirst = apsObjectList.front();

	/*if psSelected != NULL then check its in the list of suitable objects for
	this instance of the interface - this could happen when a structure is upgraded*/
	//if have reached the end of the loop and not quit out, then can't have found the selected object in the list
	if (std::find(apsObjectList.begin(), apsObjectList.end(), psSelected) == apsObjectList.end())
	{
		//initialise psSelected so gets set up with an iten in the list
		psSelected = NULL;
	}

	//order the objects according to what they are
	orderObjectInterface();

	// set the selected object if necessary
	if (psSelected == NULL)
	{
		//first check if there is an object selected of the required type
		switch (objMode)
		{
		case IOBJ_RESEARCH:
			psSelected = (BASE_OBJECT *)intCheckForStructure(REF_RESEARCH);
			break;
		case IOBJ_MANUFACTURE:
			psSelected = (BASE_OBJECT *)intCheckForStructure(REF_FACTORY);
			//if haven't got a Factory, check for specific types of factory
			if (!psSelected)
			{
				psSelected = (BASE_OBJECT *)intCheckForStructure(REF_CYBORG_FACTORY);
			}
			if (!psSelected)
			{
				psSelected = (BASE_OBJECT *)intCheckForStructure(REF_VTOL_FACTORY);
			}
			break;
		case IOBJ_BUILD:
			psSelected = (BASE_OBJECT *)intCheckForDroid(DROID_CONSTRUCT);
			if (!psSelected)
			{
				psSelected = (BASE_OBJECT *)intCheckForDroid(DROID_CYBORG_CONSTRUCT);
			}
			break;
		case IOBJ_COMMAND:
			psSelected = (BASE_OBJECT *)intCheckForDroid(DROID_COMMAND);
			break;
		default:
			break;
		}
		if (!psSelected)
		{
			if (apsPreviousObj[objMode]
			    && apsPreviousObj[objMode]->player == selectedPlayer)
			{
				psSelected = apsPreviousObj[objMode];
				//it is possible for a structure to change status - building of modules
				if (psSelected->type == OBJ_STRUCTURE
				    && ((STRUCTURE *)psSelected)->status != SS_BUILT)
				{
					//structure not complete so just set selected to the first valid object
					psSelected = psFirst;
				}
			}
			else
			{
				psSelected = psFirst;
			}
		}
	}

	/* Reset the current object and store the current list */
	psObjSelected = NULL;

	WIDGET *parent = psWScreen->psForm;

	/* Create the basic form */
	IntFormAnimated *objForm = new IntFormAnimated(parent, false);
	objForm->id = IDOBJ_FORM;
	objForm->setGeometry(OBJ_BACKX, OBJ_BACKY, OBJ_BACKWIDTH, OBJ_BACKHEIGHT);

	/* Add the close button */
	W_BUTINIT sButInit;
	sButInit.formID = IDOBJ_FORM;
	sButInit.id = IDOBJ_CLOSE;
	sButInit.x = OBJ_BACKWIDTH - CLOSE_WIDTH;
	sButInit.y = 0;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	/*add the tabbed form */
	IntListTabWidget *objList = new IntListTabWidget(objForm);
	objList->id = IDOBJ_TABFORM;
	objList->setChildSize(OBJ_BUTWIDTH, OBJ_BUTHEIGHT * 2);
	objList->setChildSpacing(OBJ_GAP, OBJ_GAP);
	int objListWidth = OBJ_BUTWIDTH * 5 + STAT_GAP * 4;
	objList->setGeometry((OBJ_BACKWIDTH - objListWidth) / 2, OBJ_TABY, objListWidth, OBJ_BACKHEIGHT - OBJ_TABY);

	/* Add the object and stats buttons */
	int nextObjButtonId = IDOBJ_OBJSTART;
	int nextStatButtonId = IDOBJ_STATSTART;

	// Action progress bar.
	W_BARINIT sBarInit;
	sBarInit.formID = IDOBJ_OBJSTART;
	sBarInit.id = IDOBJ_PROGBARSTART;
	sBarInit.style = WBAR_TROUGH | WIDG_HIDDEN;
	sBarInit.x = STAT_PROGBARX;
	sBarInit.y = STAT_PROGBARY;
	sBarInit.width = STAT_PROGBARWIDTH;
	sBarInit.height = STAT_PROGBARHEIGHT;
	sBarInit.size = 0;
	sBarInit.sCol = WZCOL_ACTION_PROGRESS_BAR_MAJOR;
	sBarInit.sMinorCol = WZCOL_ACTION_PROGRESS_BAR_MINOR;
	sBarInit.pTip = _("Progress Bar");

	// object output bar ie manuf power o/p, research power o/p
	W_BARINIT sBarInit2 = sBarInit;
	sBarInit2.id = IDOBJ_POWERBARSTART;
	sBarInit2.style = WBAR_PLAIN;
	sBarInit2.x = STAT_POWERBARX;
	sBarInit2.y = STAT_POWERBARY;
	sBarInit2.size = 50;

	W_LABINIT sLabInit;
	sLabInit.id = IDOBJ_COUNTSTART;
	sLabInit.style = WIDG_HIDDEN;
	sLabInit.x = OBJ_TEXTX;
	sLabInit.y = OBJ_T1TEXTY;
	sLabInit.width = 16;
	sLabInit.height = 16;
	sLabInit.pText = "BUG! (a)";

	W_LABINIT sLabInitCmdFac;
	sLabInitCmdFac.id = IDOBJ_CMDFACSTART;
	sLabInitCmdFac.style = WIDG_HIDDEN;
	sLabInitCmdFac.x = OBJ_TEXTX;
	sLabInitCmdFac.y = OBJ_T2TEXTY;
	sLabInitCmdFac.width = 16;
	sLabInitCmdFac.height = 16;
	sLabInitCmdFac.pText = "BUG! (b)";

	W_LABINIT sLabInitCmdFac2;
	sLabInitCmdFac2.id = IDOBJ_CMDVTOLFACSTART;
	sLabInitCmdFac2.style = WIDG_HIDDEN;
	sLabInitCmdFac2.x = OBJ_TEXTX;
	sLabInitCmdFac2.y = OBJ_T3TEXTY;
	sLabInitCmdFac2.width = 16;
	sLabInitCmdFac2.height = 16;
	sLabInitCmdFac2.pText = "BUG! (c)";

	W_LABINIT sLabIntObjText;
	sLabIntObjText.id = IDOBJ_FACTORYSTART;
	sLabIntObjText.style = WIDG_HIDDEN;
	sLabIntObjText.x = OBJ_TEXTX;
	sLabIntObjText.y = OBJ_B1TEXTY;
	sLabIntObjText.width = 16;
	sLabIntObjText.height = 16;
	sLabIntObjText.pText = "xxx/xxx - overrun";

	W_LABINIT sLabInitCmdExp;
	sLabInitCmdExp.id = IDOBJ_CMDEXPSTART;
	sLabInitCmdExp.style = WIDG_HIDDEN;
	sLabInitCmdExp.x = STAT_POWERBARX;
	sLabInitCmdExp.y = STAT_POWERBARY;
	sLabInitCmdExp.width = 16;
	sLabInitCmdExp.height = 16;
	sLabInitCmdExp.pText = "@@@@@ - overrun";

	W_LABINIT sAllyResearch;
	sAllyResearch.id = IDOBJ_ALLYRESEARCHSTART;
	sAllyResearch.width = iV_GetImageWidth(IntImages, IMAGE_ALLY_RESEARCH);
	sAllyResearch.height = iV_GetImageHeight(IntImages, IMAGE_ALLY_RESEARCH);
	sAllyResearch.y = 10;
	sAllyResearch.pDisplay = intDisplayAllyIcon;

	for (unsigned i = 0; i < apsObjectList.size(); ++i)
	{
		BASE_OBJECT *psObj = apsObjectList[i];
		if (psObj->died != 0)
		{
			continue; // Don't add the button if the objects dead.
		}
		bool IsFactory = false;
		bool isResearch = false;

		WIDGET *buttonHolder = new WIDGET(objList);
		objList->addWidgetToLayout(buttonHolder);

		IntStatusButton *statButton = new IntStatusButton(buttonHolder);
		statButton->id = nextStatButtonId;
		statButton->setGeometry(0, 0, OBJ_BUTWIDTH, OBJ_BUTHEIGHT);
		statButton->style |= WFORM_SECONDARY;

		IntObjectButton *objButton = new IntObjectButton(buttonHolder);
		objButton->id = nextObjButtonId;
		objButton->setObject(psObj);
		objButton->setGeometry(0, OBJ_STARTY, OBJ_BUTWIDTH, OBJ_BUTHEIGHT);

		/* Got an object - set the text and tip for the button */
		switch (psObj->type)
		{
		case OBJ_DROID:
			// Get the construction power of a construction droid.. Not convinced this is right.
			Droid = (DROID *)psObj;
			if (Droid->droidType == DROID_CONSTRUCT || Droid->droidType == DROID_CYBORG_CONSTRUCT)
			{
				compIndex = Droid->asBits[COMP_CONSTRUCT];
				ASSERT_OR_RETURN(false, Droid->asBits[COMP_CONSTRUCT], "Invalid droid type");
				ASSERT_OR_RETURN(false, compIndex < numConstructStats, "Invalid range referenced for numConstructStats, %d > %d", compIndex, numConstructStats);
				psStats = (BASE_STATS *)(asConstructStats + compIndex);
				sBarInit2.size = (UWORD)constructorPoints((CONSTRUCT_STATS *)psStats, Droid->player);
				if (sBarInit2.size > WBAR_SCALE)
				{
					sBarInit2.size = WBAR_SCALE;
				}
			}
			objButton->setTip(droidGetName((DROID *)psObj));
			break;

		case OBJ_STRUCTURE:
			// Get the construction power of a structure
			Structure = (STRUCTURE *)psObj;
			switch (Structure->pStructureType->type)
			{
			case REF_FACTORY:
			case REF_CYBORG_FACTORY:
			case REF_VTOL_FACTORY:
				sBarInit2.size = getBuildingProductionPoints(Structure);
				if (sBarInit2.size > WBAR_SCALE)
				{
					sBarInit2.size = WBAR_SCALE;
				}
				IsFactory = true;
				//right click on factory centres on DP
				objButton->style |= WFORM_SECONDARY;
				break;

			case REF_RESEARCH:
				sBarInit2.size = getBuildingResearchPoints(Structure);
				if (sBarInit2.size > WBAR_SCALE)
				{
					sBarInit2.size = WBAR_SCALE;
				}
				isResearch = true;
				break;

			default:
				ASSERT(false, "Invalid structure type");
				return false;
			}
			objButton->setTip(getName(((STRUCTURE *)psObj)->pStructureType));
			break;

		case OBJ_FEATURE:
			objButton->setTip(getName(((FEATURE *)psObj)->psStats));
			break;

		default:
			break;
		}

		if (IsFactory)
		{
			// Add a text label for the factory Inc.
			sLabIntObjText.formID = nextObjButtonId;
			sLabIntObjText.pCallback = intAddFactoryInc;
			sLabIntObjText.pUserData = psObj;
			if (!widgAddLabel(psWScreen, &sLabIntObjText))
			{
				return false;
			}
			sLabIntObjText.id++;
		}
		if (isResearch)
		{
			RESEARCH *Stat = ((RESEARCH_FACILITY *)((STRUCTURE *)psObj)->pFunctionality)->psSubject;
			if (Stat != NULL)
			{
				// Show if allies are researching the same as us.
				std::vector<AllyResearch> const &researches = listAllyResearch(Stat->ref);
				unsigned numResearches = std::min<unsigned>(researches.size(), 4);  // Only display at most 4 allies, since that's what there's room for.
				for (unsigned ii = 0; ii < numResearches; ++ii)
				{
					sAllyResearch.formID = nextObjButtonId;
					sAllyResearch.x = STAT_BUTWIDTH  - (sAllyResearch.width + 2) * ii - sAllyResearch.width - 2;
					sAllyResearch.UserData = PACKDWORD(Stat->ref - REF_RESEARCH_START, ii);
					sAllyResearch.pTip = getPlayerName(researches[ii].player);
					widgAddLabel(psWScreen, &sAllyResearch);

					ASSERT_OR_RETURN(false, sAllyResearch.id <= IDOBJ_ALLYRESEARCHEND, " ");
					++sAllyResearch.id;
				}
			}
		}
		// Add the power bar.
		if (psObj->type != OBJ_DROID || (((DROID *)psObj)->droidType == DROID_CONSTRUCT || ((DROID *)psObj)->droidType == DROID_CYBORG_CONSTRUCT))
		{
			sBarInit2.formID = nextObjButtonId;
			sBarInit.iRange = GAME_TICKS_PER_SEC;
			if (!widgAddBarGraph(psWScreen, &sBarInit2))
			{
				return false;
			}
		}

		// Add command droid bits
		if ((psObj->type == OBJ_DROID) &&
		    (((DROID *)psObj)->droidType == DROID_COMMAND))
		{
			// the group size label
			sLabIntObjText.formID = nextObjButtonId;
			sLabIntObjText.pCallback = intUpdateCommandSize;
			sLabIntObjText.pUserData = psObj;
			if (!widgAddLabel(psWScreen, &sLabIntObjText))
			{
				return false;
			}
			sLabIntObjText.id++;

			// the experience stars
			sLabInitCmdExp.formID = nextObjButtonId;
			sLabInitCmdExp.pCallback = intUpdateCommandExp;
			sLabInitCmdExp.pUserData = psObj;
			if (!widgAddLabel(psWScreen, &sLabInitCmdExp))
			{
				return false;
			}
			sLabInitCmdExp.id++;
		}

		/* Now do the stats button */
		psStats = objGetStatsFunc(psObj);

		if (psStats != NULL)
		{
			statButton->setTip(getName(psStats));
			statButton->setObjectAndStats(psObj, psStats);
		}
		else if ((psObj->type == OBJ_DROID) && (((DROID *)psObj)->droidType == DROID_COMMAND))
		{
			statButton->setObject(psObj);
		}
		else
		{
			statButton->setObject(nullptr);
		}

		// Add command droid bits
		if ((psObj->type == OBJ_DROID) &&
		    (((DROID *)psObj)->droidType == DROID_COMMAND))
		{
			// the assigned factories label
			sLabInit.formID = nextStatButtonId;
			sLabInit.pCallback = intUpdateCommandFact;
			sLabInit.pUserData = psObj;

			// the assigned cyborg factories label
			sLabInitCmdFac.formID = nextStatButtonId;
			sLabInitCmdFac.pCallback = intUpdateCommandFact;
			sLabInitCmdFac.pUserData = psObj;
			widgAddLabel(psWScreen, &sLabInitCmdFac);
			// the assigned VTOL factories label
			sLabInitCmdFac2.formID = nextStatButtonId;
			sLabInitCmdFac2.pCallback = intUpdateCommandFact;
			sLabInitCmdFac2.pUserData = psObj;
			widgAddLabel(psWScreen, &sLabInitCmdFac2);
		}
		else
		{
			// Add a text label for the size of the production run.
			sLabInit.formID = nextStatButtonId;
			sLabInit.pCallback = intUpdateQuantity;
			sLabInit.pUserData = psObj;
		}
		W_LABEL *label = widgAddLabel(psWScreen, &sLabInit);

		// Add the progress bar.
		sBarInit.formID = nextStatButtonId;
		// Setup widget update callback and object pointer so we can update the progress bar.
		sBarInit.pCallback = intUpdateProgressBar;
		sBarInit.pUserData = psObj;
		sBarInit.iRange = GAME_TICKS_PER_SEC;

		W_BARGRAPH *bar = widgAddBarGraph(psWScreen, &sBarInit);
		if (psObj->type != OBJ_DROID || (((DROID *)psObj)->droidType == DROID_CONSTRUCT || ((DROID *)psObj)->droidType == DROID_CYBORG_CONSTRUCT))
		{
			// Set the colour for the production run size text.
			label->setFontColour(WZCOL_ACTION_PRODUCTION_RUN_TEXT);
			bar->setBackgroundColour(WZCOL_ACTION_PRODUCTION_RUN_BACKGROUND);
		}

		/* If this matches psSelected note which form to display */
		if (psSelected == psObj)
		{
			objList->setCurrentPage(objList->pages() - 1);
			statID = nextStatButtonId;
		}

		/* Set up the next button (Objects) */
		++nextObjButtonId;
		ASSERT_OR_RETURN(false, nextObjButtonId < IDOBJ_OBJEND, "Too many object buttons");

		/* Set up the next button (Stats) */
		sLabInit.id += 1;
		sLabInitCmdFac.id += 1;
		sLabInitCmdFac2.id += 1;

		sBarInit.id += 1;
		ASSERT_OR_RETURN(false, sBarInit.id < IDOBJ_PROGBAREND, "Too many progress bars");

		sBarInit2.id += 1;
		ASSERT_OR_RETURN(false, sBarInit2.id < IDOBJ_POWERBAREND, "Too many power bars");

		++nextStatButtonId;
		ASSERT_OR_RETURN(false, nextStatButtonId < IDOBJ_STATEND, "Too many stat buttons");

		if (nextObjButtonId > IDOBJ_OBJEND)
		{
			//can't fit any more on the screen!
			debug(LOG_WARNING, "This is just a Warning! Max buttons have been allocated");
			break;
		}
	}

	// if the selected object isn't on one of the main buttons (too many objects)
	// reset the selected pointer
	if (statID == 0)
	{
		psSelected = NULL;
	}

	if (psSelected && (objMode != IOBJ_COMMAND))
	{
		if (bForceStats || widgGetFromID(psWScreen, IDSTAT_FORM))
		{
			objStatID = statID;
			addObjectStats(psSelected, statID);
			intMode = INT_STAT;
		}
		else
		{
			widgSetButtonState(psWScreen, statID, WBUT_CLICKLOCK);
			intMode = INT_OBJECT;
		}
	}
	else if (psSelected)
	{
		/* Note the object */
		psObjSelected = psSelected;
		objStatID = statID;
		intAddOrder(psSelected);
		widgSetButtonState(psWScreen, statID, WBUT_CLICKLOCK);

		intMode = INT_CMDORDER;
	}
	else
	{
		intMode = INT_OBJECT;
	}

	if (objMode == IOBJ_BUILD || objMode == IOBJ_MANUFACTURE || objMode == IOBJ_RESEARCH)
	{
		intShowPowerBar();
	}

	if (bInTutorial)
	{
		debug(LOG_NEVER, "Go with object open callback!");
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_OBJECTOPEN);
	}

	return true;
}


bool human_computer_interface::updateObject(BASE_OBJECT *psObjects, BASE_OBJECT *psSelected, bool bForceStats)
{
	addObjectWindow(psObjects, psSelected, bForceStats);

	// if the stats screen is up and..
	if (statsUp)
	{
		if (psStatsScreenOwner != NULL)
		{
			// it's owner is dead then..
			if (psStatsScreenOwner->died != 0)
			{
				// remove it.
				intRemoveStatsNoAnim();
			}
		}
	}

	return true;
}

/* Remove the build widgets from the widget screen */
void intRemoveObject(void)
{
	widgDelete(psWScreen, IDOBJ_TABFORM);
	widgDelete(psWScreen, IDOBJ_CLOSE);

	// Start the window close animation.
	IntFormAnimated *Form = (IntFormAnimated *)widgGetFromID(psWScreen, IDOBJ_FORM);
	if (Form)
	{
		Form->closeAnimateDelete();
	}

	intHidePowerBar();

	if (bInTutorial)
	{
		debug(LOG_NEVER, "Go with object close callback!");
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_OBJECTCLOSE);
	}
}


/* Remove the build widgets from the widget screen */
static void intRemoveObjectNoAnim(void)
{
	widgDelete(psWScreen, IDOBJ_TABFORM);
	widgDelete(psWScreen, IDOBJ_CLOSE);
	widgDelete(psWScreen, IDOBJ_FORM);

	intHidePowerBar();
}


/* Remove the stats widgets from the widget screen */
void human_computer_interface::removeStats()
{
	widgDelete(psWScreen, IDSTAT_CLOSE);
	widgDelete(psWScreen, IDSTAT_TABFORM);

	// Start the window close animation.
	IntFormAnimated *Form = (IntFormAnimated *)widgGetFromID(psWScreen, IDSTAT_FORM);
	if (Form)
	{
		Form->closeAnimateDelete();
	}

	statsUp = false;
	psStatsScreenOwner = NULL;
}

/* Remove the stats widgets from the widget screen */
void human_computer_interface::removeStatsNoAnim()
{
	widgDelete(psWScreen, IDSTAT_CLOSE);
	widgDelete(psWScreen, IDSTAT_TABFORM);
	widgDelete(psWScreen, IDSTAT_FORM);

	statsUp = false;
	psStatsScreenOwner = NULL;
}

BASE_OBJECT *human_computer_interface::getObject(uint32_t id)
{
	BASE_OBJECT		*psObj;

	/* If this is a stats button, find the object button linked to it */
	if (id >= IDOBJ_STATSTART && id <= IDOBJ_STATEND)
	{
		id = IDOBJ_OBJSTART + id - IDOBJ_STATSTART;
	}

	/* Find the object that the ID refers to */
	ASSERT_OR_RETURN(NULL, id - IDOBJ_OBJSTART < apsObjectList.size(), "Invalid button ID %u", id);
	psObj = apsObjectList[id - IDOBJ_OBJSTART];

	return psObj;
}

void human_computer_interface::setStats(uint32_t id, BASE_STATS *psStats)
{
	/* Update the button on the object screen */
	IntStatusButton *statButton = (IntStatusButton *)widgGetFromID(psWScreen, id);
	if (statButton == nullptr)
	{
		return;
	}
	statButton->setTip("");
	WIDGET::Children children = statButton->children();
	for (WIDGET::Children::const_iterator i = children.begin(); i != children.end(); ++i)
	{
		delete *i;
	}

	// Action progress bar.
	W_BARINIT sBarInit;
	sBarInit.formID = id;
	sBarInit.id = (id - IDOBJ_STATSTART) + IDOBJ_PROGBARSTART;
	sBarInit.style = WBAR_TROUGH;
	sBarInit.x = STAT_PROGBARX;
	sBarInit.y = STAT_PROGBARY;
	sBarInit.width = STAT_PROGBARWIDTH;
	sBarInit.height = STAT_PROGBARHEIGHT;
	sBarInit.size = 0;
	sBarInit.sCol = WZCOL_ACTION_PROGRESS_BAR_MAJOR;
	sBarInit.sMinorCol = WZCOL_ACTION_PROGRESS_BAR_MINOR;
	sBarInit.iRange = GAME_TICKS_PER_SEC;
	// Setup widget update callback and object pointer so we can update the progress bar.
	sBarInit.pCallback = intUpdateProgressBar;
	sBarInit.pUserData = getObject(id);

	W_LABINIT sLabInit;
	sLabInit.formID = id;
	sLabInit.id = (id - IDOBJ_STATSTART) + IDOBJ_COUNTSTART;
	sLabInit.style = WIDG_HIDDEN;
	sLabInit.x = OBJ_TEXTX;
	sLabInit.y = OBJ_T1TEXTY;
	sLabInit.width = 16;
	sLabInit.height = 16;
	sLabInit.pText = "BUG! (d)";

	if (psStats)
	{
		statButton->setTip(getName(psStats));
		statButton->setObjectAndStats(getObject(id), psStats);

		// Add a text label for the size of the production run.
		sLabInit.pCallback = intUpdateQuantity;
		sLabInit.pUserData = sBarInit.pUserData;
	}
	else
	{
		statButton->setObject(nullptr);

		/* Reset the stats screen button if necessary */
		if ((INTMODE)objMode == INT_STAT && statID != 0)
		{
			widgSetButtonState(psWScreen, statID, 0);
		}
	}

	// Set the colour for the production run size text.

	widgAddLabel(psWScreen, &sLabInit)->setFontColour(WZCOL_ACTION_PRODUCTION_RUN_TEXT);
	widgAddBarGraph(psWScreen, &sBarInit)->setBackgroundColour(WZCOL_ACTION_PRODUCTION_RUN_BACKGROUND);

	BASE_OBJECT *psObj = getObject(id);
	if (psObj && psObj->selected)
	{
		widgSetButtonState(psWScreen, id, WBUT_CLICKLOCK);
	}
}

StateButton *human_computer_interface::makeObsoleteButton(WIDGET *parent)
{
	StateButton *obsoleteButton = new StateButton(parent);
	obsoleteButton->id = IDSTAT_OBSOLETE_BUTTON;
	obsoleteButton->style |= WBUT_SECONDARY;
	obsoleteButton->setState(includeRedundantDesigns);
	obsoleteButton->setImages(false, StateButton::Images(Image(IntImages, IMAGE_OBSOLETE_HIDE_UP), Image(IntImages, IMAGE_OBSOLETE_HIDE_DOWN), Image(IntImages, IMAGE_OBSOLETE_HIDE_HI)));
	obsoleteButton->setTip(false, _("Hiding Obsolete Tech"));
	obsoleteButton->setImages(true,  StateButton::Images(Image(IntImages, IMAGE_OBSOLETE_SHOW_UP), Image(IntImages, IMAGE_OBSOLETE_SHOW_DOWN), Image(IntImages, IMAGE_OBSOLETE_SHOW_HI)));
	obsoleteButton->setTip(true, _("Showing Obsolete Tech"));
	obsoleteButton->move(4 + Image(IntImages, IMAGE_FDP_UP).width() + 4, STAT_SLDY);
	return obsoleteButton;
}

/* Add the stats widgets to the widget screen */
/* If psSelected != NULL it specifies which stat should be hilited
   psOwner specifies which object is hilighted on the object bar for this stat*/
bool human_computer_interface::addStats(BASE_STATS **ppsStatsList, uint32_t numStats,
                                           BASE_STATS *psSelected, BASE_OBJECT *psOwner)
{
	FACTORY				*psFactory;

	int                             allyResearchIconCount = 0;

	// should this ever be called with psOwner == NULL?

	// Is the form already up?
	if (widgGetFromID(psWScreen, IDSTAT_FORM) != NULL)
	{
		intRemoveStatsNoAnim();
	}

	// is the order form already up ?
	if (widgGetFromID(psWScreen, IDORDER_FORM) != NULL)
	{
		intRemoveOrderNoAnim();
	}

	if (psOwner != NULL)
	{
		// Return if the owner is dead.
		if (psOwner->died)
		{
			debug(LOG_GUI, "intAddStats: Owner is dead");
			return false;
		}
	}
	secondaryWindowUp = true;
	psStatsScreenOwner = psOwner;

	WIDGET *parent = psWScreen->psForm;

	/* Create the basic form */
	IntFormAnimated *statForm = new IntFormAnimated(parent, false);
	statForm->id = IDSTAT_FORM;
	statForm->setGeometry(STAT_X, STAT_Y, STAT_WIDTH, STAT_HEIGHT);

	W_LABINIT sLabInit;

	// Add the quantity slider ( if it's a factory ).
	if (objMode == IOBJ_MANUFACTURE && psOwner != nullptr)
	{
		STRUCTURE_TYPE factoryType = ((STRUCTURE *)psOwner)->pStructureType->type;

		//add the Factory DP button
		W_BUTTON *deliveryPointButton = new W_BUTTON(statForm);
		deliveryPointButton->id = IDSTAT_DP_BUTTON;
		deliveryPointButton->style |= WBUT_SECONDARY;
		switch (factoryType)
		{
		default:
		case REF_FACTORY:        deliveryPointButton->setImages(Image(IntImages, IMAGE_FDP_UP), Image(IntImages, IMAGE_FDP_DOWN), Image(IntImages, IMAGE_FDP_HI)); break;
		case REF_CYBORG_FACTORY: deliveryPointButton->setImages(Image(IntImages, IMAGE_CDP_UP), Image(IntImages, IMAGE_CDP_DOWN), Image(IntImages, IMAGE_CDP_HI)); break;
		case REF_VTOL_FACTORY:   deliveryPointButton->setImages(Image(IntImages, IMAGE_VDP_UP), Image(IntImages, IMAGE_VDP_DOWN), Image(IntImages, IMAGE_VDP_HI)); break;
		}
		deliveryPointButton->move(4, STAT_SLDY);
		deliveryPointButton->setTip(_("Factory Delivery Point"));
		deliveryPointButton->pUserData = psOwner;

		//add the Factory Loop button!
		W_BUTTON *loopButton = new W_BUTTON(statForm);
		loopButton->id = IDSTAT_LOOP_BUTTON;
		loopButton->style |= WBUT_SECONDARY;
		loopButton->setImages(Image(IntImages, IMAGE_LOOP_UP), Image(IntImages, IMAGE_LOOP_DOWN), Image(IntImages, IMAGE_LOOP_HI));
		loopButton->move(STAT_SLDX + STAT_SLDWIDTH + 2, STAT_SLDY);
		loopButton->setTip(_("Loop Production"));

		if (psOwner != NULL)
		{
			psFactory = (FACTORY *)((STRUCTURE *)psOwner)->pFunctionality;
			if (psFactory->psSubject != NULL && psFactory->productionLoops != 0)
			{
				widgSetButtonState(psWScreen, IDSTAT_LOOP_BUTTON, WBUT_CLICKLOCK);
			}
		}

		// create a text label for the loop quantity.
		sLabInit.formID = IDSTAT_FORM;
		sLabInit.id = IDSTAT_LOOP_LABEL;
		sLabInit.style = WIDG_HIDDEN;
		sLabInit.x = loopButton->x() - 15;
		sLabInit.y = loopButton->y();
		sLabInit.width = 12;
		sLabInit.height = 15;
		sLabInit.pUserData = psOwner;
		sLabInit.pCallback = intAddLoopQuantity;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
	}

	if (objMode == IOBJ_MANUFACTURE)
	{
		/* store the common values for the text labels for the quantity
		to produce (on each button).*/
		sLabInit = W_LABINIT();
		sLabInit.id = IDSTAT_PRODSTART;
		sLabInit.style = WIDG_HIDDEN | WLAB_ALIGNRIGHT;

		sLabInit.x = STAT_BUTWIDTH - 12 - 6;
		sLabInit.y = 2;

		sLabInit.width = 12;
		sLabInit.height = 15;
		sLabInit.pCallback = intAddProdQuantity;
	}

	if ((objMode == IOBJ_MANUFACTURE || objMode == IOBJ_BUILD) && psOwner != nullptr)
	{
		// Add the obsolete items button.
		makeObsoleteButton(statForm);
	}

	/* Add the close button */
	W_BUTINIT sButInit;
	sButInit.formID = IDSTAT_FORM;
	sButInit.id = IDSTAT_CLOSE;
	sButInit.x = STAT_WIDTH - CLOSE_WIDTH;
	sButInit.y = 0;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	// Add the tabbed form
	IntListTabWidget *statList = new IntListTabWidget(statForm);
	statList->id = IDSTAT_TABFORM;
	statList->setChildSize(STAT_BUTWIDTH, STAT_BUTHEIGHT);
	statList->setChildSpacing(STAT_GAP, STAT_GAP);
	int statListWidth = STAT_BUTWIDTH * 2 + STAT_GAP;
	statList->setGeometry((STAT_WIDTH - statListWidth) / 2, STAT_TABFORMY, statListWidth, STAT_HEIGHT - STAT_TABFORMY);

	/* Add the stat buttons */
	int nextButtonId = IDSTAT_START;

	W_BARINIT sBarInit;
	sBarInit.id = IDSTAT_TIMEBARSTART;
	sBarInit.x = STAT_TIMEBARX;
	sBarInit.width = STAT_PROGBARWIDTH;
	sBarInit.height = STAT_PROGBARHEIGHT;
	sBarInit.size = 50;
	sBarInit.sCol = WZCOL_ACTION_PROGRESS_BAR_MAJOR;
	sBarInit.sMinorCol = WZCOL_ACTION_PROGRESS_BAR_MINOR;

	statID = 0;
	for (unsigned i = 0; i < numStats; i++)
	{
		sBarInit.y = STAT_TIMEBARY;

		if (nextButtonId > IDSTAT_END)
		{
			//can't fit any more on the screen!
			debug(LOG_WARNING, "This is just a Warning! Max buttons have been allocated");
			break;
		}

		IntStatsButton *button = new IntStatsButton(statList);
		button->id = nextButtonId;
		button->style |= WFORM_SECONDARY;
		button->setStats(ppsStatsList[i]);
		statList->addWidgetToLayout(button);

		BASE_STATS *Stat = ppsStatsList[i];
		QString tipString = getName(ppsStatsList[i]);
		unsigned powerCost = 0;
		W_BARGRAPH *bar;
		if (Stat->ref >= REF_STRUCTURE_START &&
		    Stat->ref < REF_STRUCTURE_START + REF_RANGE)  		// It's a structure.
		{
			powerCost = ((STRUCTURE_STATS *)Stat)->powerToBuild;
			sBarInit.size = powerCost / POWERPOINTS_DROIDDIV;
			if (sBarInit.size > 100)
			{
				sBarInit.size = 100;
			}

			sBarInit.formID = nextButtonId;
			sBarInit.iRange = GAME_TICKS_PER_SEC;
			bar = widgAddBarGraph(psWScreen, &sBarInit);
			bar->setBackgroundColour(WZCOL_BLACK);
		}
		else if (Stat->ref >= REF_TEMPLATE_START &&
		         Stat->ref < REF_TEMPLATE_START + REF_RANGE)  	// It's a droid.
		{
			powerCost = calcTemplatePower((DROID_TEMPLATE *)Stat);
			sBarInit.size = powerCost / POWERPOINTS_DROIDDIV;
			if (sBarInit.size > 100)
			{
				sBarInit.size = 100;
			}

			sBarInit.formID = nextButtonId;
			sBarInit.iRange = GAME_TICKS_PER_SEC;
			bar = widgAddBarGraph(psWScreen, &sBarInit);
			bar->setBackgroundColour(WZCOL_BLACK);

			// Add a text label for the quantity to produce.
			sLabInit.formID = nextButtonId;
			sLabInit.pUserData = Stat;
			if (!widgAddLabel(psWScreen, &sLabInit))
			{
				return false;
			}
			sLabInit.id++;
		}
		else if (Stat->ref >= REF_RESEARCH_START &&
		         Stat->ref < REF_RESEARCH_START + REF_RANGE)				// It's a Research topic.
		{
			sLabInit = W_LABINIT();
			sLabInit.formID = nextButtonId;
			sLabInit.id = IDSTAT_RESICONSTART + (nextButtonId - IDSTAT_START);

			sLabInit.x = STAT_BUTWIDTH - 16;
			sLabInit.y = 3;

			sLabInit.width = 12;
			sLabInit.height = 15;
			sLabInit.pUserData = Stat;
			sLabInit.pDisplay = intDisplayResSubGroup;
			widgAddLabel(psWScreen, &sLabInit);

			//add power bar as well
			powerCost = ((RESEARCH *)Stat)->researchPower;
			sBarInit.size = powerCost / POWERPOINTS_DROIDDIV;
			if (sBarInit.size > 100)
			{
				sBarInit.size = 100;
			}

			// if multiplayer, if research topic is being done by another ally then mark as such..
			if (bMultiPlayer)
			{
				std::vector<AllyResearch> const &researches = listAllyResearch(Stat->ref);
				unsigned numResearches = std::min<unsigned>(researches.size(), 4);  // Only display at most 4 allies, since that's what there's room for.
				for (unsigned ii = 0; ii < numResearches; ++ii)
				{
					// add a label.
					sLabInit = W_LABINIT();
					sLabInit.formID = nextButtonId;
					sLabInit.id = IDSTAT_ALLYSTART + allyResearchIconCount;
					sLabInit.width = iV_GetImageWidth(IntImages, IMAGE_ALLY_RESEARCH);
					sLabInit.height = iV_GetImageHeight(IntImages, IMAGE_ALLY_RESEARCH);
					sLabInit.x = STAT_BUTWIDTH  - (sLabInit.width + 2) * ii - sLabInit.width - 2;
					sLabInit.y = STAT_BUTHEIGHT - sLabInit.height - 3 - STAT_PROGBARHEIGHT;
					sLabInit.UserData = PACKDWORD(Stat->ref - REF_RESEARCH_START, ii);
					sLabInit.pTip = getPlayerName(researches[ii].player);
					sLabInit.pDisplay = intDisplayAllyIcon;
					widgAddLabel(psWScreen, &sLabInit);

					++allyResearchIconCount;
					ASSERT_OR_RETURN(false, allyResearchIconCount < IDSTAT_ALLYEND - IDSTAT_ALLYSTART, " too many research icons? ");
				}

				if (numResearches > 0)
				{
					W_BARINIT progress;
					progress.formID = nextButtonId;
					progress.id = IDSTAT_ALLYSTART + allyResearchIconCount;
					progress.width = STAT_PROGBARWIDTH;
					progress.height = STAT_PROGBARHEIGHT;
					progress.x = STAT_TIMEBARX;
					progress.y = STAT_TIMEBARY;
					progress.UserData = Stat->ref - REF_RESEARCH_START;
					progress.pTip = _("Ally progress");
					progress.pDisplay = intDisplayAllyBar;
					W_BARGRAPH *bar = widgAddBarGraph(psWScreen, &progress);
					bar->setBackgroundColour(WZCOL_BLACK);

					++allyResearchIconCount;

					sBarInit.y -= STAT_PROGBARHEIGHT + 2;  // Move cost bar up, to avoid overlap.
				}
			}

			sBarInit.formID = nextButtonId;
			bar = widgAddBarGraph(psWScreen, &sBarInit);
			bar->setBackgroundColour(WZCOL_BLACK);
		}
		tipString.append(QString::fromUtf8(_("\nCost: %1")).arg(powerCost));
		button->setTip(tipString);

		/* If this matches psSelected note the form and button */
		if (ppsStatsList[i] == psSelected)
		{
			statID = nextButtonId;
			button->setState(WBUT_CLICKLOCK);
			statList->setCurrentPage(statList->pages() - 1);
		}

		/* Update the init struct for the next button */
		++nextButtonId;

		sBarInit.id += 1;
	}

	statsUp = true;

	// call the tutorial callbacks if necessary
	if (bInTutorial)
	{
		switch (objMode)
		{
		case IOBJ_BUILD:
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_BUILDLIST);
			break;
		case IOBJ_RESEARCH:
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_RESEARCHLIST);
			break;
		case IOBJ_MANUFACTURE:
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_MANULIST);
			break;
		default:
			break;
		}
	}

	return true;
}


/* Select a command droid */
static bool selectCommand(BASE_OBJECT *psObj)
{
	ASSERT_OR_RETURN(false, psObj && psObj->type == OBJ_DROID, "Invalid droid pointer");
	DROID *psDroid = (DROID *)psObj;
	if (psDroid->droidType == DROID_COMMAND && psDroid->died == 0)
	{
		return true;
	}
	return false;
}

/* Return the stats for a command droid */
static BASE_STATS *getCommandStats(WZ_DECL_UNUSED BASE_OBJECT *psObj)
{
	return NULL;
}

/* Set the stats for a command droid */
static bool setCommandStats(WZ_DECL_UNUSED BASE_OBJECT *psObj, WZ_DECL_UNUSED BASE_STATS *psStats)
{
	return true;
}

/* Select a construction droid */
static bool selectConstruction(BASE_OBJECT *psObj)
{
	DROID	*psDroid;

	ASSERT_OR_RETURN(false, psObj != NULL && psObj->type == OBJ_DROID, "Invalid droid pointer");
	psDroid = (DROID *)psObj;

	//check the droid type
	if ((psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT) && (psDroid->died == 0))
	{
		return true;
	}

	return false;
}

/* Return the stats for a construction droid */
static BASE_STATS *getConstructionStats(BASE_OBJECT *psObj)
{
	DROID *psDroid = (DROID *)psObj;
	BASE_STATS *Stats;
	BASE_OBJECT *Structure;
	UDWORD x, y;

	ASSERT_OR_RETURN(NULL, psObj != NULL && psObj->type == OBJ_DROID, "Invalid droid pointer");

	if (!(droidType(psDroid) == DROID_CONSTRUCT ||
	      droidType(psDroid) == DROID_CYBORG_CONSTRUCT))
	{
		return NULL;
	}

	if (orderStateStatsLoc(psDroid, DORDER_BUILD, &Stats, &x, &y)) // Moving to build location?
	{
		return Stats;
	}
	else if ((Structure = orderStateObj(psDroid, DORDER_BUILD))
	         && psDroid->order.type == DORDER_BUILD) // Is building
	{
		return psDroid->order.psStats;
	}
	else if ((Structure = orderStateObj(psDroid, DORDER_HELPBUILD))
	         && (psDroid->order.type == DORDER_HELPBUILD
	             || psDroid->order.type == DORDER_LINEBUILD)) // Is helping
	{
		return (BASE_STATS *)((STRUCTURE *)Structure)->pStructureType;
	}
	else if (orderState(psDroid, DORDER_DEMOLISH))
	{
		return (BASE_STATS *)structGetDemolishStat();
	}

	return NULL;
}

/* Set the stats for a construction droid */
static bool setConstructionStats(BASE_OBJECT *psObj, BASE_STATS *psStats)
{
	DROID *psDroid = castDroid(psObj);
	ASSERT_OR_RETURN(false, psDroid != NULL, "invalid droid pointer");

	/* psStats might be NULL if the operation is canceled in the middle */
	if (psStats != NULL)
	{
		//check for demolish first
		if (psStats == structGetDemolishStat())
		{
			objMode = IOBJ_DEMOLISHSEL;

			return true;
		}

		/* Store the stats for future use */
		psPositionStats = psStats;

		/* Now start looking for a location for the structure */
		objMode = IOBJ_BUILDSEL;

		intStartStructPosition(psStats);

		return true;
	}
	orderDroid(psDroid, DORDER_STOP, ModeQueue);
	return true;
}

/* Select a research facility */
static bool selectResearch(BASE_OBJECT *psObj)
{
	STRUCTURE	*psResFacility;

	ASSERT_OR_RETURN(false, psObj != NULL && psObj->type == OBJ_STRUCTURE, "Invalid Structure pointer");

	psResFacility = (STRUCTURE *)psObj;

	/* A Structure is a research facility if its type = REF_RESEARCH and is
	   completely built*/
	if (psResFacility->pStructureType->type == REF_RESEARCH && (psResFacility->
	        status == SS_BUILT) && (psResFacility->died == 0))
	{
		return true;
	}
	return false;
}

/* Return the stats for a research facility */
static BASE_STATS *getResearchStats(BASE_OBJECT *psObj)
{
	ASSERT_OR_RETURN(NULL, psObj != NULL && psObj->type == OBJ_STRUCTURE, "Invalid Structure pointer");
	STRUCTURE *psBuilding = (STRUCTURE *)psObj;
	RESEARCH_FACILITY *psResearchFacility = &psBuilding->pFunctionality->researchFacility;

	if (psResearchFacility->psSubjectPending != NULL && !IsResearchCompleted(&asPlayerResList[psObj->player][psResearchFacility->psSubjectPending->index]))
	{
		return psResearchFacility->psSubjectPending;
	}

	return psResearchFacility->psSubject;
}

/* Set the stats for a research facility */
static bool setResearchStats(BASE_OBJECT *psObj, BASE_STATS *psStats)
{
	STRUCTURE			*psBuilding;
	RESEARCH               *pResearch = (RESEARCH *)psStats;
	PLAYER_RESEARCH		*pPlayerRes;
	UDWORD				count;
	RESEARCH_FACILITY	*psResFacilty;

	ASSERT_OR_RETURN(false, psObj != NULL && psObj->type == OBJ_STRUCTURE, "Invalid Structure pointer");
	/* psStats might be NULL if the operation is canceled in the middle */
	psBuilding = (STRUCTURE *)psObj;
	psResFacilty = &psBuilding->pFunctionality->researchFacility;

	if (bMultiMessages)
	{
		if (pResearch != NULL)
		{
			// Say that we want to do reseach [sic].
			sendResearchStatus(psBuilding, pResearch->ref - REF_RESEARCH_START, selectedPlayer, true);
			setStatusPendingStart(*psResFacilty, pResearch);  // Tell UI that we are going to research.
		}
		else
		{
			cancelResearch(psBuilding, ModeQueue);
		}
		//stop the button from flashing once a topic has been chosen
		stopReticuleButtonFlash(IDRET_RESEARCH);
		return true;
	}

	//initialise the subject
	psResFacilty->psSubject = NULL;

	//set up the player_research
	if (pResearch != NULL)
	{
		count = pResearch->ref - REF_RESEARCH_START;
		//meant to still be in the list but greyed out
		pPlayerRes = &asPlayerResList[selectedPlayer][count];

		//set the subject up
		psResFacilty->psSubject = pResearch;

		sendResearchStatus(psBuilding, count, selectedPlayer, true);	// inform others, I'm researching this.

		MakeResearchStarted(pPlayerRes);
		psResFacilty->timeStartHold = 0;
		//stop the button from flashing once a topic has been chosen
		stopReticuleButtonFlash(IDRET_RESEARCH);
	}
	return true;
}

/* Select a Factory */
static bool selectManufacture(BASE_OBJECT *psObj)
{
	ASSERT_OR_RETURN(false, psObj != NULL && psObj->type == OBJ_STRUCTURE, "Invalid Structure pointer");
	STRUCTURE *psBuilding = (STRUCTURE *)psObj;

	/* A Structure is a Factory if its type = REF_FACTORY or REF_CYBORG_FACTORY or
	REF_VTOL_FACTORY and it is completely built*/
	if ((psBuilding->pStructureType->type == REF_FACTORY ||
	     psBuilding->pStructureType->type == REF_CYBORG_FACTORY ||
	     psBuilding->pStructureType->type == REF_VTOL_FACTORY) &&
	    (psBuilding->status == SS_BUILT) && (psBuilding->died == 0))
	{
		return true;
	}

	return false;
}

/* Return the stats for a Factory */
static BASE_STATS *getManufactureStats(BASE_OBJECT *psObj)
{
	ASSERT_OR_RETURN(NULL, psObj != NULL && psObj->type == OBJ_STRUCTURE, "Invalid Structure pointer");
	STRUCTURE *psBuilding = (STRUCTURE *)psObj;

	return ((BASE_STATS *)((FACTORY *)psBuilding->pFunctionality)->psSubject);
}


/* Set the stats for a Factory */
static bool setManufactureStats(BASE_OBJECT *psObj, BASE_STATS *psStats)
{
	ASSERT_OR_RETURN(false, psObj != NULL && psObj->type == OBJ_STRUCTURE, "Invalid Structure pointer");
	/* psStats might be NULL if the operation is canceled in the middle */

	STRUCTURE *Structure = (STRUCTURE *)psObj;
	//check to see if the factory was already building something
	if (!((FACTORY *)Structure->pFunctionality)->psSubject)
	{
		//factory not currently building so set up the factory stats
		if (psStats != NULL)
		{
			/* Set the factory to build droid(s) */
			if (!structSetManufacture(Structure, (DROID_TEMPLATE *)psStats, ModeQueue))
			{
				return false;
			}
		}
	}

	return true;
}

bool human_computer_interface::addBuild(DROID *psSelected)
{
	/* Store the correct stats list for future reference */
	ppsStatsList = (BASE_STATS **)apsStructStatsList.data();

	objSelectFunc = selectConstruction;
	objGetStatsFunc = getConstructionStats;
	objSetStatsFunc = setConstructionStats;

	/* Set the sub mode */
	objMode = IOBJ_BUILD;

	/* Create the object screen with the required data */

	return addObjectWindow((BASE_OBJECT *)apsDroidLists[selectedPlayer],
	                          (BASE_OBJECT *)psSelected, true);
}

bool human_computer_interface::addManufacture(STRUCTURE *psSelected)
{
	/* Store the correct stats list for future reference */
	if (!apsTemplateList.empty())
	{
		ppsStatsList = (BASE_STATS **)&apsTemplateList[0]; // FIXME Ugly cast, and is undefined behaviour (strict-aliasing violation) in C/C++.
	}

	objSelectFunc = selectManufacture;
	objGetStatsFunc = getManufactureStats;
	objSetStatsFunc = setManufactureStats;

	/* Set the sub mode */
	objMode = IOBJ_MANUFACTURE;

	/* Create the object screen with the required data */
	return addObjectWindow((BASE_OBJECT *)interfaceStructList(),
	                          (BASE_OBJECT *)psSelected, true);
}

bool human_computer_interface::addResearch(STRUCTURE *psSelected)
{
	ppsStatsList = (BASE_STATS **)ppResearchList.data();

	objSelectFunc = selectResearch;
	objGetStatsFunc = getResearchStats;
	objSetStatsFunc = setResearchStats;

	/* Set the sub mode */
	objMode = IOBJ_RESEARCH;

	/* Create the object screen with the required data */
	return addObjectWindow((BASE_OBJECT *)interfaceStructList(),
	                          (BASE_OBJECT *)psSelected, true);
}

bool human_computer_interface::addCommand(DROID *psSelected)
{
	ppsStatsList = NULL;//(BASE_STATS **)ppResearchList;

	objSelectFunc = selectCommand;
	objGetStatsFunc = getCommandStats;
	objSetStatsFunc = setCommandStats;

	/* Set the sub mode */
	objMode = IOBJ_COMMAND;

	/* Create the object screen with the required data */
	return addObjectWindow((BASE_OBJECT *)apsDroidLists[selectedPlayer],
	                          (BASE_OBJECT *)psSelected, true);
}

void human_computer_interface::statsRMBPressed(uint32_t id)
{
	ASSERT_OR_RETURN(, id - IDSTAT_START < numStatsListEntries, "Invalid range referenced for numStatsListEntries, %d > %d", id - IDSTAT_START, numStatsListEntries);

	if (objMode == IOBJ_MANUFACTURE)
	{
		BASE_STATS *psStats = ppsStatsList[id - IDSTAT_START];

		//this now causes the production run to be decreased by one

		ASSERT_OR_RETURN(, psObjSelected != NULL, "Invalid structure pointer");
		ASSERT_OR_RETURN(, psStats != NULL, "Invalid template pointer");
		if (productionPlayer == (SBYTE)selectedPlayer)
		{
			STRUCTURE *psStructure = (STRUCTURE *)psObjSelected;
			FACTORY  *psFactory = &psStructure->pFunctionality->factory;
			DROID_TEMPLATE *psNext = (DROID_TEMPLATE *)psStats;

			//decrease the production
			factoryProdAdjust(psStructure, psNext, false);

			//need to check if this was the template that was mid-production
			if (getProduction(psStructure, FactoryGetTemplate(psFactory)).numRemaining() == 0)
			{
				doNextProduction(psStructure, FactoryGetTemplate(psFactory), ModeQueue);
				psNext = FactoryGetTemplate(psFactory);
			}
			else if (!StructureIsManufacturingPending(psStructure))
			{
				structSetManufacture(psStructure, psNext, ModeQueue);
			}

			if (StructureIsOnHoldPending(psStructure))
			{
				releaseProduction(psStructure, ModeQueue);
			}

			// Reset the button on the object form
			setStats(objStatID, (BASE_STATS *)psNext);
		}
	}
}

void human_computer_interface::objectRMBPressed(uint32_t id)
{
	BASE_OBJECT		*psObj;
	STRUCTURE		*psStructure;

	ASSERT_OR_RETURN(, id - IDOBJ_OBJSTART < apsObjectList.size(), "Invalid object id");

	/* Find the object that the ID refers to */
	psObj = getObject(id);
	if (psObj)
	{
		//don't jump around when offworld
		if (psObj->type == OBJ_STRUCTURE && !offWorldKeepLists)
		{
			psStructure = (STRUCTURE *)psObj;
			if (psStructure->pStructureType->type == REF_FACTORY ||
			    psStructure->pStructureType->type == REF_CYBORG_FACTORY ||
			    psStructure->pStructureType->type == REF_VTOL_FACTORY)
			{
				//centre the view on the delivery point
				setViewPos(map_coord(((FACTORY *)psStructure->pFunctionality)->psAssemblyPoint->coords.x),
				           map_coord(((FACTORY *)psStructure->pFunctionality)->psAssemblyPoint->coords.y),
				           true);
			}
		}
	}
}

void human_computer_interface::objStatRMBPressed(uint32_t id)
{
	BASE_OBJECT		*psObj;
	STRUCTURE		*psStructure;

	ASSERT_OR_RETURN(, id - IDOBJ_STATSTART < apsObjectList.size(), "Invalid stat id");

	/* Find the object that the ID refers to */
	psObj = getObject(id);
	if (!psObj)
	{
		return;
	}
	resetWindows(psObj);
	if (psObj->type == OBJ_STRUCTURE)
	{
		psStructure = (STRUCTURE *)psObj;
		if (StructIsFactory(psStructure))
		{
			//check if active
			if (StructureIsManufacturingPending(psStructure))
			{
				//if not curently on hold, set it
				if (!StructureIsOnHoldPending(psStructure))
				{
					holdProduction(psStructure, ModeQueue);
				}
				else
				{
					//cancel if have RMB-clicked twice
					cancelProduction(psStructure, ModeQueue);
					//play audio to indicate cancelled
					audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
				}
			}
		}
		else if (psStructure->pStructureType->type == REF_RESEARCH)
		{
			//check if active
			if (structureIsResearchingPending(psStructure))
			{
				//if not curently on hold, set it
				if (!StructureIsOnHoldPending(psStructure))
				{
					holdResearch(psStructure, ModeQueue);
				}
				else
				{
					//cancel if have RMB-clicked twice
					cancelResearch(psStructure, ModeQueue);
					//play audio to indicate cancelled
					audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
				}
			}
		}
	}
}


//sets up the Intelligence Screen as far as the interface is concerned
void human_computer_interface::addIntelScreen()
{
	bool	radOnScreen;

	if (driveModeActive() && !driveInterfaceEnabled())
	{
		driveDisableControl();
		driveEnableInterface(true);
	}

	intResetScreen(false);

	//lock the reticule button
	widgSetButtonState(psWScreen, IDRET_INTEL_MAP, WBUT_CLICKLOCK);

	//add the power bar - for looks!
	intShowPowerBar();

	// Only do this in main game.
	if ((GetGameMode() == GS_NORMAL) && !bMultiPlayer)
	{
		radOnScreen = radarOnScreen;

		bRender3DOnly = true;
		radarOnScreen = false;

		// Just display the 3d, no interface
		displayWorld();

		radarOnScreen = radOnScreen;
		bRender3DOnly = false;
	}

	//add all the intelligence screen interface
	(void)intAddIntelMap();
	intMode = INT_INTELMAP;
}

//sets up the Transporter Screen as far as the interface is concerned
void human_computer_interface::addTransporterInterface(DROID *psSelected, bool onMission)
{
	// if psSelected = NULL add interface but if psSelected != NULL make sure its not flying
	if (!psSelected || (psSelected && !transporterFlying(psSelected)))
	{
		intResetScreen(false);
		intAddTransporter(psSelected, onMission);
		intMode = INT_TRANSPORTER;
	}
}

/*sets which list of structures to use for the interface*/
STRUCTURE *human_computer_interface::interfaceStructList()
{
	if (offWorldKeepLists)
	{
		return mission.apsStructLists[selectedPlayer];
	}
	else
	{
		return apsStructLists[selectedPlayer];
	}
}

/*causes a reticule button to start flashing*/
void human_computer_interface::flashReticuleButton(uint32_t buttonID)
{
	//get the button for the id
	WIDGET *psButton = widgGetFromID(psWScreen, buttonID);
	if (psButton)
	{
		retbutstats[psButton->UserData].flashing = 1;
	}
}

// stop a reticule button flashing
void human_computer_interface::stopReticuleButtonFlash(uint32_t buttonID)
{
	WIDGET	*psButton = widgGetFromID(psWScreen, buttonID);
	if (psButton)
	{
		retbutstats[psButton->UserData].flashTime = 0;
		retbutstats[psButton->UserData].flashing = 0;
	}
}

//displays the Power Bar
void human_computer_interface::showPowerBar()
{
	//if its not already on display
	if (widgGetFromID(psWScreen, IDPOW_POWERBAR_T))
	{
		widgReveal(psWScreen, IDPOW_POWERBAR_T);
	}
}

//hides the power bar from the display - regardless of what player requested!
void human_computer_interface::forceHidePowerBar()
{
	if (widgGetFromID(psWScreen, IDPOW_POWERBAR_T))
	{
		widgHide(psWScreen, IDPOW_POWERBAR_T);
	}
}

/* Add the Proximity message buttons */
bool human_computer_interface::addProximityButton(PROXIMITY_DISPLAY *psProxDisp, uint32_t inc)
{
	PROXIMITY_DISPLAY	*psProxDisp2;
	UDWORD				cnt;

	W_FORMINIT sBFormInit;
	sBFormInit.formID = 0;
	sBFormInit.id = IDPROX_START + inc;
	//store the ID so we can detect which one has been clicked on
	psProxDisp->buttonID = sBFormInit.id;

	// loop back and find a free one!
	if (sBFormInit.id >= IDPROX_END)
	{
		for (cnt = IDPROX_START; cnt < IDPROX_END; cnt++)
		{
			// go down the prox msgs and see if it's free.
			for (psProxDisp2 = apsProxDisp[selectedPlayer]; psProxDisp2 && psProxDisp2->buttonID != cnt; psProxDisp2 = psProxDisp2->psNext) {}

			if (psProxDisp2 == NULL)	// value was unused.
			{
				sBFormInit.id = cnt;
				break;
			}
		}
		ASSERT_OR_RETURN(false, cnt != IDPROX_END, "Ran out of proximity displays");
	}
	ASSERT_OR_RETURN(false, sBFormInit.id < IDPROX_END, "Invalid proximity message button ID %d", (int)sBFormInit.id);

	sBFormInit.majorID = 0;
	sBFormInit.style = WFORM_CLICKABLE;
	sBFormInit.width = PROX_BUTWIDTH;
	sBFormInit.height = PROX_BUTHEIGHT;
	//the x and y need to be set up each time the button is drawn - see intDisplayProximityBlips

	sBFormInit.pDisplay = intDisplayProximityBlips;
	//set the data for this button
	sBFormInit.pUserData = psProxDisp;

	if (!widgAddForm(psWScreen, &sBFormInit))
	{
		return false;
	}
	return true;
}

/*Remove a Proximity Button - when the message is deleted*/
void human_computer_interface::removeProximityButton(PROXIMITY_DISPLAY *psProxDisp)
{
	ASSERT_OR_RETURN(, psProxDisp->buttonID >= IDPROX_START && psProxDisp->buttonID <= IDPROX_END, "Invalid proximity ID");
	widgDelete(psWScreen, psProxDisp->buttonID);
}

/*deals with the proximity message when clicked on*/
void processProximityButtons(UDWORD id)
{
	PROXIMITY_DISPLAY	*psProxDisp;

	if (!doWeDrawProximitys())
	{
		return;
	}

	//find which proximity display this relates to
	psProxDisp = NULL;
	for (psProxDisp = apsProxDisp[selectedPlayer]; psProxDisp; psProxDisp = psProxDisp->psNext)
	{
		if (psProxDisp->buttonID == id)
		{
			break;
		}
	}
	if (psProxDisp)
	{
		//if not been read - display info
		if (!psProxDisp->psMessage->read)
		{
			displayProximityMessage(psProxDisp);
		}
	}
}

/*	Fools the widgets by setting a key value */
void human_computer_interface::setKeyButtonMapping(uint32_t val)
{
	keyButtonMapping = val;
}

/*Looks through the players list of structures to see if there is one selected
of the required type. If there is more than one, they are all deselected and
the first one reselected*/
STRUCTURE *intCheckForStructure(UDWORD structType)
{
	STRUCTURE	*psStruct, *psSel = NULL;

	for (psStruct = interfaceStructList(); psStruct != NULL; psStruct = psStruct->psNext)
	{
		if (psStruct->selected && psStruct->pStructureType->type == structType && psStruct->status == SS_BUILT)
		{
			if (psSel != NULL)
			{
				clearSelection();
				psSel->selected = true;
				break;
			}
			psSel = psStruct;
		}
	}
	triggerEventSelected();
	return psSel;
}

/*Looks through the players list of droids to see if there is one selected
of the required type. If there is more than one, they are all deselected and
the first one reselected*/
// no longer do this for constructor droids - (gleeful its-near-the-end-of-the-project hack - JOHN)
DROID *intCheckForDroid(UDWORD droidType)
{
	DROID	*psDroid, *psSel = NULL;

	for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psDroid->psNext)
	{
		if (psDroid->selected && psDroid->droidType == droidType)
		{
			if (psSel != NULL)
			{
				if (droidType != DROID_CONSTRUCT
				    && droidType != DROID_CYBORG_CONSTRUCT)
				{
					clearSelection();
				}
				SelectDroid(psSel);
				break;
			}
			psSel = psDroid;
		}
	}

	return psSel;
}


// count the number of selected droids of a type
static SDWORD intNumSelectedDroids(UDWORD droidType)
{
	DROID	*psDroid;
	SDWORD	num;

	num = 0;
	for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected && psDroid->droidType == droidType)
		{
			num += 1;
		}
	}

	return num;
}

void human_computer_interface::checkReticuleButtons()
{
	STRUCTURE	*psStruct;
	DROID	*psDroid;

	ReticuleEnabled[RETBUT_CANCEL].Enabled = true;
	ReticuleEnabled[RETBUT_FACTORY].Enabled = false;
	ReticuleEnabled[RETBUT_RESEARCH].Enabled = false;
	ReticuleEnabled[RETBUT_BUILD].Enabled = false;
	ReticuleEnabled[RETBUT_DESIGN].Enabled = allowDesign;
	ReticuleEnabled[RETBUT_INTELMAP].Enabled = true;
	ReticuleEnabled[RETBUT_COMMAND].Enabled = false;

	for (psStruct = interfaceStructList(); psStruct != NULL; psStruct = psStruct->psNext)
	{
		if (psStruct->status == SS_BUILT)
		{
			switch (psStruct->pStructureType->type)
			{
			case REF_RESEARCH:
				if (!missionLimboExpand())
				{
					ReticuleEnabled[RETBUT_RESEARCH].Enabled = true;
				}
				break;
			case REF_FACTORY:
			case REF_CYBORG_FACTORY:
			case REF_VTOL_FACTORY:
				if (!missionLimboExpand())
				{
					ReticuleEnabled[RETBUT_FACTORY].Enabled = true;
				}
				break;
			default:
				break;
			}
		}
	}

	for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psDroid->psNext)
	{
		switch (psDroid->droidType)
		{
		case DROID_CONSTRUCT:
		case DROID_CYBORG_CONSTRUCT:
			ReticuleEnabled[RETBUT_BUILD].Enabled = true;
			break;
		case DROID_COMMAND:
			ReticuleEnabled[RETBUT_COMMAND].Enabled = true;
			break;
		default:
			break;
		}
	}

	for (int i = 0; i < NUMRETBUTS; i++)
	{
		WIDGET *psWidget = widgGetFromID(psWScreen, ReticuleEnabled[i].id);

		if (psWidget != NULL)
		{
			if (psWidget->type != WIDG_LABEL)
			{
				if (ReticuleEnabled[i].Enabled)
				{
					widgSetButtonState(psWScreen, ReticuleEnabled[i].id, 0);
				}
				else
				{
					widgSetButtonState(psWScreen, ReticuleEnabled[i].id, WBUT_DISABLE);
				}

				if (ReticuleEnabled[i].Hidden)
				{
					widgHide(psWScreen, ReticuleEnabled[i].id);
				}
				else
				{
					widgReveal(psWScreen, ReticuleEnabled[i].id);
				}
			}
		}
	}
}

/*Checks to see if there are any research topics to do and flashes the button -
only if research facility is free*/
int human_computer_interface::getResearchState()
{
	bool resFree = false;
	for (STRUCTURE *psStruct = interfaceStructList(); psStruct != NULL; psStruct = psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_RESEARCH &&
		    psStruct->status == SS_BUILT &&
		    getResearchStats(psStruct) == NULL)
		{
			resFree = true;
			break;
		}

	}

	int count = 0;
	if (resFree)
	{
		//set to value that won't be reached in fillResearchList
		int index = asResearch.size() + 1;
		//calculate the list
		int preCount = fillResearchList(pList.data(), selectedPlayer, index, MAXRESEARCH);
		count = preCount;
		for (int n = 0; n < preCount; ++n)
		{
			for (int player = 0; player < MAX_PLAYERS; ++player)
			{
				if (aiCheckAlliances(player, selectedPlayer) && IsResearchStarted(&asPlayerResList[player][pList[n]]))
				{
					--count;  // An ally is already researching this topic, so don't flash the button because of it.
					break;
				}
			}
		}
	}

	return count;
}

void human_computer_interface::notifyResearchButton(int prevState)
{
	int newState = intGetResearchState();
	if (newState > prevState)
	{
		// Set the research reticule button to flash.
		flashReticuleButton(IDRET_RESEARCH);
	}
	else if (newState == 0 && prevState > 0)
	{
		stopReticuleButtonFlash(IDRET_RESEARCH);
	}
}

// see if a reticule button is enabled
bool human_computer_interface::checkReticuleButEnabled(uint32_t id)
{
	for (int i = 0; i < NUMRETBUTS; i++)
	{
		if (ReticuleEnabled[i].id == id)
		{
			return ReticuleEnabled[i].Enabled;
		}
	}
	return false;
}

// Find any structure. Returns NULL if none found.
//
STRUCTURE *human_computer_interface::findAStructure()
{
	STRUCTURE *Struct;

	// First try and find a factory.
	Struct = intGotoNextStructureType(REF_FACTORY, false, false);
	if (Struct == NULL)
	{
		// If that fails then look for a command center.
		Struct = intGotoNextStructureType(REF_HQ, false, false);
		if (Struct == NULL)
		{
			// If that fails then look for a any structure.
			Struct = intGotoNextStructureType(REF_ANY, false, false);
		}
	}

	return Struct;
}

// Look through the players structures and find the next one of type structType.
//
STRUCTURE *human_computer_interface::gotoNextStructureType(uint32_t structType, bool JumpTo, bool CancelDrive)
{
	STRUCTURE	*psStruct;
	bool Found = false;

	if ((SWORD)structType != CurrentStructType)
	{
		CurrentStruct = NULL;
		CurrentStructType = (SWORD)structType;
	}

	if (CurrentStruct != NULL)
	{
		psStruct = CurrentStruct;
	}
	else
	{
		psStruct = interfaceStructList();
	}

	for (; psStruct != NULL; psStruct = psStruct->psNext)
	{
		if ((psStruct->pStructureType->type == structType || structType == REF_ANY) && psStruct->status == SS_BUILT)
		{
			if (psStruct != CurrentStruct)
			{
				if (CancelDrive)
				{
					clearSelection();
				}
				else
				{
					clearSel();
				}
				psStruct->selected = true;
				CurrentStruct = psStruct;
				Found = true;
				break;
			}
		}
	}

	// Start back at the begining?
	if ((!Found) && (CurrentStruct != NULL))
	{
		for (psStruct = interfaceStructList(); psStruct != CurrentStruct && psStruct != NULL; psStruct = psStruct->psNext)
		{
			if ((psStruct->pStructureType->type == structType || structType == REF_ANY) && psStruct->status == SS_BUILT)
			{
				if (psStruct != CurrentStruct)
				{
					if (CancelDrive)
					{
						clearSelection();
					}
					else
					{
						clearSel();
					}
					psStruct->selected = true;
					CurrentStruct = psStruct;
					break;
				}
			}
		}
	}

	// Center it on screen.
	if ((CurrentStruct) && (JumpTo))
	{
		intSetMapPos(CurrentStruct->pos.x, CurrentStruct->pos.y);
	}

	triggerEventSelected();

	return CurrentStruct;
}

// Look through the players droids and find the next one of type droidType.
// If Current=NULL then start at the beginning overwise start at Current.
//
DROID *human_computer_interface::gotoNextDroidType(DROID *CurrDroid, DROID_TYPE droidType, bool AllowGroup)
{
	DROID *psDroid;
	bool Found = false;

	if (CurrDroid != NULL)
	{
		CurrentDroid = CurrDroid;
	}

	if (droidType != CurrentDroidType && droidType != DROID_ANY)
	{
		CurrentDroid = NULL;
		CurrentDroidType = droidType;
	}

	if (CurrentDroid != NULL)
	{
		psDroid = CurrentDroid;
	}
	else
	{
		psDroid = apsDroidLists[selectedPlayer];
	}

	for (; psDroid != NULL; psDroid = psDroid->psNext)
	{
		if ((psDroid->droidType == droidType
		     || (droidType == DROID_ANY && !isTransporter(psDroid)))
		    && (psDroid->group == UBYTE_MAX || AllowGroup))
		{
			if (psDroid != CurrentDroid)
			{
				clearSel();
				SelectDroid(psDroid);
				CurrentDroid = psDroid;
				Found = true;
				break;
			}
		}
	}

	// Start back at the begining?
	if ((!Found) && (CurrentDroid != NULL))
	{
		for (psDroid = apsDroidLists[selectedPlayer]; (psDroid != CurrentDroid) && (psDroid != NULL); psDroid = psDroid->psNext)
		{
			if ((psDroid->droidType == droidType ||
			     ((droidType == DROID_ANY) && !isTransporter(psDroid))) &&
			    ((psDroid->group == UBYTE_MAX) || AllowGroup))
			{
				if (psDroid != CurrentDroid)
				{
					clearSel();
					SelectDroid(psDroid);
					CurrentDroid = psDroid;
					Found = true;
					break;
				}
			}
		}
	}

	if (Found == true)
	{
		psCBSelectedDroid = CurrentDroid;
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DROID_SELECTED);
		psCBSelectedDroid = NULL;

		// Center it on screen.
		if (CurrentDroid)
		{
			intSetMapPos(CurrentDroid->pos.x, CurrentDroid->pos.y);
		}
		return CurrentDroid;
	}
	return NULL;
}

//access function for selected object in the interface
BASE_OBJECT *human_computer_interface::getCurrentSelected()
{
	return psObjSelected;
}

// Checks if a coordinate is over the build menu
bool human_computer_interface::coordInBuild(int x, int y)
{
	// This measurement is valid for the menu, so the buildmenu_height
	// value is used to "nudge" it all upwards from the command menu.
	// FIXME: hardcoded value (?)
	const int buildmenu_height = 300;
	Vector2f pos;

	pos.x = x - RET_X;
	pos.y = y - RET_Y + buildmenu_height; // guesstimation

	if ((pos.x < 0 || pos.y < 0 || pos.x >= RET_FORMWIDTH || pos.y >= buildmenu_height) || !secondaryWindowUp)
	{
		return false;
	}

	return true;
}

// Our chat dialog for global & team communication
// \mode sets if global or team communication is wanted
void human_computer_interface::chatDialog(int mode)
{
	if (!ChatDialogUp)
	{
		WIDGET *parent = psWScreen->psForm;
		static IntFormAnimated *consoleBox = NULL;
		W_EDITBOX *chatBox = NULL;
		W_CONTEXT sContext;

		consoleBox = new IntFormAnimated(parent);
		consoleBox->id = CHAT_CONSOLEBOX;
		consoleBox->setGeometry(CHAT_CONSOLEBOXX, CHAT_CONSOLEBOXY, CHAT_CONSOLEBOXW, CHAT_CONSOLEBOXH);

		chatBox = new W_EDITBOX(consoleBox);
		chatBox->id = CHAT_EDITBOX;
		chatBox->setGeometry(80, 2, 320, 16);
		if (mode == CHAT_GLOB)
		{
			chatBox->setBoxColours(WZCOL_MENU_BORDER, WZCOL_MENU_BORDER, WZCOL_MENU_BACKGROUND);
			widgSetUserData2(psWScreen, CHAT_EDITBOX, CHAT_GLOB);
		}
		else
		{
			chatBox->setBoxColours(WZCOL_YELLOW, WZCOL_YELLOW, WZCOL_MENU_BACKGROUND);
			widgSetUserData2(psWScreen, CHAT_EDITBOX, CHAT_TEAM);
		}
		sContext.xOffset = sContext.yOffset = 0;
		sContext.mx = sContext.my = 0;

		W_LABEL *label = new W_LABEL(consoleBox);
		label->setGeometry(2, 2,60, 16);
		if (mode == CHAT_GLOB)
		{
			label->setFontColour(WZCOL_TEXT_BRIGHT);
			label->setString(_("Chat: All"));
		}
		else
		{
			label->setFontColour(WZCOL_YELLOW);
			label->setString(_("Chat: Team"));
		}
		label->setTextAlignment(WLAB_ALIGNTOPLEFT);
		ChatDialogUp = true;
		// Auto-click it
		widgGetFromID(psWScreen, CHAT_EDITBOX)->clicked(&sContext);
	}
	else
	{
		debug(LOG_ERROR, "Tried to throw up chat dialog when we already had one up!");
	}
}

// If chat dialog is up
bool human_computer_interface::isChatUp()
{
	return ChatDialogUp;
}

// Helper call to see if we have builder/research/... window up or not.
bool human_computer_interface::isSecondaryWindowUp()
{
	return secondaryWindowUp;
}

/* Initialise the in game interface */
bool intInitialise(void)
{
	default_hci.reset(new human_computer_interface);
	return true;
}

/* Shut down the in game interface */
void interfaceShutDown(void)
{
	default_hci.release();
}

void intResetPreviousObj(void)
{
	default_hci->resetPreviousObj();
}

void intRefreshScreen(void)
{
	default_hci->refreshScreen();
}

bool intIsRefreshing(void)
{
	return default_hci->isRefreshing();
}

void intHidePowerBar()
{
	default_hci->hidePowerBar();
}

void intResetScreen(bool NoAnim)
{
	default_hci->resetScreen(NoAnim);
}

void intNotifyResearchButton(int prevState)
{
	default_hci->notifyResearchButton(prevState);
}

bool intCheckReticuleButEnabled(UDWORD id)
{
	return default_hci->checkReticuleButEnabled(id);
}

void hciUpdate()
{
	default_hci->update();
}

INT_RETVAL intRunWidgets(void)
{
	return default_hci->display();
}

void intSetMapPos(UDWORD x, UDWORD y)
{
	default_hci->setMapPos(x, y);
}
void intObjectSelected(BASE_OBJECT *psObj)
{
	default_hci->objectSelected(psObj);
}

void intConstructorSelected(DROID *psDroid)
{
	default_hci->constructorSelected(psDroid);
}

void intCommanderSelected(DROID *psDroid)
{
	default_hci->commanderSelected(psDroid);
}

void intNewObj(BASE_OBJECT *psObj)
{
	default_hci->notifyNewObject(psObj);
}

void intBuildFinished(DROID *psDroid)
{
	default_hci->notifyBuildFinished(psDroid);
}

void intBuildStarted(DROID *psDroid)
{
	default_hci->notifyBuildStarted(psDroid);
}

bool intBuildSelectMode(void)
{
	return default_hci->buildSelectMode();
}

bool intDemolishSelectMode(void)
{
	return default_hci->demolishSelectMode();
}

bool intBuildMode(void)
{
	return default_hci->buildMode();
}

void intDemolishCancel(void)
{
	default_hci->demolishCancel();
}

void intManufactureFinished(STRUCTURE *psBuilding)
{
	default_hci->notifyManufactureFinished(psBuilding);
}

void intUpdateManufacture(STRUCTURE *psBuilding)
{
	default_hci->updateManufacture(psBuilding);
}

void intResearchFinished(STRUCTURE *psBuilding)
{
	default_hci->notifyResearchFinished(psBuilding);
}

bool intAddReticule()
{
	return default_hci->addReticule();
}

void intRemoveReticule(void)
{
	default_hci->removeReticule();
}

void togglePowerBar(void)
{
	default_hci->togglePowerBar();
}

bool intAddPower()
{
	return default_hci->addPower();
}

bool intAddOptions(void)
{
	return default_hci->addOptions();
}

void intRemoveStats(void)
{
	default_hci->removeStats();
}

void intRemoveStatsNoAnim(void)
{
	default_hci->removeStatsNoAnim();
}

StateButton *makeObsoleteButton(WIDGET *parent)
{
	return default_hci->makeObsoleteButton(parent);
}

void addIntelScreen(void)
{
	default_hci->addIntelScreen();
}

void addTransporterInterface(DROID *psSelected, bool onMission)
{
	default_hci->addTransporterInterface(psSelected, onMission);
}

STRUCTURE *interfaceStructList(void)
{
	return default_hci->interfaceStructList();
}

void flashReticuleButton(UDWORD buttonID)
{
	default_hci->flashReticuleButton(buttonID);
}

void stopReticuleButtonFlash(UDWORD buttonID)
{
	default_hci->stopReticuleButtonFlash(buttonID);
}

void intShowPowerBar(void)
{
	default_hci->showPowerBar();
}

void forceHidePowerBar(void)
{
	default_hci->forceHidePowerBar();
}

bool intAddProximityButton(PROXIMITY_DISPLAY *psProxDisp, UDWORD inc)
{
	return default_hci->addProximityButton(psProxDisp, inc);
}

void intRemoveProximityButton(PROXIMITY_DISPLAY *psProxDisp)
{
	default_hci->removeProximityButton(psProxDisp);
}

void	setKeyButtonMapping(UDWORD	val)
{
	default_hci->setKeyButtonMapping(val);
}

int intGetResearchState()
{
	return default_hci->getResearchState();
}

STRUCTURE *intFindAStructure(void)
{
	return default_hci->findAStructure();
}

STRUCTURE *intGotoNextStructureType(UDWORD structType, bool JumpTo, bool CancelDrive)
{
	return default_hci->gotoNextStructureType(structType, JumpTo, CancelDrive);
}

DROID *intGotoNextDroidType(DROID *CurrDroid, DROID_TYPE droidType, bool AllowGroup)
{
	return default_hci->gotoNextDroidType(CurrDroid, droidType, AllowGroup);
}

BASE_OBJECT *getCurrentSelected(void)
{
	return default_hci->getCurrentSelected();
}

void chatDialog(int mode)
{
	default_hci->chatDialog(mode);
}

bool isChatUp(void)
{
	return default_hci->isChatUp();
}

bool isSecondaryWindowUp(void)
{
	return default_hci->isSecondaryWindowUp();
}

bool CoordInBuild(int x, int y)
{
	return default_hci->coordInBuild(x, y);
}
