// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Util/NamedTriggerBoxBase.h"
/**
 * 	Simple class representing a navigation destination
 */
class CARLA_API NavigationDestination
{
public:
	NavigationDestination();

	NavigationDestination(const FString& name, int ID, const FVector2D& location);

	NavigationDestination(const FString& name, int ID, float x, float y);

	NavigationDestination(ANamedTriggerBoxBase *triggerBox);

	~NavigationDestination();

	FVector2D GetDestination() const;

	const FString& GetName() const;

	uint32 GetID() const;

	int GetIndex() const;

	bool IsProximal(AActor *actor);

	bool operator<(/*const NavigationDestination & lhs,*/ const NavigationDestination & rhs) const;

	void SetTriggerBoxVisibility(bool visible);

	const TArray<EDestinationSet> *GetDestinationSets() const;

	void SetTargetValue(ETargetPointValue newValue = ETargetPointValue::None);

	ETargetPointValue GetTargetValue() const
	{
		if (triggerBox) return triggerBox->Value;
		return ETargetPointValue::None;
	}

protected:

	FString name;
	FVector2D location;
	uint32 ID;
	ANamedTriggerBoxBase *triggerBox = nullptr;
};
