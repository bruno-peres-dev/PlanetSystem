#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Rendering/Chunks/ChunkCache.h"
#include "PlanetChunkCache.generated.h"

// Forward declarations
class UObject;
struct FPlanetChunk;

/**
 * Sistema de cache avançado para chunks do PlanetSystem
 * Gerenciamento inteligente de memória e performance
 * Segue o padrão AAA data-driven inspirado na Source2
 */
UCLASS(BlueprintType, Blueprintable)
class PLANETSYSTEM_API UPlanetChunkCache : public UObject
{
    GENERATED_BODY()
    
public:
    UPlanetChunkCache();
    
    /**
     * Obtém a instância singleton do ChunkCache
     * @return Instância do ChunkCache
     */
    UFUNCTION(BlueprintCallable, Category="PlanetChunkCache", CallInEditor=true)
    static UPlanetChunkCache* GetInstance();
    
    /**
     * Adiciona um chunk ao cache
     * @param Chunk Chunk a ser adicionado
     * @param Priority Prioridade do chunk
     * @return true se adicionou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetChunkCache")
    bool AddChunk(const FPlanetChunk& Chunk, float Priority = 1.0f);
    
    /**
     * Obtém um chunk do cache
     * @param ChunkKey Chave do chunk
     * @param OutChunk Chunk encontrado
     * @return true se encontrou
     */
    UFUNCTION(BlueprintCallable, Category="PlanetChunkCache")
    bool GetChunk(const FString& ChunkKey, FPlanetChunk& OutChunk);
    
    /**
     * Remove um chunk do cache
     * @param ChunkKey Chave do chunk
     * @return true se removeu com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetChunkCache")
    bool RemoveChunk(const FString& ChunkKey);
    
    /**
     * Verifica se um chunk está no cache
     * @param ChunkKey Chave do chunk
     * @return true se está no cache
     */
    UFUNCTION(BlueprintCallable, Category="PlanetChunkCache")
    bool HasChunk(const FString& ChunkKey) const;
    
    /**
     * Obtém estatísticas do cache
     * @param OutStats String com estatísticas
     */
    UFUNCTION(BlueprintCallable, Category="PlanetChunkCache")
    void GetCacheStats(FString& OutStats) const;
    
    /**
     * Limpa o cache
     * @param bForce true para forçar limpeza
     */
    UFUNCTION(BlueprintCallable, Category="PlanetChunkCache")
    void ClearCache(bool bForce = false);
    
    /**
     * Otimiza o cache (remove chunks de baixa prioridade)
     */
    UFUNCTION(BlueprintCallable, Category="PlanetChunkCache")
    void OptimizeCache();
    
    /**
     * Define o tamanho máximo do cache
     * @param MaxSize Tamanho máximo em MB
     */
    UFUNCTION(BlueprintCallable, Category="PlanetChunkCache")
    void SetMaxCacheSize(int32 MaxSize);
    
    /**
     * Obtém o tamanho atual do cache
     * @return Tamanho em MB
     */
    UFUNCTION(BlueprintCallable, Category="PlanetChunkCache")
    int32 GetCurrentCacheSize() const;
    
    /**
     * Habilita/desabilita o cache
     * @param bEnable true para habilitar
     */
    UFUNCTION(BlueprintCallable, Category="PlanetChunkCache")
    void SetCacheEnabled(bool bEnable);
    
    /**
     * Verifica se o cache está habilitado
     * @return true se habilitado
     */
    UFUNCTION(BlueprintCallable, Category="PlanetChunkCache")
    bool IsCacheEnabled() const;
    
private:
    // Instância singleton
    static UPlanetChunkCache* Instance;
    
    // Estrutura para entrada do cache
    struct FCacheEntry
    {
        FPlanetChunk Chunk;
        float Priority;
        float LastAccessTime;
        int32 AccessCount;
        int32 SizeInBytes;
        
        FCacheEntry(const FPlanetChunk& InChunk, float InPriority)
            : Chunk(InChunk), Priority(InPriority), LastAccessTime(FPlatformTime::Seconds())
            , AccessCount(1), SizeInBytes(0)
        {}
    };
    
    // Mapa do cache
    UPROPERTY()
    TMap<FString, FCacheEntry> CacheEntries;
    
    // Configurações
    UPROPERTY()
    int32 MaxCacheSizeMB = 1024;
    
    UPROPERTY()
    bool bCacheEnabled = true;
    
    UPROPERTY()
    int32 CurrentCacheSizeBytes = 0;
    
    UPROPERTY()
    int32 TotalHits = 0;
    
    UPROPERTY()
    int32 TotalMisses = 0;
    
    UPROPERTY()
    float LastOptimizationTime = 0.0f;
}; 