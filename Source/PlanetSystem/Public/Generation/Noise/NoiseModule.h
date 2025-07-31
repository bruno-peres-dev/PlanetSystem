#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Configuration/DataAssets/CoreConfig.h"
#include "NoiseModule.generated.h"

UENUM(BlueprintType)
enum class ENoiseType : uint8 { Perlin, Ridged, Billow };

UCLASS(Blueprintable, ClassGroup=(Procedural), meta=(BlueprintSpawnableComponent))
class PLANETSYSTEM_API UNoiseModule : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise")
    ENoiseType NoiseType = ENoiseType::Perlin;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise", meta=(ClampMin="0.1"))
    float Frequency = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise", meta=(ClampMin="1"))
    int32 Octaves = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise", meta=(ClampMin="1"))
    float Lacunarity = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise", meta=(ClampMin="0", ClampMax="1"))
    float Persistence = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise")
    int32 Seed = 1337;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise|Warp")
    bool bEnableWarp = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Noise|Warp", meta=(ClampMin="0"))
    float WarpStrength = 0.5f;

    void SetSeed(int32 InSeed) { Seed = InSeed; }
    
    UFUNCTION(BlueprintCallable, Category="Noise")
    void SetNoiseConfig(const FNoiseConfig& NewConfig);
    
    UFUNCTION(BlueprintCallable, Category="Noise")
    FNoiseConfig GetNoiseConfig() const;

    float GetHeight(const FVector& Dir) const;
};
