#include "ProceduralPlanet.h"
#include "ProceduralMeshComponent.h"
#include "TimerManager.h"

AProceduralPlanet::AProceduralPlanet()
{
    PrimaryActorTick.bCanEverTick = false;
    MeshComp   = CreateDefaultSubobject<UProceduralMeshComponent>("ProcMesh");
    RootComponent = MeshComp;

    Noise      = CreateDefaultSubobject<UNoiseModule>("Noise");
    Erosion    = CreateDefaultSubobject<UErosionModule>("Erosion");
    Biomes     = CreateDefaultSubobject<UBiomeSystem>("Biomes");
    Vegetation = CreateDefaultSubobject<UVegetationSystem>("Vegetation");
    Water      = CreateDefaultSubobject<UWaterComponent>("Water");
    Editing    = CreateDefaultSubobject<UEditingSystem>("Editing");
}

void AProceduralPlanet::BeginPlay()
{
    Super::BeginPlay();
    InitializeQuadTrees();
    Water->GenerateOcean(MeshComp, 1000.f);
    GetWorldTimerManager().SetTimer(LODTimer, this, &AProceduralPlanet::UpdateLOD, 0.2f, true);
}

void AProceduralPlanet::InitializeQuadTrees()
{
    Roots.Empty();
    TArray<FVector2D> Mins = {{0,0},{0.5f,0},{0,0.5f},{0.5f,0.5f},{1,0},{1,0.5f}};
    TArray<FVector2D> Maxs = {{0.5f,0.5f},{1,0.5f},{0.5f,1},{1,1},{0.5f,0.5f},{1,1}};
    for(int i=0;i<6;++i)
    {
        FPatchNode* Root = new FPatchNode(0, Mins[i], Maxs[i]);
        Root->ErosionModule = Erosion;
        Roots.Add(Root);
    }
}

void AProceduralPlanet::UpdateLOD()
{
    MeshComp->ClearAllMeshSections();
    for(FPatchNode* Root : Roots)
    {
        if (Root->Level < 3) Root->Subdivide();
        Root->GenerateMesh(MeshComp, 1000.f, Noise);
        uint32 Seed = Root->PatchSeed;
        EBiomeType B = Biomes->GetBiome(Noise->GetHeight(FVector::ZeroVector), 0.f, 0.f);
        Vegetation->Populate(Root->Vertices, Root->Indices, B, Seed);
    }
}
