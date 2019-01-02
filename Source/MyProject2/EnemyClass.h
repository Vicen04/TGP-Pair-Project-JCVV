// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyClass.generated.h"

UCLASS()
class MYPROJECT2_API AEnemyClass : public ACharacter
{
	GENERATED_BODY()
		UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Collision, meta = (AllowPrivateAccess = "true"))
		class UBoxComponent * box;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Health, meta = (AllowPrivateAccess = "true"))
		class ULife* _health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Health, meta = (AllowPrivateAccess = "true"))
		class UTextRenderComponent* text;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Health, meta = (AllowPrivateAccess = "true"))
		class USkeletalMeshComponent * weaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Collision, meta = (AllowPrivateAccess = "true"))
		class UBoxComponent * attackCollision;

	UFUNCTION(BlueprintCallable, Category = Damage)
		void Damaged(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

	UFUNCTION(BlueprintCallable, Category = Damage)
		void AttackPlayer(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AnimationValue, meta = (AllowPrivateAccess = "true"))
		UBoolProperty* BPstopAttack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AnimationValue, meta = (AllowPrivateAccess = "true"))
		UBoolProperty* BPdisableMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = AnimationValue, meta = (AllowPrivateAccess = "true"))
		UBoolProperty* BPotherAnimation;

	class AMyProject2Character* character;
	TArray<AActor*> foundCharacter;

public:
	AEnemyClass();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Damage, meta = (AllowPrivateAccess = "true"))
		bool attack;

	float attackDamage;

	float Getattackdamage() { return attackDamage; };

	void Destroy();

	float damage;

	float DamageCooldown;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
