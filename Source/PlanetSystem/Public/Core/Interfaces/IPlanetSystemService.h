#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CoreConfig.h"
#include "IPlanetSystemService.generated.h"

UINTERFACE(MinimalAPI)
class UPlanetSystemService : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface base para todos os serviços do PlanetSystem
 * Segue o padrão AAA data-driven inspirado na Source2
 */
class PLANETSYSTEM_API IPlanetSystemService
{
    GENERATED_BODY()
    
public:
    /**
     * Inicializa o serviço com a configuração fornecida
     * @param Config Configuração do sistema planetário
     */
    virtual void Initialize(const UPlanetCoreConfig* Config) = 0;
    
    /**
     * Finaliza o serviço e libera recursos
     */
    virtual void Shutdown() = 0;
    
    /**
     * Verifica se o serviço está inicializado
     * @return true se inicializado, false caso contrário
     */
    virtual bool IsInitialized() const = 0;
    
    /**
     * Retorna o nome do serviço para identificação
     * @return Nome do serviço
     */
    virtual FString GetServiceName() const = 0;
    
    /**
     * Atualiza o serviço a cada frame
     * @param DeltaTime Tempo desde o último frame
     */
    virtual void Tick(float DeltaTime) = 0;
    
    /**
     * Verifica se o serviço está ativo e funcionando
     * @return true se ativo, false caso contrário
     */
    virtual bool IsActive() const = 0;
    
    /**
     * Retorna a versão do serviço
     * @return Versão do serviço
     */
    virtual FString GetServiceVersion() const = 0;
    
    /**
     * Obtém estatísticas de performance do serviço
     * @param OutStats Estrutura para armazenar as estatísticas
     */
    virtual void GetPerformanceStats(FString& OutStats) const = 0;
}; 