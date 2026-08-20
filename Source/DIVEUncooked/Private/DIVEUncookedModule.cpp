// Copyright (c) 2026. All Rights Reserved.

#include "DIVEUncookedModule.h"

#include "K2Nodes/K2Node_DIVEActionEvent.h"
#include "Modules/ModuleManager.h"

void FDIVEUncookedModule::ShutdownModule()
{
	DIVEUncooked_UninstallActionClassMenuRefreshHooks();
}

IMPLEMENT_MODULE(FDIVEUncookedModule, DIVEUncooked);
