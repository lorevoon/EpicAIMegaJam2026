// Director's Cut - comparing shots by the picture, not the camera.
//
// Two very different cameras can make the same picture (close with a wide lens, or far with a long one), so shots are
// compared on what they show of the scene's characters: where they sit in the frame, how big they appear, and which
// side they're seen from. Shared by the grade (ComputeMatch) and the director's live coaching.

#include "DCMultiCamDirector.h"
#include "DCPerformer.h"
#include "Algo/BinarySearch.h"

void ADCMultiCamDirector::GetSubjectPoints(float Time, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();
	if (ReferenceCut.Num() == 0)
	{
		return;
	}
	// The subjects are the characters of the beat the reference cut is filming at this moment.
	// Callers supply rehearsal scene time, not the slowed reference's film time.
	const int32 Next = Algo::UpperBoundBy(ReferenceCut, Time, &FDCTakeSample::Time);
	const FDCTakeSample& Frame = ReferenceCut[FMath::Clamp(Next - 1, 0, ReferenceCut.Num() - 1)];
	if (!ReferenceShotBeats.IsValidIndex(Frame.Slot))
	{
		return;
	}
	TArray<const ADCPerformer*, TInlineAllocator<4>> Cast;
	for (const FDCAnchor& A : Beats[ReferenceShotBeats[Frame.Slot]].Anchors)
	{
		if (const ADCPerformer* Performer = ::Cast<ADCPerformer>(A.Owner))
		{
			Cast.AddUnique(Performer);
		}
	}
	for (const ADCPerformer* Performer : Cast)
	{
		OutPoints.Add(Performer->GetAnchor(EDCAnchorPart::Head)->GetComponentLocation());
		OutPoints.Add(Performer->GetActorLocation()); // feet
	}
}

FDCFrameReading ADCMultiCamDirector::ReadFrame(const TArray<FVector>& Points, const FVector& CamLocation, const FQuat& CamRotation, float FilmFOV)
{
	FDCFrameReading Reading;
	if (Points.Num() == 0)
	{
		return Reading;
	}
	FVector Centroid = FVector::ZeroVector;
	for (const FVector& P : Points)
	{
		Centroid += P;
	}
	Centroid /= Points.Num();
	Reading.ViewDir = (CamLocation - Centroid).GetSafeNormal();

	// Project onto the film frame: x is -1..1 across its width, y uses the same angular scale.
	const float HalfTan = FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(FilmFOV, 1.f, 170.f) * 0.5f));
	FBox2D Box(ForceInit);
	int32 InFront = 0;
	for (const FVector& P : Points)
	{
		const FVector Local = CamRotation.UnrotateVector(P - CamLocation);
		if (Local.X <= 1.f)
		{
			continue;
		}
		Box += FVector2D(Local.Y / Local.X, Local.Z / Local.X) / HalfTan;
		++InFront;
	}
	Reading.bVisible = InFront == Points.Num();
	if (InFront > 0)
	{
		Reading.Center = Box.GetCenter();
		Reading.Size = FMath::Max(Box.GetSize().GetMax(), 0.02f);
	}
	return Reading;
}

FDCMatch ADCMultiCamDirector::CompareFrames(const FDCFrameReading& Shot, const FDCFrameReading& Ref) const
{
	// 1 - x^2: near misses cost little, the score only drops fast close to the tolerance.
	auto Closeness = [](float Error, float Tolerance) { return 1.f - FMath::Square(FMath::Clamp(Error / Tolerance, 0.f, 1.f)); };
	FDCMatch Result;
	const float Degrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Shot.ViewDir, Ref.ViewDir), -1.f, 1.f)));
	Result.Viewpoint = Closeness(Degrees, MatchViewpointTolerance);
	if (Shot.bVisible && Ref.bVisible)
	{
		Result.Framing = Closeness(FVector2D::Distance(Shot.Center, Ref.Center), MatchFramingTolerance);
		// Size is a ratio: twice as big is as far off as half as big.
		Result.Size = Closeness(FMath::Abs(FMath::Loge(Shot.Size / Ref.Size)), FMath::Loge(MatchSizeTolerance));
	}
	const float Weights = MatchFramingWeight + MatchSizeWeight + MatchViewpointWeight;
	Result.Overall = Weights > 0.f
		? (Result.Framing * MatchFramingWeight + Result.Size * MatchSizeWeight + Result.Viewpoint * MatchViewpointWeight) / Weights : 0.f;
	return Result;
}
