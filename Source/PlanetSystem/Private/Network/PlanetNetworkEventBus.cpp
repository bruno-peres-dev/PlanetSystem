#include "Network/PlanetNetworkEventBus.h"
#include "Debug/Logging/PlanetSystemLogger.h"
#include "Core/Events/PlanetEventBus.h"
#include "Core/Events/PlanetSystemEvents.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"

UPlanetNetworkEventBus::UPlanetNetworkEventBus()
{
    // Obter serviços do ServiceLocator
    Logger = UPlanetSystemLogger::GetInstance();
    EventBus = UPlanetEventBus::GetInstance();
    
    // Inicializar configuração
    bEnabled = true;
    bIsServer = false;
    MaxHistorySize = 1000;
    bLogEvents = true;
    
    // Inicializar estatísticas
    EventsBroadcasted = 0;
    EventsReceived = 0;
    EventsBlocked = 0;
    EventsFiltered = 0;
    TotalProcessingTime = 0.0f;
    
    // Configurar filtros padrão
    NetworkFilter.Add(EPlanetEventType::NetworkSync);
    NetworkFilter.Add(EPlanetEventType::Info);
    NetworkFilter.Add(EPlanetEventType::Warning);
    NetworkFilter.Add(EPlanetEventType::Error);
    
    // Configurar eventos bloqueados
    BlockedEvents.Add(EPlanetEventType::Debug);
    
    // Log de inicialização
    if (Logger)
    {
        Logger->LogInfo(TEXT("PlanetNetworkEventBus"), TEXT("Sistema de eventos de rede inicializado"));
    }
}

void UPlanetNetworkEventBus::BroadcastNetworkEvent(const FPlanetSystemEvent& Event)
{
    const double StartTime = FPlatformTime::Seconds();
    
    try
    {
        if (!bEnabled)
        {
            LogNetworkEvent(EPlanetEventType::Warning, TEXT("Tentativa de broadcast com sistema desabilitado"));
            return;
        }
        
        if (!ValidateEvent(Event))
        {
            LogNetworkEvent(EPlanetEventType::Error, TEXT("Evento inválido para broadcast"));
            return;
        }
        
        // Verificar filtros
        if (!ShouldBroadcastEvent(Event))
        {
            EventsFiltered++;
            LogNetworkEvent(EPlanetEventType::Info, TEXT("Evento filtrado e não transmitido"));
            return;
        }
        
        // Verificar se está bloqueado
        if (IsEventTypeBlocked(Event.EventType))
        {
            EventsBlocked++;
            LogNetworkEvent(EPlanetEventType::Warning, TEXT("Evento bloqueado e não transmitido"));
            return;
        }
        
        // Adicionar ao histórico
        AddToHistory(Event);
        
        // Transmitir evento
        if (EventBus)
        {
            EventBus->BroadcastEvent(Event);
        }
        
        EventsBroadcasted++;
        
        // Calcular tempo de processamento
        const double EndTime = FPlatformTime::Seconds();
        TotalProcessingTime += static_cast<float>(EndTime - StartTime);
        
        LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Evento transmitido: %s"), *Event.Details));
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro no broadcast de evento: %s"), UTF8_TO_TCHAR(e.what()));
        LogNetworkEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

void UPlanetNetworkEventBus::BroadcastNetworkEventWithParams(EPlanetEventType EventType, const FString& Source, const FString& Details, int32 Priority)
{
    try
    {
        FPlanetSystemEvent Event;
        Event.EventType = EventType;
        Event.Source = Source;
        Event.Details = Details;
        Event.Priority = Priority;
        Event.Timestamp = FDateTime::Now();
        
        BroadcastNetworkEvent(Event);
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro no broadcast com parâmetros: %s"), UTF8_TO_TCHAR(e.what()));
        LogNetworkEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

void UPlanetNetworkEventBus::OnNetworkEventReceived(const FPlanetSystemEvent& Event)
{
    const double StartTime = FPlatformTime::Seconds();
    
    try
    {
        if (!bEnabled)
        {
            return;
        }
        
        if (!ValidateEvent(Event))
        {
            LogNetworkEvent(EPlanetEventType::Error, TEXT("Evento recebido inválido"));
            return;
        }
        
        // Processar evento recebido
        ProcessReceivedEvent(Event);
        
        // Adicionar ao histórico
        AddToHistory(Event);
        
        EventsReceived++;
        
        // Calcular tempo de processamento
        const double EndTime = FPlatformTime::Seconds();
        TotalProcessingTime += static_cast<float>(EndTime - StartTime);
        
        LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Evento recebido: %s"), *Event.Details));
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro no processamento de evento recebido: %s"), UTF8_TO_TCHAR(e.what()));
        LogNetworkEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

bool UPlanetNetworkEventBus::ShouldBroadcastEvent(const FPlanetSystemEvent& Event)
{
    // Verificar se evento está no filtro
    if (NetworkFilter.Num() > 0)
    {
        return IsEventTypeInFilter(Event.EventType);
    }
    
    // Se não há filtro, transmitir todos (exceto bloqueados)
    return !IsEventTypeBlocked(Event.EventType);
}

void UPlanetNetworkEventBus::SetNetworkFilter(const TArray<EPlanetEventType>& AllowedEvents)
{
    NetworkFilter = AllowedEvents;
    LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Filtro de rede atualizado: %d tipos permitidos"), AllowedEvents.Num()));
}

void UPlanetNetworkEventBus::AddEventTypeToFilter(EPlanetEventType EventType)
{
    if (!NetworkFilter.Contains(EventType))
    {
        NetworkFilter.Add(EventType);
        LogNetworkEvent(EPlanetEventType::Info, TEXT("Tipo de evento adicionado ao filtro"));
    }
}

void UPlanetNetworkEventBus::RemoveEventTypeFromFilter(EPlanetEventType EventType)
{
    if (NetworkFilter.Contains(EventType))
    {
        NetworkFilter.Remove(EventType);
        LogNetworkEvent(EPlanetEventType::Info, TEXT("Tipo de evento removido do filtro"));
    }
}

void UPlanetNetworkEventBus::ClearNetworkFilter()
{
    NetworkFilter.Empty();
    LogNetworkEvent(EPlanetEventType::Info, TEXT("Filtro de rede limpo"));
}

void UPlanetNetworkEventBus::GetEventHistory(TArray<FPlanetSystemEvent>& OutHistory, int32 MaxEvents)
{
    try
    {
        OutHistory.Empty();
        
        const int32 StartIndex = FMath::Max(0, EventHistory.Num() - MaxEvents);
        const int32 Count = FMath::Min(MaxEvents, EventHistory.Num() - StartIndex);
        
        for (int32 i = StartIndex; i < StartIndex + Count; i++)
        {
            OutHistory.Add(EventHistory[i]);
        }
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro ao obter histórico: %s"), UTF8_TO_TCHAR(e.what()));
        LogNetworkEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

void UPlanetNetworkEventBus::ClearEventHistory()
{
    const int32 PreviousSize = EventHistory.Num();
    EventHistory.Empty();
    LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Histórico limpo: %d eventos removidos"), PreviousSize));
}

void UPlanetNetworkEventBus::SetMaxHistorySize(int32 MaxSize)
{
    MaxHistorySize = FMath::Max(1, MaxSize);
    CleanupOldHistory();
    LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Tamanho máximo do histórico definido: %d"), MaxSize));
}

void UPlanetNetworkEventBus::SetEnabled(bool bInEnabled)
{
    bEnabled = bInEnabled;
    LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Sistema %s"), bEnabled ? TEXT("habilitado") : TEXT("desabilitado")));
}

bool UPlanetNetworkEventBus::IsEnabled() const
{
    return bEnabled;
}

void UPlanetNetworkEventBus::SetNetworkMode(bool bInIsServer)
{
    bIsServer = bInIsServer;
    LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Modo de rede definido: %s"), bIsServer ? TEXT("Servidor") : TEXT("Cliente")));
}

void UPlanetNetworkEventBus::GetNetworkEventStats(FString& OutStats) const
{
    const float AverageProcessingTime = EventsBroadcasted + EventsReceived > 0 ? 
        TotalProcessingTime / (EventsBroadcasted + EventsReceived) : 0.0f;
    
    OutStats = FString::Printf(
        TEXT("=== Estatísticas de Eventos de Rede ===\n")
        TEXT("Habilitado: %s\n")
        TEXT("Modo: %s\n")
        TEXT("Eventos Transmitidos: %d\n")
        TEXT("Eventos Recebidos: %d\n")
        TEXT("Eventos Bloqueados: %d\n")
        TEXT("Eventos Filtrados: %d\n")
        TEXT("Tempo Total: %.3fms\n")
        TEXT("Tempo Médio: %.3fms\n")
        TEXT("Histórico: %d/%d\n")
        TEXT("Filtros: %d tipos\n"),
        bEnabled ? TEXT("Sim") : TEXT("Não"),
        bIsServer ? TEXT("Servidor") : TEXT("Cliente"),
        EventsBroadcasted,
        EventsReceived,
        EventsBlocked,
        EventsFiltered,
        TotalProcessingTime * 1000.0f,
        AverageProcessingTime * 1000.0f,
        EventHistory.Num(),
        MaxHistorySize,
        NetworkFilter.Num()
    );
}

void UPlanetNetworkEventBus::ResetStats()
{
    EventsBroadcasted = 0;
    EventsReceived = 0;
    EventsBlocked = 0;
    EventsFiltered = 0;
    TotalProcessingTime = 0.0f;
    
    LogNetworkEvent(EPlanetEventType::Info, TEXT("Estatísticas de eventos de rede resetadas"));
}

void UPlanetNetworkEventBus::AddToHistory(const FPlanetSystemEvent& Event)
{
    try
    {
        EventHistory.Add(Event);
        CleanupOldHistory();
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro ao adicionar ao histórico: %s"), UTF8_TO_TCHAR(e.what()));
        LogNetworkEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

void UPlanetNetworkEventBus::CleanupOldHistory()
{
    try
    {
        while (EventHistory.Num() > MaxHistorySize)
        {
            EventHistory.RemoveAt(0);
        }
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro na limpeza do histórico: %s"), UTF8_TO_TCHAR(e.what()));
        LogNetworkEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

bool UPlanetNetworkEventBus::IsEventTypeInFilter(EPlanetEventType EventType) const
{
    return NetworkFilter.Contains(EventType);
}

bool UPlanetNetworkEventBus::IsEventTypeBlocked(EPlanetEventType EventType) const
{
    return BlockedEvents.Contains(EventType);
}

bool UPlanetNetworkEventBus::ValidateEvent(const FPlanetSystemEvent& Event) const
{
    // Verificar se evento é válido
    if (Event.Source.IsEmpty())
    {
        return false;
    }
    
    if (Event.Details.IsEmpty())
    {
        return false;
    }
    
    // Verificar se timestamp é válido
    if (!Event.Timestamp.IsValid())
    {
        return false;
    }
    
    return true;
}

void UPlanetNetworkEventBus::LogNetworkEvent(EPlanetEventType EventType, const FString& Details)
{
    if (!bLogEvents)
    {
        return;
    }
    
    if (Logger)
    {
        Logger->LogInfo(TEXT("PlanetNetworkEventBus"), Details);
    }
    
    // Não usar EventBus aqui para evitar loop infinito
}

int32 UPlanetNetworkEventBus::CalculateEventPriority(const FPlanetSystemEvent& Event) const
{
    // Prioridade baseada no tipo de evento
    int32 BasePriority = 0;
    
    switch (Event.EventType)
    {
        case EPlanetEventType::Error:
            BasePriority = 100;
            break;
        case EPlanetEventType::Warning:
            BasePriority = 50;
            break;
        case EPlanetEventType::NetworkSync:
            BasePriority = 75;
            break;
        case EPlanetEventType::Success:
            BasePriority = 25;
            break;
        case EPlanetEventType::Info:
            BasePriority = 10;
            break;
        case EPlanetEventType::Debug:
            BasePriority = 1;
            break;
        default:
            BasePriority = 0;
            break;
    }
    
    // Adicionar prioridade personalizada
    return BasePriority + Event.Priority;
}

void UPlanetNetworkEventBus::ProcessReceivedEvent(const FPlanetSystemEvent& Event)
{
    try
    {
        // Processar evento baseado no tipo
        switch (Event.EventType)
        {
            case EPlanetEventType::NetworkSync:
                // Eventos de sincronização são processados automaticamente
                break;
                
            case EPlanetEventType::Error:
                // Logar erros
                if (Logger)
                {
                    Logger->LogError(TEXT("NetworkEventBus"), Event.Details);
                }
                break;
                
            case EPlanetEventType::Warning:
                // Logar avisos
                if (Logger)
                {
                    Logger->LogWarning(TEXT("NetworkEventBus"), Event.Details);
                }
                break;
                
            default:
                // Processar outros tipos normalmente
                break;
        }
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro no processamento de evento: %s"), UTF8_TO_TCHAR(e.what()));
        LogNetworkEvent(EPlanetEventType::Error, ErrorMsg);
    }
} 