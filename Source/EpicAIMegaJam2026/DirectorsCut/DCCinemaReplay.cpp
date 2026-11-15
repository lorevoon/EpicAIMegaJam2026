#include "DCCinemaReplay.h"
#include "DCBeat.h"
#include "DCMultiCamDirector.h"
#include "DCUI.h"
#include "DCZombieDirector.h"
#include "DCZombieHUD.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "CanvasItem.h"
#include "Algo/BinarySearch.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/Interface_PostProcessVolume.h"
#include "SceneInterface.h"

namespace
{
 /** Copies every setting Src overrides onto Dst (and marks it overridden there), like a volume at full weight. */
 void ApplyOverrides(const FPostProcessSettings& Src, FPostProcessSettings& Dst)
 {
  const UScriptStruct* Struct = FPostProcessSettings::StaticStruct();
  for (TFieldIterator<FBoolProperty> It(Struct); It; ++It)
  {
   const FString Flag = It->GetName();
   if (!Flag.StartsWith(TEXT("bOverride_")) || !It->GetPropertyValue_InContainer(&Src)) continue;
   const FString Name = Flag.RightChop(10);
   const FProperty* Value = Struct->FindPropertyByName(FName(*Name));
   Value = Value ? Value : Struct->FindPropertyByName(FName(TEXT("b") + Name));
   if (!Value) continue;
   It->SetPropertyValue_InContainer(&Dst, true);
   Value->CopyCompleteValue_InContainer(&Dst, &Src);
  }
  for (const FWeightedBlendable& Blendable : Src.WeightedBlendables.Array)
  {
   if (Blendable.Object && Blendable.Weight > 0.f) Dst.WeightedBlendables.Array.Add(Blendable);
  }
 }

 /**
  * Scene captures ignore post-process volumes - they render with their own settings only, and switch Lumen off unless
  * told otherwise - while the main view blends every volume it is in. Build the settings the main view would have at
  * Location (project lighting defaults, then each volume by priority, then the camera's own), so the projected film
  * looks exactly like the reference cut: same grade, exposure, post-process materials and lighting.
  */
 FPostProcessSettings MainViewSettings(UWorld* World, const FVector& Location, const FPostProcessSettings& CameraSettings)
 {
  FPostProcessSettings PP;
  PP.bOverride_DynamicGlobalIlluminationMethod = true;
  PP.DynamicGlobalIlluminationMethod = (EDynamicGlobalIlluminationMethod::Type)
   IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicGlobalIlluminationMethod"))->GetInt();
  PP.bOverride_ReflectionMethod = true;
  PP.ReflectionMethod = (EReflectionMethod::Type)IConsoleManager::Get().FindConsoleVariable(TEXT("r.ReflectionMethod"))->GetInt();
  PP.bOverride_LumenSurfaceCacheResolution = true; // captures default to half resolution
  PP.LumenSurfaceCacheResolution = 1.f;
  for (IInterface_PostProcessVolume& Volume : World->GetPostProcessVolumeIterator()) // ascending priority
  {
   const FPostProcessVolumeProperties Props = Volume.GetProperties();
   if (Props.bIsEnabled && Props.Settings && Props.BlendWeight > 0.f
    && (Props.bIsUnbound || Volume.EncompassesPoint(Location, 0.f, nullptr)))
   {
    ApplyOverrides(*Props.Settings, PP);
   }
  }
  ApplyOverrides(CameraSettings, PP);
  return PP;
 }

 const FLinearColor Gold(.88f,.65f,.30f), Ivory(.96f,.91f,.8f), Ink(.012f,.012f,.022f);
 const FLinearColor Green(.32f,.88f,.57f), Red(.98f,.36f,.36f);
 // Draw at a fixed design size, uniformly fitted for ultrawide and small windows alike.
 struct FScreen
 {
  AHUD* HUD; UCanvas* Canvas; UFont* Font; float Scale, OX, OY, TextScale;
  // Text is drawn larger than the original design sizes, in the HUD's typeface.
  static constexpr float TextBoost=1.7f;
  FScreen(ADCZombieHUD* H,UCanvas* C):HUD(H),Canvas(C)
  {
   Font=H->GetTextFont();
   TextScale=H->TextScale;
   Scale=FMath::Min(C->ClipX/1920.f,C->ClipY/1080.f);
   OX=(C->ClipX-1920*Scale)*.5f; OY=(C->ClipY-1080*Scale)*.5f;
  }
  void Box(float X,float Y,float W,float H,FLinearColor C) const { HUD->DrawRect(C,OX+X*Scale,OY+Y*Scale,W*Scale,H*Scale); }
  void Line(float X,float Y,float X2,float Y2,FLinearColor C,float Width=2) const { HUD->DrawLine(OX+X*Scale,OY+Y*Scale,OX+X2*Scale,OY+Y2*Scale,C,Width*Scale); }
  void Triangle(FVector2D A,FVector2D B,FVector2D C,FLinearColor Color) const
  {
   FCanvasUVTri T;
   T.V0_Pos=FVector2D(OX,OY)+A*Scale; T.V1_Pos=FVector2D(OX,OY)+B*Scale; T.V2_Pos=FVector2D(OX,OY)+C*Scale;
   T.V0_UV=T.V1_UV=T.V2_UV=FVector2D::ZeroVector;
   T.V0_Color=T.V1_Color=T.V2_Color=Color;
   FCanvasTriangleItem Item(T,GWhiteTexture); Item.BlendMode=SE_BLEND_Translucent; Canvas->DrawItem(Item);
  }
  void Oval(float X,float Y,float RX,float RY,FLinearColor C) const
  {
   for(int i=0;i<32;++i)
   {
    const float A=i*2*PI/32, B=(i+1)*2*PI/32;
    Triangle({X,Y},{X+FMath::Cos(A)*RX,Y+FMath::Sin(A)*RY},{X+FMath::Cos(B)*RX,Y+FMath::Sin(B)*RY},C);
   }
  }
  // Crisp text (glyphs rasterized at the size shown), shrunk to MaxWidth when needed.
  void Text(const FString& T,float X,float Y,float Size,FLinearColor C,float MaxWidth=1700,bool Center=true,bool Right=false) const
  {
   DCUI::Text(Canvas,Font,T,OX+X*Scale,OY+Y*Scale,Size*32.f*TextBoost*Scale*TextScale,C,
    Right?DCUI::EAlign::Right:Center?DCUI::EAlign::Center:DCUI::EAlign::Left,true,MaxWidth*Scale);
  }
  void Stripes(float X,float Y,float W,float H) const { DCUI::ClapperStripes(Canvas,OX+X*Scale,OY+Y*Scale,W*Scale,H*Scale); }
  void Ring(float X,float Y,float R,float T,FLinearColor C) const { DCUI::Ring(Canvas,FVector2D(OX+X*Scale,OY+Y*Scale),R*Scale,T*Scale,C); }
  // Font-independent emoji silhouettes: no missing Unicode glyphs in packaged builds.
  void Emoji(float X,float Y,int Kind,float Alpha,float Pop) const
  {
   const float R=26*Pop;
   FLinearColor Bubble(.045f,.035f,.055f,Alpha*.96f);
   Oval(X,Y,R+9,R+9,Bubble);
   FLinearColor C=Kind==0?Green:Kind==1?Red:Kind==2?FLinearColor(1,.27f,.46f):FLinearColor(1,.78f,.24f); C.A=Alpha;
   if(Kind==2)
   {
    Oval(X-R*.36f,Y-R*.2f,R*.52f,R*.52f,C); Oval(X+R*.36f,Y-R*.2f,R*.52f,R*.52f,C);
    Triangle({X-R*.83f,Y-R*.06f},{X+R*.83f,Y-R*.06f},{X,Y+R*.95f},C);
   }
   else if(Kind==3)
   {
    Oval(X,Y,R,R,C); const FLinearColor Face(.16f,.09f,.06f,Alpha), Tear(.25f,.72f,1,Alpha);
    Line(X-R*.65f,Y-R*.32f,X-R*.17f,Y-R*.17f,Face,3);
    Line(X+R*.17f,Y-R*.17f,X+R*.65f,Y-R*.32f,Face,3);
    Oval(X-R*.42f,Y+R*.25f,R*.13f,R*.45f,Tear); Oval(X+R*.42f,Y+R*.25f,R*.13f,R*.45f,Tear);
    Oval(X,Y+R*.45f,R*.19f,R*.23f,Face);
   }
   else
   {
    const float Sign=Kind==0?1.f:-1.f;
    auto P=[&](float PX,float PY){ return FVector2D(X+PX*R,Y+PY*R*Sign); };
    // Wrist, four fingers and raised/lowered thumb.
    Box(X-R*.85f,Y+(Kind==0?-.08f:-.69f)*R,R*.28f,R*.77f,C);
    Triangle(P(-.45f,.65f),P(.65f,.65f),P(.85f,-.2f),C);
    Triangle(P(-.45f,.65f),P(.85f,-.2f),P(-.25f,-.25f),C);
    Triangle(P(-.25f,-.25f),P(.13f,-1.f),P(.4f,-.85f),C);
    Triangle(P(-.25f,-.25f),P(.4f,-.85f),P(.25f,.3f),C);
   }
  }
 };
}

ADCCinemaReplay::ADCCinemaReplay()
{
 PrimaryActorTick.bCanEverTick=true;
 Projector=CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("FilmProjector"));
 SetRootComponent(Projector);
 Projector->bCaptureEveryFrame=false;
 Projector->bCaptureOnMovement=false;
 Projector->bAlwaysPersistRenderingState=true;
 Projector->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR;
 // Render the film like the main view did when the reference cut played: captures leave temporal anti-aliasing
 // (which Lumen and the depth of field lean on) and separate translucency (fire, smoke) off by default.
 Projector->ShowFlags.SetTemporalAA(true);
 Projector->ShowFlags.SetSeparateTranslucency(true);
}

void ADCCinemaReplay::BeginScreening(ADCMultiCamDirector* InDirector)
{
	if (Director) RemoveTickPrerequisiteActor(Director);
	if (!bActive)
	{
		bPreviousWorldRenderingEnabled = UGameplayStatics::GetEnableWorldRendering(this);
		bOwnsWorldRenderingOverride = true;
	}
 Director=InDirector; PresentationTime=0; ResultAge=-1; bActive=true;
	AddTickPrerequisiteActor(InDirector);
 if(!FilmTexture)
 {
  FilmTexture=NewObject<UTextureRenderTarget2D>(this);
  FilmTexture->ClearColor=FLinearColor::Black;
  // Linear float, not sRGB 8-bit: the canvas gamma-corrects whatever it draws, so the film must arrive linear. With
  // an sRGB target the tonemapped picture was gamma-encoded twice - lifted blacks, washed-out pastel colours.
  FilmTexture->InitCustomFormat(1600,900,PF_FloatRGBA,true);
  FilmTexture->UpdateResourceImmediate(true);
 }
 Projector->TextureTarget=FilmTexture;
	// DrawCinema covers the viewport with an opaque presentation. Rendering the same world beneath it
	// duplicated the projector's most expensive lighting and post-processing work every frame.
	UGameplayStatics::SetEnableWorldRendering(this, false);
}
void ADCCinemaReplay::EndScreening()
{
	if (Director) RemoveTickPrerequisiteActor(Director);
 bActive=false; Director=nullptr; Projector->TextureTarget=nullptr; ResultAge=-1;
	FramesToSettle=0;
	if (bOwnsWorldRenderingOverride)
	{
		UGameplayStatics::SetEnableWorldRendering(this, bPreviousWorldRenderingEnabled);
		bOwnsWorldRenderingOverride = false;
	}
}
void ADCCinemaReplay::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndScreening();
	Super::EndPlay(EndPlayReason);
}
void ADCCinemaReplay::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds);
 if(!bActive || !Director) return;
 // Coalesce scrubs and multiple pose evaluations into one render per engine frame.
 if(FramesToSettle>0 || (IsOpening() && !Director->GetRecording().IsEmpty()))
 {
  // The scene's frame number only advances when the main view renders the world, and the screening switches that
  // off. Frozen, every projector frame looks like "already updated this frame" to the renderer: the global distance
  // field stops updating while its list of moved objects grows without end, and reallocating that ever larger list
  // caused the growing hitches (GlobalDistanceFieldUpdate, 30 ms climbing past 500 ms). Advance it per film frame.
  if(!UGameplayStatics::GetEnableWorldRendering(this) && GetWorld()->Scene) GetWorld()->Scene->IncrementFrameNumber();
  Projector->CaptureScene();
  FramesToSettle=FMath::Max(0,FramesToSettle-1);
 }
 PresentationTime+=DeltaSeconds;
 if(Director->IsFilmFinished()) ResultAge=FMath::Max(0.f,ResultAge)+DeltaSeconds;
 else ResultAge=-1;
}
void ADCCinemaReplay::ProjectFrame(ACameraActor* Camera)
{
 if(!bActive || !Camera || !FilmTexture) return;
 Projector->SetWorldLocationAndRotation(Camera->GetActorLocation(),Camera->GetActorRotation());
 Projector->FOVAngle=Camera->GetCameraComponent()->FieldOfView;
 Projector->PostProcessSettings=MainViewSettings(GetWorld(),Camera->GetActorLocation(),Camera->GetCameraComponent()->PostProcessSettings);
 Projector->PostProcessBlendWeight=1.f;
 // Active playback requests a frame every tick. Two trailing captures are enough to refresh temporal
 // history after a pause, scrub, or cut without rendering a static result for almost another second.
 FramesToSettle=2;
}
void ADCCinemaReplay::SetMissionOutcome(FName BeatId,bool bSucceeded) { MissionOutcomes.Add(BeatId,bSucceeded); }
bool ADCCinemaReplay::HasPositiveReaction(FName BeatId,bool bStoryboardCaptured) const
{
 const bool* Mission=MissionOutcomes.Find(BeatId);
 return bStoryboardCaptured && (!Mission || *Mission);
}
float ADCCinemaReplay::SceneToFilmTime(const ADCMultiCamDirector* Film,float Seconds)
{
 const auto& Rec=Film->GetRecording();
 if(Rec.IsEmpty()) return 0;
 const int32 Next=Algo::LowerBoundBy(Rec,Seconds,&FDCTakeSample::Time);
 if(Next==0) return Rec[0].FilmTime;
 if(Next>=Rec.Num()) return Rec.Last().FilmTime;
 const auto& A=Rec[Next-1]; const auto& B=Rec[Next];
 if(B.bJumpCut) return B.FilmTime; // omitted mission windows surface at the next visible frame
 return FMath::Lerp(A.FilmTime,B.FilmTime,FMath::Clamp((Seconds-A.Time)/FMath::Max(B.Time-A.Time,SMALL_NUMBER),0.f,1.f));
}

void ADCCinemaReplay::DrawCinema(ADCZombieHUD* HUD,UCanvas* Canvas,ADCZombieDirector* Film)
{
 FScreen P(HUD,Canvas);
 HUD->DrawRect(FLinearColor::Black,0,0,Canvas->ClipX,Canvas->ClipY);
 P.Box(0,0,1920,1080,Ink);
 const float Time=Film->GetSceneTime();
 const bool Finished=Film->IsFilmFinished();
 const float Open=Finished?1.f:FMath::SmoothStep(.8f,FMath::Max(OpeningDuration,.81f),PresentationTime);
 // Warm wall panels and pleated velvet curtains.
 P.Box(0,0,1920,810,FLinearColor(.095f,.019f,.029f));
 for(int i=0;i<64;++i)
 {
  const float Shade=(.06f+.05f*(.5f+.5f*FMath::Sin(i*1.7f)))*FMath::Lerp(1.45f,1.f,Open);
  P.Box(i*30,0,30,820,FLinearColor(Shade,.005f,.012f));
  P.Box(i*30,0,3,820,FLinearColor(.045f,.008f,.014f));
 }
 P.Box(190,67,1540,714,FLinearColor::Black);
 // The projected picture retains its 16:9 lens framing; it is never stretched.
 const float SX=382.2f,SY=96,SW=1155.6f,SH=650;
 if(FilmTexture && !Film->GetRecording().IsEmpty())
  HUD->DrawTexture(FilmTexture,P.OX+SX*P.Scale,P.OY+SY*P.Scale,SW*P.Scale,SH*P.Scale,0,0,1,1,FLinearColor::White,BLEND_Opaque);
 else P.Text(TEXT("NO FOOTAGE RECORDED"),960,380,1.1f,Ivory,950);
 // Curtains physically part from the middle; the film clock waits until this ends.
 if(Open<1)
 {
  const float CW=SW*.5f*(1-Open);
  for(int Side=0;Side<2;++Side)
  {
   const float CX=Side?SX+SW-CW:SX;
   P.Box(CX,SY,CW,SH,FLinearColor(.24f,.012f,.032f));
   for(float X=0;X<CW;X+=18) P.Box(CX+X,SY,FMath::Min(7.f,CW-X),SH,FLinearColor(.12f,.006f,.016f));
   P.Box(Side?CX:CX+CW-3,SY,3,SH,FLinearColor::Black);
  }
  const float A=1-FMath::SmoothStep(.7f,2.5f,PresentationTime);
  P.Text(Film->MovieTitle,960,337,1.6f,FLinearColor(.96f,.91f,.8f,A),1000);
  P.Text(TEXT("A FILM BY YOU"),960,408,.55f,FLinearColor(.88f,.65f,.3f,A),800);
 }
 // Resolve reactions deterministically from film time, including scrubs and hard pause cuts.
 int Verdict=-1, Mood=-1; float VerdictAge=100, MoodAge=100;
 for(const auto& State:Film->GetBeats())
 {
  if(!State.Beat) continue;
  const bool Captured=State.Status==EDCShotStatus::Captured;
  const float Cue=Captured?State.CapturedFilmTime:SceneToFilmTime(Film,State.Beat->WindowEnd);
  const float Age=Time-Cue;
  if(Age>=0 && Age<ReactionDuration && Age<VerdictAge)
  {
   Verdict=HasPositiveReaction(FName(*State.Beat->BeatId),Captured)?0:1;
   VerdictAge=Age;
  }
  const float Start=SceneToFilmTime(Film,State.Beat->GetCaptureStartTime());
  const float End=SceneToFilmTime(Film,State.Beat->WindowEnd);
  if(State.Beat->ReplayMood!=EDCCinemaMood::Neutral && Time>=Start && Time<End && Time-Start<MoodAge)
  { Mood=State.Beat->ReplayMood==EDCCinemaMood::Love?2:3; MoodAge=Time-Start; }
 }
 P.Box(0,790,1920,290,FLinearColor(.018f,.012f,.022f));
 // Three staggered rows of seated spectators. Chair backs, shoulders and heads stay distinct.
 for(int Row=0;Row<3;++Row)
 {
  const int Count=Row==0?18:Row==1?15:12;
  const float Step=1920.f/Count, BaseY=825+Row*116.f, Size=1+Row*.28f;
  for(int i=0;i<Count;++i)
  {
   const float X=(i+.5f)*Step+(Row%2?Step*.19f:0);
   const float Bob=FMath::Sin(PresentationTime*1.1f+i*2.3f+Row)*1.5f;
   const float Y=BaseY+Bob+(i%3)*5;
   const FLinearColor Silhouette(.008f+Row*.004f,.009f+Row*.003f,.016f+Row*.005f);
   P.Oval(X,Y+22*Size,43*Size,45*Size,Silhouette);
   P.Oval(X,Y-40*Size,20*Size,28*Size,Silhouette);
   P.Oval(X,Y+66*Size,43*Size,49*Size,FLinearColor(.048f-Row*.009f,.021f,.031f));
   P.Box(X-43*Size,Y+65*Size,86*Size,76*Size,FLinearColor(.035f-Row*.006f,.016f,.025f));
   P.Line(X-35*Size,Y+40*Size,X-35*Size,Y+100*Size,FLinearColor(.10f,.035f,.046f),1);
   if((!Finished || ResultAge<ReactionDuration) && Open>=1 && Row<2)
   {
    const bool ShowVerdict=Verdict>=0 && (Mood<0 || i%3!=0);
    const int Kind=ShowVerdict?Verdict:Mood;
    const float Age=(ShowVerdict?VerdictAge:FMath::Fmod(MoodAge,ReactionDuration))-(i%4)*.09f;
    if(Kind>=0 && Age>=0 && Age<ReactionDuration-.3f && (i+Row)%2==0)
    {
     const float Fade=FMath::Min(1.f,Age*6)*FMath::Clamp((ReactionDuration-.3f-Age)*3,0.f,1.f);
     P.Emoji(X,Y-91*Size-Age*6,Kind,Fade,.75f+FMath::Min(Age*3,.25f));
    }
   }
  }
 }
 // Evaluation rises over the screen after playback.
 if(Finished)
 {
  // The evaluation arrives on a clapperboard: striped stick, then the slate with the grade, the match and the scenes.
  const float Age=FMath::Max(ResultAge,0.f), Reveal=FMath::SmoothStep(0.f,.7f,Age);
  const float X=300, W=1320, H=660, Y=96+(1-Reveal)*35;
  const float CX=X+W*.5f;
  if(Reveal>.05f) P.Stripes(X,Y,W*Reveal,46);
  P.Box(X,Y+52,W,H-52,FLinearColor(.02f,.02f,.03f,.96f*Reveal));
  P.Box(X,Y+52,W*Reveal,3,Gold);
  if(Age>.25f)
  {
   P.Text(Film->GetRecording().IsEmpty()?TEXT("THE SCREEN STAYED DARK"):TEXT("THE AUDIENCE HAS SPOKEN"),CX,Y+72,.5f,Gold,1100);
   P.Text(Film->MovieTitle,CX,Y+112,.95f,Ivory,1150);
  }
  const FDCMatch& Match=Film->GetMatch();
  const bool Good=Film->IsGoodTake();
  const FLinearColor GradeColor=Good?Gold:Red;
  // Left: the grade letter in a ring, the cut's name under it.
  const float GX=X+210, GY=Y+370;
  if(Age>.8f)
  {
   P.Ring(GX,GY,125,7,GradeColor);
   P.Text(Film->GetGrade(),GX,GY-112,3.2f,GradeColor,220);
   P.Text(Film->GetCutName(),GX,GY+150,.5f,Ivory,380);
  }
  // Right: the match, its three parts as bars, then the scenes.
  const float MX=X+460, MW=W-500;
  const auto& Beats=Film->GetBeats();
  const float ScoreStart=1.1f;
  if(Age>ScoreStart)
  {
   const float Count=FMath::SmoothStep(0.f,1.2f,Age-ScoreStart);
   P.Text(FString::Printf(TEXT("MATCH  %d%%"),FMath::RoundToInt(Match.Overall*100*Count)),MX,Y+190,1.15f,GradeColor,MW*.6f,false);
   P.Text(FString::Printf(TEXT("best %d%%"),FMath::RoundToInt(Film->GetBestMatch()*100)),MX+MW,Y+215,.5f,Ivory,MW*.35f,false,true);
   const TPair<const TCHAR*,float> Parts[]={{TEXT("Framing"),Match.Framing},{TEXT("Size"),Match.Size},{TEXT("Viewpoint"),Match.Viewpoint}};
   for(int i=0;i<3;++i)
   {
    const float RY=Y+300+i*52;
    P.Text(Parts[i].Key,MX,RY,.48f,Ivory,220,false);
    P.Box(MX+240,RY+14,MW-360,14,FLinearColor(1,1,1,.12f));
    P.Box(MX+240,RY+14,(MW-360)*FMath::Clamp(Parts[i].Value*Count,0.f,1.f),14,Gold);
    P.Text(FString::Printf(TEXT("%d%%"),FMath::RoundToInt(Parts[i].Value*100*Count)),MX+MW,RY,.48f,Ivory,100,false,true);
   }
  }
  const float RowH=FMath::Min(40.f,170.f/FMath::Max(1,Beats.Num()));
  for(int i=0;i<Beats.Num();++i)
  {
   const auto& B=Beats[i]; if(!B.Beat || Age<1.4f+i*.14f) continue;
   const float RY=Y+468+i*RowH;
   P.Text(FString::Printf(TEXT("SC %d   %s"),i+1,*B.Beat->DisplayName),MX,RY,FMath::Min(.44f,RowH/62),Ivory,MW,false);
  }
 }
}
