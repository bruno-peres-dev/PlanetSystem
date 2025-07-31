#pragma once
#include "CoreMinimal.h"
#include "BiomeSystem.h" // for EBiomeType
#include "PlanetTypes.generated.h"

USTRUCT(BlueprintType)
struct PLANETSYSTEM_API FOceanSystem
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water")
    TArray<FVector> SurfaceVertices;
};

USTRUCT(BlueprintType)
struct PLANETSYSTEM_API FRiverSystem
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water")
    TArray<TArray<FVector>> RiverPoints;
};

USTRUCT(BlueprintType)
struct PLANETSYSTEM_API FWaterSystem
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water")
    FOceanSystem OceanSystem;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Water")
    FRiverSystem RiverSystem;
};

USTRUCT(BlueprintType)
struct PLANETSYSTEM_API FErosionConfig
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion")
    bool bEnableThermalErosion = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion")
    float ThermalErosionStrength = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion")
    int32 ThermalErosionIterations = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion")
    bool bEnableHydraulicErosion = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion")
    float HydraulicErosionStrength = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion")
    int32 HydraulicErosionIterations = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion")
    bool bEnableWindErosion = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion")
    float WindErosionStrength = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Erosion")
    int32 WindErosionIterations = 1;
};

USTRUCT(BlueprintType)
struct PLANETSYSTEM_API FVegetationType
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vegetation")
    float SpawnProbability = 1.0f;
};

USTRUCT(BlueprintType)
struct PLANETSYSTEM_API FVegetationInstance
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vegetation")
    FVegetationType VegetationType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vegetation")
    FVector Position = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vegetation")
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vegetation")
    FVector Scale = FVector::OneVector;
};

USTRUCT(BlueprintType)
struct PLANETSYSTEM_API FPlanetChunk
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chunk")
    FVector Center = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chunk")
    int32 LODLevel = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chunk")
    FDateTime GenerationTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chunk")
    TArray<float> HeightMap;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chunk")
    TArray<EBiomeType> BiomeMap;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chunk")
    TArray<FVegetationInstance> Vegetation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Chunk")
    FWaterSystem WaterSystem;

    FPlanetChunk()
    {
        GenerationTime = FDateTime::Now();
    }
};

