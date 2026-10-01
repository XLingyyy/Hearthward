using UnrealBuildTool;

public class A3GameAssetEditor : ModuleRules
{
    public A3GameAssetEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new[] { "AssetRegistry", "PhysicsCore", "PhysicsUtilities", "UnrealEd" });
    }
}
