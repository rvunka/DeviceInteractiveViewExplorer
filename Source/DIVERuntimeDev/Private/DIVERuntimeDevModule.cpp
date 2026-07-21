// Copyright (c) 2026. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "Debug/DIVEDebugDump.h"

class FDIVERuntimeDevModule : public IModuleInterface
{
public:
	void StartupModule() override
	{
		DIVEDebugDump::RegisterConsoleCommands();
	}

	void ShutdownModule() override
	{
		DIVEDebugDump::UnregisterConsoleCommands();
	}
};

IMPLEMENT_MODULE(FDIVERuntimeDevModule, DIVERuntimeDev);
