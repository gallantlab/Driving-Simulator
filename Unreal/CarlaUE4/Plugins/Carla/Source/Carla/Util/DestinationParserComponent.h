// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "NeighborhoodBoxBase.h"
#include "Components/ActorComponent.h"
#include "Util/NavigationDestination.h"
#include "Math/RandomStream.h"
#include "DestinationParserComponent.generated.h"

const FString INVALID = FString("Invalid");

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CARLA_API UDestinationParserComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDestinationParserComponent();

	int GetDestinationsFromMap();

	virtual void BeginPlay() override;

	const TArray<NavigationDestination>& GetDestinations() const
	{
		return destinations;
	}

	UFUNCTION(BlueprintPure)
	int Num() const {return destinations.Num();}

	NavigationDestination& operator[](int i)
	{
		return destinations.Num() > 0 ? destinations[i] : none;
	};

	NavigationDestination& At(int i)
	{
		return destinations.Num() > 0 ? destinations[i] : none;
	};

	int GetMaxIndex() const { return maxIndex; };

	int GetMinIndex() const { return minIndex; };

	bool IsIndexed() const { return isIndexed > 0; };

	// stuff for blueprints to read information out
	UFUNCTION(BlueprintCallable)
	const FString &GetDestinationName(int index) const
	{
		if (Num() < 1 || index < 0 || index >= Num())
			return INVALID;
		return destinations[index].GetName();
	};

	UFUNCTION(BlueprintCallable)
	const FVector2D GetDestinationLocation(int index) const
	{
		if (Num() < 1 || index < 0 || index >= Num())
			return FVector2D(0, 0);
		return destinations[index].GetDestination();
	}



	/**
	 * Parses active destinations from a list specified by a string
	 * @param activeDestinationsFile
	 * @return
	 */
	int ParseActiveDestinations(const FString &activeDestinationsFile);

	/**
	 * Parse active destinations using the TMap in the settings object
	 * use for reloading, and does not check against the text value in settings
	 * @return
	 */
	int ParseActiveDestinations();

	UFUNCTION(BlueprintCallable)
	bool IsDestinationActive(int index) const
	{
		if (isActive.Num() == 0)
			return true;
		return isActive[index];
	}

	/**
	 * Get the number of destinations in active neighborhoods
	 * @return
	 */
	int GetNumberOfActiveDestinations() const ;

	// Stuff for tracking learning
	UFUNCTION(BlueprintCallable)
	bool GetHasBeenVisited(int index) const
	{
		if (hasBeenVisited.Num() < 1)
			return false;
		return hasBeenVisited[index];
	}

	UFUNCTION(BlueprintCallable)
	TArray<int> GetAllDestinations() {
		return activeDestinations;
	}

	bool SetHasBeenVisited(int index, bool beenVisited = true);

	/**
	 * Gets the number of destination that has been visited
	 * @param activeOnly count only currently active destinations?
	 * @return
	 */
	int GetNumberOfVisitedDestinations(bool activeOnly = true) const ;

	void ClearVisitedDestinations();

	// For saving/loading destinations visited
	bool SaveHasBeenVisited(const FString& fileName) const;

	bool LoadHasBeenVisited(const FString& fileName);

	// === DESTINATION PICKING METHODS ===
public:
	double GetDistanceToDestination(AActor *actor, int destinationIndex) const;

	int GetNextDestination(AActor *actor) const;

	// parameters for biasing destination picking by proximity
	double EXCLUSION_MIN;

	double EXCLUSION_MAX_START;

	double EXCLUSION_MAX_END;

	double EXCLUSION_MIN_PROBABILITY;

	EDestinationPickingMode destinationPickingMode = EDestinationPickingMode::BiasedRandom;

	int PickPureRandomDestination(AActor *actor) const
	{
		return randomStream.RandHelper(destinations.Num());
	}

	int PickBiasedRandomDestination(AActor *actor) const;

	int PickPurePermutedDestination(AActor *actor) const;

	int PickBiasedPermutedDestination(AActor *actor) const;

	void GetNeighborhoodsFromMap();

	void UpdateActiveNeighborhoods();

	bool IsActorInActiveNeighborhoods(AActor *actor) const;

	// === FORAGING ===
	void RandomizeTargetValues();

	/**
	 * Get the point value for the target; is not the underlying enum value but rather
	 * the point value specified in the config file
	 * @param target	index in the destinations to get
	 * @return	number of points for collecting this target
	 */
	int GetTargetPointsValue(int target) const;

protected:
	TArray<NavigationDestination> destinations;

	TArray<int> activeDestinations;		// destinations currently in use, contains th ID of destinations

	TArray<bool> isActive;				// whether each destination is active

	TArray<bool> hasBeenVisited;

	NavigationDestination none;

	TArray<ANeighborhoodBoxBase*> allNeighborhoods;	// all the neighborhood bounding boxes

	TArray<ANeighborhoodBoxBase*> activeNeighborhoods;	// neighborhoods that we constrain the participant to

private:
	int minIndex = NOT_INDEXED;
	int maxIndex = 0;
	int isIndexed = 0;

	FRandomStream randomStream;

	// point values for foraging task
	int lowValuePoints = 10;
	int medValuePoints = 20;
	int highValuePoints = 40;
};
