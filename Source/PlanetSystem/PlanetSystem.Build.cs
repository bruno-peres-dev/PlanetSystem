using UnrealBuildTool;

public class PlanetSystem : ModuleRules
{
    public PlanetSystem(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", 
            "CoreUObject", 
            "Engine", 
            "InputCore",
            "ProceduralMeshComponent", 
            "RenderCore", 
            "EngineSettings",
            "Json",           // Para serialização JSON
            "JsonUtilities",  // Para utilitários JSON
            "Slate",          // Para UI e widgets
            "SlateCore",      // Para core do Slate
            "ApplicationCore", // Para funcionalidades de aplicação
            "Projects",       // Para funcionalidades de projeto
            "DeveloperSettings" // Para configurações de desenvolvedor
        });
        
        PrivateDependencyModuleNames.AddRange(new[]
        {
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "Json",
            "JsonUtilities"
        });
        
        // Configurações específicas para diferentes plataformas
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.AddRange(new[]
            {
                "zlib.lib"  // Para compressão
            });
        }
        
        // Configurações de compilação
        PublicIncludePaths.AddRange(new[]
        {
            "Source/PlanetSystem/Public",
            "Source/PlanetSystem/Public/Core",
            "Source/PlanetSystem/Public/Configuration",
            "Source/PlanetSystem/Public/Services",
            "Source/PlanetSystem/Public/Generation",
            "Source/PlanetSystem/Public/Rendering",
            "Source/PlanetSystem/Public/Debug",
            "Source/PlanetSystem/Public/Serialization",
            "Source/PlanetSystem/Public/Presets",
            "Source/PlanetSystem/Public/Plugins"
        });
        
        PrivateIncludePaths.AddRange(new[]
        {
            "Source/PlanetSystem/Private",
            "Source/PlanetSystem/Private/Core",
            "Source/PlanetSystem/Private/Configuration",
            "Source/PlanetSystem/Private/Services",
            "Source/PlanetSystem/Private/Generation",
            "Source/PlanetSystem/Private/Rendering",
            "Source/PlanetSystem/Private/Debug",
            "Source/PlanetSystem/Private/Serialization",
            "Source/PlanetSystem/Private/Presets",
            "Source/PlanetSystem/Private/Plugins"
        });
        
        // Configurações de otimização
        if (Target.Configuration == UnrealTargetConfiguration.Shipping)
        {
            PublicDefinitions.Add("PLANETSYSTEM_SHIPPING=1");
        }
        else
        {
            PublicDefinitions.Add("PLANETSYSTEM_DEBUG=1");
        }
        
        // Configurações de warnings
        bEnableUndefinedIdentifierWarnings = false;
        bEnableExceptions = true;
    }
}
