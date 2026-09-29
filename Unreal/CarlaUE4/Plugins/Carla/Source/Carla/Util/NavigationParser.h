// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "NavigationDestination.h"

/**
 *Helper functions for parsing destination information
 */
class CARLA_API NavigationParser
{
public:
	static bool ParseFile(const FString& destinationFile, TArray<NavigationDestination>& destinations);

	static bool WriteFile(const FString& destinationFile, TArray<NavigationDestination>& destinations);

	static bool ParseNavigationSequence(const FString& navigationSequenceFile, TArray<int> &sequence);

	static bool ParseActiveDestinations(const FString &activeDestinationsFile, TArray<int> &activeDestinations, bool overwrite);

	static bool ParseActiveDestinations(const FString &activeDestinationsFile, TArray<int> &activeDestinations);

	static bool ParseActiveDestinations(const TArray<FString> &activeDestinationsFiles, TArray<int> &activeDestinations);

	/**
	 * Looks like all the destinations and checks them against which destination sets are active
	 * If no sets are active, then all sets are active
	 */
	static bool ParseActiveDestinations(const TArray<NavigationDestination> &destinations, const TMap<EDestinationSet, bool> &activeSets, TArray<int> &activeDestinations, bool overwrite);

	static bool ParseActiveDestinations(const TArray<NavigationDestination> &destinations, const TMap<EDestinationSet, bool> &activeSets, TArray<int> &activeDestinations);
};
