#include <map>
#include "netcore/mscoree.h"

struct NetCoreRuntime
{
	HMODULE core_clr_module;
	ICLRRuntimeHost2* runtimeHost;
	DWORD domainId;

	NetCoreRuntime();

	~NetCoreRuntime();
private:

	std::map<std::wstring, std::wstring> createAppDomain(const std::wstring& corepath, const std::wstring& targetAppPath);
};

