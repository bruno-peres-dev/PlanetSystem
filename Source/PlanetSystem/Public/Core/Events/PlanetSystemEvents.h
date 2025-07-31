#pragma once
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "PlanetSystemEvents.generated.h"

/**
 * Tipos de eventos do sistema planetário
 */
UENUM(BlueprintType)
enum class EPlanetEventType : uint8
{
    PlanetGenerated        UMETA(DisplayName = "Planet Generated"),
    ChunkGenerated         UMETA(DisplayName = "Chunk Generated"),
    BiomeCalculated        UMETA(DisplayName = "Biome Calculated"),
    ErosionApplied         UMETA(DisplayName = "Erosion Applied"),
    VegetationSpawned      UMETA(DisplayName = "Vegetation Spawned"),
    WaterSimulated         UMETA(DisplayName = "Water Simulated"),
    LODUpdated             UMETA(DisplayName = "LOD Updated"),
    CacheHit               UMETA(DisplayName = "Cache Hit"),
    CacheMiss              UMETA(DisplayName = "Cache Miss"),
    PerformanceWarning     UMETA(DisplayName = "Performance Warning"),
    ErrorOccurred          UMETA(DisplayName = "Error Occurred"),
    Custom                 UMETA(DisplayName = "Custom Event")
};

/**
 * Estrutura de evento do sistema planetário
 * Segue padrões AAA para comunicação entre módulos
 */
USTRUCT(BlueprintType)
struct PLANETSYSTEM_API FPlanetSystemEvent
{
    GENERATED_BODY()
    
    /** Tipo do evento */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    EPlanetEventType EventType = EPlanetEventType::Custom;
    
    /** Nome customizado do evento (usado quando EventType é Custom) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    FString CustomName;
    
    /** Timestamp do evento */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    FDateTime Timestamp;
    
    /** Prioridade do evento (0 = baixa, 10 = alta) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event", meta=(ClampMin="0", ClampMax="10"))
    int32 Priority = 5;
    
    /** Parâmetros do evento */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    TMap<FString, FString> Parameters;

    /** Parâmetro string rápido */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    FString StringParam = TEXT("");

    /** Parâmetro float rápido */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    float FloatParam = 0.0f;

    /** Parâmetro inteiro rápido */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    int32 IntParam = 0;
    
    /** Dados binários do evento (para dados complexos) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    TArray<uint8> BinaryData;
    
    /** ID único do evento */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    FGuid EventID;
    
    /** Nome do módulo que gerou o evento */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    FString SourceModule;
    
    FPlanetSystemEvent()
    {
        Timestamp = FDateTime::Now();
        EventID = FGuid::NewGuid();
        Priority = 5;
        StringParam = TEXT("");
        FloatParam = 0.0f;
        IntParam = 0;
    }
    
    /**
     * Adiciona um parâmetro string ao evento
     */
    void AddParameter(const FString& Key, const FString& Value)
    {
        Parameters.Add(Key, Value);
    }
    
    /**
     * Adiciona um parâmetro numérico ao evento
     */
    void AddParameter(const FString& Key, float Value)
    {
        Parameters.Add(Key, FString::Printf(TEXT("%f"), Value));
    }
    
    /**
     * Adiciona um parâmetro inteiro ao evento
     */
    void AddParameter(const FString& Key, int32 Value)
    {
        Parameters.Add(Key, FString::Printf(TEXT("%d"), Value));
    }
    
    /**
     * Obtém um parâmetro string do evento
     */
    bool GetParameter(const FString& Key, FString& OutValue) const
    {
        if (const FString* Found = Parameters.Find(Key))
        {
            OutValue = *Found;
            return true;
        }
        return false;
    }
    
    /**
     * Obtém um parâmetro numérico do evento
     */
    bool GetParameter(const FString& Key, float& OutValue) const
    {
        FString StringValue;
        if (GetParameter(Key, StringValue))
        {
            OutValue = FCString::Atof(*StringValue);
            return true;
        }
        return false;
    }
    
    /**
     * Obtém um parâmetro inteiro do evento
     */
    bool GetParameter(const FString& Key, int32& OutValue) const
    {
        FString StringValue;
        if (GetParameter(Key, StringValue))
        {
            OutValue = FCString::Atoi(*StringValue);
            return true;
        }
        return false;
    }
    
    /**
     * Retorna o nome completo do evento
     */
    FString GetEventName() const
    {
        if (EventType == EPlanetEventType::Custom)
        {
            return CustomName;
        }
        
        static const UEnum* EventTypeEnum = FindObject<UEnum>(nullptr, TEXT("/Script/PlanetSystem.EPlanetEventType"));
        if (EventTypeEnum)
        {
            return EventTypeEnum->GetNameStringByValue(static_cast<int64>(EventType));
        }
        
        return TEXT("Unknown");
    }
    
    /**
     * Converte o evento para string para debug
     */
    FString ToString() const
    {
        return FString::Printf(TEXT("[%s] %s (Priority: %d, Source: %s)"), 
            *Timestamp.ToString(), 
            *GetEventName(), 
            Priority, 
            *SourceModule);
    }
}; 