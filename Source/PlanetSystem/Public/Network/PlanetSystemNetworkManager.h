#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Net/UnrealNetwork.h"
#include "Configuration/DataAssets/CoreConfig.h"
#include "Common/PlanetTypes.h"
#include "PlanetSystemNetworkManager.generated.h"

// Forward declarations
class UPlanetSystemLogger;
class UPlanetEventBus;
class UPlanetChunkNetworkCache;
class UPlanetNetworkEventBus;

/**
 * Sistema de gerenciamento de rede para PlanetSystem
 * Implementa sincronização de seed, chunks e configurações em multiplayer
 */
UCLASS(Blueprintable, BlueprintType)
class PLANETSYSTEM_API UPlanetSystemNetworkManager : public UObject
{
    GENERATED_BODY()

public:
    UPlanetSystemNetworkManager();

    // === SINCRONIZAÇÃO DE CONFIGURAÇÃO ===
    
    /**
     * Define seed global (Server)
     * @param NewSeed - Nova seed global
     */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category="Network")
    void Server_SetGlobalSeed(int32 NewSeed);
    
    /**
     * Recebe seed global (Client)
     * @param NewSeed - Seed global recebida
     */
    UFUNCTION(Client, Reliable, BlueprintCallable, Category="Network")
    void Client_ReceiveGlobalSeed(int32 NewSeed);
    
    /**
     * Sincroniza configuração global (Server)
     * @param Config - Configuração a ser sincronizada
     */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category="Network")
    void Server_SynchronizeConfig(const UPlanetCoreConfig* Config);
    
    /**
     * Recebe configuração sincronizada (Client)
     * @param ConfigData - Dados da configuração
     */
    UFUNCTION(Client, Reliable, BlueprintCallable, Category="Network")
    void Client_ReceiveSynchronizedConfig(const FString& ConfigData);

    // === SINCRONIZAÇÃO DE CHUNKS ===
    
    /**
     * Solicita chunk (Client)
     * @param Position - Posição do chunk
     * @param LODLevel - Nível de detalhe
     */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category="Network")
    void Server_RequestChunk(const FVector& Position, int32 LODLevel);
    
    /**
     * Recebe chunk (Client)
     * @param Position - Posição do chunk
     * @param ChunkData - Dados do chunk
     */
    UFUNCTION(Client, Reliable, BlueprintCallable, Category="Network")
    void Client_ReceiveChunk(const FVector& Position, const FString& ChunkData);
    
    /**
     * Sincroniza chunk (Server)
     * @param Position - Posição do chunk
     * @param Chunk - Chunk a ser sincronizado
     */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category="Network")
    void Server_SynchronizeChunk(const FVector& Position, const FPlanetChunk& Chunk);

    // === VALIDAÇÃO DE CONFIGURAÇÕES ===
    
    /**
     * Valida configuração em rede (Server)
     * @param Config - Configuração a ser validada
     */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category="Network")
    void Server_ValidateConfiguration(const UPlanetCoreConfig* Config);
    
    /**
     * Recebe resultado da validação (Client)
     * @param bIsValid - Se a configuração é válida
     * @param Errors - Lista de erros
     */
    UFUNCTION(Client, Reliable, BlueprintCallable, Category="Network")
    void Client_ConfigurationValidated(bool bIsValid, const TArray<FString>& Errors);

    // === GERENCIAMENTO DE CONEXÃO ===
    
    /**
     * Inicializa sistema de rede
     * @param bIsServer - Se é servidor
     */
    UFUNCTION(BlueprintCallable, Category="Network")
    void InitializeNetwork(bool bIsServer);
    
    /**
     * Finaliza sistema de rede
     */
    UFUNCTION(BlueprintCallable, Category="Network")
    void ShutdownNetwork();
    
    /**
     * Verifica se está conectado
     * @return True se conectado
     */
    UFUNCTION(BlueprintCallable, Category="Network")
    bool IsNetworkConnected() const;

    // === ESTATÍSTICAS DE REDE ===
    
    /**
     * Obtém estatísticas de rede
     * @param OutStats - Estatísticas formatadas
     */
    UFUNCTION(BlueprintCallable, Category="Network")
    void GetNetworkStats(FString& OutStats) const;
    
    /**
     * Reseta estatísticas
     */
    UFUNCTION(BlueprintCallable, Category="Network")
    void ResetNetworkStats();

protected:
    // === PROPRIEDADES REPLICADAS ===
    
    /** Seed global sincronizada */
    UPROPERTY(ReplicatedUsing=OnRep_GlobalSeed)
    int32 GlobalSeed;
    
    /** Configuração sincronizada */
    UPROPERTY(ReplicatedUsing=OnRep_SynchronizedConfig)
    FString SynchronizedConfigData;
    
    /** Timestamp da última sincronização */
    UPROPERTY(Replicated)
    FDateTime LastSyncTime;

    // === CALLBACKS DE REPLICAÇÃO ===
    
    /** Callback quando seed global muda */
    UFUNCTION()
    void OnRep_GlobalSeed();
    
    /** Callback quando configuração sincronizada muda */
    UFUNCTION()
    void OnRep_SynchronizedConfig();

private:
    // === COMPONENTES ===
    
    /** Cache de chunks em rede */
    UPROPERTY()
    UPlanetChunkNetworkCache* ChunkCache;
    
    /** Event bus de rede */
    UPROPERTY()
    UPlanetNetworkEventBus* NetworkEventBus;
    
    /** Logger do sistema */
    UPROPERTY()
    UPlanetSystemLogger* Logger;
    
    /** Event bus principal */
    UPROPERTY()
    UPlanetEventBus* EventBus;

    // === ESTADO DE REDE ===
    
    /** Se é servidor */
    bool bIsServer;
    
    /** Se está conectado */
    bool bIsConnected;
    
    /** Se está inicializado */
    bool bIsInitialized;

    // === ESTATÍSTICAS ===
    
    /** Chunks enviados */
    int32 ChunksSent;
    
    /** Chunks recebidos */
    int32 ChunksReceived;
    
    /** Bytes enviados */
    int64 BytesSent;
    
    /** Bytes recebidos */
    int64 BytesReceived;
    
    /** Tempo total de rede */
    float TotalNetworkTime;
    
    /** Latência média */
    float AverageLatency;

    // === CACHE ===
    
    /** Cache de configurações */
    TMap<FString, UPlanetCoreConfig*> ConfigCache;
    
    /** Cache de chunks pendentes */
    TMap<FVector, FPlanetChunk> PendingChunks;
    
    /** Timestamps de chunks */
    TMap<FVector, FDateTime> ChunkTimestamps;

    // === UTILITÁRIOS ===
    
    /**
     * Serializa chunk para string
     * @param Chunk - Chunk a ser serializado
     * @return String serializada
     */
    FString SerializeChunk(const FPlanetChunk& Chunk) const;
    
    /**
     * Deserializa chunk de string
     * @param ChunkData - Dados serializados
     * @param OutChunk - Chunk deserializado
     * @return True se bem-sucedido
     */
    bool DeserializeChunk(const FString& ChunkData, FPlanetChunk& OutChunk) const;
    
    /**
     * Serializa configuração para string
     * @param Config - Configuração a ser serializada
     * @return String serializada
     */
    FString SerializeConfig(const UPlanetCoreConfig* Config) const;
    
    /**
     * Deserializa configuração de string
     * @param ConfigData - Dados serializados
     * @return Configuração deserializada
     */
    UPlanetCoreConfig* DeserializeConfig(const FString& ConfigData) const;
    
    /**
     * Valida dados de entrada
     * @param Position - Posição a ser validada
     * @param LODLevel - Nível de detalhe a ser validado
     * @return True se válido
     */
    bool ValidateInput(const FVector& Position, int32 LODLevel) const;
    
    /**
     * Loga evento de rede
     * @param EventType - Tipo do evento
     * @param Details - Detalhes do evento
     */
    void LogNetworkEvent(EPlanetEventType EventType, const FString& Details);
    
    /**
     * Limpa cache antigo
     */
    void CleanupOldCache();
    
    /**
     * Calcula latência
     * @param StartTime - Tempo de início
     * @return Latência em milissegundos
     */
    float CalculateLatency(const FDateTime& StartTime) const;
}; 