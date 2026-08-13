// Copyright (c) 2026. All Rights Reserved.

using UnrealBuildTool;

public class DIVEUncooked : ModuleRules
{
	public DIVEUncooked(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DIVECore",
			"DIVERuntime"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"AssetRegistry",
			"BlueprintGraph",
			"Kismet",
			"KismetCompiler",
			"SlateCore"
		});
	}
}
