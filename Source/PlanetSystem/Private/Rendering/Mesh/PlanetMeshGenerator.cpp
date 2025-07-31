#include "Rendering/Mesh/PlanetMeshGenerator.h"
#include "Debug/Logging/PlanetSystemLogger.h"
#include "Core/Events/PlanetEventBus.h"
#include "Configuration/DataAssets/CoreConfig.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Math/UnrealMathUtility.h"

UPlanetMeshGenerator::UPlanetMeshGenerator()
{
    // Obter serviços do ServiceLocator
    Logger = UPlanetSystemLogger::GetInstance();
    EventBus = UPlanetEventBus::GetInstance();
    
    // Inicializar estatísticas
    TotalMeshGenerationTime = 0.0f;
    MeshesGenerated = 0;
    AverageMeshGenerationTime = 0.0f;
    TotalVerticesGenerated = 0;
    TotalTrianglesGenerated = 0;
    MaxCacheSize = 50;
    
    // Log de inicialização
    if (Logger)
    {
        Logger->LogInfo(TEXT("PlanetMeshGenerator"), TEXT("Sistema de geração de mesh inicializado"));
    }
}

void UPlanetMeshGenerator::GeneratePlanetMesh(const FPlanetChunk& Chunk, UProceduralMeshComponent* MeshComponent)
{
    const double StartTime = FPlatformTime::Seconds();
    
    try
    {
        // Validar parâmetros
        if (!ValidateParameters(Chunk, MeshComponent))
        {
            LogMeshEvent(EPlanetEventType::Error, TEXT("Parâmetros inválidos para geração de mesh"));
            return;
        }
        
        // Log de início
        LogMeshEvent(EPlanetEventType::Info, FString::Printf(TEXT("Iniciando geração de mesh para chunk em %s"), *Chunk.Center.ToString()));
        
        // 1. Gerar mesh esférico base
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        const float Radius = CurrentConfig ? CurrentConfig->GenerationConfig.PlanetRadius : 1000.0f;
        const int32 Resolution = CurrentConfig ? CurrentConfig->GenerationConfig.BaseResolution : 64;
        
        GenerateSphereMesh(Radius, Resolution, Vertices, Triangles);
        
        // 2. Aplicar mapa de altura se disponível
        if (Chunk.HeightMap.Num() > 0)
        {
            ApplyHeightMap(Chunk.HeightMap, Vertices);
        }
        
        // 3. Gerar coordenadas UV
        TArray<FVector2D> UVs;
        GenerateUVs(Vertices, UVs);
        
        // 4. Gerar normais
        TArray<FVector> Normals;
        GenerateNormals(Vertices, Triangles, Normals);
        
        // 5. Calcular tangentes
        TArray<FProcMeshTangent> Tangents;
        CalculateTangents(Vertices, UVs, Triangles, Tangents);
        
        // 6. Aplicar ao componente de mesh
        MeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), Tangents, true);
        
        // Calcular estatísticas
        const double EndTime = FPlatformTime::Seconds();
        const float GenerationTime = static_cast<float>(EndTime - StartTime);
        
        TotalMeshGenerationTime += GenerationTime;
        MeshesGenerated++;
        AverageMeshGenerationTime = TotalMeshGenerationTime / MeshesGenerated;
        TotalVerticesGenerated += Vertices.Num();
        TotalTrianglesGenerated += Triangles.Num() / 3;
        
        // Log de sucesso
        LogMeshEvent(EPlanetEventType::Success, FString::Printf(TEXT("Mesh gerado com sucesso: %d vértices, %d triângulos em %.3fms"), 
            Vertices.Num(), Triangles.Num() / 3, GenerationTime * 1000.0f));
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro na geração de mesh: %s"), UTF8_TO_TCHAR(e.what()));
        LogMeshEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

void UPlanetMeshGenerator::GenerateLODMesh(const FPlanetChunk& Chunk, int32 LODLevel, UProceduralMeshComponent* MeshComponent)
{
    try
    {
        // Gerar mesh base
        GeneratePlanetMesh(Chunk, MeshComponent);
        
        // Aplicar LOD se necessário
        if (LODLevel > 0)
        {
            // Obter dados do mesh atual
            TArray<FVector> Vertices;
            TArray<int32> Triangles;
            TArray<FVector> Normals;
            TArray<FVector2D> UVs;
            TArray<FColor> VertexColors;
            TArray<FProcMeshTangent> Tangents;
            
            MeshComponent->GetMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents);
            
            // Aplicar LOD
            TArray<FVector> LODVertices;
            ApplyLOD(Vertices, LODLevel, LODVertices);
            
            // Simplificar mesh
            TArray<FVector> SimplifiedVertices;
            TArray<int32> SimplifiedTriangles;
            const int32 TargetTriangles = FMath::Max(100, Triangles.Num() / (1 << LODLevel));
            SimplifyMesh(Vertices, Triangles, TargetTriangles, SimplifiedVertices, SimplifiedTriangles);
            
            // Atualizar mesh
            MeshComponent->UpdateMeshSection(0, SimplifiedVertices, SimplifiedTriangles, Normals, UVs, VertexColors, Tangents);
            
            LogMeshEvent(EPlanetEventType::Info, FString::Printf(TEXT("LOD %d aplicado: %d triângulos"), LODLevel, SimplifiedTriangles.Num() / 3));
        }
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro na geração de LOD mesh: %s"), UTF8_TO_TCHAR(e.what()));
        LogMeshEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

void UPlanetMeshGenerator::OptimizeMesh(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector2D>& UVs)
{
    try
    {
        // 1. Remover vértices duplicados
        TArray<FVector> OptimizedVertices;
        TArray<int32> OptimizedTriangles;
        TArray<FVector2D> OptimizedUVs;
        
        TMap<FVector, int32> VertexMap;
        
        for (int32 i = 0; i < Triangles.Num(); i += 3)
        {
            TArray<int32> NewTriangle;
            
            for (int32 j = 0; j < 3; j++)
            {
                const int32 OriginalIndex = Triangles[i + j];
                const FVector& Vertex = Vertices[OriginalIndex];
                
                // Verificar se vértice já existe
                int32* ExistingIndex = VertexMap.Find(Vertex);
                if (ExistingIndex)
                {
                    NewTriangle.Add(*ExistingIndex);
                }
                else
                {
                    const int32 NewIndex = OptimizedVertices.Num();
                    OptimizedVertices.Add(Vertex);
                    OptimizedUVs.Add(UVs[OriginalIndex]);
                    VertexMap.Add(Vertex, NewIndex);
                    NewTriangle.Add(NewIndex);
                }
            }
            
            OptimizedTriangles.Append(NewTriangle);
        }
        
        // 2. Aplicar otimizações
        Vertices = OptimizedVertices;
        Triangles = OptimizedTriangles;
        UVs = OptimizedUVs;
        
        LogMeshEvent(EPlanetEventType::Info, FString::Printf(TEXT("Mesh otimizado: %d -> %d vértices"), 
            OptimizedVertices.Num() + VertexMap.Num(), OptimizedVertices.Num()));
    }
    catch (const std::exception& e)
    {
        const FString ErrorMsg = FString::Printf(TEXT("Erro na otimização de mesh: %s"), UTF8_TO_TCHAR(e.what()));
        LogMeshEvent(EPlanetEventType::Error, ErrorMsg);
    }
}

void UPlanetMeshGenerator::GenerateSphereMesh(float Radius, int32 Resolution, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles)
{
    // Verificar cache primeiro
    if (SphereMeshCache.Contains(Resolution))
    {
        OutVertices = SphereMeshCache[Resolution];
        OutTriangles = SphereTriangleCache[Resolution];
        return;
    }
    
    // Gerar esfera usando coordenadas esféricas
    const int32 LatitudeSegments = Resolution;
    const int32 LongitudeSegments = Resolution * 2;
    
    OutVertices.Empty();
    OutTriangles.Empty();
    
    // Gerar vértices
    for (int32 Lat = 0; Lat <= LatitudeSegments; Lat++)
    {
        const float Latitude = FMath::PI * Lat / LatitudeSegments;
        const float Y = FMath::Cos(Latitude);
        const float R = FMath::Sin(Latitude);
        
        for (int32 Lon = 0; Lon <= LongitudeSegments; Lon++)
        {
            const float Longitude = 2.0f * FMath::PI * Lon / LongitudeSegments;
            const float X = R * FMath::Cos(Longitude);
            const float Z = R * FMath::Sin(Longitude);
            
            OutVertices.Add(FVector(X, Y, Z) * Radius);
        }
    }
    
    // Gerar triângulos
    for (int32 Lat = 0; Lat < LatitudeSegments; Lat++)
    {
        for (int32 Lon = 0; Lon < LongitudeSegments; Lon++)
        {
            const int32 Current = Lat * (LongitudeSegments + 1) + Lon;
            const int32 Next = Current + LongitudeSegments + 1;
            
            // Primeiro triângulo
            OutTriangles.Add(Current);
            OutTriangles.Add(Next);
            OutTriangles.Add(Current + 1);
            
            // Segundo triângulo
            OutTriangles.Add(Next);
            OutTriangles.Add(Next + 1);
            OutTriangles.Add(Current + 1);
        }
    }
    
    // Armazenar no cache
    SphereMeshCache.Add(Resolution, OutVertices);
    SphereTriangleCache.Add(Resolution, OutTriangles);
    
    // Limpar cache se necessário
    CleanupCache();
}

void UPlanetMeshGenerator::SetMeshConfig(const UPlanetCoreConfig* Config)
{
    CurrentConfig = Config;
    LogMeshEvent(EPlanetEventType::Info, TEXT("Configuração de mesh atualizada"));
}

const UPlanetCoreConfig* UPlanetMeshGenerator::GetMeshConfig() const
{
    return CurrentConfig;
}

void UPlanetMeshGenerator::GetMeshStats(FString& OutStats) const
{
    OutStats = FString::Printf(
        TEXT("=== Estatísticas de Mesh ===\n")
        TEXT("Meshes Gerados: %d\n")
        TEXT("Tempo Total: %.3fms\n")
        TEXT("Tempo Médio: %.3fms\n")
        TEXT("Vértices Totais: %d\n")
        TEXT("Triângulos Totais: %d\n")
        TEXT("Cache Size: %d/%d\n"),
        MeshesGenerated,
        TotalMeshGenerationTime * 1000.0f,
        AverageMeshGenerationTime * 1000.0f,
        TotalVerticesGenerated,
        TotalTrianglesGenerated,
        SphereMeshCache.Num(),
        MaxCacheSize
    );
}

void UPlanetMeshGenerator::ResetStats()
{
    TotalMeshGenerationTime = 0.0f;
    MeshesGenerated = 0;
    AverageMeshGenerationTime = 0.0f;
    TotalVerticesGenerated = 0;
    TotalTrianglesGenerated = 0;
    
    LogMeshEvent(EPlanetEventType::Info, TEXT("Estatísticas de mesh resetadas"));
}

void UPlanetMeshGenerator::ApplyHeightMap(const TArray<float>& HeightMap, TArray<FVector>& Vertices)
{
    if (HeightMap.Num() == 0 || Vertices.Num() == 0)
    {
        return;
    }
    
    const int32 HeightMapSize = FMath::Sqrt(static_cast<float>(HeightMap.Num()));
    
    for (int32 i = 0; i < Vertices.Num(); i++)
    {
        FVector& Vertex = Vertices[i];
        
        // Normalizar posição para coordenadas de altura
        const FVector NormalizedPos = Vertex.GetSafeNormal();
        const float U = (FMath::Atan2(NormalizedPos.Z, NormalizedPos.X) + FMath::PI) / (2.0f * FMath::PI);
        const float V = (FMath::Asin(NormalizedPos.Y) + FMath::PI / 2.0f) / FMath::PI;
        
        // Calcular índice no mapa de altura
        const int32 X = FMath::Clamp(FMath::FloorToInt(U * (HeightMapSize - 1)), 0, HeightMapSize - 1);
        const int32 Y = FMath::Clamp(FMath::FloorToInt(V * (HeightMapSize - 1)), 0, HeightMapSize - 1);
        const int32 Index = Y * HeightMapSize + X;
        
        // Aplicar altura
        if (Index < HeightMap.Num())
        {
            const float Height = HeightMap[Index];
            Vertex = Vertex.GetSafeNormal() * (Vertex.Size() + Height);
        }
    }
}

void UPlanetMeshGenerator::GenerateUVs(const TArray<FVector>& Vertices, TArray<FVector2D>& OutUVs)
{
    OutUVs.Empty();
    OutUVs.SetNum(Vertices.Num());
    
    for (int32 i = 0; i < Vertices.Num(); i++)
    {
        const FVector& Vertex = Vertices[i];
        const FVector Normalized = Vertex.GetSafeNormal();
        
        // Coordenadas esféricas para UV
        const float U = (FMath::Atan2(Normalized.Z, Normalized.X) + FMath::PI) / (2.0f * FMath::PI);
        const float V = (FMath::Asin(Normalized.Y) + FMath::PI / 2.0f) / FMath::PI;
        
        OutUVs[i] = FVector2D(U, V);
    }
}

void UPlanetMeshGenerator::GenerateNormals(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, TArray<FVector>& OutNormals)
{
    OutNormals.Empty();
    OutNormals.SetNum(Vertices.Num());
    
    // Inicializar normais
    for (int32 i = 0; i < OutNormals.Num(); i++)
    {
        OutNormals[i] = FVector::ZeroVector;
    }
    
    // Calcular normais dos triângulos
    for (int32 i = 0; i < Triangles.Num(); i += 3)
    {
        const FVector& V0 = Vertices[Triangles[i]];
        const FVector& V1 = Vertices[Triangles[i + 1]];
        const FVector& V2 = Vertices[Triangles[i + 2]];
        
        const FVector Edge1 = V1 - V0;
        const FVector Edge2 = V2 - V0;
        const FVector Normal = FVector::CrossProduct(Edge1, Edge2).GetSafeNormal();
        
        // Adicionar normal aos vértices
        OutNormals[Triangles[i]] += Normal;
        OutNormals[Triangles[i + 1]] += Normal;
        OutNormals[Triangles[i + 2]] += Normal;
    }
    
    // Normalizar
    for (int32 i = 0; i < OutNormals.Num(); i++)
    {
        OutNormals[i] = OutNormals[i].GetSafeNormal();
    }
}

void UPlanetMeshGenerator::CalculateTangents(const TArray<FVector>& Vertices, const TArray<FVector2D>& UVs, const TArray<int32>& Triangles, TArray<FProcMeshTangent>& OutTangents)
{
    OutTangents.Empty();
    OutTangents.SetNum(Vertices.Num());
    
    // Inicializar tangentes
    for (int32 i = 0; i < OutTangents.Num(); i++)
    {
        OutTangents[i] = FProcMeshTangent(FVector::ZeroVector, false);
    }
    
    // Calcular tangentes dos triângulos
    for (int32 i = 0; i < Triangles.Num(); i += 3)
    {
        const FVector& V0 = Vertices[Triangles[i]];
        const FVector& V1 = Vertices[Triangles[i + 1]];
        const FVector& V2 = Vertices[Triangles[i + 2]];
        
        const FVector2D& UV0 = UVs[Triangles[i]];
        const FVector2D& UV1 = UVs[Triangles[i + 1]];
        const FVector2D& UV2 = UVs[Triangles[i + 2]];
        
        const FVector Edge1 = V1 - V0;
        const FVector Edge2 = V2 - V0;
        const FVector2D UVEdge1 = UV1 - UV0;
        const FVector2D UVEdge2 = UV2 - UV0;
        
        const float Det = UVEdge1.X * UVEdge2.Y - UVEdge2.X * UVEdge1.Y;
        const float InvDet = FMath::Abs(Det) > SMALL_NUMBER ? 1.0f / Det : 0.0f;
        
        const FVector Tangent = (Edge1 * UVEdge2.Y - Edge2 * UVEdge1.Y) * InvDet;
        
        // Adicionar tangente aos vértices
        OutTangents[Triangles[i]].TangentX += Tangent;
        OutTangents[Triangles[i + 1]].TangentX += Tangent;
        OutTangents[Triangles[i + 2]].TangentX += Tangent;
    }
    
    // Normalizar
    for (int32 i = 0; i < OutTangents.Num(); i++)
    {
        OutTangents[i].TangentX = OutTangents[i].TangentX.GetSafeNormal();
    }
}

void UPlanetMeshGenerator::ApplyLOD(const TArray<FVector>& Vertices, int32 LODLevel, TArray<FVector>& OutVertices)
{
    if (LODLevel <= 0)
    {
        OutVertices = Vertices;
        return;
    }
    
    // Simplificar mesh baseado no LOD
    const int32 TargetVertexCount = FMath::Max(100, Vertices.Num() / (1 << LODLevel));
    
    OutVertices.Empty();
    OutVertices.SetNum(TargetVertexCount);
    
    // Algoritmo simples de decimação: pegar vértices espaçados
    const float Step = static_cast<float>(Vertices.Num()) / TargetVertexCount;
    
    for (int32 i = 0; i < TargetVertexCount; i++)
    {
        const int32 SourceIndex = FMath::FloorToInt(i * Step);
        if (SourceIndex < Vertices.Num())
        {
            OutVertices[i] = Vertices[SourceIndex];
        }
    }
}

void UPlanetMeshGenerator::SimplifyMesh(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, int32 TargetTriangleCount, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles)
{
    // Algoritmo simples de simplificação
    // Em uma implementação real, usaríamos algoritmos mais sofisticados como Quadric Error Metrics
    
    if (Triangles.Num() / 3 <= TargetTriangleCount)
    {
        OutVertices = Vertices;
        OutTriangles = Triangles;
        return;
    }
    
    // Simplificação por amostragem
    const int32 Step = FMath::Max(1, Triangles.Num() / (TargetTriangleCount * 3));
    
    OutVertices = Vertices;
    OutTriangles.Empty();
    
    for (int32 i = 0; i < Triangles.Num(); i += Step * 3)
    {
        if (i + 2 < Triangles.Num())
        {
            OutTriangles.Add(Triangles[i]);
            OutTriangles.Add(Triangles[i + 1]);
            OutTriangles.Add(Triangles[i + 2]);
        }
    }
}

void UPlanetMeshGenerator::CleanupCache()
{
    if (SphereMeshCache.Num() > MaxCacheSize)
    {
        const int32 ItemsToRemove = SphereMeshCache.Num() - MaxCacheSize;
        TArray<int32> KeysToRemove;
        
        for (const auto& Pair : SphereMeshCache)
        {
            KeysToRemove.Add(Pair.Key);
            if (KeysToRemove.Num() >= ItemsToRemove)
            {
                break;
            }
        }
        
        for (const int32& Key : KeysToRemove)
        {
            SphereMeshCache.Remove(Key);
            SphereTriangleCache.Remove(Key);
        }
    }
}

bool UPlanetMeshGenerator::ValidateParameters(const FPlanetChunk& Chunk, UProceduralMeshComponent* MeshComponent) const
{
    if (!MeshComponent)
    {
        return false;
    }
    
    if (!FMath::IsFinite(Chunk.Center.X) || !FMath::IsFinite(Chunk.Center.Y) || !FMath::IsFinite(Chunk.Center.Z))
    {
        return false;
    }
    
    return true;
}

void UPlanetMeshGenerator::LogMeshEvent(EPlanetEventType EventType, const FString& Details)
{
    if (Logger)
    {
        Logger->LogInfo(TEXT("PlanetMeshGenerator"), Details);
    }
    
    if (EventBus)
    {
        FPlanetSystemEvent Event;
        Event.EventType = EventType;
        Event.Source = TEXT("PlanetMeshGenerator");
        Event.Details = Details;
        Event.Timestamp = FDateTime::Now();
        
        EventBus->BroadcastEvent(Event);
    }
}

float UPlanetMeshGenerator::CalculateMeshQuality(int32 LODLevel) const
{
    return FMath::Clamp(1.0f - (LODLevel * 0.1f), 0.1f, 1.0f);
}

void UPlanetMeshGenerator::ApplyVertexTransform(TArray<FVector>& Vertices, const FTransform& Transform)
{
    for (FVector& Vertex : Vertices)
    {
        Vertex = Transform.TransformPosition(Vertex);
    }
} 