#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Services/Core/ServiceLocator.h"
#include "Generation/Terrain/ProceduralPlanet.h"
#include "Services/Environment/BiomeSystem.h"
#include "ExamplePlugin.generated.h"

UCLASS(Blueprintable, ClassGroup=(Procedural))
class PLANETSYSTEM_API UExamplePlanetPlugin : public UObject, public IPlanetSystemPlugin
{
    GENERATED_BODY()
    
public:
    UExamplePlanetPlugin();
    
    // IPlanetSystemPlugin interface
    virtual void OnPlanetGenerated(AProceduralPlanet* Planet) override;
    virtual void OnBiomeCalculated(EBiomeType Biome, const FVector& Location) override;
    virtual void OnChunkGenerated(const FVector& Center, int32 LODLevel) override;
    virtual void OnErosionApplied(const TArray<FVector>& Vertices, uint32 Seed) override;
    
    // Plugin specific functionality
    UFUNCTION(BlueprintCallable, Category="ExamplePlugin")
    void LogPlanetStatistics();
    
    UFUNCTION(BlueprintCallable, Category="ExamplePlugin")
    void EnableDebugVisualization(bool bEnable);
    
private:
    UPROPERTY()
    int32 TotalChunksProcessed = 0;
    
    UPROPERTY()
    int32 TotalBiomesCalculated = 0;
    
    UPROPERTY()
    TArray<EBiomeType> BiomeHistory;
    
    UPROPERTY()
    bool bDebugVisualizationEnabled = false;
}; 