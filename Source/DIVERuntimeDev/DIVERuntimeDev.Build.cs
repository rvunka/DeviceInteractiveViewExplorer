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

		if (System.IO.Directory.Exists(System.IO.Path.Combine(ModuleDirectory, "..", "..", "..", "GraspRigidbodyInertialPhysics", "Source", "GRIPRuntime")))
		{
			PublicDefinitions.Add("DIVE_WITH_GRIP=1");
			PrivateDependencyModuleNames.Add("GRIPRuntime");
		}

		if (System.IO.Directory.Exists(System.IO.Path.Combine(ModuleDirectory, "..", "DIVEGRIPBridge")))
		{
			PublicDefinitions.Add("DIVE_WITH_GRIP_BRIDGE=1");
			PrivateDependencyModuleNames.Add("DIVEGRIPBridge");
		}
	}
}
