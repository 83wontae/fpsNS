// Fill out your copyright notice in the Description page of Project Settings.


#include "NSGameStateBase.h"
#include "Net/UnrealNetwork.h"

ANSGameStateBase::ANSGameStateBase()
{
	bInMenu = false;
}

void ANSGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANSGameStateBase, bInMenu);
}