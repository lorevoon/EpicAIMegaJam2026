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

 // ---------------------------------------------------------------------------------------------
 // Audience comments (plain ASCII: the HUD font has no emoji glyphs)

 enum ECommentKind { CK_Good, CK_Bad, CK_Love, CK_Sad, CK_Action, CK_Idle, CK_Num };

 /** How long a comment bubble stays up, in film seconds. */
 constexpr float CommentLife = 3.4f;

 const TArray<const TCHAR*>& CommentPool(int32 Kind)
 {
  static const TArray<const TCHAR*> Pools[CK_Num] = {
   { // CK_Good: the shot landed
    TEXT("Cinema. Absolute cinema."), TEXT("Who shot this? Give them an Oscar."), TEXT("Okay, I'm invested now."),
    TEXT("That angle... *chef's kiss*"), TEXT("Ooh, cinematic!"), TEXT("Framed like a painting!"),
    TEXT("Now THAT is filmmaking."), TEXT("Goosebumps. Actual goosebumps."), TEXT("The camera person deserves a raise."),
    TEXT("10 out of 10, no notes."), TEXT("I'd frame that and hang it on my wall."), TEXT("Spielberg is shaking right now."),
    TEXT("See? Clay CAN act."), TEXT("Peak cinema, right there."), TEXT("I'm telling all my friends about this."),
    TEXT("This is better than the trailer!"), TEXT("Wow. Just... wow."), TEXT("I felt that one in my soul."),
    TEXT("Whoever placed that camera is a genius."), TEXT("Instant classic."), TEXT("They nailed that shot!"),
    TEXT("I'd watch this twice."),
   },
   { // CK_Bad: the shot missed the scene
    TEXT("Uhhh... what are we looking at?"), TEXT("Is the camera... okay?"), TEXT("I think the camera guy sneezed."),
    TEXT("Wait, where did everyone go?"), TEXT("That's... a nice rock, I guess."), TEXT("Riveting shot of the floor."),
    TEXT("I paid for this?"), TEXT("Bold choice. Wrong, but bold."), TEXT("Can someone wake up the camera guy?"),
    TEXT("Am I watching the blooper reel?"), TEXT("Who's even in this scene?"), TEXT("Plot twist: it all happened off-screen."),
    TEXT("My grandma films better than this."), TEXT("Was that important? I couldn't see."), TEXT("...is it buffering?"),
    TEXT("So we're just staring at a wall now?"), TEXT("Very avant-garde. Very confusing."), TEXT("I want a refund. Or popcorn."),
    TEXT("Director's cut or director's mistake?"), TEXT("Something happened. Probably."), TEXT("The action is THAT way!!"),
    TEXT("Did the camera fall asleep?"),
   },
   { // CK_Love: a romantic scene
    TEXT("Kissing? Gross!"), TEXT("Ewww, they're being romantic."), TEXT("Get a room!"), TEXT("Awww, they're so cute together!"),
    TEXT("I ship it. I SHIP IT."), TEXT("*covers kid's eyes*"), TEXT("Mom, why are they hugging so long?"),
    TEXT("My heart can't take this."), TEXT("Ugh, I wish someone looked at me like that."), TEXT("So cheesy. ...I love it."),
    TEXT("*aggressive squealing*"), TEXT("Not the forehead touch!!"), TEXT("Clay romance just hits different."),
    TEXT("Wait, how do clay people even kiss?"), TEXT("Kiss already!!"), TEXT("Aww, true love!"),
    TEXT("*throws popcorn in excitement*"), TEXT("Somebody's in looove~"), TEXT("I'm not blushing, YOU'RE blushing."),
   },
   { // CK_Sad: a sad scene
    TEXT("*sobs*"), TEXT("*sniffles loudly*"), TEXT("NOOOOO!"), TEXT("Why would you DO this to us?"),
    TEXT("I'm fine. I'm totally fine. *sobs*"), TEXT("Pass the tissues. All of them."), TEXT("My heart just broke into clay pieces."),
    TEXT("Who's cutting onions in here?"), TEXT("I wasn't ready for this."), TEXT("Not like this... NOT LIKE THIS."),
    TEXT("*ugly crying*"), TEXT("The director is a monster."), TEXT("I need a hug."), TEXT("This is worse than my breakup."),
    TEXT("Somebody hold me."), TEXT("I came here to have FUN!!"), TEXT("*blows nose very loudly*"), TEXT("Too soon. Way too soon."),
    TEXT("I'm not crying, YOU'RE crying."),
   },
   { // CK_Action: a scene with no mood tag
    TEXT("OHHH!"), TEXT("Did you see that?!"), TEXT("Run! RUN!"), TEXT("Behind you!!"), TEXT("I can't watch! ...okay, I'm watching."),
    TEXT("*gasp*"), TEXT("No way!"), TEXT("Get 'em!"), TEXT("This is intense..."), TEXT("Whoa whoa whoa!"),
    TEXT("My popcorn is everywhere now."), TEXT("Edge of my seat!"), TEXT("Don't go in there!"), TEXT("Ooooh, drama!"),
    TEXT("Here it comes...!"), TEXT("I KNEW it!"),
   },
   { // CK_Idle: between scenes
    TEXT("*crunch crunch*"), TEXT("Shhh!"), TEXT("Can you pass the popcorn?"), TEXT("Who's that in the front row?"),
    TEXT("I need to pee, but I don't want to miss anything."), TEXT("Are those... clay people?"), TEXT("I love the colors in this."),
    TEXT("*slurps soda*"), TEXT("Is this based on a true story?"), TEXT("How long is this movie?"), TEXT("Phones off, please!"),
    TEXT("This seat is sticky..."), TEXT("*whispering* What did I miss?"), TEXT("Who brought a baby?"),
   },
  };
  return Pools[Kind];
 }

 /** Spreads a moment's key into a well-mixed number, for picking seats and odds. */
 uint32 MixMoment(uint64 Moment)
 {
  Moment ^= Moment >> 33; Moment *= 0xff51afd7ed558ccdull; Moment ^= Moment >> 33;
  return uint32(Moment);
 }
 uint64 MomentKey(int32 Source, int32 Index) { return (uint64(uint32(Source)) << 32) | uint32(Index); }
}

/** Where the theater's design space sits on the canvas (FScreen's transform), for the comment bubbles. */
struct FCommentScreen
{
 float Scale, OX, OY, TextScale; UFont* Font;
 float Open; bool bShow;
};

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
 CommentPicks.Reset();
 CommentSeats.Reset();
 if(CommentRandom.GetInitialSeed()==0) CommentRandom.Initialize(int32(FPlatformTime::Cycles()));
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
 FCommentScreen CommentScreen{P.Scale,P.OX,P.OY,P.TextScale,P.Font,Open,(!Finished || ResultAge<ReactionDuration) && Open>=1};
 DrawComments(Film,Canvas,CommentScreen);
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

const TCHAR* ADCCinemaReplay::PickComment(int32 Kind, uint64 Moment)
{
	const TArray<const TCHAR*>& Pool = CommentPool(Kind);
	if (const int32* Found = CommentPicks.Find(Moment))
	{
		return Pool[*Found];
	}
	TArray<int32>& Bag = CommentBags[Kind];
	if (Bag.IsEmpty())
	{
		for (int32 i = 0; i < Pool.Num(); ++i)
		{
			Bag.Add(i);
		}
		for (int32 i = Bag.Num() - 1; i > 0; --i)
		{
			Bag.Swap(i, CommentRandom.RandRange(0, i));
		}
	}
	const int32 Line = Bag.Pop();
	CommentPicks.Add(Moment, Line);
	return Pool[Line];
}

void ADCCinemaReplay::DrawComments(ADCZombieDirector* Film, UCanvas* Canvas, FCommentScreen& Screen)
{
	if (!Screen.bShow)
	{
		return;
	}
	// The moments the audience talks at: two people per scene verdict, a running stream while a scene plays (romance,
	// sadness, or action when untagged), and the odd mutter between scenes.
	struct FComment { int32 Kind; uint64 Moment; float Age; };
	TArray<FComment> Comments;
	const float Time = Film->GetSceneTime();
	auto Odds = [](uint64 Moment, uint32 Percent) { return MixMoment(Moment) % 100u < Percent; };
	bool bInScene = false;
	const TArray<FDCMultiCamBeat>& Beats = Film->GetBeats();
	for (int32 b = 0; b < Beats.Num(); ++b)
	{
		const FDCMultiCamBeat& State = Beats[b];
		if (!State.Beat)
		{
			continue;
		}
		const bool bCaptured = State.Status == EDCShotStatus::Captured;
		const float Cue = bCaptured ? State.CapturedFilmTime : SceneToFilmTime(Film, State.Beat->WindowEnd);
		const int32 Verdict = HasPositiveReaction(FName(*State.Beat->BeatId), bCaptured) ? CK_Good : CK_Bad;
		Comments.Add({ Verdict, MomentKey(b, 0), Time - Cue - 0.15f });
		Comments.Add({ Verdict, MomentKey(b, 1), Time - Cue - 1.1f });

		const float Start = SceneToFilmTime(Film, State.Beat->GetCaptureStartTime());
		const float End = SceneToFilmTime(Film, State.Beat->WindowEnd);
		if (Time < Start || Time >= End + CommentLife)
		{
			continue;
		}
		bInScene |= Time < End;
		const EDCCinemaMood Mood = State.Beat->ReplayMood;
		const int32 Kind = Mood == EDCCinemaMood::Love ? CK_Love : Mood == EDCCinemaMood::Sad ? CK_Sad : CK_Action;
		const float Period = Kind == CK_Action ? 3.2f : 2.4f;
		const uint32 Percent = Kind == CK_Action ? 55u : 85u;
		const int32 Latest = FMath::FloorToInt((FMath::Min(Time, End) - Start) / Period);
		for (int32 k = FMath::Max(0, Latest - 1); k <= Latest; ++k)
		{
			const float Said = Start + k * Period + 0.4f;
			const uint64 Moment = MomentKey(b, 10 + k);
			if (Said < End && Odds(Moment, Percent))
			{
				Comments.Add({ Kind, Moment, Time - Said });
			}
		}
	}
	if (!bInScene)
	{
		constexpr float IdlePeriod = 5.5f;
		const int32 Latest = FMath::FloorToInt(Time / IdlePeriod);
		for (int32 k = FMath::Max(0, Latest - 1); k <= Latest; ++k)
		{
			const uint64 Moment = MomentKey(-1, k);
			if (Odds(Moment, 40u))
			{
				Comments.Add({ CK_Idle, Moment, Time - (k * IdlePeriod + 1.f) });
			}
		}
	}
	Comments.RemoveAll([](const FComment& C) { return C.Age < 0.f || C.Age >= CommentLife; });
	// Oldest first, so a comment keeps its seat while newer ones find room around it.
	Comments.Sort([](const FComment& A, const FComment& B) { return A.Age > B.Age; });

	// The seats that talk: the back two rows, between the ones that show emoji (same layout as the audience drawing).
	struct FSeat { float X, HeadTop; };
	TArray<FSeat> Seats;
	for (int32 Row = 0; Row < 2; ++Row)
	{
		const int32 Count = Row == 0 ? 18 : 15;
		const float Step = 1920.f / Count, BaseY = 825 + Row * 116.f, Size = 1 + Row * .28f;
		for (int32 i = 0; i < Count; ++i)
		{
			if ((i + Row) % 2 == 1)
			{
				const float Bob = FMath::Sin(PresentationTime * 1.1f + i * 2.3f + Row) * 1.5f;
				Seats.Add({ (i + .5f) * Step + (Row % 2 ? Step * .19f : 0), BaseY + Bob + (i % 3) * 5 - 70 * Size });
			}
		}
	}
	const float TextPx = .40f * 32.f * 1.7f * Screen.Scale * Screen.TextScale;
	const FLinearColor Paper(.97f, .94f, .87f), CommentInk(.07f, .06f, .06f);
	const FLinearColor Edges[CK_Num] = { Green, Red, FLinearColor(1, .45f, .62f), FLinearColor(.45f, .7f, 1), Gold, FLinearColor(.6f, .6f, .65f) };
	TArray<float> UsedX;
	constexpr int32 MaxShown = 5;
	for (const FComment& Comment : Comments)
	{
		if (UsedX.Num() >= MaxShown)
		{
			break;
		}
		// A seat of its own, not crowding another bubble - picked once, when the comment first shows, and kept until it
		// fades (re-picking every frame made a bubble jump seats when an older one beside it faded away).
		int32 Seat = INDEX_NONE;
		if (const int32* Kept = CommentSeats.Find(Comment.Moment))
		{
			Seat = *Kept;
		}
		else
		{
			Seat = MixMoment(Comment.Moment) % Seats.Num();
			int32 Tries = 0;
			while (Tries < Seats.Num() && UsedX.ContainsByPredicate([&](float X) { return FMath::Abs(X - Seats[Seat].X) < 330.f; }))
			{
				Seat = (Seat + 1) % Seats.Num();
				++Tries;
			}
			if (Tries == Seats.Num())
			{
				continue; // no room yet: it gets a seat once one frees up
			}
			CommentSeats.Add(Comment.Moment, Seat);
		}
		UsedX.Add(Seats[Seat].X);

		const FString Line = PickComment(Comment.Kind, Comment.Moment);
		const FVector2D Size = DCUI::Measure(Canvas, Screen.Font, Line, TextPx);
		const float S = Screen.Scale;
		const float Pad = 13.f * S, W = Size.X + 2.f * Pad, H = Size.Y + 1.3f * Pad;
		const float HeadX = Screen.OX + Seats[Seat].X * S, HeadY = Screen.OY + Seats[Seat].HeadTop * S;
		const float Alpha = FMath::Min(1.f, Comment.Age * 7.f) * FMath::Clamp((CommentLife - Comment.Age) * 2.5f, 0.f, 1.f);
		const float BX = FMath::Clamp(HeadX - W * .5f, 8.f * S, Canvas->ClipX - W - 8.f * S);
		const float BY = HeadY - H - 14.f * S - Comment.Age * 7.f * S;
		auto Faded = [Alpha](FLinearColor C) { C.A *= Alpha; return C; };
		const float Edge = FMath::Max(2.5f * S, 1.f);
		DCUI::RoundedRect(Canvas, BX - Edge, BY - Edge, W + 2.f * Edge, H + 2.f * Edge, 12.f * S + Edge, Faded(Edges[Comment.Kind]));
		DCUI::Triangle(Canvas, FVector2D(HeadX - 9.f * S, BY + H - 1.f), FVector2D(HeadX + 9.f * S, BY + H - 1.f), FVector2D(HeadX, BY + H + 13.f * S), Faded(Paper));
		DCUI::RoundedRect(Canvas, BX, BY, W, H, 12.f * S, Faded(Paper));
		DCUI::Text(Canvas, Screen.Font, Line, BX + Pad, BY + (H - Size.Y) * .5f, TextPx, Faded(CommentInk), DCUI::EAlign::Left, false);
	}
}
