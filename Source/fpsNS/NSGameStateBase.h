// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "NSGameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class FPSNS_API ANSGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	ANSGameStateBase();

UPROPERTY(Replicated)
	bool bInMenu;
};
