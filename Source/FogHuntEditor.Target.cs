using UnrealBuildTool;

public class FogHuntEditorTarget : TargetRules
{
	public FogHuntEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("FogHunt");
	}
}
