#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "HomeFlock.generated.h"

class UStaticMeshComponent;

UCLASS()
class LASTDISPATCH_API AHomeFlockGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AHomeFlockGameMode();
};

UCLASS()
class LASTDISPATCH_API AHomeFlock : public AActor
{
    GENERATED_BODY()
public:
    AHomeFlock();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boids", meta=(ClampMin="0", ClampMax="5"))
    float Separation = 2.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boids", meta=(ClampMin="0", ClampMax="5"))
    float Alignment = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boids", meta=(ClampMin="0", ClampMax="5"))
    float Cohesion = 0.8f;
    UPROPERTY(EditAnywhere, Category="Boids", meta=(ClampMin="200", ClampMax="2000"))
    float NeighbourRadius = 850;

    UFUNCTION(CallInEditor, BlueprintCallable, Category="Boids")
    void ResetFlock();

private:
    void Step(float Seconds);
    UPROPERTY(VisibleAnywhere)
    TArray<TObjectPtr<UStaticMeshComponent>> Creatures;
    TArray<FVector> Positions;
    TArray<FVector> Velocities;
    TArray<FVector> Starts;
    FVector Home = FVector(-1800, 0, 110);
    FVector Explore = FVector(1800, 0, 110);
    int32 Phase = 0;
    float Accumulator = 0;
    float Elapsed = 0;
    float ReportTime = 0;
    float ClosestPair = MAX_flt;
};
