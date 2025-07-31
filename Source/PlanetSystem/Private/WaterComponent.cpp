#include "WaterComponent.h"
#include "ProceduralMeshComponent.h"

void UWaterComponent::GenerateOcean(UProceduralMeshComponent* MeshComp, float PlanetRadius)
{
    TArray<FVector> V; TArray<int32> I;
    float R = PlanetRadius + SeaLevel;
    // exemplo simplificado: um triângulo por face
    V.Add(FVector(0, R, 0)); V.Add(FVector(R, 0, 0)); V.Add(FVector(0, 0, R));
    I = {0,1,2};
    MeshComp->CreateMeshSection(99, V, I, {}, {}, {}, {}, false);
}
