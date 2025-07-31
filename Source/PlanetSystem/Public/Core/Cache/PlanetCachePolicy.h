#pragma once

#include "CoreMinimal.h"

/** Cache replacement policies */
enum class EPlanetCachePolicy
{
    LRU,
    LFU,
    Random
};
