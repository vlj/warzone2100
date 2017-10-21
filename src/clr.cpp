#include "clr.h"
#include "wzapi.h"
#include "netcore/mscoree.h"
#include <memory>

NetCoreRuntime::NetCoreRuntime()
{
	std::string dll = "C:\\Program Files (x86)\\dotnet\\shared\\Microsoft.NETCore.App\\2.0.0\\coreclr.dll";
	core_clr_module = LoadLibraryA(dll.data());

	FnGetCLRRuntimeHost pfnGetCLRRuntimeHost =
		(FnGetCLRRuntimeHost)::GetProcAddress(core_clr_module, "GetCLRRuntimeHost");

	if (!pfnGetCLRRuntimeHost)
	{
		printf("ERROR - GetCLRRuntimeHost not found");
	}

	// Get the hosting interface
	HRESULT hr = pfnGetCLRRuntimeHost(IID_ICLRRuntimeHost2, (IUnknown**)&runtimeHost);
	hr = runtimeHost->SetStartupFlags(
		// These startup flags control runtime-wide behaviors.
		// A complete list of STARTUP_FLAGS can be found in mscoree.h,
		// but some of the more common ones are listed below.
		static_cast<STARTUP_FLAGS>(
			// STARTUP_FLAGS::STARTUP_SERVER_GC |								// Use server GC
			// STARTUP_FLAGS::STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN |		// Maximize domain-neutral loading
			// STARTUP_FLAGS::STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST |	// Domain-neutral loading for strongly-named assemblies
			STARTUP_FLAGS::STARTUP_CONCURRENT_GC |						// Use concurrent GC
			STARTUP_FLAGS::STARTUP_SINGLE_APPDOMAIN |					// All code executes in the default AppDomain 
																		// (required to use the runtimeHost->ExecuteAssembly helper function)
			STARTUP_FLAGS::STARTUP_LOADER_OPTIMIZATION_SINGLE_DOMAIN	// Prevents domain-neutral loading
			)
	);
	hr = runtimeHost->Start();
	int appDomainFlags =
		// APPDOMAIN_FORCE_TRIVIAL_WAIT_OPERATIONS |		// Do not pump messages during wait
		// APPDOMAIN_SECURITY_SANDBOXED |					// Causes assemblies not from the TPA list to be loaded as partially trusted
		APPDOMAIN_ENABLE_PLATFORM_SPECIFIC_APPS |			// Enable platform-specific assemblies to run
		APPDOMAIN_ENABLE_PINVOKE_AND_CLASSIC_COMINTEROP |	// Allow PInvoking from non-TPA assemblies
		APPDOMAIN_DISABLE_TRANSPARENCY_ENFORCEMENT;			// Entirely disables transparency checks 

															// TRUSTED_PLATFORM_ASSEMBLIES
															// "Trusted Platform Assemblies" are prioritized by the loader and always loaded with full trust.
															// A common pattern is to include any assemblies next to CoreCLR.dll as platform assemblies.
															// More sophisticated hosts may also include their own Framework extensions (such as AppDomain managers)
															// in this list.

	const auto& properties = createAppDomain(L"", L"");

	auto&& propertyKeys = std::unique_ptr<const wchar_t*[]>(new const wchar_t*[properties.size()]);
	auto&& propertyValues = std::unique_ptr<const wchar_t*[]>(new const wchar_t*[properties.size()]);

	size_t i = 0;
	for (const auto& key_value : properties)
	{
		propertyKeys[i] = key_value.first.data();
		propertyValues[i] = key_value.second.data();
		i++;
	}

	// Create the AppDomain
	hr = runtimeHost->CreateAppDomainWithManager(
		L"Sample Host AppDomain",		// Friendly AD name
		appDomainFlags,
		NULL,							// Optional AppDomain manager assembly name
		NULL,							// Optional AppDomain manager type (including namespace)
		properties.size(),
		propertyKeys.get(),
		propertyValues.get(),
		&domainId);

	void *pfnDelegate = NULL;
	hr = runtimeHost->CreateDelegate(
		domainId,
		L"HW, Version=1.0.0.0, Culture=neutral",  // Target managed assembly
		L"ConsoleApplication.Program",            // Target managed type
		L"Main",                                  // Target entry point (static method)
		(INT_PTR*)&pfnDelegate);

	//((MainMethodFp*)pfnDelegate)(NULL);
}

NetCoreRuntime::~NetCoreRuntime()
{
	runtimeHost->UnloadAppDomain(domainId, true /* Wait until unload complete */);
	runtimeHost->Stop();
	runtimeHost->Release();
}

std::map<std::wstring, std::wstring> NetCoreRuntime::createAppDomain(const std::wstring& corepath, const std::wstring& targetAppPath)
{
	// Probe next to CoreCLR.dll for any files matching the extensions from tpaExtensions and
	// add them to the TPA list. In a real host, this would likely be extracted into a separate function
	// and perhaps also run on other directories of interest.
	std::wstring trustedPlatformAssemblies;

	// Extensions to probe for when finding TPA list files
	std::wstring tpaExtensions[] = {
		L"*.dll",
		L"*.exe",
		L"*.winmd"
	};
	for (const auto& extension : tpaExtensions)
	{
		// Construct the file name search pattern
		const std::wstring& searchPath = corepath + L"\\" + extension;

		// Find files matching the search pattern
		WIN32_FIND_DATAW findData;
		HANDLE fileHandle = FindFirstFileW(searchPath.data(), &findData);

		if (fileHandle == INVALID_HANDLE_VALUE)
			continue;
		do
		{
			trustedPlatformAssemblies += corepath + L"\\" + findData.cFileName + L";";
		} while (FindNextFileW(fileHandle, &findData));
		FindClose(fileHandle);
	}


	// APP_PATHS
	// App paths are directories to probe in for assemblies which are not one of the well-known Framework assemblies
	// included in the TPA list.
	std::wstring appPaths = targetAppPath;

	// APP_NI_PATHS
	// App (NI) paths are the paths that will be probed for native images not found on the TPA list.
	// It will typically be similar to the app paths.
	const std::wstring& appNiPaths = targetAppPath + L";";


	// NATIVE_DLL_SEARCH_DIRECTORIES
	// Native dll search directories are paths that the runtime will probe for native DLLs called via PInvoke
	const std::wstring& nativeDllSearchDirectories = targetAppPath + L";" + corepath;


	// PLATFORM_RESOURCE_ROOTS
	// Platform resource roots are paths to probe in for resource assemblies (in culture-specific sub-directories)
	const std::wstring& platformResourceRoots = targetAppPath;

	// AppDomainCompatSwitch
	// Specifies compatibility behavior for the app domain. This indicates which compatibility
	// quirks to apply if an assembly doesn't have an explicit Target Framework Moniker. If a TFM is
	// present on an assembly, the runtime will always attempt to use quirks appropriate to the version
	// of the TFM.
	// 
	// Typically the latest behavior is desired, but some hosts may want to default to older Silverlight
	// or Windows Phone behaviors for compatibility reasons.
	const std::wstring& appDomainCompatSwitch = L"UseLatestBehaviorWhenTFMNotSpecified";

	return {
		std::make_pair(L"TRUSTED_PLATFORM_ASSEMBLIES", trustedPlatformAssemblies),
		std::make_pair(L"APP_PATHS", appPaths),
		std::make_pair(L"APP_NI_PATHS", appNiPaths),
		std::make_pair(L"NATIVE_DLL_SEARCH_DIRECTORIES", nativeDllSearchDirectories),
		std::make_pair(L"PLATFORM_RESOURCE_ROOTS", platformResourceRoots),
		std::make_pair(L"AppDomainCompatSwitch", appDomainCompatSwitch)
	};
}



