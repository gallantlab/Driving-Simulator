// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "NamedTriggerBoxBase.generated.h"

#define NOT_INDEXED 2147483647

/**
 * An enum for the point value of this box target
 */
UENUM(BlueprintType)
enum class ETargetPointValue: uint8
{
	None = 0	UMETA(DisplayName = "None"),
	Low	 = 1	UMETA(DisplayName = "Low"),
	Medium = 2	UMETA(DisplayName = "Medium"),
	High = 4	UMETA(DisplayName = "High"),
};

/**
 * A box trigger with a name/ID property for navigation, and
 * also a point value for foraging
 */
UCLASS()
class CARLA_API ANamedTriggerBoxBase : public ATriggerBox
{
	GENERATED_BODY()

public:
	ANamedTriggerBoxBase(const FObjectInitializer& ObjectInitializer);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString Name;

	/**
	 * Used by the track task to figure out what order this destination is, since the
	 * names need to get flipped when approaching from the other side. Not used
	 * in other tasks.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int Index = NOT_INDEXED;

	// is this a destination to be used in test runs?
	UPROPERTY(EditAnywhere)
	bool TestDestination = false;

	// should we use this destination at all?
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool Active = true;

	// What destination sets does this destination belong to?
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<EDestinationSet> DestinationSets;

	uint32 GetID();

	void BeginPlay() override;

	bool operator<(ANamedTriggerBoxBase &other) const
	{
		return Index < other.Index;
	};

	// for the blueprint to know when to change the box material
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateAppearance();

	// How much is this item worth?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	ETargetPointValue Value = ETargetPointValue::None;

	// we need to replicate the values across time
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	uint32 id = 0;
};
