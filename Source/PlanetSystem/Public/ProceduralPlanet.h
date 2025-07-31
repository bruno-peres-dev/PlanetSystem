#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PatchNode.h"
#include "NoiseModule.h"
#include "ErosionModule.h"
#include "BiomeSystem.h"
#include "VegetationSystem.h"
#include "WaterComponent.h"
#include "EditingSystem.h"
#include "ProceduralPlanet.generated.h"

UCLASS()
class PLANETSYSTEM_API AProceduralPlanet : public AActor
{
    GENERATED_BODY()
public:
    AProceduralPlanet();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    UProceduralMeshComponent* MeshComp = nullptr;
    UPROPERTY()
    UNoiseModule* Noise = nullptr;
    UPROPERTY()
    UErosionModule* Erosion = nullptr;
    UPROPERTY()
    UBiomeSystem* Biomes = nullptr;
    UPROPERTY()
    UVegetationSystem* Vegetation = nullptr;
    UPROPERTY()
    UWaterComponent* Water = nullptr;
    UPROPERTY()
    UEditingSystem* Editing = nullptr;

    TArray<FPatchNode*> Roots;
    FTimerHandle LODTimer;

    void InitializeQuadTrees();
    void UpdateLOD();
};
