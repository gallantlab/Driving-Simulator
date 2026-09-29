// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "NavigationDestination.h"

NavigationDestination::NavigationDestination()
{
	name = FString(TEXT("uninitialized"));
	ID = -1;
	location = FVector2D(-1, -1);
}

NavigationDestination::NavigationDestination(const FString& name, int ID, const FVector2D& location)
{
	this->name = name;
	this->ID = ID;
	this->location = location;
}

NavigationDestination::NavigationDestination(const FString& name, int ID, float x, float y)
{
	this->name = name;
	this->ID = ID;
	location = FVector2D(x, y);
}

NavigationDestination::NavigationDestination(ANamedTriggerBoxBase *triggerBox)
{
	this->triggerBox = triggerBox;
	name = FString(triggerBox->Name);
	location = FVector2D(triggerBox->GetActorLocation());
	ID = triggerBox->GetID();
}

FVector2D NavigationDestination::GetDestination() const
{
	return location;
}

const FString& NavigationDestination::GetName() const
{
	return name;
}

uint32 NavigationDestination::GetID() const
{
	return ID;
}

bool NavigationDestination::operator<(/*const NavigationDestination & lhs,*/ const NavigationDestination & rhs) const
{
	// unordered triggerboxes
	if (!triggerBox || triggerBox->Index == NOT_INDEXED)
		return this->ID < rhs.ID;
	else	// else sort by explicit index
		return this->triggerBox->Index < rhs.triggerBox->Index;
}

bool NavigationDestination::IsProximal(AActor *actor)
{
	if (triggerBox)
		return triggerBox->IsOverlappingActor(actor);
	else
	{
		if (FVector2D::Distance(location, FVector2D(actor->GetActorLocation())) < 750)
			return true;
		else return false;
	}
}

void NavigationDestination::SetTriggerBoxVisibility(bool visible)
{
	if (triggerBox)
		triggerBox->SetActorHiddenInGame(!visible);
}

const TArray<EDestinationSet>* NavigationDestination::GetDestinationSets() const
{
	if (triggerBox)
		return &(triggerBox->DestinationSets);
	else
		return nullptr;
}


NavigationDestination::~NavigationDestination()
{
}


int NavigationDestination::GetIndex() const
{
	if (triggerBox) return triggerBox->Index;
	else return NOT_INDEXED;
}


void NavigationDestination::SetTargetValue(ETargetPointValue newValue)
{
	if (triggerBox)
	{
		triggerBox->Value = newValue;
		triggerBox->UpdateAppearance();
	}
}
