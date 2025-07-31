#include "Services/Environment/VegetationSystem.h"
#include "Services/Environment/BiomeSystem.h"
#include "Math/UnrealMathUtility.h"

void UVegetationSystem::Populate(const TArray<FVector>& Vertices, const TArray<int32>& Indices, EBiomeType Biome, uint32 Seed)
{
    if (!HISM) return;
    FRandomStream Stream(Seed ^ 0xA5A5A5A5);
    HISM->ClearInstances();
    float Density = (Biome==EBiomeType::Forest ? 0.02f : 0.005f);

    for(const FVector& V : Vertices)
        if (Stream.FRand() < Density)
            HISM->AddInstance(FTransform(V.Rotation(), V, FVector(1.f)));
}
