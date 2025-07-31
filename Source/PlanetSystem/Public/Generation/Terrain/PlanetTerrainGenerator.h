#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Configuration/DataAssets/CoreConfig.h"
#include "Generation/Noise/NoiseModule.h"
#include "Services/Terrain/ErosionModule.h"
#include "Services/Environment/BiomeSystem.h"
#include "Services/Environment/VegetationSystem.h"
#include "Services/Environment/WaterComponent.h"
#include "Common/PlanetTypes.h"
#include "PlanetTerrainGenerator.generated.h"

// Forward declarations
class UProceduralMeshComponent;
class UPlanetSystemLogger;
class UPlanetEventBus;

/**
 * Sistema de geração real de terreno para planetas
 * Implementa algoritmos de geração procedural AAA
 */
UCLASS(Blueprintable, BlueprintType)
class PLANETSYSTEM_API UPlanetTerrainGenerator : public UObject
{
    GENERATED_BODY()

public:
    UPlanetTerrainGenerator();

    // === GERAÇÃO DE TERRENO ===
    
    /**
     * Gera um chunk de terreno completo
     * @param Center - Centro do chunk
     * @param LODLevel - Nível de detalhe
     * @return Chunk gerado
     */
    UFUNCTION(BlueprintCallable, Category="Terrain Generation")
    FPlanetChunk GenerateTerrainChunk(const FVector& Center, int32 LODLevel);

    /**
     * Aplica biomas ao chunk
     * @param Chunk - Chunk a ser modificado
     * @param BiomeConfig - Configuração de biomas
     */
    UFUNCTION(BlueprintCallable, Category="Terrain Generation")
    void ApplyBiomesToChunk(FPlanetChunk& Chunk, const FBiomeConfig& BiomeConfig);

    /**
     * Gera vegetação para o chunk
     * @param Chunk - Chunk base
     * @param OutVegetation - Vegetação gerada
     */
    UFUNCTION(BlueprintCallable, Category="Terrain Generation")
    void GenerateVegetationForChunk(const FPlanetChunk& Chunk, TArray<FVegetationInstance>& OutVegetation);

    /**
     * Gera sistema de água para o chunk
     * @param Chunk - Chunk base
     * @param OutWaterSystem - Sistema de água gerado
     */
    UFUNCTION(BlueprintCallable, Category="Terrain Generation")
    void GenerateWaterSystem(const FPlanetChunk& Chunk, FWaterSystem& OutWaterSystem);

    // === CONFIGURAÇÃO ===
    
    /**
     * Define a configuração de geração
     * @param Config - Nova configuração
     */
    UFUNCTION(BlueprintCallable, Category="Terrain Generation")
    void SetGenerationConfig(const UPlanetCoreConfig* Config);

    /**
     * Obtém a configuração atual
     * @return Configuração atual
     */
    UFUNCTION(BlueprintCallable, Category="Terrain Generation")
    const UPlanetCoreConfig* GetGenerationConfig() const;

    // === ESTATÍSTICAS ===
    
    /**
     * Obtém estatísticas de geração
     * @param OutStats - Estatísticas formatadas
     */
    UFUNCTION(BlueprintCallable, Category="Terrain Generation")
    void GetGenerationStats(FString& OutStats) const;

    /**
     * Reseta estatísticas
     */
    UFUNCTION(BlueprintCallable, Category="Terrain Generation")
    void ResetStats();

protected:
    // === ALGORITMOS DE GERAÇÃO ===
    
    /**
     * Gera mapa de altura
     * @param Center - Centro do chunk
     * @param LODLevel - Nível de detalhe
     * @param OutHeightMap - Mapa de altura gerado
     */
    void GenerateHeightMap(const FVector& Center, int32 LODLevel, TArray<float>& OutHeightMap);

    /**
     * Aplica erosão ao mapa de altura
     * @param HeightMap - Mapa de altura a ser modificado
     * @param ErosionConfig - Configuração de erosão
     */
    void ApplyErosion(TArray<float>& HeightMap, const FErosionConfig& ErosionConfig);

    /**
     * Gera mapa de biomas
     * @param HeightMap - Mapa de altura base
     * @param OutBiomeMap - Mapa de biomas gerado
     */
    void GenerateBiomeMap(const TArray<float>& HeightMap, TArray<EBiomeType>& OutBiomeMap);

    /**
     * Gera mapa de vegetação
     * @param BiomeMap - Mapa de biomas base
     * @param OutVegetation - Vegetação gerada
     */
    void GenerateVegetationMap(const TArray<EBiomeType>& BiomeMap, TArray<FVegetationInstance>& OutVegetation);

    /**
     * Calcula temperatura baseada na latitude
     * @param Latitude - Latitude em radianos
     * @return Temperatura normalizada (0-1)
     */
    float CalculateTemperature(float Latitude);

    /**
     * Calcula umidade baseada na altura e temperatura
     * @param Height - Altura normalizada
     * @param Temperature - Temperatura normalizada
     * @return Umidade normalizada (0-1)
     */
    float CalculateHumidity(float Height, float Temperature);

    /**
     * Aplica ruído de detalhe ao mapa de altura
     * @param HeightMap - Mapa de altura a ser modificado
     * @param DetailScale - Escala do detalhe
     * @param DetailStrength - Força do detalhe
     */
    void ApplyDetailNoise(TArray<float>& HeightMap, float DetailScale, float DetailStrength);

private:
    // === COMPONENTES ===
    
    /** Sistema de noise */
    UPROPERTY()
    UNoiseModule* NoiseModule;

    /** Sistema de erosão */
    UPROPERTY()
    UErosionModule* ErosionModule;

    /** Sistema de biomas */
    UPROPERTY()
    UBiomeSystem* BiomeSystem;

    /** Sistema de vegetação */
    UPROPERTY()
    UVegetationSystem* VegetationSystem;

    /** Sistema de água */
    UPROPERTY()
    UWaterComponent* WaterSystem;

    /** Logger do sistema */
    UPROPERTY()
    UPlanetSystemLogger* Logger;

    /** Event bus */
    UPROPERTY()
    UPlanetEventBus* EventBus;

    // === CONFIGURAÇÃO ===
    
    /** Configuração atual de geração */
    UPROPERTY()
    const UPlanetCoreConfig* CurrentConfig;

    // === ESTATÍSTICAS ===
    
    /** Tempo total de geração */
    float TotalGenerationTime;

    /** Número de chunks gerados */
    int32 ChunksGenerated;

    /** Tempo médio por chunk */
    float AverageGenerationTime;

    /** Memória máxima utilizada */
    float MaxMemoryUsage;

    // === CACHE ===
    
    /** Cache de mapas de altura */
    TMap<FVector, TArray<float>> HeightMapCache;

    /** Cache de mapas de biomas */
    TMap<FVector, TArray<EBiomeType>> BiomeMapCache;

    /** Tamanho máximo do cache */
    int32 MaxCacheSize;

    // === UTILITÁRIOS ===
    
    /**
     * Limpa cache antigo
     */
    void CleanupCache();

    /**
     * Calcula hash da posição para cache
     * @param Position - Posição
     * @param LODLevel - Nível de detalhe
     * @return Hash da posição
     */
    FString CalculatePositionHash(const FVector& Position, int32 LODLevel) const;

    /**
     * Valida parâmetros de entrada
     * @param Center - Centro do chunk
     * @param LODLevel - Nível de detalhe
     * @return True se válido
     */
    bool ValidateParameters(const FVector& Center, int32 LODLevel) const;

    /**
     * Loga evento de geração
     * @param EventType - Tipo do evento
     * @param Details - Detalhes do evento
     */
    void LogGenerationEvent(EPlanetEventType EventType, const FString& Details);
}; 