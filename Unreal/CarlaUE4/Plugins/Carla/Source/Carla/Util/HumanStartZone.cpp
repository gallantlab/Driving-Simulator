// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "HumanStartZone.h"




bool AHumanStartZone::IsOverlappingNonCollidingActor(AActor *actor) const
{
	return GetComponentsBoundingBox(true).Intersect(actor->GetComponentsBoundingBox(true));
}