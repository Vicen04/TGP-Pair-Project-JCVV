// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Life.h"
#include "Components/TextRenderComponent.h"

// Sets default values
AEnemy::AEnemy()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Ignore);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> EnemyMesh(TEXT("/Game/InfinityBladeAdversaries/Enemy/Enemy_Golem/SK_Fire_Golem.SK_Fire_Golem"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> WeaponMesh(TEXT("/Game/InfinityBladeAdversaries/Enemy/Enemy_Golem/SK_Enemy_Golem01_Weapon.SK_Enemy_Golem01_Weapon"));



	box = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	
	box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	box->AttachTo(GetCapsuleComponent());
	box->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
	box->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	box->SetGenerateOverlapEvents(true);
	box->SetNotifyRigidBodyCollision(true);
	box->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));

	GetMesh()->SetSkeletalMesh(EnemyMesh.Object);
	GetMesh()->AttachTo(box);

	weaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon"));
	weaponMesh->SetSkeletalMesh(WeaponMesh.Object);
	weaponMesh->AttachTo(GetMesh(), "weapon");

	attackCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Attack_Collision"));
	attackCollision->AttachTo(weaponMesh);
	attackCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	attackCollision->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	attackCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	attackCollision->SetGenerateOverlapEvents(true);
	


	_health = CreateDefaultSubobject<ULife>(TEXT("Health"));
	_health->SetHealth(50);

	text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthDisplay"));
	text->AttachTo(box);
	text->SetTextRenderColor(FColor(255, 0, 0, 100));
	text->SetText(FString("Health: ") + FString::SanitizeFloat(_health->Health));
	text->WorldSize = 20.0f;

	DamageCooldown = 0.0f;

	damage = 0.0f;

	box->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::Damaged);

	attack = false;
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	GetCapsuleComponent()->SetCapsuleSize(1.0f, GetMesh()->CalcBounds(GetMesh()->GetComponentTransform()).BoxExtent.Z / 2.0f);
	box->SetBoxExtent(GetMesh()->CalcBounds(GetMesh()->GetComponentTransform()).BoxExtent/2);
	GetMesh()->SetRelativeLocation(FVector(0, 0, -GetMesh()->CalcBounds(GetMesh()->GetComponentTransform()).BoxExtent.Z / 2.0f));
	text->SetRelativeLocationAndRotation(FVector(0,0, GetMesh()->CalcBounds(GetMesh()->GetComponentTransform()).BoxExtent.Z), FQuat(FRotator(0,90,0)));
	text->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	if (GetMesh()->SkeletalMesh->GetName() == "SK_Fire_Golem")
	{
		attackCollision->SetBoxExtent(weaponMesh->CalcBounds(weaponMesh->GetComponentTransform()).BoxExtent / 2);
		attackCollision->SetRelativeLocation(FVector(0, 0, weaponMesh->CalcBounds(weaponMesh->GetComponentTransform()).BoxExtent.Z / 2));
		text->SetText(FString("Health: ") + FString::SanitizeFloat(weaponMesh->CalcBounds(weaponMesh->GetComponentTransform()).BoxExtent.X) + ", "+ FString::SanitizeFloat(weaponMesh->CalcBounds(weaponMesh->GetComponentTransform()).BoxExtent.Y) + ", " + FString::SanitizeFloat(weaponMesh->CalcBounds(weaponMesh->GetComponentTransform()).BoxExtent.Z));
	}
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (DamageCooldown > 0.0f)
		DamageCooldown -= DeltaTime;
	if (_health->GetHealth() <= 0.0f)
		DamageCooldown -= DeltaTime;
}

void AEnemy::Damaged(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	if (DamageCooldown <= 0.0f) 
	{
	    _health->UpdateHealth(damage);
		DamageCooldown = 2.0f;
		text->SetText(FString("Health: ") + FString::SanitizeFloat(_health->Health));
	}
}

void AEnemy::AttackPlayer(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	attack = true;
	attackCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AEnemy::Destroy()
{
	attackCollision->DestroyComponent();
	text->DestroyComponent();
	GetMesh()->DestroyComponent();
	box->DestroyComponent();
}
