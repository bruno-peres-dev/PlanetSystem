#include "Generation/Noise/NoiseModule.h"
#include "FastNoise/FastNoise.h"

float UNoiseModule::GetHeight(const FVector& Dir) const
{
    FastNoise BaseNoise, WarpNoise;
    BaseNoise.SetSeed(Seed);
    WarpNoise.SetSeed(Seed ^ 0x9E3779B1);

    WarpNoise.SetNoiseType(FastNoise::Simplex);
    WarpNoise.SetFrequency(Frequency * 0.5f);

    FVector sampleDir = Dir * Frequency;
    if (bEnableWarp)
    {
        FVector warp(
            WarpNoise.GetNoise(Dir.X+31,Dir.Y+17,Dir.Z+59),
            WarpNoise.GetNoise(Dir.X+97,Dir.Y+43,Dir.Z+11),
            WarpNoise.GetNoise(Dir.X+59,Dir.Y+71,Dir.Z+23)
        );
        sampleDir += warp * WarpStrength;
    }

    switch(NoiseType)
    {
    case ENoiseType::Ridged: BaseNoise.SetNoiseType(FastNoise::RidgedMulti); break;
    case ENoiseType::Billow: BaseNoise.SetNoiseType(FastNoise::Billow);     break;
    default:                  BaseNoise.SetNoiseType(FastNoise::Perlin);
    }

    BaseNoise.SetFrequency(1.0f);

    float amp=1.f, freq=1.f, h=0.f;
    for(int32 i=0;i<Octaves;++i)
    {
        float n = BaseNoise.GetNoise(sampleDir.X*freq, sampleDir.Y*freq, sampleDir.Z*freq);
        h += n * amp;
        amp *= Persistence;
        freq *= Lacunarity;
    }
    return h * 200.f;
}
