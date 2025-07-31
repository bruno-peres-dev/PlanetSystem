#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Configuration/DataAssets/CoreConfig.h"
#include "Core/Cache/PlanetCachePolicy.h"
#include "Common/PlanetTypes.h"
#include "PlanetChunkNetworkCache.generated.h"

// Forward declarations
class UPlanetSystemLogger;
class UPlanetEventBus;

/**
 * Sistema de cache distribuído para chunks de planeta
 * Implementa cache inteligente com sincronização de rede
 */
UCLASS(Blueprintable, BlueprintType)
class PLANETSYSTEM_API UPlanetChunkNetworkCache : public UObject
{
    GENERATED_BODY()

public:
    UPlanetChunkNetworkCache();

    // === SINCRONIZAÇÃO DE CHUNKS ===
    
    /**
     * Sincroniza chunk no cache
     * @param Position - Posição do chunk
     * @param Chunk - Chunk a ser sincronizado
     */
    UFUNCTION(BlueprintCallable, Category="Network Cache")
    void SynchronizeChunk(const FVector& Position, const FPlanetChunk& Chunk);
    
    /**
     * Obtém chunk do cache
     * @param Position - Posição do chunk
     * @param OutChunk - Chunk obtido
     * @return True se encontrado
     */
    UFUNCTION(BlueprintCallable, Category="Network Cache")
    bool GetChunk(const FVector& Position, FPlanetChunk& OutChunk);
    
    /**
     * Remove chunk do cache
     * @param Position - Posição do chunk
     */
    UFUNCTION(BlueprintCallable, Category="Network Cache")
    void RemoveChunk(const FVector& Position);
    
    /**
     * Limpa chunks antigos
     * @param MaxAge - Idade máxima em segundos
     */
    UFUNCTION(BlueprintCallable, Category="Network Cache")
    void CleanupOldChunks(float MaxAge);

    // === GERENCIAMENTO DE CACHE ===
    
    /**
     * Define tamanho máximo do cache
     * @param MaxSize - Tamanho máximo
     */
    UFUNCTION(BlueprintCallable, Category="Network Cache")
    void SetMaxCacheSize(int32 MaxSize);
    
    /**
     * Obtém tamanho atual do cache
     * @return Tamanho atual
     */
    UFUNCTION(BlueprintCallable, Category="Network Cache")
    int32 GetCacheSize() const;
    
    /**
     * Verifica se cache está cheio
     * @return True se cheio
     */
    UFUNCTION(BlueprintCallable, Category="Network Cache")
    bool IsCacheFull() const;
    
    /**
     * Limpa todo o cache
     */
    UFUNCTION(BlueprintCallable, Category="Network Cache")
    void ClearCache();

    // === ESTATÍSTICAS ===
    
    /**
     * Obtém estatísticas de rede
     * @param OutStats - Estatísticas formatadas
     */
    UFUNCTION(BlueprintCallable, Category="Network Cache")
    void GetNetworkStats(FString& OutStats) const;
    
    /**
     * Obtém estatísticas de cache
     * @param OutStats - Estatísticas formatadas
     */
    UFUNCTION(BlueprintCallable, Category="Network Cache")
    void GetCacheStats(FString& OutStats) const;
    
    /**
     * Reseta estatísticas
     */
    UFUNCTION(BlueprintCallable, Category="Network Cache")
    void ResetStats();

    // === OTIMIZAÇÃO ===
    
    /**
     * Otimiza cache baseado em uso
     */
    UFUNCTION(BlueprintCallable, Category="Network Cache")
    void OptimizeCache();
    
    /**
     * Define política de substituição
     * @param Policy - Política (LRU, LFU, etc.)
     */
    UFUNCTION(BlueprintCallable, Category="Network Cache")
    void SetReplacementPolicy(EPlanetCachePolicy Policy);
    
    /**
     * Obtém política atual
     * @return Política atual
     */
    UFUNCTION(BlueprintCallable, Category="Network Cache")
    EPlanetCachePolicy GetReplacementPolicy() const;

protected:
    // === ESTRUTURAS DE DADOS ===
    
    /** Cache de chunks em rede */
    UPROPERTY()
    TMap<FVector, FPlanetChunk> NetworkedChunks;
    
    /** Timestamps de chunks */
    UPROPERTY()
    TMap<FVector, float> ChunkTimestamps;
    
    /** Contadores de acesso */
    UPROPERTY()
    TMap<FVector, int32> ChunkAccessCounts;
    
    /** Ordem de acesso (LRU) */
    UPROPERTY()
    TArray<FVector> AccessOrder;

    // === CONFIGURAÇÃO ===
    
    /** Tamanho máximo do cache */
    UPROPERTY()
    int32 MaxCacheSize;
    
    /** Política de substituição */
    UPROPERTY()
    EPlanetCachePolicy ReplacementPolicy;
    
    /** Tempo máximo de vida dos chunks */
    UPROPERTY()
    float MaxChunkAge;

    // === ESTATÍSTICAS ===
    
    /** Hits no cache */
    UPROPERTY()
    int32 CacheHits;
    
    /** Misses no cache */
    UPROPERTY()
    int32 CacheMisses;
    
    /** Chunks sincronizados */
    UPROPERTY()
    int32 ChunksSynchronized;
    
    /** Chunks removidos */
    UPROPERTY()
    int32 ChunksRemoved;
    
    /** Tempo total de operações */
    UPROPERTY()
    float TotalOperationTime;

    // === COMPONENTES ===
    
    /** Logger do sistema */
    UPROPERTY()
    UPlanetSystemLogger* Logger;
    
    /** Event bus */
    UPROPERTY()
    UPlanetEventBus* EventBus;

private:
    // === UTILITÁRIOS ===
    
    /**
     * Atualiza estatísticas de acesso
     * @param Position - Posição do chunk
     */
    void UpdateAccessStats(const FVector& Position);
    
    /**
     * Remove chunk menos usado (LRU)
     */
    void RemoveLeastRecentlyUsed();
    
    /**
     * Remove chunk menos frequentemente usado (LFU)
     */
    void RemoveLeastFrequentlyUsed();
    
    /**
     * Remove chunk aleatório
     */
    void RemoveRandomChunk();
    
    /**
     * Calcula hash da posição
     * @param Position - Posição
     * @return Hash da posição
     */
    FString CalculatePositionHash(const FVector& Position) const;
    
    /**
     * Valida posição
     * @param Position - Posição a ser validada
     * @return True se válida
     */
    bool ValidatePosition(const FVector& Position) const;
    
    /**
     * Loga evento de cache
     * @param EventType - Tipo do evento
     * @param Details - Detalhes do evento
     */
    void LogCacheEvent(EPlanetEventType EventType, const FString& Details);
    
    /**
     * Calcula taxa de hit
     * @return Taxa de hit (0-1)
     */
    float CalculateHitRate() const;
    
    /**
     * Calcula eficiência do cache
     * @return Eficiência (0-1)
     */
    float CalculateCacheEfficiency() const;
}; 