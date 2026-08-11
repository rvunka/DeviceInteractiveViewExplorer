// Copyright (c) 2026. All Rights Reserved.

using UnrealBuildTool;

public class DIVECore : ModuleRules
{
	public DIVECore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("DataValidation");
		}
	}
}
