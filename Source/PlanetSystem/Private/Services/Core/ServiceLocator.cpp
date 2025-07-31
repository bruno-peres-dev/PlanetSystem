#include "Services/Core/ServiceLocator.h"
#include "Generation/Noise/NoiseModule.h"
#include "Services/Environment/BiomeSystem.h"
#include "Services/Terrain/ErosionModule.h"
#include "Services/Environment/VegetationSystem.h"
#include "Services/Environment/WaterComponent.h"
#include "Services/Core/EditingSystem.h"
#include "Generation/Terrain/ProceduralPlanet.h"
#include "Configuration/Validators/PlanetConfigValidator.h"
#include "Debug/Logging/PlanetSystemLogger.h"
#include "Engine/Engine.h"

UPlanetSystemServiceLocator* UPlanetSystemServiceLocator::Instance = nullptr;

UPlanetSystemServiceLocator::UPlanetSystemServiceLocator()
{
    if (Instance == nullptr)
    {
        Instance = this;
    }
}

UPlanetSystemServiceLocator* UPlanetSystemServiceLocator::GetInstance()
{
    if (Instance == nullptr)
    {
        Instance = NewObject<UPlanetSystemServiceLocator>();
        Instance->AddToRoot(); // Prevent garbage collection
    }
    return Instance;
}

void UPlanetSystemServiceLocator::InitializeServices(UPlanetCoreConfig* Config)
{
    if (!Config)
    {
        UPlanetSystemLogger::LogWarning(TEXT("ServiceLocator"), TEXT("CoreConfig is null, using default configuration"));
        return;
    }
    
    // Validate configuration before initializing services
    TArray<FPlanetValidationError> ValidationErrors;
    UPlanetConfigValidator::ValidateCoreConfig(Config, ValidationErrors);
    
    if (ValidationErrors.Num() > 0)
    {
        UPlanetSystemLogger::LogWarning(TEXT("ServiceLocator"), 
            FString::Printf(TEXT("Configuration validation found %d issues"), ValidationErrors.Num()));
        
        // Log validation errors
        for (const FPlanetValidationError& Error : ValidationErrors)
        {
            UPlanetSystemLogger::LogWarning(TEXT("ServiceLocator"), 
                FString::Printf(TEXT("Validation Error: %s - %s"), *Error.FieldName, *Error.ErrorMessage));
        }
        
        // Apply auto-fixes if possible
        UPlanetConfigValidator::ApplyAutoFixes(Config, ValidationErrors);
    }
    
    CoreConfig = Config;
    
    // Initialize services with configuration
    if (!NoiseService)
    {
        NoiseService = NewObject<UNoiseModule>();
        NoiseService->SetSeed(Config->NoiseConfig.GlobalSeed);
    }
    
    if (!BiomeService)
    {
        BiomeService = NewObject<UBiomeSystem>();
    }
    
    if (!ErosionService)
    {
        ErosionService = NewObject<UErosionModule>();
    }
    
    if (!VegetationService)
    {
        VegetationService = NewObject<UVegetationSystem>();
    }
    
    if (!WaterService)
    {
        WaterService = NewObject<UWaterComponent>();
    }
    
    if (!EditingService)
    {
        EditingService = NewObject<UEditingSystem>();
    }
    
    UPlanetSystemLogger::LogInfo(TEXT("ServiceLocator"), TEXT("Services initialized successfully"));
}

UNoiseModule* UPlanetSystemServiceLocator::GetNoiseService()
{
    if (Instance && Instance->NoiseService)
    {
        return Instance->NoiseService;
    }
    UE_LOG(LogTemp, Warning, TEXT("PlanetSystem: NoiseService not initialized"));
    return nullptr;
}

UBiomeSystem* UPlanetSystemServiceLocator::GetBiomeService()
{
    if (Instance && Instance->BiomeService)
    {
        return Instance->BiomeService;
    }
    UE_LOG(LogTemp, Warning, TEXT("PlanetSystem: BiomeService not initialized"));
    return nullptr;
}

UErosionModule* UPlanetSystemServiceLocator::GetErosionService()
{
    if (Instance && Instance->ErosionService)
    {
        return Instance->ErosionService;
    }
    UE_LOG(LogTemp, Warning, TEXT("PlanetSystem: ErosionService not initialized"));
    return nullptr;
}

UVegetationSystem* UPlanetSystemServiceLocator::GetVegetationService()
{
    if (Instance && Instance->VegetationService)
    {
        return Instance->VegetationService;
    }
    UE_LOG(LogTemp, Warning, TEXT("PlanetSystem: VegetationService not initialized"));
    return nullptr;
}

UWaterComponent* UPlanetSystemServiceLocator::GetWaterService()
{
    if (Instance && Instance->WaterService)
    {
        return Instance->WaterService;
    }
    UE_LOG(LogTemp, Warning, TEXT("PlanetSystem: WaterService not initialized"));
    return nullptr;
}

UEditingSystem* UPlanetSystemServiceLocator::GetEditingService()
{
    if (Instance && Instance->EditingService)
    {
        return Instance->EditingService;
    }
    UE_LOG(LogTemp, Warning, TEXT("PlanetSystem: EditingService not initialized"));
    return nullptr;
}

void UPlanetSystemServiceLocator::RegisterPlugin(TScriptInterface<IPlanetSystemPlugin> Plugin)
{
    if (Plugin.GetInterface())
    {
        RegisteredPlugins.Add(Plugin);
        UE_LOG(LogTemp, Log, TEXT("PlanetSystem: Plugin registered successfully"));
    }
}

void UPlanetSystemServiceLocator::UnregisterPlugin(TScriptInterface<IPlanetSystemPlugin> Plugin)
{
    RegisteredPlugins.Remove(Plugin);
    UE_LOG(LogTemp, Log, TEXT("PlanetSystem: Plugin unregistered"));
}

void UPlanetSystemServiceLocator::BroadcastPlanetGenerated(AProceduralPlanet* Planet)
{
    // Broadcast event
    BroadcastEventWithParams(EPlanetEventType::PlanetGenerated, TEXT("PlanetGenerated"), 
                            Planet ? Planet->GetName() : TEXT("Unknown"), 0.0f, 0);
    
    // Notify plugins
    for (const auto& Plugin : RegisteredPlugins)
    {
        if (Plugin.GetInterface())
        {
            Plugin.GetInterface()->OnPlanetGenerated(Planet);
        }
    }
}

void UPlanetSystemServiceLocator::BroadcastBiomeCalculated(EBiomeType Biome, const FVector& Location)
{
    // Broadcast event
    FString BiomeName;
    switch (Biome)
    {
        case EBiomeType::Desert: BiomeName = TEXT("Desert"); break;
        case EBiomeType::Plains: BiomeName = TEXT("Plains"); break;
        case EBiomeType::Mountains: BiomeName = TEXT("Mountains"); break;
        case EBiomeType::Forest: BiomeName = TEXT("Forest"); break;
        case EBiomeType::Snow: BiomeName = TEXT("Snow"); break;
        default: BiomeName = TEXT("Unknown"); break;
    }
    
    BroadcastEventWithParams(EPlanetEventType::BiomeCalculated, TEXT("BiomeCalculated"), 
                            BiomeName, Location.Size(), static_cast<int32>(Biome));
    
    // Notify plugins
    for (const auto& Plugin : RegisteredPlugins)
    {
        if (Plugin.GetInterface())
        {
            Plugin.GetInterface()->OnBiomeCalculated(Biome, Location);
        }
    }
}

void UPlanetSystemServiceLocator::BroadcastChunkGenerated(const FVector& Center, int32 LODLevel)
{
    // Broadcast event
    BroadcastEventWithParams(EPlanetEventType::ChunkGenerated, TEXT("ChunkGenerated"), 
                            Center.ToString(), Center.Size(), LODLevel);
    
    // Notify plugins
    for (const auto& Plugin : RegisteredPlugins)
    {
        if (Plugin.GetInterface())
        {
            Plugin.GetInterface()->OnChunkGenerated(Center, LODLevel);
        }
    }
}

void UPlanetSystemServiceLocator::BroadcastErosionApplied(const TArray<FVector>& Vertices, uint32 Seed)
{
    // Broadcast event
    BroadcastEventWithParams(EPlanetEventType::ErosionApplied, TEXT("ErosionApplied"), 
                            FString::Printf(TEXT("Vertices: %d"), Vertices.Num()), 
                            static_cast<float>(Vertices.Num()), static_cast<int32>(Seed));
    
    // Notify plugins
    for (const auto& Plugin : RegisteredPlugins)
    {
        if (Plugin.GetInterface())
        {
            Plugin.GetInterface()->OnErosionApplied(Vertices, Seed);
        }
    }
}

void UPlanetSystemServiceLocator::ShutdownServices()
{
    RegisteredPlugins.Empty();
    
    if (Instance)
    {
        Instance->RemoveFromRoot();
        Instance = nullptr;
    }
    
    UE_LOG(LogTemp, Log, TEXT("PlanetSystem: Services shutdown successfully"));
}

void UPlanetSystemServiceLocator::BeginDestroy()
{
    if (Instance == this)
    {
        Instance = nullptr;
    }
    
    Super::BeginDestroy();
}

void UPlanetSystemServiceLocator::BroadcastEvent(const FPlanetSystemEvent& Event)
{
    // Log the event
    UPlanetSystemLogger::LogEvent(TEXT("ServiceLocator"), Event);
    
    // Broadcast to plugins (future enhancement)
    for (const auto& Plugin : RegisteredPlugins)
    {
        if (Plugin.GetInterface())
        {
            // Plugin event handling could be extended here
        }
    }
}

void UPlanetSystemServiceLocator::BroadcastEventWithParams(EPlanetEventType EventType, const FString& CustomName, 
                                                          const FString& StringParam, float FloatParam, int32 IntParam)
{
    FPlanetSystemEvent Event;
    Event.EventType = EventType;
    Event.CustomName = CustomName;
    Event.StringParam = StringParam;
    Event.FloatParam = FloatParam;
    Event.IntParam = IntParam;
    Event.Timestamp = FDateTime::Now();
    Event.SourceModule = TEXT("ServiceLocator");
    
    BroadcastEvent(Event);
} 