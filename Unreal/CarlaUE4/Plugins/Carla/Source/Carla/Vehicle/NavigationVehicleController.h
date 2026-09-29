// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "Carla.h"
#include "CoreMinimal.h"
#include "Vehicle/MRIPlayerController.h"
#include "Game/NavigationPlayerState.h"
#include "Util/DestinationParserComponent.h"

#include "NavigationVehicleController.generated.h"

const FString GOTO = FString("Go to ");
const FString ARRIVEDAT = FString("Arrived at ");
const FString LOST = FString("Trial ended b/c you are lost");


/**
 * Delegate declaration for telling HUD to temporarily show things
 * because we don't want a circular reference between the controller and HUD
 */
 DECLARE_DELEGATE_OneParam(FShowHelpInfoDelegate, bool);

/**
 * Used for navigation experiments. Generates destinations, checks for arrival, etc.
 */
UCLASS()
class CARLA_API ANavigationVehicleController : public AMRIPlayerController
{
	GENERATED_BODY()

public:
	ANavigationVehicleController(const FObjectInitializer& ObjectInitializer);

	// this has to be handlled by the controller, because all the player state stores is the ID of the destination for fast replication
	FString GetCurrentDestinationName() const;

	// messages for the HUD
	bool IsArrived() const
	{
		return showArrival > 0;
	};

	bool NewDestination() const
	{
		return showDestination > 0;
	};

	bool IsLost() const
	{
		return showLost > 0;
	};

	bool HasDestination() const;

	virtual void Possess(APawn *aPawn) override;

	virtual void BeginPlay() override;

	virtual void ExperimentTick(float deltaTime) override;

	virtual void Tick(float dTime) override;

	// makes respawning close to where you are
	virtual void RestartLevel() override;

	// ===== Navigation experiment stuff =====
public:
	double GetDistanceToDestination() const
	{
		return destinations->GetDistanceToDestination(GetPawn(), navigationPlayerState->currentDestination);
	}

	double GetDistanceToDestination(int destinationIndex) const
	{
		return destinations->GetDistanceToDestination(GetPawn(), destinationIndex);
	}

/// Get direction returns [-1, 1] in which negative is left and positive is right
/// value is the dot product of the view vector and the vector to the destination
/// Returns 0 for no destination to center arrow
	double GetDirectionToDestination();

	double GetDirectionToDestination(int destinationIndex);

	/// Gets an egocentric vector to the destination
	void GetDestinationParameters(FVector2D &vector, float &distance, float& direction);

	const FString GetProximalTarget();

	// reset state at end of a single run
	virtual void Reset() override;

	virtual void ResetExperimentState() override;

	// Sets variables that record things in the player state object about when the actual run starts in the demo
	// used for determining which frame is the first frame in rendering
	UFUNCTION(BlueprintCallable)
	void RecordSecondsBeforeFirstDestination(float seconds);


	// === Display related stuff ===
	/**
	 * Gets (if) any instructions to be displayed at the middle of the screen
	 * @return FString, always, if no instructions, returns an empty string.
	 */
	virtual FString GetDisplayText() const;

	EDisplayTextColor GetDisplayTextColor() const
	{
		return textColor;
	}

	UDestinationParserComponent *GetDestinationParserComponent() const
	{
		return destinations;
	}

	int GetCurrentDestinationID() const
	{
		return navigationPlayerState->GetCurrentDestination();
	}

	/**
	 * Stuff that displays text and readies the controller to generate the next destination
	 * factored out of ExperimentTick and made public so that the game mode's debug exec
	 * function can call this and have the controller pretend it's arrived. For debugging.
	 * @param maxWait	longest time until next destination (it's at least 4 seconds)
	 * @param isLost	is this segment ending because the subject is lost? false means they arrived
	 */
	virtual void OnSegmentEnd(float maxWait, bool isLost = false);

	/**
	 * Reload the active destination set, because what's active has been changed by the subject
	 * Called by Settings HUD
	 */
	virtual void ReloadActiveDestinations();

	void OpenSettingsMenu();

	int GetCurrentDestination() const
	{
		return navigationPlayerState ? navigationPlayerState->currentDestination : -1;
	}

	void ShowControls();

private:
	// Used to keep track of destinations across trials
	// i.e. when a segment is interrupted by the end of the scan,
	// the next scan with start with the same destination for continuity
	// -1: no destination to pick up from
	// anything >-1: destination to use
	int destination = -1;

protected:
	virtual void ConfigureNextDestination();

	virtual void PickNextDestination(int minIndex = 0, int maxIndex = -1);

	UDestinationParserComponent *destinations;

	EDisplayTextColor textColor = EDisplayTextColor::White;

	virtual void SetupInputComponent() override;

	// mechanisms for the subject to indicate that they are lost
	// they must press the button three times in three seconds (Guards against accidental press)
	void LostDown()
	{
		if (!lost)
		{
			lost = true;
			lostDown = 3.0;
			lostPressCount = 0;
		}
		lostPressCount++;
	}

	void LostUp()
	{
		//lost = false;
	}

	void LostConfirm()
	{
		if (lost)
			OnSegmentEnd(12.0, true);
	}

	/**
	 * Temporarily display the Learning HUD
	 */

public:
	FShowHelpInfoDelegate ShowHelpInfoDelegate;

protected:
	void NeedHelp();

	void ShowHelp(double duration = 2.0);


protected:
	bool lost = false;	// for a gated way of confirming that the subject is lost
	double lostDown = 0.0;
	int lostPressCount = 0;

	double showHelp = 0.0;	// number of seconds to show help HUD


protected:
	virtual bool CheckForArrival();

	bool usingTriggerBoxes = true;

	// number of TTLs to wait for before generating the first destination
	int TTLsUntilFirstDestination = 5;

	// number of seconds until the next destination is generated; jittered
	// first one set to 0 to arrive with fifth TTL
	double secondsUntilNextDestination = 0;

	// how much longer, if any, to display messages on the HUD
	double showArrival;

	double showDestination;

	double showLost;

	// string pointer the name of the current destination; needs this because the name needs to be held after arrival,
	// when the destination id is set to -1
	const FString *destinationName;

	ANavigationPlayerState* navigationPlayerState;


protected:
	virtual void TTLup() override;

	virtual void TTLdown() override;

	bool randomDestinations = true;

	/**
	 * Sets the current destination.
	 * doesn't do much for this class but useful for the track running & foraging controllers
	 * the markerOff parameter exist because behavvior differs between the track running and foraging controllers
	 * @param newDestination	new destination to set
	 * @param markerOff			turn the marker of the current destination off?
	 */
	virtual void SetCurrentDestination(int newDestination, bool markerOff = true);
};
