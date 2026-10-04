#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerrainGenerator.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

/** Flat area blended into the generated heightfield (future roads / command post). */
USTRUCT(BlueprintType)
struct FFlattenZone
{
	GENERATED_BODY()

	/** Local grid coordinates in cm (actor space). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flatten Zone")
	FVector2D Center = FVector2D::ZeroVector;

	/** Full-flatten radius in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flatten Zone", meta = (ClampMin = "0.0"))
	float Radius = 1500.0f;

	/** Width of the soft blend band outside Radius, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flatten Zone", meta = (ClampMin = "0.0"))
	float BlendRadius = 800.0f;
};

UCLASS(Blueprintable)
class LASTDISPATCH_API ATerrainGenerator : public AActor
{
	GENERATED_BODY()

public:
	ATerrainGenerator();

	virtual void BeginPlay() override;

	/** Builds (or rebuilds) the heightfield mesh from the current noise settings and seed. */
	UFUNCTION(BlueprintCallable, Category = "Terrain")
	void GenerateTerrain();

	/** Removes the generated mesh section, leaving the actor ready for a new seed. */
	UFUNCTION(BlueprintCallable, Category = "Terrain")
	void ClearTerrain();

	/** World-space terrain height at a location (for props, pickups and road placement). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terrain")
	float GetHeightAt(const FVector& WorldLocation) const;

protected:
	/** Fractal Perlin noise + flatten-zone blend at grid cell (X, Y). */
	float ComputeTerrainHeight(int32 X, int32 Y) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terrain")
	TObjectPtr<UProceduralMeshComponent> TerrainMesh;

	/** Vertices per side (max 252 per mesh section). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "4", ClampMax = "252"))
	int32 GridSize = 96;

	/** Distance between grid vertices, in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "10.0"))
	float CellSize = 200.0f;

	/** Noise frequency; smaller values create larger features. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.0001"))
	float NoiseScale = 0.004f;

	/** Maximum height deviation in cm (before flatten zones). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	float NoiseAmplitude = 600.0f;

	/** Fractal detail layers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "1", ClampMax = "6"))
	int32 Octaves = 3;

	/** Deterministic layout: the same seed always produces the same terrain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	int32 Seed = 1234;

	/** Flat areas blended into the terrain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	TArray<FFlattenZone> FlattenZones;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	TObjectPtr<UMaterialInterface> TerrainMaterial;

	/** Noise-space offset derived from the seed; makes each seed a different slice of noise. */
	FVector2D NoiseOffset = FVector2D::ZeroVector;

	FRandomStream RandomStream;
};
