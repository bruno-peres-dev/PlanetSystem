#include "PlanetSystemLogger.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

UPlanetSystemLogger* UPlanetSystemLogger::Instance = nullptr;

UPlanetSystemLogger::UPlanetSystemLogger()
{
    StartTime = FDateTime::Now();
    
    // Habilita todas as categorias por padrão
    for (int32 i = 0; i < static_cast<int32>(EPlanetLogCategory::Serialization) + 1; ++i)
    {
        EnabledCategories.Add(static_cast<EPlanetLogCategory>(i));
    }
    
    // Inicializa contadores
    for (int32 i = 0; i < static_cast<int32>(EPlanetLogLevel::Critical) + 1; ++i)
    {
        LogCounts.Add(static_cast<EPlanetLogLevel>(i), 0);
    }
}

UPlanetSystemLogger* UPlanetSystemLogger::GetInstance()
{
    if (!Instance)
    {
        Instance = NewObject<UPlanetSystemLogger>();
        Instance->AddToRoot(); // Previne garbage collection
    }
    return Instance;
}

void UPlanetSystemLogger::Initialize(const FString& InLogFilePath)
{
    if (InLogFilePath.IsEmpty())
    {
        LogFilePath = FPaths::ProjectLogDir() + TEXT("PlanetSystem_") + 
                     FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + TEXT(".log");
    }
    else
    {
        LogFilePath = InLogFilePath;
    }
    
    // Cria o diretório se não existir
    FString Directory = FPaths::GetPath(LogFilePath);
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*Directory))
    {
        PlatformFile.CreateDirectoryTree(*Directory);
    }
    
    // Escreve cabeçalho do log
    FString Header = FString::Printf(TEXT("=== PlanetSystem Log Started at %s ===\n"), 
                                    *FDateTime::Now().ToString());
    WriteToFile(Header);
    
    Log(TEXT("Logging system initialized"), EPlanetLogLevel::Info, EPlanetLogCategory::General);
}

void UPlanetSystemLogger::Shutdown()
{
    if (bEnableLogging)
    {
        Log(TEXT("Logging system shutting down"), EPlanetLogLevel::Info, EPlanetLogCategory::General);
        
        // Escreve estatísticas finais
        FString Stats;
        for (const auto& Pair : LogCounts)
        {
            if (Pair.Value > 0)
            {
                Stats += FString::Printf(TEXT("%s: %d, "), 
                                        *GetLogLevelName(Pair.Key), 
                                        Pair.Value);
            }
        }
        
        if (!Stats.IsEmpty())
        {
            Stats.RemoveFromEnd(TEXT(", "));
            Log(TEXT("Final statistics: ") + Stats, EPlanetLogLevel::Info, EPlanetLogCategory::General);
        }
        
        FString Footer = FString::Printf(TEXT("=== PlanetSystem Log Ended at %s ===\n"), 
                                        *FDateTime::Now().ToString());
        WriteToFile(Footer);
    }
    
    if (Instance)
    {
        Instance->RemoveFromRoot();
        Instance = nullptr;
    }
}

void UPlanetSystemLogger::SetMinLogLevel(EPlanetLogLevel Level)
{
    MinLogLevel = Level;
    Log(FString::Printf(TEXT("Minimum log level set to: %s"), *GetLogLevelName(Level)), 
        EPlanetLogLevel::Info, EPlanetLogCategory::General);
}

void UPlanetSystemLogger::SetCategoryEnabled(EPlanetLogCategory Category, bool bEnabled)
{
    if (bEnabled)
    {
        EnabledCategories.Add(Category);
    }
    else
    {
        EnabledCategories.Remove(Category);
    }
    
    Log(FString::Printf(TEXT("Category %s %s"), 
                        *GetLogCategoryName(Category), 
                        bEnabled ? TEXT("enabled") : TEXT("disabled")), 
        EPlanetLogLevel::Info, EPlanetLogCategory::General);
}

void UPlanetSystemLogger::SetLoggingEnabled(bool bEnabled)
{
    bEnableLogging = bEnabled;
    Log(FString::Printf(TEXT("Logging %s"), bEnabled ? TEXT("enabled") : TEXT("disabled")), 
        EPlanetLogLevel::Info, EPlanetLogCategory::General);
}

void UPlanetSystemLogger::Log(const FString& Message, EPlanetLogLevel Level, EPlanetLogCategory Category)
{
    if (Instance)
    {
        Instance->WriteLog(Message, Level, Category);
    }
}

void UPlanetSystemLogger::LogPerformance(const FString& Operation, double Duration, EPlanetLogCategory Category)
{
    FString Message = FString::Printf(TEXT("%s took %.3f ms"), *Operation, Duration * 1000.0);
    Log(Message, EPlanetLogLevel::Info, Category);
}

void UPlanetSystemLogger::LogMemory(const FString& Operation, int64 BytesUsed, EPlanetLogCategory Category)
{
    FString Message = FString::Printf(TEXT("%s used %lld bytes (%.2f MB)"), 
                                     *Operation, BytesUsed, BytesUsed / (1024.0 * 1024.0));
    Log(Message, EPlanetLogLevel::Info, Category);
}

void UPlanetSystemLogger::LogError(const FString& Message, const FString& Context, EPlanetLogCategory Category)
{
    FString FullMessage = Context.IsEmpty() ? Message : FString::Printf(TEXT("%s (Context: %s)"), *Message, *Context);
    Log(FullMessage, EPlanetLogLevel::Error, Category);
}

void UPlanetSystemLogger::LogWarning(const FString& Message, const FString& Context, EPlanetLogCategory Category)
{
    FString FullMessage = Context.IsEmpty() ? Message : FString::Printf(TEXT("%s (Context: %s)"), *Message, *Context);
    Log(FullMessage, EPlanetLogLevel::Warning, Category);
}

void UPlanetSystemLogger::LogDebug(const FString& Message, const FString& Context, EPlanetLogCategory Category)
{
    FString FullMessage = Context.IsEmpty() ? Message : FString::Printf(TEXT("%s (Context: %s)"), *Message, *Context);
    Log(FullMessage, EPlanetLogLevel::Debug, Category);
}

void UPlanetSystemLogger::LogInfo(const FString& Message, const FString& Context, EPlanetLogCategory Category)
{
    FString FullMessage = Context.IsEmpty() ? Message : FString::Printf(TEXT("%s (Context: %s)"), *Message, *Context);
    Log(FullMessage, EPlanetLogLevel::Info, Category);
}

void UPlanetSystemLogger::LogVerbose(const FString& Message, const FString& Context, EPlanetLogCategory Category)
{
    FString FullMessage = Context.IsEmpty() ? Message : FString::Printf(TEXT("%s (Context: %s)"), *Message, *Context);
    Log(FullMessage, EPlanetLogLevel::Verbose, Category);
}

void UPlanetSystemLogger::LogCritical(const FString& Message, const FString& Context, EPlanetLogCategory Category)
{
    FString FullMessage = Context.IsEmpty() ? Message : FString::Printf(TEXT("%s (Context: %s)"), *Message, *Context);
    Log(FullMessage, EPlanetLogLevel::Critical, Category);
}

void UPlanetSystemLogger::LogEvent(const FString& EventName, const FString& Details, EPlanetLogCategory Category)
{
    FString Message = Details.IsEmpty() ? EventName : FString::Printf(TEXT("%s: %s"), *EventName, *Details);
    Log(Message, EPlanetLogLevel::Info, Category);
}

void UPlanetSystemLogger::GetLogStatistics(TMap<EPlanetLogLevel, int32>& OutLogCounts, FString& OutLogFilePath)
{
    OutLogCounts = LogCounts;
    OutLogFilePath = LogFilePath;
}

bool UPlanetSystemLogger::ExportLogs(const FString& FilePath)
{
    if (LogBuffer.Num() == 0)
    {
        return false;
    }
    
    FString ExportContent;
    for (const FString& LogEntry : LogBuffer)
    {
        ExportContent += LogEntry + TEXT("\n");
    }
    
    return FFileHelper::SaveStringToFile(ExportContent, *FilePath);
}

void UPlanetSystemLogger::ClearLogBuffer()
{
    LogBuffer.Empty();
    Log(TEXT("Log buffer cleared"), EPlanetLogLevel::Info, EPlanetLogCategory::General);
}

void UPlanetSystemLogger::GetLogBuffer(TArray<FString>& OutLogs, int32 MaxCount)
{
    if (MaxCount <= 0 || MaxCount > LogBuffer.Num())
    {
        OutLogs = LogBuffer;
    }
    else
    {
        OutLogs.Append(LogBuffer.GetData() + (LogBuffer.Num() - MaxCount), MaxCount);
    }
}

bool UPlanetSystemLogger::IsCategoryEnabled(EPlanetLogCategory Category) const
{
    return EnabledCategories.Contains(Category);
}

double UPlanetSystemLogger::GetUptime() const
{
    return (FDateTime::Now() - StartTime).GetTotalSeconds();
}

void UPlanetSystemLogger::WriteLog(const FString& Message, EPlanetLogLevel Level, EPlanetLogCategory Category)
{
    if (!bEnableLogging || !ShouldLog(Level, Category))
    {
        return;
    }
    
    FString FormattedMessage = FormatLogMessage(Message, Level, Category);
    
    // Adiciona ao buffer
    AddToBuffer(FormattedMessage);
    
    // Escreve no arquivo
    WriteToFile(FormattedMessage);
    
    // Atualiza contadores
    LogCounts[Level]++;
    
    // Output para console se for erro ou warning
    if (Level >= EPlanetLogLevel::Warning)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *FormattedMessage);
    }
}

FString UPlanetSystemLogger::FormatLogMessage(const FString& Message, EPlanetLogLevel Level, EPlanetLogCategory Category)
{
    FString Timestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S.%s"));
    FString LevelName = GetLogLevelName(Level);
    FString CategoryName = GetLogCategoryName(Category);
    
    return FString::Printf(TEXT("[%s] [%s] [%s] %s"), 
                          *Timestamp, 
                          *LevelName, 
                          *CategoryName, 
                          *Message);
}

FString UPlanetSystemLogger::GetLogLevelName(EPlanetLogLevel Level) const
{
    static const UEnum* LogLevelEnum = FindObject<UEnum>(nullptr, TEXT("/Script/PlanetSystem.EPlanetLogLevel"));
    if (LogLevelEnum)
    {
        return LogLevelEnum->GetNameStringByValue(static_cast<int64>(Level));
    }
    return TEXT("Unknown");
}

FString UPlanetSystemLogger::GetLogCategoryName(EPlanetLogCategory Category) const
{
    static const UEnum* CategoryEnum = FindObject<UEnum>(nullptr, TEXT("/Script/PlanetSystem.EPlanetLogCategory"));
    if (CategoryEnum)
    {
        return CategoryEnum->GetNameStringByValue(static_cast<int64>(Category));
    }
    return TEXT("Unknown");
}

bool UPlanetSystemLogger::ShouldLog(EPlanetLogLevel Level, EPlanetLogCategory Category) const
{
    return static_cast<int32>(Level) >= static_cast<int32>(MinLogLevel) && 
           EnabledCategories.Contains(Category);
}

void UPlanetSystemLogger::AddToBuffer(const FString& LogEntry)
{
    LogBuffer.Add(LogEntry);
    
    // Remove entradas antigas se exceder o tamanho máximo
    while (LogBuffer.Num() > MaxBufferSize)
    {
        LogBuffer.RemoveAt(0);
    }
}

void UPlanetSystemLogger::WriteToFile(const FString& LogEntry)
{
    if (!LogFilePath.IsEmpty())
    {
        FFileHelper::SaveStringToFile(LogEntry + TEXT("\n"), *LogFilePath, FFileHelper::EEncodingOptions::ForceUTF8, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
    }
} 