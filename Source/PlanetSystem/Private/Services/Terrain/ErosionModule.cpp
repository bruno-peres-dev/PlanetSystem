#include "Services/Terrain/ErosionModule.h"
#include "Math/UnrealMathUtility.h"

void UErosionModule::ApplyHydraulicErosion(TArray<FVector>& Vertices, int32 Res, uint32 Seed)
{
    if (!bEnableHydraulic) return;
    FRandomStream Stream(Seed);
    int32 Size = Res+1;
    auto GetIdx=[&](int X,int Y){ return Y*Size + X; };

    TArray<float> HeightMap;
    HeightMap.SetNumUninitialized(Vertices.Num());
    for(int i=0;i<Vertices.Num();++i)
        HeightMap[i] = Vertices[i].Size() - 1000.f;

    for(int i=0;i<Iterations; ++i)
    {
        int x=Stream.RandRange(1,Res-1), y=Stream.RandRange(1,Res-1);
        float water=1.f, sediment=0.f;
        for(int step=0; step<MaxSteps; ++step)
        {
            int idx = GetIdx(x,y);
            int nx=x, ny=y; float minH=HeightMap[idx];
            for(int oy=-1; oy<=1; ++oy) for(int ox=-1; ox<=1; ++ox)
            {
                float h = HeightMap[GetIdx(x+ox,y+oy)];
                if(h<minH){ minH=h; nx=x+ox; ny=y+oy; }
            }
            if(nx==x && ny==y) break;
            float delta = minH - HeightMap[idx];
            float cap = FMath::Max(-delta * SedimentCapacity, 0.01f);
            if(sediment > cap)
            {
                float dep = (sediment - cap) * DepositRate;
                sediment -= dep;
                HeightMap[idx] += dep;
            }
            else
            {
                float er = FMath::Min((cap - sediment) * ErodeRate, -delta);
                sediment += er;
                HeightMap[idx] -= er;
            }
            water *= 0.9f;
            x=nx; y=ny;
            if(water < 0.01f) break;
        }
    }

    for(int i=0;i<Vertices.Num();++i)
    {
        FVector dir = Vertices[i].GetSafeNormal();
        Vertices[i] = dir * (1000.f + HeightMap[i]);
    }
}
