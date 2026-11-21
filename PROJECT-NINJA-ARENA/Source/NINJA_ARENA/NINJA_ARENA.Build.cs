// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NINJA_ARENA : ModuleRules
{
	public NINJA_ARENA(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "EngineSettings", "Niagara", "IKRig" });

		PublicIncludePaths.AddRange(new string[] {
			"NINJA_ARENA",
			"NINJA_ARENA/Public",
			"NINJA_ARENA/Public/Core",
			"NINJA_ARENA/Public/Combat",
			"NINJA_ARENA/Public/Modes",
			"NINJA_ARENA/Public/Stage",
			"NINJA_ARENA/Variant_Platforming",
			"NINJA_ARENA/Variant_Platforming/Animation",
			"NINJA_ARENA/Variant_Combat",
			"NINJA_ARENA/Variant_Combat/AI",
			"NINJA_ARENA/Variant_Combat/Animation",
			"NINJA_ARENA/Variant_Combat/Gameplay",
			"NINJA_ARENA/Variant_Combat/Interfaces",
			"NINJA_ARENA/Variant_Combat/UI",
			"NINJA_ARENA/Variant_SideScrolling",
			"NINJA_ARENA/Variant_SideScrolling/AI",
			"NINJA_ARENA/Variant_SideScrolling/Gameplay",
			"NINJA_ARENA/Variant_SideScrolling/Interfaces",
			"NINJA_ARENA/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
