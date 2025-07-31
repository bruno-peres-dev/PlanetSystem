#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Generation/Terrain/PatchNode.h"
#include "Configuration/DataAssets/CoreConfig.h"
#include "Services/Core/ServiceLocator.h"
#include "Rendering/Chunks/ChunkCache.h"
#include "ProceduralPlanet.generated.h"

UCLASS()
class PLANETSYSTEM_API AProceduralPlanet : public AActor
{
    GENERATED_BODY()
public:
    AProceduralPlanet();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    
    // Blueprint accessible functions
    UFUNCTION(BlueprintCallable, Category="Planet")
    void SetCoreConfig(UPlanetCoreConfig* NewConfig);
    
    UFUNCTION(BlueprintCallable, Category="Planet")
    UPlanetCoreConfig* GetCoreConfig() const { return CoreConfig; }
    
    UFUNCTION(BlueprintCallable, Category="Planet")
    void GetPerformanceStats(int32& OutTotalChunks, int32& OutCachedChunks, float& OutCacheHitRate);

private:
    UPROPERTY()
    UProceduralMeshComponent* MeshComp = nullptr;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Planet")
    UPlanetCoreConfig* CoreConfig = nullptr;
    
    UPROPERTY()
    UChunkCache* ChunkCache = nullptr;
    
    TArray<FPatchNode*> Roots;
    FTimerHandle LODTimer;
    FTimerHandle CacheCleanupTimer;

    void InitializeQuadTrees();
    void UpdateLOD();
    void InitializeServices();
    void CleanupCache();
    
    // Performance tracking
    double LastLODUpdateTime = 0.0;
    int32 TotalChunksGenerated = 0;
    int32 CachedChunksUsed = 0;
};
