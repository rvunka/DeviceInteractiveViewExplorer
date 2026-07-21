// Copyright (c) 2026. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

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
			"DIVECore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"DIVERuntime"
		});

		// Conditional GRIP link (§7): module stays in .uplugin; without GRIP it builds as a no-op stub.
		if (IsSiblingPluginEnabled(Target, ModuleDirectory, "GraspRigidbodyInertialPhysics", "GRIPRuntime"))
		{
			PublicDependencyModuleNames.AddRange(new string[]
			{
				"GRIPCore",
				"GRIPRuntime",
				"SharedPluginUtils"
			});
			PrivateDefinitions.Add("DIVE_GRIP_BRIDGE_WITH_GRIP=1");
		}
		else
		{
			PrivateDefinitions.Add("DIVE_GRIP_BRIDGE_WITH_GRIP=0");
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
