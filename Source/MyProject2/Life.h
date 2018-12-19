// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Life.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MYPROJECT2_API ULife : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULife();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Health)
		float Health;

	float GetHealth() { return Health; };
	void UpdateHealth(float damage);
	void SetHealth(float health);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
};
