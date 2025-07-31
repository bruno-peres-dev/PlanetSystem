using UnrealBuildTool;

public class PlanetSystem : ModuleRules
{
    public PlanetSystem(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore",
            "ProceduralMeshComponent", "RenderCore"
        });
    }
}
