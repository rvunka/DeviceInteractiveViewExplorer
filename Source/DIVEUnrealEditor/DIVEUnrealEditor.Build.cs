// Copyright (c) 2026. All Rights Reserved.

using UnrealBuildTool;

public class DIVEUnrealEditor : ModuleRules
{
	public DIVEUnrealEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DIVECore",
			"DIVERuntime",
			"DIVERuntimeDev"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"AssetTools",
			"AssetRegistry",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"EditorFramework",
			"DataValidation",
			"Kismet",
			"KismetCompiler",
			"BlueprintGraph",
			"PropertyEditor",
			"InputCore"
		});
	}
}
