#include "Configuration/DataAssets/CoreConfig.h"
#include "Debug/Logging/PlanetSystemLogger.h"
#include "Configuration/Validators/PlanetConfigValidator.h"
#include "Core/Events/PlanetEventBus.h"

UPlanetCoreConfig::UPlanetCoreConfig()
{
    // Inicializa com valores padrão otimizados
    InitializeDefaultValues();
    
    UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("PlanetCoreConfig created with default values"));
}

void UPlanetCoreConfig::InitializeDefaultValues()
{
    // Configuração de geração padrão
    GenerationConfig.BaseRadius = 1000.0f;
    GenerationConfig.MaxLODLevel = 8;
    GenerationConfig.LODUpdateInterval = 0.2f;
    GenerationConfig.BaseMeshResolution = 8;
    GenerationConfig.bEnableErosion = true;
    GenerationConfig.bEnableVegetation = true;
    GenerationConfig.bEnableWater = true;
    
    // Configuração de noise padrão
    NoiseConfig.GlobalSeed = 1337;
    NoiseConfig.BaseFrequency = 1.0f;
    NoiseConfig.Octaves = 6;
    NoiseConfig.Lacunarity = 2.0f;
    NoiseConfig.Persistence = 0.5f;
    NoiseConfig.bEnableWarp = true;
    NoiseConfig.WarpStrength = 0.5f;
    
    // Configuração de biomas padrão
    BiomeConfig.DesertAltitudeThreshold = 0.7f;
    BiomeConfig.MountainAltitudeThreshold = 0.5f;
    BiomeConfig.SnowAltitudeThreshold = 0.8f;
    BiomeConfig.ForestHumidityThreshold = 0.6f;
    BiomeConfig.PlainsSlopeThreshold = 0.3f;
    
    // Configurações de debug e performance
    bEnableDebugVisualization = false;
    bEnablePerformanceProfiling = false;
}

bool UPlanetCoreConfig::ValidateConfiguration(TArray<FPlanetValidationError>& OutErrors) const
{
    try
    {
        UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Starting configuration validation"));
        
        // Valida a configuração completa usando o validador
        bool bIsValid = UPlanetConfigValidator::ValidateConfig(this, OutErrors);
        
        if (bIsValid)
        {
            UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Configuration validation passed"));
        }
        else
        {
            UPlanetSystemLogger::LogWarning(TEXT("CoreConfig"), 
                FString::Printf(TEXT("Configuration validation failed with %d errors"), OutErrors.Num()));
        }
        
        // Broadcast evento de validação
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::ConfigValidated, TEXT("CoreConfig"), TEXT(""), 0.0f, OutErrors.Num());
        
        return bIsValid;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("CoreConfig"), 
            FString::Printf(TEXT("Exception during configuration validation: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

bool UPlanetCoreConfig::ApplyOptimizations()
{
    try
    {
        UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Applying configuration optimizations"));
        
        bool bOptimizationsApplied = false;
        
        // Otimizações de performance
        if (GenerationConfig.MaxLODLevel > 10)
        {
            GenerationConfig.MaxLODLevel = 10;
            UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Reduced MaxLODLevel to 10 for better performance"));
            bOptimizationsApplied = true;
        }
        
        if (NoiseConfig.Octaves > 8)
        {
            NoiseConfig.Octaves = 8;
            UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Reduced Octaves to 8 for better performance"));
            bOptimizationsApplied = true;
        }
        
        if (GenerationConfig.BaseMeshResolution > 16)
        {
            GenerationConfig.BaseMeshResolution = 16;
            UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Reduced BaseMeshResolution to 16 for better performance"));
            bOptimizationsApplied = true;
        }
        
        // Otimizações de qualidade
        if (GenerationConfig.LODUpdateInterval < 0.1f)
        {
            GenerationConfig.LODUpdateInterval = 0.1f;
            UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Increased LODUpdateInterval to 0.1f for better quality"));
            bOptimizationsApplied = true;
        }
        
        if (NoiseConfig.BaseFrequency < 0.5f)
        {
            NoiseConfig.BaseFrequency = 0.5f;
            UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Increased BaseFrequency to 0.5f for better detail"));
            bOptimizationsApplied = true;
        }
        
        if (bOptimizationsApplied)
        {
            UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Configuration optimizations applied successfully"));
            
            UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                EPlanetEventType::ConfigOptimized, TEXT("CoreConfig"));
        }
        else
        {
            UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("No optimizations needed"));
        }
        
        return bOptimizationsApplied;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("CoreConfig"), 
            FString::Printf(TEXT("Exception applying optimizations: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

void UPlanetCoreConfig::GetConfigurationSummary(FString& OutSummary) const
{
    try
    {
        OutSummary = FString::Printf(
            TEXT("Planet Core Configuration Summary:\n")
            TEXT("==================================\n\n")
            TEXT("Generation Settings:\n")
            TEXT("- Base Radius: %.1f\n")
            TEXT("- Max LOD Level: %d\n")
            TEXT("- LOD Update Interval: %.2f\n")
            TEXT("- Base Mesh Resolution: %d\n")
            TEXT("- Erosion Enabled: %s\n")
            TEXT("- Vegetation Enabled: %s\n")
            TEXT("- Water Enabled: %s\n\n")
            TEXT("Noise Settings:\n")
            TEXT("- Global Seed: %d\n")
            TEXT("- Base Frequency: %.2f\n")
            TEXT("- Octaves: %d\n")
            TEXT("- Lacunarity: %.2f\n")
            TEXT("- Persistence: %.2f\n")
            TEXT("- Warp Enabled: %s\n")
            TEXT("- Warp Strength: %.2f\n\n")
            TEXT("Biome Settings:\n")
            TEXT("- Desert Altitude Threshold: %.2f\n")
            TEXT("- Mountain Altitude Threshold: %.2f\n")
            TEXT("- Snow Altitude Threshold: %.2f\n")
            TEXT("- Forest Humidity Threshold: %.2f\n")
            TEXT("- Plains Slope Threshold: %.2f\n\n")
            TEXT("Debug Settings:\n")
            TEXT("- Debug Visualization: %s\n")
            TEXT("- Performance Profiling: %s\n"),
            GenerationConfig.BaseRadius,
            GenerationConfig.MaxLODLevel,
            GenerationConfig.LODUpdateInterval,
            GenerationConfig.BaseMeshResolution,
            GenerationConfig.bEnableErosion ? TEXT("Yes") : TEXT("No"),
            GenerationConfig.bEnableVegetation ? TEXT("Yes") : TEXT("No"),
            GenerationConfig.bEnableWater ? TEXT("Yes") : TEXT("No"),
            NoiseConfig.GlobalSeed,
            NoiseConfig.BaseFrequency,
            NoiseConfig.Octaves,
            NoiseConfig.Lacunarity,
            NoiseConfig.Persistence,
            NoiseConfig.bEnableWarp ? TEXT("Yes") : TEXT("No"),
            NoiseConfig.WarpStrength,
            BiomeConfig.DesertAltitudeThreshold,
            BiomeConfig.MountainAltitudeThreshold,
            BiomeConfig.SnowAltitudeThreshold,
            BiomeConfig.ForestHumidityThreshold,
            BiomeConfig.PlainsSlopeThreshold,
            bEnableDebugVisualization ? TEXT("Yes") : TEXT("No"),
            bEnablePerformanceProfiling ? TEXT("Yes") : TEXT("No")
        );
        
        UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Configuration summary generated"));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("CoreConfig"), 
            FString::Printf(TEXT("Exception generating configuration summary: %s"), UTF8_TO_TCHAR(e.what())));
        OutSummary = TEXT("Error generating configuration summary");
    }
}

bool UPlanetCoreConfig::IsProductionReady() const
{
    try
    {
        TArray<FPlanetValidationError> ValidationErrors;
        bool bIsProductionReady = UPlanetConfigValidator::IsProductionReady(this, ValidationErrors);
        
        if (bIsProductionReady)
        {
            UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Configuration is production ready"));
        }
        else
        {
            UPlanetSystemLogger::LogWarning(TEXT("CoreConfig"), 
                FString::Printf(TEXT("Configuration is not production ready (%d issues)"), ValidationErrors.Num()));
        }
        
        return bIsProductionReady;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("CoreConfig"), 
            FString::Printf(TEXT("Exception checking production readiness: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

void UPlanetCoreConfig::GetPerformanceEstimates(float& OutEstimatedMemoryMB, float& OutEstimatedTimePerFrame) const
{
    try
    {
        // Estimativas baseadas na configuração
        const float BaseMemoryPerChunk = 0.5f; // MB por chunk
        const int32 EstimatedChunks = FMath::Pow(2, GenerationConfig.MaxLODLevel);
        OutEstimatedMemoryMB = BaseMemoryPerChunk * EstimatedChunks;
        
        // Estimativa de tempo por frame baseada na complexidade
        const float BaseTimePerFrame = 0.016f; // 16ms base
        const float ComplexityMultiplier = 1.0f + (GenerationConfig.MaxLODLevel - 5) * 0.1f;
        const float NoiseMultiplier = 1.0f + (NoiseConfig.Octaves - 4) * 0.05f;
        const float ResolutionMultiplier = 1.0f + (GenerationConfig.BaseMeshResolution - 8) * 0.02f;
        
        OutEstimatedTimePerFrame = BaseTimePerFrame * ComplexityMultiplier * NoiseMultiplier * ResolutionMultiplier;
        
        UPlanetSystemLogger::LogDebug(TEXT("CoreConfig"), 
            FString::Printf(TEXT("Performance estimates: %.2f MB memory, %.3f ms per frame"), 
                OutEstimatedMemoryMB, OutEstimatedTimePerFrame * 1000.0f));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("CoreConfig"), 
            FString::Printf(TEXT("Exception calculating performance estimates: %s"), UTF8_TO_TCHAR(e.what())));
        OutEstimatedMemoryMB = 0.0f;
        OutEstimatedTimePerFrame = 0.0f;
    }
}

void UPlanetCoreConfig::ResetToDefaults()
{
    try
    {
        UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Resetting configuration to defaults"));
        
        InitializeDefaultValues();
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::ConfigReset, TEXT("CoreConfig"));
        
        UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Configuration reset to defaults completed"));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("CoreConfig"), 
            FString::Printf(TEXT("Exception resetting to defaults: %s"), UTF8_TO_TCHAR(e.what())));
    }
}

bool UPlanetCoreConfig::CopyFrom(const UPlanetCoreConfig* SourceConfig)
{
    try
    {
        if (!SourceConfig)
        {
            UPlanetSystemLogger::LogWarning(TEXT("CoreConfig"), TEXT("Cannot copy from null source configuration"));
            return false;
        }
        
        UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Copying configuration from source"));
        
        // Copia todas as configurações
        GenerationConfig = SourceConfig->GenerationConfig;
        NoiseConfig = SourceConfig->NoiseConfig;
        BiomeConfig = SourceConfig->BiomeConfig;
        bEnableDebugVisualization = SourceConfig->bEnableDebugVisualization;
        bEnablePerformanceProfiling = SourceConfig->bEnablePerformanceProfiling;
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::ConfigCopied, TEXT("CoreConfig"));
        
        UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), TEXT("Configuration copied successfully"));
        return true;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("CoreConfig"), 
            FString::Printf(TEXT("Exception copying configuration: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

void UPlanetCoreConfig::GetOptimizationSuggestions(TArray<FString>& OutSuggestions) const
{
    try
    {
        OutSuggestions.Empty();
        
        // Sugestões baseadas na configuração atual
        if (GenerationConfig.MaxLODLevel > 10)
        {
            OutSuggestions.Add(TEXT("Consider reducing MaxLODLevel to 10 or less for better performance"));
        }
        
        if (NoiseConfig.Octaves > 8)
        {
            OutSuggestions.Add(TEXT("Consider reducing Octaves to 8 or less for better performance"));
        }
        
        if (GenerationConfig.BaseMeshResolution > 16)
        {
            OutSuggestions.Add(TEXT("Consider reducing BaseMeshResolution to 16 or less for better performance"));
        }
        
        if (GenerationConfig.LODUpdateInterval < 0.1f)
        {
            OutSuggestions.Add(TEXT("Consider increasing LODUpdateInterval to 0.1f or higher for better quality"));
        }
        
        if (NoiseConfig.BaseFrequency < 0.5f)
        {
            OutSuggestions.Add(TEXT("Consider increasing BaseFrequency to 0.5f or higher for better detail"));
        }
        
        if (bEnableDebugVisualization && bEnablePerformanceProfiling)
        {
            OutSuggestions.Add(TEXT("Consider disabling debug visualization and performance profiling in production"));
        }
        
        UPlanetSystemLogger::LogInfo(TEXT("CoreConfig"), 
            FString::Printf(TEXT("Generated %d optimization suggestions"), OutSuggestions.Num()));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("CoreConfig"), 
            FString::Printf(TEXT("Exception generating optimization suggestions: %s"), UTF8_TO_TCHAR(e.what())));
    }
} 