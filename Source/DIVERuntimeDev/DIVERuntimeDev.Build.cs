// Copyright (c) 2026. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.Linq;
using EpicGames.Core;
using UnrealBuildBase;

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
			"DIVEUncooked",
			"SharedPluginUtils"
		});

		// §6.2.9: UBT plugin enablement (ProjectDescriptor / ReadAvailablePlugins).
		if (IsPluginEnabledForTarget(Target, "GraspRigidbodyInertialPhysics"))
		{
			PrivateDefinitions.Add("DIVE_WITH_GRIP=1");
			PrivateDependencyModuleNames.Add("GRIPRuntime");
		}
		else
		{
			PrivateDefinitions.Add("DIVE_WITH_GRIP=0");
		}

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("DataValidation");
		}
	}

	static bool IsPluginEnabledForTarget(ReadOnlyTargetRules Target, string PluginName)
	{
		if (Target.DisablePlugins != null
			&& Target.DisablePlugins.Any(Name =>
				string.Equals(Name, PluginName, StringComparison.OrdinalIgnoreCase)))
		{
			return false;
		}

		if (Target.EnablePlugins != null
			&& Target.EnablePlugins.Any(Name =>
				string.Equals(Name, PluginName, StringComparison.OrdinalIgnoreCase)))
		{
			return true;
		}

		if (Target.ProjectFile == null)
		{
			return false;
		}

		ProjectDescriptor Project = ProjectDescriptor.FromFile(Target.ProjectFile);
		if (Project.Plugins != null)
		{
			foreach (PluginReferenceDescriptor Plugin in Project.Plugins)
			{
				if (string.Equals(Plugin.Name, PluginName, StringComparison.OrdinalIgnoreCase))
				{
					return Plugin.bEnabled;
				}
			}
		}

		System.Collections.Generic.List<PluginInfo> Available = Plugins.ReadAvailablePlugins(
			Unreal.EngineDirectory,
			Target.ProjectFile.Directory,
			Project.AdditionalPluginDirectories);
		PluginInfo Found = Available.FirstOrDefault(Plugin =>
			string.Equals(Plugin.Name, PluginName, StringComparison.OrdinalIgnoreCase));
		if (Found == null)
		{
			return false;
		}

		return Found.IsEnabledByDefault(!Project.DisableEnginePluginsByDefault);
	}
}
