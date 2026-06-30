// Copyright (c) 2026. All Rights Reserved.

using UnrealBuildTool;

public class DIVEGRIPBridge : ModuleRules
{
	public DIVEGRIPBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DIVECore",
			"GRIPCore",
			"GRIPRuntime"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"DIVERuntime"
		});
	}
}
