#include "Rendering/Chunks/ChunkCache.h"
#include "HAL/PlatformTime.h"
#include "Engine/Engine.h"

UChunkCache::UChunkCache()
{
    MaxCacheSize = 1000;
    CacheTimeoutSeconds = 300.0;
    bEnableCache = true;
    CacheHits = 0;
    CacheMisses = 0;
}

void UChunkCache::SetMaxCacheSize(int32 NewMaxSize)
{
    MaxCacheSize = FMath::Max(1, NewMaxSize);
    
    // If we're over the new limit, evict oldest chunks
    if (CachedChunks.Num() > MaxCacheSize)
    {
        EvictOldestChunks(CachedChunks.Num() - MaxCacheSize);
    }
}

void UChunkCache::SetCacheTimeout(double TimeoutSeconds)
{
    CacheTimeoutSeconds = FMath::Max(1.0, TimeoutSeconds);
}

void UChunkCache::EnableCache(bool bEnable)
{
    bEnableCache = bEnable;
    
    if (!bEnableCache)
    {
        ClearCache();
    }
}

bool UChunkCache::GetChunk(const FChunkKey& Key, FChunkData& OutChunkData)
{
    if (!bEnableCache)
    {
        CacheMisses++;
        return false;
    }
    
    if (FChunkData* FoundChunk = CachedChunks.Find(Key))
    {
        // Check if chunk has expired
        double CurrentTime = GetCurrentTime();
        if (CurrentTime - FoundChunk->LastAccessTime > CacheTimeoutSeconds)
        {
            CachedChunks.Remove(Key);
            CacheMisses++;
            return false;
        }
        
        OutChunkData = *FoundChunk;
        OutChunkData.UpdateAccessTime();
        CachedChunks[Key] = OutChunkData; // Update access time in cache
        CacheHits++;
        return true;
    }
    
    CacheMisses++;
    return false;
}

void UChunkCache::StoreChunk(const FChunkKey& Key, const FChunkData& ChunkData)
{
    if (!bEnableCache || !ChunkData.IsValid())
    {
        return;
    }
    
    // Check if we need to evict chunks before storing
    if (CachedChunks.Num() >= MaxCacheSize)
    {
        EvictOldestChunks(1);
    }
    
    FChunkData NewChunkData = ChunkData;
    NewChunkData.UpdateAccessTime();
    CachedChunks.Add(Key, NewChunkData);
}

void UChunkCache::RemoveChunk(const FChunkKey& Key)
{
    CachedChunks.Remove(Key);
}

void UChunkCache::ClearCache()
{
    CachedChunks.Empty();
    CacheHits = 0;
    CacheMisses = 0;
}

int32 UChunkCache::GetCacheSize() const
{
    return CachedChunks.Num();
}

float UChunkCache::GetCacheHitRate() const
{
    int32 TotalRequests = CacheHits + CacheMisses;
    if (TotalRequests == 0)
    {
        return 0.0f;
    }
    
    return static_cast<float>(CacheHits) / static_cast<float>(TotalRequests);
}

void UChunkCache::GetCacheStats(int32& OutSize, int32& OutMaxSize, float& OutHitRate)
{
    OutSize = GetCacheSize();
    OutMaxSize = MaxCacheSize;
    OutHitRate = GetCacheHitRate();
}

void UChunkCache::CleanupExpiredChunks()
{
    if (!bEnableCache)
    {
        return;
    }
    
    double CurrentTime = GetCurrentTime();
    TArray<FChunkKey> KeysToRemove;
    
    for (const auto& Pair : CachedChunks)
    {
        if (CurrentTime - Pair.Value.LastAccessTime > CacheTimeoutSeconds)
        {
            KeysToRemove.Add(Pair.Key);
        }
    }
    
    for (const FChunkKey& Key : KeysToRemove)
    {
        CachedChunks.Remove(Key);
    }
    
    if (KeysToRemove.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("ChunkCache: Cleaned up %d expired chunks"), KeysToRemove.Num());
    }
}

void UChunkCache::OptimizeCache()
{
    if (!bEnableCache)
    {
        return;
    }
    
    // Sort chunks by access time (oldest first)
    TArray<TPair<FChunkKey, double>> ChunksWithTime;
    
    for (const auto& Pair : CachedChunks)
    {
        ChunksWithTime.Add(TPair<FChunkKey, double>(Pair.Key, Pair.Value.LastAccessTime));
    }
    
    ChunksWithTime.Sort([](const TPair<FChunkKey, double>& A, const TPair<FChunkKey, double>& B)
    {
        return A.Value < B.Value;
    });
    
    // Remove oldest 10% of chunks if we're over 80% capacity
    int32 CurrentSize = CachedChunks.Num();
    int32 TargetSize = static_cast<int32>(MaxCacheSize * 0.8f);
    
    if (CurrentSize > TargetSize)
    {
        int32 ChunksToRemove = CurrentSize - TargetSize;
        int32 RemoveCount = FMath::Min(ChunksToRemove, ChunksWithTime.Num());
        
        for (int32 i = 0; i < RemoveCount; ++i)
        {
            CachedChunks.Remove(ChunksWithTime[i].Key);
        }
        
        UE_LOG(LogTemp, Log, TEXT("ChunkCache: Optimized cache, removed %d old chunks"), RemoveCount);
    }
}

void UChunkCache::EvictOldestChunks(int32 Count)
{
    if (Count <= 0 || CachedChunks.Num() == 0)
    {
        return;
    }
    
    // Find oldest chunks
    TArray<TPair<FChunkKey, double>> ChunksWithTime;
    
    for (const auto& Pair : CachedChunks)
    {
        ChunksWithTime.Add(TPair<FChunkKey, double>(Pair.Key, Pair.Value.LastAccessTime));
    }
    
    ChunksWithTime.Sort([](const TPair<FChunkKey, double>& A, const TPair<FChunkKey, double>& B)
    {
        return A.Value < B.Value;
    });
    
    int32 RemoveCount = FMath::Min(Count, ChunksWithTime.Num());
    
    for (int32 i = 0; i < RemoveCount; ++i)
    {
        CachedChunks.Remove(ChunksWithTime[i].Key);
    }
    
    UE_LOG(LogTemp, Log, TEXT("ChunkCache: Evicted %d oldest chunks"), RemoveCount);
}

double UChunkCache::GetCurrentTime() const
{
    return FPlatformTime::Seconds();
} 