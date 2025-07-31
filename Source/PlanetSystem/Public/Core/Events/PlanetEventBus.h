#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/Events/PlanetSystemEvents.h"
#include "PlanetEventBus.generated.h"

// Forward declarations
class UObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlanetEventReceived, const FPlanetSystemEvent&, Event);

/**
 * Sistema de eventos centralizado para o PlanetSystem
 * Permite comunicação desacoplada entre componentes
 * Segue o padrão AAA data-driven inspirado na Source2
 */
UCLASS(BlueprintType, Blueprintable)
class PLANETSYSTEM_API UPlanetEventBus : public UObject
{
    GENERATED_BODY()
    
public:
    UPlanetEventBus();
    
    /**
     * Obtém a instância singleton do EventBus
     * @return Instância do EventBus
     */
    UFUNCTION(BlueprintCallable, Category="PlanetEventBus", CallInEditor=true)
    static UPlanetEventBus* GetInstance();
    
    /**
     * Registra um listener para um tipo específico de evento
     * @param EventType Tipo de evento para escutar
     * @param Listener Objeto que irá receber os eventos
     * @param FunctionName Nome da função a ser chamada
     */
    UFUNCTION(BlueprintCallable, Category="PlanetEventBus")
    void RegisterListener(EPlanetEventType EventType, UObject* Listener, const FString& FunctionName);
    
    /**
     * Remove um listener de um tipo específico de evento
     * @param EventType Tipo de evento
     * @param Listener Objeto a ser removido
     */
    UFUNCTION(BlueprintCallable, Category="PlanetEventBus")
    void UnregisterListener(EPlanetEventType EventType, UObject* Listener);
    
    /**
     * Remove um listener de todos os tipos de eventos
     * @param Listener Objeto a ser removido
     */
    UFUNCTION(BlueprintCallable, Category="PlanetEventBus")
    void UnregisterListenerFromAll(UObject* Listener);
    
    /**
     * Envia um evento para todos os listeners registrados
     * @param Event Evento a ser enviado
     */
    UFUNCTION(BlueprintCallable, Category="PlanetEventBus")
    void BroadcastEvent(const FPlanetSystemEvent& Event);
    
    /**
     * Envia um evento com parâmetros básicos
     * @param EventType Tipo do evento
     * @param CustomName Nome customizado do evento
     * @param StringParam Parâmetro string
     * @param FloatParam Parâmetro float
     * @param IntParam Parâmetro inteiro
     */
    UFUNCTION(BlueprintCallable, Category="PlanetEventBus")
    void BroadcastEventWithParams(EPlanetEventType EventType, const FString& CustomName = TEXT(""), 
                                 const FString& StringParam = TEXT(""), float FloatParam = 0.0f, 
                                 int32 IntParam = 0);
    
    /**
     * Obtém estatísticas do EventBus
     * @param OutStats String com estatísticas
     */
    UFUNCTION(BlueprintCallable, Category="PlanetEventBus")
    void GetEventBusStats(FString& OutStats) const;
    
    /**
     * Limpa todos os listeners registrados
     */
    UFUNCTION(BlueprintCallable, Category="PlanetEventBus")
    void ClearAllListeners();
    
    /**
     * Obtém o número de listeners para um tipo específico de evento
     * @param EventType Tipo do evento
     * @return Número de listeners
     */
    UFUNCTION(BlueprintCallable, Category="PlanetEventBus")
    int32 GetListenerCount(EPlanetEventType EventType) const;
    
    /**
     * Verifica se há listeners para um tipo específico de evento
     * @param EventType Tipo do evento
     * @return true se há listeners
     */
    UFUNCTION(BlueprintCallable, Category="PlanetEventBus")
    bool HasListeners(EPlanetEventType EventType) const;
    
    /**
     * Obtém o histórico de eventos (últimos N eventos)
     * @param MaxEvents Número máximo de eventos a retornar
     * @return Array com eventos
     */
    UFUNCTION(BlueprintCallable, Category="PlanetEventBus")
    TArray<FPlanetSystemEvent> GetEventHistory(int32 MaxEvents = 100) const;
    
    /**
     * Limpa o histórico de eventos
     */
    UFUNCTION(BlueprintCallable, Category="PlanetEventBus")
    void ClearEventHistory();
    
    /**
     * Habilita/desabilita o logging de eventos
     * @param bEnable true para habilitar, false para desabilitar
     */
    UFUNCTION(BlueprintCallable, Category="PlanetEventBus")
    void SetEventLoggingEnabled(bool bEnable);
    
    /**
     * Verifica se o logging de eventos está habilitado
     * @return true se habilitado
     */
    UFUNCTION(BlueprintCallable, Category="PlanetEventBus")
    bool IsEventLoggingEnabled() const;
    
protected:
    // Delegate para eventos recebidos
    UPROPERTY(BlueprintAssignable, Category="PlanetEventBus")
    FOnPlanetEventReceived OnEventReceived;
    
private:
    // Estrutura para armazenar listeners
    struct FEventListener
    {
        UObject* Listener;
        FString FunctionName;
        float RegistrationTime;
        
        FEventListener(UObject* InListener, const FString& InFunctionName)
            : Listener(InListener), FunctionName(InFunctionName), RegistrationTime(FPlatformTime::Seconds())
        {}
    };
    
    // Mapa de listeners por tipo de evento
    UPROPERTY()
    TMap<EPlanetEventType, TArray<FEventListener>> EventListeners;
    
    // Histórico de eventos
    UPROPERTY()
    TArray<FPlanetSystemEvent> EventHistory;
    
    // Configurações
    UPROPERTY()
    bool bEventLoggingEnabled = true;
    
    UPROPERTY()
    int32 MaxEventHistorySize = 1000;
    
    UPROPERTY()
    int32 TotalEventsBroadcasted = 0;
    
    UPROPERTY()
    float LastEventTime = 0.0f;
    
    // Instância singleton
    static UPlanetEventBus* Instance;
    
    /**
     * Processa um evento e notifica os listeners
     * @param Event Evento a ser processado
     */
    void ProcessEvent(const FPlanetSystemEvent& Event);
    
    /**
     * Adiciona evento ao histórico
     * @param Event Evento a ser adicionado
     */
    void AddToHistory(const FPlanetSystemEvent& Event);
    
    /**
     * Limpa listeners inválidos
     */
    void CleanupInvalidListeners();
}; 