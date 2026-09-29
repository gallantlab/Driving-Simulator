// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Game/CircuitTrackBoxBase.h"
#include "Circuit.generated.h"


/**
 * Stuff about how long this circuit is
 */
UENUM(BlueprintType)
enum class ECircuitLength : uint8
{
	Short				UMETA(DisplayName = "Short"),
	Medium				UMETA(DisplayName = "Medium"),
	Long				UMETA(DisplayName = "Long"),
	None				UMETA(DisplayName = "None")
};


enum class EPlayerCircuitState;

/**
 * A closed loop circuit with some meta information
 */
UCLASS()
class CARLA_API ACircuit : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACircuit();

	TArray<ACircuitTrackBoxBase *> triggerBoxes;

	UPROPERTY(EditAnywhere)
	ECircuitLength CircuitLength = ECircuitLength::Medium;

	UFUNCTION(BlueprintPure)
	bool IsActive() const { return bIsActive; };

	// because I'm too lazy to figure out all the component attachment/detachment stuff
	EPlayerCircuitState Overlaps(const AActor * other);

	UFUNCTION(BlueprintCallable)
	void SetIsActive(bool state);

	UPROPERTY(EditAnywhere)
	FName CircuitName;

protected:
	void BeginPlay() override;

private:

	/**
	 * Finds the triggerboxes associated with this circuit. Uses FName tags on the triggerboxes
	 * Since in the level editor the triggerboxes are simply tagged and not actually associated
	 * with ACircuit objects
	 */
	void FindTriggerBoxes();

	bool bIsActive = false;
};
