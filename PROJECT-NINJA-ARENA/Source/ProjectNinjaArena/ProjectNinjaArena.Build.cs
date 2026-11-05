using UnrealBuildTool;

public class ProjectNinjaArena : ModuleRules
{
    public ProjectNinjaArena(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "EnhancedInput"
            }
        );
    }
}
