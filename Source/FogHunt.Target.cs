using UnrealBuildTool;

public class FogHuntTarget : TargetRules
{
	public FogHuntTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("FogHunt");
	}
}
