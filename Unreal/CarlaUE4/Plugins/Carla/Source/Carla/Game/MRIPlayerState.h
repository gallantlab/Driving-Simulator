// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/CarlaPlayerState.h"
#include "MRIPlayerState.generated.h"

/// Display text categories
UENUM()
enum class EDisplayedPromptType: uint8
{
	None,
	GoTo,
	Arrive,
	Lost,
	
	QueryConfidence,
	ConfidenceLow,
	ConfidenceLowMed,
	ConfidenceMed,
	ConfidenceMedHigh,
	ConfidenceHigh,
	ConfidenceNoResponse,
	
	Unknown,

	ForagingStart,
	ItemForaged,
	ForagingEnd,

	OutOfBounds,

	Paused,

	TargetVisited,
	QueryHeadingDirection,
	SessionEnd,

	TargetListUpdated
};

/**
 * 
 */
UCLASS()
class CARLA_API AMRIPlayerState : public ACarlaPlayerState
{
	GENERATED_BODY()
public:
	AMRIPlayerState();

	virtual void Reset() override;

	virtual void ResetExperimentState() override;

	virtual void CopyProperties(APlayerState *PlayerState) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	EDisplayedPromptType GetDisplayedPromptType() const
	{
		return displayedPromptType;
	}


private:
	friend class AMRIPlayerController;

	// What prompt is being displayed at this time
	UPROPERTY(VisibleAnywhere, Replicated)
	EDisplayedPromptType displayedPromptType = EDisplayedPromptType::None;
};
