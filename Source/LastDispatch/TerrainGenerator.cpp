#include "TerrainGenerator.h"

#include "ProceduralMeshComponent.h"

ATerrainGenerator::ATerrainGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

	TerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMesh"));
	RootComponent = TerrainMesh;

	TerrainMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TerrainMesh->SetCollisionObjectType(ECC_WorldStatic);
	TerrainMesh->bUseComplexAsSimpleCollision = true;
}

void ATerrainGenerator::BeginPlay()
{
	Super::BeginPlay();

	RandomStream.Initialize(Seed);
	NoiseOffset = FVector2D(RandomStream.FRand() * 10000.0f, RandomStream.FRand() * 10000.0f);

	GenerateTerrain();
}

float ATerrainGenerator::ComputeTerrainHeight(int32 X, int32 Y) const
{
	float Height = 0.0f;
	float Amp = NoiseAmplitude;
	float Freq = NoiseScale;

	for (int32 Octave = 0; Octave < Octaves; ++Octave)
	{
		const FVector2D Position(X * Freq + NoiseOffset.X, Y * Freq + NoiseOffset.Y);
		Height += FMath::PerlinNoise2D(Position) * Amp;

		Amp *= 0.5f;
		Freq *= 2.0f;
	}

	const float HalfGrid = static_cast<float>(GridSize - 1) * 0.5f;
	const float LocalX = (static_cast<float>(X) - HalfGrid) * CellSize;
	const float LocalY = (static_cast<float>(Y) - HalfGrid) * CellSize;

	for (const FFlattenZone& Zone : FlattenZones)
	{
		const float Dist = FVector2D::Distance(Zone.Center, FVector2D(LocalX, LocalY));
		if (Dist < Zone.Radius + Zone.BlendRadius)
		{
			const float Inside = FMath::Min(Dist, Zone.Radius);
			const float Blend = FMath::Min((Dist - Inside) / FMath::Max(Zone.BlendRadius, 1.0f), 1.0f);
			Height *= 1.0f - Blend;
		}
	}

	return Height;
}

void ATerrainGenerator::GenerateTerrain()
{
	if (!TerrainMesh || GridSize < 2)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<FVector2D> UVs;
	TArray<int32> Triangles;

	Vertices.Reserve(GridSize * GridSize);
	UVs.Reserve(GridSize * GridSize);
	Triangles.Reserve((GridSize - 1) * (GridSize - 1) * 6);

	const float HalfGrid = static_cast<float>(GridSize - 1) * 0.5f;

	for (int32 X = 0; X < GridSize; ++X)
	{
		for (int32 Y = 0; Y < GridSize; ++Y)
		{
			const float LocalX = (static_cast<float>(X) - HalfGrid) * CellSize;
			const float LocalY = (static_cast<float>(Y) - HalfGrid) * CellSize;

			Vertices.Add(FVector(LocalX, LocalY, ComputeTerrainHeight(X, Y)));
			UVs.Add(FVector2D(X * 0.5f, Y * 0.5f));
		}
	}

	for (int32 X = 0; X < GridSize - 1; ++X)
	{
		for (int32 Y = 0; Y < GridSize - 1; ++Y)
		{
			const int32 A = X * GridSize + Y;
			const int32 B = A + GridSize;
			const int32 C = A + 1;
			const int32 D = B + 1;

			// Winding chosen so both triangles face +Z (visible from above).
			Triangles.Add(A);
			Triangles.Add(B);
			Triangles.Add(D);
			Triangles.Add(A);
			Triangles.Add(D);
			Triangles.Add(C);
		}
	}

	// Empty normals/tangents/colors are auto-computed by the mesh component.
	TArray<FVector> Normals;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	TerrainMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, /*bCreateCollision=*/true);

	if (TerrainMaterial)
	{
		TerrainMesh->SetMaterial(0, TerrainMaterial);
	}
}

void ATerrainGenerator::ClearTerrain()
{
	if (TerrainMesh)
	{
		TerrainMesh->ClearMeshSection(0);
	}
}

float ATerrainGenerator::GetHeightAt(const FVector& WorldLocation) const
{
	const FVector Local = WorldLocation - GetActorLocation();

	const float GridX = FMath::Clamp(Local.X / CellSize + static_cast<float>(GridSize - 1) * 0.5f, 0.0f, static_cast<float>(GridSize - 1));
	const float GridY = FMath::Clamp(Local.Y / CellSize + static_cast<float>(GridSize - 1) * 0.5f, 0.0f, static_cast<float>(GridSize - 1));

	return ComputeTerrainHeight(FMath::RoundToInt(GridX), FMath::RoundToInt(GridY));
}
