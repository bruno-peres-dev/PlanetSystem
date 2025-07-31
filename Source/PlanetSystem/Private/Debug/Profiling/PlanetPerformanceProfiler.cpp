#include "Debug/Profiling/PlanetPerformanceProfiler.h"
#include "Debug/Logging/PlanetSystemLogger.h"
#include "Core/Events/PlanetEventBus.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/DateTime.h"

// Instância singleton
UPlanetPerformanceProfiler* UPlanetPerformanceProfiler::Instance = nullptr;

UPlanetPerformanceProfiler::UPlanetPerformanceProfiler()
{
    ProfilingStartTime = FPlatformTime::Seconds();
    UPlanetSystemLogger::LogInfo(TEXT("PlanetPerformanceProfiler"), TEXT("Performance profiler created"));
}

UPlanetPerformanceProfiler* UPlanetPerformanceProfiler::GetInstance()
{
    if (!Instance)
    {
        Instance = NewObject<UPlanetPerformanceProfiler>();
        Instance->AddToRoot(); // Previne garbage collection
        UPlanetSystemLogger::LogInfo(TEXT("PlanetPerformanceProfiler"), TEXT("Performance profiler singleton created"));
    }
    return Instance;
}

void UPlanetPerformanceProfiler::BeginSection(const FString& SectionName)
{
    try
    {
        if (!bProfilingEnabled)
        {
            return;
        }
        
        if (SectionName.IsEmpty())
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetPerformanceProfiler"), TEXT("Cannot begin section with empty name"));
            return;
        }
        
        const float CurrentTime = FPlatformTime::Seconds();
        
        // Verifica se a seção já está ativa
        FSectionStats* ExistingStats = SectionStats.Find(SectionName);
        if (ExistingStats && ExistingStats->bIsActive)
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetPerformanceProfiler"), 
                FString::Printf(TEXT("Section %s is already active, ending previous session"), *SectionName));
            EndSection(SectionName);
        }
        
        // Cria ou atualiza estatísticas da seção
        FSectionStats& Stats = SectionStats.FindOrAdd(SectionName);
        Stats.LastStartTime = CurrentTime;
        Stats.bIsActive = true;
        
        UPlanetSystemLogger::LogDebug(TEXT("PlanetPerformanceProfiler"), 
            FString::Printf(TEXT("Section started: %s"), *SectionName));
        
        // Broadcast evento
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::ProfilingStarted, TEXT("Profiling"), SectionName);
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPerformanceProfiler"), 
            FString::Printf(TEXT("Exception beginning section: %s"), UTF8_TO_TCHAR(e.what())));
    }
}

void UPlanetPerformanceProfiler::EndSection(const FString& SectionName)
{
    try
    {
        if (!bProfilingEnabled)
        {
            return;
        }
        
        if (SectionName.IsEmpty())
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetPerformanceProfiler"), TEXT("Cannot end section with empty name"));
            return;
        }
        
        const float CurrentTime = FPlatformTime::Seconds();
        
        // Encontra as estatísticas da seção
        FSectionStats* Stats = SectionStats.Find(SectionName);
        if (!Stats)
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetPerformanceProfiler"), 
                FString::Printf(TEXT("Section %s not found or never started"), *SectionName));
            return;
        }
        
        if (!Stats->bIsActive)
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetPerformanceProfiler"), 
                FString::Printf(TEXT("Section %s is not active"), *SectionName));
            return;
        }
        
        // Calcula o tempo decorrido
        const float ElapsedTime = CurrentTime - Stats->LastStartTime;
        
        // Atualiza estatísticas
        Stats->TotalTime += ElapsedTime;
        Stats->CallCount++;
        Stats->MinTime = FMath::Min(Stats->MinTime, ElapsedTime);
        Stats->MaxTime = FMath::Max(Stats->MaxTime, ElapsedTime);
        Stats->bIsActive = false;
        
        UPlanetSystemLogger::LogDebug(TEXT("PlanetPerformanceProfiler"), 
            FString::Printf(TEXT("Section ended: %s (%.3f ms)"), *SectionName, ElapsedTime * 1000.0f));
        
        // Log de performance se o tempo for alto
        if (ElapsedTime > 0.016f) // Mais de 16ms (60 FPS)
        {
            UPlanetSystemLogger::LogPerformance(TEXT("PlanetPerformanceProfiler"), 
                FString::Printf(TEXT("Slow section detected: %s (%.3f ms)"), *SectionName, ElapsedTime * 1000.0f));
        }
        
        // Broadcast evento
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::ProfilingEnded, TEXT("Profiling"), SectionName, ElapsedTime);
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPerformanceProfiler"), 
            FString::Printf(TEXT("Exception ending section: %s"), UTF8_TO_TCHAR(e.what())));
    }
}

void UPlanetPerformanceProfiler::GetPerformanceStats(FString& OutStats)
{
    try
    {
        if (SectionStats.Num() == 0)
        {
            OutStats = TEXT("No performance data available");
            return;
        }
        
        const float TotalProfilingTime = FPlatformTime::Seconds() - ProfilingStartTime;
        
        FString StatsString = FString::Printf(
            TEXT("Performance Statistics (Total Time: %.2f seconds)\n")
            TEXT("==============================================\n\n"),
            TotalProfilingTime
        );
        
        // Ordena seções por tempo total (mais tempo primeiro)
        TArray<TPair<FString, FSectionStats>> SortedSections;
        for (const auto& Pair : SectionStats)
        {
            SortedSections.Add(TPair<FString, FSectionStats>(Pair.Key, Pair.Value));
        }
        
        SortedSections.Sort([](const TPair<FString, FSectionStats>& A, const TPair<FString, FSectionStats>& B) {
            return A.Value.TotalTime > B.Value.TotalTime;
        });
        
        // Gera estatísticas para cada seção
        for (const auto& Pair : SortedSections)
        {
            const FString& SectionName = Pair.Key;
            const FSectionStats& Stats = Pair.Value;
            
            const float AvgTime = Stats.CallCount > 0 ? Stats.TotalTime / Stats.CallCount : 0.0f;
            const float PercentageOfTotal = TotalProfilingTime > 0 ? (Stats.TotalTime / TotalProfilingTime) * 100.0f : 0.0f;
            
            StatsString += FString::Printf(
                TEXT("Section: %s\n")
                TEXT("  Total Time: %.3f ms (%.1f%% of total)\n")
                TEXT("  Call Count: %d\n")
                TEXT("  Average Time: %.3f ms\n")
                TEXT("  Min Time: %.3f ms\n")
                TEXT("  Max Time: %.3f ms\n")
                TEXT("  Active: %s\n\n"),
                *SectionName,
                Stats.TotalTime * 1000.0f, PercentageOfTotal,
                Stats.CallCount,
                AvgTime * 1000.0f,
                Stats.MinTime * 1000.0f,
                Stats.MaxTime * 1000.0f,
                Stats.bIsActive ? TEXT("Yes") : TEXT("No")
            );
        }
        
        // Adiciona estatísticas gerais
        int32 TotalCalls = 0;
        float TotalTime = 0.0f;
        int32 ActiveSections = 0;
        
        for (const auto& Pair : SectionStats)
        {
            const FSectionStats& Stats = Pair.Value;
            TotalCalls += Stats.CallCount;
            TotalTime += Stats.TotalTime;
            if (Stats.bIsActive) ActiveSections++;
        }
        
        StatsString += FString::Printf(
            TEXT("General Statistics:\n")
            TEXT("  Total Sections: %d\n")
            TEXT("  Active Sections: %d\n")
            TEXT("  Total Calls: %d\n")
            TEXT("  Total Time: %.3f ms\n")
            TEXT("  Profiling Enabled: %s\n"),
            SectionStats.Num(), ActiveSections, TotalCalls, TotalTime * 1000.0f,
            bProfilingEnabled ? TEXT("Yes") : TEXT("No")
        );
        
        OutStats = StatsString;
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetPerformanceProfiler"), TEXT("Performance statistics retrieved"));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPerformanceProfiler"), 
            FString::Printf(TEXT("Exception getting performance stats: %s"), UTF8_TO_TCHAR(e.what())));
        OutStats = TEXT("Error retrieving performance statistics");
    }
}

void UPlanetPerformanceProfiler::SetProfilingEnabled(bool bEnable)
{
    try
    {
        bProfilingEnabled = bEnable;
        
        if (bEnable)
        {
            ProfilingStartTime = FPlatformTime::Seconds();
            UPlanetSystemLogger::LogInfo(TEXT("PlanetPerformanceProfiler"), TEXT("Performance profiling enabled"));
            
            UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                EPlanetEventType::ProfilingEnabled, TEXT("Profiling"));
        }
        else
        {
            UPlanetSystemLogger::LogInfo(TEXT("PlanetPerformanceProfiler"), TEXT("Performance profiling disabled"));
            
            UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                EPlanetEventType::ProfilingDisabled, TEXT("Profiling"));
        }
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPerformanceProfiler"), 
            FString::Printf(TEXT("Exception setting profiling enabled: %s"), UTF8_TO_TCHAR(e.what())));
    }
}

bool UPlanetPerformanceProfiler::IsProfilingEnabled() const
{
    return bProfilingEnabled;
}

void UPlanetPerformanceProfiler::ClearStats()
{
    try
    {
        // Finaliza todas as seções ativas
        TArray<FString> ActiveSections;
        for (const auto& Pair : SectionStats)
        {
            if (Pair.Value.bIsActive)
            {
                ActiveSections.Add(Pair.Key);
            }
        }
        
        for (const FString& SectionName : ActiveSections)
        {
            EndSection(SectionName);
        }
        
        // Limpa todas as estatísticas
        SectionStats.Empty();
        ProfilingStartTime = FPlatformTime::Seconds();
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetPerformanceProfiler"), TEXT("Performance statistics cleared"));
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::ProfilingCleared, TEXT("Profiling"));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPerformanceProfiler"), 
            FString::Printf(TEXT("Exception clearing stats: %s"), UTF8_TO_TCHAR(e.what())));
    }
}

bool UPlanetPerformanceProfiler::ExportStats(const FString& Filename)
{
    try
    {
        if (SectionStats.Num() == 0)
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetPerformanceProfiler"), TEXT("No statistics to export"));
            return false;
        }
        
        FString FullPath = FPaths::ProjectSavedDir() / TEXT("PlanetSystem/Profiling") / Filename;
        
        // Cria o diretório se não existir
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        PlatformFile.CreateDirectoryTree(*FPaths::GetPath(FullPath));
        
        // Serializa estatísticas para JSON
        FString JsonString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
        
        Writer->WriteObjectStart();
        
        // Informações gerais
        Writer->WriteValue(TEXT("ExportTime"), FDateTime::Now().ToString());
        Writer->WriteValue(TEXT("TotalProfilingTime"), FPlatformTime::Seconds() - ProfilingStartTime);
        Writer->WriteValue(TEXT("ProfilingEnabled"), bProfilingEnabled);
        Writer->WriteValue(TEXT("TotalSections"), SectionStats.Num());
        
        // Estatísticas por seção
        Writer->WriteArrayStart(TEXT("Sections"));
        
        for (const auto& Pair : SectionStats)
        {
            const FString& SectionName = Pair.Key;
            const FSectionStats& Stats = Pair.Value;
            
            Writer->WriteObjectStart();
            Writer->WriteValue(TEXT("Name"), SectionName);
            Writer->WriteValue(TEXT("TotalTime"), Stats.TotalTime);
            Writer->WriteValue(TEXT("CallCount"), Stats.CallCount);
            Writer->WriteValue(TEXT("MinTime"), Stats.MinTime);
            Writer->WriteValue(TEXT("MaxTime"), Stats.MaxTime);
            Writer->WriteValue(TEXT("AverageTime"), Stats.CallCount > 0 ? Stats.TotalTime / Stats.CallCount : 0.0f);
            Writer->WriteValue(TEXT("IsActive"), Stats.bIsActive);
            Writer->WriteObjectEnd();
        }
        
        Writer->WriteArrayEnd();
        
        // Estatísticas agregadas
        Writer->WriteObjectStart(TEXT("AggregatedStats"));
        
        int32 TotalCalls = 0;
        float TotalTime = 0.0f;
        int32 ActiveSections = 0;
        float MinTime = FLT_MAX;
        float MaxTime = 0.0f;
        
        for (const auto& Pair : SectionStats)
        {
            const FSectionStats& Stats = Pair.Value;
            TotalCalls += Stats.CallCount;
            TotalTime += Stats.TotalTime;
            if (Stats.bIsActive) ActiveSections++;
            MinTime = FMath::Min(MinTime, Stats.MinTime);
            MaxTime = FMath::Max(MaxTime, Stats.MaxTime);
        }
        
        Writer->WriteValue(TEXT("TotalCalls"), TotalCalls);
        Writer->WriteValue(TEXT("TotalTime"), TotalTime);
        Writer->WriteValue(TEXT("ActiveSections"), ActiveSections);
        Writer->WriteValue(TEXT("MinTime"), MinTime);
        Writer->WriteValue(TEXT("MaxTime"), MaxTime);
        Writer->WriteValue(TEXT("AverageTime"), TotalCalls > 0 ? TotalTime / TotalCalls : 0.0f);
        
        Writer->WriteObjectEnd();
        
        Writer->WriteObjectEnd();
        Writer->Close();
        
        // Salva o arquivo
        if (FFileHelper::SaveStringToFile(JsonString, *FullPath))
        {
            UPlanetSystemLogger::LogInfo(TEXT("PlanetPerformanceProfiler"), 
                FString::Printf(TEXT("Performance statistics exported to: %s"), *FullPath));
            
            UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                EPlanetEventType::ProfilingExported, TEXT("Profiling"), Filename);
            
            return true;
        }
        
        UPlanetSystemLogger::LogError(TEXT("PlanetPerformanceProfiler"), 
            FString::Printf(TEXT("Failed to export performance statistics to: %s"), *FullPath));
        return false;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPerformanceProfiler"), 
            FString::Printf(TEXT("Exception exporting stats: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

// Funções auxiliares para profiling automático

void UPlanetPerformanceProfiler::BeginFrame()
{
    BeginSection(TEXT("Frame"));
}

void UPlanetPerformanceProfiler::EndFrame()
{
    EndSection(TEXT("Frame"));
}

void UPlanetPerformanceProfiler::BeginChunkGeneration(const FVector& Center, int32 LODLevel)
{
    FString SectionName = FString::Printf(TEXT("ChunkGeneration_%d_%d_%d_LOD%d"), 
        FMath::RoundToInt(Center.X), FMath::RoundToInt(Center.Y), FMath::RoundToInt(Center.Z), LODLevel);
    BeginSection(SectionName);
}

void UPlanetPerformanceProfiler::EndChunkGeneration(const FVector& Center, int32 LODLevel)
{
    FString SectionName = FString::Printf(TEXT("ChunkGeneration_%d_%d_%d_LOD%d"), 
        FMath::RoundToInt(Center.X), FMath::RoundToInt(Center.Y), FMath::RoundToInt(Center.Z), LODLevel);
    EndSection(SectionName);
}

void UPlanetPerformanceProfiler::BeginNoiseGeneration(const FString& NoiseType)
{
    FString SectionName = FString::Printf(TEXT("NoiseGeneration_%s"), *NoiseType);
    BeginSection(SectionName);
}

void UPlanetPerformanceProfiler::EndNoiseGeneration(const FString& NoiseType)
{
    FString SectionName = FString::Printf(TEXT("NoiseGeneration_%s"), *NoiseType);
    EndSection(SectionName);
}

void UPlanetPerformanceProfiler::BeginBiomeCalculation(const FVector& Position)
{
    FString SectionName = FString::Printf(TEXT("BiomeCalculation_%d_%d_%d"), 
        FMath::RoundToInt(Position.X), FMath::RoundToInt(Position.Y), FMath::RoundToInt(Position.Z));
    BeginSection(SectionName);
}

void UPlanetPerformanceProfiler::EndBiomeCalculation(const FVector& Position)
{
    FString SectionName = FString::Printf(TEXT("BiomeCalculation_%d_%d_%d"), 
        FMath::RoundToInt(Position.X), FMath::RoundToInt(Position.Y), FMath::RoundToInt(Position.Z));
    EndSection(SectionName);
}

void UPlanetPerformanceProfiler::BeginErosionSimulation(const FString& ErosionType)
{
    FString SectionName = FString::Printf(TEXT("ErosionSimulation_%s"), *ErosionType);
    BeginSection(SectionName);
}

void UPlanetPerformanceProfiler::EndErosionSimulation(const FString& ErosionType)
{
    FString SectionName = FString::Printf(TEXT("ErosionSimulation_%s"), *ErosionType);
    EndSection(SectionName);
}

void UPlanetPerformanceProfiler::BeginVegetationPlacement(const FString& VegetationType)
{
    FString SectionName = FString::Printf(TEXT("VegetationPlacement_%s"), *VegetationType);
    BeginSection(SectionName);
}

void UPlanetPerformanceProfiler::EndVegetationPlacement(const FString& VegetationType)
{
    FString SectionName = FString::Printf(TEXT("VegetationPlacement_%s"), *VegetationType);
    EndSection(SectionName);
}

void UPlanetPerformanceProfiler::BeginWaterSimulation(const FString& WaterType)
{
    FString SectionName = FString::Printf(TEXT("WaterSimulation_%s"), *WaterType);
    BeginSection(SectionName);
}

void UPlanetPerformanceProfiler::EndWaterSimulation(const FString& WaterType)
{
    FString SectionName = FString::Printf(TEXT("WaterSimulation_%s"), *WaterType);
    EndSection(SectionName);
}

void UPlanetPerformanceProfiler::BeginCacheOperation(const FString& OperationType)
{
    FString SectionName = FString::Printf(TEXT("CacheOperation_%s"), *OperationType);
    BeginSection(SectionName);
}

void UPlanetPerformanceProfiler::EndCacheOperation(const FString& OperationType)
{
    FString SectionName = FString::Printf(TEXT("CacheOperation_%s"), *OperationType);
    EndSection(SectionName);
}

void UPlanetPerformanceProfiler::BeginSerialization(const FString& DataType)
{
    FString SectionName = FString::Printf(TEXT("Serialization_%s"), *DataType);
    BeginSection(SectionName);
}

void UPlanetPerformanceProfiler::EndSerialization(const FString& DataType)
{
    FString SectionName = FString::Printf(TEXT("Serialization_%s"), *DataType);
    EndSection(SectionName);
}

void UPlanetPerformanceProfiler::BeginPresetOperation(const FString& OperationType)
{
    FString SectionName = FString::Printf(TEXT("PresetOperation_%s"), *OperationType);
    BeginSection(SectionName);
}

void UPlanetPerformanceProfiler::EndPresetOperation(const FString& OperationType)
{
    FString SectionName = FString::Printf(TEXT("PresetOperation_%s"), *OperationType);
    EndSection(SectionName);
}

// Funções para análise de performance

float UPlanetPerformanceProfiler::GetSectionTotalTime(const FString& SectionName) const
{
    const FSectionStats* Stats = SectionStats.Find(SectionName);
    return Stats ? Stats->TotalTime : 0.0f;
}

int32 UPlanetPerformanceProfiler::GetSectionCallCount(const FString& SectionName) const
{
    const FSectionStats* Stats = SectionStats.Find(SectionName);
    return Stats ? Stats->CallCount : 0;
}

float UPlanetPerformanceProfiler::GetSectionAverageTime(const FString& SectionName) const
{
    const FSectionStats* Stats = SectionStats.Find(SectionName);
    if (!Stats || Stats->CallCount == 0)
    {
        return 0.0f;
    }
    return Stats->TotalTime / Stats->CallCount;
}

float UPlanetPerformanceProfiler::GetSectionMinTime(const FString& SectionName) const
{
    const FSectionStats* Stats = SectionStats.Find(SectionName);
    return Stats ? Stats->MinTime : 0.0f;
}

float UPlanetPerformanceProfiler::GetSectionMaxTime(const FString& SectionName) const
{
    const FSectionStats* Stats = SectionStats.Find(SectionName);
    return Stats ? Stats->MaxTime : 0.0f;
}

bool UPlanetPerformanceProfiler::IsSectionActive(const FString& SectionName) const
{
    const FSectionStats* Stats = SectionStats.Find(SectionName);
    return Stats ? Stats->bIsActive : false;
}

TArray<FString> UPlanetPerformanceProfiler::GetActiveSections() const
{
    TArray<FString> ActiveSections;
    for (const auto& Pair : SectionStats)
    {
        if (Pair.Value.bIsActive)
        {
            ActiveSections.Add(Pair.Key);
        }
    }
    return ActiveSections;
}

TArray<FString> UPlanetPerformanceProfiler::GetAllSections() const
{
    TArray<FString> AllSections;
    for (const auto& Pair : SectionStats)
    {
        AllSections.Add(Pair.Key);
    }
    return AllSections;
}

void UPlanetPerformanceProfiler::GetSlowestSections(int32 Count, TArray<TPair<FString, float>>& OutSlowestSections) const
{
    OutSlowestSections.Empty();
    
    // Cria lista de seções ordenadas por tempo total
    TArray<TPair<FString, float>> SortedSections;
    for (const auto& Pair : SectionStats)
    {
        SortedSections.Add(TPair<FString, float>(Pair.Key, Pair.Value.TotalTime));
    }
    
    // Ordena por tempo total (mais lento primeiro)
    SortedSections.Sort([](const TPair<FString, float>& A, const TPair<FString, float>& B) {
        return A.Value > B.Value;
    });
    
    // Retorna as N seções mais lentas
    Count = FMath::Min(Count, SortedSections.Num());
    for (int32 i = 0; i < Count; ++i)
    {
        OutSlowestSections.Add(SortedSections[i]);
    }
}

void UPlanetPerformanceProfiler::GetMostCalledSections(int32 Count, TArray<TPair<FString, int32>>& OutMostCalledSections) const
{
    OutMostCalledSections.Empty();
    
    // Cria lista de seções ordenadas por número de chamadas
    TArray<TPair<FString, int32>> SortedSections;
    for (const auto& Pair : SectionStats)
    {
        SortedSections.Add(TPair<FString, int32>(Pair.Key, Pair.Value.CallCount));
    }
    
    // Ordena por número de chamadas (mais chamadas primeiro)
    SortedSections.Sort([](const TPair<FString, int32>& A, const TPair<FString, int32>& B) {
        return A.Value > B.Value;
    });
    
    // Retorna as N seções mais chamadas
    Count = FMath::Min(Count, SortedSections.Num());
    for (int32 i = 0; i < Count; ++i)
    {
        OutMostCalledSections.Add(SortedSections[i]);
    }
}

float UPlanetPerformanceProfiler::GetTotalProfilingTime() const
{
    return FPlatformTime::Seconds() - ProfilingStartTime;
}

int32 UPlanetPerformanceProfiler::GetTotalSectionCount() const
{
    return SectionStats.Num();
}

int32 UPlanetPerformanceProfiler::GetTotalCallCount() const
{
    int32 TotalCalls = 0;
    for (const auto& Pair : SectionStats)
    {
        TotalCalls += Pair.Value.CallCount;
    }
    return TotalCalls;
}

float UPlanetPerformanceProfiler::GetTotalTime() const
{
    float TotalTime = 0.0f;
    for (const auto& Pair : SectionStats)
    {
        TotalTime += Pair.Value.TotalTime;
    }
    return TotalTime;
} 