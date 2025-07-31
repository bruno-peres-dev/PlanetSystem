#include "Network/PlanetSystemNetworkManager.h"
#include "Network/PlanetChunkNetworkCache.h"
#include "Network/PlanetNetworkEventBus.h"
#include "Debug/Logging/PlanetSystemLogger.h"
#include "Core/Events/PlanetEventBus.h"
#include "Configuration/DataAssets/CoreConfig.h"
#include "Serialization/PlanetDataSerializer.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Json/Public/Json.h"
#include "Json/Public/JsonUtilities.h"

UPlanetSystemNetworkManager::UPlanetSystemNetworkManager()
{
    // Inicializar componentes
    ChunkCache = NewObject<UPlanetChunkNetworkCache>(this, UPlanetChunkNetworkCache::StaticClass(), TEXT("ChunkCache"));
    if (ChunkCache)
    {
        ChunkCache->AddToRoot();
    }

    NetworkEventBus = NewObject<UPlanetNetworkEventBus>(this, UPlanetNetworkEventBus::StaticClass(), TEXT("NetworkEventBus"));
    if (NetworkEventBus)
    {
        NetworkEventBus->AddToRoot();
    }
    
    // Obter serviços do ServiceLocator
    Logger = UPlanetSystemLogger::GetInstance();
    EventBus = UPlanetEventBus::GetInstance();
    
    // Inicializar estado
    bIsServer = false;
    bIsConnected = false;
    bIsInitialized = false;
    GlobalSeed = 0;
    LastSyncTime = FDateTime::Now();
    
    // Inicializar estatísticas
    ChunksSent = 0;
    ChunksReceived = 0;
    BytesSent = 0;
    BytesReceived = 0;
    TotalNetworkTime = 0.0f;
    AverageLatency = 0.0f;
    
    // Log de inicialização
    if (Logger)
    {
        Logger->LogInfo(TEXT("PlanetSystemNetworkManager"), TEXT("Sistema de rede inicializado"));
    }
}

void UPlanetSystemNetworkManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UPlanetSystemNetworkManager, GlobalSeed);
    DOREPLIFETIME(UPlanetSystemNetworkManager, SynchronizedConfigData);
    DOREPLIFETIME(UPlanetSystemNetworkManager, LastSyncTime);
}

void UPlanetSystemNetworkManager::Server_SetGlobalSeed_Implementation(int32 NewSeed)
{
    if (!bIsServer)
    {
        LogNetworkEvent(EPlanetEventType::Warning, TEXT("Tentativa de definir seed em cliente"));
        return;
    }
    
    GlobalSeed = NewSeed;
    LastSyncTime = FDateTime::Now();
    
    LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Seed global definida: %d"), NewSeed));
    
    // Broadcast para todos os clientes
    if (NetworkEventBus)
    {
        FPlanetSystemEvent Event;
        Event.EventType = EPlanetEventType::NetworkSync;
        Event.Source = TEXT("NetworkManager");
        Event.Details = FString::Printf(TEXT("Seed global sincronizada: %d"), NewSeed);
        Event.Timestamp = FDateTime::Now();
        
        NetworkEventBus->BroadcastNetworkEvent(Event);
    }
}

void UPlanetSystemNetworkManager::Client_ReceiveGlobalSeed_Implementation(int32 NewSeed)
{
    if (bIsServer)
    {
        return; // Servidor não deve receber seeds
    }
    
    GlobalSeed = NewSeed;
    
    LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Seed global recebida: %d"), NewSeed));
    
    // Notificar sistemas locais
    if (EventBus)
    {
        FPlanetSystemEvent Event;
        Event.EventType = EPlanetEventType::NetworkSync;
        Event.Source = TEXT("NetworkManager");
        Event.Details = FString::Printf(TEXT("Seed global atualizada: %d"), NewSeed);
        Event.Timestamp = FDateTime::Now();
        
        EventBus->BroadcastEvent(Event);
    }
}

void UPlanetSystemNetworkManager::Server_SynchronizeConfig_Implementation(const UPlanetCoreConfig* Config)
{
    if (!bIsServer || !Config)
    {
        LogNetworkEvent(EPlanetEventType::Warning, TEXT("Tentativa de sincronizar configuração inválida"));
        return;
    }
    
    // Serializar configuração
    SynchronizedConfigData = SerializeConfig(Config);
    LastSyncTime = FDateTime::Now();
    
    LogNetworkEvent(EPlanetEventType::Info, TEXT("Configuração sincronizada no servidor"));
    
    // Broadcast para todos os clientes
    if (NetworkEventBus)
    {
        FPlanetSystemEvent Event;
        Event.EventType = EPlanetEventType::NetworkSync;
        Event.Source = TEXT("NetworkManager");
        Event.Details = TEXT("Configuração sincronizada");
        Event.Timestamp = FDateTime::Now();
        
        NetworkEventBus->BroadcastNetworkEvent(Event);
    }
}

void UPlanetSystemNetworkManager::Client_ReceiveSynchronizedConfig_Implementation(const FString& ConfigData)
{
    if (bIsServer)
    {
        return; // Servidor não deve receber configurações
    }
    
    // Deserializar configuração
    UPlanetCoreConfig* Config = DeserializeConfig(ConfigData);
    if (Config)
    {
        // Armazenar no cache
        ConfigCache.Add(ConfigData, Config);
        
        LogNetworkEvent(EPlanetEventType::Info, TEXT("Configuração recebida e armazenada"));
        
        // Notificar sistemas locais
        if (EventBus)
        {
            FPlanetSystemEvent Event;
            Event.EventType = EPlanetEventType::NetworkSync;
            Event.Source = TEXT("NetworkManager");
            Event.Details = TEXT("Configuração atualizada");
            Event.Timestamp = FDateTime::Now();
            
            EventBus->BroadcastEvent(Event);
        }
    }
    else
    {
        LogNetworkEvent(EPlanetEventType::Error, TEXT("Falha ao deserializar configuração"));
    }
}

void UPlanetSystemNetworkManager::Server_RequestChunk_Implementation(const FVector& Position, int32 LODLevel)
{
    if (!bIsServer)
    {
        LogNetworkEvent(EPlanetEventType::Warning, TEXT("Tentativa de solicitar chunk em cliente"));
        return;
    }
    
    if (!ValidateInput(Position, LODLevel))
    {
        LogNetworkEvent(EPlanetEventType::Error, TEXT("Parâmetros inválidos para solicitação de chunk"));
        return;
    }
    
    // Verificar cache primeiro
    if (ChunkCache)
    {
        FPlanetChunk CachedChunk;
        if (ChunkCache->GetChunk(Position, CachedChunk))
        {
            // Enviar chunk do cache
            const FString ChunkData = SerializeChunk(CachedChunk);
            Client_ReceiveChunk_Implementation(Position, ChunkData);
            
            LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Chunk enviado do cache: %s"), *Position.ToString()));
            return;
        }
    }
    
    // Chunk não encontrado no cache - gerar ou buscar
    LogNetworkEvent(EPlanetEventType::Warning, FString::Printf(TEXT("Chunk não encontrado no cache: %s"), *Position.ToString()));
}

void UPlanetSystemNetworkManager::Client_ReceiveChunk_Implementation(const FVector& Position, const FString& ChunkData)
{
    if (bIsServer)
    {
        return; // Servidor não deve receber chunks
    }
    
    // Deserializar chunk
    FPlanetChunk Chunk;
    if (DeserializeChunk(ChunkData, Chunk))
    {
        // Armazenar no cache local
        if (ChunkCache)
        {
            ChunkCache->SynchronizeChunk(Position, Chunk);
        }
        
        ChunksReceived++;
        BytesReceived += ChunkData.Len();
        
        LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Chunk recebido: %s"), *Position.ToString()));
        
        // Notificar sistemas locais
        if (EventBus)
        {
            FPlanetSystemEvent Event;
            Event.EventType = EPlanetEventType::NetworkSync;
            Event.Source = TEXT("NetworkManager");
            Event.Details = FString::Printf(TEXT("Chunk recebido: %s"), *Position.ToString());
            Event.Timestamp = FDateTime::Now();
            
            EventBus->BroadcastEvent(Event);
        }
    }
    else
    {
        LogNetworkEvent(EPlanetEventType::Error, FString::Printf(TEXT("Falha ao deserializar chunk: %s"), *Position.ToString()));
    }
}

void UPlanetSystemNetworkManager::Server_SynchronizeChunk_Implementation(const FVector& Position, const FPlanetChunk& Chunk)
{
    if (!bIsServer)
    {
        LogNetworkEvent(EPlanetEventType::Warning, TEXT("Tentativa de sincronizar chunk em cliente"));
        return;
    }
    
    if (!ValidateInput(Position, Chunk.LODLevel))
    {
        LogNetworkEvent(EPlanetEventType::Error, TEXT("Parâmetros inválidos para sincronização de chunk"));
        return;
    }
    
    // Armazenar no cache do servidor
    if (ChunkCache)
    {
        ChunkCache->SynchronizeChunk(Position, Chunk);
    }
    
    // Armazenar timestamp
    ChunkTimestamps.Add(Position, FDateTime::Now());
    
    ChunksSent++;
    
    LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Chunk sincronizado no servidor: %s"), *Position.ToString()));
}

void UPlanetSystemNetworkManager::Server_ValidateConfiguration_Implementation(const UPlanetCoreConfig* Config)
{
    if (!bIsServer || !Config)
    {
        LogNetworkEvent(EPlanetEventType::Warning, TEXT("Tentativa de validar configuração inválida"));
        return;
    }
    
    // Validar configuração
    TArray<FPlanetValidationError> Errors;
    const bool bIsValid = Config->ValidateConfiguration(Errors);
    
    // Converter erros para strings
    TArray<FString> ErrorStrings;
    for (const FPlanetValidationError& Error : Errors)
    {
        ErrorStrings.Add(Error.ErrorMessage);
    }
    
    // Enviar resultado para o cliente
    Client_ConfigurationValidated_Implementation(bIsValid, ErrorStrings);
    
    LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Configuração validada: %s"), bIsValid ? TEXT("Válida") : TEXT("Inválida")));
}

void UPlanetSystemNetworkManager::Client_ConfigurationValidated_Implementation(bool bIsValid, const TArray<FString>& Errors)
{
    if (bIsServer)
    {
        return; // Servidor não deve receber validações
    }
    
    LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Validação recebida: %s"), bIsValid ? TEXT("Válida") : TEXT("Inválida")));
    
    if (!bIsValid)
    {
        for (const FString& Error : Errors)
        {
            LogNetworkEvent(EPlanetEventType::Warning, FString::Printf(TEXT("Erro de validação: %s"), *Error));
        }
    }
    
    // Notificar sistemas locais
    if (EventBus)
    {
        FPlanetSystemEvent Event;
        Event.EventType = EPlanetEventType::NetworkSync;
        Event.Source = TEXT("NetworkManager");
        Event.Details = FString::Printf(TEXT("Validação concluída: %s"), bIsValid ? TEXT("Válida") : TEXT("Inválida"));
        Event.Timestamp = FDateTime::Now();
        
        EventBus->BroadcastEvent(Event);
    }
}

void UPlanetSystemNetworkManager::InitializeNetwork(bool bInIsServer)
{
    bIsServer = bInIsServer;
    bIsInitialized = true;
    
    if (bIsServer)
    {
        LogNetworkEvent(EPlanetEventType::Info, TEXT("Sistema de rede inicializado como servidor"));
    }
    else
    {
        LogNetworkEvent(EPlanetEventType::Info, TEXT("Sistema de rede inicializado como cliente"));
    }
    
    // Inicializar componentes de rede
    if (ChunkCache)
    {
        // ChunkCache já inicializado no construtor
    }
    
    if (NetworkEventBus)
    {
        // NetworkEventBus já inicializado no construtor
    }
}

void UPlanetSystemNetworkManager::ShutdownNetwork()
{
    bIsConnected = false;
    bIsInitialized = false;
    
    // Limpar caches
    ConfigCache.Empty();
    PendingChunks.Empty();
    ChunkTimestamps.Empty();
    
    LogNetworkEvent(EPlanetEventType::Info, TEXT("Sistema de rede finalizado"));
}

bool UPlanetSystemNetworkManager::IsNetworkConnected() const
{
    return bIsConnected && bIsInitialized;
}

void UPlanetSystemNetworkManager::GetNetworkStats(FString& OutStats) const
{
    OutStats = FString::Printf(
        TEXT("=== Estatísticas de Rede ===\n")
        TEXT("Conectado: %s\n")
        TEXT("Servidor: %s\n")
        TEXT("Chunks Enviados: %d\n")
        TEXT("Chunks Recebidos: %d\n")
        TEXT("Bytes Enviados: %lld\n")
        TEXT("Bytes Recebidos: %lld\n")
        TEXT("Tempo Total: %.3fms\n")
        TEXT("Latência Média: %.3fms\n")
        TEXT("Seed Global: %d\n")
        TEXT("Última Sincronização: %s\n"),
        bIsConnected ? TEXT("Sim") : TEXT("Não"),
        bIsServer ? TEXT("Sim") : TEXT("Não"),
        ChunksSent,
        ChunksReceived,
        BytesSent,
        BytesReceived,
        TotalNetworkTime * 1000.0f,
        AverageLatency * 1000.0f,
        GlobalSeed,
        *LastSyncTime.ToString()
    );
}

void UPlanetSystemNetworkManager::ResetNetworkStats()
{
    ChunksSent = 0;
    ChunksReceived = 0;
    BytesSent = 0;
    BytesReceived = 0;
    TotalNetworkTime = 0.0f;
    AverageLatency = 0.0f;
    
    LogNetworkEvent(EPlanetEventType::Info, TEXT("Estatísticas de rede resetadas"));
}

void UPlanetSystemNetworkManager::OnRep_GlobalSeed()
{
    LogNetworkEvent(EPlanetEventType::Info, FString::Printf(TEXT("Seed global replicada: %d"), GlobalSeed));
}

void UPlanetSystemNetworkManager::OnRep_SynchronizedConfig()
{
    LogNetworkEvent(EPlanetEventType::Info, TEXT("Configuração sincronizada replicada"));
}

FString UPlanetSystemNetworkManager::SerializeChunk(const FPlanetChunk& Chunk) const
{
    try
    {
        // Usar o sistema de serialização existente
        UPlanetDataSerializer* Serializer = UPlanetDataSerializer::GetInstance();
        if (Serializer)
        {
            FString SerializedData;
            if (Serializer->SerializeChunk(Chunk, SerializedData))
            {
                return SerializedData;
            }
        }
        
        // Fallback: serialização JSON simples
        TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
        
        // Serializar dados básicos
        JsonObject->SetNumberField(TEXT("CenterX"), Chunk.Center.X);
        JsonObject->SetNumberField(TEXT("CenterY"), Chunk.Center.Y);
        JsonObject->SetNumberField(TEXT("CenterZ"), Chunk.Center.Z);
        JsonObject->SetNumberField(TEXT("LODLevel"), Chunk.LODLevel);
        
        // Serializar mapa de altura
        TArray<TSharedPtr<FJsonValue>> HeightMapArray;
        for (float Height : Chunk.HeightMap)
        {
            HeightMapArray.Add(MakeShared<FJsonValueNumber>(Height));
        }
        JsonObject->SetArrayField(TEXT("HeightMap"), HeightMapArray);
        
        // Serializar mapa de biomas
        TArray<TSharedPtr<FJsonValue>> BiomeMapArray;
        for (EBiomeType Biome : Chunk.BiomeMap)
        {
            BiomeMapArray.Add(MakeShared<FJsonValueNumber>(static_cast<int32>(Biome)));
        }
        JsonObject->SetArrayField(TEXT("BiomeMap"), BiomeMapArray);
        
        // Converter para string
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
        
        return OutputString;
    }
    catch (const std::exception& e)
    {
        LogNetworkEvent(EPlanetEventType::Error, FString::Printf(TEXT("Erro na serialização de chunk: %s"), UTF8_TO_TCHAR(e.what())));
        return TEXT("");
    }
}

bool UPlanetSystemNetworkManager::DeserializeChunk(const FString& ChunkData, FPlanetChunk& OutChunk) const
{
    try
    {
        // Usar o sistema de serialização existente
        UPlanetDataSerializer* Serializer = UPlanetDataSerializer::GetInstance();
        if (Serializer)
        {
            if (Serializer->DeserializeChunk(ChunkData, OutChunk))
            {
                return true;
            }
        }
        
        // Fallback: deserialização JSON simples
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ChunkData);
        
        if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        {
            return false;
        }
        
        // Deserializar dados básicos
        OutChunk.Center.X = JsonObject->GetNumberField(TEXT("CenterX"));
        OutChunk.Center.Y = JsonObject->GetNumberField(TEXT("CenterY"));
        OutChunk.Center.Z = JsonObject->GetNumberField(TEXT("CenterZ"));
        OutChunk.LODLevel = JsonObject->GetIntegerField(TEXT("LODLevel"));
        
        // Deserializar mapa de altura
        const TArray<TSharedPtr<FJsonValue>>* HeightMapArray;
        if (JsonObject->TryGetArrayField(TEXT("HeightMap"), HeightMapArray))
        {
            OutChunk.HeightMap.Empty();
            for (const TSharedPtr<FJsonValue>& Value : *HeightMapArray)
            {
                OutChunk.HeightMap.Add(Value->AsNumber());
            }
        }
        
        // Deserializar mapa de biomas
        const TArray<TSharedPtr<FJsonValue>>* BiomeMapArray;
        if (JsonObject->TryGetArrayField(TEXT("BiomeMap"), BiomeMapArray))
        {
            OutChunk.BiomeMap.Empty();
            for (const TSharedPtr<FJsonValue>& Value : *BiomeMapArray)
            {
                OutChunk.BiomeMap.Add(static_cast<EBiomeType>(Value->AsNumber()));
            }
        }
        
        return true;
    }
    catch (const std::exception& e)
    {
        LogNetworkEvent(EPlanetEventType::Error, FString::Printf(TEXT("Erro na deserialização de chunk: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

FString UPlanetSystemNetworkManager::SerializeConfig(const UPlanetCoreConfig* Config) const
{
    try
    {
        // Usar o sistema de serialização existente
        UPlanetDataSerializer* Serializer = UPlanetDataSerializer::GetInstance();
        if (Serializer)
        {
            FString SerializedData;
            if (Serializer->SerializeConfig(Config, SerializedData))
            {
                return SerializedData;
            }
        }
        
        // Fallback: serialização JSON simples
        TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
        
        // Serializar dados básicos da configuração
        JsonObject->SetNumberField(TEXT("PlanetRadius"), Config->GenerationConfig.PlanetRadius);
        JsonObject->SetNumberField(TEXT("BaseResolution"), Config->GenerationConfig.BaseResolution);
        JsonObject->SetNumberField(TEXT("ChunkSize"), Config->GenerationConfig.ChunkSize);
        
        // Converter para string
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
        
        return OutputString;
    }
    catch (const std::exception& e)
    {
        LogNetworkEvent(EPlanetEventType::Error, FString::Printf(TEXT("Erro na serialização de configuração: %s"), UTF8_TO_TCHAR(e.what())));
        return TEXT("");
    }
}

UPlanetCoreConfig* UPlanetSystemNetworkManager::DeserializeConfig(const FString& ConfigData) const
{
    try
    {
        // Usar o sistema de serialização existente
        UPlanetDataSerializer* Serializer = UPlanetDataSerializer::GetInstance();
        if (Serializer)
        {
            UPlanetCoreConfig* Config = nullptr;
            if (Serializer->DeserializeConfig(ConfigData, Config))
            {
                return Config;
            }
        }
        
        // Fallback: deserialização JSON simples
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ConfigData);
        
        if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        {
            return nullptr;
        }
        
        // Criar nova configuração
        UPlanetCoreConfig* Config = NewObject<UPlanetCoreConfig>();
        
        // Deserializar dados básicos
        Config->GenerationConfig.PlanetRadius = JsonObject->GetNumberField(TEXT("PlanetRadius"));
        Config->GenerationConfig.BaseResolution = JsonObject->GetIntegerField(TEXT("BaseResolution"));
        Config->GenerationConfig.ChunkSize = JsonObject->GetNumberField(TEXT("ChunkSize"));
        
        return Config;
    }
    catch (const std::exception& e)
    {
        LogNetworkEvent(EPlanetEventType::Error, FString::Printf(TEXT("Erro na deserialização de configuração: %s"), UTF8_TO_TCHAR(e.what())));
        return nullptr;
    }
}

bool UPlanetSystemNetworkManager::ValidateInput(const FVector& Position, int32 LODLevel) const
{
    if (!FMath::IsFinite(Position.X) || !FMath::IsFinite(Position.Y) || !FMath::IsFinite(Position.Z))
    {
        return false;
    }
    
    if (LODLevel < 0 || LODLevel > 10)
    {
        return false;
    }
    
    return true;
}

void UPlanetSystemNetworkManager::LogNetworkEvent(EPlanetEventType EventType, const FString& Details)
{
    if (Logger)
    {
        Logger->LogInfo(TEXT("PlanetSystemNetworkManager"), Details);
    }
    
    if (EventBus)
    {
        FPlanetSystemEvent Event;
        Event.EventType = EventType;
        Event.Source = TEXT("PlanetSystemNetworkManager");
        Event.Details = Details;
        Event.Timestamp = FDateTime::Now();
        
        EventBus->BroadcastEvent(Event);
    }
}

void UPlanetSystemNetworkManager::CleanupOldCache()
{
    const FDateTime Now = FDateTime::Now();
    const FTimespan MaxAge = FTimespan::FromMinutes(30); // 30 minutos
    
    // Limpar chunks antigos
    TArray<FVector> KeysToRemove;
    for (const auto& Pair : ChunkTimestamps)
    {
        if (Now - Pair.Value > MaxAge)
        {
            KeysToRemove.Add(Pair.Key);
        }
    }
    
    for (const FVector& Key : KeysToRemove)
    {
        ChunkTimestamps.Remove(Key);
        PendingChunks.Remove(Key);
    }
    
    // Limpar configurações antigas
    ConfigCache.Empty();
}

float UPlanetSystemNetworkManager::CalculateLatency(const FDateTime& StartTime) const
{
    const FDateTime Now = FDateTime::Now();
    const FTimespan Duration = Now - StartTime;
    return Duration.GetTotalMilliseconds() / 1000.0f; // Converter para segundos
} 