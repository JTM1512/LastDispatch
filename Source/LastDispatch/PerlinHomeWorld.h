#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PerlinHomeWorld.generated.h"

class UProceduralMeshComponent;
class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class LASTDISPATCH_API APerlinHomeWorld : public AActor
{
    GENERATED_BODY()
public:
    APerlinHomeWorld();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(CallInEditor, BlueprintCallable, Category="Generation")
    void Generate();

    UPROPERTY(EditAnywhere, Category="Generation")
    int32 Seed = 1234;

    UPROPERTY(EditAnywhere, Category="Generation")
    TObjectPtr<UMaterialInterface> SurfaceMaterial;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UProceduralMeshComponent> Terrain;
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Home;
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Enemy;
    TArray<FVector> Route;
    int32 Target = 1;
    int32 Laps = 0;
    float Travelled = 0;
};
