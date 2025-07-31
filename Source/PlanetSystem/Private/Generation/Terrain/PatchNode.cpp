#include "Generation/Terrain/PatchNode.h"
#include "Generation/Noise/NoiseModule.h"
#include "Services/Terrain/ErosionModule.h"
#include "ProceduralMeshComponent.h"

void FPatchNode::Subdivide()
{
    if (bIsSplit) return;
    FVector2D Mid = (UVMin + UVMax) * 0.5f;
    Children[0] = new FPatchNode(Level+1, UVMin, Mid);
    Children[1] = new FPatchNode(Level+1, FVector2D(Mid.X,UVMin.Y), FVector2D(UVMax.X,Mid.Y));
    Children[2] = new FPatchNode(Level+1, FVector2D(UVMin.X,Mid.Y), FVector2D(Mid.X,UVMax.Y));
    Children[3] = new FPatchNode(Level+1, Mid, UVMax);
    bIsSplit = true;
}

void FPatchNode::GenerateMesh(UProceduralMeshComponent* MeshComp, float PlanetRadius, UNoiseModule* Noise)
{
    Vertices.Empty();
    Indices.Empty();
    const int32 Res = FMath::Clamp(8 >> Level, 2, 16);

    // configure noise seed
    Noise->SetSeed(PatchSeed);

    // gerar grid UV
    for (int32 y=0; y<=Res; ++y)
    {
        float v = FMath::Lerp(UVMin.Y, UVMax.Y, float(y)/Res);
        for (int32 x=0; x<=Res; ++x)
        {
            float u = FMath::Lerp(UVMin.X, UVMax.X, float(x)/Res);
            FVector Dir((u-0.5f)*2.f, (v-0.5f)*2.f, 1.f);
            Dir.Normalize();
            float Alt = Noise->GetHeight(Dir);
            Vertices.Add(Dir*(PlanetRadius+Alt));
        }
    }

    // índices
    for (int32 y=0; y<Res; ++y)
        for (int32 x=0; x<Res; ++x)
        {
            int i0=y*(Res+1)+x, i1=i0+1, i2=i0+Res+1, i3=i2+1;
            Indices.Append({i0,i2,i1, i1,i2,i3});
        }

    // erosão determinística
    if (ErosionModule)
    {
        ErosionModule->ApplyHydraulicErosion(Vertices, Res, PatchSeed);
        
        // Notify plugins about erosion
        UPlanetSystemServiceLocator::GetInstance()->BroadcastErosionApplied(Vertices, PatchSeed);
    }

    // criar seção
    MeshComp->CreateMeshSection_LinearColor(Level, Vertices, Indices, {}, {}, {}, {}, false);
}
