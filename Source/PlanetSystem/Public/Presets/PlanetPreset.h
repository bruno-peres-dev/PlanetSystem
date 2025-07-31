#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Configuration/DataAssets/CoreConfig.h"
#include "PlanetPreset.generated.h"

// Forward declarations
class UPlanetCoreConfig;

/**
 * Sistema de presets para configurações pré-definidas do PlanetSystem
 * Permite salvar/carregar configurações completas como presets
 * Segue o padrão AAA data-driven inspirado na Source2
 */
UCLASS(BlueprintType, Blueprintable)
class PLANETSYSTEM_API UPlanetPreset : public UDataAsset
{
    GENERATED_BODY()
    
public:
    UPlanetPreset();
    
    /**
     * Obtém a instância singleton do sistema de presets
     * @return Instância do sistema de presets
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset", CallInEditor=true)
    static UPlanetPreset* GetInstance();
    
    /**
     * Cria um novo preset a partir de uma configuração
     * @param Config Configuração base
     * @param PresetName Nome do preset
     * @param Description Descrição do preset
     * @param Tags Tags para categorização
     * @return Preset criado
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static UPlanetPreset* CreatePreset(const UPlanetCoreConfig* Config, const FString& PresetName, 
                                      const FString& Description = TEXT(""), const TArray<FString>& Tags = {});
    
    /**
     * Salva um preset no disco
     * @param Preset Preset a ser salvo
     * @param FileName Nome do arquivo (opcional)
     * @return true se salvou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static bool SavePreset(const UPlanetPreset* Preset, const FString& FileName = TEXT(""));
    
    /**
     * Carrega um preset do disco
     * @param OutPreset Preset carregado
     * @param FileName Nome do arquivo
     * @return true se carregou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static bool LoadPreset(UPlanetPreset*& OutPreset, const FString& FileName);
    
    /**
     * Aplica um preset a uma configuração
     * @param Preset Preset a ser aplicado
     * @param TargetConfig Configuração de destino
     * @return true se aplicou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static bool ApplyPreset(const UPlanetPreset* Preset, UPlanetCoreConfig* TargetConfig);
    
    /**
     * Lista todos os presets disponíveis
     * @param Category Categoria para filtrar (opcional)
     * @return Array com presets
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static TArray<UPlanetPreset*> ListAvailablePresets(const FString& Category = TEXT(""));
    
    /**
     * Busca presets por nome ou tags
     * @param SearchTerm Termo de busca
     * @return Array com presets encontrados
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static TArray<UPlanetPreset*> SearchPresets(const FString& SearchTerm);
    
    /**
     * Obtém presets por categoria
     * @param Category Categoria
     * @return Array com presets da categoria
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static TArray<UPlanetPreset*> GetPresetsByCategory(const FString& Category);
    
    /**
     * Obtém todas as categorias disponíveis
     * @return Array com categorias
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static TArray<FString> GetAvailableCategories();
    
    /**
     * Obtém todas as tags disponíveis
     * @return Array com tags
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static TArray<FString> GetAvailableTags();
    
    /**
     * Verifica se um preset é compatível com uma configuração
     * @param Preset Preset a verificar
     * @param Config Configuração de destino
     * @return true se compatível
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static bool IsPresetCompatible(const UPlanetPreset* Preset, const UPlanetCoreConfig* Config);
    
    /**
     * Obtém informações de compatibilidade
     * @param Preset Preset a verificar
     * @param Config Configuração de destino
     * @param OutCompatibilityInfo Informações de compatibilidade
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static void GetCompatibilityInfo(const UPlanetPreset* Preset, const UPlanetCoreConfig* Config, 
                                   FString& OutCompatibilityInfo);
    
    /**
     * Valida um preset
     * @param Preset Preset a validar
     * @param OutValidationErrors Erros de validação
     * @return true se válido
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static bool ValidatePreset(const UPlanetPreset* Preset, TArray<FString>& OutValidationErrors);
    
    /**
     * Obtém estatísticas de um preset
     * @param Preset Preset
     * @param OutStats Estatísticas
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static void GetPresetStats(const UPlanetPreset* Preset, FString& OutStats);
    
    /**
     * Compara dois presets
     * @param PresetA Primeiro preset
     * @param PresetB Segundo preset
     * @param OutDifferences Diferenças encontradas
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static void ComparePresets(const UPlanetPreset* PresetA, const UPlanetPreset* PresetB, 
                             TArray<FString>& OutDifferences);
    
    /**
     * Cria um preset de diferença entre dois presets
     * @param PresetA Primeiro preset
     * @param PresetB Segundo preset
     * @return Preset de diferença
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static UPlanetPreset* CreateDifferencePreset(const UPlanetPreset* PresetA, const UPlanetPreset* PresetB);
    
    /**
     * Mescla dois presets
     * @param PresetA Primeiro preset
     * @param PresetB Segundo preset
     * @param MergeStrategy Estratégia de mesclagem
     * @return Preset mesclado
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static UPlanetPreset* MergePresets(const UPlanetPreset* PresetA, const UPlanetPreset* PresetB, 
                                     const FString& MergeStrategy = TEXT("Priority"));
    
    /**
     * Obtém estatísticas do sistema de presets
     * @param OutStats Estatísticas
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static void GetPresetSystemStats(FString& OutStats);
    
    /**
     * Limpa todos os presets
     * @return true se limpou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static bool ClearAllPresets();
    
    /**
     * Exporta presets para um arquivo
     * @param Presets Array de presets
     * @param FileName Nome do arquivo
     * @return true se exportou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static bool ExportPresets(const TArray<UPlanetPreset*>& Presets, const FString& FileName);
    
    /**
     * Importa presets de um arquivo
     * @param OutPresets Array de presets importados
     * @param FileName Nome do arquivo
     * @return true se importou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPreset")
    static bool ImportPresets(TArray<UPlanetPreset*>& OutPresets, const FString& FileName);
    
    // Propriedades do preset
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
    FString PresetName;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
    FString Description;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
    TArray<FString> Tags;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
    FString Category;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
    FString Author;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
    FString Version;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
    FDateTime CreationDate;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
    FDateTime LastModifiedDate;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
    float Rating;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
    int32 UsageCount;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
    bool bIsDefault;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
    bool bIsReadOnly;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Preset")
    bool bIsHidden;
    
    // Configuração do preset
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Configuration")
    UPlanetCoreConfig* Configuration;
    
    // Metadados adicionais
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Metadata")
    TMap<FString, FString> CustomProperties;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Metadata")
    TArray<FString> Dependencies;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Metadata")
    FString CompatibilityVersion;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Metadata")
    TArray<FString> SupportedPlatforms;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Metadata")
    FString MinimumEngineVersion;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Metadata")
    FString RecommendedEngineVersion;
    
protected:
    /**
     * Obtém o diretório base para presets
     * @return Caminho do diretório
     */
    static FString GetPresetsDirectory();
    
    /**
     * Obtém o caminho completo de um arquivo de preset
     * @param FileName Nome do arquivo
     * @return Caminho completo
     */
    static FString GetPresetFilePath(const FString& FileName);
    
    /**
     * Cria o diretório de presets se não existir
     */
    static void EnsurePresetsDirectoryExists();
    
private:
    // Instância singleton
    static UPlanetPreset* Instance;
    
    /**
     * Registra o uso de um preset
     * @param Preset Preset usado
     */
    static void RecordPresetUsage(UPlanetPreset* Preset);
    
    /**
     * Atualiza as estatísticas de um preset
     * @param Preset Preset a atualizar
     */
    static void UpdatePresetStats(UPlanetPreset* Preset);
    
    /**
     * Valida a estrutura de um preset
     * @param Preset Preset a validar
     * @param OutErrors Erros encontrados
     * @return true se válido
     */
    static bool ValidatePresetStructure(const UPlanetPreset* Preset, TArray<FString>& OutErrors);
}; 