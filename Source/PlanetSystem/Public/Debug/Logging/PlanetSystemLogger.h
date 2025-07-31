#pragma once
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "PlanetSystemLogger.generated.h"

/**
 * Níveis de log do sistema planetário
 */
UENUM(BlueprintType)
enum class EPlanetLogLevel : uint8
{
    Verbose    UMETA(DisplayName = "Verbose"),
    Debug      UMETA(DisplayName = "Debug"),
    Info       UMETA(DisplayName = "Info"),
    Warning    UMETA(DisplayName = "Warning"),
    Error      UMETA(DisplayName = "Error"),
    Critical   UMETA(DisplayName = "Critical")
};

/**
 * Categorias de log do sistema planetário
 */
UENUM(BlueprintType)
enum class EPlanetLogCategory : uint8
{
    General        UMETA(DisplayName = "General"),
    Performance    UMETA(DisplayName = "Performance"),
    Memory         UMETA(DisplayName = "Memory"),
    Generation     UMETA(DisplayName = "Generation"),
    Rendering      UMETA(DisplayName = "Rendering"),
    Cache          UMETA(DisplayName = "Cache"),
    Events         UMETA(DisplayName = "Events"),
    Plugins        UMETA(DisplayName = "Plugins"),
    Validation     UMETA(DisplayName = "Validation"),
    Serialization  UMETA(DisplayName = "Serialization")
};

/**
 * Sistema de logging profissional para o PlanetSystem
 * Segue padrões AAA para debug e monitoramento
 */
UCLASS(Blueprintable, ClassGroup=(Debug))
class PLANETSYSTEM_API UPlanetSystemLogger : public UObject
{
    GENERATED_BODY()
    
private:
    static UPlanetSystemLogger* Instance;
    
    /** Configuração de logging */
    UPROPERTY()
    bool bEnableLogging = true;
    
    /** Nível mínimo de log */
    UPROPERTY()
    EPlanetLogLevel MinLogLevel = EPlanetLogLevel::Info;
    
    /** Categorias habilitadas */
    UPROPERTY()
    TSet<EPlanetLogCategory> EnabledCategories;
    
    /** Arquivo de log */
    UPROPERTY()
    FString LogFilePath;
    
    /** Buffer de logs em memória */
    UPROPERTY()
    TArray<FString> LogBuffer;
    
    /** Tamanho máximo do buffer */
    UPROPERTY()
    int32 MaxBufferSize = 1000;
    
    /** Estatísticas de logging */
    UPROPERTY()
    TMap<EPlanetLogLevel, int32> LogCounts;
    
    /** Timestamp de início */
    UPROPERTY()
    FDateTime StartTime;
    
public:
    UPlanetSystemLogger();
    
    /** Singleton access */
    static UPlanetSystemLogger* GetInstance();
    
    /** Inicializa o sistema de logging */
    UFUNCTION(BlueprintCallable, Category="Logging")
    void Initialize(const FString& InLogFilePath = TEXT(""));
    
    /** Finaliza o sistema de logging */
    UFUNCTION(BlueprintCallable, Category="Logging")
    void Shutdown();
    
    /** Configura o nível mínimo de log */
    UFUNCTION(BlueprintCallable, Category="Logging")
    void SetMinLogLevel(EPlanetLogLevel Level);
    
    /** Habilita/desabilita uma categoria de log */
    UFUNCTION(BlueprintCallable, Category="Logging")
    void SetCategoryEnabled(EPlanetLogCategory Category, bool bEnabled);
    
    /** Habilita/desabilita o logging */
    UFUNCTION(BlueprintCallable, Category="Logging")
    void SetLoggingEnabled(bool bEnabled);
    
    /** Log básico */
    UFUNCTION(BlueprintCallable, Category="Logging")
    static void Log(const FString& Message, EPlanetLogLevel Level = EPlanetLogLevel::Info, 
                   EPlanetLogCategory Category = EPlanetLogCategory::General);
    
    /** Log de performance */
    UFUNCTION(BlueprintCallable, Category="Logging")
    static void LogPerformance(const FString& Operation, double Duration, 
                              EPlanetLogCategory Category = EPlanetLogCategory::Performance);
    
    /** Log de uso de memória */
    UFUNCTION(BlueprintCallable, Category="Logging")
    static void LogMemory(const FString& Operation, int64 BytesUsed, 
                         EPlanetLogCategory Category = EPlanetLogCategory::Memory);
    
    /** Log de erro com contexto */
    UFUNCTION(BlueprintCallable, Category="Logging")
    static void LogError(const FString& Message, const FString& Context = TEXT(""), 
                        EPlanetLogCategory Category = EPlanetLogCategory::General);
    
    /** Log de warning com contexto */
    UFUNCTION(BlueprintCallable, Category="Logging")
    static void LogWarning(const FString& Message, const FString& Context = TEXT(""), 
                          EPlanetLogCategory Category = EPlanetLogCategory::General);
    
    /** Log de debug com contexto */
    UFUNCTION(BlueprintCallable, Category="Logging")
    static void LogDebug(const FString& Message, const FString& Context = TEXT(""), 
                        EPlanetLogCategory Category = EPlanetLogCategory::General);
    
    /** Log de evento do sistema */
    UFUNCTION(BlueprintCallable, Category="Logging")
    static void LogEvent(const FString& EventName, const FString& Details = TEXT(""), 
                        EPlanetLogCategory Category = EPlanetLogCategory::Events);
    
    /** Obtém estatísticas de logging */
    UFUNCTION(BlueprintCallable, Category="Logging")
    void GetLogStatistics(TMap<EPlanetLogLevel, int32>& OutLogCounts, FString& OutLogFilePath);
    
    /** Exporta logs para arquivo */
    UFUNCTION(BlueprintCallable, Category="Logging")
    bool ExportLogs(const FString& FilePath);
    
    /** Limpa o buffer de logs */
    UFUNCTION(BlueprintCallable, Category="Logging")
    void ClearLogBuffer();
    
    /** Obtém logs do buffer */
    UFUNCTION(BlueprintCallable, Category="Logging")
    void GetLogBuffer(TArray<FString>& OutLogs, int32 MaxCount = -1);
    
    /** Verifica se uma categoria está habilitada */
    UFUNCTION(BlueprintCallable, Category="Logging")
    bool IsCategoryEnabled(EPlanetLogCategory Category) const;
    
    /** Verifica se o logging está habilitado */
    UFUNCTION(BlueprintCallable, Category="Logging")
    bool IsLoggingEnabled() const { return bEnableLogging; }
    
    /** Obtém o tempo de execução desde o início */
    UFUNCTION(BlueprintCallable, Category="Logging")
    double GetUptime() const;
    
private:
    /** Escreve uma mensagem de log */
    void WriteLog(const FString& Message, EPlanetLogLevel Level, EPlanetLogCategory Category);
    
    /** Formata uma mensagem de log */
    FString FormatLogMessage(const FString& Message, EPlanetLogLevel Level, EPlanetLogCategory Category);
    
    /** Obtém o nome do nível de log */
    FString GetLogLevelName(EPlanetLogLevel Level) const;
    
    /** Obtém o nome da categoria de log */
    FString GetLogCategoryName(EPlanetLogCategory Category) const;
    
    /** Verifica se deve logar baseado no nível e categoria */
    bool ShouldLog(EPlanetLogLevel Level, EPlanetLogCategory Category) const;
    
    /** Adiciona uma entrada ao buffer */
    void AddToBuffer(const FString& LogEntry);
    
    /** Escreve logs para arquivo */
    void WriteToFile(const FString& LogEntry);
}; 