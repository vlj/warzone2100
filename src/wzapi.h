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

	struct droid_id_player
	{
		int id;
		int player;
	};

	struct string_list
	{
		const char** strings;
		size_t count;
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
	//-- \subsection{enableComponent(component, player)}
	//-- The given component is made available for research for the given player.
	bool enableComponent(const char* componentName, int player);
	//-- \subsection{makeComponentAvailable(component, player)}
	//-- The given component is made available to the given player. This means the player can
	//-- actually build designs with it.
	bool makeComponentAvailable(const char* componentName, int player);
	//-- \subsection{allianceExistsBetween(player, player)}
	//-- Returns true if an alliance exists between the two players, or they are the same player.
	bool allianceExistsBetween(int player1, int player2);
	//-- \subsection{_(string)}
	//-- Mark string for translation.
	const char* translate(const char* txt);
	//-- \subsection{safeDest(player, x, y)} Returns true if given player is safe from hostile fire at
	//-- the given location, to the best of that player's map knowledge.
	bool safeDest(int player, int x, int y);
	//-- \subsection{setNoGoArea(x1, y1, x2, y2, player)}
	//-- Creates an area on the map on which nothing can be built. If player is zero,
	//-- then landing lights are placed. If player is -1, then a limbo landing zone
	//-- is created and limbo droids placed.
	// FIXME: missing a way to call initNoGoAreas(); check if we can call this in
	// every level start instead of through scripts
	bool _setNoGoArea(int x1, int y1, int x2, int y2, int player);
	//-- \subsection{setScrollLimits(x1, y1, x2, y2)}
	//-- Limit the scrollable area of the map to the given rectangle. (3.2+ only)
	bool setScrollLimits(int minX, int minY, int maxX, int maxY);
	//-- \subsection{loadLevel(level name)}
	//-- Load the level with the given name.
	bool loadLevel(const char* level);
	//-- \subsection{setAlliance(player1, player2, value)}
	//-- Set alliance status between two players to either true or false. (3.2+ only)
	bool setAlliance(int player1, int player2, bool value);
	//-- \subsection{getExperienceModifier(player)}
	//-- Get the percentage of experience this player droids are going to gain. (3.2+ only)
	int getExperienceModifier(int player);
	//-- \subsection{setExperienceModifier(player, percent)}
	//-- Set the percentage of experience this player droids are going to gain. (3.2+ only)
	bool setExperienceModifier(int player, int percent);
	//-- \subsection{setSunPosition(x, y, z)}
	//-- Move the position of the Sun, which in turn moves where shadows are cast. (3.2+ only)
	bool setSunPosition(float x, float y, float z);
	//-- \subsection{setSunIntensity(ambient r, g, b, diffuse r, g, b, specular r, g, b)}
	//-- Set the ambient, diffuse and specular colour intensities of the Sun lighting source. (3.2+ only)
	bool setSunIntensity(float ambient_r, float ambient_g, float ambient_b, float diffuse_r, float diffuse_g, float diffuse_b, float specular_r, float specular_g, float specular_b);
	//-- \subsection{setWeather(weather type)}
	//-- Set the current weather. This should be one of WEATHER_RAIN, WEATHER_SNOW or WEATHER_CLEAR. (3.2+ only)
	bool setWeather(int _weather);
	//-- \subsection{setSky(texture file, wind speed, skybox scale)}
	//-- Change the skybox. (3.2+ only)
	bool setSky(const char* page, float wind, float scale);
	//-- \subsection{cameraSlide(x, y)}
	//-- Slide the camera over to the given position on the map. (3.2+ only)
	bool cameraSlide(float x, float y);
	//-- \subsection{cameraZoom(z, speed)}
	//-- Slide the camera to the given zoom distance. Normal camera zoom ranges between 500 and 5000. (3.2+ only)
	bool cameraZoom(float z, float speed);
	//-- \subsection{addSpotter(x, y, player, range, type, expiry)}
	//-- Add an invisible viewer at a given position for given player that shows map in given range. \emph{type}
	//-- is zero for vision reveal, or one for radar reveal. The difference is that a radar reveal can be obstructed
	//-- by ECM jammers. \emph{expiry}, if non-zero, is the game time at which the spotter shall automatically be
	//-- removed. The function returns a unique ID that can be used to remove the spotter with \emph{removeSpotter}. (3.2+ only)
	unsigned int _addSpotter(int x, int y, int player, int range, bool radar, unsigned int expiry);
	//-- \subsection{removeSpotter(id)}
	//-- Remove a spotter given its unique ID. (3.2+ only)
	bool _removeSpotter(unsigned int id);
	//-- \subsection{syncRandom(limit)}
	//-- Generate a synchronized random number in range 0...(limit - 1) that will be the same if this function is
	//-- run on all network peers in the same game frame. If it is called on just one peer (such as would be
	//-- the case for AIs, for instance), then game sync will break. (3.2+ only)
	int syncRandom(unsigned int limit);
	//-- \subsection{replaceTexture(old_filename, new_filename)}
	//-- Replace one texture with another. This can be used to for example give buildings on a specific tileset different
	//-- looks, or to add variety to the looks of droids in campaign missions. (3.2+ only)
	bool _replaceTexture(const char* oldfile, const char* newfile);
	//-- \subsection{fireWeaponAtLoc(x, y, weapon_name)}
	//-- Fires a weapon at the given coordinates (3.2.4+ only).
	bool fireWeaponAtLoc(int xLocation, int yLocation, const char *weaponName);
	//-- \subsection{changePlayerColour(player, colour)}
	//-- Change a player's colour slot. The current player colour can be read from the playerData array. There are as many
	//-- colour slots as the maximum number of players. (3.2.3+ only)
	bool changePlayerColour(int player, int colour);
	//-- \subsection{pursueResearch(lab, research)}
	//-- Start researching the first available technology on the way to the given technology.
	//-- First parameter is the structure to research in, which must be a research lab. The
	//-- second parameter is the technology to pursue, as a text string as defined in "research.json".
	//-- The second parameter may also be an array of such strings. The first technology that has
	//-- not yet been researched in that list will be pursued.
	bool pursueResearch(structure_id_player structVal, string_list list);
	//-- \subsection{addFeature(name, x, y)}
	//-- Create and place a feature at the given x, y position. Will cause a desync in multiplayer.
	//-- Returns the created game object on success, null otherwise. (3.2+ only)
	FEATURE* _addFeature(const char* featName, int x, int y);
	//-- \subsection{addDroidToTransporter(transporter, droid)}
	//-- Load a droid, which is currently located on the campaign off-world mission list,
	//-- into a transporter, which is also currently on the campaign off-world mission list.
	//-- (3.2+ only)
	bool addDroidToTransporter(droid_id_player transporter, droid_id_player droid);
}
