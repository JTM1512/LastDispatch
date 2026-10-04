#include "HomeFlock.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"

AHomeFlock::AHomeFlock()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("FlockRoot")));
    UStaticMesh* Shape = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
    for (int32 I = 0; I < 8; ++I)
    {
        UStaticMeshComponent* Creature = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Creature%d"), I+1));
        Creature->SetupAttachment(RootComponent);
        Creature->SetStaticMesh(Shape);
        Creature->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Creature->SetRelativeScale3D(FVector(0.8, 0.8, 1.5));
        Creature->SetRelativeRotation(FRotator(90, 0, 0));
        Creatures.Add(Creature);
    }
    ResetFlock();
}

void AHomeFlock::ResetFlock()
{
    Positions.Reset(); Velocities.Reset(); Starts.Reset();
    for (int32 I = 0; I < Creatures.Num(); ++I)
    {
        const float Angle = I * 2 * PI / Creatures.Num();
        const FVector Start = Home + FVector(FMath::Cos(Angle)*320, FMath::Sin(Angle)*320, 0);
        Starts.Add(Start); Positions.Add(Start);
        Velocities.Add(FVector(180, FMath::Sin(Angle)*80, 0));
        Creatures[I]->SetRelativeLocation(Start);
    }
    Phase = 0; Accumulator = 0; Elapsed = 0; ReportTime = 0; ClosestPair = MAX_flt;
}

void AHomeFlock::BeginPlay()
{
    Super::BeginPlay();
    ResetFlock();
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It) { PC->SetViewTarget(*It); break; }
}

void AHomeFlock::Step(float Seconds)
{
    // All agents read the same snapshot, so array order does not affect steering.
    TArray<FVector> NextVelocity = Velocities;
    for (int32 I = 0; I < Positions.Num(); ++I)
    {
        FVector Away = FVector::ZeroVector, MeanVelocity = FVector::ZeroVector, Centre = FVector::ZeroVector;
        int32 Neighbours = 0;
        for (int32 J = 0; J < Positions.Num(); ++J)
        {
            if (I == J) continue;
            const FVector Difference = Positions[I] - Positions[J];
            const float Distance = Difference.Size();
            ClosestPair = FMath::Min(ClosestPair, Distance);
            if (Distance < NeighbourRadius)
            {
                MeanVelocity += Velocities[J]; Centre += Positions[J]; ++Neighbours;
                if (Distance < 260)
                    Away += Difference.GetSafeNormal() * (1 - Distance / 260) * 450;
            }
        }
        const FVector Goal = Phase == 0 ? Explore : Starts[I];
        const FVector ToGoal = Goal - Positions[I];
        FVector Steering = (ToGoal.GetSafeNormal() * FMath::Min(450.0, ToGoal.Size()*1.5) - Velocities[I]) * 1.2;
        Steering += Away * FMath::Clamp(Separation, 0.0f, 5.0f);
        if (Neighbours > 0)
        {
            Steering += (MeanVelocity / Neighbours - Velocities[I]) * FMath::Clamp(Alignment, 0.0f, 5.0f);
            Steering += (Centre / Neighbours - Positions[I]) * FMath::Clamp(Cohesion, 0.0f, 5.0f) * 0.45;
        }
        NextVelocity[I] = (Velocities[I] + Steering.GetClampedToMaxSize(700) * Seconds).GetClampedToMaxSize(450);
    }
    Velocities = MoveTemp(NextVelocity);
    bool AllArrived = true;
    for (int32 I = 0; I < Positions.Num(); ++I)
    {
        Positions[I] += Velocities[I] * Seconds;
        Creatures[I]->SetRelativeLocation(Positions[I]);
        if (!Velocities[I].IsNearlyZero())
            Creatures[I]->SetRelativeRotation(FRotator(90, Velocities[I].Rotation().Yaw, 0));
        AllArrived &= FVector::Dist(Positions[I], Phase == 0 ? Explore : Home) < 650;
    }
    if (AllArrived)
    {
        ++Phase;
        UE_LOG(LogTemp, Display, TEXT("Home flock: %s at %.2fs; agents=%d closest pair=%.1fcm"),
            Phase == 1 ? TEXT("reached exploration area") : TEXT("returned home"), Elapsed, Positions.Num(), ClosestPair);
    }
}

void AHomeFlock::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        const FKey Up[] = {EKeys::Q, EKeys::W, EKeys::E};
        const FKey Down[] = {EKeys::A, EKeys::S, EKeys::D};
        float* Values[] = {&Separation, &Cohesion, &Alignment};
        for (int32 I = 0; I < 3; ++I)
        {
            if (PC->WasInputKeyJustPressed(Up[I])) *Values[I] = FMath::Clamp(*Values[I]+0.2f, 0.0f, 5.0f);
            if (PC->WasInputKeyJustPressed(Down[I])) *Values[I] = FMath::Clamp(*Values[I]-0.2f, 0.0f, 5.0f);
        }
        if (PC->WasInputKeyJustPressed(EKeys::R)) ResetFlock();
    }
    Elapsed += DeltaSeconds;
    if (Phase < 2)
    {
        Accumulator += FMath::Min(DeltaSeconds, 0.1f);
        while (Accumulator >= 1.0f/60 && Phase < 2)
        {
            Step(1.0f/60); Accumulator -= 1.0f/60;
        }
    }
    ReportTime += DeltaSeconds;
    if (ReportTime >= 5)
    {
        ReportTime = 0;
        UE_LOG(LogTemp, Display, TEXT("Home flock: phase=%d separation=%.1f cohesion=%.1f alignment=%.1f"), Phase, Separation, Cohesion, Alignment);
    }
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.1f, FColor::White,
            FString::Printf(TEXT("Don't lose the flock!  %s  |  Challenge: %ds\nQ/A Separation %.1f   W/S Cohesion %.1f   E/D Alignment %.1f   R Restart\nSeparation spreads out | Cohesion gathers | Alignment matches direction"),
                Phase == 0 ? TEXT("Exploring") : Phase == 1 ? TEXT("Returning home") : TEXT("All 8 home"),
                FMath::Max(0, 180-FMath::FloorToInt(Elapsed)), Separation, Cohesion, Alignment));
}
