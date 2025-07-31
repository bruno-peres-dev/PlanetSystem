#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProceduralMeshComponent.h"
#include "Configuration/DataAssets/CoreConfig.h"
#include "Common/PlanetTypes.h"
#include "PlanetMeshGenerator.generated.h"

// Forward declarations
class UPlanetSystemLogger;
class UPlanetEventBus;
class FPlanetChunk;

/**
 * Sistema de geração de mesh procedural para planetas
 * Implementa algoritmos de mesh AAA com LOD e otimizações
 */
UCLASS(Blueprintable, BlueprintType)
class PLANETSYSTEM_API UPlanetMeshGenerator : public UObject
{
    GENERATED_BODY()

public:
    UPlanetMeshGenerator();

    // === GERAÇÃO DE MESH ===
    
    /**
     * Gera mesh para um chunk de planeta
     * @param Chunk - Chunk com dados de terreno
     * @param MeshComponent - Componente de mesh a ser preenchido
     */
    UFUNCTION(BlueprintCallable, Category="Mesh Generation")
    void GeneratePlanetMesh(const FPlanetChunk& Chunk, UProceduralMeshComponent* MeshComponent);

    /**
     * Gera mesh com LOD específico
     * @param Chunk - Chunk com dados de terreno
     * @param LODLevel - Nível de detalhe
     * @param MeshComponent - Componente de mesh a ser preenchido
     */
    UFUNCTION(BlueprintCallable, Category="Mesh Generation")
    void GenerateLODMesh(const FPlanetChunk& Chunk, int32 LODLevel, UProceduralMeshComponent* MeshComponent);

    /**
     * Otimiza mesh existente
     * @param Vertices - Array de vértices
     * @param Triangles - Array de triângulos
     * @param UVs - Array de coordenadas UV
     */
    UFUNCTION(BlueprintCallable, Category="Mesh Generation")
    void OptimizeMesh(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector2D>& UVs);

    /**
     * Gera mesh esférico base
     * @param Radius - Raio da esfera
     * @param Resolution - Resolução da esfera
     * @param OutVertices - Vértices gerados
     * @param OutTriangles - Triângulos gerados
     */
    UFUNCTION(BlueprintCallable, Category="Mesh Generation")
    void GenerateSphereMesh(float Radius, int32 Resolution, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles);

    // === CONFIGURAÇÃO ===
    
    /**
     * Define configuração de mesh
     * @param Config - Nova configuração
     */
    UFUNCTION(BlueprintCallable, Category="Mesh Generation")
    void SetMeshConfig(const UPlanetCoreConfig* Config);

    /**
     * Obtém configuração atual
     * @return Configuração atual
     */
    UFUNCTION(BlueprintCallable, Category="Mesh Generation")
    const UPlanetCoreConfig* GetMeshConfig() const;

    // === ESTATÍSTICAS ===
    
    /**
     * Obtém estatísticas de geração de mesh
     * @param OutStats - Estatísticas formatadas
     */
    UFUNCTION(BlueprintCallable, Category="Mesh Generation")
    void GetMeshStats(FString& OutStats) const;

    /**
     * Reseta estatísticas
     */
    UFUNCTION(BlueprintCallable, Category="Mesh Generation")
    void ResetStats();

protected:
    // === ALGORITMOS DE MESH ===
    
    /**
     * Aplica mapa de altura aos vértices
     * @param HeightMap - Mapa de altura
     * @param Vertices - Vértices a serem modificados
     */
    void ApplyHeightMap(const TArray<float>& HeightMap, TArray<FVector>& Vertices);

    /**
     * Gera coordenadas UV para os vértices
     * @param Vertices - Vértices base
     * @param OutUVs - Coordenadas UV geradas
     */
    void GenerateUVs(const TArray<FVector>& Vertices, TArray<FVector2D>& OutUVs);

    /**
     * Gera normais para os vértices
     * @param Vertices - Vértices base
     * @param Triangles - Triângulos
     * @param OutNormals - Normais geradas
     */
    void GenerateNormals(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, TArray<FVector>& OutNormals);

    /**
     * Calcula tangentes para os vértices
     * @param Vertices - Vértices base
     * @param UVs - Coordenadas UV
     * @param Triangles - Triângulos
     * @param OutTangents - Tangentes calculadas
     */
    void CalculateTangents(const TArray<FVector>& Vertices, const TArray<FVector2D>& UVs, const TArray<int32>& Triangles, TArray<FProcMeshTangent>& OutTangents);

    /**
     * Aplica LOD aos vértices
     * @param Vertices - Vértices originais
     * @param LODLevel - Nível de detalhe
     * @param OutVertices - Vértices com LOD aplicado
     */
    void ApplyLOD(const TArray<FVector>& Vertices, int32 LODLevel, TArray<FVector>& OutVertices);

    /**
     * Simplifica mesh usando algoritmo de decimação
     * @param Vertices - Vértices originais
     * @param Triangles - Triângulos originais
     * @param TargetTriangleCount - Número alvo de triângulos
     * @param OutVertices - Vértices simplificados
     * @param OutTriangles - Triângulos simplificados
     */
    void SimplifyMesh(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, int32 TargetTriangleCount, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles);

private:
    // === COMPONENTES ===
    
    /** Logger do sistema */
    UPROPERTY()
    UPlanetSystemLogger* Logger;

    /** Event bus */
    UPROPERTY()
    UPlanetEventBus* EventBus;

    // === CONFIGURAÇÃO ===
    
    /** Configuração atual de mesh */
    UPROPERTY()
    const UPlanetCoreConfig* CurrentConfig;

    // === ESTATÍSTICAS ===
    
    /** Tempo total de geração de mesh */
    float TotalMeshGenerationTime;

    /** Número de meshes gerados */
    int32 MeshesGenerated;

    /** Tempo médio por mesh */
    float AverageMeshGenerationTime;

    /** Vértices totais gerados */
    int32 TotalVerticesGenerated;

    /** Triângulos totais gerados */
    int32 TotalTrianglesGenerated;

    // === CACHE ===
    
    /** Cache de meshes esféricos */
    TMap<int32, TArray<FVector>> SphereMeshCache;

    /** Cache de triângulos esféricos */
    TMap<int32, TArray<int32>> SphereTriangleCache;

    /** Tamanho máximo do cache */
    int32 MaxCacheSize;

    // === UTILITÁRIOS ===
    
    /**
     * Limpa cache antigo
     */
    void CleanupCache();

    /**
     * Valida parâmetros de entrada
     * @param Chunk - Chunk a ser validado
     * @param MeshComponent - Componente de mesh a ser validado
     * @return True se válido
     */
    bool ValidateParameters(const FPlanetChunk& Chunk, UProceduralMeshComponent* MeshComponent) const;

    /**
     * Loga evento de geração de mesh
     * @param EventType - Tipo do evento
     * @param Details - Detalhes do evento
     */
    void LogMeshEvent(EPlanetEventType EventType, const FString& Details);

    /**
     * Calcula qualidade do mesh baseada no LOD
     * @param LODLevel - Nível de detalhe
     * @return Qualidade (0-1)
     */
    float CalculateMeshQuality(int32 LODLevel) const;

    /**
     * Aplica transformações de vértice
     * @param Vertices - Vértices a serem transformados
     * @param Transform - Transformação a ser aplicada
     */
    void ApplyVertexTransform(TArray<FVector>& Vertices, const FTransform& Transform);
}; 