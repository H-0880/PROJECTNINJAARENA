using UnrealBuildTool;
using System.Collections.Generic;

public class ProjectNinjaArenaTarget : TargetRules
{
    public ProjectNinjaArenaTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("ProjectNinjaArena");
    }
}
