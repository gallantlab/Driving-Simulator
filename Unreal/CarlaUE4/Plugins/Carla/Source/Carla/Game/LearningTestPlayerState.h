// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/NavigationPlayerState.h"
#include "LearningTestPlayerState.generated.h"

UENUM()
enum class EConfidence: uint8
{
	None,
	LOW,
	LOW_MED,
	MEDIUM,
	MED_HIGH,
	HIGH,

	Undefined
};

/**
 * 
 */
UCLASS()
class CARLA_API ALearningTestPlayerState : public ANavigationPlayerState
{
	GENERATED_BODY()
public:
	ALearningTestPlayerState();
	virtual void Reset() override;
	virtual void ResetExperimentState() override;
	virtual void CopyProperties(APlayerState *PlayerState) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	EConfidence GetPreConfidenceRating() const
	{
		return preconfidence;
	}

	EConfidence GetPostConfidenceRating() const
	{
		return postconfidence;
	}

	FRotator GetRelativeHeadingDirection() const
	{
		return relativeHeadingDirection;
	}

	FRotator GetAbsoluteHeadingDirection() const
	{
		return absoluteHeadingDirection;
	}

private:
	friend class ALearningTestController;

	UPROPERTY(VisibleAnywhere, Replicated)
	EConfidence preconfidence;

	UPROPERTY(VisibleAnywhere, Replicated)
	EConfidence postconfidence;

	UPROPERTY(VisibleAnywhere, Replicated)
	FRotator relativeHeadingDirection;

	UPROPERTY(VisibleAnywhere, Replicated)
	FRotator absoluteHeadingDirection;
};
