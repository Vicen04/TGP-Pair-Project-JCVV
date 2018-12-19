// Fill out your copyright notice in the Description page of Project Settings.

#include "Life.h"
#include "MyProject2Character.h"

// Sets default values for this component's properties
ULife::ULife()
{
	
}


// Called when the game starts
void ULife::BeginPlay()
{
	Super::BeginPlay();
	
}

void ULife::SetHealth(float health)
{
	Health = health;
}

void ULife::UpdateHealth(float damage)
{
	Health = Health - damage;
	if (Health < 0)
		Health = 0;
}


