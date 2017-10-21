#include <map>
#include "netcore/mscoree.h"

struct NetCoreScript
{
	NetCoreScript(ICLRRuntimeHost2* runtimeHost, const DWORD& domainId, const std::wstring& ScriptName);

	typedef void(*void_to_void)(void);

	void_to_void eventGameInit;
	void_to_void eventStartLevel;
	void_to_void eventMissionTimeout;
	void_to_void eventVideoDone;
	void_to_void eventGameLoaded;
	void_to_void eventGameSaving;
	void_to_void eventGameSaved;
};

struct NetCoreRuntime
{
	HMODULE core_clr_module;
	ICLRRuntimeHost2* runtimeHost;
	DWORD domainId;

/*			case TRIGGER_TRANSPORTER_LAUNCH:
				callFunction(engine, "eventLaunchTransporter", QScriptValueList()); // deprecated!
				callFunction(engine, "eventTransporterLaunch", args);
				break;
			case TRIGGER_TRANSPORTER_ARRIVED:
				callFunction(engine, "eventReinforcementsArrived", QScriptValueList()); // deprecated!
				callFunction(engine, "eventTransporterArrived", args);
				break;
			case TRIGGER_OBJECT_RECYCLED:
				callFunction(engine, "eventObjectRecycled", args);
				break;
			case TRIGGER_TRANSPORTER_EXIT:
				callFunction(engine, "eventTransporterExit", args);
				break;
			case TRIGGER_TRANSPORTER_DONE:
				callFunction(engine, "eventTransporterDone", args);
				break;
			case TRIGGER_TRANSPORTER_LANDED:
				callFunction(engine, "eventTransporterLanded", args);
				break;*/

	NetCoreRuntime();
	~NetCoreRuntime();
private:

	std::map<std::wstring, std::wstring> createAppDomain(const std::wstring& corepath, const std::wstring& targetAppPath);
};

