// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "HumanStartZone.generated.h"

/**
 * PlayerStarts inside HumanStartZone can only be used for humans and not AI vehicles
 * If there are none on a map, then it's a free-for-all
 */
UCLASS()
class CARLA_API AHumanStartZone : public ATriggerBox
{
	GENERATED_BODY()
public:
	// Check if a non-colliding actor, e.g. APlayerStart, intersects
	bool IsOverlappingNonCollidingActor(AActor *actor) const;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int runningDirection = 0;
};
