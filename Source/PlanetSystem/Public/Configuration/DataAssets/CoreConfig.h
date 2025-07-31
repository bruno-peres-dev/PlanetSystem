#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Configuration/Validators/PlanetConfigValidator.h"
#include "CoreConfig.generated.h"

USTRUCT(BlueprintType)
struct FPlanetGenerationConfig
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation", meta=(ClampMin="100", ClampMax="10000"))
    float BaseRadius = 1000.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation", meta=(ClampMin="1", ClampMax="12"))
    int32 MaxLODLevel = 8;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation", meta=(ClampMin="0.1", ClampMax="2.0"))
    float LODUpdateInterval = 0.2f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation", meta=(ClampMin="2", ClampMax="32"))
    int32 BaseMeshResolution = 8;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation")
    bool bEnableErosion = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation")
    bool bEnableVegetation = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generation")
    bool bEnableWater = true;
};

USTRUCT(BlueprintType)
struct FNoiseConfig
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise")
    int32 GlobalSeed = 1337;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise", meta=(ClampMin="0.1"))
    float BaseFrequency = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise", meta=(ClampMin="1"))
    int32 Octaves = 6;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise", meta=(ClampMin="1"))
    float Lacunarity = 2.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise", meta=(ClampMin="0", ClampMax="1"))
    float Persistence = 0.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise")
    bool bEnableWarp = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise", meta=(ClampMin="0"))
    float WarpStrength = 0.5f;
};

USTRUCT(BlueprintType)
struct FBiomeConfig
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biomes")
    float DesertAltitudeThreshold = 0.7f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biomes")
    float MountainAltitudeThreshold = 0.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biomes")
    float SnowAltitudeThreshold = 0.8f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biomes")
    float ForestHumidityThreshold = 0.6f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biomes")
    float PlainsSlopeThreshold = 0.3f;
};

UCLASS(BlueprintType)
class PLANETSYSTEM_API UPlanetCoreConfig : public UDataAsset
{
    GENERATED_BODY()
    
public:
    UPlanetCoreConfig();
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Configuration")
    FPlanetGenerationConfig GenerationConfig;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Configuration")
    FNoiseConfig NoiseConfig;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Configuration")
    FBiomeConfig BiomeConfig;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Configuration")
    bool bEnableDebugVisualization = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Configuration")
    bool bEnablePerformanceProfiling = false;
    
    // Funções de validação e otimização
    UFUNCTION(BlueprintCallable, Category="Configuration")
    bool ValidateConfiguration(TArray<FPlanetValidationError>& OutErrors) const;
    
    UFUNCTION(BlueprintCallable, Category="Configuration")
    bool ApplyOptimizations();
    
    UFUNCTION(BlueprintCallable, Category="Configuration")
    void GetConfigurationSummary(FString& OutSummary) const;
    
    UFUNCTION(BlueprintCallable, Category="Configuration")
    bool IsProductionReady() const;
    
    UFUNCTION(BlueprintCallable, Category="Configuration")
    void GetPerformanceEstimates(float& OutEstimatedMemoryMB, float& OutEstimatedTimePerFrame) const;
    
    UFUNCTION(BlueprintCallable, Category="Configuration")
    void ResetToDefaults();
    
    UFUNCTION(BlueprintCallable, Category="Configuration")
    bool CopyFrom(const UPlanetCoreConfig* SourceConfig);
    
    UFUNCTION(BlueprintCallable, Category="Configuration")
    void GetOptimizationSuggestions(TArray<FString>& OutSuggestions) const;
    
private:
    void InitializeDefaultValues();
}; 