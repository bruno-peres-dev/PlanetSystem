#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VegetationSystem.generated.h"

UCLASS(ClassGroup=(Procedural), meta=(BlueprintSpawnableComponent))
class PLANETSYSTEM_API UVegetationSystem : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Vegetation")
    UHierarchicalInstancedStaticMeshComponent* HISM = nullptr;

    void Populate(const TArray<FVector>& Vertices, const TArray<int32>& Indices, EBiomeType Biome, uint32 Seed);
};
