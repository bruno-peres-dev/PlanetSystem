#include "Network/PlanetChunkNetworkCache.h"
#include "Debug/Logging/PlanetSystemLogger.h"
#include "Core/Events/PlanetEventBus.h"
#include "Configuration/DataAssets/CoreConfig.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

UPlanetChunkNetworkCache::UPlanetChunkNetworkCache()
{
    // Obter serviços do ServiceLocator
    Logger = UPlanetSystemLogger::GetInstance();
    EventBus = UPlanetEventBus::GetInstance();
    
    // Inicializar configuração
    MaxCacheSize = 1000;
    ReplacementPolicy = EPlanetCachePolicy::LRU;
    MaxChunkAge = 300.0f; // 5 minutos
    
    // Inicializar estatísticas
    CacheHits = 0;
    CacheMisses = 0;
    ChunksSynchronized = 0;
    ChunksRemoved = 0;
    TotalOperationTime = 0.0f;
    
    // Log de inicialização
    if (Logger)
    {
        Logger->LogInfo(TEXT("PlanetChunkNetworkCache"), TEXT("Sistema de cache distribuído inicializado"));
    }
}

void UPlanetChunkNetworkCache::SynchronizeChunk(const FVector& Position, const FPlanetChunk& Chunk)
{
    const double StartTime = FPlatformTime::Seconds();
    
    try
    {
        if (!ValidatePosition(Position))
        {
            LogCacheEvent(EPlanetEventType::Error, TEXT("Posição inválida para sincronização"));
            return;
        }
        
        // Verificar se cache está cheio
        if (IsCacheFull())
        {
            // Aplicar política de substituição
            switch (ReplacementPolicy)
            {
                case EPlanetCachePolicy::LRU:
                    RemoveLeastRecentlyUsed();
                    break;
                case EPlanetCachePolicy::LFU:
                    RemoveLeastFrequentlyUsed();
                    break;
                case EPlanetCachePolicy::Random:
                    RemoveRandomChunk();
                    break;
                default:
                    RemoveLeastRecentlyUsed();
                    break;
            }
        }
        
        // Armazenar chunk
        NetworkedChunks.Add(Position, Chunk);
        ChunkTimestamps.Add(Position, FPlatformTime::Seconds());
        ChunkAccessCounts.Add(Position, 0);
        
        // Atualizar ordem de acesso
        AccessOrder.Remove(Position);
        AccessOrder.Add(Position);
        
        ChunksSynchronized++;
        
        // Calcular tempo de operação
        const double EndTime = FPlatformTime::Seconds();
        TotalOperationTime += static_cast<float>(EndTime - StartTime);
        
        LogCacheEvent(EPlanetEventType::Info, FString::Printf(TEXT("Chunk sincronizado: %s"), *Position.ToString()));
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro na sincronização de chunk: %s"), UTF8_TO_TCHAR(e.what()));
        LogCacheEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

bool UPlanetChunkNetworkCache::GetChunk(const FVector& Position, FPlanetChunk& OutChunk)
{
    const double StartTime = FPlatformTime::Seconds();
    
    try
    {
        if (!ValidatePosition(Position))
        {
            LogCacheEvent(EPlanetEventType::Error, TEXT("Posição inválida para busca"));
            return false;
        }
        
        // Buscar chunk no cache
        FPlanetChunk* FoundChunk = NetworkedChunks.Find(Position);
        if (FoundChunk)
        {
            OutChunk = *FoundChunk;
            
            // Atualizar estatísticas
            UpdateAccessStats(Position);
            CacheHits++;
            
            // Calcular tempo de operação
            const double EndTime = FPlatformTime::Seconds();
            TotalOperationTime += static_cast<float>(EndTime - StartTime);
            
            LogCacheEvent(EPlanetEventType::Info, FString::Printf(TEXT("Cache hit: %s"), *Position.ToString()));
            return true;
        }
        else
        {
            CacheMisses++;
            
            // Calcular tempo de operação
            const double EndTime = FPlatformTime::Seconds();
            TotalOperationTime += static_cast<float>(EndTime - StartTime);
            
            LogCacheEvent(EPlanetEventType::Warning, FString::Printf(TEXT("Cache miss: %s"), *Position.ToString()));
            return false;
        }
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro na busca de chunk: %s"), UTF8_TO_TCHAR(e.what()));
        LogCacheEvent(EPlanetEventType::Error, ErrorMsg);
        return false;
    }
}

void UPlanetChunkNetworkCache::RemoveChunk(const FVector& Position)
{
    try
    {
        if (!ValidatePosition(Position))
        {
            LogCacheEvent(EPlanetEventType::Error, TEXT("Posição inválida para remoção"));
            return;
        }
        
        // Remover chunk
        NetworkedChunks.Remove(Position);
        ChunkTimestamps.Remove(Position);
        ChunkAccessCounts.Remove(Position);
        AccessOrder.Remove(Position);
        
        ChunksRemoved++;
        
        LogCacheEvent(EPlanetEventType::Info, FString::Printf(TEXT("Chunk removido: %s"), *Position.ToString()));
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro na remoção de chunk: %s"), UTF8_TO_TCHAR(e.what()));
        LogCacheEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

void UPlanetChunkNetworkCache::CleanupOldChunks(float MaxAge)
{
    try
    {
        const double CurrentTime = FPlatformTime::Seconds();
        TArray<FVector> ChunksToRemove;
        
        // Identificar chunks antigos
        for (const auto& Pair : ChunkTimestamps)
        {
            const float Age = static_cast<float>(CurrentTime - Pair.Value);
            if (Age > MaxAge)
            {
                ChunksToRemove.Add(Pair.Key);
            }
        }
        
        // Remover chunks antigos
        for (const FVector& Position : ChunksToRemove)
        {
            RemoveChunk(Position);
        }
        
        if (ChunksToRemove.Num() > 0)
        {
            LogCacheEvent(EPlanetEventType::Info, FString::Printf(TEXT("Limpeza concluída: %d chunks removidos"), ChunksToRemove.Num()));
        }
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro na limpeza de chunks: %s"), UTF8_TO_TCHAR(e.what()));
        LogCacheEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

void UPlanetChunkNetworkCache::SetMaxCacheSize(int32 MaxSize)
{
    MaxCacheSize = FMath::Max(1, MaxSize);
    LogCacheEvent(EPlanetEventType::Info, FString::Printf(TEXT("Tamanho máximo do cache definido: %d"), MaxSize));
}

int32 UPlanetChunkNetworkCache::GetCacheSize() const
{
    return NetworkedChunks.Num();
}

bool UPlanetChunkNetworkCache::IsCacheFull() const
{
    return NetworkedChunks.Num() >= MaxCacheSize;
}

void UPlanetChunkNetworkCache::ClearCache()
{
    try
    {
        const int32 PreviousSize = NetworkedChunks.Num();
        
        NetworkedChunks.Empty();
        ChunkTimestamps.Empty();
        ChunkAccessCounts.Empty();
        AccessOrder.Empty();
        
        LogCacheEvent(EPlanetEventType::Info, FString::Printf(TEXT("Cache limpo: %d chunks removidos"), PreviousSize));
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro na limpeza do cache: %s"), UTF8_TO_TCHAR(e.what()));
        LogCacheEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

void UPlanetChunkNetworkCache::GetNetworkStats(FString& OutStats) const
{
    OutStats = FString::Printf(
        TEXT("=== Estatísticas de Cache de Rede ===\n")
        TEXT("Chunks Sincronizados: %d\n")
        TEXT("Chunks Removidos: %d\n")
        TEXT("Tamanho Atual: %d/%d\n")
        TEXT("Taxa de Hit: %.2f%%\n")
        TEXT("Tempo Total: %.3fms\n")
        TEXT("Política: %s\n"),
        ChunksSynchronized,
        ChunksRemoved,
        NetworkedChunks.Num(),
        MaxCacheSize,
        CalculateHitRate() * 100.0f,
        TotalOperationTime * 1000.0f,
        *GetCachePolicyString(ReplacementPolicy)
    );
}

void UPlanetChunkNetworkCache::GetCacheStats(FString& OutStats) const
{
    const float HitRate = CalculateHitRate();
    const float Efficiency = CalculateCacheEfficiency();
    
    OutStats = FString::Printf(
        TEXT("=== Estatísticas de Cache ===\n")
        TEXT("Hits: %d\n")
        TEXT("Misses: %d\n")
        TEXT("Taxa de Hit: %.2f%%\n")
        TEXT("Eficiência: %.2f%%\n")
        TEXT("Tamanho: %d/%d\n")
        TEXT("Política: %s\n")
        TEXT("Idade Máxima: %.1fs\n"),
        CacheHits,
        CacheMisses,
        HitRate * 100.0f,
        Efficiency * 100.0f,
        NetworkedChunks.Num(),
        MaxCacheSize,
        *GetCachePolicyString(ReplacementPolicy),
        MaxChunkAge
    );
}

void UPlanetChunkNetworkCache::ResetStats()
{
    CacheHits = 0;
    CacheMisses = 0;
    ChunksSynchronized = 0;
    ChunksRemoved = 0;
    TotalOperationTime = 0.0f;
    
    LogCacheEvent(EPlanetEventType::Info, TEXT("Estatísticas de cache resetadas"));
}

void UPlanetChunkNetworkCache::OptimizeCache()
{
    try
    {
        const int32 InitialSize = NetworkedChunks.Num();
        
        // Limpar chunks antigos
        CleanupOldChunks(MaxChunkAge);
        
        // Otimizar baseado na política
        switch (ReplacementPolicy)
        {
            case EPlanetCachePolicy::LRU:
                // Manter apenas os chunks mais recentemente usados
                while (NetworkedChunks.Num() > MaxCacheSize * 0.8f)
                {
                    RemoveLeastRecentlyUsed();
                }
                break;
                
            case EPlanetCachePolicy::LFU:
                // Manter apenas os chunks mais frequentemente usados
                while (NetworkedChunks.Num() > MaxCacheSize * 0.8f)
                {
                    RemoveLeastFrequentlyUsed();
                }
                break;
                
            default:
                break;
        }
        
        const int32 FinalSize = NetworkedChunks.Num();
        LogCacheEvent(EPlanetEventType::Info, FString::Printf(TEXT("Cache otimizado: %d -> %d chunks"), InitialSize, FinalSize));
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro na otimização do cache: %s"), UTF8_TO_TCHAR(e.what()));
        LogCacheEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

void UPlanetChunkNetworkCache::SetReplacementPolicy(EPlanetCachePolicy Policy)
{
    ReplacementPolicy = Policy;
    LogCacheEvent(EPlanetEventType::Info, FString::Printf(TEXT("Política de cache alterada: %s"), *GetCachePolicyString(Policy)));
}

EPlanetCachePolicy UPlanetChunkNetworkCache::GetReplacementPolicy() const
{
    return ReplacementPolicy;
}

void UPlanetChunkNetworkCache::UpdateAccessStats(const FVector& Position)
{
    // Atualizar contador de acesso
    int32* AccessCount = ChunkAccessCounts.Find(Position);
    if (AccessCount)
    {
        (*AccessCount)++;
    }
    
    // Atualizar ordem de acesso (LRU)
    AccessOrder.Remove(Position);
    AccessOrder.Add(Position);
}

void UPlanetChunkNetworkCache::RemoveLeastRecentlyUsed()
{
    if (AccessOrder.Num() > 0)
    {
        const FVector PositionToRemove = AccessOrder[0];
        RemoveChunk(PositionToRemove);
    }
}

void UPlanetChunkNetworkCache::RemoveLeastFrequentlyUsed()
{
    if (ChunkAccessCounts.Num() == 0)
    {
        return;
    }
    
    // Encontrar chunk com menor contador de acesso
    FVector PositionToRemove = FVector::ZeroVector;
    int32 MinAccessCount = MAX_int32;
    
    for (const auto& Pair : ChunkAccessCounts)
    {
        if (Pair.Value < MinAccessCount)
        {
            MinAccessCount = Pair.Value;
            PositionToRemove = Pair.Key;
        }
    }
    
    if (MinAccessCount != MAX_int32)
    {
        RemoveChunk(PositionToRemove);
    }
}

void UPlanetChunkNetworkCache::RemoveRandomChunk()
{
    if (NetworkedChunks.Num() > 0)
    {
        // Escolher posição aleatória
        const int32 RandomIndex = FMath::RandRange(0, NetworkedChunks.Num() - 1);
        int32 CurrentIndex = 0;
        
        for (const auto& Pair : NetworkedChunks)
        {
            if (CurrentIndex == RandomIndex)
            {
                RemoveChunk(Pair.Key);
                break;
            }
            CurrentIndex++;
        }
    }
}

FString UPlanetChunkNetworkCache::CalculatePositionHash(const FVector& Position) const
{
    return FString::Printf(TEXT("%.2f_%.2f_%.2f"), Position.X, Position.Y, Position.Z);
}

bool UPlanetChunkNetworkCache::ValidatePosition(const FVector& Position) const
{
    if (!FMath::IsFinite(Position.X) || !FMath::IsFinite(Position.Y) || !FMath::IsFinite(Position.Z))
    {
        return false;
    }
    
    return true;
}

void UPlanetChunkNetworkCache::LogCacheEvent(EPlanetEventType EventType, const FString& Details)
{
    if (Logger)
    {
        Logger->LogInfo(TEXT("PlanetChunkNetworkCache"), Details);
    }
    
    if (EventBus)
    {
        FPlanetSystemEvent Event;
        Event.EventType = EventType;
        Event.Source = TEXT("PlanetChunkNetworkCache");
        Event.Details = Details;
        Event.Timestamp = FDateTime::Now();
        
        EventBus->BroadcastEvent(Event);
    }
}

float UPlanetChunkNetworkCache::CalculateHitRate() const
{
    const int32 TotalRequests = CacheHits + CacheMisses;
    if (TotalRequests == 0)
    {
        return 0.0f;
    }
    
    return static_cast<float>(CacheHits) / static_cast<float>(TotalRequests);
}

float UPlanetChunkNetworkCache::CalculateCacheEfficiency() const
{
    if (MaxCacheSize == 0)
    {
        return 0.0f;
    }
    
    const float HitRate = CalculateHitRate();
    const float Utilization = static_cast<float>(NetworkedChunks.Num()) / static_cast<float>(MaxCacheSize);
    
    // Eficiência = Taxa de Hit * Utilização
    return HitRate * Utilization;
}

FString UPlanetChunkNetworkCache::GetCachePolicyString(EPlanetCachePolicy Policy) const
{
    switch (Policy)
    {
        case EPlanetCachePolicy::LRU:
            return TEXT("LRU (Least Recently Used)");
        case EPlanetCachePolicy::LFU:
            return TEXT("LFU (Least Frequently Used)");
        case EPlanetCachePolicy::Random:
            return TEXT("Random");
        default:
            return TEXT("Unknown");
    }
} 