#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EditingSystem.generated.h"

UCLASS(ClassGroup=(Procedural), meta=(BlueprintSpawnableComponent))
class PLANETSYSTEM_API UEditingSystem : public UObject
{
    GENERATED_BODY()
public:
    void ApplyBrush(const FVector& WorldPos, float Radius, float Strength,
                    TArray<FVector>& Vertices, class UProceduralMeshComponent* MeshComp);
};
