// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"

/**
 * Used by foraging - a thing to keep track of how well the patient
 * is doing with the task, and adjust task parameters as necessary
 * to make the task easier or harder. Kinda sorta staircasing the trial difficulty
 */
class CARLA_API AdaptiveTrialParameterGenerator
{
public:
	AdaptiveTrialParameterGenerator();

	/**
	 * Stores information about a completed trial
	 * @param numDesired	number of items the subject was asked to collect
	 * @param numCollected	number of items actually collected
	 * @param duration		duration of this trial in seconds
	 * @param availabilityMultiplier the availability multiplier used on this trial
	 */
	void AddTrialInfo(int numDesired, int numCollected, int duration, float availabilityMultiplier);

	/**
	 * Computes parameters for a new trial. Adjusts numDesired, duration, and availability multiplier
	 * based on prior subject performances
	 * @param numDesired				initial num targets desired on this trial
	 * @param duration					initial trial duration desired
	 * @param availabilityMultiplier	initial availability multiplier
	 * @param minTargets				min targets to give to subject, lower bound
	 * @param maxTargets				max targets to give to subject, upper bound
	 * @param baseTime					base time in settings, is a lower bound
	 * @param maxTrialTime				max trial time, is an upper bound
	 */
	void ValidateTrialInfo(int& numDesired, int& duration, double& availabilityMultiplier,
						   int& baseTime, int maxTrialTime,
	                       int minTargets, int maxTargets) const;

	~AdaptiveTrialParameterGenerator();

private:
	TArray<int> NumItemsDesired;
	TArray<int> NumItemsCollected;
	TArray<int> TimeToCollect;
	TArray<float> AvailabilityMultiplier;
};
