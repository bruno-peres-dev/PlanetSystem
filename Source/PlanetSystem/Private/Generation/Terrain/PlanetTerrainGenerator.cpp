#include "Generation/Terrain/PlanetTerrainGenerator.h"
#include "Generation/Noise/NoiseModule.h"
#include "Services/Terrain/ErosionModule.h"
#include "Services/Environment/BiomeSystem.h"
#include "Services/Environment/VegetationSystem.h"
#include "Services/Environment/WaterComponent.h"
#include "Debug/Logging/PlanetSystemLogger.h"
#include "Core/Events/PlanetEventBus.h"
#include "Configuration/DataAssets/CoreConfig.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

UPlanetTerrainGenerator::UPlanetTerrainGenerator()
{
    // Inicializar componentes
    NoiseModule = CreateDefaultSubobject<UNoiseModule>(TEXT("NoiseModule"));
    ErosionModule = CreateDefaultSubobject<UErosionModule>(TEXT("ErosionModule"));
    BiomeSystem = CreateDefaultSubobject<UBiomeSystem>(TEXT("BiomeSystem"));
    VegetationSystem = CreateDefaultSubobject<UVegetationSystem>(TEXT("VegetationSystem"));
    WaterSystem = CreateDefaultSubobject<UWaterComponent>(TEXT("WaterSystem"));
    
    // Obter serviços do ServiceLocator
    Logger = UPlanetSystemLogger::GetInstance();
    EventBus = UPlanetEventBus::GetInstance();
    
    // Inicializar estatísticas
    TotalGenerationTime = 0.0f;
    ChunksGenerated = 0;
    AverageGenerationTime = 0.0f;
    MaxMemoryUsage = 0.0f;
    MaxCacheSize = 100;
    
    // Log de inicialização
    if (Logger)
    {
        Logger->LogInfo(TEXT("PlanetTerrainGenerator"), TEXT("Sistema de geração de terreno inicializado"));
    }
}

FPlanetChunk UPlanetTerrainGenerator::GenerateTerrainChunk(const FVector& Center, int32 LODLevel)
{
    const double StartTime = FPlatformTime::Seconds();
    
    try
    {
        // Validar parâmetros
        if (!ValidateParameters(Center, LODLevel))
        {
            LogGenerationEvent(EPlanetEventType::Error, TEXT("Parâmetros inválidos para geração de chunk"));
            return FPlanetChunk();
        }
        
        // Log de início
        LogGenerationEvent(EPlanetEventType::Info, FString::Printf(TEXT("Iniciando geração de chunk em %s, LOD %d"), *Center.ToString(), LODLevel));
        
        FPlanetChunk Chunk;
        Chunk.Center = Center;
        Chunk.LODLevel = LODLevel;
        Chunk.GenerationTime = FDateTime::Now();
        
        // 1. Gerar mapa de altura
        TArray<float> HeightMap;
        GenerateHeightMap(Center, LODLevel, HeightMap);
        Chunk.HeightMap = HeightMap;
        
        // 2. Aplicar erosão se habilitada
        if (CurrentConfig && CurrentConfig->ErosionConfig.bEnableErosion)
        {
            ApplyErosion(HeightMap, CurrentConfig->ErosionConfig);
            Chunk.HeightMap = HeightMap; // Atualizar com erosão
        }
        
        // 3. Gerar mapa de biomas
        TArray<EBiomeType> BiomeMap;
        GenerateBiomeMap(HeightMap, BiomeMap);
        Chunk.BiomeMap = BiomeMap;
        
        // 4. Gerar vegetação
        TArray<FVegetationInstance> Vegetation;
        GenerateVegetationMap(BiomeMap, Vegetation);
        Chunk.Vegetation = Vegetation;
        
        // 5. Gerar sistema de água
        FWaterSystem WaterSystem;
        GenerateWaterSystem(Chunk, WaterSystem);
        Chunk.WaterSystem = WaterSystem;
        
        // Calcular estatísticas
        const double EndTime = FPlatformTime::Seconds();
        const float GenerationTime = static_cast<float>(EndTime - StartTime);
        
        TotalGenerationTime += GenerationTime;
        ChunksGenerated++;
        AverageGenerationTime = TotalGenerationTime / ChunksGenerated;
        
        // Log de sucesso
        LogGenerationEvent(EPlanetEventType::Success, FString::Printf(TEXT("Chunk gerado com sucesso em %.3fms"), GenerationTime * 1000.0f));
        
        return Chunk;
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro na geração de chunk: %s"), UTF8_TO_TCHAR(e.what()));
        LogGenerationEvent(EPlanetEventType::Error, ErrorMsg);
        return FPlanetChunk();
    }
}

void UPlanetTerrainGenerator::ApplyBiomesToChunk(FPlanetChunk& Chunk, const FBiomeConfig& BiomeConfig)
{
    try
    {
        if (Chunk.HeightMap.Num() == 0)
        {
            LogGenerationEvent(EPlanetEventType::Warning, TEXT("Tentativa de aplicar biomas a chunk sem altura"));
            return;
        }
        
        // Gerar mapa de biomas baseado na altura
        TArray<EBiomeType> BiomeMap;
        GenerateBiomeMap(Chunk.HeightMap, BiomeMap);
        
        // Aplicar regras de bioma
        if (BiomeSystem)
        {
            BiomeSystem->ApplyBiomeRules(BiomeMap, BiomeConfig);
        }
        
        Chunk.BiomeMap = BiomeMap;
        
        LogGenerationEvent(EPlanetEventType::Info, TEXT("Biomas aplicados ao chunk"));
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro ao aplicar biomas: %s"), UTF8_TO_TCHAR(e.what()));
        LogGenerationEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

void UPlanetTerrainGenerator::GenerateVegetationForChunk(const FPlanetChunk& Chunk, TArray<FVegetationInstance>& OutVegetation)
{
    try
    {
        if (Chunk.BiomeMap.Num() == 0)
        {
            LogGenerationEvent(EPlanetEventType::Warning, TEXT("Tentativa de gerar vegetação sem mapa de biomas"));
            return;
        }
        
        // Gerar vegetação baseada no mapa de biomas
        GenerateVegetationMap(Chunk.BiomeMap, OutVegetation);
        
        // Otimizar densidade se necessário
        if (CurrentConfig && CurrentConfig->VegetationConfig.bOptimizeDensity)
        {
            if (VegetationSystem)
            {
                VegetationSystem->OptimizeVegetationDensity(OutVegetation, CurrentConfig->VegetationConfig.MaxDensity);
            }
        }
        
        LogGenerationEvent(EPlanetEventType::Info, FString::Printf(TEXT("Vegetação gerada: %d instâncias"), OutVegetation.Num()));
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro ao gerar vegetação: %s"), UTF8_TO_TCHAR(e.what()));
        LogGenerationEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

void UPlanetTerrainGenerator::GenerateWaterSystem(const FPlanetChunk& Chunk, FWaterSystem& OutWaterSystem)
{
    try
    {
        if (Chunk.HeightMap.Num() == 0)
        {
            LogGenerationEvent(EPlanetEventType::Warning, TEXT("Tentativa de gerar água sem mapa de altura"));
            return;
        }
        
        if (WaterSystem)
        {
            // Gerar sistema de oceanos
            FOceanSystem OceanSystem;
            WaterSystem->GenerateOceanSystem(Chunk, OceanSystem);
            OutWaterSystem.OceanSystem = OceanSystem;
            
            // Gerar sistema de rios
            FRiverSystem RiverSystem;
            WaterSystem->GenerateRiverSystem(Chunk, RiverSystem);
            OutWaterSystem.RiverSystem = RiverSystem;
        }
        
        LogGenerationEvent(EPlanetEventType::Info, TEXT("Sistema de água gerado"));
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro ao gerar sistema de água: %s"), UTF8_TO_TCHAR(e.what()));
        LogGenerationEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

void UPlanetTerrainGenerator::SetGenerationConfig(const UPlanetCoreConfig* Config)
{
    CurrentConfig = Config;
    
    if (Config)
    {
        // Aplicar configuração aos sistemas
        if (NoiseModule)
        {
            NoiseModule->SetNoiseConfig(Config->NoiseConfig);
        }
        
        if (ErosionModule)
        {
            ErosionModule->SetErosionConfig(Config->ErosionConfig);
        }
        
        if (BiomeSystem)
        {
            BiomeSystem->SetBiomeConfig(Config->BiomeConfig);
        }
        
        if (VegetationSystem)
        {
            VegetationSystem->SetVegetationConfig(Config->VegetationConfig);
        }
        
        if (WaterSystem)
        {
            WaterSystem->SetWaterConfig(Config->WaterConfig);
        }
        
        LogGenerationEvent(EPlanetEventType::Info, TEXT("Configuração de geração atualizada"));
    }
}

const UPlanetCoreConfig* UPlanetTerrainGenerator::GetGenerationConfig() const
{
    return CurrentConfig;
}

void UPlanetTerrainGenerator::GetGenerationStats(FString& OutStats) const
{
    OutStats = FString::Printf(
        TEXT("=== Estatísticas de Geração ===\n")
        TEXT("Chunks Gerados: %d\n")
        TEXT("Tempo Total: %.3fms\n")
        TEXT("Tempo Médio: %.3fms\n")
        TEXT("Memória Máxima: %.2fMB\n")
        TEXT("Cache Size: %d/%d\n"),
        ChunksGenerated,
        TotalGenerationTime * 1000.0f,
        AverageGenerationTime * 1000.0f,
        MaxMemoryUsage,
        HeightMapCache.Num(),
        MaxCacheSize
    );
}

void UPlanetTerrainGenerator::ResetStats()
{
    TotalGenerationTime = 0.0f;
    ChunksGenerated = 0;
    AverageGenerationTime = 0.0f;
    MaxMemoryUsage = 0.0f;
    
    LogGenerationEvent(EPlanetEventType::Info, TEXT("Estatísticas resetadas"));
}

void UPlanetTerrainGenerator::GenerateHeightMap(const FVector& Center, int32 LODLevel, TArray<float>& OutHeightMap)
{
    if (!NoiseModule || !CurrentConfig)
    {
        LogGenerationEvent(EPlanetEventType::Error, TEXT("NoiseModule ou CurrentConfig não disponível"));
        return;
    }
    
    // Calcular resolução baseada no LOD
    const int32 BaseResolution = CurrentConfig->GenerationConfig.BaseResolution;
    const int32 Resolution = FMath::Max(2, BaseResolution >> LODLevel);
    const int32 TotalVertices = Resolution * Resolution;
    
    OutHeightMap.SetNum(TotalVertices);
    
    // Calcular tamanho do chunk
    const float ChunkSize = CurrentConfig->GenerationConfig.ChunkSize;
    const float StepSize = ChunkSize / (Resolution - 1);
    
    // Gerar mapa de altura usando noise
    for (int32 Y = 0; Y < Resolution; Y++)
    {
        for (int32 X = 0; X < Resolution; X++)
        {
            const int32 Index = Y * Resolution + X;
            
            // Calcular posição no mundo
            const FVector WorldPos = Center + FVector(
                (X - Resolution / 2.0f) * StepSize,
                (Y - Resolution / 2.0f) * StepSize,
                0.0f
            );
            
            // Gerar altura usando noise
            float Height = 0.0f;
            
            // Noise base
            Height += NoiseModule->GetNoiseValue(WorldPos, CurrentConfig->NoiseConfig.BaseNoiseScale) * CurrentConfig->NoiseConfig.BaseNoiseStrength;
            
            // Noise de detalhe
            if (CurrentConfig->NoiseConfig.bEnableDetailNoise)
            {
                Height += NoiseModule->GetNoiseValue(WorldPos, CurrentConfig->NoiseConfig.DetailNoiseScale) * CurrentConfig->NoiseConfig.DetailNoiseStrength;
            }
            
            // Noise de rugosidade
            if (CurrentConfig->NoiseConfig.bEnableRoughnessNoise)
            {
                Height += NoiseModule->GetNoiseValue(WorldPos, CurrentConfig->NoiseConfig.RoughnessNoiseScale) * CurrentConfig->NoiseConfig.RoughnessNoiseStrength;
            }
            
            // Aplicar curva de altura
            Height = FMath::Pow(Height, CurrentConfig->NoiseConfig.HeightCurve);
            
            // Clampar altura
            Height = FMath::Clamp(Height, CurrentConfig->NoiseConfig.MinHeight, CurrentConfig->NoiseConfig.MaxHeight);
            
            OutHeightMap[Index] = Height;
        }
    }
    
    // Aplicar ruído de detalhe se habilitado
    if (CurrentConfig->NoiseConfig.bEnableDetailNoise)
    {
        ApplyDetailNoise(OutHeightMap, CurrentConfig->NoiseConfig.DetailNoiseScale, CurrentConfig->NoiseConfig.DetailNoiseStrength);
    }
}

void UPlanetTerrainGenerator::ApplyErosion(TArray<float>& HeightMap, const FErosionConfig& ErosionConfig)
{
    if (!ErosionModule)
    {
        LogGenerationEvent(EPlanetEventType::Warning, TEXT("ErosionModule não disponível"));
        return;
    }
    
    // Aplicar erosão térmica
    if (ErosionConfig.bEnableThermalErosion)
    {
        ErosionModule->ApplyThermalErosion(HeightMap, ErosionConfig.ThermalErosionStrength, ErosionConfig.ThermalErosionIterations);
    }
    
    // Aplicar erosão hidráulica
    if (ErosionConfig.bEnableHydraulicErosion)
    {
        ErosionModule->ApplyHydraulicErosion(HeightMap, ErosionConfig.HydraulicErosionStrength, ErosionConfig.HydraulicErosionIterations);
    }
    
    // Aplicar erosão de vento
    if (ErosionConfig.bEnableWindErosion)
    {
        ErosionModule->ApplyWindErosion(HeightMap, ErosionConfig.WindErosionStrength, ErosionConfig.WindErosionIterations);
    }
}

void UPlanetTerrainGenerator::GenerateBiomeMap(const TArray<float>& HeightMap, TArray<EBiomeType>& OutBiomeMap)
{
    if (!CurrentConfig)
    {
        LogGenerationEvent(EPlanetEventType::Error, TEXT("CurrentConfig não disponível para geração de biomas"));
        return;
    }
    
    const int32 NumVertices = HeightMap.Num();
    OutBiomeMap.SetNum(NumVertices);
    
    const FBiomeConfig& BiomeConfig = CurrentConfig->BiomeConfig;
    
    for (int32 i = 0; i < NumVertices; i++)
    {
        const float Height = HeightMap[i];
        
        // Determinar bioma baseado na altura
        EBiomeType BiomeType = EBiomeType::Ocean;
        
        if (Height > BiomeConfig.OceanThreshold)
        {
            if (Height < BiomeConfig.BeachThreshold)
            {
                BiomeType = EBiomeType::Beach;
            }
            else if (Height < BiomeConfig.PlainsThreshold)
            {
                BiomeType = EBiomeType::Plains;
            }
            else if (Height < BiomeConfig.ForestThreshold)
            {
                BiomeType = EBiomeType::Forest;
            }
            else if (Height < BiomeConfig.MountainThreshold)
            {
                BiomeType = EBiomeType::Mountain;
            }
            else
            {
                BiomeType = EBiomeType::Peak;
            }
        }
        
        OutBiomeMap[i] = BiomeType;
    }
    
    // Aplicar suavização de transições se habilitada
    if (BiomeConfig.bSmoothTransitions)
    {
        if (BiomeSystem)
        {
            BiomeSystem->SmoothBiomeTransitions(OutBiomeMap);
        }
    }
}

void UPlanetTerrainGenerator::GenerateVegetationMap(const TArray<EBiomeType>& BiomeMap, TArray<FVegetationInstance>& OutVegetation)
{
    if (!CurrentConfig || !VegetationSystem)
    {
        LogGenerationEvent(EPlanetEventType::Warning, TEXT("Configuração ou VegetationSystem não disponível"));
        return;
    }
    
    const FVegetationConfig& VegetationConfig = CurrentConfig->VegetationConfig;
    
    for (int32 i = 0; i < BiomeMap.Num(); i++)
    {
        const EBiomeType BiomeType = BiomeMap[i];
        
        // Obter tipos de vegetação para este bioma
        TArray<FVegetationType> VegetationTypes = VegetationSystem->GetVegetationForBiome(BiomeType);
        
        for (const FVegetationType& VegetationType : VegetationTypes)
        {
            // Calcular probabilidade de spawn
            const float SpawnChance = FMath::FRand();
            if (SpawnChance <= VegetationType.SpawnProbability)
            {
                FVegetationInstance Instance;
                Instance.VegetationType = VegetationType;
                Instance.Position = FVector::ZeroVector; // Será calculado pelo sistema de vegetação
                Instance.Rotation = FRotator::ZeroRotator;
                Instance.Scale = FVector::OneVector;
                
                OutVegetation.Add(Instance);
            }
        }
    }
}

float UPlanetTerrainGenerator::CalculateTemperature(float Latitude)
{
    // Simular temperatura baseada na latitude
    // Equador = quente, polos = frio
    const float NormalizedLatitude = FMath::Abs(FMath::Sin(Latitude));
    return 1.0f - NormalizedLatitude; // 1 = quente, 0 = frio
}

float UPlanetTerrainGenerator::CalculateHumidity(float Height, float Temperature)
{
    // Simular umidade baseada na altura e temperatura
    // Altura baixa + temperatura alta = alta umidade
    const float HeightFactor = 1.0f - FMath::Clamp(Height, 0.0f, 1.0f);
    const float TemperatureFactor = Temperature;
    
    return (HeightFactor + TemperatureFactor) * 0.5f;
}

void UPlanetTerrainGenerator::ApplyDetailNoise(TArray<float>& HeightMap, float DetailScale, float DetailStrength)
{
    if (!NoiseModule)
    {
        return;
    }
    
    const int32 NumVertices = HeightMap.Num();
    const int32 Resolution = FMath::Sqrt(static_cast<float>(NumVertices));
    
    for (int32 i = 0; i < NumVertices; i++)
    {
        const int32 X = i % Resolution;
        const int32 Y = i / Resolution;
        
        const FVector NoisePos = FVector(X * DetailScale, Y * DetailScale, 0.0f);
        const float DetailNoise = NoiseModule->GetNoiseValue(NoisePos, DetailScale) * DetailStrength;
        
        HeightMap[i] += DetailNoise;
    }
}

void UPlanetTerrainGenerator::CleanupCache()
{
    // Limpar cache se exceder tamanho máximo
    if (HeightMapCache.Num() > MaxCacheSize)
    {
        const int32 ItemsToRemove = HeightMapCache.Num() - MaxCacheSize;
        TArray<FVector> KeysToRemove;
        
        // Obter chaves mais antigas
        for (const auto& Pair : HeightMapCache)
        {
            KeysToRemove.Add(Pair.Key);
            if (KeysToRemove.Num() >= ItemsToRemove)
            {
                break;
            }
        }
        
        // Remover itens
        for (const FVector& Key : KeysToRemove)
        {
            HeightMapCache.Remove(Key);
            BiomeMapCache.Remove(Key);
        }
    }
}

FString UPlanetTerrainGenerator::CalculatePositionHash(const FVector& Position, int32 LODLevel) const
{
    return FString::Printf(TEXT("%.2f_%.2f_%.2f_LOD%d"), Position.X, Position.Y, Position.Z, LODLevel);
}

bool UPlanetTerrainGenerator::ValidateParameters(const FVector& Center, int32 LODLevel) const
{
    if (LODLevel < 0 || LODLevel > 10)
    {
        return false;
    }
    
    if (!FMath::IsFinite(Center.X) || !FMath::IsFinite(Center.Y) || !FMath::IsFinite(Center.Z))
    {
        return false;
    }
    
    return true;
}

void UPlanetTerrainGenerator::LogGenerationEvent(EPlanetEventType EventType, const FString& Details)
{
    if (Logger)
    {
        Logger->LogInfo(TEXT("PlanetTerrainGenerator"), Details);
    }
    
    if (EventBus)
    {
        FPlanetSystemEvent Event;
        Event.EventType = EventType;
        Event.Source = TEXT("PlanetTerrainGenerator");
        Event.Details = Details;
        Event.Timestamp = FDateTime::Now();
        
        EventBus->BroadcastEvent(Event);
    }
} 