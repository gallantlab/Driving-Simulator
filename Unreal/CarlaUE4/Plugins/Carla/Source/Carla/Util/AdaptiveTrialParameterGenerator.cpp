// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "AdaptiveTrialParameterGenerator.h"

AdaptiveTrialParameterGenerator::AdaptiveTrialParameterGenerator()
{
	NumItemsDesired = TArray<int>();
	NumItemsCollected = TArray<int>();
	TimeToCollect = TArray<int>();
	AvailabilityMultiplier = TArray<float>();
}

AdaptiveTrialParameterGenerator::~AdaptiveTrialParameterGenerator()
{
}


void AdaptiveTrialParameterGenerator::AddTrialInfo(int numDesired, int numCollected, int duration, float availabilityMultiplier)
{
	NumItemsDesired.Add(numDesired);
	NumItemsCollected.Add(numCollected);
	TimeToCollect.Add(duration);
	AvailabilityMultiplier.Add(availabilityMultiplier);

	// we only keep the last 5 trials to do a running average
	if (NumItemsCollected.Num() > 5)
	{
		NumItemsDesired.RemoveAt(0);
		NumItemsCollected.RemoveAt(0);
		TimeToCollect.RemoveAt(0);
		AvailabilityMultiplier.RemoveAt(0);
	}
}

#define IS_TIME_MORE_THAN_PAST_PERFORMANCE(duration, numDesired, averageTime) duration > numDesired * averageTime

void AdaptiveTrialParameterGenerator::ValidateTrialInfo(int& numDesired, int& duration, double& availabilityMultiplier,
	int& baseTime, int maxTrialTime,
	int minTargets, int maxTargets) const
{
	int trialCount = NumItemsCollected.Num();
	if (trialCount < 1)	// no trials yet so we don't do anything with the params
		return;

	float numCollected = 0,
		pastNumsDesired = 0,
		collectionDuration = 0,
		averageMultiplier = 0;
	int successes = 0;
	for (int i = 0; i < trialCount; i++)
	{
		numCollected += NumItemsCollected[i];
		pastNumsDesired += NumItemsDesired[i];
		collectionDuration += TimeToCollect[i];
		averageMultiplier += AvailabilityMultiplier[i];

		successes += (NumItemsCollected[i] == NumItemsDesired[i]);
	}

	float actualTimePerItem = collectionDuration / numCollected;
	float thisTimePerItem = (float)duration / pastNumsDesired;

	// we want success on ~80% of items to collect
	// subject hasn't been doing well, so we bump parameters up
	if (numCollected < pastNumsDesired * 0.8f)	
	{
		// primary - set time to use how long on average it took subjects to collect items
		// but we put a ceiling at the max possible trial time
		duration = actualTimePerItem * numDesired;

		// and we also immediately floor the multiplier at 2
		// otherwise it's likely few things will spawn
		availabilityMultiplier = 2;

		// secondary - if the time is maxed out, we reduce the number of items to collect
		// to a number that could be reasonably expected for the subject to collect in this
		// duration given past performance
		if (duration > maxTrialTime)
		{
			duration = maxTrialTime;

			numDesired = (int)(duration / actualTimePerItem);

			// tertiary: if this is below the minimum number to collect, we bump up
			// the availability multiplier. We don't really want to reach this bit because
			// this part doesn't necessarily have a closed loop with subject performance
			if (numDesired < minTargets)
			{
				numDesired = minTargets;
				availabilityMultiplier += 0.5;
				if (availabilityMultiplier > 4) availabilityMultiplier = 4;	// hard ceiling
			}
		}
	}
	// subject has been doing too well, so we bump things down
	else
	{
		// primary - reduce time by 5 seconds per item
		duration = (actualTimePerItem - 5) * numDesired;

		// secondary - if the time is bottomed out, we increase the number of items to collect
		if (duration < baseTime)
		{
			duration = baseTime;

			numDesired = (duration / actualTimePerItem) + 1;

			//tertiary: if this is more than the max amount + 1 (we give a bit of wiggle room)
			// we bump down the availability. Again, we don't really want to do this because
			// the availability is more nebulously connected to things
			if (numDesired > maxTargets + 1)
			{
				numDesired = maxTargets + 1;
				availabilityMultiplier -= 0.5;
				if (availabilityMultiplier < 1) availabilityMultiplier = 1;
			}
		}
	}

	// round duration to nearest 10 seconds
	int mod = duration % 10;
	duration = (duration / 10) * 10;
	if (mod > 4) duration += 10;
}
