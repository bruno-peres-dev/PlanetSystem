#include "Core/Events/PlanetEventBus.h"
#include "Debug/Logging/PlanetSystemLogger.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"

// Instância singleton
UPlanetEventBus* UPlanetEventBus::Instance = nullptr;

UPlanetEventBus::UPlanetEventBus()
{
    UPlanetSystemLogger::LogInfo(TEXT("PlanetEventBus"), TEXT("EventBus created"));
}

UPlanetEventBus* UPlanetEventBus::GetInstance()
{
    if (!Instance)
    {
        Instance = NewObject<UPlanetEventBus>();
        Instance->AddToRoot(); // Previne garbage collection
        UPlanetSystemLogger::LogInfo(TEXT("PlanetEventBus"), TEXT("EventBus singleton created"));
    }
    return Instance;
}

void UPlanetEventBus::RegisterListener(EPlanetEventType EventType, UObject* Listener, const FString& FunctionName)
{
    if (!Listener)
    {
        UPlanetSystemLogger::LogWarning(TEXT("PlanetEventBus"), TEXT("Attempted to register null listener"));
        return;
    }
    
    // Verifica se já existe
    if (EventListeners.Contains(EventType))
    {
        for (const FEventListener& ExistingListener : EventListeners[EventType])
        {
            if (ExistingListener.Listener == Listener && ExistingListener.FunctionName == FunctionName)
            {
                UPlanetSystemLogger::LogWarning(TEXT("PlanetEventBus"), 
                    FString::Printf(TEXT("Listener already registered for event type %d"), (int32)EventType));
                return;
            }
        }
    }
    
    // Adiciona o listener
    EventListeners.FindOrAdd(EventType).Add(FEventListener(Listener, FunctionName));
    
    UPlanetSystemLogger::LogInfo(TEXT("PlanetEventBus"), 
        FString::Printf(TEXT("Listener registered for event type %d. Total listeners: %d"), 
        (int32)EventType, EventListeners[EventType].Num()));
}

void UPlanetEventBus::UnregisterListener(EPlanetEventType EventType, UObject* Listener)
{
    if (!EventListeners.Contains(EventType))
    {
        return;
    }
    
    TArray<FEventListener>& Listeners = EventListeners[EventType];
    Listeners.RemoveAll([Listener](const FEventListener& EventListener)
    {
        return EventListener.Listener == Listener;
    });
    
    UPlanetSystemLogger::LogInfo(TEXT("PlanetEventBus"), 
        FString::Printf(TEXT("Listener unregistered from event type %d. Remaining listeners: %d"), 
        (int32)EventType, Listeners.Num()));
}

void UPlanetEventBus::UnregisterListenerFromAll(UObject* Listener)
{
    int32 TotalRemoved = 0;
    
    for (auto& EventTypePair : EventListeners)
    {
        TArray<FEventListener>& Listeners = EventTypePair.Value;
        int32 BeforeCount = Listeners.Num();
        
        Listeners.RemoveAll([Listener](const FEventListener& EventListener)
        {
            return EventListener.Listener == Listener;
        });
        
        TotalRemoved += (BeforeCount - Listeners.Num());
    }
    
    UPlanetSystemLogger::LogInfo(TEXT("PlanetEventBus"), 
        FString::Printf(TEXT("Listener unregistered from all events. Total removed: %d"), TotalRemoved));
}

void UPlanetEventBus::BroadcastEvent(const FPlanetSystemEvent& Event)
{
    TotalEventsBroadcasted++;
    LastEventTime = FPlatformTime::Seconds();
    
    // Log do evento se habilitado
    if (bEventLoggingEnabled)
    {
        UPlanetSystemLogger::LogEvent(TEXT("PlanetEventBus"), Event);
    }
    
    // Adiciona ao histórico
    AddToHistory(Event);
    
    // Processa o evento
    ProcessEvent(Event);
    
    // Dispara o delegate
    OnEventReceived.Broadcast(Event);
}

void UPlanetEventBus::BroadcastEventWithParams(EPlanetEventType EventType, const FString& CustomName, 
                                              const FString& StringParam, float FloatParam, int32 IntParam)
{
    FPlanetSystemEvent Event;
    Event.EventType = EventType;
    Event.CustomName = CustomName;
    Event.Timestamp = FPlatformTime::Seconds();
    Event.Priority = 1.0f;
    Event.StringParam = StringParam;
    Event.FloatParam = FloatParam;
    Event.IntParam = IntParam;
    Event.UniqueID = FGuid::NewGuid();
    Event.SourceModule = TEXT("PlanetEventBus");
    
    BroadcastEvent(Event);
}

void UPlanetEventBus::GetEventBusStats(FString& OutStats) const
{
    OutStats = FString::Printf(TEXT("PlanetEventBus Statistics:\n")
        TEXT("- Total Events Broadcasted: %d\n")
        TEXT("- Event History Size: %d\n")
        TEXT("- Max History Size: %d\n")
        TEXT("- Event Logging Enabled: %s\n")
        TEXT("- Last Event Time: %.2f seconds ago\n")
        TEXT("- Total Event Types: %d\n"),
        TotalEventsBroadcasted,
        EventHistory.Num(),
        MaxEventHistorySize,
        bEventLoggingEnabled ? TEXT("Yes") : TEXT("No"),
        FPlatformTime::Seconds() - LastEventTime,
        EventListeners.Num());
    
    // Adiciona estatísticas por tipo de evento
    for (const auto& EventTypePair : EventListeners)
    {
        OutStats += FString::Printf(TEXT("- Event Type %d: %d listeners\n"), 
            (int32)EventTypePair.Key, EventTypePair.Value.Num());
    }
}

void UPlanetEventBus::ClearAllListeners()
{
    int32 TotalListeners = 0;
    for (const auto& EventTypePair : EventListeners)
    {
        TotalListeners += EventTypePair.Value.Num();
    }
    
    EventListeners.Empty();
    
    UPlanetSystemLogger::LogInfo(TEXT("PlanetEventBus"), 
        FString::Printf(TEXT("All listeners cleared. Total removed: %d"), TotalListeners));
}

int32 UPlanetEventBus::GetListenerCount(EPlanetEventType EventType) const
{
    if (EventListeners.Contains(EventType))
    {
        return EventListeners[EventType].Num();
    }
    return 0;
}

bool UPlanetEventBus::HasListeners(EPlanetEventType EventType) const
{
    return GetListenerCount(EventType) > 0;
}

TArray<FPlanetSystemEvent> UPlanetEventBus::GetEventHistory(int32 MaxEvents) const
{
    TArray<FPlanetSystemEvent> Result;
    
    int32 StartIndex = FMath::Max(0, EventHistory.Num() - MaxEvents);
    for (int32 i = StartIndex; i < EventHistory.Num(); ++i)
    {
        Result.Add(EventHistory[i]);
    }
    
    return Result;
}

void UPlanetEventBus::ClearEventHistory()
{
    int32 HistorySize = EventHistory.Num();
    EventHistory.Empty();
    
    UPlanetSystemLogger::LogInfo(TEXT("PlanetEventBus"), 
        FString::Printf(TEXT("Event history cleared. Removed %d events"), HistorySize));
}

void UPlanetEventBus::SetEventLoggingEnabled(bool bEnable)
{
    bEventLoggingEnabled = bEnable;
    
    UPlanetSystemLogger::LogInfo(TEXT("PlanetEventBus"), 
        FString::Printf(TEXT("Event logging %s"), bEnable ? TEXT("enabled") : TEXT("disabled")));
}

bool UPlanetEventBus::IsEventLoggingEnabled() const
{
    return bEventLoggingEnabled;
}

void UPlanetEventBus::ProcessEvent(const FPlanetSystemEvent& Event)
{
    if (!EventListeners.Contains(Event.EventType))
    {
        return;
    }
    
    const TArray<FEventListener>& Listeners = EventListeners[Event.EventType];
    int32 ProcessedCount = 0;
    
    for (const FEventListener& EventListener : Listeners)
    {
        if (EventListener.Listener && EventListener.Listener->IsValidLowLevel())
        {
            // Tenta chamar a função via reflection
            UFunction* Function = EventListener.Listener->GetClass()->FindFunctionByName(*EventListener.FunctionName);
            if (Function)
            {
                // Prepara parâmetros para a função
                uint8* Params = (uint8*)FMemory_Alloca(Function->ParmsSize);
                FMemory::Memzero(Params, Function->ParmsSize);
                
                // Copia o evento para o primeiro parâmetro
                for (TFieldIterator<FProperty> PropIt(Function); PropIt; ++PropIt)
                {
                    FProperty* Property = *PropIt;
                    if (Property->HasAnyPropertyFlags(CPF_Parm))
                    {
                        if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
                        {
                            if (StructProp->Struct == FPlanetSystemEvent::StaticStruct())
                            {
                                StructProp->CopyCompleteValue(Property->ContainerPtrToValuePtr<uint8>(Params), &Event);
                                break;
                            }
                        }
                    }
                }
                
                // Chama a função
                EventListener.Listener->ProcessEvent(Function, Params, nullptr);
                ProcessedCount++;
            }
        }
    }
    
    if (ProcessedCount > 0)
    {
        UPlanetSystemLogger::LogInfo(TEXT("PlanetEventBus"), 
            FString::Printf(TEXT("Event processed by %d listeners"), ProcessedCount));
    }
}

void UPlanetEventBus::AddToHistory(const FPlanetSystemEvent& Event)
{
    EventHistory.Add(Event);
    
    // Mantém o tamanho máximo do histórico
    if (EventHistory.Num() > MaxEventHistorySize)
    {
        EventHistory.RemoveAt(0, EventHistory.Num() - MaxEventHistorySize);
    }
}

void UPlanetEventBus::CleanupInvalidListeners()
{
    for (auto& EventTypePair : EventListeners)
    {
        TArray<FEventListener>& Listeners = EventTypePair.Value;
        Listeners.RemoveAll([](const FEventListener& EventListener)
        {
            return !EventListener.Listener || !EventListener.Listener->IsValidLowLevel();
        });
    }
} 