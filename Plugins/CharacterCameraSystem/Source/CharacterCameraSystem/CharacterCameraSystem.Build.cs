// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CharacterCameraSystem : ModuleRules
{
	public CharacterCameraSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreOnline",
				"CoreUObject",
				"InputCore",
				"Engine",
				"NetCore",
				"PhysicsCore",
				"DataRegistry"
			}
		);
		
	}
}
