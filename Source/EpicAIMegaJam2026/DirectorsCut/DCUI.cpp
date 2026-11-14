// Director's Cut - the cinematography UI kit.

#include "DCUI.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Fonts/SlateFontInfo.h"

namespace
{
	/** Slate font sizes are points; at the canvas' 96 DPI one point is 4/3 of a pixel. */
	constexpr float PointsPerPixel = 0.75f;

	UFont* OrDefault(UFont* Font)
	{
		return Font ? Font : GEngine->GetLargeFont();
	}

	/** Measuring scale: the font's legacy size scaled to the requested points. */
	float MeasureScale(UFont* Font, float Px)
	{
		return Px * PointsPerPixel / FMath::Max<float>(Font->LegacyFontSize, 1.f);
	}
}

FVector2D DCUI::Measure(UCanvas* Canvas, UFont* Font, const FString& Text, float Px)
{
	Font = OrDefault(Font);
	const float Scale = MeasureScale(Font, Px);
	float W = 0.f, H = 0.f;
	Canvas->TextSize(Font, Text, W, H, Scale, Scale);
	return FVector2D(W, H);
}

FVector2D DCUI::Text(UCanvas* Canvas, UFont* Font, const FString& Text, float X, float Y, float Px, const FLinearColor& Color,
	EAlign Align, bool bShadow, float MaxWidth)
{
	Font = OrDefault(Font);
	FVector2D Size = Measure(Canvas, Font, Text, Px);
	if (MaxWidth > 0.f && Size.X > MaxWidth)
	{
		Px *= MaxWidth / Size.X;
		Size *= MaxWidth / Size.X;
	}
	if (Align == EAlign::Center)
	{
		X -= Size.X * 0.5f;
	}
	else if (Align == EAlign::Right)
	{
		X -= Size.X;
	}
	// Ask for glyphs at the size they're shown; stretching a base-size bitmap is what makes canvas text look blocky.
	FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), FSlateFontInfo(Font, FMath::Max(Px * PointsPerPixel, 4.f)), Color);
	if (bShadow)
	{
		const float Offset = FMath::Max(1.f, Px * 0.05f);
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.75f * Color.A), FVector2D(Offset, Offset));
	}
	Canvas->DrawItem(Item);
	return Size;
}

void DCUI::Rect(UCanvas* Canvas, float X, float Y, float W, float H, const FLinearColor& Color)
{
	FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(W, H), Color);
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);
}

void DCUI::Outline(UCanvas* Canvas, float X, float Y, float W, float H, float Thickness, const FLinearColor& Color)
{
	Rect(Canvas, X, Y, W, Thickness, Color);
	Rect(Canvas, X, Y + H - Thickness, W, Thickness, Color);
	Rect(Canvas, X, Y + Thickness, Thickness, H - 2.f * Thickness, Color);
	Rect(Canvas, X + W - Thickness, Y + Thickness, Thickness, H - 2.f * Thickness, Color);
}

void DCUI::Line(UCanvas* Canvas, const FVector2D& A, const FVector2D& B, float Thickness, const FLinearColor& Color)
{
	FCanvasLineItem Item(A, B);
	Item.SetColor(Color);
	Item.LineThickness = Thickness;
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void DCUI::Triangle(UCanvas* Canvas, const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Color)
{
	FCanvasTriangleItem Item(A, B, C, GWhiteTexture);
	Item.SetColor(Color);
	Item.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Item);
}

void DCUI::Disc(UCanvas* Canvas, const FVector2D& Center, float Radius, const FLinearColor& Color, int32 Segments)
{
	Wedge(Canvas, Center, Radius, 0.f, 360.f, Color, Segments);
}

void DCUI::Ring(UCanvas* Canvas, const FVector2D& Center, float Radius, float Thickness, const FLinearColor& Color, int32 Segments)
{
	const float Inner = FMath::Max(Radius - Thickness, 0.f);
	for (int32 i = 0; i < Segments; ++i)
	{
		const float A = 2.f * PI * i / Segments;
		const float B = 2.f * PI * (i + 1) / Segments;
		const FVector2D DA(FMath::Cos(A), FMath::Sin(A)), DB(FMath::Cos(B), FMath::Sin(B));
		Triangle(Canvas, Center + DA * Inner, Center + DA * Radius, Center + DB * Radius, Color);
		Triangle(Canvas, Center + DA * Inner, Center + DB * Radius, Center + DB * Inner, Color);
	}
}

void DCUI::Wedge(UCanvas* Canvas, const FVector2D& Center, float Radius, float StartDeg, float SweepDeg, const FLinearColor& Color, int32 Segments)
{
	const int32 Steps = FMath::Max(1, FMath::CeilToInt(Segments * FMath::Abs(SweepDeg) / 360.f));
	for (int32 i = 0; i < Steps; ++i)
	{
		const float A = FMath::DegreesToRadians(StartDeg + SweepDeg * i / Steps);
		const float B = FMath::DegreesToRadians(StartDeg + SweepDeg * (i + 1) / Steps);
		Triangle(Canvas, Center, Center + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Radius, Center + FVector2D(FMath::Cos(B), FMath::Sin(B)) * Radius, Color);
	}
}

void DCUI::Corners(UCanvas* Canvas, float X, float Y, float W, float H, float Arm, float Thickness, const FLinearColor& Color)
{
	const float R = X + W, B = Y + H;
	Rect(Canvas, X, Y, Arm, Thickness, Color);          Rect(Canvas, X, Y, Thickness, Arm, Color);
	Rect(Canvas, R - Arm, Y, Arm, Thickness, Color);    Rect(Canvas, R - Thickness, Y, Thickness, Arm, Color);
	Rect(Canvas, X, B - Thickness, Arm, Thickness, Color); Rect(Canvas, X, B - Arm, Thickness, Arm, Color);
	Rect(Canvas, R - Arm, B - Thickness, Arm, Thickness, Color); Rect(Canvas, R - Thickness, B - Arm, Thickness, Arm, Color);
}

void DCUI::Panel(UCanvas* Canvas, float X, float Y, float W, float H, float Unit, const FLinearColor& InFill)
{
	Rect(Canvas, X, Y, W, H, InFill);
	Outline(Canvas, X, Y, W, H, 1.f, Hairline);
	Corners(Canvas, X - 3.f * Unit, Y - 3.f * Unit, W + 6.f * Unit, H + 6.f * Unit, 16.f * Unit, FMath::Max(2.f * Unit, 1.f), Gold);
}

void DCUI::ClapperStripes(UCanvas* Canvas, float X, float Y, float W, float H)
{
	Rect(Canvas, X, Y, W, H, FLinearColor(0.035f, 0.035f, 0.04f, 0.96f));
	// Slanted stripes, clipped to the stick at its two ends.
	const float Stripe = H * 1.1f;
	const float R = X + W;
	for (float S = X - H; S < R; S += 2.f * Stripe)
	{
		auto Clip = [X, R](float V) { return FMath::Clamp(V, X, R); };
		const FVector2D A(Clip(S), Y + H), B(Clip(S + Stripe), Y + H), C(Clip(S + Stripe + H), Y), D(Clip(S + H), Y);
		Triangle(Canvas, A, B, C, Ivory);
		Triangle(Canvas, A, C, D, Ivory);
	}
	Outline(Canvas, X, Y, W, H, 1.f, FLinearColor(0.f, 0.f, 0.f, 0.8f));
}

void DCUI::Sprockets(UCanvas* Canvas, float X, float Y, float W, float H)
{
	Rect(Canvas, X, Y, W, H, FLinearColor(0.015f, 0.015f, 0.02f, 0.9f));
	const float HoleW = H * 0.55f, HoleH = H * 0.5f, Step = H * 1.25f;
	for (float HX = X + Step * 0.4f; HX + HoleW < X + W; HX += Step)
	{
		Rect(Canvas, HX, Y + (H - HoleH) * 0.5f, HoleW, HoleH, FLinearColor(1.f, 1.f, 1.f, 0.16f));
	}
}

FString DCUI::Timecode(float Seconds)
{
	const int32 Frames = FMath::Max(0, FMath::FloorToInt(Seconds * 24.f));
	return FString::Printf(TEXT("%02d:%02d:%02d"), Frames / (24 * 60), (Frames / 24) % 60, Frames % 24);
}
