// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Life.h"
#include "Components/TextRenderComponent.h"

// Sets default values
AEnemy::AEnemy()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> EnemyMesh(TEXT("/Game/InfinityBladeAdversaries/Enemy/Enemy_Golem/SK_Fire_Golem.SK_Fire_Golem"));

	

	box = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	
	box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	box->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	box->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	box->SetGenerateOverlapEvents(true);
	box->SetNotifyRigidBodyCollision(true);

	mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	mesh->SetSkeletalMesh(EnemyMesh.Object);
	mesh->AttachTo(box);

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
}

// Called when the game starts or when spawned
void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	box->SetBoxExtent(mesh->CalcBounds(mesh->GetComponentTransform()).BoxExtent/2);
	mesh->SetRelativeLocation(FVector(0, 0, -mesh->CalcBounds(mesh->GetComponentTransform()).BoxExtent.Z / 2.0f));
	text->SetRelativeLocationAndRotation(FVector(0,0, mesh->CalcBounds(mesh->GetComponentTransform()).BoxExtent.Z), FQuat(FRotator(0,90,0)));
	text->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (DamageCooldown > 0.0f)
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
