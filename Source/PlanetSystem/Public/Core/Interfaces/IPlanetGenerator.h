#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/Events/PlanetSystemEvents.h"
#include "IPlanetGenerator.generated.h"

// Forward declarations
struct FPlanetChunk;
class UPlanetCoreConfig;

UINTERFACE(MinimalAPI)
class UPlanetGenerator : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface para geradores de terreno do PlanetSystem
 * Segue o padrão AAA data-driven inspirado na Source2
 */
class PLANETSYSTEM_API IPlanetGenerator
{
    GENERATED_BODY()
    
public:
    /**
     * Gera um chunk de terreno na posição especificada
     * @param Center Centro do chunk no espaço 3D
     * @param LODLevel Nível de detalhe (0 = mais detalhado)
     * @return Chunk gerado com dados de terreno
     */
    virtual FPlanetChunk GenerateChunk(const FVector& Center, int32 LODLevel) = 0;
    
    /**
     * Verifica se pode gerar um chunk na posição especificada
     * @param Center Centro do chunk
     * @param LODLevel Nível de detalhe
     * @return true se pode gerar, false caso contrário
     */
    virtual bool CanGenerateChunk(const FVector& Center, int32 LODLevel) const = 0;
    
    /**
     * Calcula a prioridade de geração para um chunk
     * @param Center Centro do chunk
     * @param LODLevel Nível de detalhe
     * @return Valor de prioridade (maior = mais prioritário)
     */
    virtual float GetGenerationPriority(const FVector& Center, int32 LODLevel) const = 0;
    
    /**
     * Inicializa o gerador com configuração
     * @param Config Configuração do sistema
     */
    virtual void Initialize(const UPlanetCoreConfig* Config) = 0;
    
    /**
     * Finaliza o gerador e libera recursos
     */
    virtual void Shutdown() = 0;
    
    /**
     * Verifica se o gerador está inicializado
     * @return true se inicializado
     */
    virtual bool IsInitialized() const = 0;
    
    /**
     * Retorna o nome do gerador
     * @return Nome do gerador
     */
    virtual FString GetGeneratorName() const = 0;
    
    /**
     * Retorna a versão do gerador
     * @return Versão do gerador
     */
    virtual FString GetGeneratorVersion() const = 0;
    
    /**
     * Obtém estatísticas de performance do gerador
     * @param OutStats String com estatísticas
     */
    virtual void GetPerformanceStats(FString& OutStats) const = 0;
    
    /**
     * Verifica se o gerador suporta um tipo específico de terreno
     * @param TerrainType Tipo de terreno
     * @return true se suporta
     */
    virtual bool SupportsTerrainType(const FString& TerrainType) const = 0;
    
    /**
     * Obtém os tipos de terreno suportados
     * @return Array com tipos suportados
     */
    virtual TArray<FString> GetSupportedTerrainTypes() const = 0;
}; 