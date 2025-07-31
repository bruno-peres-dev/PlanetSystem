#include "Serialization/PlanetDataSerializer.h"
#include "Debug/Logging/PlanetSystemLogger.h"
#include "Core/Events/PlanetEventBus.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Misc/Compression.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformTime.h"

// Instância singleton
UPlanetDataSerializer* UPlanetDataSerializer::Instance = nullptr;

UPlanetDataSerializer::UPlanetDataSerializer()
{
    UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), TEXT("Serializer created"));
}

UPlanetDataSerializer* UPlanetDataSerializer::GetInstance()
{
    if (!Instance)
    {
        Instance = NewObject<UPlanetDataSerializer>();
        Instance->AddToRoot(); // Previne garbage collection
        UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), TEXT("Serializer singleton created"));
    }
    return Instance;
}

bool UPlanetDataSerializer::SaveCoreConfig(const UPlanetCoreConfig* Config, const FString& Filename)
{
    if (!Config)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), TEXT("Cannot save null CoreConfig"));
        return false;
    }

    const float StartTime = FPlatformTime::Seconds();
    
    try
    {
        FString FullPath = GetFullPath(TEXT("Configs"), Filename);
        FString JsonString;
        
        // Serializa para JSON
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(Config, Writer);
        Writer->Close();
        
        // Aplica compressão se habilitada
        if (bCompressionEnabled)
        {
            TArray<uint8> CompressedData;
            if (FCompression::CompressMemory(NAME_Zlib, CompressedData, (uint8*)TCHAR_TO_UTF8(*JsonString), JsonString.Len()))
            {
                if (FFileHelper::SaveArrayToFile(CompressedData, *FullPath))
                {
                    UpdateStats(true, JsonString.Len(), StartTime);
                    UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                        FString::Printf(TEXT("CoreConfig saved successfully: %s (compressed)"), *Filename));
                    
                    // Cria backup se habilitado
                    if (bBackupEnabled)
                    {
                        CreateBackup(FullPath);
                    }
                    
                    // Broadcast evento
                    UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                        EPlanetEventType::ConfigSaved, TEXT("CoreConfig"), Filename);
                    
                    return true;
                }
            }
        }
        else
        {
            if (FFileHelper::SaveStringToFile(JsonString, *FullPath))
            {
                UpdateStats(true, JsonString.Len(), StartTime);
                UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                    FString::Printf(TEXT("CoreConfig saved successfully: %s"), *Filename));
                
                if (bBackupEnabled)
                {
                    CreateBackup(FullPath);
                }
                
                UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                    EPlanetEventType::ConfigSaved, TEXT("CoreConfig"), Filename);
                
                return true;
            }
        }
        
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Failed to save CoreConfig: %s"), *Filename));
        return false;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception saving CoreConfig: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

UPlanetCoreConfig* UPlanetDataSerializer::LoadCoreConfig(const FString& Filename)
{
    const float StartTime = FPlatformTime::Seconds();
    
    try
    {
        FString FullPath = GetFullPath(TEXT("Configs"), Filename);
        
        if (!FPaths::FileExists(FullPath))
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetDataSerializer"), 
                FString::Printf(TEXT("CoreConfig file not found: %s"), *Filename));
            return nullptr;
        }
        
        FString JsonString;
        
        // Tenta carregar como arquivo comprimido primeiro
        if (bCompressionEnabled)
        {
            TArray<uint8> CompressedData;
            if (FFileHelper::LoadFileToArray(CompressedData, *FullPath))
            {
                TArray<uint8> DecompressedData;
                if (FCompression::UncompressMemory(NAME_Zlib, DecompressedData, CompressedData.GetData(), CompressedData.Num()))
                {
                    JsonString = FString(UTF8_TO_TCHAR(DecompressedData.GetData()), DecompressedData.Num());
                }
            }
        }
        
        // Se não conseguiu descomprimir, tenta como texto normal
        if (JsonString.IsEmpty())
        {
            if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
            {
                UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
                    FString::Printf(TEXT("Failed to load CoreConfig file: %s"), *Filename));
                return nullptr;
            }
        }
        
        // Deserializa do JSON
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
        UPlanetCoreConfig* Config = NewObject<UPlanetCoreConfig>();
        
        if (FJsonSerializer::Deserialize(Reader, Config))
        {
            UpdateStats(false, JsonString.Len(), StartTime);
            UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                FString::Printf(TEXT("CoreConfig loaded successfully: %s"), *Filename));
            
            UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                EPlanetEventType::ConfigLoaded, TEXT("CoreConfig"), Filename);
            
            return Config;
        }
        
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Failed to deserialize CoreConfig: %s"), *Filename));
        return nullptr;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception loading CoreConfig: %s"), UTF8_TO_TCHAR(e.what())));
        return nullptr;
    }
}

bool UPlanetDataSerializer::SaveChunk(const FPlanetChunk& Chunk, const FString& Filename)
{
    const float StartTime = FPlatformTime::Seconds();
    
    try
    {
        FString FullPath = GetFullPath(TEXT("Chunks"), Filename);
        
        // Serializa o chunk para array de bytes
        TArray<uint8> ChunkData;
        FMemoryWriter Writer(ChunkData);
        Writer << Chunk;
        
        // Aplica compressão se habilitada
        if (bCompressionEnabled)
        {
            TArray<uint8> CompressedData;
            if (FCompression::CompressMemory(NAME_Zlib, CompressedData, ChunkData.GetData(), ChunkData.Num()))
            {
                if (FFileHelper::SaveArrayToFile(CompressedData, *FullPath))
                {
                    UpdateStats(true, ChunkData.Num(), StartTime);
                    UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                        FString::Printf(TEXT("Chunk saved successfully: %s (compressed)"), *Filename));
                    
                    if (bBackupEnabled)
                    {
                        CreateBackup(FullPath);
                    }
                    
                    UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                        EPlanetEventType::ChunkSaved, TEXT("Chunk"), Filename, 0.0f, ChunkData.Num());
                    
                    return true;
                }
            }
        }
        else
        {
            if (FFileHelper::SaveArrayToFile(ChunkData, *FullPath))
            {
                UpdateStats(true, ChunkData.Num(), StartTime);
                UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                    FString::Printf(TEXT("Chunk saved successfully: %s"), *Filename));
                
                if (bBackupEnabled)
                {
                    CreateBackup(FullPath);
                }
                
                UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                    EPlanetEventType::ChunkSaved, TEXT("Chunk"), Filename, 0.0f, ChunkData.Num());
                
                return true;
            }
        }
        
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Failed to save chunk: %s"), *Filename));
        return false;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception saving chunk: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

FPlanetChunk* UPlanetDataSerializer::LoadChunk(const FString& Filename)
{
    const float StartTime = FPlatformTime::Seconds();
    
    try
    {
        FString FullPath = GetFullPath(TEXT("Chunks"), Filename);
        
        if (!FPaths::FileExists(FullPath))
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetDataSerializer"), 
                FString::Printf(TEXT("Chunk file not found: %s"), *Filename));
            return nullptr;
        }
        
        TArray<uint8> ChunkData;
        
        // Tenta carregar como arquivo comprimido primeiro
        if (bCompressionEnabled)
        {
            TArray<uint8> CompressedData;
            if (FFileHelper::LoadFileToArray(CompressedData, *FullPath))
            {
                if (!FCompression::UncompressMemory(NAME_Zlib, ChunkData, CompressedData.GetData(), CompressedData.Num()))
                {
                    UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
                        FString::Printf(TEXT("Failed to decompress chunk: %s"), *Filename));
                    return nullptr;
                }
            }
        }
        
        // Se não conseguiu descomprimir, tenta como dados normais
        if (ChunkData.Num() == 0)
        {
            if (!FFileHelper::LoadFileToArray(ChunkData, *FullPath))
            {
                UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
                    FString::Printf(TEXT("Failed to load chunk file: %s"), *Filename));
                return nullptr;
            }
        }
        
        // Deserializa o chunk
        FMemoryReader Reader(ChunkData);
        FPlanetChunk* Chunk = new FPlanetChunk();
        Reader << *Chunk;
        
        UpdateStats(false, ChunkData.Num(), StartTime);
        UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Chunk loaded successfully: %s"), *Filename));
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::ChunkLoaded, TEXT("Chunk"), Filename, 0.0f, ChunkData.Num());
        
        return Chunk;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception loading chunk: %s"), UTF8_TO_TCHAR(e.what())));
        return nullptr;
    }
}

bool UPlanetDataSerializer::SaveEvents(const TArray<FPlanetSystemEvent>& Events, const FString& Filename)
{
    const float StartTime = FPlatformTime::Seconds();
    
    try
    {
        FString FullPath = GetFullPath(TEXT("Events"), Filename);
        FString JsonString;
        
        // Serializa eventos para JSON
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
        Writer->WriteArrayStart();
        
        for (const FPlanetSystemEvent& Event : Events)
        {
            Writer->WriteObjectStart();
            Writer->WriteValue(TEXT("EventType"), (int32)Event.EventType);
            Writer->WriteValue(TEXT("CustomName"), Event.CustomName);
            Writer->WriteValue(TEXT("Timestamp"), Event.Timestamp);
            Writer->WriteValue(TEXT("Priority"), Event.Priority);
            Writer->WriteValue(TEXT("StringParam"), Event.StringParam);
            Writer->WriteValue(TEXT("FloatParam"), Event.FloatParam);
            Writer->WriteValue(TEXT("IntParam"), Event.IntParam);
            Writer->WriteValue(TEXT("SourceModule"), Event.SourceModule);
            Writer->WriteObjectEnd();
        }
        
        Writer->WriteArrayEnd();
        Writer->Close();
        
        // Aplica compressão se habilitada
        if (bCompressionEnabled)
        {
            TArray<uint8> CompressedData;
            if (FCompression::CompressMemory(NAME_Zlib, CompressedData, (uint8*)TCHAR_TO_UTF8(*JsonString), JsonString.Len()))
            {
                if (FFileHelper::SaveArrayToFile(CompressedData, *FullPath))
                {
                    UpdateStats(true, JsonString.Len(), StartTime);
                    UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                        FString::Printf(TEXT("Events saved successfully: %s (compressed, %d events)"), *Filename, Events.Num()));
                    
                    if (bBackupEnabled)
                    {
                        CreateBackup(FullPath);
                    }
                    
                    UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                        EPlanetEventType::EventsSaved, TEXT("Events"), Filename, 0.0f, Events.Num());
                    
                    return true;
                }
            }
        }
        else
        {
            if (FFileHelper::SaveStringToFile(JsonString, *FullPath))
            {
                UpdateStats(true, JsonString.Len(), StartTime);
                UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                    FString::Printf(TEXT("Events saved successfully: %s (%d events)"), *Filename, Events.Num()));
                
                if (bBackupEnabled)
                {
                    CreateBackup(FullPath);
                }
                
                UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                    EPlanetEventType::EventsSaved, TEXT("Events"), Filename, 0.0f, Events.Num());
                
                return true;
            }
        }
        
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Failed to save events: %s"), *Filename));
        return false;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception saving events: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

TArray<FPlanetSystemEvent> UPlanetDataSerializer::LoadEvents(const FString& Filename)
{
    const float StartTime = FPlatformTime::Seconds();
    
    try
    {
        FString FullPath = GetFullPath(TEXT("Events"), Filename);
        
        if (!FPaths::FileExists(FullPath))
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetDataSerializer"), 
                FString::Printf(TEXT("Events file not found: %s"), *Filename));
            return TArray<FPlanetSystemEvent>();
        }
        
        FString JsonString;
        
        // Tenta carregar como arquivo comprimido primeiro
        if (bCompressionEnabled)
        {
            TArray<uint8> CompressedData;
            if (FFileHelper::LoadFileToArray(CompressedData, *FullPath))
            {
                TArray<uint8> DecompressedData;
                if (FCompression::UncompressMemory(NAME_Zlib, DecompressedData, CompressedData.GetData(), CompressedData.Num()))
                {
                    JsonString = FString(UTF8_TO_TCHAR(DecompressedData.GetData()), DecompressedData.Num());
                }
            }
        }
        
        // Se não conseguiu descomprimir, tenta como texto normal
        if (JsonString.IsEmpty())
        {
            if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
            {
                UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
                    FString::Printf(TEXT("Failed to load events file: %s"), *Filename));
                return TArray<FPlanetSystemEvent>();
            }
        }
        
        // Deserializa do JSON
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
        TArray<FPlanetSystemEvent> Events;
        
        TSharedPtr<FJsonValue> JsonValue;
        if (FJsonSerializer::Deserialize(Reader, JsonValue) && JsonValue.IsValid())
        {
            const TArray<TSharedPtr<FJsonValue>>& JsonArray = JsonValue->AsArray();
            
            for (const TSharedPtr<FJsonValue>& EventValue : JsonArray)
            {
                const TSharedPtr<FJsonObject>& EventObj = EventValue->AsObject();
                FPlanetSystemEvent Event;
                
                Event.EventType = (EPlanetEventType)EventObj->GetIntegerField(TEXT("EventType"));
                Event.CustomName = EventObj->GetStringField(TEXT("CustomName"));
                Event.Timestamp = EventObj->GetNumberField(TEXT("Timestamp"));
                Event.Priority = EventObj->GetNumberField(TEXT("Priority"));
                Event.StringParam = EventObj->GetStringField(TEXT("StringParam"));
                Event.FloatParam = EventObj->GetNumberField(TEXT("FloatParam"));
                Event.IntParam = EventObj->GetIntegerField(TEXT("IntParam"));
                Event.SourceModule = EventObj->GetStringField(TEXT("SourceModule"));
                
                Events.Add(Event);
            }
        }
        
        UpdateStats(false, JsonString.Len(), StartTime);
        UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Events loaded successfully: %s (%d events)"), *Filename, Events.Num()));
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::EventsLoaded, TEXT("Events"), Filename, 0.0f, Events.Num());
        
        return Events;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception loading events: %s"), UTF8_TO_TCHAR(e.what())));
        return TArray<FPlanetSystemEvent>();
    }
}

bool UPlanetDataSerializer::SaveJsonData(const TSharedPtr<FJsonObject>& JsonData, const FString& Filename)
{
    const float StartTime = FPlatformTime::Seconds();
    
    try
    {
        FString FullPath = GetFullPath(TEXT("JsonData"), Filename);
        FString JsonString;
        
        // Serializa para JSON
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(JsonData.ToSharedRef(), Writer);
        Writer->Close();
        
        // Aplica compressão se habilitada
        if (bCompressionEnabled)
        {
            TArray<uint8> CompressedData;
            if (FCompression::CompressMemory(NAME_Zlib, CompressedData, (uint8*)TCHAR_TO_UTF8(*JsonString), JsonString.Len()))
            {
                if (FFileHelper::SaveArrayToFile(CompressedData, *FullPath))
                {
                    UpdateStats(true, JsonString.Len(), StartTime);
                    UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                        FString::Printf(TEXT("JSON data saved successfully: %s (compressed)"), *Filename));
                    
                    if (bBackupEnabled)
                    {
                        CreateBackup(FullPath);
                    }
                    
                    return true;
                }
            }
        }
        else
        {
            if (FFileHelper::SaveStringToFile(JsonString, *FullPath))
            {
                UpdateStats(true, JsonString.Len(), StartTime);
                UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                    FString::Printf(TEXT("JSON data saved successfully: %s"), *Filename));
                
                if (bBackupEnabled)
                {
                    CreateBackup(FullPath);
                }
                
                return true;
            }
        }
        
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Failed to save JSON data: %s"), *Filename));
        return false;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception saving JSON data: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

TSharedPtr<FJsonObject> UPlanetDataSerializer::LoadJsonData(const FString& Filename)
{
    const float StartTime = FPlatformTime::Seconds();
    
    try
    {
        FString FullPath = GetFullPath(TEXT("JsonData"), Filename);
        
        if (!FPaths::FileExists(FullPath))
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetDataSerializer"), 
                FString::Printf(TEXT("JSON data file not found: %s"), *Filename));
            return nullptr;
        }
        
        FString JsonString;
        
        // Tenta carregar como arquivo comprimido primeiro
        if (bCompressionEnabled)
        {
            TArray<uint8> CompressedData;
            if (FFileHelper::LoadFileToArray(CompressedData, *FullPath))
            {
                TArray<uint8> DecompressedData;
                if (FCompression::UncompressMemory(NAME_Zlib, DecompressedData, CompressedData.GetData(), CompressedData.Num()))
                {
                    JsonString = FString(UTF8_TO_TCHAR(DecompressedData.GetData()), DecompressedData.Num());
                }
            }
        }
        
        // Se não conseguiu descomprimir, tenta como texto normal
        if (JsonString.IsEmpty())
        {
            if (!FFileHelper::LoadFileToString(JsonString, *FullPath))
            {
                UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
                    FString::Printf(TEXT("Failed to load JSON data file: %s"), *Filename));
                return nullptr;
            }
        }
        
        // Deserializa do JSON
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
        TSharedPtr<FJsonObject> JsonData = MakeShared<FJsonObject>();
        
        if (FJsonSerializer::Deserialize(Reader, JsonData))
        {
            UpdateStats(false, JsonString.Len(), StartTime);
            UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                FString::Printf(TEXT("JSON data loaded successfully: %s"), *Filename));
            
            return JsonData;
        }
        
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Failed to deserialize JSON data: %s"), *Filename));
        return nullptr;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception loading JSON data: %s"), UTF8_TO_TCHAR(e.what())));
        return nullptr;
    }
}

bool UPlanetDataSerializer::SaveBinaryData(const TArray<uint8>& Data, const FString& Filename)
{
    const float StartTime = FPlatformTime::Seconds();
    
    try
    {
        FString FullPath = GetFullPath(TEXT("BinaryData"), Filename);
        
        // Aplica compressão se habilitada
        if (bCompressionEnabled)
        {
            TArray<uint8> CompressedData;
            if (FCompression::CompressMemory(NAME_Zlib, CompressedData, Data.GetData(), Data.Num()))
            {
                if (FFileHelper::SaveArrayToFile(CompressedData, *FullPath))
                {
                    UpdateStats(true, Data.Num(), StartTime);
                    UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                        FString::Printf(TEXT("Binary data saved successfully: %s (compressed)"), *Filename));
                    
                    if (bBackupEnabled)
                    {
                        CreateBackup(FullPath);
                    }
                    
                    return true;
                }
            }
        }
        else
        {
            if (FFileHelper::SaveArrayToFile(Data, *FullPath))
            {
                UpdateStats(true, Data.Num(), StartTime);
                UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                    FString::Printf(TEXT("Binary data saved successfully: %s"), *Filename));
                
                if (bBackupEnabled)
                {
                    CreateBackup(FullPath);
                }
                
                return true;
            }
        }
        
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Failed to save binary data: %s"), *Filename));
        return false;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception saving binary data: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

TArray<uint8> UPlanetDataSerializer::LoadBinaryData(const FString& Filename)
{
    const float StartTime = FPlatformTime::Seconds();
    
    try
    {
        FString FullPath = GetFullPath(TEXT("BinaryData"), Filename);
        
        if (!FPaths::FileExists(FullPath))
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetDataSerializer"), 
                FString::Printf(TEXT("Binary data file not found: %s"), *Filename));
            return TArray<uint8>();
        }
        
        TArray<uint8> Data;
        
        // Tenta carregar como arquivo comprimido primeiro
        if (bCompressionEnabled)
        {
            TArray<uint8> CompressedData;
            if (FFileHelper::LoadFileToArray(CompressedData, *FullPath))
            {
                if (!FCompression::UncompressMemory(NAME_Zlib, Data, CompressedData.GetData(), CompressedData.Num()))
                {
                    UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
                        FString::Printf(TEXT("Failed to decompress binary data: %s"), *Filename));
                    return TArray<uint8>();
                }
            }
        }
        
        // Se não conseguiu descomprimir, tenta como dados normais
        if (Data.Num() == 0)
        {
            if (!FFileHelper::LoadFileToArray(Data, *FullPath))
            {
                UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
                    FString::Printf(TEXT("Failed to load binary data file: %s"), *Filename));
                return TArray<uint8>();
            }
        }
        
        UpdateStats(false, Data.Num(), StartTime);
        UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Binary data loaded successfully: %s"), *Filename));
        
        return Data;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception loading binary data: %s"), UTF8_TO_TCHAR(e.what())));
        return TArray<uint8>();
    }
}

bool UPlanetDataSerializer::FileExists(const FString& Subdirectory, const FString& Filename)
{
    FString FullPath = GetFullPath(Subdirectory, Filename);
    bool bExists = FPaths::FileExists(FullPath);
    
    UPlanetSystemLogger::LogDebug(TEXT("PlanetDataSerializer"), 
        FString::Printf(TEXT("File exists check: %s -> %s"), *Filename, bExists ? TEXT("true") : TEXT("false")));
    
    return bExists;
}

bool UPlanetDataSerializer::DeleteFile(const FString& Subdirectory, const FString& Filename)
{
    FString FullPath = GetFullPath(Subdirectory, Filename);
    
    if (!FPaths::FileExists(FullPath))
    {
        UPlanetSystemLogger::LogWarning(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Cannot delete non-existent file: %s"), *Filename));
        return false;
    }
    
    if (IFileManager::Get().Delete(*FullPath))
    {
        UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("File deleted successfully: %s"), *Filename));
        return true;
    }
    
    UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
        FString::Printf(TEXT("Failed to delete file: %s"), *Filename));
    return false;
}

int64 UPlanetDataSerializer::GetFileSize(const FString& Subdirectory, const FString& Filename)
{
    FString FullPath = GetFullPath(Subdirectory, Filename);
    
    if (!FPaths::FileExists(FullPath))
    {
        UPlanetSystemLogger::LogWarning(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Cannot get size of non-existent file: %s"), *Filename));
        return -1;
    }
    
    int64 FileSize = IFileManager::Get().FileSize(*FullPath);
    
    UPlanetSystemLogger::LogDebug(TEXT("PlanetDataSerializer"), 
        FString::Printf(TEXT("File size: %s -> %lld bytes"), *Filename, FileSize));
    
    return FileSize;
}

FDateTime UPlanetDataSerializer::GetFileModificationTime(const FString& Subdirectory, const FString& Filename)
{
    FString FullPath = GetFullPath(Subdirectory, Filename);
    
    if (!FPaths::FileExists(FullPath))
    {
        UPlanetSystemLogger::LogWarning(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Cannot get modification time of non-existent file: %s"), *Filename));
        return FDateTime::MinValue();
    }
    
    FDateTime ModTime = IFileManager::Get().GetTimeStamp(*FullPath);
    
    UPlanetSystemLogger::LogDebug(TEXT("PlanetDataSerializer"), 
        FString::Printf(TEXT("File modification time: %s -> %s"), *Filename, *ModTime.ToString()));
    
    return ModTime;
}

TArray<FString> UPlanetDataSerializer::ListFiles(const FString& Subdirectory, const FString& Extension)
{
    FString FullPath = GetFullPath(Subdirectory, TEXT(""));
    TArray<FString> Files;
    
    IFileManager::Get().FindFiles(Files, *FullPath, *Extension);
    
    UPlanetSystemLogger::LogDebug(TEXT("PlanetDataSerializer"), 
        FString::Printf(TEXT("Listed %d files in %s with extension %s"), Files.Num(), *Subdirectory, *Extension));
    
    return Files;
}

bool UPlanetDataSerializer::CreateBackup(const FString& OriginalPath)
{
    try
    {
        FString BackupPath = OriginalPath + TEXT(".backup_") + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
        
        if (IFileManager::Get().Copy(*BackupPath, *OriginalPath))
        {
            UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                FString::Printf(TEXT("Backup created: %s"), *BackupPath));
            
            // Remove backups antigos se exceder o limite
            CleanupOldBackups(FPaths::GetPath(OriginalPath));
            
            return true;
        }
        
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Failed to create backup: %s"), *OriginalPath));
        return false;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception creating backup: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

bool UPlanetDataSerializer::RestoreBackup(const FString& OriginalPath)
{
    try
    {
        FString BackupPath = OriginalPath + TEXT(".backup_") + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
        
        if (FPaths::FileExists(BackupPath))
        {
            if (IFileManager::Get().Copy(*OriginalPath, *BackupPath))
            {
                UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                    FString::Printf(TEXT("Backup restored: %s"), *BackupPath));
                return true;
            }
        }
        
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Failed to restore backup: %s"), *OriginalPath));
        return false;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception restoring backup: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

void UPlanetDataSerializer::GetSerializerStats(FString& OutStats) const
{
    OutStats = FString::Printf(
        TEXT("PlanetDataSerializer Statistics:\n")
        TEXT("- Total Files Saved: %d\n")
        TEXT("- Total Files Loaded: %d\n")
        TEXT("- Total Bytes Saved: %lld\n")
        TEXT("- Total Bytes Loaded: %lld\n")
        TEXT("- Last Operation Time: %.3f seconds\n")
        TEXT("- Compression Enabled: %s\n")
        TEXT("- Backup Enabled: %s\n")
        TEXT("- Max Backup Files: %d\n")
        TEXT("- Base Directory: %s"),
        TotalFilesSaved, TotalFilesLoaded, TotalBytesSaved, TotalBytesLoaded,
        LastOperationTime, bCompressionEnabled ? TEXT("Yes") : TEXT("No"),
        bBackupEnabled ? TEXT("Yes") : TEXT("No"), MaxBackupFiles, *BaseDirectory
    );
    
    UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), TEXT("Serializer statistics retrieved"));
}

void UPlanetDataSerializer::ClearAllData()
{
    try
    {
        FString BaseDir = FPaths::ProjectSavedDir() / BaseDirectory;
        
        if (IFileManager::Get().DeleteDirectory(*BaseDir, false, true))
        {
            UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
                FString::Printf(TEXT("All data cleared from: %s"), *BaseDir));
            
            // Reset statistics
            TotalFilesSaved = 0;
            TotalFilesLoaded = 0;
            TotalBytesSaved = 0;
            TotalBytesLoaded = 0;
            LastOperationTime = 0.0f;
            
            UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                EPlanetEventType::DataCleared, TEXT("AllData"));
        }
        else
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
                FString::Printf(TEXT("Failed to clear data from: %s"), *BaseDir));
        }
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception clearing data: %s"), UTF8_TO_TCHAR(e.what())));
    }
}

bool UPlanetDataSerializer::ValidateFileIntegrity(const FString& Subdirectory, const FString& Filename)
{
    FString FullPath = GetFullPath(Subdirectory, Filename);
    
    if (!FPaths::FileExists(FullPath))
    {
        UPlanetSystemLogger::LogWarning(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Cannot validate non-existent file: %s"), *Filename));
        return false;
    }
    
    try
    {
        // Verifica tamanho do arquivo
        int64 FileSize = IFileManager::Get().FileSize(*FullPath);
        if (FileSize <= 0)
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
                FString::Printf(TEXT("File integrity check failed - invalid size: %s"), *Filename));
            return false;
        }
        
        // Tenta ler o arquivo para verificar se está corrompido
        TArray<uint8> TestData;
        if (!FFileHelper::LoadFileToArray(TestData, *FullPath))
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
                FString::Printf(TEXT("File integrity check failed - cannot read: %s"), *Filename));
            return false;
        }
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("File integrity check passed: %s"), *Filename));
        return true;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception during file integrity check: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

void UPlanetDataSerializer::CompactData()
{
    try
    {
        FString BaseDir = FPaths::ProjectSavedDir() / BaseDirectory;
        
        // Lista todos os arquivos
        TArray<FString> AllFiles;
        IFileManager::Get().FindFilesRecursive(AllFiles, *BaseDir, TEXT("*.*"), true, false);
        
        int32 CompactedFiles = 0;
        int64 TotalSpaceSaved = 0;
        
        for (const FString& FilePath : AllFiles)
        {
            // Verifica se é um arquivo de backup
            if (FilePath.Contains(TEXT(".backup_")))
            {
                int64 FileSize = IFileManager::Get().FileSize(*FilePath);
                if (IFileManager::Get().Delete(*FilePath))
                {
                    TotalSpaceSaved += FileSize;
                    CompactedFiles++;
                }
            }
        }
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Data compaction completed: %d files removed, %lld bytes saved"), 
                CompactedFiles, TotalSpaceSaved));
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::DataCompacted, TEXT("Compaction"), TEXT(""), 0.0f, CompactedFiles);
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception during data compaction: %s"), UTF8_TO_TCHAR(e.what())));
    }
}

// Funções privadas auxiliares

FString UPlanetDataSerializer::GetFullPath(const FString& Subdirectory, const FString& Filename)
{
    FString FullPath = FPaths::ProjectSavedDir() / BaseDirectory / Subdirectory;
    
    // Cria o diretório se não existir
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*FullPath);
    
    if (!Filename.IsEmpty())
    {
        FullPath = FullPath / Filename;
    }
    
    return FullPath;
}

void UPlanetDataSerializer::UpdateStats(bool bIsSave, int64 Bytes, float StartTime)
{
    if (bIsSave)
    {
        TotalFilesSaved++;
        TotalBytesSaved += Bytes;
    }
    else
    {
        TotalFilesLoaded++;
        TotalBytesLoaded += Bytes;
    }
    
    LastOperationTime = FPlatformTime::Seconds() - StartTime;
}

void UPlanetDataSerializer::CleanupOldBackups(const FString& Directory)
{
    try
    {
        TArray<FString> BackupFiles;
        IFileManager::Get().FindFiles(BackupFiles, *Directory, TEXT("*.backup_*"));
        
        // Ordena por data de modificação (mais antigos primeiro)
        BackupFiles.Sort([&](const FString& A, const FString& B) {
            FDateTime TimeA = IFileManager::Get().GetTimeStamp(*(Directory / A));
            FDateTime TimeB = IFileManager::Get().GetTimeStamp(*(Directory / B));
            return TimeA < TimeB;
        });
        
        // Remove backups antigos se exceder o limite
        int32 FilesToRemove = BackupFiles.Num() - MaxBackupFiles;
        for (int32 i = 0; i < FilesToRemove && i < BackupFiles.Num(); ++i)
        {
            IFileManager::Get().Delete(*(Directory / BackupFiles[i]));
            UPlanetSystemLogger::LogDebug(TEXT("PlanetDataSerializer"), 
                FString::Printf(TEXT("Removed old backup: %s"), *BackupFiles[i]));
        }
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetDataSerializer"), 
            FString::Printf(TEXT("Exception cleaning up old backups: %s"), UTF8_TO_TCHAR(e.what())));
    }
} 