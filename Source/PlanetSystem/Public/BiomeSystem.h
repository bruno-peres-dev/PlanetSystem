#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BiomeSystem.generated.h"

UENUM(BlueprintType)
enum class EBiomeType : uint8
{
    Desert,
    Plains,
    Mountains,
    Forest,
    Snow
};

UCLASS(Blueprintable, ClassGroup=(Procedural), meta=(BlueprintSpawnableComponent))
class PLANETSYSTEM_API UBiomeSystem : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Biomes")
    UTexture2D* BiomeLookup = nullptr;

    EBiomeType GetBiome(float Altitude, float Slope, float Humidity) const;
};
