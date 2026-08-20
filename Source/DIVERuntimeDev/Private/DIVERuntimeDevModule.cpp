// Copyright (c) 2026. All Rights Reserved.

#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "Debug/DIVEDebugDump.h"
#endif

class FDIVERuntimeDevModule : public IModuleInterface
{
public:
	void StartupModule() override
	{
#if WITH_EDITOR
		DIVEDebugDump::RegisterConsoleCommands();
#endif
	}

	void ShutdownModule() override
	{
#if WITH_EDITOR
		DIVEDebugDump::UnregisterConsoleCommands();
#endif
	}
};

IMPLEMENT_MODULE(FDIVERuntimeDevModule, DIVERuntimeDev);
