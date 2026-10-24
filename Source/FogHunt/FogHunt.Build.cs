using UnrealBuildTool;

public class FogHunt : ModuleRules
{
	public FogHunt(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput"
		});

		// Lets sources include each other as "Characters/FHCharacterBase.h" etc.
		PublicIncludePaths.Add(ModuleDirectory);
	}
}
