// Copyright (c) 2026. All Rights Reserved.

using UnrealBuildTool;

public class DIVERuntimeDev : ModuleRules
{
	public DIVERuntimeDev(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"DIVECore",
			"DIVERuntime",
			"SharedPluginUtils"
		});

		// Dump console lives in DIVEUncooked (UnrealEd). Instantiating that module
		// from a game target constructor pulls UnrealEd and fails makefile generation.
		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.Add("DIVEUncooked");
			PrivateDependencyModuleNames.Add("DataValidation");
		}

		// GRIP is DIVEGRIPBridge-only. Linking GRIPRuntime here would force Optional GRIP on this .uplugin.
	}
}
