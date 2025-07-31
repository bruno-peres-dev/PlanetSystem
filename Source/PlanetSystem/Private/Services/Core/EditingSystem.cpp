#include "Services/Core/EditingSystem.h"
#include "ProceduralMeshComponent.h"

void UEditingSystem::ApplyBrush(const FVector& WorldPos, float Radius, float Strength,
                                TArray<FVector>& Vertices, UProceduralMeshComponent* MeshComp)
{
    for(FVector& V : Vertices)
    {
        float Dist = FVector::Dist(V, WorldPos);
        if (Dist < Radius)
        {
            float Falloff = 1.f - (Dist / Radius);
            V += V.GetSafeNormal() * Strength * Falloff;
        }
    }
    MeshComp->UpdateMeshSection(0, Vertices, {}, {}, {}, {});
}
