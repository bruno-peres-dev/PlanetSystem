#pragma once
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "CoreConfig.h"
#include "PlanetConfigValidator.generated.h"

/**
 * Tipos de erro de validação
 */
UENUM(BlueprintType)
enum class EPlanetValidationError : uint8
{
    None                    UMETA(DisplayName = "None"),
    InvalidValue            UMETA(DisplayName = "Invalid Value"),
    OutOfRange              UMETA(DisplayName = "Out of Range"),
    MissingRequired         UMETA(DisplayName = "Missing Required"),
    ConflictingValues       UMETA(DisplayName = "Conflicting Values"),
    PerformanceWarning      UMETA(DisplayName = "Performance Warning"),
    DeprecatedValue         UMETA(DisplayName = "Deprecated Value")
};

/**
 * Estrutura de erro de validação
 */
USTRUCT(BlueprintType)
struct PLANETSYSTEM_API FPlanetValidationError
{
    GENERATED_BODY()
    
    /** Tipo do erro */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Validation")
    EPlanetValidationError ErrorType = EPlanetValidationError::None;
    
    /** Campo que causou o erro */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Validation")
    FString FieldName;
    
    /** Valor atual que causou o erro */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Validation")
    FString CurrentValue;
    
    /** Valor esperado ou recomendado */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Validation")
    FString ExpectedValue;
    
    /** Mensagem de erro detalhada */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Validation")
    FString ErrorMessage;
    
    /** Severidade do erro (0 = baixa, 10 = alta) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Validation", meta=(ClampMin="0", ClampMax="10"))
    int32 Severity = 5;
    
    /** Sugestão de correção */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Validation")
    FString Suggestion;
    
    FPlanetValidationError()
    {
        ErrorType = EPlanetValidationError::None;
        Severity = 5;
    }
    
    FPlanetValidationError(EPlanetValidationError InErrorType, const FString& InFieldName, 
                          const FString& InErrorMessage, int32 InSeverity = 5)
        : ErrorType(InErrorType)
        , FieldName(InFieldName)
        , ErrorMessage(InErrorMessage)
        , Severity(InSeverity)
    {
    }
    
    /**
     * Converte o erro para string
     */
    FString ToString() const
    {
        return FString::Printf(TEXT("[%s] %s: %s (Severity: %d)"), 
            *GetErrorTypeName(), 
            *FieldName, 
            *ErrorMessage, 
            Severity);
    }
    
private:
    FString GetErrorTypeName() const
    {
        static const UEnum* ErrorTypeEnum = FindObject<UEnum>(nullptr, TEXT("/Script/PlanetSystem.EPlanetValidationError"));
        if (ErrorTypeEnum)
        {
            return ErrorTypeEnum->GetNameStringByValue(static_cast<int64>(ErrorType));
        }
        return TEXT("Unknown");
    }
};

/**
 * Sistema de validação de configuração do PlanetSystem
 * Segue padrões AAA para validação de dados
 */
UCLASS(Blueprintable, ClassGroup=(Configuration))
class PLANETSYSTEM_API UPlanetConfigValidator : public UObject
{
    GENERATED_BODY()
    
public:
    UPlanetConfigValidator();
    
    /** Valida a configuração completa do planeta */
    UFUNCTION(BlueprintCallable, Category="Validation")
    static bool ValidateConfig(const UPlanetCoreConfig* Config, TArray<FPlanetValidationError>& OutErrors);
    
    /** Valida apenas a configuração de geração */
    UFUNCTION(BlueprintCallable, Category="Validation")
    static bool ValidateGenerationConfig(const FPlanetGenerationConfig& Config, TArray<FPlanetValidationError>& OutErrors);
    
    /** Valida apenas a configuração de noise */
    UFUNCTION(BlueprintCallable, Category="Validation")
    static bool ValidateNoiseConfig(const FNoiseConfig& Config, TArray<FPlanetValidationError>& OutErrors);
    
    /** Valida apenas a configuração de biomas */
    UFUNCTION(BlueprintCallable, Category="Validation")
    static bool ValidateBiomeConfig(const FBiomeConfig& Config, TArray<FPlanetValidationError>& OutErrors);
    
    /** Valida valores individuais */
    UFUNCTION(BlueprintCallable, Category="Validation")
    static bool ValidateFloatValue(float Value, float MinValue, float MaxValue, const FString& FieldName, 
                                  TArray<FPlanetValidationError>& OutErrors);
    
    UFUNCTION(BlueprintCallable, Category="Validation")
    static bool ValidateIntValue(int32 Value, int32 MinValue, int32 MaxValue, const FString& FieldName, 
                                TArray<FPlanetValidationError>& OutErrors);
    
    /** Verifica conflitos entre configurações */
    UFUNCTION(BlueprintCallable, Category="Validation")
    static bool CheckConfigConflicts(const UPlanetCoreConfig* Config, TArray<FPlanetValidationError>& OutErrors);
    
    /** Verifica avisos de performance */
    UFUNCTION(BlueprintCallable, Category="Validation")
    static bool CheckPerformanceWarnings(const UPlanetCoreConfig* Config, TArray<FPlanetValidationError>& OutErrors);
    
    /** Aplica correções automáticas quando possível */
    UFUNCTION(BlueprintCallable, Category="Validation")
    static bool ApplyAutoFixes(UPlanetCoreConfig* Config, const TArray<FPlanetValidationError>& Errors);
    
    /** Obtém sugestões de otimização */
    UFUNCTION(BlueprintCallable, Category="Validation")
    static void GetOptimizationSuggestions(const UPlanetCoreConfig* Config, TArray<FString>& OutSuggestions);
    
    /** Verifica se a configuração é válida para uso em produção */
    UFUNCTION(BlueprintCallable, Category="Validation")
    static bool IsProductionReady(const UPlanetCoreConfig* Config, TArray<FPlanetValidationError>& OutErrors);
    
    /** Exporta relatório de validação */
    UFUNCTION(BlueprintCallable, Category="Validation")
    static bool ExportValidationReport(const UPlanetCoreConfig* Config, const FString& FilePath);
    
    /** Obtém estatísticas de validação */
    UFUNCTION(BlueprintCallable, Category="Validation")
    static void GetValidationStatistics(const TArray<FPlanetValidationError>& Errors, 
                                       TMap<EPlanetValidationError, int32>& OutErrorCounts,
                                       int32& OutTotalErrors, int32& OutTotalWarnings);
    
private:
    /** Validações específicas de geração */
    static void ValidateGenerationSpecifics(const FPlanetGenerationConfig& Config, TArray<FPlanetValidationError>& OutErrors);
    
    /** Validações específicas de noise */
    static void ValidateNoiseSpecifics(const FNoiseConfig& Config, TArray<FPlanetValidationError>& OutErrors);
    
    /** Validações específicas de biomas */
    static void ValidateBiomeSpecifics(const FBiomeConfig& Config, TArray<FPlanetValidationError>& OutErrors);
    
    /** Verifica limites de memória */
    static void CheckMemoryLimits(const UPlanetCoreConfig* Config, TArray<FPlanetValidationError>& OutErrors);
    
    /** Verifica limites de performance */
    static void CheckPerformanceLimits(const UPlanetCoreConfig* Config, TArray<FPlanetValidationError>& OutErrors);
    
    /** Verifica compatibilidade de versão */
    static void CheckVersionCompatibility(const UPlanetCoreConfig* Config, TArray<FPlanetValidationError>& OutErrors);
    
    /** Adiciona um erro à lista */
    static void AddError(TArray<FPlanetValidationError>& OutErrors, EPlanetValidationError ErrorType, 
                        const FString& FieldName, const FString& ErrorMessage, int32 Severity = 5);
    
    /** Adiciona um warning à lista */
    static void AddWarning(TArray<FPlanetValidationError>& OutErrors, const FString& FieldName, 
                          const FString& WarningMessage, int32 Severity = 3);
}; 