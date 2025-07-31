#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Configuration/DataAssets/CoreConfig.h"
#include "Core/Events/PlanetSystemEvents.h"
#include "Common/PlanetTypes.h"
#include "PlanetDataSerializer.generated.h"

// Forward declarations
class UObject;
struct FPlanetChunk;

/**
 * Sistema de serialização para dados do PlanetSystem
 * Permite salvar/carregar configurações, chunks, eventos e outros dados
 * Segue o padrão AAA data-driven inspirado na Source2
 */
UCLASS(BlueprintType, Blueprintable)
class PLANETSYSTEM_API UPlanetDataSerializer : public UObject
{
    GENERATED_BODY()
    
public:
    UPlanetDataSerializer();
    
    /**
     * Obtém a instância singleton do Serializer
     * @return Instância do Serializer
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer", CallInEditor=true)
    static UPlanetDataSerializer* GetInstance();
    
    /**
     * Salva uma configuração do sistema
     * @param Config Configuração a ser salva
     * @param FileName Nome do arquivo (opcional)
     * @return true se salvou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool SaveCoreConfig(const UPlanetCoreConfig* Config, const FString& FileName = TEXT(""));
    
    /**
     * Carrega uma configuração do sistema
     * @param OutConfig Configuração carregada
     * @param FileName Nome do arquivo (opcional)
     * @return true se carregou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool LoadCoreConfig(UPlanetCoreConfig*& OutConfig, const FString& FileName = TEXT(""));
    
    /**
     * Salva um chunk de terreno
     * @param Chunk Chunk a ser salvo
     * @param FileName Nome do arquivo
     * @return true se salvou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool SaveChunk(const FPlanetChunk& Chunk, const FString& FileName);
    
    /**
     * Carrega um chunk de terreno
     * @param OutChunk Chunk carregado
     * @param FileName Nome do arquivo
     * @return true se carregou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool LoadChunk(FPlanetChunk& OutChunk, const FString& FileName);
    
    /**
     * Salva um array de eventos
     * @param Events Array de eventos a ser salvo
     * @param FileName Nome do arquivo
     * @return true se salvou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool SaveEvents(const TArray<FPlanetSystemEvent>& Events, const FString& FileName);
    
    /**
     * Carrega um array de eventos
     * @param OutEvents Array de eventos carregado
     * @param FileName Nome do arquivo
     * @return true se carregou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool LoadEvents(TArray<FPlanetSystemEvent>& OutEvents, const FString& FileName);
    
    /**
     * Salva dados genéricos em formato JSON
     * @param Data Dados a serem salvos
     * @param FileName Nome do arquivo
     * @return true se salvou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool SaveJsonData(const FString& Data, const FString& FileName);
    
    /**
     * Carrega dados genéricos em formato JSON
     * @param OutData Dados carregados
     * @param FileName Nome do arquivo
     * @return true se carregou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool LoadJsonData(FString& OutData, const FString& FileName);
    
    /**
     * Salva dados em formato binário
     * @param Data Dados binários
     * @param FileName Nome do arquivo
     * @return true se salvou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool SaveBinaryData(const TArray<uint8>& Data, const FString& FileName);
    
    /**
     * Carrega dados em formato binário
     * @param OutData Dados binários carregados
     * @param FileName Nome do arquivo
     * @return true se carregou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool LoadBinaryData(TArray<uint8>& OutData, const FString& FileName);
    
    /**
     * Verifica se um arquivo existe
     * @param FileName Nome do arquivo
     * @return true se existe
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool FileExists(const FString& FileName) const;
    
    /**
     * Deleta um arquivo
     * @param FileName Nome do arquivo
     * @return true se deletou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool DeleteFile(const FString& FileName);
    
    /**
     * Obtém o tamanho de um arquivo
     * @param FileName Nome do arquivo
     * @return Tamanho em bytes
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    int64 GetFileSize(const FString& FileName) const;
    
    /**
     * Obtém a data de modificação de um arquivo
     * @param FileName Nome do arquivo
     * @return Data de modificação
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    FDateTime GetFileModificationTime(const FString& FileName) const;
    
    /**
     * Lista todos os arquivos em um diretório
     * @param Directory Diretório a ser listado
     * @param FileExtension Extensão dos arquivos (opcional)
     * @return Array com nomes dos arquivos
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    TArray<FString> ListFiles(const FString& Directory, const FString& FileExtension = TEXT(""));
    
    /**
     * Cria um backup de um arquivo
     * @param FileName Nome do arquivo
     * @param BackupSuffix Sufixo do backup (opcional)
     * @return true se criou backup com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool CreateBackup(const FString& FileName, const FString& BackupSuffix = TEXT("_backup"));
    
    /**
     * Restaura um arquivo de backup
     * @param FileName Nome do arquivo
     * @param BackupSuffix Sufixo do backup (opcional)
     * @return true se restaurou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool RestoreBackup(const FString& FileName, const FString& BackupSuffix = TEXT("_backup"));
    
    /**
     * Obtém estatísticas do Serializer
     * @param OutStats String com estatísticas
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    void GetSerializerStats(FString& OutStats) const;
    
    /**
     * Limpa todos os dados salvos
     * @return true se limpou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool ClearAllData();
    
    /**
     * Verifica a integridade de um arquivo
     * @param FileName Nome do arquivo
     * @return true se arquivo está íntegro
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool ValidateFileIntegrity(const FString& FileName);
    
    /**
     * Compacta dados salvos (remove dados desnecessários)
     * @return true se compactou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetDataSerializer")
    bool CompactData();
    
protected:
    /**
     * Obtém o diretório base para salvamento
     * @return Caminho do diretório
     */
    FString GetBaseDirectory() const;
    
    /**
     * Obtém o caminho completo de um arquivo
     * @param FileName Nome do arquivo
     * @return Caminho completo
     */
    FString GetFullPath(const FString& FileName) const;
    
    /**
     * Cria o diretório base se não existir
     */
    void EnsureBaseDirectoryExists() const;
    
private:
    // Instância singleton
    static UPlanetDataSerializer* Instance;
    
    // Configurações
    UPROPERTY()
    FString BaseDirectory = TEXT("PlanetSystem/Data");
    
    UPROPERTY()
    bool bCompressionEnabled = true;
    
    UPROPERTY()
    bool bBackupEnabled = true;
    
    UPROPERTY()
    int32 MaxBackupFiles = 5;
    
    // Estatísticas
    UPROPERTY()
    int32 TotalFilesSaved = 0;
    
    UPROPERTY()
    int32 TotalFilesLoaded = 0;
    
    UPROPERTY()
    int64 TotalBytesSaved = 0;
    
    UPROPERTY()
    int64 TotalBytesLoaded = 0;
    
    UPROPERTY()
    float LastOperationTime = 0.0f;
    
    /**
     * Registra uma operação de salvamento
     * @param FileName Nome do arquivo
     * @param FileSize Tamanho do arquivo
     */
    void RecordSaveOperation(const FString& FileName, int64 FileSize);
    
    /**
     * Registra uma operação de carregamento
     * @param FileName Nome do arquivo
     * @param FileSize Tamanho do arquivo
     */
    void RecordLoadOperation(const FString& FileName, int64 FileSize);
    
    /**
     * Compressa dados
     * @param UncompressedData Dados não comprimidos
     * @param OutCompressedData Dados comprimidos
     * @return true se comprimiu com sucesso
     */
    bool CompressData(const TArray<uint8>& UncompressedData, TArray<uint8>& OutCompressedData);
    
    /**
     * Descomprime dados
     * @param CompressedData Dados comprimidos
     * @param OutUncompressedData Dados descomprimidos
     * @return true se descomprimiu com sucesso
     */
    bool DecompressData(const TArray<uint8>& CompressedData, TArray<uint8>& OutUncompressedData);
}; 