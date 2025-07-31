#include "Plugins/ExamplePlugin.h"
#include "Generation/Terrain/ProceduralPlanet.h"
#include "Engine/Engine.h"

UExamplePlanetPlugin::UExamplePlanetPlugin()
{
    TotalChunksProcessed = 0;
    TotalBiomesCalculated = 0;
    bDebugVisualizationEnabled = false;
}

void UExamplePlanetPlugin::OnPlanetGenerated(AProceduralPlanet* Planet)
{
    if (!Planet)
    {
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("ExamplePlugin: Planet generated at location %s"), 
           *Planet->GetActorLocation().ToString());
    
    if (bDebugVisualizationEnabled)
    {
        // Add debug visualization here
        UE_LOG(LogTemp, Log, TEXT("ExamplePlugin: Debug visualization enabled for planet"));
    }
}

void UExamplePlanetPlugin::OnBiomeCalculated(EBiomeType Biome, const FVector& Location)
{
    TotalBiomesCalculated++;
    BiomeHistory.Add(Biome);
    
    // Keep only last 100 biomes in history
    if (BiomeHistory.Num() > 100)
    {
        BiomeHistory.RemoveAt(0);
    }
    
    if (bDebugVisualizationEnabled)
    {
        FString BiomeName;
        switch (Biome)
        {
            case EBiomeType::Desert: BiomeName = TEXT("Desert"); break;
            case EBiomeType::Plains: BiomeName = TEXT("Plains"); break;
            case EBiomeType::Mountains: BiomeName = TEXT("Mountains"); break;
            case EBiomeType::Forest: BiomeName = TEXT("Forest"); break;
            case EBiomeType::Snow: BiomeName = TEXT("Snow"); break;
            default: BiomeName = TEXT("Unknown"); break;
        }
        
        UE_LOG(LogTemp, Log, TEXT("ExamplePlugin: Biome calculated - %s at %s"), 
               *BiomeName, *Location.ToString());
    }
}

void UExamplePlanetPlugin::OnChunkGenerated(const FVector& Center, int32 LODLevel)
{
    TotalChunksProcessed++;
    
    if (bDebugVisualizationEnabled && TotalChunksProcessed % 10 == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("ExamplePlugin: Chunk %d generated at LOD %d, center: %s"), 
               TotalChunksProcessed, LODLevel, *Center.ToString());
    }
}

void UExamplePlanetPlugin::OnErosionApplied(const TArray<FVector>& Vertices, uint32 Seed)
{
    if (bDebugVisualizationEnabled)
    {
        UE_LOG(LogTemp, Log, TEXT("ExamplePlugin: Erosion applied to %d vertices with seed %u"), 
               Vertices.Num(), Seed);
    }
}

void UExamplePlanetPlugin::LogPlanetStatistics()
{
    UE_LOG(LogTemp, Log, TEXT("=== ExamplePlugin Statistics ==="));
    UE_LOG(LogTemp, Log, TEXT("Total Chunks Processed: %d"), TotalChunksProcessed);
    UE_LOG(LogTemp, Log, TEXT("Total Biomes Calculated: %d"), TotalBiomesCalculated);
    
    // Calculate biome distribution
    TMap<EBiomeType, int32> BiomeCounts;
    for (EBiomeType Biome : BiomeHistory)
    {
        BiomeCounts.FindOrAdd(Biome)++;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Biome Distribution (last 100):"));
    for (const auto& Pair : BiomeCounts)
    {
        FString BiomeName;
        switch (Pair.Key)
        {
            case EBiomeType::Desert: BiomeName = TEXT("Desert"); break;
            case EBiomeType::Plains: BiomeName = TEXT("Plains"); break;
            case EBiomeType::Mountains: BiomeName = TEXT("Mountains"); break;
            case EBiomeType::Forest: BiomeName = TEXT("Forest"); break;
            case EBiomeType::Snow: BiomeName = TEXT("Snow"); break;
            default: BiomeName = TEXT("Unknown"); break;
        }
        
        float Percentage = (float)Pair.Value / (float)BiomeHistory.Num() * 100.0f;
        UE_LOG(LogTemp, Log, TEXT("  %s: %d (%.1f%%)"), *BiomeName, Pair.Value, Percentage);
    }
    
    UE_LOG(LogTemp, Log, TEXT("================================"));
}

void UExamplePlanetPlugin::EnableDebugVisualization(bool bEnable)
{
    bDebugVisualizationEnabled = bEnable;
    UE_LOG(LogTemp, Log, TEXT("ExamplePlugin: Debug visualization %s"), 
           bEnable ? TEXT("enabled") : TEXT("disabled"));
} 