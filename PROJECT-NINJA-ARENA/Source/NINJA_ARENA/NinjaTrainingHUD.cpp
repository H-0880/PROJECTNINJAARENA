#include "NinjaTrainingHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "NINJA_ARENACharacter.h"
#include "NINJA_ARENAGameMode.h"
#include "NinjaArenaHazards.h"
#include "NinjaCombatStyle.h"
#include "NinjaElementActors.h"
#include "NinjaElementComponent.h"
#include "PNAVitalsComponent.h"

void ANinjaTrainingHUD::DrawHUD()
{
	Super::DrawHUD();
	const ANINJA_ARENAGameMode* Mode = GetWorld()->GetAuthGameMode<ANINJA_ARENAGameMode>();
	const ANINJA_ARENACharacter* Player = PlayerOwner ? Cast<ANINJA_ARENACharacter>(PlayerOwner->GetPawn()) : nullptr;
	if (!Canvas || !Mode || !Player)
	{
		return;
	}

	const bool bVersus = Mode->IsLocalVersus();
	if (Mode->IsModeMenu())
	{
		DrawDemoMenu(*Mode);
		return;
	}
	if (Mode->IsArenaTrial())
	{
		DrawText(TEXT("F10  Volver al menu"), FLinearColor::White, 24.f, Canvas->ClipY - 26.f, GEngine->GetSmallFont());
		return;
	}
	const bool bSecond = Mode->IsSecondLocalPlayer(PlayerOwner);
	const bool bTeams = Mode->IsTeamBattle();
	if (Mode->IsSelectingFighters())
	{
		DrawFighterSelection(*Player, *Mode, bSecond);
		return;
	}
	const ANINJA_ARENACharacter* Opponent = Mode->GetDuelOpponent(Player);
	const bool bFullTeams = Mode->GetTeamSize() == 4;
	if (const ANINJA_ARENACharacter* Locked = Player->GetLockedOpponent())
	{
		const FString Lock = FString::Printf(TEXT("FIJADO  %s  |  %s"),
			Locked->GetCombatStyle()->GetFighterName(), bSecond ? TEXT("M  Soltar") : TEXT("Tab  Soltar"));
		DrawRect(FLinearColor(.06f,.04f,.09f,.91f), 24.f, Canvas->ClipY - 192.f, 250.f, 23.f);
		DrawText(Lock, FLinearColor(.85f,.66f,1.f), 31.f, Canvas->ClipY - 187.f, GEngine->GetSmallFont());
	}
	if (!bVersus && Player->GetLastMeleeDamage() > 0.f)
	{
		const FString Metrics = FString::Printf(TEXT("GOLPE  %.0f    SERIE  %d / %.0f"),
			Player->GetLastMeleeDamage(), Player->GetMeleeChainHits(), Player->GetMeleeChainDamage());
		DrawRect(FLinearColor(.03f,.045f,.06f,.91f), 24.f, 148.f, 228.f, 24.f);
		DrawText(Metrics, FLinearColor::White, 31.f, 153.f, GEngine->GetSmallFont());
	}
	const float Margin = 24.f;
	const float PanelWidth = FMath::Clamp((Canvas->ClipX - Margin * 3.f) * 0.5f, 100.f, 320.f);
	const TCHAR* Name = Player->GetCombatStyle()->GetFighterName();
	const FString LocalName = bVersus ? FString::Printf(TEXT("P%d  %s"), bSecond ? 2 : 1, Name) : FString(Name);
	DrawFighterPanel(*Player, *LocalName, Margin, PanelWidth, Player->GetCombatStyle()->GetProfile().Accent, false);
	TArray<FVector2D, TInlineAllocator<8>> Markers;
	const auto MarkerFor = [this, bTeams, &Markers](const ANINJA_ARENACharacter& Fighter, const TCHAR* Label, const FLinearColor& Accent)
	{
		FVector2D Marker;
		if (!Fighter.GetVitals()->IsDefeated() && PlayerOwner->GetViewTarget() != &Fighter
			&& PlayerOwner->ProjectWorldLocationToScreen(Fighter.GetActorLocation() + FVector(0.f, 0.f, 125.f), Marker, true)
			&& Marker.X > 44.f && Marker.X < Canvas->ClipX - 44.f
			&& Marker.Y > (bTeams ? 218.f : 150.f) && Marker.Y < Canvas->ClipY - 134.f)
		{
			const auto IsCrowded = [&Markers](const FVector2D& Spot)
			{
				return Markers.ContainsByPredicate([&Spot](const FVector2D& Used)
				{
					return FMath::Abs(Used.X - Spot.X) < 87.f && FMath::Abs(Used.Y - Spot.Y) < 20.f;
				});
			};
			if (bTeams && IsCrowded(Marker))
			{
				Marker.Y += 21.f;
				if (IsCrowded(Marker) || Marker.Y >= Canvas->ClipY - 134.f)
				{
					return;
				}
			}
			Markers.Add(Marker);
			DrawRect(FLinearColor(0.03f, 0.045f, 0.06f, 0.9f), Marker.X - 43.f, Marker.Y - 9.f, 86.f, 19.f);
			DrawText(Label, Accent, Marker.X - 35.f, Marker.Y - 8.f, GEngine->GetSmallFont());
		}
	};
	if (IsValid(Opponent))
	{
		const bool bWarning = Mode->IsOpponentPreparingAttack();
		const FLinearColor RivalAccent = Opponent->GetCombatStyle()->GetProfile().Accent;
		const TCHAR* NameOfRival = Opponent->GetCombatStyle()->GetFighterName();
		const FString RivalName = bVersus ? FString::Printf(TEXT("P%d  %s"), bSecond ? 1 : 2, NameOfRival) : FString(NameOfRival);
		DrawFighterPanel(*Opponent, *RivalName, Canvas->ClipX - Margin - PanelWidth, PanelWidth, RivalAccent, bWarning);
		MarkerFor(*Opponent, bWarning ? TEXT("ATAQUE") : *RivalName, RivalAccent);
	}
	if (bTeams)
	{
		const auto DrawTeam = [this, PanelWidth, &MarkerFor](TConstArrayView<TObjectPtr<ANINJA_ARENACharacter>> Bots,
			float Left, bool bAllied, const FLinearColor& Accent)
		{
			if (Bots.IsEmpty())
			{
				return;
			}
			const bool bSingle = Bots.Num() == 1;
			const float CellWidth = (PanelWidth - 20.f) / Bots.Num();
			const float BarWidth = CellWidth - (bSingle ? 0.f : 8.f);
			DrawRect(FLinearColor(0.03f, 0.045f, 0.06f, 0.92f), Left, 143.f, PanelWidth, bSingle ? 39.f : 48.f);
			if (!bSingle)
			{
				DrawText(bAllied ? TEXT("ALIADOS IA") : TEXT("RIVALES IA"), Accent, Left + 10.f, 146.f, GEngine->GetSmallFont());
			}
			for (int32 Index = 0; Index < Bots.Num(); ++Index)
			{
				const ANINJA_ARENACharacter* Fighter = Bots[Index].Get();
				const UPNAVitalsComponent* Vitals = IsValid(Fighter) ? Fighter->GetVitals() : nullptr;
				const int32 CurrentHealth = Vitals ? FMath::CeilToInt(Vitals->Health) : 0;
				const float Health = Vitals ? FMath::Clamp(Vitals->Health / FMath::Max(1.f, Vitals->MaxHealth), 0.f, 1.f) : 0.f;
				const float CellX = Left + 10.f + Index * CellWidth;
				const float BarY = bSingle ? 170.f : 180.f;
				const TCHAR* Role = bAllied ? TEXT("ALIADO") : TEXT("RIVAL");
				const FString Label = bSingle && Vitals
					? FString::Printf(TEXT("%s IA  %s  %d/%d"), Role, Fighter->GetCombatStyle()->GetFighterName(), CurrentHealth, FMath::CeilToInt(Vitals->MaxHealth))
					: FString::Printf(TEXT("IA %d   %d"), Index + 1, CurrentHealth);
				DrawText(Label, Accent, CellX, bSingle ? 150.f : 163.f, GEngine->GetSmallFont());
				DrawRect(FLinearColor(0.16f, 0.19f, 0.22f), CellX, BarY, BarWidth, 5.f);
				DrawRect(Accent, CellX, BarY, BarWidth * Health, 5.f);
				if (Vitals)
				{
					const FString MarkerLabel = bSingle ? FString::Printf(TEXT("%s IA"), Role) : FString::Printf(TEXT("%s %d"), Role, Index + 1);
					MarkerFor(*Fighter, *MarkerLabel, Accent);
				}
			}
		};
		const int32 PlayerTeam = Player->GetCombatTeam();
		DrawTeam(Mode->GetTeamBots(PlayerTeam), Margin, true, FLinearColor(0.30f, 0.82f, 1.0f));
		DrawTeam(Mode->GetTeamBots(1 - PlayerTeam), Canvas->ClipX - Margin - PanelWidth, false, FLinearColor(1.0f, 0.47f, 0.29f));
		if (Mode->GetRoundResult() == ENinjaRoundResult::Fighting && Player->GetVitals()->IsDefeated()
			&& Mode->GetTeamCompanion(Player))
		{
			DrawText(TEXT("ESPECTANDO A TU EQUIPO"), FLinearColor::White, Canvas->ClipX * 0.5f - 90.f, bFullTeams ? 220.f : 216.f, GEngine->GetSmallFont());
		}
	}

	const TCHAR* TeamLabel = bTeams ? (bFullTeams ? TEXT("4V4    ") : TEXT("2V2    ")) : TEXT("");
	const FString Round = FString::Printf(TEXT("%sRONDA %d"), TeamLabel, Mode->GetRoundNumber());
	const float RoundY = bTeams ? (bFullTeams ? 198.f : 190.f) : 140.f;
	DrawText(Round, FLinearColor::White, Canvas->ClipX * 0.5f - (bTeams ? 62.f : 42.f), RoundY, GEngine->GetSmallFont(), 1.25f);
	if (Mode->IsCaptureMatch())
	{
		const FString Score = FString::Printf(TEXT("CAPTURA  %d / 100   -   %d / 100"),
			FMath::FloorToInt(Mode->GetCaptureScore(0)), FMath::FloorToInt(Mode->GetCaptureScore(1)));
		DrawRect(FLinearColor(0.03f, 0.045f, 0.06f, 0.92f), Canvas->ClipX * 0.5f - 115.f, 222.f, 230.f, 25.f);
		DrawText(Score, FLinearColor::White, Canvas->ClipX * 0.5f - 103.f, 227.f, GEngine->GetSmallFont());
		for (int32 Point = 0; Point < 3; ++Point)
		{
			const int32 PointTeam = Mode->GetCaptureOwner(Point);
			const TCHAR* State = PointTeam == 2 ? TEXT("Disputa") : (PointTeam == 0 ? TEXT("P1") : (PointTeam == 1 ? TEXT("P2") : TEXT("Libre")));
			const FLinearColor Color = PointTeam == 0 ? FLinearColor(0.30f, 0.82f, 1.f)
				: (PointTeam == 1 ? FLinearColor(1.f, 0.47f, 0.29f) : FLinearColor::White);
			const float X = Canvas->ClipX * 0.5f - 115.f + Point * 78.f;
			DrawRect(FLinearColor(0.03f, 0.045f, 0.06f, 0.92f), X, 252.f, 73.f, 23.f);
			DrawText(FString::Printf(TEXT("%c  %s"), TCHAR('A' + Point), State), Color, X + 7.f, 256.f, GEngine->GetSmallFont());
			const FVector Center = Mode->GetCaptureCenter(Point);
			const int32 Distance = FMath::RoundToInt(FVector::Dist2D(Player->GetActorLocation(), Center) / 100.0);
			DrawRect(FLinearColor(0.03f, 0.045f, 0.06f, 0.92f), X, 275.f, 73.f, 19.f);
			DrawText(FString::Printf(TEXT("%d m"), Distance), Color, X + 7.f, 276.f, GEngine->GetSmallFont());
			FVector2D Marker;
			if (PlayerOwner->ProjectWorldLocationToScreen(Center + FVector(0, 0, 285), Marker, true)
				&& Marker.X > 45.f && Marker.X < Canvas->ClipX - 45.f && Marker.Y > 298.f && Marker.Y < Canvas->ClipY - 105.f)
			{
				DrawRect(FLinearColor(0.03f, 0.045f, 0.06f, 0.90f), Marker.X - 35.f, Marker.Y, 70.f, 22.f);
				DrawText(FString::Printf(TEXT("%c   %d m"), TCHAR('A' + Point), Distance),
					Color, Marker.X - 28.f, Marker.Y + 3.f, GEngine->GetSmallFont());
			}
		}
	}
	const float BridgeTime = Mode->GetTempleBridgeTime();
	if (BridgeTime > 0.f)
	{
		const bool bDown = Mode->IsTempleBridgeDown();
		const float X = Canvas->ClipX * 0.5f - 110.f;
		DrawRect(FLinearColor(0.06f, 0.035f, 0.025f, 0.92f), X, 174.f, 220.f, 24.f);
		const FString Label = FString::Printf(TEXT("%s %.1f s"), bDown ? TEXT("Puente vuelve en") : TEXT("Cede el puente en"), BridgeTime);
		DrawText(Label, FLinearColor(1.f, 0.71f, 0.38f), X + 12.f, 179.f, GEngine->GetSmallFont());
	}
	if (Mode->GetRoundResult() != ENinjaRoundResult::Fighting)
	{
		const bool bVictory = (Mode->GetRoundResult() == ENinjaRoundResult::Victory) != bSecond;
		const float Left = Canvas->ClipX * 0.5f - 120.f;
		const float Top = Canvas->ClipY * 0.5f - 35.f;
		DrawRect(FLinearColor(0.03f, 0.045f, 0.06f, 0.95f), Left, Top, 240.f, 70.f);
		const TCHAR* Result = Mode->GetRoundResult() == ENinjaRoundResult::Draw ? TEXT("EMPATE")
			: (bVictory ? TEXT("VICTORIA") : TEXT("DERROTA"));
		DrawText(Result, FLinearColor::White, Left + 65.f, Top + 9.f,
			GEngine->GetSmallFont(), 1.8f);
		DrawText(TEXT("La siguiente ronda inicia pronto"), FLinearColor::White, Left + 31.f, Top + 42.f, GEngine->GetSmallFont());
	}

	DrawRect(FLinearColor(0.03f, 0.045f, 0.06f, 0.88f), Margin, Canvas->ClipY - 67.f,
		FMath::Min(Canvas->ClipX - Margin * 2.f, 530.f), 44.f);
	if (Player->CanAirRecover())
	{
		const TCHAR* JumpKey = bSecond ? TEXT("Ctrl der. / A") : TEXT("Espacio");
		const float X = Canvas->ClipX * 0.5f - 130.f;
		const float Y = Canvas->ClipY - 165.f;
		DrawRect(FLinearColor(0.025f, 0.055f, 0.077f, 0.91f), X, Y, 260.f, 24.f);
		DrawText(FString::Printf(TEXT("%s + direccion  Recuperar"), JumpKey),
			FLinearColor(0.36f, 1.f, 0.85f), X + 10.f, Y + 4.f, GEngine->GetSmallFont());
	}
	DrawText(bVersus ? (bSecond ? TEXT("M  Fijar    F10  Menu") : TEXT("Tab  Fijar    F10  Menu")) : TEXT("Tab  Fijar    F3  Personaje    F10  Menu"), FLinearColor::White,
		Margin, Canvas->ClipY - 20.f, GEngine->GetSmallFont());
	const TCHAR* AttackLegend = bSecond ? TEXT("U/O  Golpes    P  Bloqueo    Flechas  Mover    IJKL  Camara")
		: TEXT("Clic izq.  Ligero     Clic der.  Fuerte     Q  Bloqueo     R  Reiniciar");
	const TCHAR* MoveLegend = bSecond ? TEXT("Ctrl der.  Salto    Mayus der.  Dash    Y  Perseguir    N  Pared    F2  Elegir")
		: (bVersus ? TEXT("Espacio x2  Salto     F  Dash     E  Pared     C  Perseguir     F2  Elegir")
			: TEXT("Espacio x2  Salto     F  Dash     E  Pared     C  Perseguir"));
	DrawText(AttackLegend, FLinearColor::White,
		Margin + 10.f, Canvas->ClipY - 62.f, GEngine->GetSmallFont());
	DrawText(MoveLegend, FLinearColor::White,
		Margin + 10.f, Canvas->ClipY - 44.f, GEngine->GetSmallFont());

	const UNinjaElementComponent* Elements = Player->GetElements();
	const float SlotWidth = FMath::Clamp((Canvas->ClipX - Margin * 2.f - 32.f) / 5.f, 65.f, 105.f);
	const float RowLeft = (Canvas->ClipX - SlotWidth * 5.f - 32.f) * 0.5f;
	const float RowY = Canvas->ClipY - 102.f;
	const float UltimateCooldown = Player->GetUltimateCooldownRemaining();
	const float UltimateCost = Player->GetUltimateEnergyCost();
	const bool bUltimateReady = UltimateCooldown <= 0.f && Player->GetVitals()->Energy >= UltimateCost;
	const FLinearColor UltimateColor = bUltimateReady || Player->IsUltimateCharging()
		? FLinearColor(0.22f, 0.96f, 0.84f) : FLinearColor(0.60f, 0.66f, 0.70f);
	const bool bRaizen = Player->GetCombatStyle()->GetFighter() == ENinjaFighter::Raizen;
	if (Player->IsCounterReady() || bRaizen)
	{
		const TCHAR* Label = Player->IsCounterReady()
			? (bSecond ? TEXT("O  Contraataque") : TEXT("Clic der.  Contraataque"))
			: (Player->IsRaizenPowerForm() ? (bSecond ? TEXT("B  Volver a forma ligera") : TEXT("V  Volver a forma ligera"))
				: (bSecond ? TEXT("B  Forma pesada  14 energia") : TEXT("V  Forma pesada  14 energia")));
		const float X = Canvas->ClipX * 0.5f - 110.f;
		DrawRect(FLinearColor(0.03f, 0.045f, 0.06f, 0.9f), X, RowY - 57.f, 220.f, 22.f);
		DrawText(Label, FLinearColor(0.98f, 0.80f, 0.46f), X + 9.f, RowY - 53.f, GEngine->GetSmallFont());
	}
	const TCHAR* UltimateKey = bSecond ? TEXT("H") : TEXT("G");
	const TCHAR* UltimateName = Player->GetCombatStyle()->GetFighter() == ENinjaFighter::Kuroboshi
		? TEXT("CORTE ESPECTRAL") : TEXT("PULSO NINJA");
	const FString UltimateLabel = Player->IsUltimateCharging() ? FString::Printf(TEXT("%s  CARGANDO"), UltimateName)
		: (UltimateCooldown > 0.f ? FString::Printf(TEXT("%s  %s  %.1fs"), UltimateKey, UltimateName, UltimateCooldown)
			: FString::Printf(TEXT("%s  %s  %d ENERGIA"), UltimateKey, UltimateName, FMath::CeilToInt(UltimateCost)));
	const float UltimateLeft = Canvas->ClipX * 0.5f - 103.f;
	DrawRect(FLinearColor(0.03f, 0.045f, 0.06f, 0.9f), UltimateLeft, RowY - 32.f, 206.f, 24.f);
	DrawRect(UltimateColor, UltimateLeft, RowY - 32.f, 206.f, 2.f);
	DrawText(UltimateLabel, UltimateColor, UltimateLeft + 10.f, RowY - 26.f, GEngine->GetSmallFont());
	const ENinjaElement Slots[] = { ENinjaElement::Fire, ENinjaElement::Lightning, ENinjaElement::Wind,
		ENinjaElement::Ice, ENinjaElement::Water };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Slots); ++Index)
	{
		if (const FNinjaElementAbility* Ability = Elements->GetAbility(Slots[Index]))
		{
			const float Left = RowLeft + Index * (SlotWidth + 8.f);
			const float Cooldown = Elements->GetCooldownRemaining(Slots[Index]);
			const bool bReady = Cooldown <= 0.f && Player->GetVitals()->Energy >= Ability->EnergyCost;
			const FLinearColor Accent = bReady ? Ability->Color : FLinearColor(0.53f, 0.59f, 0.63f);
			const int32 KeyNumber = (Index + (bSecond ? 6 : 1)) % 10;
			const FString Label = Cooldown > 0.1f
				? FString::Printf(TEXT("%d %s %.1fs"), KeyNumber, *Ability->Name, Cooldown)
				: FString::Printf(TEXT("%d %s %d"), KeyNumber, *Ability->Name, FMath::CeilToInt(Ability->EnergyCost));
			DrawRect(FLinearColor(0.03f, 0.045f, 0.06f, 0.9f), Left, RowY, SlotWidth, 24.f);
			DrawRect(Accent, Left, RowY, SlotWidth, 2.f);
			DrawText(Label, Accent, Left + 6.f, RowY + 5.f, GEngine->GetSmallFont(), SlotWidth < 90.f ? 0.85f : 1.f);
		}
	}

	const auto DrawProp = [this, RowY](const ANinjaElementProp* Prop)
	{
		FVector2D Marker;
		if (IsValid(Prop) && Prop->GetHealth() > 0.f && PlayerOwner->ProjectWorldLocationToScreen(
			Prop->GetActorLocation() + FVector(0.0, 0.0, Prop->GetActorScale3D().Z * 50.0 + 28.0), Marker, true)
			&& Marker.X > 70.f && Marker.X < Canvas->ClipX - 70.f && Marker.Y > 155.f && Marker.Y < RowY - 20.f)
		{
			const FString Label = FString::Printf(TEXT("%s %d  %s"), Prop->GetDisplayName(),
				FMath::CeilToInt(Prop->GetHealth()), Prop->GetElements()->GetStatusLabel());
			DrawRect(FLinearColor(0.03f, 0.045f, 0.06f, 0.85f), Marker.X - 70.f, Marker.Y - 9.f, 140.f, 19.f);
			DrawText(Label, Prop->GetElements()->GetStatusColor(FLinearColor::White), Marker.X - 64.f, Marker.Y - 8.f,
				GEngine->GetSmallFont());
		}
	};
	for (const ANinjaElementProp* Prop : Mode->GetElementProps())
	{
		DrawProp(Prop);
	}
	for (const ANinjaElementProp* Structure : Mode->GetArenaStructures())
	{
		DrawProp(Structure);
	}

	const auto DrawZone = [this, Player, RowY](const ANinjaArenaDangerZone* Zone)
	{
		if (!IsValid(Zone))
		{
			return;
		}
		const ENinjaArenaZonePhase Phase = Zone->GetPhase();
		FVector2D Marker;
		const bool bOnScreen = PlayerOwner->ProjectWorldLocationToScreen(Zone->GetActorLocation() + FVector(0.0, 0.0, 24.0), Marker, true)
			&& Marker.X > 90.f && Marker.X < Canvas->ClipX - 90.f && Marker.Y > 159.f && Marker.Y < RowY - 20.f;
		if (!bOnScreen)
		{
			if (Phase == ENinjaArenaZonePhase::Rest || FVector::DistSquared2D(Player->GetActorLocation(), Zone->GetActorLocation())
				> FMath::Square(static_cast<double>(Zone->GetRadius() + 32.f)))
			{
				return;
			}
			Marker = FVector2D(Canvas->ClipX * 0.5f, 173.f);
		}
		const TCHAR* Label = TEXT("ZONA EN DESCANSO");
		FLinearColor Color = FLinearColor(0.7f, 0.76f, 0.84f);
		if (Phase == ENinjaArenaZonePhase::Warning)
		{
			Label = Zone->IsCollapse() ? TEXT("DERRUMBE INMINENTE") : TEXT("AVISO: SAL DE LA ZONA");
			Color = FLinearColor(1.f, 0.72f, 0.32f);
		}
		else if (Phase == ENinjaArenaZonePhase::Active)
		{
			Label = Zone->IsCollapse() ? TEXT("DERRUMBE") : TEXT("ZONA ACTIVA");
			Color = FLinearColor(1.f, 0.38f, 0.24f);
		}
		DrawRect(FLinearColor(0.03f, 0.045f, 0.06f, 0.86f), Marker.X - 90.f, Marker.Y - 9.f, 180.f, 20.f);
		DrawText(Label, Color, Marker.X - 84.f, Marker.Y - 8.f, GEngine->GetSmallFont());
	};
	DrawZone(Mode->GetArenaHazard());
	for (const TWeakObjectPtr<ANinjaArenaDangerZone>& Zone : Mode->GetCollapseZones())
	{
		DrawZone(Zone.Get());
	}
}

void ANinjaTrainingHUD::DrawFighterSelection(const ANINJA_ARENACharacter& Fighter,
	const ANINJA_ARENAGameMode& Mode, bool bSecond)
{
	const float Width = FMath::Min(Canvas->ClipX - 36.f, 540.f);
	const float Left = (Canvas->ClipX - Width) * 0.5f;
	const float Top = Canvas->ClipY - 201.f;
	const int32 ChoiceCount = static_cast<int32>(ENinjaFighter::Count);
	const float CardWidth = (Width - 32.f - (ChoiceCount - 1) * 12.f) / ChoiceCount;
	const FLinearColor Muted(0.62f, 0.70f, 0.75f);
	const bool bConfirmed = Mode.IsFighterConfirmed(PlayerOwner);
	const FNinjaFighterProfile& Profile = Fighter.GetCombatStyle()->GetProfile();
	UFont* Font = GEngine->GetSmallFont();

	DrawRect(FLinearColor(0.024f, 0.031f, 0.046f, 0.97f), Left, Top, Width, 177.f);
	DrawRect(Profile.Accent, Left, Top, Width, 3.f);
	const TCHAR* TeamLabel = Mode.IsTeamBattle() ? (Mode.GetTeamSize() == 4 ? TEXT("4V4    ") : TEXT("2V2    ")) : TEXT("");
	DrawText(FString::Printf(TEXT("P%d    %sELIGE TU PERSONAJE"), bSecond ? 2 : 1, TeamLabel), FLinearColor::White,
		Left + 16.f, Top + 14.f, Font, 1.55f);

	for (int32 Index = 0; Index < ChoiceCount; ++Index)
	{
		const ENinjaFighter Id = static_cast<ENinjaFighter>(Index);
		const FNinjaFighterProfile& Choice = UNinjaCombatStyleComponent::GetProfile(Id);
		const bool bSelected = Fighter.GetCombatStyle()->GetFighter() == Id;
		const float CardX = Left + 16.f + Index * (CardWidth + 12.f);
		const float CardY = Top + 51.f;
		DrawRect(bSelected ? FLinearColor(0.085f, 0.11f, 0.15f) : FLinearColor(0.045f, 0.059f, 0.081f),
			CardX, CardY, CardWidth, 74.f);
		DrawRect(bSelected ? Choice.Accent : Muted, CardX, CardY, 3.f, 74.f);
		float NameWidth;
		float NameHeight;
		GetTextSize(Choice.Name, NameWidth, NameHeight, Font);
		const float NameScale = FMath::Min(1.35f, (CardWidth - 14.f) / FMath::Max(1.f, NameWidth));
		DrawText(Choice.Name, bSelected ? Choice.Accent : FLinearColor::White, CardX + 8.f, CardY + 12.f, Font, NameScale);
	}

	const TCHAR* Hint = bSecond ? TEXT("Flechas o cruceta  Elegir    Intro o A  Confirmar") : TEXT("A / D  Elegir    Espacio  Confirmar    F10  Menu");
	DrawText(bConfirmed ? TEXT("LISTO    Esperando al otro jugador") : Hint,
		bConfirmed ? Profile.Accent : FLinearColor::White, Left + 16.f, Top + 145.f, Font, 1.10f);
}

void ANinjaTrainingHUD::DrawFighterPanel(const ANINJA_ARENACharacter& Fighter, const TCHAR* Name,
	float Left, float Width, FLinearColor Accent, bool bWarning)
{
	const UPNAVitalsComponent* Vitals = Fighter.GetVitals();
	const float InnerLeft = Left + 12.f;
	const float BarWidth = Width - 24.f;
	const float HealthRatio = FMath::Clamp(Vitals->Health / FMath::Max(1.f, Vitals->MaxHealth), 0.f, 1.f);
	const float EnergyRatio = FMath::Clamp(Vitals->Energy / FMath::Max(1.f, Vitals->MaxEnergy), 0.f, 1.f);
	const FLinearColor PanelBackground(0.03f, 0.045f, 0.06f, 0.9f);
	const FLinearColor Track(0.16f, 0.19f, 0.22f);
	const FLinearColor EnergyColor(0.36f, 0.68f, 1.f);
	DrawRect(PanelBackground, Left, 24.f, Width, 113.f);
	DrawRect(Accent, Left, 24.f, Width, 3.f);
	DrawText(Name, Accent, InnerLeft, 32.f, GEngine->GetSmallFont(), 1.2f);
	DrawText(FString::Printf(TEXT("VIDA  %d / %d"), FMath::CeilToInt(Vitals->Health), FMath::CeilToInt(Vitals->MaxHealth)),
		FLinearColor::White, InnerLeft, 52.f, GEngine->GetSmallFont());
	DrawRect(Track, InnerLeft, 70.f, BarWidth, 7.f);
	DrawRect(Accent, InnerLeft, 70.f, BarWidth * HealthRatio, 7.f);
	DrawText(FString::Printf(TEXT("ENERGIA  %d / %d"), FMath::FloorToInt(Vitals->Energy), FMath::FloorToInt(Vitals->MaxEnergy)),
		FLinearColor::White, InnerLeft, 80.f, GEngine->GetSmallFont());
	DrawRect(Track, InnerLeft, 97.f, BarWidth, 5.f);
	DrawRect(EnergyColor, InnerLeft, 97.f, BarWidth * EnergyRatio, 5.f);

	const UNinjaElementComponent* Elements = Fighter.GetElements();
	const TCHAR* State = Elements->GetStatusLabel();
	if (Vitals->IsDefeated())
	{
		State = TEXT("DERROTADO");
	}
	else if (Fighter.IsLaunched())
	{
		State = TEXT("EN EL AIRE");
	}
	else if (Fighter.IsStunned())
	{
		State = TEXT("ATURDIDO");
	}
	else if (Fighter.IsGuarding())
	{
		State = TEXT("BLOQUEO");
	}
	else if (Fighter.IsWallRunning())
	{
		State = TEXT("PARED");
	}
	else if (Fighter.CanAirPursue())
	{
		State = TEXT("PERSECUCION");
	}
	else if (bWarning && !*State)
	{
		State = TEXT("PREPARA ATAQUE");
	}
	const int32 AirComboStep = Fighter.GetAirComboStep();
	const FString Status = FString::Printf(TEXT("%s %d/3   %s"), AirComboStep > 0 ? TEXT("AEREO") : TEXT("COMBO"),
		AirComboStep > 0 ? AirComboStep : Fighter.GetComboStep(), State);
	DrawText(Status, Elements->GetStatusColor(bWarning ? FLinearColor(1.f, 0.78f, 0.28f) : FLinearColor::White),
		InnerLeft, 110.f, GEngine->GetSmallFont());
}

void ANinjaTrainingHUD::DrawDemoMenu(const ANINJA_ARENAGameMode& Mode)
{
	const float Scale = FMath::Min(Canvas->ClipX / 1280.f, Canvas->ClipY / 720.f);
	const float OriginX = (Canvas->ClipX - 1280.f * Scale) * .5f;
	const float OriginY = (Canvas->ClipY - 720.f * Scale) * .5f;
	const FLinearColor Ink(.022f,.027f,.068f);
	const FLinearColor Panel(.047f,.056f,.115f);
	const FLinearColor Muted(.61f,.66f,.80f);
	const FLinearColor Neon(.66f,.45f,1.f);
	const FLinearColor Gold(1.f,.71f,.34f);
	UFont* Font = GEngine->GetSmallFont();
	const int32 Step = Mode.GetMenuStage();
	const int32 Match = Mode.GetMenuMode();
	const int32 Arena = Mode.GetMenuArena();
	const TCHAR* Modes[] = {TEXT("ENTRENAMIENTO"), TEXT("VERSUS 1V1"), TEXT("EQUIPOS 2V2"), TEXT("EQUIPOS 4V4"), TEXT("CAPTURA 4V4"), TEXT("RUTA COLISEO")};
	const TCHAR* ModeDetails[] = {TEXT("Practica tus movimientos contra un rival controlado por el juego."),
		TEXT("Dos jugadores locales. Un duelo mano a mano."),
		TEXT("Dos jugadores locales, con un aliado de IA por equipo."),
		TEXT("Dos jugadores locales, con tres aliados de IA por equipo."),
		TEXT("Domina los puntos A, B y C para sumar antes que el rival."), TEXT("Activa tres sellos y cruza el santuario contra el reloj.")};
	const TCHAR* Arenas[] = {TEXT("PATIO DE ENTRENAMIENTO"), TEXT("TEMPLO DEL VALLE"), TEXT("CAMPO DE CAPTURA"), TEXT("COLISEO DORADO")};
	const TCHAR* ArenaDetails[] = {TEXT("Patio abierto para practicar distancia y combos."),
		TEXT("Galerias laterales, impulsores y un puente que cede."),
		TEXT("Cobertura de piedra, plataformas y tres objetivos."), TEXT("Ruta individual con ascensor, impulso, porton y marcas locales.")};
	static const TSoftObjectPtr<UTexture2D> Previews[] = {
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/NinjaArena/MenuDemo/Preview_Patio.Preview_Patio"))),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/NinjaArena/MenuDemo/Preview_Templo.Preview_Templo"))),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/NinjaArena/MenuDemo/Preview_Campo.Preview_Campo"))),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/NinjaArena/ColiseoDoradoV4/Preview_Coliseo.Preview_Coliseo")))};
	static const TSoftObjectPtr<UTexture2D> Portraits[] = {
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/NinjaArena/MenuDemo/Portrait_Junco.Portrait_Junco"))),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/NinjaArena/MenuDemo/Portrait_Ascua.Portrait_Ascua"))),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/NinjaArena/MenuDemo/Portrait_Kuroboshi.Portrait_Kuroboshi"))),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/NinjaArena/MenuDemo/Portrait_Raizen.Portrait_Raizen"))),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/NinjaArena/MenuDemo/Portrait_Akira.Portrait_Akira"))),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/NinjaArena/MenuDemo/Portrait_Byakuro.Portrait_Byakuro")))};
	const auto Box = [&](FLinearColor Color, float X, float Y, float W, float H)
	{
		DrawRect(Color, OriginX + X * Scale, OriginY + Y * Scale, W * Scale, H * Scale);
	};
	const auto Line = [&](const FString& Label, FLinearColor Color, float X, float Y, float Size)
	{
		DrawText(Label, Color, OriginX + X * Scale, OriginY + Y * Scale, Font, Size * Scale);
	};
	const auto Photo = [&](const TSoftObjectPtr<UTexture2D>& Image, float X, float Y, float W, float H, bool bPortrait, bool bSelected)
	{
		if (UTexture2D* Texture = Image.LoadSynchronous())
		{
			DrawTexture(Texture, OriginX + X * Scale, OriginY + Y * Scale, W * Scale, H * Scale,
				bPortrait ? .36f : .04f, bPortrait ? .28f : .20f, bPortrait ? .27f : .92f, bPortrait ? .49f : .61f,
				bSelected ? FLinearColor::White : FLinearColor(.55f,.55f,.68f), BLEND_Opaque);
		}
	};
	DrawRect(Ink, 0, 0, Canvas->ClipX, Canvas->ClipY);
	Box(FLinearColor(.056f,.067f,.151f), 0, 0, 1280, 128);
	Box(Neon, 46, 30, 5, 65);
	Line(TEXT("NINJA ARENA"), FLinearColor::White, 65, 38, 3.25f);
	const TCHAR* Stages[] = {TEXT("MODO"),TEXT("ARENA"),TEXT("LUCHADORES")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Stages); ++Index)
	{
		const bool bActive = Index == FMath::Min(2, Step);
		const float X = 810.f + 140.f * Index;
		Box(bActive ? Neon : Panel, X, 62, 126, 34);
		Line(Stages[Index], bActive ? Ink : Muted, X + 12, 69, 1.3f);
	}
	Box(Neon, 0, 127, 1280, 2);
	Box(Panel, 46, 156, 1188, 462);
	if (Step == 0)
	{
		Photo(Previews[Match >= 4 ? Arena : (Match == 0 ? 0 : 1)], 46, 156, 1188, 317, false, true);
		Box(FLinearColor(.022f,.027f,.068f,.84f), 46, 350, 1188, 123);
		Line(Modes[Match], FLinearColor::White, 75, 359, 3.1f);
		Line(ModeDetails[Match], Muted, 76, 420, 1.6f);
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Modes); ++Index)
		{
			const float X = 60 + Index * 195.f;
			const bool bSelected = Index == Match;
			Box(bSelected ? Neon : Ink, X, 507, 186, 76);
			Line(Modes[Index], bSelected ? Ink : Muted, X + 12, 534, 1.2f);
		}
	}
	else if (Step == 1)
	{
		const int32 First = Match >= 4 ? Arena : 0;
		const int32 End = Match >= 4 ? Arena + 1 : 3;
		const float Width = Match >= 4 ? 1050.f : 371.f;
		for (int32 Index = First; Index < End; ++Index)
		{
			const float X = Match >= 4 ? 115.f : 67.f + Index * 388.f;
			const bool bSelected = Index == Arena;
			Box(bSelected ? Neon : Ink, X, 182, Width, 355);
			Photo(Previews[Index], X + 4, 186, Width - 8, 278, false, bSelected);
			Box(Ink, X + 4, 464, Width - 8, 69);
			Line(Arenas[Index], bSelected ? FLinearColor::White : Muted, X + 14, 485, 1.52f);
		}
		Line(ArenaDetails[Arena], Muted, 74, 565, 1.46f);
	}
	else
	{
		const int32 First = Mode.GetMenuFighter(false);
		const int32 Second = Mode.GetMenuFighter(true);
		const bool bSecond = Step == 3;
		Line(bSecond ? TEXT("ELIGE AL JUGADOR 2") : TEXT("ELIGE A TU LUCHADOR"), FLinearColor::White, 72, 177, 2.15f);
		const auto FighterBanner = [&](int32 Index, float X, FLinearColor Accent, const TCHAR* Side)
		{
			const FNinjaFighterProfile& Profile = UNinjaCombatStyleComponent::GetProfile(static_cast<ENinjaFighter>(Index));
			Box(Ink, X, 236, 555, 108);
			Box(Accent, X, 236, 5, 108);
			Line(Side, Accent, X + 16, 250, 1.18f);
			Line(Profile.Name, FLinearColor::White, X + 16, 278, 2.45f);
		};
		FighterBanner(First, 72, Neon, TEXT("JUGADOR 1"));
		if (!Mode.IsSoloMenuMode()) { FighterBanner(Second, 651, Gold, TEXT("JUGADOR 2")); }
		const int32 FighterCount = static_cast<int32>(ENinjaFighter::Count);
		const float CardStep = 1136.f / FighterCount;
		const float CardWidth = CardStep - 10.f;
		for (int32 Index = 0; Index < FighterCount; ++Index)
		{
			const float X = 72.f + Index * CardStep;
			const bool bSelected = Index == (bSecond ? Second : First);
			const FLinearColor Accent = bSelected ? (bSecond ? Gold : Neon) : Ink;
			const FNinjaFighterProfile& Profile = UNinjaCombatStyleComponent::GetProfile(static_cast<ENinjaFighter>(Index));
			Box(Accent, X, 380, CardWidth, 205);
			if (Index < UE_ARRAY_COUNT(Portraits))
			{
				Photo(Portraits[Index], X + 4, 384, CardWidth - 8.f, 151, true, bSelected);
			}
			else
			{
				Box(Profile.Armor, X + 4, 384, CardWidth - 8.f, 151);
				Line(TEXT("CAPOEIRA"), Profile.Accent, X + 10, 452, 1.12f);
			}
			Box(Ink, X + 4, 535, CardWidth - 8.f, 46);
			float NameWidth;
			float NameHeight;
			GetTextSize(Profile.Name, NameWidth, NameHeight, Font);
			Line(Profile.Name, bSelected ? FLinearColor::White : Muted, X + 10, 549,
				FMath::Min(1.19f, (CardWidth - 20.f) / FMath::Max(1.f, NameWidth)));
		}
	}
	Box(FLinearColor(.10f,.11f,.23f), 46, 639, 1188, 1);
	const bool bLaunch = Step == (Mode.IsSoloMenuMode() ? 2 : 3);
	Line(bLaunch ? (Match == 5 ? TEXT("ENTER / A     INICIAR RUTA") : TEXT("ENTER / A     INICIAR COMBATE")) : TEXT("ENTER / A     CONTINUAR"), Gold, 72, 661, 1.4f);
	Line(TEXT("FLECHAS / CRUCETA   ELEGIR       ESC / B   ATRAS"), Muted, 729, 661, 1.3f);
}
