#include "droid.h"

extern "C" {

	struct object_id_player_type
	{
		int id;
		int player;
		OBJECT_TYPE type;
	};

	struct structure_id_player
	{
		int id;
		int player;
	};

	bool activateStructure(structure_id_player structVal, object_id_player_type objVal);
	//-- \subsection{structureIdle(structure)}
	//-- Is given structure idle?
	bool structureIdle_(structure_id_player structVal);
	//-- \subsection{removeStruct(structure)}
	//-- Immediately remove the given structure from the map. Returns a boolean that is true on success.
	//-- No special effects are applied. Deprecated since 3.2.
	bool removeStruct_(structure_id_player structVal);
	//-- \subsection{removeObject(game object[, special effects?])}
	//-- Remove the given game object with special effects. Returns a boolean that is true on success.
	//-- A second, optional boolean parameter specifies whether special effects are to be applied. (3.2+ only)
	bool removeObject(object_id_player_type qval, bool sfx);
	float distBetweenTwoPoints(float x1, float y1, float x2, float y2);
	//-- \subsection{droidCanReach(droid, x, y)}
	//-- Return whether or not the given droid could possibly drive to the given position. Does
	//-- not take player built blockades into account.
	bool droidCanReach(DROID* psDroid, int x, int y);
	//-- \subsection{propulsionCanReach(propulsion, x1, y1, x2, y2)}
	//-- Return true if a droid with a given propulsion is able to travel from (x1, y1) to (x2, y2).
	//-- Does not take player built blockades into account. (3.2+ only)
	bool propulsionCanReach(const char* propulsionValueName, int x1, int y1, int x2, int y2);
	//-- \subsection{terrainType(x, y)}
	//-- Returns tile type of a given map tile, such as TER_WATER for water tiles or TER_CLIFFFACE for cliffs.
	//-- Tile types regulate which units may pass through this tile. (3.2+ only)
	unsigned char terrainType_(int x, int y);
	bool orderDroid_(DROID* psDroid, int _order);
	//-- \subsection{orderDroidObj(droid, order, object)}
	//-- Give a droid an order to do something to something.
	bool orderDroidObj_(DROID* psDroid, int order_, object_id_player_type objVal);
	bool orderDroidBuild5(DROID* psDroid, int _order, const char* statName, int x, int y, float _direction);
	bool orderDroidBuild4(DROID* psDroid, int _order, const char* statName, int x, int y);
	//-- \subsection{orderDroidLoc(droid, order, x, y)}
	//-- Give a droid an order to do something at the given location.
	bool orderDroidLoc_(DROID* psDroid, int order_, int x, int y);
	//-- \subsection{setMissionTime(time)} Set mission countdown in seconds.
	bool SetMissionTime(int time);
	//-- \subsection{getMissionTime()} Get time remaining on mission countdown in seconds. (3.2+ only)
	int getMissionTime();
	//-- \subsection{setTransporterExit(x, y, player)}
	//-- Set the exit position for the mission transporter. (3.2+ only)
	bool setTransporterExit(int x, int y, int player);
	//-- \subsection{startTransporterEntry(x, y, player)}
	//-- Set the entry position for the mission transporter, and make it start flying in
	//-- reinforcements. If you want the camera to follow it in, use cameraTrack() on it.
	//-- The transport needs to be set up with the mission droids, and the first transport
	//-- found will be used. (3.2+ only)
	bool startTransporterEntry(int x, int y, int player);
	//-- \subsection{useSafetyTransport(flag)}
	//-- Change if the mission transporter will fetch droids in non offworld missions
	//-- setReinforcementTime() is be used to hide it before coming back after the set time
	//-- which is handled by the campaign library in the victory data section (3.2.4+ only).
	bool useSafetyTransport(bool flag);
	//-- \subsection{restoreLimboMissionData()}
	//-- Swap mission type and bring back units previously stored at the start
	//-- of the mission (see cam3-c mission). (3.2.4+ only).
	bool restoreLimboMissionData();
	//-- \subsection{setReinforcementTime(time)} Set time for reinforcements to arrive. If time is
	//-- negative, the reinforcement GUI is removed and the timer stopped. Time is in seconds.
	//-- If time equals to the magic LZ_COMPROMISED_TIME constant, reinforcement GUI ticker
	//-- is set to "--:--" and reinforcements are suppressed until this function is called
	//-- again with a regular time value.
	bool setReinforcementTime(int time);
	//-- \subsection{centreView(x, y)}
	//-- Center the player's camera at the given position.
	bool centreView(int x, int y);
	//-- \subsection{setTutorialMode(bool)} Sets a number of restrictions appropriate for tutorial if set to true.
	bool setTutorialMode(bool intutorial);
	//-- \subsection{setMiniMap(bool)} Turns visible minimap on or off in the GUI.
	bool setMiniMap(bool radarpermitted);
	//-- \subsection{setDesign(bool)} Whether to allow player to design stuff.
	bool setDesign(bool allowdesign);
	//-- \subsection{enableTemplate(template name)} Enable a specific template (even if design is disabled).
	bool enableTemplate(const char* templateName);
	//-- \subsection{removeTemplate(template name)} Remove a template.
	bool removeTemplate(const char* templateName);
	bool setDroidExperience(DROID* psDroid, float exp);
	bool setAssemblyPoint_(structure_id_player structVal, int x, int y);
}
