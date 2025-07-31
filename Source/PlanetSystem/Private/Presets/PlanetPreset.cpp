#include "Presets/PlanetPreset.h"
#include "Debug/Logging/PlanetSystemLogger.h"
#include "Core/Events/PlanetEventBus.h"
#include "Serialization/PlanetDataSerializer.h"
#include "Configuration/Validators/PlanetConfigValidator.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformTime.h"

// Instância singleton
UPlanetPreset* UPlanetPreset::Instance = nullptr;

UPlanetPreset::UPlanetPreset()
{
    UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), TEXT("Preset system created"));
}

UPlanetPreset* UPlanetPreset::GetInstance()
{
    if (!Instance)
    {
        Instance = NewObject<UPlanetPreset>();
        Instance->AddToRoot(); // Previne garbage collection
        UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), TEXT("Preset system singleton created"));
    }
    return Instance;
}

UPlanetPreset* UPlanetPreset::CreatePreset(const FString& PresetName, const FString& Description, 
                                          const FString& Category, const TArray<FString>& Tags, 
                                          UPlanetCoreConfig* Config)
{
    const float StartTime = FPlatformTime::Seconds();
    
    try
    {
        if (!Config)
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), TEXT("Cannot create preset with null configuration"));
            return nullptr;
        }
        
        // Valida a configuração antes de criar o preset
        TArray<FPlanetValidationError> ValidationErrors;
        UPlanetConfigValidator::ValidateCoreConfig(Config, ValidationErrors);
        
        if (ValidationErrors.Num() > 0)
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetPreset"), 
                FString::Printf(TEXT("Creating preset with %d validation warnings"), ValidationErrors.Num()));
        }
        
        // Cria o preset
        UPlanetPreset* NewPreset = NewObject<UPlanetPreset>();
        NewPreset->PresetName = PresetName;
        NewPreset->Description = Description;
        NewPreset->Category = Category;
        NewPreset->Tags = Tags;
        NewPreset->Configuration = Config;
        NewPreset->Author = TEXT("Unknown");
        NewPreset->Version = TEXT("1.0.0");
        NewPreset->CreationDate = FDateTime::Now();
        NewPreset->LastModifiedDate = FDateTime::Now();
        NewPreset->Rating = 0.0f;
        NewPreset->UsageCount = 0;
        NewPreset->bIsPublic = true;
        NewPreset->bIsDefault = false;
        NewPreset->bIsFavorite = false;
        
        // Gera ID único
        NewPreset->PresetID = FGuid::NewGuid().ToString();
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Preset created successfully: %s"), *PresetName));
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::PresetCreated, TEXT("Preset"), PresetName);
        
        return NewPreset;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception creating preset: %s"), UTF8_TO_TCHAR(e.what())));
        return nullptr;
    }
}

bool UPlanetPreset::SavePreset(UPlanetPreset* Preset, const FString& Filename)
{
    const float StartTime = FPlatformTime::Seconds();
    
    try
    {
        if (!Preset)
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), TEXT("Cannot save null preset"));
            return false;
        }
        
        if (!Preset->Configuration)
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), TEXT("Cannot save preset with null configuration"));
            return false;
        }
        
        // Atualiza data de modificação
        Preset->LastModifiedDate = FDateTime::Now();
        
        // Serializa o preset para JSON
        FString JsonString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
        
        Writer->WriteObjectStart();
        
        // Metadados do preset
        Writer->WriteValue(TEXT("PresetName"), Preset->PresetName);
        Writer->WriteValue(TEXT("Description"), Preset->Description);
        Writer->WriteValue(TEXT("Category"), Preset->Category);
        Writer->WriteValue(TEXT("Author"), Preset->Author);
        Writer->WriteValue(TEXT("Version"), Preset->Version);
        Writer->WriteValue(TEXT("PresetID"), Preset->PresetID);
        Writer->WriteValue(TEXT("CreationDate"), Preset->CreationDate.ToString());
        Writer->WriteValue(TEXT("LastModifiedDate"), Preset->LastModifiedDate.ToString());
        Writer->WriteValue(TEXT("Rating"), Preset->Rating);
        Writer->WriteValue(TEXT("UsageCount"), Preset->UsageCount);
        Writer->WriteValue(TEXT("IsPublic"), Preset->bIsPublic);
        Writer->WriteValue(TEXT("IsDefault"), Preset->bIsDefault);
        Writer->WriteValue(TEXT("IsFavorite"), Preset->bIsFavorite);
        
        // Tags
        Writer->WriteArrayStart(TEXT("Tags"));
        for (const FString& Tag : Preset->Tags)
        {
            Writer->WriteValue(Tag);
        }
        Writer->WriteArrayEnd();
        
        // Propriedades customizadas
        Writer->WriteObjectStart(TEXT("CustomProperties"));
        for (const auto& Pair : Preset->CustomProperties)
        {
            Writer->WriteValue(Pair.Key, Pair.Value);
        }
        Writer->WriteObjectEnd();
        
        // Configuração (serializada separadamente)
        Writer->WriteValue(TEXT("ConfigFilename"), Filename + TEXT("_config.json"));
        
        Writer->WriteObjectEnd();
        Writer->Close();
        
        // Salva o preset
        FString PresetPath = GetPresetsDirectory() / Filename;
        if (FFileHelper::SaveStringToFile(JsonString, *PresetPath))
        {
            // Salva a configuração separadamente
            UPlanetDataSerializer::GetInstance()->SaveCoreConfig(Preset->Configuration, Filename + TEXT("_config.json"));
            
            UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), 
                FString::Printf(TEXT("Preset saved successfully: %s"), *Filename));
            
            UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                EPlanetEventType::PresetSaved, TEXT("Preset"), Filename);
            
            return true;
        }
        
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Failed to save preset: %s"), *Filename));
        return false;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception saving preset: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

UPlanetPreset* UPlanetPreset::LoadPreset(const FString& Filename)
{
    const float StartTime = FPlatformTime::Seconds();
    
    try
    {
        FString PresetPath = GetPresetsDirectory() / Filename;
        
        if (!FPaths::FileExists(PresetPath))
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetPreset"), 
                FString::Printf(TEXT("Preset file not found: %s"), *Filename));
            return nullptr;
        }
        
        // Carrega o JSON do preset
        FString JsonString;
        if (!FFileHelper::LoadFileToString(JsonString, *PresetPath))
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
                FString::Printf(TEXT("Failed to load preset file: %s"), *Filename));
            return nullptr;
        }
        
        // Deserializa o JSON
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
        TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
        
        if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
                FString::Printf(TEXT("Failed to deserialize preset: %s"), *Filename));
            return nullptr;
        }
        
        // Cria o preset
        UPlanetPreset* Preset = NewObject<UPlanetPreset>();
        
        // Carrega metadados
        Preset->PresetName = JsonObject->GetStringField(TEXT("PresetName"));
        Preset->Description = JsonObject->GetStringField(TEXT("Description"));
        Preset->Category = JsonObject->GetStringField(TEXT("Category"));
        Preset->Author = JsonObject->GetStringField(TEXT("Author"));
        Preset->Version = JsonObject->GetStringField(TEXT("Version"));
        Preset->PresetID = JsonObject->GetStringField(TEXT("PresetID"));
        Preset->Rating = JsonObject->GetNumberField(TEXT("Rating"));
        Preset->UsageCount = JsonObject->GetIntegerField(TEXT("UsageCount"));
        Preset->bIsPublic = JsonObject->GetBoolField(TEXT("IsPublic"));
        Preset->bIsDefault = JsonObject->GetBoolField(TEXT("IsDefault"));
        Preset->bIsFavorite = JsonObject->GetBoolField(TEXT("IsFavorite"));
        
        // Carrega datas
        FString CreationDateStr = JsonObject->GetStringField(TEXT("CreationDate"));
        FString LastModifiedStr = JsonObject->GetStringField(TEXT("LastModifiedDate"));
        FDateTime::Parse(CreationDateStr, Preset->CreationDate);
        FDateTime::Parse(LastModifiedStr, Preset->LastModifiedDate);
        
        // Carrega tags
        const TArray<TSharedPtr<FJsonValue>>& TagsArray = JsonObject->GetArrayField(TEXT("Tags"));
        for (const TSharedPtr<FJsonValue>& TagValue : TagsArray)
        {
            Preset->Tags.Add(TagValue->AsString());
        }
        
        // Carrega propriedades customizadas
        const TSharedPtr<FJsonObject>& CustomProps = JsonObject->GetObjectField(TEXT("CustomProperties"));
        for (const auto& Pair : CustomProps->Values)
        {
            Preset->CustomProperties.Add(Pair.Key, Pair.Value->AsString());
        }
        
        // Carrega a configuração
        FString ConfigFilename = JsonObject->GetStringField(TEXT("ConfigFilename"));
        Preset->Configuration = UPlanetDataSerializer::GetInstance()->LoadCoreConfig(ConfigFilename);
        
        if (!Preset->Configuration)
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
                FString::Printf(TEXT("Failed to load configuration for preset: %s"), *Filename));
            return nullptr;
        }
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Preset loaded successfully: %s"), *Filename));
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::PresetLoaded, TEXT("Preset"), Filename);
        
        return Preset;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception loading preset: %s"), UTF8_TO_TCHAR(e.what())));
        return nullptr;
    }
}

bool UPlanetPreset::ApplyPreset(UPlanetPreset* Preset, UPlanetCoreConfig* TargetConfig)
{
    try
    {
        if (!Preset || !Preset->Configuration)
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), TEXT("Cannot apply null preset or preset with null configuration"));
            return false;
        }
        
        if (!TargetConfig)
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), TEXT("Cannot apply preset to null target configuration"));
            return false;
        }
        
        // Copia as configurações do preset para o alvo
        TargetConfig->PlanetGenerationConfig = Preset->Configuration->PlanetGenerationConfig;
        TargetConfig->NoiseConfig = Preset->Configuration->NoiseConfig;
        TargetConfig->BiomeConfig = Preset->Configuration->BiomeConfig;
        TargetConfig->bEnableDebugMode = Preset->Configuration->bEnableDebugMode;
        TargetConfig->bEnableProfiling = Preset->Configuration->bEnableProfiling;
        
        // Incrementa contador de uso
        Preset->UsageCount++;
        Preset->LastModifiedDate = FDateTime::Now();
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Preset applied successfully: %s"), *Preset->PresetName));
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::PresetApplied, TEXT("Preset"), Preset->PresetName);
        
        return true;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception applying preset: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

TArray<FString> UPlanetPreset::ListAvailablePresets()
{
    try
    {
        FString PresetsDir = GetPresetsDirectory();
        TArray<FString> PresetFiles;
        
        IFileManager::Get().FindFiles(PresetFiles, *PresetsDir, TEXT("*.json"));
        
        TArray<FString> PresetNames;
        for (const FString& FilePath : PresetFiles)
        {
            FString Filename = FPaths::GetBaseFilename(FilePath);
            if (!Filename.EndsWith(TEXT("_config")))
            {
                PresetNames.Add(Filename);
            }
        }
        
        UPlanetSystemLogger::LogDebug(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Found %d available presets"), PresetNames.Num()));
        
        return PresetNames;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception listing presets: %s"), UTF8_TO_TCHAR(e.what())));
        return TArray<FString>();
    }
}

TArray<FString> UPlanetPreset::SearchPresets(const FString& SearchTerm, const FString& Category, 
                                           const TArray<FString>& Tags, float MinRating)
{
    try
    {
        TArray<FString> AllPresets = ListAvailablePresets();
        TArray<FString> MatchingPresets;
        
        for (const FString& PresetName : AllPresets)
        {
            UPlanetPreset* Preset = LoadPreset(PresetName);
            if (Preset)
            {
                bool bMatches = true;
                
                // Verifica termo de busca
                if (!SearchTerm.IsEmpty())
                {
                    bool bNameMatch = Preset->PresetName.Contains(SearchTerm, ESearchCase::IgnoreCase);
                    bool bDescMatch = Preset->Description.Contains(SearchTerm, ESearchCase::IgnoreCase);
                    if (!bNameMatch && !bDescMatch)
                    {
                        bMatches = false;
                    }
                }
                
                // Verifica categoria
                if (!Category.IsEmpty() && Preset->Category != Category)
                {
                    bMatches = false;
                }
                
                // Verifica tags
                if (Tags.Num() > 0)
                {
                    bool bHasMatchingTag = false;
                    for (const FString& Tag : Tags)
                    {
                        if (Preset->Tags.Contains(Tag))
                        {
                            bHasMatchingTag = true;
                            break;
                        }
                    }
                    if (!bHasMatchingTag)
                    {
                        bMatches = false;
                    }
                }
                
                // Verifica rating mínimo
                if (Preset->Rating < MinRating)
                {
                    bMatches = false;
                }
                
                if (bMatches)
                {
                    MatchingPresets.Add(PresetName);
                }
            }
        }
        
        UPlanetSystemLogger::LogDebug(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Search found %d matching presets"), MatchingPresets.Num()));
        
        return MatchingPresets;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception searching presets: %s"), UTF8_TO_TCHAR(e.what())));
        return TArray<FString>();
    }
}

TArray<FString> UPlanetPreset::GetPresetsByCategory(const FString& Category)
{
    try
    {
        TArray<FString> AllPresets = ListAvailablePresets();
        TArray<FString> CategoryPresets;
        
        for (const FString& PresetName : AllPresets)
        {
            UPlanetPreset* Preset = LoadPreset(PresetName);
            if (Preset && Preset->Category == Category)
            {
                CategoryPresets.Add(PresetName);
            }
        }
        
        UPlanetSystemLogger::LogDebug(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Found %d presets in category: %s"), CategoryPresets.Num(), *Category));
        
        return CategoryPresets;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception getting presets by category: %s"), UTF8_TO_TCHAR(e.what())));
        return TArray<FString>();
    }
}

TArray<FString> UPlanetPreset::GetAvailableCategories()
{
    try
    {
        TArray<FString> AllPresets = ListAvailablePresets();
        TSet<FString> Categories;
        
        for (const FString& PresetName : AllPresets)
        {
            UPlanetPreset* Preset = LoadPreset(PresetName);
            if (Preset && !Preset->Category.IsEmpty())
            {
                Categories.Add(Preset->Category);
            }
        }
        
        TArray<FString> CategoryArray = Categories.Array();
        CategoryArray.Sort();
        
        UPlanetSystemLogger::LogDebug(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Found %d available categories"), CategoryArray.Num()));
        
        return CategoryArray;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception getting available categories: %s"), UTF8_TO_TCHAR(e.what())));
        return TArray<FString>();
    }
}

TArray<FString> UPlanetPreset::GetAvailableTags()
{
    try
    {
        TArray<FString> AllPresets = ListAvailablePresets();
        TSet<FString> AllTags;
        
        for (const FString& PresetName : AllPresets)
        {
            UPlanetPreset* Preset = LoadPreset(PresetName);
            if (Preset)
            {
                for (const FString& Tag : Preset->Tags)
                {
                    if (!Tag.IsEmpty())
                    {
                        AllTags.Add(Tag);
                    }
                }
            }
        }
        
        TArray<FString> TagArray = AllTags.Array();
        TagArray.Sort();
        
        UPlanetSystemLogger::LogDebug(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Found %d available tags"), TagArray.Num()));
        
        return TagArray;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception getting available tags: %s"), UTF8_TO_TCHAR(e.what())));
        return TArray<FString>();
    }
}

bool UPlanetPreset::IsPresetCompatible(UPlanetPreset* Preset, const FString& EngineVersion, 
                                      const FString& PluginVersion)
{
    try
    {
        if (!Preset)
        {
            return false;
        }
        
        // Verifica versão do preset
        FString PresetVersion = Preset->Version;
        
        // Compara versões (implementação simples)
        if (PresetVersion == TEXT("1.0.0") && EngineVersion.StartsWith(TEXT("5.0")))
        {
            return true;
        }
        
        // Verifica propriedades customizadas para compatibilidade
        FString* CompatibleEngine = Preset->CustomProperties.Find(TEXT("CompatibleEngine"));
        if (CompatibleEngine && !CompatibleEngine->IsEmpty())
        {
            if (!EngineVersion.StartsWith(*CompatibleEngine))
            {
                return false;
            }
        }
        
        FString* CompatiblePlugin = Preset->CustomProperties.Find(TEXT("CompatiblePlugin"));
        if (CompatiblePlugin && !CompatiblePlugin->IsEmpty())
        {
            if (!PluginVersion.StartsWith(*CompatiblePlugin))
            {
                return false;
            }
        }
        
        return true;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception checking preset compatibility: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

void UPlanetPreset::GetCompatibilityInfo(UPlanetPreset* Preset, FString& OutInfo)
{
    try
    {
        if (!Preset)
        {
            OutInfo = TEXT("Invalid preset");
            return;
        }
        
        OutInfo = FString::Printf(
            TEXT("Preset Compatibility Info:\n")
            TEXT("- Name: %s\n")
            TEXT("- Version: %s\n")
            TEXT("- Author: %s\n")
            TEXT("- Category: %s\n")
            TEXT("- Rating: %.1f/5.0\n")
            TEXT("- Usage Count: %d\n")
            TEXT("- Creation Date: %s\n")
            TEXT("- Last Modified: %s\n")
            TEXT("- Tags: %s"),
            *Preset->PresetName, *Preset->Version, *Preset->Author, *Preset->Category,
            Preset->Rating, Preset->UsageCount, *Preset->CreationDate.ToString(),
            *Preset->LastModifiedDate.ToString(), *FString::Join(Preset->Tags, TEXT(", "))
        );
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), TEXT("Compatibility info retrieved"));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception getting compatibility info: %s"), UTF8_TO_TCHAR(e.what())));
        OutInfo = TEXT("Error retrieving compatibility info");
    }
}

bool UPlanetPreset::ValidatePreset(UPlanetPreset* Preset, TArray<FPlanetValidationError>& OutErrors)
{
    try
    {
        if (!Preset)
        {
            OutErrors.Add(FPlanetValidationError(EPlanetValidationError::InvalidPreset, TEXT("Preset"), 
                TEXT(""), TEXT("Preset is null"), EPlanetValidationErrorSeverity::Error));
            return false;
        }
        
        bool bIsValid = true;
        
        // Valida nome
        if (Preset->PresetName.IsEmpty())
        {
            OutErrors.Add(FPlanetValidationError(EPlanetValidationError::InvalidPresetName, TEXT("PresetName"), 
                Preset->PresetName, TEXT("Preset name cannot be empty"), EPlanetValidationErrorSeverity::Error));
            bIsValid = false;
        }
        
        // Valida configuração
        if (!Preset->Configuration)
        {
            OutErrors.Add(FPlanetValidationError(EPlanetValidationError::InvalidConfiguration, TEXT("Configuration"), 
                TEXT(""), TEXT("Preset configuration is null"), EPlanetValidationErrorSeverity::Error));
            bIsValid = false;
        }
        else
        {
            // Valida a configuração usando o validador
            TArray<FPlanetValidationError> ConfigErrors;
            UPlanetConfigValidator::ValidateCoreConfig(Preset->Configuration, ConfigErrors);
            OutErrors.Append(ConfigErrors);
            
            if (ConfigErrors.Num() > 0)
            {
                bIsValid = false;
            }
        }
        
        // Valida rating
        if (Preset->Rating < 0.0f || Preset->Rating > 5.0f)
        {
            OutErrors.Add(FPlanetValidationError(EPlanetValidationError::InvalidRating, TEXT("Rating"), 
                FString::Printf(TEXT("%.1f"), Preset->Rating), TEXT("Rating must be between 0.0 and 5.0"), 
                EPlanetValidationErrorSeverity::Warning));
        }
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Preset validation completed: %s (%d errors)"), 
                bIsValid ? TEXT("Valid") : TEXT("Invalid"), OutErrors.Num()));
        
        return bIsValid;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception validating preset: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

void UPlanetPreset::GetPresetStats(UPlanetPreset* Preset, FString& OutStats)
{
    try
    {
        if (!Preset)
        {
            OutStats = TEXT("Invalid preset");
            return;
        }
        
        OutStats = FString::Printf(
            TEXT("Preset Statistics:\n")
            TEXT("- Name: %s\n")
            TEXT("- ID: %s\n")
            TEXT("- Category: %s\n")
            TEXT("- Author: %s\n")
            TEXT("- Version: %s\n")
            TEXT("- Rating: %.1f/5.0\n")
            TEXT("- Usage Count: %d\n")
            TEXT("- Creation Date: %s\n")
            TEXT("- Last Modified: %s\n")
            TEXT("- Tags: %s\n")
            TEXT("- Is Public: %s\n")
            TEXT("- Is Default: %s\n")
            TEXT("- Is Favorite: %s\n")
            TEXT("- Custom Properties: %d"),
            *Preset->PresetName, *Preset->PresetID, *Preset->Category, *Preset->Author,
            *Preset->Version, Preset->Rating, Preset->UsageCount, *Preset->CreationDate.ToString(),
            *Preset->LastModifiedDate.ToString(), *FString::Join(Preset->Tags, TEXT(", ")),
            Preset->bIsPublic ? TEXT("Yes") : TEXT("No"), Preset->bIsDefault ? TEXT("Yes") : TEXT("No"),
            Preset->bIsFavorite ? TEXT("Yes") : TEXT("No"), Preset->CustomProperties.Num()
        );
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), TEXT("Preset statistics retrieved"));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception getting preset stats: %s"), UTF8_TO_TCHAR(e.what())));
        OutStats = TEXT("Error retrieving preset statistics");
    }
}

void UPlanetPreset::ComparePresets(UPlanetPreset* PresetA, UPlanetPreset* PresetB, FString& OutComparison)
{
    try
    {
        if (!PresetA || !PresetB)
        {
            OutComparison = TEXT("Cannot compare null presets");
            return;
        }
        
        OutComparison = FString::Printf(
            TEXT("Preset Comparison:\n")
            TEXT("=== %s vs %s ===\n\n")
            TEXT("Name: %s | %s\n")
            TEXT("Category: %s | %s\n")
            TEXT("Author: %s | %s\n")
            TEXT("Version: %s | %s\n")
            TEXT("Rating: %.1f | %.1f\n")
            TEXT("Usage Count: %d | %d\n")
            TEXT("Creation Date: %s | %s\n")
            TEXT("Last Modified: %s | %s\n")
            TEXT("Tags: %s | %s\n")
            TEXT("Is Public: %s | %s\n")
            TEXT("Is Default: %s | %s\n")
            TEXT("Is Favorite: %s | %s\n")
            TEXT("Custom Properties: %d | %d"),
            *PresetA->PresetName, *PresetB->PresetName,
            *PresetA->PresetName, *PresetB->PresetName,
            *PresetA->Category, *PresetB->Category,
            *PresetA->Author, *PresetB->Author,
            *PresetA->Version, *PresetB->Version,
            PresetA->Rating, PresetB->Rating,
            PresetA->UsageCount, PresetB->UsageCount,
            *PresetA->CreationDate.ToString(), *PresetB->CreationDate.ToString(),
            *PresetA->LastModifiedDate.ToString(), *PresetB->LastModifiedDate.ToString(),
            *FString::Join(PresetA->Tags, TEXT(", ")), *FString::Join(PresetB->Tags, TEXT(", ")),
            PresetA->bIsPublic ? TEXT("Yes") : TEXT("No"), PresetB->bIsPublic ? TEXT("Yes") : TEXT("No"),
            PresetA->bIsDefault ? TEXT("Yes") : TEXT("No"), PresetB->bIsDefault ? TEXT("Yes") : TEXT("No"),
            PresetA->bIsFavorite ? TEXT("Yes") : TEXT("No"), PresetB->bIsFavorite ? TEXT("Yes") : TEXT("No"),
            PresetA->CustomProperties.Num(), PresetB->CustomProperties.Num()
        );
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), TEXT("Preset comparison completed"));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception comparing presets: %s"), UTF8_TO_TCHAR(e.what())));
        OutComparison = TEXT("Error comparing presets");
    }
}

UPlanetPreset* UPlanetPreset::CreateDifferencePreset(UPlanetPreset* PresetA, UPlanetPreset* PresetB, 
                                                     const FString& DifferenceName)
{
    try
    {
        if (!PresetA || !PresetB || !PresetA->Configuration || !PresetB->Configuration)
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), TEXT("Cannot create difference preset with null presets or configurations"));
            return nullptr;
        }
        
        // Cria uma nova configuração com as diferenças
        UPlanetCoreConfig* DiffConfig = NewObject<UPlanetCoreConfig>();
        
        // Compara e aplica diferenças (implementação simplificada)
        // Em uma implementação real, você compararia cada campo individualmente
        
        // Cria o preset de diferença
        UPlanetPreset* DiffPreset = CreatePreset(
            DifferenceName,
            FString::Printf(TEXT("Difference between %s and %s"), *PresetA->PresetName, *PresetB->PresetName),
            TEXT("Differences"),
            TArray<FString>{TEXT("difference"), TEXT("comparison")},
            DiffConfig
        );
        
        if (DiffPreset)
        {
            DiffPreset->CustomProperties.Add(TEXT("SourcePresetA"), PresetA->PresetName);
            DiffPreset->CustomProperties.Add(TEXT("SourcePresetB"), PresetB->PresetName);
            DiffPreset->CustomProperties.Add(TEXT("DifferenceType"), TEXT("Configuration"));
            
            UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), 
                FString::Printf(TEXT("Difference preset created: %s"), *DifferenceName));
        }
        
        return DiffPreset;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception creating difference preset: %s"), UTF8_TO_TCHAR(e.what())));
        return nullptr;
    }
}

UPlanetPreset* UPlanetPreset::MergePresets(UPlanetPreset* PresetA, UPlanetPreset* PresetB, 
                                          const FString& MergedName, float BlendFactor)
{
    try
    {
        if (!PresetA || !PresetB || !PresetA->Configuration || !PresetB->Configuration)
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), TEXT("Cannot merge null presets or presets with null configurations"));
            return nullptr;
        }
        
        if (BlendFactor < 0.0f || BlendFactor > 1.0f)
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), TEXT("Blend factor must be between 0.0 and 1.0"));
            return nullptr;
        }
        
        // Cria uma nova configuração mesclada
        UPlanetCoreConfig* MergedConfig = NewObject<UPlanetCoreConfig>();
        
        // Mescla as configurações (implementação simplificada)
        // Em uma implementação real, você interpolaria cada campo individualmente
        
        // Cria o preset mesclado
        UPlanetPreset* MergedPreset = CreatePreset(
            MergedName,
            FString::Printf(TEXT("Merged preset from %s and %s"), *PresetA->PresetName, *PresetB->PresetName),
            TEXT("Merged"),
            TArray<FString>{TEXT("merged"), TEXT("blend")},
            MergedConfig
        );
        
        if (MergedPreset)
        {
            MergedPreset->CustomProperties.Add(TEXT("SourcePresetA"), PresetA->PresetName);
            MergedPreset->CustomProperties.Add(TEXT("SourcePresetB"), PresetB->PresetName);
            MergedPreset->CustomProperties.Add(TEXT("BlendFactor"), FString::Printf(TEXT("%.2f"), BlendFactor));
            MergedPreset->CustomProperties.Add(TEXT("MergeType"), TEXT("Configuration"));
            
            UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), 
                FString::Printf(TEXT("Merged preset created: %s (blend: %.2f)"), *MergedName, BlendFactor));
        }
        
        return MergedPreset;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception merging presets: %s"), UTF8_TO_TCHAR(e.what())));
        return nullptr;
    }
}

void UPlanetPreset::GetPresetSystemStats(FString& OutStats)
{
    try
    {
        TArray<FString> AllPresets = ListAvailablePresets();
        
        int32 TotalPresets = AllPresets.Num();
        int32 TotalCategories = GetAvailableCategories().Num();
        int32 TotalTags = GetAvailableTags().Num();
        
        float TotalRating = 0.0f;
        int32 TotalUsage = 0;
        int32 PublicPresets = 0;
        int32 DefaultPresets = 0;
        int32 FavoritePresets = 0;
        
        for (const FString& PresetName : AllPresets)
        {
            UPlanetPreset* Preset = LoadPreset(PresetName);
            if (Preset)
            {
                TotalRating += Preset->Rating;
                TotalUsage += Preset->UsageCount;
                if (Preset->bIsPublic) PublicPresets++;
                if (Preset->bIsDefault) DefaultPresets++;
                if (Preset->bIsFavorite) FavoritePresets++;
            }
        }
        
        float AverageRating = TotalPresets > 0 ? TotalRating / TotalPresets : 0.0f;
        
        OutStats = FString::Printf(
            TEXT("Preset System Statistics:\n")
            TEXT("- Total Presets: %d\n")
            TEXT("- Total Categories: %d\n")
            TEXT("- Total Tags: %d\n")
            TEXT("- Average Rating: %.1f/5.0\n")
            TEXT("- Total Usage Count: %d\n")
            TEXT("- Public Presets: %d\n")
            TEXT("- Default Presets: %d\n")
            TEXT("- Favorite Presets: %d\n")
            TEXT("- Storage Directory: %s"),
            TotalPresets, TotalCategories, TotalTags, AverageRating, TotalUsage,
            PublicPresets, DefaultPresets, FavoritePresets, *GetPresetsDirectory()
        );
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), TEXT("Preset system statistics retrieved"));
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception getting system stats: %s"), UTF8_TO_TCHAR(e.what())));
        OutStats = TEXT("Error retrieving system statistics");
    }
}

void UPlanetPreset::ClearAllPresets()
{
    try
    {
        FString PresetsDir = GetPresetsDirectory();
        
        if (IFileManager::Get().DeleteDirectory(*PresetsDir, false, true))
        {
            UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), 
                FString::Printf(TEXT("All presets cleared from: %s"), *PresetsDir));
            
            UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
                EPlanetEventType::PresetsCleared, TEXT("AllPresets"));
        }
        else
        {
            UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
                FString::Printf(TEXT("Failed to clear presets from: %s"), *PresetsDir));
        }
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception clearing presets: %s"), UTF8_TO_TCHAR(e.what())));
    }
}

bool UPlanetPreset::ExportPresets(const FString& ExportPath, const TArray<FString>& PresetNames)
{
    try
    {
        if (PresetNames.Num() == 0)
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetPreset"), TEXT("No presets specified for export"));
            return false;
        }
        
        int32 ExportedCount = 0;
        
        for (const FString& PresetName : PresetNames)
        {
            UPlanetPreset* Preset = LoadPreset(PresetName);
            if (Preset)
            {
                FString ExportFilePath = ExportPath / PresetName + TEXT(".json");
                if (SavePreset(Preset, ExportFilePath))
                {
                    ExportedCount++;
                }
            }
        }
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exported %d/%d presets to: %s"), ExportedCount, PresetNames.Num(), *ExportPath));
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::PresetsExported, TEXT("Export"), ExportPath, 0.0f, ExportedCount);
        
        return ExportedCount > 0;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception exporting presets: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

bool UPlanetPreset::ImportPresets(const FString& ImportPath, const TArray<FString>& Filenames)
{
    try
    {
        if (Filenames.Num() == 0)
        {
            UPlanetSystemLogger::LogWarning(TEXT("PlanetPreset"), TEXT("No files specified for import"));
            return false;
        }
        
        int32 ImportedCount = 0;
        
        for (const FString& Filename : Filenames)
        {
            FString ImportFilePath = ImportPath / Filename;
            UPlanetPreset* Preset = LoadPreset(ImportFilePath);
            if (Preset)
            {
                // Salva no diretório de presets local
                FString LocalFilename = FPaths::GetBaseFilename(Filename);
                if (SavePreset(Preset, LocalFilename))
                {
                    ImportedCount++;
                }
            }
        }
        
        UPlanetSystemLogger::LogInfo(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Imported %d/%d presets from: %s"), ImportedCount, Filenames.Num(), *ImportPath));
        
        UPlanetEventBus::GetInstance()->BroadcastEventWithParams(
            EPlanetEventType::PresetsImported, TEXT("Import"), ImportPath, 0.0f, ImportedCount);
        
        return ImportedCount > 0;
    }
    catch (const std::exception& e)
    {
        UPlanetSystemLogger::LogError(TEXT("PlanetPreset"), 
            FString::Printf(TEXT("Exception importing presets: %s"), UTF8_TO_TCHAR(e.what())));
        return false;
    }
}

// Funções privadas auxiliares

FString UPlanetPreset::GetPresetsDirectory()
{
    FString PresetsDir = FPaths::ProjectSavedDir() / TEXT("PlanetSystem/Presets");
    
    // Cria o diretório se não existir
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*PresetsDir);
    
    return PresetsDir;
} 