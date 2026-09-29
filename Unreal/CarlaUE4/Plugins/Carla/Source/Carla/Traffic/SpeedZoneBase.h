// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpeedZoneBase.generated.h"

UCLASS()
class CARLA_API ASpeedZoneBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpeedZoneBase(const FObjectInitializer &ObjectInitializer);

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly)
	bool bIsIntersection = false;

	UPROPERTY(BlueprintReadWrite)
	float speedLimit = 35 * 1.609;

};
