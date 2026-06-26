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
			"DIVERuntime"
		});

		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateDependencyModuleNames.Add("AutomationController");
		}
	}
}
