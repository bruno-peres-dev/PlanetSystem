#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/Events/PlanetSystemEvents.h"
#include "PlanetNetworkEventBus.generated.h"

// Forward declarations
class UPlanetSystemLogger;
class UPlanetEventBus;

/**
 * Sistema de eventos de rede para PlanetSystem
 * Implementa broadcast de eventos em rede com filtros e prioridades
 */
UCLASS(Blueprintable, BlueprintType)
class PLANETSYSTEM_API UPlanetNetworkEventBus : public UObject
{
    GENERATED_BODY()

public:
    UPlanetNetworkEventBus();

    // === BROADCAST DE EVENTOS ===
    
    /**
     * Transmite evento em rede
     * @param Event - Evento a ser transmitido
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    void BroadcastNetworkEvent(const FPlanetSystemEvent& Event);
    
    /**
     * Transmite evento com parâmetros em rede
     * @param EventType - Tipo do evento
     * @param Source - Origem do evento
     * @param Details - Detalhes do evento
     * @param Priority - Prioridade do evento
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    void BroadcastNetworkEventWithParams(EPlanetEventType EventType, const FString& Source, const FString& Details, int32 Priority = 0);
    
    /**
     * Recebe evento de rede
     * @param Event - Evento recebido
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    void OnNetworkEventReceived(const FPlanetSystemEvent& Event);

    // === FILTROS DE REDE ===
    
    /**
     * Verifica se evento deve ser transmitido
     * @param Event - Evento a ser verificado
     * @return True se deve ser transmitido
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    bool ShouldBroadcastEvent(const FPlanetSystemEvent& Event);
    
    /**
     * Define filtro de rede
     * @param AllowedEvents - Tipos de eventos permitidos
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    void SetNetworkFilter(const TArray<EPlanetEventType>& AllowedEvents);
    
    /**
     * Adiciona tipo de evento ao filtro
     * @param EventType - Tipo de evento a ser adicionado
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    void AddEventTypeToFilter(EPlanetEventType EventType);
    
    /**
     * Remove tipo de evento do filtro
     * @param EventType - Tipo de evento a ser removido
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    void RemoveEventTypeFromFilter(EPlanetEventType EventType);
    
    /**
     * Limpa filtro de rede
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    void ClearNetworkFilter();

    // === GERENCIAMENTO DE HISTÓRICO ===
    
    /**
     * Obtém histórico de eventos
     * @param OutHistory - Histórico de eventos
     * @param MaxEvents - Número máximo de eventos
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    void GetEventHistory(TArray<FPlanetSystemEvent>& OutHistory, int32 MaxEvents = 100);
    
    /**
     * Limpa histórico de eventos
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    void ClearEventHistory();
    
    /**
     * Define tamanho máximo do histórico
     * @param MaxSize - Tamanho máximo
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    void SetMaxHistorySize(int32 MaxSize);

    // === CONFIGURAÇÃO ===
    
    /**
     * Define se está habilitado
     * @param bEnabled - Se habilitado
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    void SetEnabled(bool bEnabled);
    
    /**
     * Verifica se está habilitado
     * @return True se habilitado
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    bool IsEnabled() const;
    
    /**
     * Define modo de rede
     * @param bIsServer - Se é servidor
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    void SetNetworkMode(bool bIsServer);

    // === ESTATÍSTICAS ===
    
    /**
     * Obtém estatísticas de eventos de rede
     * @param OutStats - Estatísticas formatadas
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    void GetNetworkEventStats(FString& OutStats) const;
    
    /**
     * Reseta estatísticas
     */
    UFUNCTION(BlueprintCallable, Category="Network Events")
    void ResetStats();

protected:
    // === ESTRUTURAS DE DADOS ===
    
    /** Histórico de eventos */
    UPROPERTY()
    TArray<FPlanetSystemEvent> EventHistory;
    
    /** Filtro de tipos de eventos */
    UPROPERTY()
    TArray<EPlanetEventType> NetworkFilter;
    
    /** Eventos bloqueados */
    UPROPERTY()
    TArray<EPlanetEventType> BlockedEvents;

    // === CONFIGURAÇÃO ===
    
    /** Se está habilitado */
    UPROPERTY()
    bool bEnabled;
    
    /** Se é servidor */
    UPROPERTY()
    bool bIsServer;
    
    /** Tamanho máximo do histórico */
    UPROPERTY()
    int32 MaxHistorySize;
    
    /** Se deve logar eventos */
    UPROPERTY()
    bool bLogEvents;

    // === ESTATÍSTICAS ===
    
    /** Eventos transmitidos */
    UPROPERTY()
    int32 EventsBroadcasted;
    
    /** Eventos recebidos */
    UPROPERTY()
    int32 EventsReceived;
    
    /** Eventos bloqueados */
    UPROPERTY()
    int32 EventsBlocked;
    
    /** Eventos filtrados */
    UPROPERTY()
    int32 EventsFiltered;
    
    /** Tempo total de processamento */
    UPROPERTY()
    float TotalProcessingTime;

    // === COMPONENTES ===
    
    /** Logger do sistema */
    UPROPERTY()
    UPlanetSystemLogger* Logger;
    
    /** Event bus principal */
    UPROPERTY()
    UPlanetEventBus* EventBus;

private:
    // === UTILITÁRIOS ===
    
    /**
     * Adiciona evento ao histórico
     * @param Event - Evento a ser adicionado
     */
    void AddToHistory(const FPlanetSystemEvent& Event);
    
    /**
     * Limpa histórico antigo
     */
    void CleanupOldHistory();
    
    /**
     * Verifica se evento está no filtro
     * @param EventType - Tipo do evento
     * @return True se está no filtro
     */
    bool IsEventTypeInFilter(EPlanetEventType EventType) const;
    
    /**
     * Verifica se evento está bloqueado
     * @param EventType - Tipo do evento
     * @return True se está bloqueado
     */
    bool IsEventTypeBlocked(EPlanetEventType EventType) const;
    
    /**
     * Valida evento
     * @param Event - Evento a ser validado
     * @return True se válido
     */
    bool ValidateEvent(const FPlanetSystemEvent& Event) const;
    
    /**
     * Loga evento de rede
     * @param EventType - Tipo do evento
     * @param Details - Detalhes do evento
     */
    void LogNetworkEvent(EPlanetEventType EventType, const FString& Details);
    
    /**
     * Calcula prioridade do evento
     * @param Event - Evento
     * @return Prioridade calculada
     */
    int32 CalculateEventPriority(const FPlanetSystemEvent& Event) const;
    
    /**
     * Processa evento recebido
     * @param Event - Evento recebido
     */
    void ProcessReceivedEvent(const FPlanetSystemEvent& Event);
}; 