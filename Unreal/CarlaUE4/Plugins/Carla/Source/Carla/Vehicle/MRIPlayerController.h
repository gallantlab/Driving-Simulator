// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "Carla.h"
#include "CoreMinimal.h"
#include "Vehicle/CarlaVehicleController.h"
#include "Game/CarlaGameInstance.h"
#include "Game/MRIPlayerState.h"

#include "MRIPlayerController.generated.h"

/// DISPLAY STUFF
const FString EMPTY_STRING = FString("");

/// Display text color
UENUM(BlueprintType)
enum class EDisplayTextColor : uint8
{
	White	UMETA(DisplayName = "White"),
	Red 	UMETA(DisplayName = "Red"),
};

// Delegates for displaying things
DECLARE_DELEGATE_OneParam(FSetDisplayTextDelegate, FString);

DECLARE_DELEGATE_OneParam(FSetDisplayTextColorDelegate, EDisplayTextColor);

DECLARE_DELEGATE_OneParam(FSetDisplayPointsDelegate, int);

DECLARE_DELEGATE_OneParam(FSetTimeRemainingDelegate, int);


/**
 * Interface for controllers that the subject will actively use during an experiment
 * Introduced because of the track-running simplified experiment in which there are no
 * destinations, but will share a lot of the TTL recording and eyetracking stuff
 */
UCLASS()
class CARLA_API AMRIPlayerController : public ACarlaVehicleController
{
	GENERATED_BODY()

public:
	AMRIPlayerController(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	virtual void Possess(APawn *pawn) override;


	UFUNCTION(BlueprintCallable)
	void SetEyetrackingEnded()
	{
		eyetrackingState = 2;
	}

	// override because we don't want to depsawn the subject
	UFUNCTION(Category = "Wheeled Vehicle Controller", BlueprintCallable)
	virtual void DespawnVehicle(EDespawnReason reason) override;


	// is the game demo recording ended? if so, reset
	bool IsDemoEnded();

	// function for experiment logic, to be overridden by inherited classes
	// experiment ticking starts _after_ 5 TRs have started in the
	virtual void ExperimentTick(float deltaTime) {};

	// Display stuff
	FSetDisplayTextDelegate SetDisplayTextDelegate;

	FSetDisplayTextColorDelegate SetDisplayTextColorDelegate;

	FSetDisplayPointsDelegate SetDisplayPointsDelegate;

	FSetTimeRemainingDelegate SetTimeRemainingDelegate;

protected:

	double displayTextTimeRemaining = 0;

	/**
	 * overridden to also try to update the display text
	 * @param increment 
	 */
	virtual void UpdatePoints(int increment = 1) override;

	/**
	 * Method used by all inheriting classes to set display text and color
	 * and this base class takes care of how long to show it
	 * @param text		text to display
	 * @param duration	time in seconds to display it; -1 to display continuously
	 * @param color		color of text to show
	 */
	void SetDisplayText(const FString& text, double duration = 2.0,
						EDisplayedPromptType displayedPromptType = EDisplayedPromptType::Unknown,
						EDisplayTextColor color = EDisplayTextColor::White);

	void SetDisplayText(const FString& text,
						EDisplayedPromptType displayedPromptType = EDisplayedPromptType::Unknown,
						double duration = 2.0,
						EDisplayTextColor color = EDisplayTextColor::White);

	void ClearDisplayText();

	/**
	 * Callback on end of text display. Override to do things when text finishes displaying
	 */
	virtual void OnEndDisplayText();

	virtual void ResetExperimentState();

	virtual void TTLup() override;

	virtual void TTLdown() override;

	double timeSinceDemoStart = 0.0;

	double secondsToFirstTTL = -1.90;

	// attach self to a game instance to check whether a demo recording is happening
	// needed because experiment params shouldn't continue during breaks and
	// need to be reset between runs
	bool FindGameInstance();

	UCarlaGameInstance* gameInstance = nullptr;

	UCarlaGameInstance* GetGameInstance();


	// eyetracking stuff
	// automatically do eyetracking at beginning of run?
	bool autoEyetrack = false;
	int eyetrackingState = 0;	// 0 = not started, 1 = started, 2 = ended

	FRandomStream randomStream;

private:
	int lastDemoState = 0;		// used to check for changes in demo recording state on each tick
	int currentDemoState = 0;	// used in conjunction with lastDemoStates

	void UpdateDemoState();

	// number of TTLs to wait for before starting experiment logic
	int TTLsToExperimentStart = 5;

	bool autoTriggerDemo = false;
	bool bAutoStopDemo = false;
	float autoStopDemoLimit = 0;
	float secondsWithoutTTL = 0.0;

	// used for auto render all; wait n seconds into game init before render
	// because calling gameInstance->FindReplays() to autotrigger rendering only works once per launch
	// and on the second trigger, no spectator controller is spawned
	bool autoRender = false;
	double secondsUntilRenderNext = 10;

	AMRIPlayerState *MRIPlayerState;
};
