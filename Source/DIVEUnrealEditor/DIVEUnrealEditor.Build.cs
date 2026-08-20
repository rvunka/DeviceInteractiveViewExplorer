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
			"DIVERuntime"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"AssetTools",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"DataValidation",
			"PropertyEditor",
			// Slate SComboBox in DIVEActionBindingCustomization instantiates EKeys (LNK2019 without this).
			"InputCore",
			"DIVEUncooked"
		});
	}
}
