using UnrealBuildTool;

public class HearthwardTarget : TargetRules
{
    public HearthwardTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Hearthward");
    }
}
