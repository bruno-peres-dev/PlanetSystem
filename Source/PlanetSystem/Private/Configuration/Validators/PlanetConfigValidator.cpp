#include "Configuration/Validators/PlanetConfigValidator.h"
#include "Debug/Logging/PlanetSystemLogger.h"
#include "Core/Events/PlanetEventBus.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/DateTime.h"

UPlanetConfigValidator::UPlanetConfigValidator()
{
    UPlanetSystemLogger::LogInfo(TEXT("PlanetConfigValidator"), TEXT("Configuration validator created"));
}

bool UPlanetConfigValidator::ValidateConfig(const UPlanetCoreConfig* Config, TArray<FPlanetValidationError>& OutErrors)
{
    try
    {
        if (!Config)
        {
            AddError(OutErrors, EPlanetValidationError::MissingRequired, TEXT("Config"), 
                TEXT("Configuration object is null"), 10);
            return false;
        }
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetConfigValidator"), TEXT("Starting comprehensive configuration validation"));
        
        bool bIsValid = true;
        
        // Valida cada seção da configuração
        TArray<FPlanetValidationError> GenerationErrors;
        if (!ValidateGenerationConfig(Config->GenerationConfig, GenerationErrors))
        {
            bIsValid = false;
            OutErrors.Append(GenerationErrors);
        }
        
        TArray<FPlanetValidationError> NoiseErrors;
        if (!ValidateNoiseConfig(Config->NoiseConfig, NoiseErrors))
        {
            bIsValid = false;
            OutErrors.Append(NoiseErrors);
        }
        
        TArray<FPlanetValidationError> BiomeErrors;
        if (!ValidateBiomeConfig(Config->BiomeConfig, BiomeErrors))
        {
            bIsValid = false;
            OutErrors.Append(BiomeErrors);
        }
        
        // Verifica conflitos entre configurações
        TArray<FPlanetValidationError> ConflictErrors;
        if (!CheckConfigConflicts(Config, ConflictErrors))
        {
            bIsValid = false;
            OutErrors.Append(ConflictErrors);
        }
        
        // Verifica avisos de performance
        TArray<FPlanetValidationError> PerformanceWarnings;
        CheckPerformanceWarnings(Config, PerformanceWarnings);
        OutErrors.Append(PerformanceWarnings);
        
        // Verificações específicas
        ValidateGenerationSpecifics(Config->GenerationConfig, OutErrors);
        ValidateNoiseSpecifics(Config->NoiseConfig, OutErrors);
        ValidateBiomeSpecifics(Config->BiomeConfig, OutErrors);
        
        // Verificações de limites
        CheckMemoryLimits(Config, OutErrors);
        CheckPerformanceLimits(Config, OutErrors);
        CheckVersionCompatibility(Config, OutErrors);
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Configuration validation completed: %s (%d errors)"), 
                bIsValid ? TEXT("Valid") : TEXT("Invalid"), OutErrors.Num()));
        
        return bIsValid;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Exception during configuration validation: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

bool UPlanetConfigValidator::ValidateGenerationConfig(const FPlanetGenerationConfig& Config, TArray<FPlanetValidationError>& OutErrors)
{
    try
    {
        bool bIsValid = true;
        
        // Valida BaseRadius
        if (!ValidateFloatValue(Config.BaseRadius, 100.0f, 10000.0f, TEXT("BaseRadius"), OutErrors))
        {
            bIsValid = false;
        }
        
        // Valida MaxLODLevel
        if (!ValidateIntValue(Config.MaxLODLevel, 1, 12, TEXT("MaxLODLevel"), OutErrors))
        {
            bIsValid = false;
        }
        
        // Valida LODUpdateInterval
        if (!ValidateFloatValue(Config.LODUpdateInterval, 0.1f, 2.0f, TEXT("LODUpdateInterval"), OutErrors))
        {
            bIsValid = false;
        }
        
        // Valida BaseMeshResolution
        if (!ValidateIntValue(Config.BaseMeshResolution, 2, 32, TEXT("BaseMeshResolution"), OutErrors))
        {
            bIsValid = false;
        }
        
        return bIsValid;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Exception validating generation config: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

bool UPlanetConfigValidator::ValidateNoiseConfig(const FNoiseConfig& Config, TArray<FPlanetValidationError>& OutErrors)
{
    try
    {
        bool bIsValid = true;
        
        // Valida GlobalSeed
        if (Config.GlobalSeed < 0)
        {
            AddError(OutErrors, EPlanetValidationError::InvalidValue, TEXT("GlobalSeed"), 
                FString::Printf(TEXT("%d"), Config.GlobalSeed), TEXT("Seed must be non-negative"), 7);
            bIsValid = false;
        }
        
        // Valida BaseFrequency
        if (!ValidateFloatValue(Config.BaseFrequency, 0.1f, 10.0f, TEXT("BaseFrequency"), OutErrors))
        {
            bIsValid = false;
        }
        
        // Valida Octaves
        if (!ValidateIntValue(Config.Octaves, 1, 12, TEXT("Octaves"), OutErrors))
        {
            bIsValid = false;
        }
        
        // Valida Lacunarity
        if (!ValidateFloatValue(Config.Lacunarity, 1.0f, 4.0f, TEXT("Lacunarity"), OutErrors))
        {
            bIsValid = false;
        }
        
        // Valida Persistence
        if (!ValidateFloatValue(Config.Persistence, 0.0f, 1.0f, TEXT("Persistence"), OutErrors))
        {
            bIsValid = false;
        }
        
        // Valida WarpStrength
        if (Config.bEnableWarp && !ValidateFloatValue(Config.WarpStrength, 0.0f, 2.0f, TEXT("WarpStrength"), OutErrors))
        {
            bIsValid = false;
        }
        
        return bIsValid;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Exception validating noise config: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

bool UPlanetConfigValidator::ValidateBiomeConfig(const FBiomeConfig& Config, TArray<FPlanetValidationError>& OutErrors)
{
    try
    {
        bool bIsValid = true;
        
        // Valida thresholds de altitude
        if (!ValidateFloatValue(Config.DesertAltitudeThreshold, 0.0f, 1.0f, TEXT("DesertAltitudeThreshold"), OutErrors))
        {
            bIsValid = false;
        }
        
        if (!ValidateFloatValue(Config.MountainAltitudeThreshold, 0.0f, 1.0f, TEXT("MountainAltitudeThreshold"), OutErrors))
        {
            bIsValid = false;
        }
        
        if (!ValidateFloatValue(Config.SnowAltitudeThreshold, 0.0f, 1.0f, TEXT("SnowAltitudeThreshold"), OutErrors))
        {
            bIsValid = false;
        }
        
        if (!ValidateFloatValue(Config.ForestHumidityThreshold, 0.0f, 1.0f, TEXT("ForestHumidityThreshold"), OutErrors))
        {
            bIsValid = false;
        }
        
        if (!ValidateFloatValue(Config.PlainsSlopeThreshold, 0.0f, 1.0f, TEXT("PlainsSlopeThreshold"), OutErrors))
        {
            bIsValid = false;
        }
        
        // Verifica consistência dos thresholds
        if (Config.DesertAltitudeThreshold <= Config.MountainAltitudeThreshold)
        {
            AddError(OutErrors, EPlanetValidationError::ConflictingValues, TEXT("AltitudeThresholds"), 
                TEXT("Desert <= Mountain"), TEXT("Desert threshold should be higher than mountain threshold"), 8);
            bIsValid = false;
        }
        
        if (Config.SnowAltitudeThreshold <= Config.MountainAltitudeThreshold)
        {
            AddError(OutErrors, EPlanetValidationError::ConflictingValues, TEXT("AltitudeThresholds"), 
                TEXT("Snow <= Mountain"), TEXT("Snow threshold should be higher than mountain threshold"), 8);
            bIsValid = false;
        }
        
        return bIsValid;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Exception validating biome config: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

bool UPlanetConfigValidator::ValidateFloatValue(float Value, float MinValue, float MaxValue, const FString& FieldName, 
                                                TArray<FPlanetValidationError>& OutErrors)
{
    if (Value < MinValue || Value > MaxValue)
    {
        AddError(OutErrors, EPlanetValidationError::OutOfRange, FieldName, 
            FString::Printf(TEXT("%.3f"), Value), 
            FString::Printf(TEXT("Value must be between %.3f and %.3f"), MinValue, MaxValue), 7);
        return false;
    }
    return true;
}

bool UPlanetConfigValidator::ValidateIntValue(int32 Value, int32 MinValue, int32 MaxValue, const FString& FieldName, 
                                              TArray<FPlanetValidationError>& OutErrors)
{
    if (Value < MinValue || Value > MaxValue)
    {
        AddError(OutErrors, EPlanetValidationError::OutOfRange, FieldName, 
            FString::Printf(TEXT("%d"), Value), 
            FString::Printf(TEXT("Value must be between %d and %d"), MinValue, MaxValue), 7);
        return false;
    }
    return true;
}

bool UPlanetConfigValidator::CheckConfigConflicts(const UPlanetCoreConfig* Config, TArray<FPlanetValidationError>& OutErrors)
{
    try
    {
        bool bHasConflicts = false;
        
        // Verifica se LOD muito alto com resolução baixa
        if (Config->GenerationConfig.MaxLODLevel > 10 && Config->GenerationConfig.BaseMeshResolution < 8)
        {
            AddError(OutErrors, EPlanetValidationError::ConflictingValues, TEXT("LOD_Resolution"), 
                TEXT("High LOD + Low Resolution"), TEXT("High LOD levels should use higher mesh resolution"), 6);
            bHasConflicts = true;
        }
        
        // Verifica se muitos octaves com frequência baixa
        if (Config->NoiseConfig.Octaves > 8 && Config->NoiseConfig.BaseFrequency < 0.5f)
        {
            AddError(OutErrors, EPlanetValidationError::ConflictingValues, TEXT("Octaves_Frequency"), 
                TEXT("High Octaves + Low Frequency"), TEXT("High octave count should use higher base frequency"), 5);
            bHasConflicts = true;
        }
        
        // Verifica se erosão habilitada sem água
        if (Config->GenerationConfig.bEnableErosion && !Config->GenerationConfig.bEnableWater)
        {
            AddError(OutErrors, EPlanetValidationError::ConflictingValues, TEXT("Erosion_Water"), 
                TEXT("Erosion without Water"), TEXT("Erosion simulation requires water to be enabled"), 8);
            bHasConflicts = true;
        }
        
        return !bHasConflicts;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Exception checking config conflicts: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

bool UPlanetConfigValidator::CheckPerformanceWarnings(const UPlanetCoreConfig* Config, TArray<FPlanetValidationError>& OutErrors)
{
    try
    {
        // Avisos de performance
        if (Config->GenerationConfig.MaxLODLevel > 10)
        {
            AddWarning(OutErrors, TEXT("MaxLODLevel"), 
                TEXT("High LOD levels may impact performance significantly"), 4);
        }
        
        if (Config->NoiseConfig.Octaves > 8)
        {
            AddWarning(OutErrors, TEXT("Octaves"), 
                TEXT("High octave count may impact performance"), 3);
        }
        
        if (Config->GenerationConfig.BaseMeshResolution > 16)
        {
            AddWarning(OutErrors, TEXT("BaseMeshResolution"), 
                TEXT("High mesh resolution may impact performance"), 4);
        }
        
        if (Config->GenerationConfig.LODUpdateInterval < 0.1f)
        {
            AddWarning(OutErrors, TEXT("LODUpdateInterval"), 
                TEXT("Very frequent LOD updates may impact performance"), 3);
        }
        
        if (Config->bEnableDebugVisualization && Config->bEnablePerformanceProfiling)
        {
            AddWarning(OutErrors, TEXT("Debug_Profiling"), 
                TEXT("Both debug visualization and performance profiling enabled may impact performance"), 2);
        }
        
        return true;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Exception checking performance warnings: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

bool UPlanetConfigValidator::ApplyAutoFixes(UPlanetCoreConfig* Config, const TArray<FPlanetValidationError>& Errors)
{
    try
    {
        if (!Config)
        {
            return false;
        }
        
        int32 FixesApplied = 0;
        
        for (const FPlanetValidationError& Error : Errors)
        {
            // Aplica correções automáticas baseadas no tipo de erro
            if (Error.ErrorType == EPlanetValidationError::OutOfRange)
            {
                if (Error.FieldName == TEXT("BaseRadius"))
                {
                    float Value = FCString::Atof(*Error.CurrentValue);
                    if (Value < 100.0f) Config->GenerationConfig.BaseRadius = 100.0f;
                    else if (Value > 10000.0f) Config->GenerationConfig.BaseRadius = 10000.0f;
                    FixesApplied++;
                }
                else if (Error.FieldName == TEXT("MaxLODLevel"))
                {
                    int32 Value = FCString::Atoi(*Error.CurrentValue);
                    if (Value < 1) Config->GenerationConfig.MaxLODLevel = 1;
                    else if (Value > 12) Config->GenerationConfig.MaxLODLevel = 12;
                    FixesApplied++;
                }
                else if (Error.FieldName == TEXT("Octaves"))
                {
                    int32 Value = FCString::Atoi(*Error.CurrentValue);
                    if (Value < 1) Config->NoiseConfig.Octaves = 1;
                    else if (Value > 12) Config->NoiseConfig.Octaves = 12;
                    FixesApplied++;
                }
            }
            else if (Error.ErrorType == EPlanetValidationError::ConflictingValues)
            {
                if (Error.FieldName == TEXT("Erosion_Water"))
                {
                    Config->GenerationConfig.bEnableWater = true;
                    FixesApplied++;
                }
            }
        }
        
        if (FixesApplied > 0)
        {
            UPlanetSystemLogger::LogInfo(TEXT("PlanetConfigValidator"), 
                FString::Printf(TEXT("Applied %d automatic fixes"), FixesApplied));
        }
        
        return FixesApplied > 0;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Exception applying auto fixes: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

void UPlanetConfigValidator::GetOptimizationSuggestions(const UPlanetCoreConfig* Config, TArray<FString>& OutSuggestions)
{
    try
    {
        OutSuggestions.Empty();
        
        // Sugestões baseadas na configuração
        if (Config->GenerationConfig.MaxLODLevel > 10)
        {
            OutSuggestions.Add(TEXT("Consider reducing MaxLODLevel to 10 or less for better performance"));
        }
        
        if (Config->NoiseConfig.Octaves > 8)
        {
            OutSuggestions.Add(TEXT("Consider reducing Octaves to 8 or less for better performance"));
        }
        
        if (Config->GenerationConfig.BaseMeshResolution > 16)
        {
            OutSuggestions.Add(TEXT("Consider reducing BaseMeshResolution to 16 or less for better performance"));
        }
        
        if (Config->GenerationConfig.LODUpdateInterval < 0.1f)
        {
            OutSuggestions.Add(TEXT("Consider increasing LODUpdateInterval to 0.1f or higher for better quality"));
        }
        
        if (Config->NoiseConfig.BaseFrequency < 0.5f)
        {
            OutSuggestions.Add(TEXT("Consider increasing BaseFrequency to 0.5f or higher for better detail"));
        }
        
        if (Config->bEnableDebugVisualization && Config->bEnablePerformanceProfiling)
        {
            OutSuggestions.Add(TEXT("Consider disabling debug visualization and performance profiling in production"));
        }
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Generated %d optimization suggestions"), OutSuggestions.Num()));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Exception getting optimization suggestions: %s"), UTF8_TO_TCHAR(e.what())));
    }
}

bool UPlanetConfigValidator::IsProductionReady(const UPlanetCoreConfig* Config, TArray<FPlanetValidationError>& OutErrors)
{
    try
    {
        if (!Config)
        {
            return false;
        }
        
        // Verifica se há erros críticos
        TArray<FPlanetValidationError> AllErrors;
        ValidateConfig(Config, AllErrors);
        
        // Filtra apenas erros críticos (severidade >= 7)
        for (const FPlanetValidationError& Error : AllErrors)
        {
            if (Error.Severity >= 7)
            {
                OutErrors.Add(Error);
            }
        }
        
        // Verificações específicas para produção
        if (Config->bEnableDebugVisualization)
        {
            AddError(OutErrors, EPlanetValidationError::PerformanceWarning, TEXT("DebugVisualization"), 
                TEXT("Enabled"), TEXT("Debug visualization should be disabled in production"), 8);
        }
        
        if (Config->bEnablePerformanceProfiling)
        {
            AddError(OutErrors, EPlanetValidationError::PerformanceWarning, TEXT("PerformanceProfiling"), 
                TEXT("Enabled"), TEXT("Performance profiling should be disabled in production"), 7);
        }
        
        bool bIsProductionReady = OutErrors.Num() == 0;
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Production readiness check: %s (%d critical issues)"), 
                bIsProductionReady ? TEXT("Ready") : TEXT("Not Ready"), OutErrors.Num()));
        
        return bIsProductionReady;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Exception checking production readiness: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

bool UPlanetConfigValidator::ExportValidationReport(const UPlanetCoreConfig* Config, const FString& FilePath)
{
    try
    {
        if (!Config)
        {
            return false;
        }
        
        TArray<FPlanetValidationError> Errors;
        ValidateConfig(Config, Errors);
        
        // Cria relatório JSON
        FString JsonString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
        
        Writer->WriteObjectStart();
        
        // Informações do relatório
        Writer->WriteValue(TEXT("ReportTime"), FDateTime::Now().ToString());
        Writer->WriteValue(TEXT("TotalErrors"), Errors.Num());
        
        // Estatísticas
        TMap<EPlanetValidationError, int32> ErrorCounts;
        int32 TotalErrors, TotalWarnings;
        GetValidationStatistics(Errors, ErrorCounts, TotalErrors, TotalWarnings);
        
        Writer->WriteValue(TEXT("TotalErrors"), TotalErrors);
        Writer->WriteValue(TEXT("TotalWarnings"), TotalWarnings);
        
        // Contagem por tipo
        Writer->WriteObjectStart(TEXT("ErrorCounts"));
        for (const auto& Pair : ErrorCounts)
        {
            const UEnum* ErrorTypeEnum = FindObject<UEnum>(nullptr, TEXT("/Script/PlanetSystem.EPlanetValidationError"));
            if (ErrorTypeEnum)
            {
                FString ErrorTypeName = ErrorTypeEnum->GetNameStringByValue(static_cast<int64>(Pair.Key));
                Writer->WriteValue(ErrorTypeName, Pair.Value);
            }
        }
        Writer->WriteObjectEnd();
        
        // Lista de erros
        Writer->WriteArrayStart(TEXT("Errors"));
        for (const FPlanetValidationError& Error : Errors)
        {
            Writer->WriteObjectStart();
            Writer->WriteValue(TEXT("Type"), static_cast<int32>(Error.ErrorType));
            Writer->WriteValue(TEXT("FieldName"), Error.FieldName);
            Writer->WriteValue(TEXT("CurrentValue"), Error.CurrentValue);
            Writer->WriteValue(TEXT("ExpectedValue"), Error.ExpectedValue);
            Writer->WriteValue(TEXT("ErrorMessage"), Error.ErrorMessage);
            Writer->WriteValue(TEXT("Severity"), Error.Severity);
            Writer->WriteValue(TEXT("Suggestion"), Error.Suggestion);
            Writer->WriteObjectEnd();
        }
        Writer->WriteArrayEnd();
        
        Writer->WriteObjectEnd();
        Writer->Close();
        
        // Salva o arquivo
        if (FFileHelper::SaveStringToFile(JsonString, *FilePath))
        {
            UPlanetSystemLogger::LogInfo(TEXT("PlanetConfigValidator"), 
                FString::Printf(TEXT("Validation report exported to: %s"), *FilePath));
            return true;
        }
        
        UPlanetSystemLogger::LogError(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Failed to export validation report to: %s"), *FilePath));
        return false;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Exception exporting validation report: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

void UPlanetConfigValidator::GetValidationStatistics(const TArray<FPlanetValidationError>& Errors, 
                                                     TMap<EPlanetValidationError, int32>& OutErrorCounts,
                                                     int32& OutTotalErrors, int32& OutTotalWarnings)
{
    try
    {
        OutErrorCounts.Empty();
        OutTotalErrors = 0;
        OutTotalWarnings = 0;
        
        for (const FPlanetValidationError& Error : Errors)
        {
            // Conta por tipo
            OutErrorCounts.FindOrAdd(Error.ErrorType)++;
            
            // Conta por severidade
            if (Error.Severity >= 7)
            {
                OutTotalErrors++;
            }
            else
            {
                OutTotalWarnings++;
            }
        }
        
        UPlanetSystemLogger::LogDebug(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Validation statistics: %d errors, %d warnings"), OutTotalErrors, OutTotalWarnings));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetConfigValidator"), 
            FString::Printf(TEXT("Exception getting validation statistics: %s"), UTF8_TO_TCHAR(e.what())));
    }
}

// Funções privadas auxiliares

void UPlanetConfigValidator::ValidateGenerationSpecifics(const FPlanetGenerationConfig& Config, TArray<FPlanetValidationError>& OutErrors)
{
    // Validações específicas de geração
    if (Config.bEnableErosion && Config.LODUpdateInterval < 0.05f)
    {
        AddWarning(OutErrors, TEXT("Erosion_LODUpdate"), 
            TEXT("Erosion with very frequent LOD updates may cause visual artifacts"), 4);
    }
}

void UPlanetConfigValidator::ValidateNoiseSpecifics(const FNoiseConfig& Config, TArray<FPlanetValidationError>& OutErrors)
{
    // Validações específicas de noise
    if (Config.bEnableWarp && Config.WarpStrength > 1.0f && Config.Octaves < 4)
    {
        AddWarning(OutErrors, TEXT("Warp_Octaves"), 
            TEXT("High warp strength with low octaves may cause noise artifacts"), 5);
    }
}

void UPlanetConfigValidator::ValidateBiomeSpecifics(const FBiomeConfig& Config, TArray<FPlanetValidationError>& OutErrors)
{
    // Validações específicas de biomas
    if (Config.DesertAltitudeThreshold + Config.SnowAltitudeThreshold > 1.5f)
    {
        AddWarning(OutErrors, TEXT("BiomeThresholds"), 
            TEXT("Sum of desert and snow thresholds may leave little room for other biomes"), 3);
    }
}

void UPlanetConfigValidator::CheckMemoryLimits(const UPlanetCoreConfig* Config, TArray<FPlanetValidationError>& OutErrors)
{
    // Verifica limites de memória estimados
    const float EstimatedMemoryMB = Config->GenerationConfig.BaseRadius * Config->GenerationConfig.MaxLODLevel * 0.001f;
    
    if (EstimatedMemoryMB > 1000.0f) // 1GB
    {
        AddError(OutErrors, EPlanetValidationError::PerformanceWarning, TEXT("MemoryEstimate"), 
            FString::Printf(TEXT("%.1f MB"), EstimatedMemoryMB), 
            TEXT("Estimated memory usage exceeds 1GB"), 8);
    }
}

void UPlanetConfigValidator::CheckPerformanceLimits(const UPlanetCoreConfig* Config, TArray<FPlanetValidationError>& OutErrors)
{
    // Verifica limites de performance estimados
    const float ComplexityScore = Config->GenerationConfig.MaxLODLevel * Config->NoiseConfig.Octaves * 
                                 Config->GenerationConfig.BaseMeshResolution;
    
    if (ComplexityScore > 1000.0f)
    {
        AddError(OutErrors, EPlanetValidationError::PerformanceWarning, TEXT("ComplexityScore"), 
            FString::Printf(TEXT("%.1f"), ComplexityScore), 
            TEXT("Configuration complexity may impact performance significantly"), 7);
    }
}

void UPlanetConfigValidator::CheckVersionCompatibility(const UPlanetCoreConfig* Config, TArray<FPlanetValidationError>& OutErrors)
{
    // Verifica compatibilidade de versão (implementação futura)
    // Por enquanto, sempre retorna compatível
}

void UPlanetConfigValidator::AddError(TArray<FPlanetValidationError>& OutErrors, EPlanetValidationError ErrorType, 
                                      const FString& FieldName, const FString& ErrorMessage, int32 Severity)
{
    FPlanetValidationError Error(ErrorType, FieldName, ErrorMessage, Severity);
    OutErrors.Add(Error);
}

void UPlanetConfigValidator::AddWarning(TArray<FPlanetValidationError>& OutErrors, const FString& FieldName, 
                                        const FString& WarningMessage, int32 Severity)
{
    FPlanetValidationError Warning(EPlanetValidationError::PerformanceWarning, FieldName, WarningMessage, Severity);
    OutErrors.Add(Warning);
} 