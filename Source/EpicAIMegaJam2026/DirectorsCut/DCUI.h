// Director's Cut - the cinematography UI kit: crisp text plus the shapes the HUD and the cinema are built from
// (translucent panels with viewfinder corners, clapperboard stripes, film-strip sprockets, film-leader circles).

#pragma once

#include "CoreMinimal.h"

class UCanvas;
class UFont;

namespace DCUI
{
	enum class EAlign : uint8 { Left, Center, Right };

	// Palette.
	inline const FLinearColor Ivory(0.96f, 0.93f, 0.86f);
	inline const FLinearColor Gold(1.f, 0.74f, 0.3f);
	inline const FLinearColor Rec(0.95f, 0.17f, 0.14f);
	inline const FLinearColor Muted(0.68f, 0.68f, 0.72f);
	inline const FLinearColor Good(0.45f, 0.95f, 0.55f);
	inline const FLinearColor Fill(0.02f, 0.02f, 0.03f, 0.62f);
	inline const FLinearColor Hairline(1.f, 1.f, 1.f, 0.14f);

	/** Width and line height of Text at Px pixels. */
	FVector2D Measure(UCanvas* Canvas, UFont* Font, const FString& Text, float Px);

	/**
	 * Crisp text: glyphs are rasterized at the size shown. Px is the font size in pixels; a line wider than MaxWidth
	 * (when > 0) shrinks to fit. Returns the drawn size.
	 */
	FVector2D Text(UCanvas* Canvas, UFont* Font, const FString& Text, float X, float Y, float Px, const FLinearColor& Color,
		EAlign Align = EAlign::Left, bool bShadow = true, float MaxWidth = 0.f, float RenderScale = 1.f);

	/** Breaks Text into lines no wider than MaxWidth at Px pixels, at spaces (a single overlong word gets its own line). */
	void Wrap(UCanvas* Canvas, UFont* Font, const FString& Text, float Px, float MaxWidth, TArray<FString>& OutLines);

	void Rect(UCanvas* Canvas, float X, float Y, float W, float H, const FLinearColor& Color);
	void Outline(UCanvas* Canvas, float X, float Y, float W, float H, float Thickness, const FLinearColor& Color);
	void Line(UCanvas* Canvas, const FVector2D& A, const FVector2D& B, float Thickness, const FLinearColor& Color);
	void Triangle(UCanvas* Canvas, const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Color);
	void Disc(UCanvas* Canvas, const FVector2D& Center, float Radius, const FLinearColor& Color, int32 Segments = 32);
	void Ring(UCanvas* Canvas, const FVector2D& Center, float Radius, float Thickness, const FLinearColor& Color, int32 Segments = 64);
	/** Pie slice from StartDeg (0 = right, clockwise), sweeping SweepDeg. */
	void Wedge(UCanvas* Canvas, const FVector2D& Center, float Radius, float StartDeg, float SweepDeg, const FLinearColor& Color, int32 Segments = 64);
	/** A filled rectangle with rounded corners of the given radius (clamped to fit). */
	void RoundedRect(UCanvas* Canvas, float X, float Y, float W, float H, float Radius, const FLinearColor& Color);

	/** A soft glow along the inside of a rectangle: Color at the edges, fading to clear Depth pixels in. */
	void Vignette(UCanvas* Canvas, float L, float T, float R, float B, float Depth, const FLinearColor& Color);

	/** A four-pointed sparkle. */
	void Sparkle(UCanvas* Canvas, const FVector2D& Center, float Radius, const FLinearColor& Color);

	/** Translucent panel with a hairline edge and gold viewfinder marks in its corners. Unit scales the marks. */
	void Panel(UCanvas* Canvas, float X, float Y, float W, float H, float Unit, const FLinearColor& Fill = DCUI::Fill);
	/** Viewfinder corner marks only. */
	void Corners(UCanvas* Canvas, float X, float Y, float W, float H, float Arm, float Thickness, const FLinearColor& Color);
	/** A clapperboard stick: near-black with slanted ivory stripes. */
	void ClapperStripes(UCanvas* Canvas, float X, float Y, float W, float H);
	/** A strip of film edge: a dark band with sprocket holes. */
	void Sprockets(UCanvas* Canvas, float X, float Y, float W, float H);

	/** Film timecode (minutes:seconds:frames at 24 fps). */
	FString Timecode(float Seconds);
}
