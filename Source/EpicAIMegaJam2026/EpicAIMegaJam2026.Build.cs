// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class EpicAIMegaJam2026 : ModuleRules
{
	public EpicAIMegaJam2026(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// The Director's Cut prototypes keep file-local helpers in anonymous namespaces; unity builds would merge them.
		bUseUnity = false;

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

		PrivateDependencyModuleNames.AddRange(new string[] { "SlateCore", "RenderCore" });

		// NVIDIA DLSS (Plugins/DLSS, Windows only): asked at startup whether DLSS runs here, to pick the render resolution.
		bool bWithDLSS = Target.Platform == UnrealTargetPlatform.Win64;
		if (bWithDLSS)
		{
			PrivateDependencyModuleNames.Add("DLSSBlueprint");
		}
		PublicDefinitions.Add("WITH_DLSS=" + (bWithDLSS ? "1" : "0"));

		PublicIncludePaths.AddRange(new string[] {
			"EpicAIMegaJam2026",
			"EpicAIMegaJam2026/DirectorsCut",
			"EpicAIMegaJam2026/Variant_Horror",
			"EpicAIMegaJam2026/Variant_Horror/UI",
			"EpicAIMegaJam2026/Variant_Shooter",
			"EpicAIMegaJam2026/Variant_Shooter/AI",
			"EpicAIMegaJam2026/Variant_Shooter/UI",
			"EpicAIMegaJam2026/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
