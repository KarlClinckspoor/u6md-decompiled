;----------------------------------------
;The far message strings (segment 2BB4, the first of the far data after the code):
;messages printed by the combat, movement, command and system modules, as in Savage
;Empire's and Ultima VI's msgs.c. A '`' makes the console capitalise the next
;character. Assembly, not C: one paragraph-aligned segment holds every string under
;its own public name (Turbo C would give each far variable a segment of its own), and
;the module has an empty WORD-aligned code segment, as Origin's macros gave every
;module: GAME.EXE's segment table lists it at 1649:A, after map.c, where it puts one
;byte of padding before egg.c (MONSDATA.ASM's follows, at the same address).
;----------------------------------------

MSGS_TEXT	SEGMENT WORD PUBLIC 'CODE'
MSGS_TEXT	ENDS

MSGS_DATA	SEGMENT PARA PUBLIC 'FAR_DATA'

PUBLIC	CriticalMsg
CriticalMsg	LABEL	BYTE	;2BB4:0000
	db	'critical!',10,0
PUBLIC	HeavilyMsg
HeavilyMsg	LABEL	BYTE	;2BB4:000B
	db	'heavily',0
PUBLIC	LightlyMsg
LightlyMsg	LABEL	BYTE	;2BB4:0013
	db	'lightly',0
PUBLIC	BarelyMsg
BarelyMsg	LABEL	BYTE	;2BB4:001B
	db	'barely',0
PUBLIC	WoundedMsg
WoundedMsg	LABEL	BYTE	;2BB4:0022
	db	' wounded.',10,0
PUBLIC	DamagedMsg
DamagedMsg	LABEL	BYTE	;2BB4:002D
	db	' damaged.',10,0
PUBLIC	GrazedMsg
GrazedMsg	LABEL	BYTE	;2BB4:0038
	db	'`%s grazed.',10,0
PUBLIC	KilledMsg
KilledMsg	LABEL	BYTE	;2BB4:0045
	db	'`%s killed!',10,0
PUBLIC	DestroyedMsg
DestroyedMsg	LABEL	BYTE	;2BB4:0052
	db	'`%s destroyed!',10,0
PUBLIC	FallsMsg
FallsMsg	LABEL	BYTE	;2BB4:0062
	db	'`%s falls!',10,0
PUBLIC	UnconsciousMsg
UnconsciousMsg	LABEL	BYTE	;2BB4:006E
	db	10,'Overcome by your wounds, you fall unconscious...','.. ',10,0
PUBLIC	AwakenAtMsg
AwakenAtMsg	LABEL	BYTE	;2BB4:00A4
	db	10,'....You awaken back at %s, feeling restored.',10,0
PUBLIC	AttackWithMsg
AttackWithMsg	LABEL	BYTE	;2BB4:00D3
	db	10,'Attack with %s-',0
PUBLIC	AttackMsg
AttackMsg	LABEL	BYTE	;2BB4:00E4
	db	'Attack',0
PUBLIC	BareHandsMsg
BareHandsMsg	LABEL	BYTE	;2BB4:00EB
	db	'with bare hands-',0
PUBLIC	WithMsg
WithMsg	LABEL	BYTE	;2BB4:00FC
	db	'with %s-',0
PUBLIC	OutOfArrowsMsg
OutOfArrowsMsg	LABEL	BYTE	;2BB4:0105
	db	'Out of arrows!',10,0
PUBLIC	OutOfAmmoMsg
OutOfAmmoMsg	LABEL	BYTE	;2BB4:0115
	db	'Out of ammunition!',10,0
PUBLIC	AttackSelfPassMsg
AttackSelfPassMsg	LABEL	BYTE	;2BB4:0129
	db	'pass.',10,0
PUBLIC	PassMsg
PassMsg	LABEL	BYTE	;2BB4:0130
	db	'Pass!',10,0
PUBLIC	CombatMsg
CombatMsg	LABEL	BYTE	;2BB4:0137
	db	'Begin combat!',10,0
PUBLIC	BreakOffMsg
BreakOffMsg	LABEL	BYTE	;2BB4:0146
	db	'Break off combat!',10,0
PUBLIC	SoundOnMsg
SoundOnMsg	LABEL	BYTE	;2BB4:0159
	db	'Sound on',10,0
PUBLIC	SoundOffMsg
SoundOffMsg	LABEL	BYTE	;2BB4:0163
	db	'Sound off',10,0
PUBLIC	MusicOffMsg
MusicOffMsg	LABEL	BYTE	;2BB4:016E
	db	'Music off',10,0
PUBLIC	MusicOnMsg
MusicOnMsg	LABEL	BYTE	;2BB4:0179
	db	'Music on',10,0
PUBLIC	ExitToDOSMsg
ExitToDOSMsg	LABEL	BYTE	;2BB4:0183
	db	'Exit to DOS? ',0
PUBLIC	SaveMsg
SaveMsg	LABEL	BYTE	;2BB4:0191
	db	'Save game? ',0
PUBLIC	SoloModeMsg
SoloModeMsg	LABEL	BYTE	;2BB4:019D
	db	'Solo mode',10,0
PUBLIC	PartyModeMsg
PartyModeMsg	LABEL	BYTE	;2BB4:01A8
	db	'Party mode',10,0
PUBLIC	DirectionMsg
DirectionMsg	LABEL	BYTE	;2BB4:01B4
	db	'Direction-',10,0
PUBLIC	OnMsg
OnMsg	LABEL	BYTE	;2BB4:01C0
	db	'On - ',0
PUBLIC	WhereMsg
WhereMsg	LABEL	BYTE	;2BB4:01C6
	db	'Where? ',0
PUBLIC	PressAnyKeyMsg
PressAnyKeyMsg	LABEL	BYTE	;2BB4:01CE
	db	10,'Press any key...',0
PUBLIC	NoSleepInDreamMsg
NoSleepInDreamMsg	LABEL	BYTE	;2BB4:01E0
	db	'You can',39,'t sleep in a dream.',10,0
PUBLIC	AlreadyLitMsg
AlreadyLitMsg	LABEL	BYTE	;2BB4:01FD
	db	'It',39,'s already lit.',10,0
PUBLIC	WontGoThatWayMsg
WontGoThatWayMsg	LABEL	BYTE	;2BB4:0210
	db	'It won',39,'t go in that direction!',10,0
PUBLIC	BlockedMsg
BlockedMsg	LABEL	BYTE	;2BB4:0230
	db	'Blocked!',10,0
PUBLIC	CantReadMsg
CantReadMsg	LABEL	BYTE	;2BB4:023A
	db	'You can',39,'t read it.',10,0
PUBLIC	TooMuchMsg
TooMuchMsg	LABEL	BYTE	;2BB4:024E
	db	'You are carrying too much already.',10,0
PUBLIC	FailedMsg
FailedMsg	LABEL	BYTE	;2BB4:0272
	db	'Failed.',10,0
PUBLIC	FullyChargedMsg
FullyChargedMsg	LABEL	BYTE	;2BB4:027B
	db	'The %s is fully charged.',10,0
PUBLIC	LockedMsg
LockedMsg	LABEL	BYTE	;2BB4:0295
	db	'It is locked.',10,0
PUBLIC	NoEffectMsg
NoEffectMsg	LABEL	BYTE	;2BB4:02A4
	db	'It has no effect.',10,0
PUBLIC	NoResponseMsg
NoResponseMsg	LABEL	BYTE	;2BB4:02B7
	db	10,'There is no response.',10,0
PUBLIC	NoMsg
NoMsg	LABEL	BYTE	;2BB4:02CF
	db	'No.',10,0
PUBLIC	NotAllHereMsg
NotAllHereMsg	LABEL	BYTE	;2BB4:02D4
	db	'Not everyone is here.',10,0
PUBLIC	NotAtNightMsg
NotAtNightMsg	LABEL	BYTE	;2BB4:02EB
	db	'Not at night!',10,0
PUBLIC	NotInCombatMsg
NotInCombatMsg	LABEL	BYTE	;2BB4:02FA
	db	'Not while in combat mode!',10,0
PUBLIC	NotInSoloMsg
NotInSoloMsg	LABEL	BYTE	;2BB4:0315
	db	'Not while in solo mode!',10,0
PUBLIC	NotNowMsg
NotNowMsg	LABEL	BYTE	;2BB4:032E
	db	'Not now.',10,0
PUBLIC	NotWhileDreamingMsg
NotWhileDreamingMsg	LABEL	BYTE	;2BB4:0338
	db	'Not while dreaming!',10,0
PUBLIC	NotPossibleMsg
NotPossibleMsg	LABEL	BYTE	;2BB4:034D
	db	'That is not possible.',10,0
PUBLIC	NotUndergroundMsg
NotUndergroundMsg	LABEL	BYTE	;2BB4:0364
	db	'Not while underground.',10,0
PUBLIC	WasteOutOfCombatMsg
WasteOutOfCombatMsg	LABEL	BYTE	;2BB4:037C
	db	'That would be a waste outside of combat.',10,0
PUBLIC	OutOfRangeMsg
OutOfRangeMsg	LABEL	BYTE	;2BB4:03A6
	db	'Out of range!',10,0
PUBLIC	MustBeInPartyMsg
MustBeInPartyMsg	LABEL	BYTE	;2BB4:03B5
	db	'You must be in party mode to %s.',10,0
PUBLIC	TotalTooHeavyMsg
TotalTooHeavyMsg	LABEL	BYTE	;2BB4:03D7
	db	10,'The total is too heavy.',10,0
PUBLIC	WhatMsg
WhatMsg	LABEL	BYTE	;2BB4:03F1
	db	'what?',10,0
PUBLIC	NowhereMsg
NowhereMsg	LABEL	BYTE	;2BB4:03F8
	db	'nowhere.',10,0
PUBLIC	NothingMsg
NothingMsg	LABEL	BYTE	;2BB4:0402
	db	'nothing!',10,0
PUBLIC	NobodyMsg
NobodyMsg	LABEL	BYTE	;2BB4:040C
	db	'Nobody.',10,0
PUBLIC	NothingToLightWithMsg
NothingToLightWithMsg	LABEL	BYTE	;2BB4:0415
	db	10,'Nothing to light the %s with.',10,0
PUBLIC	NoFuelMsg
NoFuelMsg	LABEL	BYTE	;2BB4:0435
	db	10,'There is no fuel.',10,0
PUBLIC	HandsFullMsg
HandsFullMsg	LABEL	BYTE	;2BB4:0449
	db	10,'Your hands are full.',10,0
PUBLIC	ExtinguishedMsg
ExtinguishedMsg	LABEL	BYTE	;2BB4:0460
	db	'The %s is extinguished.',10,0
PUBLIC	IncineratedMsg
IncineratedMsg	LABEL	BYTE	;2BB4:0479
	db	10,'You incinerated the %s.',10,0
PUBLIC	AlreadyFullMsg
AlreadyFullMsg	LABEL	BYTE	;2BB4:0493
	db	10,'It',39,'s already full.',10,0
PUBLIC	MoneyPyreMsg
MoneyPyreMsg	LABEL	BYTE	;2BB4:04A8
	db	'The paper money flares into a great pyre!',10,0
PUBLIC	FireMsg
FireMsg	LABEL	BYTE	;2BB4:04D3
	db	10,'Fire!',10,0
PUBLIC	SplatMsg
SplatMsg	LABEL	BYTE	;2BB4:04DB
	db	'Splat! There is no effect.',10,0
PUBLIC	KaboomMsg
KaboomMsg	LABEL	BYTE	;2BB4:04F7
	db	'Kaboom! You blew the door down!',10,0
PUBLIC	HasSteelBallsMsg
HasSteelBallsMsg	LABEL	BYTE	;2BB4:0518
	db	'It already has steel cannon balls.',10,0
PUBLIC	SteelBallsMsg
SteelBallsMsg	LABEL	BYTE	;2BB4:053C
	db	'The cannon will fire steel cannon balls.',10,0
PUBLIC	PressKeysToPlayMsg
PressKeysToPlayMsg	LABEL	BYTE	;2BB4:0566
	db	'Press keys 1-0 to play -',0
PUBLIC	ToccataMsg
ToccataMsg	LABEL	BYTE	;2BB4:057F
	db	'Tocatta and Fugue in D minor (J.S. Bach).',10,0
PUBLIC	AffidavitMsg
AffidavitMsg	LABEL	BYTE	;2BB4:05AA
	db	'--Affidavit--',10,10,0
PUBLIC	AffidavitText1Msg
AffidavitText1Msg	LABEL	BYTE	;2BB4:05BA
	db	'The undersigned do hereby vouch for the ',0
PUBLIC	AffidavitText2Msg
AffidavitText2Msg	LABEL	BYTE	;2BB4:05E3
	db	'trust-',10,'worthiness of $P, who bears ',0
PUBLIC	AffidavitText3Msg
AffidavitText3Msg	LABEL	BYTE	;2BB4:0607
	db	'this chit, commending same to the members ',0
PUBLIC	AffidavitText4Msg
AffidavitText4Msg	LABEL	BYTE	;2BB4:0632
	db	'of the Olympus Mons outpost.',0
PUBLIC	SignedMsg
SignedMsg	LABEL	BYTE	;2BB4:064F
	db	10,10,'*Signed:',10,0
PUBLIC	ShermanMsg
ShermanMsg	LABEL	BYTE	;2BB4:065B
	db	' Richard Sherman',10,0
PUBLIC	DupreyMsg
DupreyMsg	LABEL	BYTE	;2BB4:066D
	db	' Capt. Gregory Duprey',10,0
PUBLIC	YellinMsg
YellinMsg	LABEL	BYTE	;2BB4:0684
	db	' Doctor David Yellin',10,0
PUBLIC	BucketEmptyMsg
BucketEmptyMsg	LABEL	BYTE	;2BB4:069A
	db	'There is nothing in the bucket.',10,0
PUBLIC	PourWaterOnGroundMsg
PourWaterOnGroundMsg	LABEL	BYTE	;2BB4:06BB
	db	'You pour the water onto the ground.',10,0
PUBLIC	PourWaterOnSeedMsg
PourWaterOnSeedMsg	LABEL	BYTE	;2BB4:06E0
	db	'You pour the water onto the seed.',10,0
PUBLIC	NPCAllWetMsg
NPCAllWetMsg	LABEL	BYTE	;2BB4:0703
	db	'%s is all wet.',10,0
PUBLIC	AllWetMsg
AllWetMsg	LABEL	BYTE	;2BB4:0713
	db	'The %s is all wet.',10,0
PUBLIC	PourNitrogenMsg
PourNitrogenMsg	LABEL	BYTE	;2BB4:0727
	db	'You pour the nitrogen compound onto the mound.',10,0
PUBLIC	PourPhosphorousMsg
PourPhosphorousMsg	LABEL	BYTE	;2BB4:0757
	db	'You pour the phosphorous solution onto the mound','.',10,0
PUBLIC	PourPotashMsg
PourPotashMsg	LABEL	BYTE	;2BB4:078A
	db	'You pour the potash mixture onto the mound.',10,0
PUBLIC	SeedAcidMsg
SeedAcidMsg	LABEL	BYTE	;2BB4:07B7
	db	'The seed is destroyed by acid.',10,0
PUBLIC	SeedOilMsg
SeedOilMsg	LABEL	BYTE	;2BB4:07D7
	db	'The seed is poisoned by oil.',10,0
PUBLIC	NothingToDrillMsg
NothingToDrillMsg	LABEL	BYTE	;2BB4:07F5
	db	'There is nothing to drill into.',10,0
PUBLIC	NoRoomForOreMsg
NoRoomForOreMsg	LABEL	BYTE	;2BB4:0816
	db	'There is no room left for the ore.',10,0
PUBLIC	DrillPowerMsg
DrillPowerMsg	LABEL	BYTE	;2BB4:083A
	db	'The drill',39,'s power is connected.',10,0
PUBLIC	EatBerryMsg
EatBerryMsg	LABEL	BYTE	;2BB4:085B
	db	'You eat a Martian berry. You feel %s strange ',0
PUBLIC	BerryRelationshipMsg
BerryRelationshipMsg	LABEL	BYTE	;2BB4:0889
	db	'relationship with things around you.',10,0
PUBLIC	BerrySensitivityMsg
BerrySensitivityMsg	LABEL	BYTE	;2BB4:08AF
	db	'sensitivity to the feelings of objects around yo','u.',10,0
PUBLIC	BerrySpatialMsg
BerrySpatialMsg	LABEL	BYTE	;2BB4:08E3
	db	'sense of your spatial location.',10,0
PUBLIC	BerryOddMsg
BerryOddMsg	LABEL	BYTE	;2BB4:0904
	db	'feelings, like the berries were really odd.',10,0
PUBLIC	BattleFrenzyMsg
BattleFrenzyMsg	LABEL	BYTE	;2BB4:0931
	db	'%s enters a battle frenzy!',10,0
PUBLIC	SwitchUnfixableMsg
SwitchUnfixableMsg	LABEL	BYTE	;2BB4:094D
	db	'This switch cannot be fixed.',10,0
PUBLIC	SwitchFastenedMsg
SwitchFastenedMsg	LABEL	BYTE	;2BB4:096B
	db	'The switch is fastened.',10,0
PUBLIC	SwitchLooseMsg
SwitchLooseMsg	LABEL	BYTE	;2BB4:0984
	db	'The switch is loose.',10,0
PUBLIC	RepairFirstMsg
RepairFirstMsg	LABEL	BYTE	;2BB4:099A
	db	'It must be repaired first.',10,0
PUBLIC	PanelFastenedMsg
PanelFastenedMsg	LABEL	BYTE	;2BB4:09B6
	db	'The panel is fastened.',10,0
PUBLIC	PanelLooseMsg
PanelLooseMsg	LABEL	BYTE	;2BB4:09CE
	db	'The panel is loose.',10,0
PUBLIC	PanelBrokenMsg
PanelBrokenMsg	LABEL	BYTE	;2BB4:09E3
	db	'The panel is broken.',10,0
PUBLIC	PanelNotInstalledMsg
PanelNotInstalledMsg	LABEL	BYTE	;2BB4:09F9
	db	'The panel is not installed.',10,0
PUBLIC	PanelsOnCabinetsMsg
PanelsOnCabinetsMsg	LABEL	BYTE	;2BB4:0A16
	db	'Panels are only installed onto cabinets.',10,0
PUBLIC	WrongPanelMsg
WrongPanelMsg	LABEL	BYTE	;2BB4:0A40
	db	'This cabinet requires a different type of panel.',10,0
PUBLIC	NothingToUnloadMsg
NothingToUnloadMsg	LABEL	BYTE	;2BB4:0A72
	db	'There is nothing to unload.',10,0
PUBLIC	NoMoreRoomMsg
NoMoreRoomMsg	LABEL	BYTE	;2BB4:0A8F
	db	'There is no more room.',10,0
PUBLIC	IntoWheelbarrowMsg
IntoWheelbarrowMsg	LABEL	BYTE	;2BB4:0AA7
	db	'You put the %s into the wheelbarrow.',10,0
PUBLIC	CantMixLoadMsg
CantMixLoadMsg	LABEL	BYTE	;2BB4:0ACD
	db	'You can',39,'t mix different things in the same load!',10,0
PUBLIC	IntoRailCarMsg
IntoRailCarMsg	LABEL	BYTE	;2BB4:0AFF
	db	'You put the %s into the rail car.',10,0
PUBLIC	CantBuryHereMsg
CantBuryHereMsg	LABEL	BYTE	;2BB4:0B22
	db	'You can',39,'t bury anything here.',10,0
PUBLIC	PlantedSeedMsg
PlantedSeedMsg	LABEL	BYTE	;2BB4:0B41
	db	'You planted the Martian seed.',10,0
PUBLIC	FilledHoleMsg
FilledHoleMsg	LABEL	BYTE	;2BB4:0B60
	db	'You filled in the hole.',10,0
PUBLIC	CantDigHereMsg
CantDigHereMsg	LABEL	BYTE	;2BB4:0B79
	db	'You can',39,'t dig here.',10,0
PUBLIC	UprootedPodMsg
UprootedPodMsg	LABEL	BYTE	;2BB4:0B8E
	db	'You uprooted the pod plant.',10,0
PUBLIC	MartianFallsOutMsg
MartianFallsOutMsg	LABEL	BYTE	;2BB4:0BAB
	db	'An unconscious Martian falls out of the pod.',10,0
PUBLIC	PriedNailsMsg
PriedNailsMsg	LABEL	BYTE	;2BB4:0BD9
	db	'You pried the nails out of the crate.',10,0
PUBLIC	NotFullyGrownMsg
NotFullyGrownMsg	LABEL	BYTE	;2BB4:0C00
	db	'It is not fully grown.',10,0
PUBLIC	FastenedSecurelyMsg
FastenedSecurelyMsg	LABEL	BYTE	;2BB4:0C18
	db	'It is fastened securely.',10,0
PUBLIC	TurnsLooselyMsg
TurnsLooselyMsg	LABEL	BYTE	;2BB4:0C32
	db	'It turns loosely.',10,0
PUBLIC	NewSettingMsg
NewSettingMsg	LABEL	BYTE	;2BB4:0C45
	db	'  New setting (R%/S%/C)?',10,0
PUBLIC	LeverStuckMsg
LeverStuckMsg	LABEL	BYTE	;2BB4:0C5F
	db	'the lever seems stuck.',10,0
PUBLIC	PushLeverMsg
PushLeverMsg	LABEL	BYTE	;2BB4:0C77
	db	'Push lever to ',0
PUBLIC	LatLongMsg
LatLongMsg	LABEL	BYTE	;2BB4:0C86
	db	'You are somewhere near ',10,'Lat:%d %c',10,'Long:%d %c',10,0
PUBLIC	NeedCueStickMsg
NeedCueStickMsg	LABEL	BYTE	;2BB4:0CB4
	db	'You need a cue stick in your hands.',10,0
PUBLIC	NeedPipeOrganMsg
NeedPipeOrganMsg	LABEL	BYTE	;2BB4:0CD9
	db	'Organ music must be played on a pipe organ.',10,0
PUBLIC	WhichRailMsg
WhichRailMsg	LABEL	BYTE	;2BB4:0D06
	db	'Which section of rail needs fixing -',10,0
PUBLIC	RadiumAlreadyMsg
RadiumAlreadyMsg	LABEL	BYTE	;2BB4:0D2C
	db	'Radium has already been installed.',10,0
PUBLIC	VeryHotMsg
VeryHotMsg	LABEL	BYTE	;2BB4:0D50
	db	'OUCH. It is very hot.',10,0
PUBLIC	RadiumBlocksMsg
RadiumBlocksMsg	LABEL	BYTE	;2BB4:0D67
	db	'The %s only needed %d radium blocks.',10,0
PUBLIC	TongsProtectMsg
TongsProtectMsg	LABEL	BYTE	;2BB4:0D8D
	db	'The tongs will now protect your hands.',10,0
PUBLIC	RadiumInstalledMsg
RadiumInstalledMsg	LABEL	BYTE	;2BB4:0DB5
	db	'The radium has been installed.',10,0
PUBLIC	HeadgearInstalledMsg
HeadgearInstalledMsg	LABEL	BYTE	;2BB4:0DD5
	db	'The headgear is installed.',10,0
PUBLIC	MachineDoesNotWorkMsg
MachineDoesNotWorkMsg	LABEL	BYTE	;2BB4:0DF1
	db	'The machine does not work.',10,0
PUBLIC	NobodyInMachineMsg
NobodyInMachineMsg	LABEL	BYTE	;2BB4:0E0D
	db	'There is nobody sitting in the machine.',10,0
PUBLIC	DreamMachinesCeasedMsg
DreamMachinesCeasedMsg	LABEL	BYTE	;2BB4:0E36
	db	'The dream machines seem to have ceased functioni','ng.',10,0
PUBLIC	SprayerRubberMsg
SprayerRubberMsg	LABEL	BYTE	;2BB4:0E6B
	db	'The sprayer system now has rubber.',10,0
PUBLIC	SprayerDyeMsg
SprayerDyeMsg	LABEL	BYTE	;2BB4:0E8F
	db	'The sprayer system now has dye.',10,0
PUBLIC	NobodyUnderSprayerMsg
NobodyUnderSprayerMsg	LABEL	BYTE	;2BB4:0EB0
	db	'Nobody is standing under the sprayer.',10,0
PUBLIC	NoRubberCoatMsg
NoRubberCoatMsg	LABEL	BYTE	;2BB4:0ED7
	db	'You don',39,'t think it would be safe to coat an animal or pl'
	db	'ant with rubber.',10,0
PUBLIC	ChsheketTreatedMsg
ChsheketTreatedMsg	LABEL	BYTE	;2BB4:0F21
	db	'Chsheket has already been treated.',10,0
PUBLIC	NoRubberMsg
NoRubberMsg	LABEL	BYTE	;2BB4:0F45
	db	'There is no rubber in the sprayer system.',10,0
PUBLIC	RubberNotFleshMsg
RubberNotFleshMsg	LABEL	BYTE	;2BB4:0F70
	db	'The rubber in the sprayer system is not flesh-co','lored.',10,0
PUBLIC	AlsoNotFleshMsg
AlsoNotFleshMsg	LABEL	BYTE	;2BB4:0FA8
	db	'It is also not flesh-colored.',10,0
PUBLIC	KilledPlantMsg
KilledPlantMsg	LABEL	BYTE	;2BB4:0FC7
	db	'You killed the plant.',10,0
PUBLIC	BecomesVisibleMsg
BecomesVisibleMsg	LABEL	BYTE	;2BB4:0FDE
	db	'`%s becomes visible!',10,0
PUBLIC	RollersUnfixableMsg
RollersUnfixableMsg	LABEL	BYTE	;2BB4:0FF4
	db	'Ooops. These rollers can never be fixed!',10,0
PUBLIC	OxiumFromBinMsg
OxiumFromBinMsg	LABEL	BYTE	;2BB4:101E
	db	'You get two handfuls of oxium from the bin.',10,0
PUBLIC	DoorNotRustedMsg
DoorNotRustedMsg	LABEL	BYTE	;2BB4:104B
	db	'This door is not rusted.',10,0
PUBLIC	DoorUnstuckMsg
DoorUnstuckMsg	LABEL	BYTE	;2BB4:1065
	db	'The door is unstuck.',10,0
PUBLIC	ImagineMsg
ImagineMsg	LABEL	BYTE	;2BB4:107B
	db	'You imagine ',0
PUBLIC	PullDreamstuffMsg
PullDreamstuffMsg	LABEL	BYTE	;2BB4:1088
	db	'You pull some of the dreamstuff from the hoard.',10,0
PUBLIC	LackMentalPowerMsg
LackMentalPowerMsg	LABEL	BYTE	;2BB4:10B9
	db	'You lack the mental power to affect the dreamstu','ff hoard.'
	db	10,0
PUBLIC	LensesNotRespondingMsg
LensesNotRespondingMsg	LABEL	BYTE	;2BB4:10F4
	db	'Some of the lenses are not responding.',10,0
PUBLIC	NoWaterPressureMsg
NoWaterPressureMsg	LABEL	BYTE	;2BB4:111C
	db	'There is no water pressure in the valve system.',10,0
PUBLIC	NoBacktalkMsg
NoBacktalkMsg	LABEL	BYTE	;2BB4:114D
	db	'No backtalk from you, young %sman!',10,0
PUBLIC	UsingMsg
UsingMsg	LABEL	BYTE	;2BB4:1171
	db	'`(using %s)',0
PUBLIC	FillBucketMsg
FillBucketMsg	LABEL	BYTE	;2BB4:117D
	db	'`You fill the bucket with water.',10,0
PUBLIC	DumpIceMsg
DumpIceMsg	LABEL	BYTE	;2BB4:119F
	db	'`Dump out the ice? ',0
PUBLIC	WormEatsPodMsg
WormEatsPodMsg	LABEL	BYTE	;2BB4:11B3
	db	'`The cave worm eats the pod plant!',10,0
PUBLIC	DontBuryPeopleMsg
DontBuryPeopleMsg	LABEL	BYTE	;2BB4:11D7
	db	'`Don',39,'t bury people on Mars!',10,0
PUBLIC	AutomaticDoorMsg
AutomaticDoorMsg	LABEL	BYTE	;2BB4:11F4
	db	'`It is automatic and can',39,'t be opened by hand.',10,0
PUBLIC	RustedShutMsg
RustedShutMsg	LABEL	BYTE	;2BB4:1223
	db	'It is apparently rusted shut.',10,0
PUBLIC	ApparentlyLockedMsg
ApparentlyLockedMsg	LABEL	BYTE	;2BB4:1242
	db	'It is apparently locked.',10,0
PUBLIC	NoPowerMsg
NoPowerMsg	LABEL	BYTE	;2BB4:125C
	db	'It has no power.',10,0
PUBLIC	TooPreciseMsg
TooPreciseMsg	LABEL	BYTE	;2BB4:126E
	db	'The work is too precise to perform telekinetical','ly.',10,0
PUBLIC	NoMoreDirtMsg
NoMoreDirtMsg	LABEL	BYTE	;2BB4:12A3
	db	'You can',39,'t pile any more dirt there.',10,0
PUBLIC	CanalsWorkingMsg
CanalsWorkingMsg	LABEL	BYTE	;2BB4:12C8
	db	'The canals are already working.',10,0
PUBLIC	LensesAlignedMsg
LensesAlignedMsg	LABEL	BYTE	;2BB4:12E9
	db	'The ruby lenses are already aligned.',10,0
PUBLIC	GoodShotMsg
GoodShotMsg	LABEL	BYTE	;2BB4:130F
	db	'Good shot, old bean!',10,0
PUBLIC	WheezeMsg
WheezeMsg	LABEL	BYTE	;2BB4:1325
	db	'Wheeeeze',10,0
PUBLIC	ChingMsg
ChingMsg	LABEL	BYTE	;2BB4:132F
	db	'Ching!',10,0
PUBLIC	ThumpMsg
ThumpMsg	LABEL	BYTE	;2BB4:1337
	db	'Thump! Thump!',10,0
PUBLIC	ShingMsg
ShingMsg	LABEL	BYTE	;2BB4:1346
	db	'Shing!',10,0
PUBLIC	HearSomethingMsg
HearSomethingMsg	LABEL	BYTE	;2BB4:134E
	db	'You hear something to the %s!',10,0
PUBLIC	PoisonedMsg
PoisonedMsg	LABEL	BYTE	;2BB4:136D
	db	'`%s is poisoned!',10,0
PUBLIC	BurnOutMsg
BurnOutMsg	LABEL	BYTE	;2BB4:137F
	db	'A torch burns out.',10,0
PUBLIC	AttacksMsg
AttacksMsg	LABEL	BYTE	;2BB4:1393
	db	'`%s attacks!',10,0
PUBLIC	HealedMsg
HealedMsg	LABEL	BYTE	;2BB4:13A1
	db	10,'%s is healed.',10,0
PUBLIC	ParalyzedMsg
ParalyzedMsg	LABEL	BYTE	;2BB4:13B1
	db	'`%s paralyzed!',10,0
PUBLIC	FailsToRespondMsg
FailsToRespondMsg	LABEL	BYTE	;2BB4:13C1
	db	'`%s fails to respond.',10,0
PUBLIC	ControlsADreamMachineMsg
ControlsADreamMachineMsg	LABEL	BYTE	;2BB4:13D8
	db	'It appears to control a dream machine.',10,0
PUBLIC	ControlsASpraySystemMsg
ControlsASpraySystemMsg	LABEL	BYTE	;2BB4:1400
	db	'It appears to control a spray system.',10,0
PUBLIC	ControlsRubyLensMsg
ControlsRubyLensMsg	LABEL	BYTE	;2BB4:1427
	db	'It appears to control the ruby lens system.',10,0
PUBLIC	ControlsCisternMsg
ControlsCisternMsg	LABEL	BYTE	;2BB4:1454
	db	'It appears to control the cistern valves.',10,0
PUBLIC	MechanicalControlsMsg
MechanicalControlsMsg	LABEL	BYTE	;2BB4:147F
	db	'These appear to be mechanical controls.',10,0
PUBLIC	ValveControlsMsg
ValveControlsMsg	LABEL	BYTE	;2BB4:14A8
	db	'These appear to be valve controls.',10,0
PUBLIC	ElectricalControlsMsg
ElectricalControlsMsg	LABEL	BYTE	;2BB4:14CC
	db	'These appear to be electrical controls.',10,0
PUBLIC	CantDecipherPurposeMsg
CantDecipherPurposeMsg	LABEL	BYTE	;2BB4:14F5
	db	'You cannot decipher its purpose.',10,0
PUBLIC	ControlsThisDreamMachineMsg
ControlsThisDreamMachineMsg	LABEL	BYTE	;2BB4:1517
	db	'It appears to control this dream machine.',10,0
PUBLIC	ControlsThisSpraySystemMsg
ControlsThisSpraySystemMsg	LABEL	BYTE	;2BB4:1542
	db	'It appears to control this spray system.',10,0
PUBLIC	ControlsRubyLensMsg2
ControlsRubyLensMsg2	LABEL	BYTE	;2BB4:156C
	db	'It appears to control the ruby lens system.',10,0
PUBLIC	ControlsCisternMsg2
ControlsCisternMsg2	LABEL	BYTE	;2BB4:1599
	db	'It appears to control the cistern valves.',10,0
PUBLIC	ControlsMechanicalMsg
ControlsMechanicalMsg	LABEL	BYTE	;2BB4:15C4
	db	'It appears to control a mechanical system.',10,0
PUBLIC	ControlsValveMsg
ControlsValveMsg	LABEL	BYTE	;2BB4:15F0
	db	'It appears to control a valve system.',10,0
PUBLIC	ControlsElectricalMsg
ControlsElectricalMsg	LABEL	BYTE	;2BB4:1617
	db	'It appears to control an electrical system.',10,0
PUBLIC	CantDecipherControlsMsg
CantDecipherControlsMsg	LABEL	BYTE	;2BB4:1644
	db	'You cannot decipher what it controls.',10,0
PUBLIC	NoApparentEffectMsg
NoApparentEffectMsg	LABEL	BYTE	;2BB4:166B
	db	'You actuate the mechanism to no apparent effect.',10,0
PUBLIC	StatusLightsChangeMsg
StatusLightsChangeMsg	LABEL	BYTE	;2BB4:169D
	db	'Status lights change but you see no other effect','.',10,0
PUBLIC	LightsFlashMsg
LightsFlashMsg	LABEL	BYTE	;2BB4:16D0
	db	'Lights flash and change color but nothing else h','appens.',10
	db	0
PUBLIC	UnsureWhatYouDidMsg
UnsureWhatYouDidMsg	LABEL	BYTE	;2BB4:1709
	db	'You are completely unsure what you just did.',10,0
PUBLIC	IceMeltedMsg
IceMeltedMsg	LABEL	BYTE	;2BB4:1737
	db	'Some ice has melted.',10,0
PUBLIC	KilledHelperMsg
KilledHelperMsg	LABEL	BYTE	;2BB4:174D
	db	'You have killed someone whose help you need to w'
	db	'in the game!',10,0
PUBLIC	HistoricalFigureDeadMsg
HistoricalFigureDeadMsg	LABEL	BYTE	;2BB4:178B
	db	'One of the important historical figures you were'
	db	' here to rescue is dead!',10,0
PUBLIC	GroundNotFlatMsg
GroundNotFlatMsg	LABEL	BYTE	;2BB4:17D5
	db	'The ground is not flat enough.',10,0
PUBLIC	InTheWayMsg
InTheWayMsg	LABEL	BYTE	;2BB4:17F5
	db	'The %s is in the way.',10,0
PUBLIC	DrillOnCartMsg
DrillOnCartMsg	LABEL	BYTE	;2BB4:180C
	db	'The drill must be installed onto a drill cart.',10,0
PUBLIC	NotStrongEnoughMsg
NotStrongEnoughMsg	LABEL	BYTE	;2BB4:183C
	db	'%s is not strong enough.',10,0
PUBLIC	HasHeartstoneAlreadyMsg
HasHeartstoneAlreadyMsg	LABEL	BYTE	;2BB4:1856
	db	'The robot already has a heartstone.',10,0
PUBLIC	HeartstoneInstalledMsg
HeartstoneInstalledMsg	LABEL	BYTE	;2BB4:187B
	db	'The heartstone is installed.',10,0
PUBLIC	PodAlreadyOpenMsg
PodAlreadyOpenMsg	LABEL	BYTE	;2BB4:1899
	db	'The pod is already open.',10,0
PUBLIC	CantDigDeeperMsg
CantDigDeeperMsg	LABEL	BYTE	;2BB4:18B3
	db	'You can',39,'t dig any deeper.',10,0
PUBLIC	StuckMsg
StuckMsg	LABEL	BYTE	;2BB4:18CE
	db	'It is stuck.',10,0
PUBLIC	PanelOperatesDreamMachineMsg
PanelOperatesDreamMachineMsg	LABEL	BYTE	;2BB4:18DC
	db	'The control panel operates the dream machine.',10,0
PUBLIC	NeedPliersMsg
NeedPliersMsg	LABEL	BYTE	;2BB4:190B
	db	'You need to use pliers to attach the cable.',10,0
PUBLIC	ChewInstinctivelyMsg
ChewInstinctivelyMsg	LABEL	BYTE	;2BB4:1938
	db	'You will chew it instinctively.',10,0
PUBLIC	GroundNotClearMsg
GroundNotClearMsg	LABEL	BYTE	;2BB4:1959
	db	'The ground is not clear for digging.',10,0
PUBLIC	BuryInHoleMsg
BuryInHoleMsg	LABEL	BYTE	;2BB4:197F
	db	'You must bury the %s in a hole.',10,0
PUBLIC	CoveredWithDirtMsg
CoveredWithDirtMsg	LABEL	BYTE	;2BB4:19A0
	db	'It is covered with dirt.',10,0
PUBLIC	TheyCoveredWithDirtMsg
TheyCoveredWithDirtMsg	LABEL	BYTE	;2BB4:19BA
	db	'They have been covered with dirt.',10,0
PUBLIC	NoSprayerNearMsg
NoSprayerNearMsg	LABEL	BYTE	;2BB4:19DD
	db	'Funny, no sprayer near sprayer controls!',10,0
PUBLIC	AlignmentMsg
AlignmentMsg	LABEL	BYTE	;2BB4:1A07
	db	'Alignment coordinate:',10,0
PUBLIC	WaitUntilDawnMsg
WaitUntilDawnMsg	LABEL	BYTE	;2BB4:1A1E
	db	'You wait until dawn...*',10,0
PUBLIC	NoSeedYetMsg
NoSeedYetMsg	LABEL	BYTE	;2BB4:1A37
	db	'No seed to harvest yet.',10,0
PUBLIC	LocationMsg
LocationMsg	LABEL	BYTE	;2BB4:1A50
	db	'Location: ',0
PUBLIC	WontGrowMsg
WontGrowMsg	LABEL	BYTE	;2BB4:1A5B
	db	'Won',39,'t grow there!',10,0
PUBLIC	NeedsWateringMsg
NeedsWateringMsg	LABEL	BYTE	;2BB4:1A6E
	db	'Now it just needs watering.',10,0
PUBLIC	MouthTooDryMsg
MouthTooDryMsg	LABEL	BYTE	;2BB4:1A8B
	db	'Your mouth is too dry.',10,0
PUBLIC	SpitMsg
SpitMsg	LABEL	BYTE	;2BB4:1AA3
	db	'You spit into the spittoon.',10,0
PUBLIC	ClickMsg
ClickMsg	LABEL	BYTE	;2BB4:1AC0
	db	'Click.',10,0
PUBLIC	RubySlippersMsg
RubySlippersMsg	LABEL	BYTE	;2BB4:1AC8
	db	'You may use the ruby slippers to go home. If you'
	db	' choose to, you may now view the Grand Finale.',10,10,'*',0
PUBLIC	FinaleExitMsg
FinaleExitMsg	LABEL	BYTE	;2BB4:1B2A
	db	'Afterwards, you will exit to DOS. You will be ab'
	db	'le to resume play from your last saved game.',10,10,'*',0
PUBLIC	SeeFinaleMsg
SeeFinaleMsg	LABEL	BYTE	;2BB4:1B8A
	db	'Would you like to see the Grand Finale now? If s'
	db	'o, press Y. To resume play now, hit N: (Y N)',10,0
PUBLIC	BrokenMsg
BrokenMsg	LABEL	BYTE	;2BB4:1BE8
	db	'The %s is broken.',10,0
PUBLIC	CantInstallThereMsg
CantInstallThereMsg	LABEL	BYTE	;2BB4:1BFB
	db	'You cannot install it there.',10,0
PUBLIC	CantBeUsedMsg
CantBeUsedMsg	LABEL	BYTE	;2BB4:1C19
	db	'%s can',39,'t be used.',10,0
PUBLIC	HasNoEffectOnMsg
HasNoEffectOnMsg	LABEL	BYTE	;2BB4:1C2C
	db	'%s has no effect on ',0
PUBLIC	HasNoEffectMsg
HasNoEffectMsg	LABEL	BYTE	;2BB4:1C41
	db	'%s has no effect.',10,0
PUBLIC	NailedShutMsg
NailedShutMsg	LABEL	BYTE	;2BB4:1C54
	db	'It is nailed shut.',10,0
PUBLIC	BlankLineMsg
BlankLineMsg	LABEL	BYTE	;2BB4:1C68
	db	'              ',10,0
PUBLIC	FindNothingMsg
FindNothingMsg	LABEL	BYTE	;2BB4:1C78
	db	10,'Searching here, you find nothing.',10,0
PUBLIC	SearchFindMsg
SearchFindMsg	LABEL	BYTE	;2BB4:1C9C
	db	10,'Searching here, you find ',0
PUBLIC	YouSeeMsg
YouSeeMsg	LABEL	BYTE	;2BB4:1CB7
	db	'You see ',0
PUBLIC	DarknessMsg
DarknessMsg	LABEL	BYTE	;2BB4:1CC0
	db	'darkness.',10,0
PUBLIC	YourselfMsg
YourselfMsg	LABEL	BYTE	;2BB4:1CCB
	db	'Yourself',10,0
PUBLIC	GrowingMsg
GrowingMsg	LABEL	BYTE	;2BB4:1CD5
	db	'It is growing.',10,0
PUBLIC	RipeMsg
RipeMsg	LABEL	BYTE	;2BB4:1CE5
	db	'It is ripe.',10,0
PUBLIC	NotGrowingMsg
NotGrowingMsg	LABEL	BYTE	;2BB4:1CF2
	db	'It is not growing.',10,0
PUBLIC	HasHeartstoneMsg
HasHeartstoneMsg	LABEL	BYTE	;2BB4:1D06
	db	'It has a heartstone.',10,0
PUBLIC	EmptyMsg
EmptyMsg	LABEL	BYTE	;2BB4:1D1C
	db	'It is empty.',10,0
PUBLIC	HasLoadsMsg
HasLoadsMsg	LABEL	BYTE	;2BB4:1D2A
	db	'It has %d load\\s of ',0
PUBLIC	TimeIsMsg
TimeIsMsg	LABEL	BYTE	;2BB4:1D40
	db	10,10,'The time is ',0
PUBLIC	SetToMsg
SetToMsg	LABEL	BYTE	;2BB4:1D4F
	db	'It is set to ',39,'%s',39,0
PUBLIC	ChargesLeftMsg
ChargesLeftMsg	LABEL	BYTE	;2BB4:1D61
	db	', and has %d charge\\s left.',10,0
PUBLIC	AppearsLooseMsg
AppearsLooseMsg	LABEL	BYTE	;2BB4:1D7F
	db	'It appears to be loose.',10,0
PUBLIC	AppearsBrokenMsg
AppearsBrokenMsg	LABEL	BYTE	;2BB4:1D98
	db	'It appears to be broken.',10,0
PUBLIC	LoadedWithMsg
LoadedWithMsg	LABEL	BYTE	;2BB4:1DB2
	db	'It',39,'s loaded with %s',0
PUBLIC	ReadyToUseMsg
ReadyToUseMsg	LABEL	BYTE	;2BB4:1DC6
	db	'It is ready to use.',10,0
PUBLIC	CantDropOnGroundMsg
CantDropOnGroundMsg	LABEL	BYTE	;2BB4:1DDB
	db	'The %s cannot be dropped on the ground.',10,0
PUBLIC	SearchBodyMsg
SearchBodyMsg	LABEL	BYTE	;2BB4:1E04
	db	10,'Searching the body, you find ',0
PUBLIC	WontBudgeMsg
WontBudgeMsg	LABEL	BYTE	;2BB4:1E23
	db	'Won',39,'t budge !',10,0
PUBLIC	IceMeltingMsg
IceMeltingMsg	LABEL	BYTE	;2BB4:1E32
	db	'The ice is melting.',10,0
PUBLIC	CantMoveOnGroundMsg
CantMoveOnGroundMsg	LABEL	BYTE	;2BB4:1E47
	db	'The %s cannot be moved onto the ground.',10,0
PUBLIC	PersonNotHereMsg
PersonNotHereMsg	LABEL	BYTE	;2BB4:1E70
	db	10,'That person isn',39,'t here!',10,0
PUBLIC	NPCTooMuchMsg
NPCTooMuchMsg	LABEL	BYTE	;2BB4:1E8A
	db	'%s is carrying too much already.',10,0
PUBLIC	UnreadyFirstMsg
UnreadyFirstMsg	LABEL	BYTE	;2BB4:1EAC
	db	10,10,'You have to unready it first.',10,0
PUBLIC	InsideItselfMsg
InsideItselfMsg	LABEL	BYTE	;2BB4:1ECD
	db	10,10,'An item can',39,'t be placed inside itself!',10,0
PUBLIC	CantDoThatMsg
CantDoThatMsg	LABEL	BYTE	;2BB4:1EF7
	db	10,10,'You can',39,'t do that!',10,0
PUBLIC	ContainerFullMsg
ContainerFullMsg	LABEL	BYTE	;2BB4:1F0D
	db	'That container is full.',10,0
PUBLIC	RobotCantEatMsg
RobotCantEatMsg	LABEL	BYTE	;2BB4:1F26
	db	'A mechanical person can',39,'t eat berries!',10,0
PUBLIC	PlateExposedMsg
PlateExposedMsg	LABEL	BYTE	;2BB4:1F4E
	db	'The plate in the camera has already been exposed','.',10,0
PUBLIC	NeedShovelMsg
NeedShovelMsg	LABEL	BYTE	;2BB4:1F81
	db	'You must use a shovel to move that.',10,0
PUBLIC	BucketNotEmptyMsg
BucketNotEmptyMsg	LABEL	BYTE	;2BB4:1FA6
	db	'There is already something in the bucket.',10,0
PUBLIC	CantReadTimeMsg
CantReadTimeMsg	LABEL	BYTE	;2BB4:1FD1
	db	'You can',39,'t determine how to read the time.',10,0
PUBLIC	NailCrateMsg
NailCrateMsg	LABEL	BYTE	;2BB4:1FFC
	db	'You nail the crate shut.',10,0
PUBLIC	SprayGunRefillMsg
SprayGunRefillMsg	LABEL	BYTE	;2BB4:2016
	db	'The spray gun has 10 more charges.',10,0
PUBLIC	SprayGunPaintMsg
SprayGunPaintMsg	LABEL	BYTE	;2BB4:203A
	db	'The spray gun now contains 20 charges of green p','aint.',10,0
PUBLIC	SprayGunWeedKillerMsg
SprayGunWeedKillerMsg	LABEL	BYTE	;2BB4:2071
	db	'The spray gun now contains 10 charges of weed ki','ller.',10,0
PUBLIC	SprayGunEmptyMsg
SprayGunEmptyMsg	LABEL	BYTE	;2BB4:20A8
	db	'There is nothing in the gun.',10,0
PUBLIC	IceThawsMsg
IceThawsMsg	LABEL	BYTE	;2BB4:20C6
	db	'The ice thaws.',10,0
PUBLIC	WaterFreezesMsg
WaterFreezesMsg	LABEL	BYTE	;2BB4:20D6
	db	'The water freezes.',10,0
PUBLIC	CantBuryWithItselfMsg
CantBuryWithItselfMsg	LABEL	BYTE	;2BB4:20EA
	db	'You can',39,'t bury something using itself!',10,0
PUBLIC	CableFineMsg
CableFineMsg	LABEL	BYTE	;2BB4:2112
	db	'The cable does not need replacement.',10,0
PUBLIC	BerryNauseaMsg
BerryNauseaMsg	LABEL	BYTE	;2BB4:2138
	db	'sudden flush, dizziness, and nausea.',10,0
PUBLIC	NoEmsMsg
NoEmsMsg	LABEL	BYTE	;2BB4:215E
	db	'No EMS found; no music possible.',10,0
PUBLIC	StokerBlockedMsg
StokerBlockedMsg	LABEL	BYTE	;2BB4:2180
	db	'Stoker',39,'s path is blocked!',10,0
PUBLIC	AlienDeviceMsg
AlienDeviceMsg	LABEL	BYTE	;2BB4:219B
	db	'The alien device doesn',39,'t seem to be functioning.',10,0
PUBLIC	NothingHappensMsg
NothingHappensMsg	LABEL	BYTE	;2BB4:21CD
	db	'Nothing appears to happen.',10,0
PUBLIC	PsychicPowerFadesMsg
PsychicPowerFadesMsg	LABEL	BYTE	;2BB4:21E9
	db	'A psychic power fades...',10,0
PUBLIC	CoalMovingMsg
CoalMovingMsg	LABEL	BYTE	;2BB4:2203
	db	'The coal begins moving down the belt.',10,0
PUBLIC	BeltStopsMsg
BeltStopsMsg	LABEL	BYTE	;2BB4:222A
	db	'The conveyor belt stops.',10,0
PUBLIC	BerriesNotEdibleMsg
BerriesNotEdibleMsg	LABEL	BYTE	;2BB4:2244
	db	'The berries are not edible.',10,0
PUBLIC	TooManyTreesMsg
TooManyTreesMsg	LABEL	BYTE	;2BB4:2261
	db	'There are too many moving trees nearby.',10,0
PUBLIC	NoAirNeededMsg
NoAirNeededMsg	LABEL	BYTE	;2BB4:228A
	db	'The %s doesn',39,'t need oxygenated air.',10,0
PUBLIC	BeyondHelpMsg
BeyondHelpMsg	LABEL	BYTE	;2BB4:22AF
	db	'%s is beyond help.',10,0
PUBLIC	RevivedMsg
RevivedMsg	LABEL	BYTE	;2BB4:22C3
	db	'%s is revived!',10,0
PUBLIC	DontKnowHowMsg
DontKnowHowMsg	LABEL	BYTE	;2BB4:22D3
	db	'You don',39,'t know how it works.',10,0
PUBLIC	AarghMsg
AarghMsg	LABEL	BYTE	;2BB4:22F1
	db	'Aargh!',10,0
PUBLIC	VersionMsg
VersionMsg	LABEL	BYTE	;2BB4:22F9
	db	'Version 1.4',10,0

MSGS_DATA	ENDS

END
