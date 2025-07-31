#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ChunkCache.generated.h"

USTRUCT(BlueprintType)
struct FChunkData
{
    GENERATED_BODY()
    
    UPROPERTY()
    TArray<FVector> Vertices;
    
    UPROPERTY()
    TArray<int32> Indices;
    
    UPROPERTY()
    TArray<FVector> Normals;
    
    UPROPERTY()
    TArray<FVector2D> UVs;
    
    UPROPERTY()
    uint32 Seed;
    
    UPROPERTY()
    int32 LODLevel;
    
    UPROPERTY()
    FVector2D UVMin;
    
    UPROPERTY()
    FVector2D UVMax;
    
    UPROPERTY()
    double LastAccessTime;
    
    FChunkData()
        : Seed(0), LODLevel(0), LastAccessTime(0.0)
    {
    }
    
    bool IsValid() const
    {
        return Vertices.Num() > 0 && Indices.Num() > 0;
    }
    
    void UpdateAccessTime()
    {
        LastAccessTime = FPlatformTime::Seconds();
    }
};

USTRUCT(BlueprintType)
struct FChunkKey
{
    GENERATED_BODY()
    
    UPROPERTY()
    FVector2D UVMin;
    
    UPROPERTY()
    FVector2D UVMax;
    
    UPROPERTY()
    int32 LODLevel;
    
    UPROPERTY()
    uint32 Seed;
    
    FChunkKey()
        : LODLevel(0), Seed(0)
    {
    }
    
    FChunkKey(const FVector2D& InUVMin, const FVector2D& InUVMax, int32 InLODLevel, uint32 InSeed)
        : UVMin(InUVMin), UVMax(InUVMax), LODLevel(InLODLevel), Seed(InSeed)
    {
    }
    
    bool operator==(const FChunkKey& Other) const
    {
        return UVMin.Equals(Other.UVMin, 0.001f) && 
               UVMax.Equals(Other.UVMax, 0.001f) && 
               LODLevel == Other.LODLevel && 
               Seed == Other.Seed;
    }
    
    friend uint32 GetTypeHash(const FChunkKey& Key)
    {
        return FCrc::MemCrc32(&Key, sizeof(Key));
    }
};

UCLASS(Blueprintable, ClassGroup=(Procedural))
class PLANETSYSTEM_API UChunkCache : public UObject
{
    GENERATED_BODY()
    
private:
    UPROPERTY()
    TMap<FChunkKey, FChunkData> CachedChunks;
    
    UPROPERTY()
    int32 MaxCacheSize = 1000;
    
    UPROPERTY()
    double CacheTimeoutSeconds = 300.0; // 5 minutes
    
    UPROPERTY()
    bool bEnableCache = true;
    
public:
    UChunkCache();
    
    // Cache management
    UFUNCTION(BlueprintCallable, Category="ChunkCache")
    void SetMaxCacheSize(int32 NewMaxSize);
    
    UFUNCTION(BlueprintCallable, Category="ChunkCache")
    void SetCacheTimeout(double TimeoutSeconds);
    
    UFUNCTION(BlueprintCallable, Category="ChunkCache")
    void EnableCache(bool bEnable);
    
    // Cache operations
    UFUNCTION(BlueprintCallable, Category="ChunkCache")
    bool GetChunk(const FChunkKey& Key, FChunkData& OutChunkData);
    
    UFUNCTION(BlueprintCallable, Category="ChunkCache")
    void StoreChunk(const FChunkKey& Key, const FChunkData& ChunkData);
    
    UFUNCTION(BlueprintCallable, Category="ChunkCache")
    void RemoveChunk(const FChunkKey& Key);
    
    UFUNCTION(BlueprintCallable, Category="ChunkCache")
    void ClearCache();
    
    // Cache statistics
    UFUNCTION(BlueprintCallable, Category="ChunkCache")
    int32 GetCacheSize() const;
    
    UFUNCTION(BlueprintCallable, Category="ChunkCache")
    float GetCacheHitRate() const;
    
    UFUNCTION(BlueprintCallable, Category="ChunkCache")
    void GetCacheStats(int32& OutSize, int32& OutMaxSize, float& OutHitRate);
    
    // Maintenance
    UFUNCTION(BlueprintCallable, Category="ChunkCache")
    void CleanupExpiredChunks();
    
    UFUNCTION(BlueprintCallable, Category="ChunkCache")
    void OptimizeCache();
    
private:
    void EvictOldestChunks(int32 Count);
    double GetCurrentTime() const;
    
    // Statistics
    mutable int32 CacheHits = 0;
    mutable int32 CacheMisses = 0;
}; 