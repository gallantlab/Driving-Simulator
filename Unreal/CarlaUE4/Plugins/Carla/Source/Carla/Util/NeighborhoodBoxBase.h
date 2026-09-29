// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "Settings/CarlaSettings.h"
#include "NeighborhoodBoxBase.generated.h"

/**
 * Similar to NamedTriggerBoxBase, but used for delineating neighborhoods
 */
UCLASS()
class CARLA_API ANeighborhoodBoxBase : public ATriggerBox
{
	GENERATED_BODY()
	
public:
	ANeighborhoodBoxBase(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EDestinationSet neighborhood = EDestinationSet::None;
	
};
