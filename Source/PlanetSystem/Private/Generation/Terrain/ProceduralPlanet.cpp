#include "Generation/Terrain/ProceduralPlanet.h"
#include "ProceduralMeshComponent.h"
#include "TimerManager.h"
#include "Services/Core/ServiceLocator.h"
#include "Rendering/Chunks/ChunkCache.h"
#include "Debug/Logging/PlanetSystemLogger.h"

AProceduralPlanet::AProceduralPlanet()
{
    PrimaryActorTick.bCanEverTick = false;
    MeshComp = CreateDefaultSubobject<UProceduralMeshComponent>("ProcMesh");
    RootComponent = MeshComp;
    
    ChunkCache = CreateDefaultSubobject<UChunkCache>("ChunkCache");
}

void AProceduralPlanet::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize logging
    UPlanetSystemLogger::Initialize();
    UPlanetSystemLogger::LogInfo(TEXT("ProceduralPlanet"), TEXT("BeginPlay started"));
    
    InitializeServices();
    InitializeQuadTrees();
    
    // Setup timers
    float LODInterval = CoreConfig ? CoreConfig->GenerationConfig.LODUpdateInterval : 0.2f;
    GetWorldTimerManager().SetTimer(LODTimer, this, &AProceduralPlanet::UpdateLOD, LODInterval, true);
    GetWorldTimerManager().SetTimer(CacheCleanupTimer, this, &AProceduralPlanet::CleanupCache, 30.0f, true);
    
    // Notify plugins
    UPlanetSystemServiceLocator::GetInstance()->BroadcastPlanetGenerated(this);
    
    UPlanetSystemLogger::LogInfo(TEXT("ProceduralPlanet"), TEXT("BeginPlay completed successfully"));
}

void AProceduralPlanet::InitializeServices()
{
    // Initialize ServiceLocator if not already done
    if (!CoreConfig)
    {
        UPlanetSystemLogger::LogWarning(TEXT("ProceduralPlanet"), TEXT("No CoreConfig provided, using default configuration"));
        CoreConfig = NewObject<UPlanetCoreConfig>();
    }
    
    UPlanetSystemServiceLocator::GetInstance()->InitializeServices(CoreConfig);
    UPlanetSystemLogger::LogInfo(TEXT("ProceduralPlanet"), TEXT("Services initialized successfully"));
}

void AProceduralPlanet::InitializeQuadTrees()
{
    Roots.Empty();
    TArray<FVector2D> Mins = {{0,0},{0.5f,0},{0,0.5f},{0.5f,0.5f},{1,0},{1,0.5f}};
    TArray<FVector2D> Maxs = {{0.5f,0.5f},{1,0.5f},{0.5f,1},{1,1},{0.5f,0.5f},{1,1}};
    
    for(int i=0;i<6;++i)
    {
        FPatchNode* Root = new FPatchNode(0, Mins[i], Maxs[i]);
        Root->ErosionModule = UPlanetSystemServiceLocator::GetErosionService();
        Roots.Add(Root);
    }
}

void AProceduralPlanet::UpdateLOD()
{
    double StartTime = FPlatformTime::Seconds();
    
    MeshComp->ClearAllMeshSections();
    
    UNoiseModule* Noise = UPlanetSystemServiceLocator::GetNoiseService();
    UBiomeSystem* Biomes = UPlanetSystemServiceLocator::GetBiomeService();
    UVegetationSystem* Vegetation = UPlanetSystemServiceLocator::GetVegetationService();
    UWaterComponent* Water = UPlanetSystemServiceLocator::GetWaterService();
    
    if (!Noise || !Biomes || !Vegetation || !Water)
    {
        UPlanetSystemLogger::LogError(TEXT("ProceduralPlanet"), TEXT("Required services not initialized"));
        return;
    }
    
    float PlanetRadius = CoreConfig ? CoreConfig->GenerationConfig.BaseRadius : 1000.0f;
    int32 MaxLOD = CoreConfig ? CoreConfig->GenerationConfig.MaxLODLevel : 8;
    
    for(FPatchNode* Root : Roots)
    {
        if (Root->Level < MaxLOD) Root->Subdivide();
        
        // Try to get from cache first
        FChunkKey CacheKey(Root->UVMin, Root->UVMax, Root->Level, Root->PatchSeed);
        FChunkData CachedData;
        
        if (ChunkCache && ChunkCache->GetChunk(CacheKey, CachedData))
        {
            // Use cached data
            MeshComp->CreateMeshSection_LinearColor(Root->Level, CachedData.Vertices, CachedData.Indices, 
                                                   CachedData.Normals, CachedData.UVs, {}, {}, false);
            CachedChunksUsed++;
        }
        else
        {
            // Generate new chunk
            Root->GenerateMesh(MeshComp, PlanetRadius, Noise);
            
            // Cache the generated data
            if (ChunkCache)
            {
                FChunkData NewChunkData;
                NewChunkData.Vertices = Root->Vertices;
                NewChunkData.Indices = Root->Indices;
                NewChunkData.Seed = Root->PatchSeed;
                NewChunkData.LODLevel = Root->Level;
                NewChunkData.UVMin = Root->UVMin;
                NewChunkData.UVMax = Root->UVMax;
                NewChunkData.UpdateAccessTime();
                
                ChunkCache->StoreChunk(CacheKey, NewChunkData);
            }
            
            TotalChunksGenerated++;
        }
        
        // Calculate biome and populate vegetation
        uint32 Seed = Root->PatchSeed;
        float Height = Noise->GetHeight(FVector::ZeroVector);
        EBiomeType Biome = Biomes->GetBiome(Height, 0.f, 0.f);
        
        if (CoreConfig && CoreConfig->GenerationConfig.bEnableVegetation)
        {
            Vegetation->Populate(Root->Vertices, Root->Indices, Biome, Seed);
        }
        
        // Notify plugins
        UPlanetSystemServiceLocator::GetInstance()->BroadcastChunkGenerated(
            Root->Vertices.Num() > 0 ? Root->Vertices[0] : FVector::ZeroVector, 
            Root->Level
        );
    }
    
    // Generate water if enabled
    if (CoreConfig && CoreConfig->GenerationConfig.bEnableWater)
    {
        Water->GenerateOcean(MeshComp, PlanetRadius);
    }
    
    LastLODUpdateTime = FPlatformTime::Seconds() - StartTime;
    
    // Log performance metrics
    UPlanetSystemLogger::LogPerformance(TEXT("ProceduralPlanet"), 
        FString::Printf(TEXT("LOD Update took %.3fms, Generated: %d, Cached: %d"), 
        LastLODUpdateTime * 1000.0, TotalChunksGenerated, CachedChunksUsed));
    
    if (CoreConfig && CoreConfig->bEnablePerformanceProfiling)
    {
        UE_LOG(LogTemp, Log, TEXT("PlanetSystem: LOD Update took %.3fms, Generated: %d, Cached: %d"), 
               LastLODUpdateTime * 1000.0, TotalChunksGenerated, CachedChunksUsed);
    }
}

void AProceduralPlanet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    
    UPlanetSystemLogger::LogInfo(TEXT("ProceduralPlanet"), TEXT("EndPlay started - cleaning up resources"));
    
    // Cleanup timers
    GetWorldTimerManager().ClearTimer(LODTimer);
    GetWorldTimerManager().ClearTimer(CacheCleanupTimer);
    
    // Cleanup quad tree
    for (FPatchNode* Root : Roots)
    {
        delete Root;
    }
    Roots.Empty();
    
    UPlanetSystemLogger::LogInfo(TEXT("ProceduralPlanet"), TEXT("EndPlay completed - resources cleaned up"));
}

void AProceduralPlanet::SetCoreConfig(UPlanetCoreConfig* NewConfig)
{
    if (NewConfig != CoreConfig)
    {
        CoreConfig = NewConfig;
        
        // Reinitialize services with new config
        if (CoreConfig)
        {
            UPlanetSystemServiceLocator::GetInstance()->InitializeServices(CoreConfig);
            
            // Update timers with new configuration
            GetWorldTimerManager().ClearTimer(LODTimer);
            GetWorldTimerManager().SetTimer(LODTimer, this, &AProceduralPlanet::UpdateLOD, 
                                           CoreConfig->GenerationConfig.LODUpdateInterval, true);
        }
    }
}

void AProceduralPlanet::GetPerformanceStats(int32& OutTotalChunks, int32& OutCachedChunks, float& OutCacheHitRate)
{
    OutTotalChunks = TotalChunksGenerated;
    OutCachedChunks = CachedChunksUsed;
    
    if (ChunkCache)
    {
        OutCacheHitRate = ChunkCache->GetCacheHitRate();
    }
    else
    {
        OutCacheHitRate = 0.0f;
    }
}

void AProceduralPlanet::CleanupCache()
{
    if (ChunkCache)
    {
        ChunkCache->CleanupExpiredChunks();
        ChunkCache->OptimizeCache();
        
        // Log cache statistics
        int32 CacheSize, MaxSize;
        float HitRate;
        ChunkCache->GetCacheStats(CacheSize, MaxSize, HitRate);
        
        UPlanetSystemLogger::LogPerformance(TEXT("ProceduralPlanet"), 
            FString::Printf(TEXT("Cache Stats - Size: %d/%d, Hit Rate: %.2f%%"), 
            CacheSize, MaxSize, HitRate * 100.0f));
        
        if (CoreConfig && CoreConfig->bEnablePerformanceProfiling)
        {
            UE_LOG(LogTemp, Log, TEXT("PlanetSystem: Cache Stats - Size: %d/%d, Hit Rate: %.2f%%"), 
                   CacheSize, MaxSize, HitRate * 100.0f);
        }
    }
}
