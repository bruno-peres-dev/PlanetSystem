#include "Rendering/Chunks/PlanetChunkCache.h"
#include "Debug/Logging/PlanetSystemLogger.h"
#include "Core/Events/PlanetEventBus.h"
#include "Debug/Profiling/PlanetPerformanceProfiler.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"

// Instância singleton
UPlanetChunkCache* UPlanetChunkCache::Instance = nullptr;

UPlanetChunkCache::UPlanetChunkCache()
{
    UPlanetSystemLogger::LogInfo(TEXT("PlanetChunkCache"), TEXT("Advanced chunk cache created"));
}

UPlanetChunkCache* UPlanetChunkCache::GetInstance()
{
    if (!Instance)
    {
        Instance = NewObject<UPlanetChunkCache>();
        Instance->AddToRoot(); // Previne garbage collection
        UPlanetSystemLogger::LogInfo(TEXT("PlanetChunkCache"), TEXT("Advanced chunk cache singleton created"));
    }
    return Instance;
}

bool UPlanetChunkCache::AddChunk(const FString& ChunkKey, const FPlanetChunk& Chunk, float Priority)
{
    try
    {
        if (!bCacheEnabled)
        {
            UPlanetSystemLogger::LogDebug(TEXT("PlanetChunkCache"), TEXT("Cache is disabled, chunk not added"));
            return false;
        }
        
        if (ChunkKey.IsEmpty())
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetChunkCache"), TEXT("Cannot add chunk with empty key"));
            return false;
        }
        
        UPlanetPerformanceProfiler::GetInstance()->BeginCacheOperation(TEXT("AddChunk"));
        
        // Verifica se o chunk já existe
        if (CacheEntries.Contains(ChunkKey))
        {
            UPlanetSystemLogger::LogDebug(TEXT("PlanetChunkCache"), 
                FString::Printf(TEXT("Chunk already exists in cache: %s"), *ChunkKey));
            
            // Atualiza o chunk existente
            FCacheEntry& ExistingEntry = CacheEntries[ChunkKey];
            ExistingEntry.Chunk = Chunk;
            ExistingEntry.Priority = Priority;
            ExistingEntry.LastAccessTime = FPlatformTime::Seconds();
            ExistingEntry.AccessCount++;
            
            UPlanetPerformanceProfiler::GetInstance()->EndCacheOperation(TEXT("AddChunk"));
            return true;
        }
        
        // Calcula o tamanho estimado do chunk
        int32 EstimatedSize = EstimateChunkSize(Chunk);
        
        // Verifica se há espaço suficiente
        if (CurrentCacheSizeBytes + EstimatedSize > MaxCacheSizeMB * 1024 * 1024)
        {
            // Otimiza o cache para liberar espaço
            if (!OptimizeCacheForNewChunk(EstimatedSize))
            {
                UPlanetSystemLogger::LogWarning(TEXT("PlanetChunkCache"), 
                    FString::Printf(TEXT("Cannot add chunk %s - insufficient cache space"), *ChunkKey));
                
                UPlanetPerformanceProfiler::GetInstance()->EndCacheOperation(TEXT("AddChunk"));
                return false;
            }
        }
        
        // Adiciona o novo chunk
        FCacheEntry NewEntry(Chunk, Priority);
        NewEntry.SizeInBytes = EstimatedSize;
        CacheEntries.Add(ChunkKey, NewEntry);
        
        CurrentCacheSizeBytes += EstimatedSize;
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Chunk added to cache: %s (Priority: %.2f, Size: %d bytes)"), 
                *ChunkKey, Priority, EstimatedSize));
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::ChunkCached, TEXT("ChunkCache"), ChunkKey, Priority, EstimatedSize);
        
        UPlanetPerformanceProfiler::GetInstance()->EndCacheOperation(TEXT("AddChunk"));
        return true;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Exception adding chunk: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

FPlanetChunk* UPlanetChunkCache::GetChunk(const FString& ChunkKey)
{
    try
    {
        if (!bCacheEnabled)
        {
            UPlanetSystemLogger::LogDebug(TEXT("PlanetChunkCache"), TEXT("Cache is disabled, chunk not found"));
            return nullptr;
        }
        
        if (ChunkKey.IsEmpty())
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetChunkCache"), TEXT("Cannot get chunk with empty key"));
            return nullptr;
        }
        
        UPlanetPerformanceProfiler::GetInstance()->BeginCacheOperation(TEXT("GetChunk"));
        
        FCacheEntry* Entry = CacheEntries.Find(ChunkKey);
        if (Entry)
        {
            // Atualiza estatísticas de acesso
            Entry->LastAccessTime = FPlatformTime::Seconds();
            Entry->AccessCount++;
            TotalHits++;
            
            UPlanetSystemLogger::LogDebug(TEXT("PlanetChunkCache"), 
                FString::Printf(TEXT("Cache hit: %s (Access count: %d)"), *ChunkKey, Entry->AccessCount));
            
            UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                EPlanetEventType::ChunkCacheHit, TEXT("ChunkCache"), ChunkKey, 0.0f, Entry->AccessCount);
            
            UPlanetPerformanceProfiler::GetInstance()->EndCacheOperation(TEXT("GetChunk"));
            return &Entry->Chunk;
        }
        else
        {
            TotalMisses++;
            
            UPlanetSystemLogger::LogDebug(TEXT("PlanetChunkCache"), 
                FString::Printf(TEXT("Cache miss: %s"), *ChunkKey));
            
            UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                EPlanetEventType::ChunkCacheMiss, TEXT("ChunkCache"), ChunkKey);
            
            UPlanetPerformanceProfiler::GetInstance()->EndCacheOperation(TEXT("GetChunk"));
            return nullptr;
        }
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Exception getting chunk: %s"), UTF8_TO_TCHAR(e.what())));
        return nullptr;
    }
}

bool UPlanetChunkCache::RemoveChunk(const FString& ChunkKey)
{
    try
    {
        if (ChunkKey.IsEmpty())
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetChunkCache"), TEXT("Cannot remove chunk with empty key"));
            return false;
        }
        
        UPlanetPerformanceProfiler::GetInstance()->BeginCacheOperation(TEXT("RemoveChunk"));
        
        FCacheEntry* Entry = CacheEntries.Find(ChunkKey);
        if (Entry)
        {
            int32 RemovedSize = Entry->SizeInBytes;
            CacheEntries.Remove(ChunkKey);
            CurrentCacheSizeBytes -= RemovedSize;
            
            UPlanetSystemLogger::LogInfo(TEXT("PlanetChunkCache"), 
                FString::Printf(TEXT("Chunk removed from cache: %s (Freed: %d bytes)"), *ChunkKey, RemovedSize));
            
            UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                EPlanetEventType::ChunkRemoved, TEXT("ChunkCache"), ChunkKey, 0.0f, RemovedSize);
            
            UPlanetPerformanceProfiler::GetInstance()->EndCacheOperation(TEXT("RemoveChunk"));
            return true;
        }
        else
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetChunkCache"), 
                FString::Printf(TEXT("Chunk not found in cache: %s"), *ChunkKey));
            
            UPlanetPerformanceProfiler::GetInstance()->EndCacheOperation(TEXT("RemoveChunk"));
            return false;
        }
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Exception removing chunk: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

bool UPlanetChunkCache::HasChunk(const FString& ChunkKey) const
{
    try
    {
        if (!bCacheEnabled)
        {
            return false;
        }
        
        if (ChunkKey.IsEmpty())
        {
            return false;
        }
        
        bool bHasChunk = CacheEntries.Contains(ChunkKey);
        
        UPlanetSystemLogger::LogDebug(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Chunk existence check: %s -> %s"), *ChunkKey, bHasChunk ? TEXT("true") : TEXT("false")));
        
        return bHasChunk;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Exception checking chunk existence: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

void UPlanetChunkCache::GetCacheStats(FString& OutStats)
{
    try
    {
        const float HitRate = (TotalHits + TotalMisses) > 0 ? (float)TotalHits / (TotalHits + TotalMisses) * 100.0f : 0.0f;
        const float CacheUsagePercent = MaxCacheSizeMB > 0 ? (float)CurrentCacheSizeBytes / (MaxCacheSizeMB * 1024 * 1024) * 100.0f : 0.0f;
        
        OutStats = FString::Printf(
            TEXT("Advanced Chunk Cache Statistics:\n")
            TEXT("================================\n\n")
            TEXT("Cache Status:\n")
            TEXT("- Enabled: %s\n")
            TEXT("- Total Entries: %d\n")
            TEXT("- Current Size: %.2f MB / %.2f MB (%.1f%%)\n")
            TEXT("- Hit Rate: %.1f%% (%d hits, %d misses)\n\n")
            TEXT("Performance:\n")
            TEXT("- Total Hits: %d\n")
            TEXT("- Total Misses: %d\n")
            TEXT("- Last Optimization: %s\n\n")
            TEXT("Memory Management:\n")
            TEXT("- Max Cache Size: %.2f MB\n")
            TEXT("- Current Usage: %.2f MB\n")
            TEXT("- Available Space: %.2f MB\n"),
            bCacheEnabled ? TEXT("Yes") : TEXT("No"),
            CacheEntries.Num(),
            CurrentCacheSizeBytes / (1024.0f * 1024.0f), MaxCacheSizeMB, CacheUsagePercent,
            HitRate, TotalHits, TotalMisses,
            LastOptimizationTime > 0 ? *FDateTime::FromUnixTimestamp(LastOptimizationTime).ToString() : TEXT("Never"),
            TotalHits, TotalMisses,
            MaxCacheSizeMB,
            CurrentCacheSizeBytes / (1024.0f * 1024.0f),
            (MaxCacheSizeMB * 1024 * 1024 - CurrentCacheSizeBytes) / (1024.0f * 1024.0f)
        );
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetChunkCache"), TEXT("Cache statistics retrieved"));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Exception getting cache stats: %s"), UTF8_TO_TCHAR(e.what())));
        OutStats = TEXT("Error retrieving cache statistics");
    }
}

void UPlanetChunkCache::ClearCache()
{
    try
    {
        UPlanetPerformanceProfiler::GetInstance()->BeginCacheOperation(TEXT("ClearCache"));
        
        int32 RemovedEntries = CacheEntries.Num();
        int64 FreedBytes = CurrentCacheSizeBytes;
        
        CacheEntries.Empty();
        CurrentCacheSizeBytes = 0;
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Cache cleared: %d entries removed, %.2f MB freed"), 
                RemovedEntries, FreedBytes / (1024.0f * 1024.0f)));
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::ChunkCacheCleared, TEXT("ChunkCache"), TEXT(""), 0.0f, RemovedEntries);
        
        UPlanetPerformanceProfiler::GetInstance()->EndCacheOperation(TEXT("ClearCache"));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Exception clearing cache: %s"), UTF8_TO_TCHAR(e.what())));
    }
}

void UPlanetChunkCache::OptimizeCache()
{
    try
    {
        UPlanetPerformanceProfiler::GetInstance()->BeginCacheOperation(TEXT("OptimizeCache"));
        
        const float StartTime = FPlatformTime::Seconds();
        int32 RemovedEntries = 0;
        int64 FreedBytes = 0;
        
        // Lista de chunks candidatos para remoção
        TArray<TPair<FString, float>> RemovalCandidates;
        
        for (const auto& Pair : CacheEntries)
        {
            const FString& ChunkKey = Pair.Key;
            const FCacheEntry& Entry = Pair.Value;
            
            // Calcula score de remoção baseado em:
            // - Tempo desde último acesso (mais antigo = maior score)
            // - Número de acessos (menos acessos = maior score)
            // - Prioridade (menor prioridade = maior score)
            // - Tamanho (maior tamanho = maior score)
            
            const float TimeSinceAccess = FPlatformTime::Seconds() - Entry.LastAccessTime;
            const float AccessScore = Entry.AccessCount > 0 ? 1.0f / Entry.AccessCount : 1.0f;
            const float PriorityScore = 1.0f - Entry.Priority; // Inverte prioridade
            const float SizeScore = Entry.SizeInBytes / (1024.0f * 1024.0f); // Normaliza por MB
            
            const float RemovalScore = TimeSinceAccess * AccessScore * PriorityScore * SizeScore;
            RemovalCandidates.Add(TPair<FString, float>(ChunkKey, RemovalScore));
        }
        
        // Ordena por score de remoção (maior score primeiro)
        RemovalCandidates.Sort([](const TPair<FString, float>& A, const TPair<FString, float>& B) {
            return A.Value > B.Value;
        });
        
        // Remove chunks até liberar pelo menos 20% do cache ou remover no máximo 30% dos chunks
        const int32 MaxRemovals = FMath::Min(CacheEntries.Num() / 3, CacheEntries.Num());
        const int64 TargetFreedBytes = CurrentCacheSizeBytes / 5; // 20% do cache
        
        for (int32 i = 0; i < MaxRemovals && FreedBytes < TargetFreedBytes; ++i)
        {
            const FString& ChunkKey = RemovalCandidates[i].Key;
            FCacheEntry* Entry = CacheEntries.Find(ChunkKey);
            
            if (Entry)
            {
                FreedBytes += Entry->SizeInBytes;
                CacheEntries.Remove(ChunkKey);
                RemovedEntries++;
            }
        }
        
        CurrentCacheSizeBytes -= FreedBytes;
        LastOptimizationTime = FPlatformTime::Seconds();
        
        const float OptimizationTime = FPlatformTime::Seconds() - StartTime;
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Cache optimization completed: %d entries removed, %.2f MB freed in %.3f seconds"), 
                RemovedEntries, FreedBytes / (1024.0f * 1024.0f), OptimizationTime));
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::ChunkCacheOptimized, TEXT("ChunkCache"), TEXT(""), OptimizationTime, RemovedEntries);
        
        UPlanetPerformanceProfiler::GetInstance()->EndCacheOperation(TEXT("OptimizeCache"));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Exception optimizing cache: %s"), UTF8_TO_TCHAR(e.what())));
    }
}

void UPlanetChunkCache::SetMaxCacheSize(int32 MaxSizeMB)
{
    try
    {
        if (MaxSizeMB <= 0)
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetChunkCache"), 
                FString::Printf(TEXT("Invalid max cache size: %d MB"), MaxSizeMB));
            return;
        }
        
        const int32 OldMaxSize = MaxCacheSizeMB;
        MaxCacheSizeMB = MaxSizeMB;
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Max cache size changed: %d MB -> %d MB"), OldMaxSize, MaxSizeMB));
        
        // Se o novo tamanho é menor que o uso atual, otimiza o cache
        if (CurrentCacheSizeBytes > MaxCacheSizeMB * 1024 * 1024)
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetChunkCache"), 
                TEXT("Current cache usage exceeds new max size, optimizing..."));
            OptimizeCache();
        }
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::ChunkCacheResized, TEXT("ChunkCache"), TEXT(""), 0.0f, MaxSizeMB);
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Exception setting max cache size: %s"), UTF8_TO_TCHAR(e.what())));
    }
}

int32 UPlanetChunkCache::GetCurrentCacheSize() const
{
    return CurrentCacheSizeBytes;
}

void UPlanetChunkCache::SetCacheEnabled(bool bEnable)
{
    try
    {
        bCacheEnabled = bEnable;
        
        if (bEnable)
        {
            UPlanetSystemLogger::LogInfo(TEXT("PlanetChunkCache"), TEXT("Chunk cache enabled"));
            
            UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                EPlanetEventType::ChunkCacheEnabled, TEXT("ChunkCache"));
        }
        else
        {
            UPlanetSystemLogger::LogInfo(TEXT("PlanetChunkCache"), TEXT("Chunk cache disabled"));
            
            UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                EPlanetEventType::ChunkCacheDisabled, TEXT("ChunkCache"));
        }
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Exception setting cache enabled: %s"), UTF8_TO_TCHAR(e.what())));
    }
}

bool UPlanetChunkCache::IsCacheEnabled() const
{
    return bCacheEnabled;
}

// Funções auxiliares

int32 UPlanetChunkCache::EstimateChunkSize(const FPlanetChunk& Chunk) const
{
    try
    {
        // Estimativa básica do tamanho do chunk
        // Em uma implementação real, você calcularia o tamanho real baseado nos dados do chunk
        
        int32 EstimatedSize = 0;
        
        // Tamanho base para estrutura do chunk
        EstimatedSize += sizeof(FPlanetChunk);
        
        // Adiciona tamanho estimado para vértices, índices, etc.
        // Esta é uma estimativa simplificada
        EstimatedSize += 1024; // 1KB base
        
        return EstimatedSize;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Exception estimating chunk size: %s"), UTF8_TO_TCHAR(e.what())));
        return 1024; // Tamanho padrão em caso de erro
    }
}

bool UPlanetChunkCache::OptimizeCacheForNewChunk(int32 RequiredSize)
{
    try
    {
        // Se o chunk requerido é maior que o cache inteiro, não pode ser adicionado
        if (RequiredSize > MaxCacheSizeMB * 1024 * 1024)
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetChunkCache"), 
                FString::Printf(TEXT("Required chunk size (%d bytes) exceeds max cache size"), RequiredSize));
            return false;
        }
        
        // Calcula quanto espaço precisa ser liberado
        const int64 RequiredFreedSpace = RequiredSize - (MaxCacheSizeMB * 1024 * 1024 - CurrentCacheSizeBytes);
        
        if (RequiredFreedSpace <= 0)
        {
            return true; // Há espaço suficiente
        }
        
        // Lista chunks candidatos para remoção
        TArray<TPair<FString, float>> RemovalCandidates;
        
        for (const auto& Pair : CacheEntries)
        {
            const FString& ChunkKey = Pair.Key;
            const FCacheEntry& Entry = Pair.Value;
            
            // Score baseado em prioridade e tempo de acesso
            const float TimeSinceAccess = FPlatformTime::Seconds() - Entry.LastAccessTime;
            const float PriorityScore = 1.0f - Entry.Priority;
            const float RemovalScore = TimeSinceAccess * PriorityScore;
            
            RemovalCandidates.Add(TPair<FString, float>(ChunkKey, RemovalScore));
        }
        
        // Ordena por score de remoção
        RemovalCandidates.Sort([](const TPair<FString, float>& A, const TPair<FString, float>& B) {
            return A.Value > B.Value;
        });
        
        // Remove chunks até liberar espaço suficiente
        int64 FreedSpace = 0;
        for (const auto& Candidate : RemovalCandidates)
        {
            FCacheEntry* Entry = CacheEntries.Find(Candidate.Key);
            if (Entry)
            {
                FreedSpace += Entry->SizeInBytes;
                CacheEntries.Remove(Candidate.Key);
                
                if (FreedSpace >= RequiredFreedSpace)
                {
                    break;
                }
            }
        }
        
        CurrentCacheSizeBytes -= FreedSpace;
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Cache optimized for new chunk: %.2f MB freed"), FreedSpace / (1024.0f * 1024.0f)));
        
        return FreedSpace >= RequiredFreedSpace;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetChunkCache"), 
            FString::Printf(TEXT("Exception optimizing cache for new chunk: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

// Funções para análise de cache

TArray<FString> UPlanetChunkCache::GetAllChunkKeys() const
{
    TArray<FString> Keys;
    CacheEntries.GetKeys(Keys);
    return Keys;
}

int32 UPlanetChunkCache::GetChunkAccessCount(const FString& ChunkKey) const
{
    const FCacheEntry* Entry = CacheEntries.Find(ChunkKey);
    return Entry ? Entry->AccessCount : 0;
}

float UPlanetChunkCache::GetChunkPriority(const FString& ChunkKey) const
{
    const FCacheEntry* Entry = CacheEntries.Find(ChunkKey);
    return Entry ? Entry->Priority : 0.0f;
}

float UPlanetChunkCache::GetChunkLastAccessTime(const FString& ChunkKey) const
{
    const FCacheEntry* Entry = CacheEntries.Find(ChunkKey);
    return Entry ? Entry->LastAccessTime : 0.0f;
}

int32 UPlanetChunkCache::GetChunkSize(const FString& ChunkKey) const
{
    const FCacheEntry* Entry = CacheEntries.Find(ChunkKey);
    return Entry ? Entry->SizeInBytes : 0;
}

void UPlanetChunkCache::GetMostAccessedChunks(int32 Count, TArray<TPair<FString, int32>>& OutMostAccessed) const
{
    OutMostAccessed.Empty();
    
    TArray<TPair<FString, int32>> SortedChunks;
    for (const auto& Pair : CacheEntries)
    {
        SortedChunks.Add(TPair<FString, int32>(Pair.Key, Pair.Value.AccessCount));
    }
    
    SortedChunks.Sort([](const TPair<FString, int32>& A, const TPair<FString, int32>& B) {
        return A.Value > B.Value;
    });
    
    Count = FMath::Min(Count, SortedChunks.Num());
    for (int32 i = 0; i < Count; ++i)
    {
        OutMostAccessed.Add(SortedChunks[i]);
    }
}

void UPlanetChunkCache::GetLargestChunks(int32 Count, TArray<TPair<FString, int32>>& OutLargestChunks) const
{
    OutLargestChunks.Empty();
    
    TArray<TPair<FString, int32>> SortedChunks;
    for (const auto& Pair : CacheEntries)
    {
        SortedChunks.Add(TPair<FString, int32>(Pair.Key, Pair.Value.SizeInBytes));
    }
    
    SortedChunks.Sort([](const TPair<FString, int32>& A, const TPair<FString, int32>& B) {
        return A.Value > B.Value;
    });
    
    Count = FMath::Min(Count, SortedChunks.Num());
    for (int32 i = 0; i < Count; ++i)
    {
        OutLargestChunks.Add(SortedChunks[i]);
    }
}

void UPlanetChunkCache::GetOldestChunks(int32 Count, TArray<TPair<FString, float>>& OutOldestChunks) const
{
    OutOldestChunks.Empty();
    
    TArray<TPair<FString, float>> SortedChunks;
    for (const auto& Pair : CacheEntries)
    {
        SortedChunks.Add(TPair<FString, float>(Pair.Key, Pair.Value.LastAccessTime));
    }
    
    SortedChunks.Sort([](const TPair<FString, float>& A, const TPair<FString, float>& B) {
        return A.Value < B.Value; // Ordena por tempo mais antigo primeiro
    });
    
    Count = FMath::Min(Count, SortedChunks.Num());
    for (int32 i = 0; i < Count; ++i)
    {
        OutOldestChunks.Add(SortedChunks[i]);
    }
}

float UPlanetChunkCache::GetHitRate() const
{
    const int32 TotalRequests = TotalHits + TotalMisses;
    return TotalRequests > 0 ? (float)TotalHits / TotalRequests * 100.0f : 0.0f;
}

int32 UPlanetChunkCache::GetTotalHits() const
{
    return TotalHits;
}

int32 UPlanetChunkCache::GetTotalMisses() const
{
    return TotalMisses;
}

int32 UPlanetChunkCache::GetEntryCount() const
{
    return CacheEntries.Num();
}

float UPlanetChunkCache::GetCacheUsagePercent() const
{
    return MaxCacheSizeMB > 0 ? (float)CurrentCacheSizeBytes / (MaxCacheSizeMB * 1024 * 1024) * 100.0f : 0.0f;
} 