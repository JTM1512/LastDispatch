#include "PerlinHomeWorld.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"

APerlinHomeWorld::APerlinHomeWorld()
{
    PrimaryActorTick.bCanEverTick = true;
    Terrain = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Terrain"));
    SetRootComponent(Terrain);
    Terrain->bUseComplexAsSimpleCollision = true;
    Home = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Home"));
    Home->SetupAttachment(Terrain);
    Enemy = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Enemy"));
    Enemy->SetupAttachment(Terrain);
    Home->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    Enemy->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone")));
    Home->SetRelativeScale3D(FVector(4, 4, 0.3));
    Enemy->SetRelativeScale3D(FVector(1.5, 1.5, 2));
    Home->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Enemy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APerlinHomeWorld::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    Generate();
}

void APerlinHomeWorld::Generate()
{
    FRandomStream Random(Seed);
    const FVector2D Offset(Random.FRandRange(0, 10000), Random.FRandRange(0, 10000));
    // A wide closed road is graded into the terrain, as a tower-defence road would be.
    Route = {FVector(-4000, 0, 0), FVector(-2600, -2300, 0),
        FVector(Random.FRandRange(-500, 500), -2600, 0), FVector(2600, -1800, 0),
        FVector(2800, Random.FRandRange(-300, 300), 0), FVector(2200, 2300, 0),
        FVector(-700, 2600, 0), FVector(-2700, 1800, 0), FVector(-4000, 0, 0)};
    TArray<FVector> V, N;
    TArray<FVector2D> UV;
    TArray<int32> T;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
    constexpr int32 Side = 65;
    float Low = MAX_flt, High = -MAX_flt;
    for (int32 X = 0; X < Side; ++X)
    {
        for (int32 Y = 0; Y < Side; ++Y)
        {
            const FVector P(-4000 + X * 125, -4000 + Y * 125, 0);
            float Distance = MAX_flt;
            for (int32 I = 1; I < Route.Num(); ++I)
                Distance = FMath::Min(Distance, FVector::Dist(P, FMath::ClosestPointOnSegment(P, Route[I-1], Route[I])));
            const FVector2D Sample = Offset + FVector2D(X, Y) * 0.065;
            const float Noise = FMath::PerlinNoise2D(Sample) * 650 + FMath::PerlinNoise2D(Sample * 2) * 180;
            const float Blend = FMath::SmoothStep(240.0f, 650.0f, Distance);
            const float Height = Noise * Blend;
            Low = FMath::Min(Low, Height); High = FMath::Max(High, Height);
            V.Add(FVector(P.X, P.Y, Height));
            UV.Add(FVector2D(X / 64.0, Y / 64.0));
            Colors.Add(FMath::Lerp(FLinearColor(0.42f, 0.26f, 0.10f), FLinearColor(0.10f, 0.30f + Height / 4000, 0.08f), Blend));
            if (X < Side-1 && Y < Side-1)
            {
                const int32 A = X * Side + Y;
                T.Append({A, A+Side, A+Side+1, A, A+Side+1, A+1});
            }
        }
    }
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(V, T, UV, N, Tangents);
    Terrain->CreateMeshSection_LinearColor(0, V, T, N, UV, Colors, Tangents, true);
    if (SurfaceMaterial) Terrain->SetMaterial(0, SurfaceMaterial);
    Home->SetRelativeLocation(Route[0] + FVector(0, 0, 20));
    Enemy->SetRelativeLocation(Route[0] + FVector(0, 0, 110));
    Target = 1; Laps = 0; Travelled = 0;
    UE_LOG(LogTemp, Display, TEXT("Perlin home: seed=%d vertices=%d height range=%.1f..%.1f closed=%d road grade=0"), Seed, V.Num(), Low, High, Route[0].Equals(Route.Last()));
}

void APerlinHomeWorld::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(), TEXT("PerlinSeed="), Seed);
    Generate();
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It) { PC->SetViewTarget(*It); break; }
}

void APerlinHomeWorld::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        const int32 Seeds[] = {1234, 5678, 9012};
        const FKey Keys[] = {EKeys::One, EKeys::Two, EKeys::Three};
        for (int32 I = 0; I < 3; ++I)
            if (PC->WasInputKeyJustPressed(Keys[I])) { Seed = Seeds[I]; Generate(); }
    }
    if (Route.Num() < 2) return;
    float Budget = FMath::Max(DeltaSeconds, 0.0f) * 700;
    while (Budget > 0)
    {
        const FVector Destination = Route[Target] + FVector(0, 0, 110);
        const FVector Current = Enemy->GetRelativeLocation();
        const float Distance = FVector::Dist(Current, Destination);
        const float Step = FMath::Min(Budget, Distance);
        Enemy->SetRelativeLocation(Distance > KINDA_SMALL_NUMBER ? Current + (Destination-Current) * (Step / Distance) : Destination);
        Budget -= Step;
        Travelled += Step;
        if (Distance <= Step + KINDA_SMALL_NUMBER)
        {
            if (++Target == Route.Num())
            {
                Target = 1; ++Laps;
                UE_LOG(LogTemp, Display, TEXT("Perlin home: seed=%d returned home lap=%d"), Seed, Laps);
            }
        }
        else break;
    }
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.1f, FColor::White,
            FString::Printf(TEXT("Will you find your way home?   Seeds: [1] 1234  [2] 5678  [3] 9012\nSeed %d | Returns home: %d | Road grade: 0 degrees"), Seed, Laps));
}

