// Copyright (c) 2026. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

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

		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateDependencyModuleNames.Add("AutomationController");
		}

		// MUST NOT use Directory.Exists alone as "plugin enabled" (§6.2.9).
		if (IsSiblingPluginEnabled(Target, ModuleDirectory, "GraspRigidbodyInertialPhysics", "GRIPRuntime"))
		{
			PrivateDefinitions.Add("DIVE_WITH_GRIP=1");
			PrivateDependencyModuleNames.Add("GRIPRuntime");
		}
		else
		{
			PrivateDefinitions.Add("DIVE_WITH_GRIP=0");
		}

		// Bridge module always exists; GRIP link inside it is conditional.
		if (Directory.Exists(Path.Combine(ModuleDirectory, "..", "DIVEGRIPBridge")))
		{
			PrivateDefinitions.Add("DIVE_WITH_GRIP_BRIDGE=1");
			PrivateDependencyModuleNames.Add("DIVEGRIPBridge");
		}
		else
		{
			PrivateDefinitions.Add("DIVE_WITH_GRIP_BRIDGE=0");
		}
	}

	static bool IsSiblingPluginEnabled(
		ReadOnlyTargetRules Target,
		string ModuleDir,
		string PluginName,
		string RuntimeModuleFolderName)
	{
		string RuntimePath = Path.GetFullPath(Path.Combine(
			ModuleDir, "..", "..", "..", PluginName, "Source", RuntimeModuleFolderName));
		if (!Directory.Exists(RuntimePath))
		{
			return false;
		}

		if (Target.DisablePlugins != null
			&& Target.DisablePlugins.Any(Name =>
				string.Equals(Name, PluginName, StringComparison.OrdinalIgnoreCase)))
		{
			return false;
		}

		if (Target.ProjectFile != null && File.Exists(Target.ProjectFile.FullName))
		{
			string Text = File.ReadAllText(Target.ProjectFile.FullName);
			if (IsPluginExplicitlyDisabled(Text, PluginName))
			{
				return false;
			}
		}

		return true;
	}

	static bool IsPluginExplicitlyDisabled(string UProjectJson, string PluginName)
	{
		int NameIdx = UProjectJson.IndexOf("\"" + PluginName + "\"", StringComparison.OrdinalIgnoreCase);
		if (NameIdx < 0)
		{
			return false;
		}

		int BlockStart = UProjectJson.LastIndexOf('{', NameIdx);
		int BlockEnd = UProjectJson.IndexOf('}', NameIdx);
		if (BlockStart < 0 || BlockEnd < 0 || BlockEnd <= BlockStart)
		{
			return false;
		}

		string Block = UProjectJson.Substring(BlockStart, BlockEnd - BlockStart + 1);
		return Block.IndexOf("\"Enabled\"", StringComparison.OrdinalIgnoreCase) >= 0
			&& Regex.IsMatch(Block, "\"Enabled\"\\s*:\\s*false", RegexOptions.IgnoreCase);
	}
}
