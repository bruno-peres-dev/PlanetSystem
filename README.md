# PlanetSystem Plugin

PlanetSystem is a runtime plugin for Unreal Engine that provides a fully data-driven solution for procedural planet generation. It features modular services for terrain creation, biomes, vegetation, water and more, all connected through a central `ServiceLocator`. Extensive caching, logging and profiling systems ensure AAA performance, and the architecture allows additional plugins without modifying the core.

## Building the Plugin

1. Place the `PlanetSystem` folder inside your Unreal project's `Plugins` directory.
2. Regenerate project files if needed and build the project normally with the Unreal Editor or Unreal Build Tool.
3. Build settings and module dependencies are defined in `Source/PlanetSystem/PlanetSystem.Build.cs`. Custom compilation macros such as `PLANETSYSTEM_DEBUG` and `PLANETSYSTEM_SHIPPING` are already configured.
4. For a complete list of required modules and configuration options see [PLUGIN_CONFIGURATION.md](PLUGIN_CONFIGURATION.md).

## Example Usage

Create a configuration `DataAsset`, spawn a `ProceduralPlanet` actor and register additional plugins using the `ServiceLocator`:

```cpp
// Load configuration
UPlanetCoreConfig* Config = LoadObject<UPlanetCoreConfig>(nullptr, TEXT("/Game/Configs/PlanetConfig"));

// Spawn planet
AProceduralPlanet* Planet = GetWorld()->SpawnActor<AProceduralPlanet>();
Planet->SetCoreConfig(Config);

// Register example plugin
UExamplePlanetPlugin* Plugin = NewObject<UExamplePlanetPlugin>();
UPlanetSystemServiceLocator::GetInstance()->RegisterPlugin(Plugin);
```

A more complete walkthrough, including logging, events and cache management, can be found in [EXAMPLE_USAGE.md](EXAMPLE_USAGE.md).

## Further Documentation

This repository contains detailed documents describing every system in depth:

- [README_IMPROVEMENTS.md](README_IMPROVEMENTS.md) – overview of the main improvements introduced in this version.
- [STRUCTURE_IMPROVEMENTS.md](STRUCTURE_IMPROVEMENTS.md) – shows the recommended folder organization and architecture.
- [FINAL_COMPLETE_IMPLEMENTATION_SUMMARY.md](FINAL_COMPLETE_IMPLEMENTATION_SUMMARY.md) – summary of all implemented systems.

Consult these files for more information about configuration, features and future plans.
