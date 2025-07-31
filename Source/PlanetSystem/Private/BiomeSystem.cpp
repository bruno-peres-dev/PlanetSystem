#include "BiomeSystem.h"

EBiomeType UBiomeSystem::GetBiome(float Altitude, float Slope, float Humidity) const
{
    if (!BiomeLookup) return EBiomeType::Plains;
    int32 X = FMath::Clamp(int32((Altitude/2000.f)*BiomeLookup->GetSizeX()), 0, BiomeLookup->GetSizeX()-1);
    int32 Y = FMath::Clamp(int32(Slope*BiomeLookup->GetSizeY()), 0, BiomeLookup->GetSizeY()-1);
    FColor* Colors = static_cast<FColor*>(BiomeLookup->PlatformData->Mips[0].BulkData.LockReadOnly());
    FColor C = Colors[Y*BiomeLookup->GetSizeX() + X];
    BiomeLookup->PlatformData->Mips[0].BulkData.Unlock();
    return EBiomeType(C.R % (int)EBiomeType::Snow + 1);
}
