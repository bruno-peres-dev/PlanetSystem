#pragma once
#include "CoreMinimal.h"
#include "Services/Terrain/ErosionModule.h"
#include "Generation/Noise/NoiseModule.h"

struct FPatchNode
{
    int32 Level;
    FVector2D UVMin, UVMax;
    uint32 PatchSeed;
    TArray<FVector> Vertices;
    TArray<int32> Indices;
    struct UErosionModule* ErosionModule = nullptr;
    FPatchNode* Children[4] = { nullptr, nullptr, nullptr, nullptr };
    bool bIsSplit = false;

    FPatchNode(int32 InLevel, const FVector2D& InMin, const FVector2D& InMax)
        : Level(InLevel), UVMin(InMin), UVMax(InMax)
    {
        PatchSeed = FCrc::MemCrc32(&Level, sizeof(Level), 0)
                  ^ FCrc::MemCrc32(&UVMin, sizeof(UVMin), 0)
                  ^ FCrc::MemCrc32(&UVMax, sizeof(UVMax), 0);
    }

    ~FPatchNode()
    {
        for (auto*& Child : Children)
            delete Child;
    }

    void Subdivide();
    void GenerateMesh(class UProceduralMeshComponent* MeshComp, float PlanetRadius, class UNoiseModule* Noise);
};
