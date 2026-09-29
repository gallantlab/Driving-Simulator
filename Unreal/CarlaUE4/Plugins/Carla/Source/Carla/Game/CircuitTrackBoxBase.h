// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TriggerBox.h"
#include "CircuitTrackBoxBase.generated.h"

/**
 * Base class for trigger boxes used to mark out circuits on road segments.
 * Child class is stretchy trigger box
 * Exists for easy C++ reference
 */
UCLASS()
class CARLA_API ACircuitTrackBoxBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACircuitTrackBoxBase(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere)
	/**
	 * The circuit(s) that use this road segments
	 */
	TArray<FName> Circuits;

	/**
	 * The nominal direction of travel for this circuit.
	 * To be set by blueprint child class.
	 */
	UPROPERTY(BluePrintReadWrite, EditAnywhere)
	FVector Direction;

	bool TriggerBoxOverlaps(const AActor* other) const ;

	virtual void BeginPlay() override;

private:
	ATriggerBox* triggerBox = nullptr;
};
