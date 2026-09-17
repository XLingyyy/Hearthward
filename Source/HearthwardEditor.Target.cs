using UnrealBuildTool;

public class HearthwardEditorTarget : TargetRules
{
    public HearthwardEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Hearthward");
    }
}
