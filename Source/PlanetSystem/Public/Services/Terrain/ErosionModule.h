#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ErosionModule.generated.h"

UCLASS(Blueprintable, ClassGroup=(Procedural), meta=(BlueprintSpawnableComponent))
class PLANETSYSTEM_API UErosionModule : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion")
    bool bEnableHydraulic = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion", meta=(ClampMin="1"))
    int32 Iterations = 50000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion", meta=(ClampMin="0", ClampMax="1"))
    float SedimentCapacity = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion", meta=(ClampMin="0", ClampMax="1"))
    float ErodeRate = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion", meta=(ClampMin="0", ClampMax="1"))
    float DepositRate = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion")
    int32 MaxSteps = 30;

    void ApplyHydraulicErosion(TArray<FVector>& Vertices, int32 Resolution, uint32 Seed);
};
