#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PlanetPerformanceProfiler.generated.h"

// Forward declarations
class UObject;

/**
 * Sistema de profiling para monitoramento de performance do PlanetSystem
 * Segue o padrão AAA data-driven inspirado na Source2
 */
UCLASS(BlueprintType, Blueprintable)
class PLANETSYSTEM_API UPlanetPerformanceProfiler : public UObject
{
    GENERATED_BODY()
    
public:
    UPlanetPerformanceProfiler();
    
    /**
     * Obtém a instância singleton do Profiler
     * @return Instância do Profiler
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPerformanceProfiler", CallInEditor=true)
    static UPlanetPerformanceProfiler* GetInstance();
    
    /**
     * Inicia o profiling de uma seção
     * @param SectionName Nome da seção
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPerformanceProfiler")
    void BeginSection(const FString& SectionName);
    
    /**
     * Finaliza o profiling de uma seção
     * @param SectionName Nome da seção
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPerformanceProfiler")
    void EndSection(const FString& SectionName);
    
    /**
     * Obtém estatísticas de performance
     * @param OutStats String com estatísticas
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPerformanceProfiler")
    void GetPerformanceStats(FString& OutStats) const;
    
    /**
     * Habilita/desabilita o profiling
     * @param bEnable true para habilitar
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPerformanceProfiler")
    void SetProfilingEnabled(bool bEnable);
    
    /**
     * Verifica se o profiling está habilitado
     * @return true se habilitado
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPerformanceProfiler")
    bool IsProfilingEnabled() const;
    
    /**
     * Limpa todas as estatísticas
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPerformanceProfiler")
    void ClearStats();
    
    /**
     * Exporta estatísticas para arquivo
     * @param FileName Nome do arquivo
     * @return true se exportou com sucesso
     */
    UFUNCTION(BlueprintCallable, Category="PlanetPerformanceProfiler")
    bool ExportStats(const FString& FileName);
    
private:
    // Instância singleton
    static UPlanetPerformanceProfiler* Instance;
    
    // Estrutura para estatísticas de seção
    struct FSectionStats
    {
        float TotalTime = 0.0f;
        int32 CallCount = 0;
        float MinTime = FLT_MAX;
        float MaxTime = 0.0f;
        float LastStartTime = 0.0f;
        bool bIsActive = false;
    };
    
    // Mapa de estatísticas por seção
    UPROPERTY()
    TMap<FString, FSectionStats> SectionStats;
    
    // Configurações
    UPROPERTY()
    bool bProfilingEnabled = true;
    
    UPROPERTY()
    float ProfilingStartTime = 0.0f;
}; 