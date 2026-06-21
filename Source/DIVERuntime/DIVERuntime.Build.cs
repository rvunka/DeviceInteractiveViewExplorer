// Copyright (c) 2026. All Rights Reserved.

using UnrealBuildTool;

public class DIVERuntime : ModuleRules
{
	public DIVERuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DIVECore"
		});
	}
}
