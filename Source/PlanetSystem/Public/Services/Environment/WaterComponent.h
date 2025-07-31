#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WaterComponent.generated.h"

UCLASS(ClassGroup=(Procedural), meta=(BlueprintSpawnableComponent))
class PLANETSYSTEM_API UWaterComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Water")
    float SeaLevel = 0.f;

    void GenerateOcean(class UProceduralMeshComponent* MeshComp, float PlanetRadius);
};
