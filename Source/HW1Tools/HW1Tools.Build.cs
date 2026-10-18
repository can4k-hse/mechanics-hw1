using UnrealBuildTool;

public class HW1Tools : ModuleRules
{
    public HW1Tools(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "UnrealEd", "MechanicsHW1",
            "EnhancedInput", "InputCore", "BlueprintGraph", "Kismet", "KismetCompiler",
            "AnimGraph", "AnimGraphRuntime", "AssetRegistry"
        });
    }
}
