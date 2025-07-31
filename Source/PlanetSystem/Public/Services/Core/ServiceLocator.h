#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Configuration/DataAssets/CoreConfig.h"
#include "Core/Events/PlanetSystemEvents.h"
#include "ServiceLocator.generated.h"

class UNoiseModule;
class UBiomeSystem;
class UErosionModule;
class UVegetationSystem;
class UWaterComponent;
class UEditingSystem;

UINTERFACE(MinimalAPI)
class UPlanetSystemPlugin : public UInterface
{
    GENERATED_BODY()
};

class IPlanetSystemPlugin
{
    GENERATED_BODY()
public:
    virtual void OnPlanetGenerated(class AProceduralPlanet* Planet) = 0;
    virtual void OnBiomeCalculated(EBiomeType Biome, const FVector& Location) = 0;
    virtual void OnChunkGenerated(const FVector& Center, int32 LODLevel) = 0;
    virtual void OnErosionApplied(const TArray<FVector>& Vertices, uint32 Seed) = 0;
};

UCLASS(Blueprintable, ClassGroup=(Procedural))
class PLANETSYSTEM_API UPlanetSystemServiceLocator : public UObject
{
    GENERATED_BODY()
    
private:
    static UPlanetSystemServiceLocator* Instance;
    
    UPROPERTY()
    UPlanetCoreConfig* CoreConfig = nullptr;
    
    UPROPERTY()
    UNoiseModule* NoiseService = nullptr;
    
    UPROPERTY()
    UBiomeSystem* BiomeService = nullptr;
    
    UPROPERTY()
    UErosionModule* ErosionService = nullptr;
    
    UPROPERTY()
    UVegetationSystem* VegetationService = nullptr;
    
    UPROPERTY()
    UWaterComponent* WaterService = nullptr;
    
    UPROPERTY()
    UEditingSystem* EditingService = nullptr;
    
    TArray<TScriptInterface<IPlanetSystemPlugin>> RegisteredPlugins;
    
public:
    UPlanetSystemServiceLocator();
    
    // Singleton access
    static UPlanetSystemServiceLocator* GetInstance();
    
    // Configuration management
    UFUNCTION(BlueprintCallable, Category="ServiceLocator")
    void InitializeServices(UPlanetCoreConfig* Config);
    
    UFUNCTION(BlueprintCallable, Category="ServiceLocator")
    UPlanetCoreConfig* GetCoreConfig() const { return CoreConfig; }
    
    // Service accessors
    UFUNCTION(BlueprintCallable, Category="ServiceLocator")
    static UNoiseModule* GetNoiseService();
    
    UFUNCTION(BlueprintCallable, Category="ServiceLocator")
    static UBiomeSystem* GetBiomeService();
    
    UFUNCTION(BlueprintCallable, Category="ServiceLocator")
    static UErosionModule* GetErosionService();
    
    UFUNCTION(BlueprintCallable, Category="ServiceLocator")
    static UVegetationSystem* GetVegetationService();
    
    UFUNCTION(BlueprintCallable, Category="ServiceLocator")
    static UWaterComponent* GetWaterService();
    
    UFUNCTION(BlueprintCallable, Category="ServiceLocator")
    static UEditingSystem* GetEditingService();
    
    // Plugin system
    UFUNCTION(BlueprintCallable, Category="ServiceLocator")
    void RegisterPlugin(TScriptInterface<IPlanetSystemPlugin> Plugin);
    
    UFUNCTION(BlueprintCallable, Category="ServiceLocator")
    void UnregisterPlugin(TScriptInterface<IPlanetSystemPlugin> Plugin);
    
    // Event broadcasting
    void BroadcastPlanetGenerated(class AProceduralPlanet* Planet);
    void BroadcastBiomeCalculated(EBiomeType Biome, const FVector& Location);
    void BroadcastChunkGenerated(const FVector& Center, int32 LODLevel);
    void BroadcastErosionApplied(const TArray<FVector>& Vertices, uint32 Seed);
    
    // Event system
    UFUNCTION(BlueprintCallable, Category="ServiceLocator")
    void BroadcastEvent(const FPlanetSystemEvent& Event);
    
    UFUNCTION(BlueprintCallable, Category="ServiceLocator")
    void BroadcastEventWithParams(EPlanetEventType EventType, const FString& CustomName = TEXT(""), 
                                  const FString& StringParam = TEXT(""), float FloatParam = 0.0f, 
                                  int32 IntParam = 0);
    
    // Cleanup
    UFUNCTION(BlueprintCallable, Category="ServiceLocator")
    void ShutdownServices();
    
protected:
    virtual void BeginDestroy() override;
}; 