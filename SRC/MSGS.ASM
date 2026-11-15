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

PUBLIC	C_2BB4_0000
C_2BB4_0000	LABEL	BYTE	;2BB4:0000
	db	'critical!',10,0
PUBLIC	C_2BB4_000B
C_2BB4_000B	LABEL	BYTE	;2BB4:000B
	db	'heavily',0
PUBLIC	C_2BB4_0013
C_2BB4_0013	LABEL	BYTE	;2BB4:0013
	db	'lightly',0
PUBLIC	C_2BB4_001B
C_2BB4_001B	LABEL	BYTE	;2BB4:001B
	db	'barely',0
PUBLIC	C_2BB4_0022
C_2BB4_0022	LABEL	BYTE	;2BB4:0022
	db	' wounded.',10,0
PUBLIC	C_2BB4_002D
C_2BB4_002D	LABEL	BYTE	;2BB4:002D
	db	' damaged.',10,0
PUBLIC	C_2BB4_0038
C_2BB4_0038	LABEL	BYTE	;2BB4:0038
	db	'`%s grazed.',10,0
PUBLIC	C_2BB4_0045
C_2BB4_0045	LABEL	BYTE	;2BB4:0045
	db	'`%s killed!',10,0
PUBLIC	C_2BB4_0052
C_2BB4_0052	LABEL	BYTE	;2BB4:0052
	db	'`%s destroyed!',10,0
PUBLIC	C_2BB4_0062
C_2BB4_0062	LABEL	BYTE	;2BB4:0062
	db	'`%s falls!',10,0
PUBLIC	C_2BB4_006E
C_2BB4_006E	LABEL	BYTE	;2BB4:006E
	db	10,'Overcome by your wounds, you fall unconscious...','.. ',10,0
PUBLIC	C_2BB4_00A4
C_2BB4_00A4	LABEL	BYTE	;2BB4:00A4
	db	10,'....You awaken back at %s, feeling restored.',10,0
PUBLIC	C_2BB4_00D3
C_2BB4_00D3	LABEL	BYTE	;2BB4:00D3
	db	10,'Attack with %s-',0
PUBLIC	C_2BB4_00E4
C_2BB4_00E4	LABEL	BYTE	;2BB4:00E4
	db	'Attack',0
PUBLIC	C_2BB4_00EB
C_2BB4_00EB	LABEL	BYTE	;2BB4:00EB
	db	'with bare hands-',0
PUBLIC	C_2BB4_00FC
C_2BB4_00FC	LABEL	BYTE	;2BB4:00FC
	db	'with %s-',0
PUBLIC	C_2BB4_0105
C_2BB4_0105	LABEL	BYTE	;2BB4:0105
	db	'Out of arrows!',10,0
PUBLIC	C_2BB4_0115
C_2BB4_0115	LABEL	BYTE	;2BB4:0115
	db	'Out of ammunition!',10,0
PUBLIC	C_2BB4_0129
C_2BB4_0129	LABEL	BYTE	;2BB4:0129
	db	'pass.',10,0
PUBLIC	C_2BB4_0130
C_2BB4_0130	LABEL	BYTE	;2BB4:0130
	db	'Pass!',10,0
PUBLIC	C_2BB4_0137
C_2BB4_0137	LABEL	BYTE	;2BB4:0137
	db	'Begin combat!',10,0
PUBLIC	C_2BB4_0146
C_2BB4_0146	LABEL	BYTE	;2BB4:0146
	db	'Break off combat!',10,0
PUBLIC	C_2BB4_0159
C_2BB4_0159	LABEL	BYTE	;2BB4:0159
	db	'Sound on',10,0
PUBLIC	C_2BB4_0163
C_2BB4_0163	LABEL	BYTE	;2BB4:0163
	db	'Sound off',10,0
PUBLIC	C_2BB4_016E
C_2BB4_016E	LABEL	BYTE	;2BB4:016E
	db	'Music off',10,0
PUBLIC	C_2BB4_0179
C_2BB4_0179	LABEL	BYTE	;2BB4:0179
	db	'Music on',10,0
PUBLIC	C_2BB4_0183
C_2BB4_0183	LABEL	BYTE	;2BB4:0183
	db	'Exit to DOS? ',0
PUBLIC	C_2BB4_0191
C_2BB4_0191	LABEL	BYTE	;2BB4:0191
	db	'Save game? ',0
PUBLIC	C_2BB4_019D
C_2BB4_019D	LABEL	BYTE	;2BB4:019D
	db	'Solo mode',10,0
PUBLIC	C_2BB4_01A8
C_2BB4_01A8	LABEL	BYTE	;2BB4:01A8
	db	'Party mode',10,0
PUBLIC	C_2BB4_01B4
C_2BB4_01B4	LABEL	BYTE	;2BB4:01B4
	db	'Direction-',10,0
PUBLIC	C_2BB4_01C0
C_2BB4_01C0	LABEL	BYTE	;2BB4:01C0
	db	'On - ',0
PUBLIC	C_2BB4_01C6
C_2BB4_01C6	LABEL	BYTE	;2BB4:01C6
	db	'Where? ',0
PUBLIC	C_2BB4_01CE
C_2BB4_01CE	LABEL	BYTE	;2BB4:01CE
	db	10,'Press any key...',0
PUBLIC	C_2BB4_01E0
C_2BB4_01E0	LABEL	BYTE	;2BB4:01E0
	db	'You can',39,'t sleep in a dream.',10,0
PUBLIC	C_2BB4_01FD
C_2BB4_01FD	LABEL	BYTE	;2BB4:01FD
	db	'It',39,'s already lit.',10,0
PUBLIC	C_2BB4_0210
C_2BB4_0210	LABEL	BYTE	;2BB4:0210
	db	'It won',39,'t go in that direction!',10,0
PUBLIC	C_2BB4_0230
C_2BB4_0230	LABEL	BYTE	;2BB4:0230
	db	'Blocked!',10,0
PUBLIC	C_2BB4_023A
C_2BB4_023A	LABEL	BYTE	;2BB4:023A
	db	'You can',39,'t read it.',10,0
PUBLIC	C_2BB4_024E
C_2BB4_024E	LABEL	BYTE	;2BB4:024E
	db	'You are carrying too much already.',10,0
PUBLIC	C_2BB4_0272
C_2BB4_0272	LABEL	BYTE	;2BB4:0272
	db	'Failed.',10,0
PUBLIC	C_2BB4_027B
C_2BB4_027B	LABEL	BYTE	;2BB4:027B
	db	'The %s is fully charged.',10,0
PUBLIC	C_2BB4_0295
C_2BB4_0295	LABEL	BYTE	;2BB4:0295
	db	'It is locked.',10,0
PUBLIC	C_2BB4_02A4
C_2BB4_02A4	LABEL	BYTE	;2BB4:02A4
	db	'It has no effect.',10,0
PUBLIC	C_2BB4_02B7
C_2BB4_02B7	LABEL	BYTE	;2BB4:02B7
	db	10,'There is no response.',10,0
PUBLIC	C_2BB4_02CF
C_2BB4_02CF	LABEL	BYTE	;2BB4:02CF
	db	'No.',10,0
PUBLIC	C_2BB4_02D4
C_2BB4_02D4	LABEL	BYTE	;2BB4:02D4
	db	'Not everyone is here.',10,0
PUBLIC	C_2BB4_02EB
C_2BB4_02EB	LABEL	BYTE	;2BB4:02EB
	db	'Not at night!',10,0
PUBLIC	C_2BB4_02FA
C_2BB4_02FA	LABEL	BYTE	;2BB4:02FA
	db	'Not while in combat mode!',10,0
PUBLIC	C_2BB4_0315
C_2BB4_0315	LABEL	BYTE	;2BB4:0315
	db	'Not while in solo mode!',10,0
PUBLIC	C_2BB4_032E
C_2BB4_032E	LABEL	BYTE	;2BB4:032E
	db	'Not now.',10,0
PUBLIC	C_2BB4_0338
C_2BB4_0338	LABEL	BYTE	;2BB4:0338
	db	'Not while dreaming!',10,0
PUBLIC	C_2BB4_034D
C_2BB4_034D	LABEL	BYTE	;2BB4:034D
	db	'That is not possible.',10,0
PUBLIC	C_2BB4_0364
C_2BB4_0364	LABEL	BYTE	;2BB4:0364
	db	'Not while underground.',10,0
PUBLIC	C_2BB4_037C
C_2BB4_037C	LABEL	BYTE	;2BB4:037C
	db	'That would be a waste outside of combat.',10,0
PUBLIC	C_2BB4_03A6
C_2BB4_03A6	LABEL	BYTE	;2BB4:03A6
	db	'Out of range!',10,0
PUBLIC	C_2BB4_03B5
C_2BB4_03B5	LABEL	BYTE	;2BB4:03B5
	db	'You must be in party mode to %s.',10,0
PUBLIC	C_2BB4_03D7
C_2BB4_03D7	LABEL	BYTE	;2BB4:03D7
	db	10,'The total is too heavy.',10,0
PUBLIC	C_2BB4_03F1
C_2BB4_03F1	LABEL	BYTE	;2BB4:03F1
	db	'what?',10,0
PUBLIC	C_2BB4_03F8
C_2BB4_03F8	LABEL	BYTE	;2BB4:03F8
	db	'nowhere.',10,0
PUBLIC	C_2BB4_0402
C_2BB4_0402	LABEL	BYTE	;2BB4:0402
	db	'nothing!',10,0
PUBLIC	C_2BB4_040C
C_2BB4_040C	LABEL	BYTE	;2BB4:040C
	db	'Nobody.',10,0
PUBLIC	C_2BB4_0415
C_2BB4_0415	LABEL	BYTE	;2BB4:0415
	db	10,'Nothing to light the %s with.',10,0
PUBLIC	C_2BB4_0435
C_2BB4_0435	LABEL	BYTE	;2BB4:0435
	db	10,'There is no fuel.',10,0
PUBLIC	C_2BB4_0449
C_2BB4_0449	LABEL	BYTE	;2BB4:0449
	db	10,'Your hands are full.',10,0
PUBLIC	C_2BB4_0460
C_2BB4_0460	LABEL	BYTE	;2BB4:0460
	db	'The %s is extinguished.',10,0
PUBLIC	C_2BB4_0479
C_2BB4_0479	LABEL	BYTE	;2BB4:0479
	db	10,'You incinerated the %s.',10,0
PUBLIC	C_2BB4_0493
C_2BB4_0493	LABEL	BYTE	;2BB4:0493
	db	10,'It',39,'s already full.',10,0
PUBLIC	C_2BB4_04A8
C_2BB4_04A8	LABEL	BYTE	;2BB4:04A8
	db	'The paper money flares into a great pyre!',10,0
PUBLIC	C_2BB4_04D3
C_2BB4_04D3	LABEL	BYTE	;2BB4:04D3
	db	10,'Fire!',10,0
PUBLIC	C_2BB4_04DB
C_2BB4_04DB	LABEL	BYTE	;2BB4:04DB
	db	'Splat! There is no effect.',10,0
PUBLIC	C_2BB4_04F7
C_2BB4_04F7	LABEL	BYTE	;2BB4:04F7
	db	'Kaboom! You blew the door down!',10,0
PUBLIC	C_2BB4_0518
C_2BB4_0518	LABEL	BYTE	;2BB4:0518
	db	'It already has steel cannon balls.',10,0
PUBLIC	C_2BB4_053C
C_2BB4_053C	LABEL	BYTE	;2BB4:053C
	db	'The cannon will fire steel cannon balls.',10,0
PUBLIC	C_2BB4_0566
C_2BB4_0566	LABEL	BYTE	;2BB4:0566
	db	'Press keys 1-0 to play -',0
PUBLIC	C_2BB4_057F
C_2BB4_057F	LABEL	BYTE	;2BB4:057F
	db	'Tocatta and Fugue in D minor (J.S. Bach).',10,0
PUBLIC	C_2BB4_05AA
C_2BB4_05AA	LABEL	BYTE	;2BB4:05AA
	db	'--Affidavit--',10,10,0
PUBLIC	C_2BB4_05BA
C_2BB4_05BA	LABEL	BYTE	;2BB4:05BA
	db	'The undersigned do hereby vouch for the ',0
PUBLIC	C_2BB4_05E3
C_2BB4_05E3	LABEL	BYTE	;2BB4:05E3
	db	'trust-',10,'worthiness of $P, who bears ',0
PUBLIC	C_2BB4_0607
C_2BB4_0607	LABEL	BYTE	;2BB4:0607
	db	'this chit, commending same to the members ',0
PUBLIC	C_2BB4_0632
C_2BB4_0632	LABEL	BYTE	;2BB4:0632
	db	'of the Olympus Mons outpost.',0
PUBLIC	C_2BB4_064F
C_2BB4_064F	LABEL	BYTE	;2BB4:064F
	db	10,10,'*Signed:',10,0
PUBLIC	C_2BB4_065B
C_2BB4_065B	LABEL	BYTE	;2BB4:065B
	db	' Richard Sherman',10,0
PUBLIC	C_2BB4_066D
C_2BB4_066D	LABEL	BYTE	;2BB4:066D
	db	' Capt. Gregory Duprey',10,0
PUBLIC	C_2BB4_0684
C_2BB4_0684	LABEL	BYTE	;2BB4:0684
	db	' Doctor David Yellin',10,0
PUBLIC	C_2BB4_069A
C_2BB4_069A	LABEL	BYTE	;2BB4:069A
	db	'There is nothing in the bucket.',10,0
PUBLIC	C_2BB4_06BB
C_2BB4_06BB	LABEL	BYTE	;2BB4:06BB
	db	'You pour the water onto the ground.',10,0
PUBLIC	C_2BB4_06E0
C_2BB4_06E0	LABEL	BYTE	;2BB4:06E0
	db	'You pour the water onto the seed.',10,0
PUBLIC	C_2BB4_0703
C_2BB4_0703	LABEL	BYTE	;2BB4:0703
	db	'%s is all wet.',10,0
PUBLIC	C_2BB4_0713
C_2BB4_0713	LABEL	BYTE	;2BB4:0713
	db	'The %s is all wet.',10,0
PUBLIC	C_2BB4_0727
C_2BB4_0727	LABEL	BYTE	;2BB4:0727
	db	'You pour the nitrogen compound onto the mound.',10,0
PUBLIC	C_2BB4_0757
C_2BB4_0757	LABEL	BYTE	;2BB4:0757
	db	'You pour the phosphorous solution onto the mound','.',10,0
PUBLIC	C_2BB4_078A
C_2BB4_078A	LABEL	BYTE	;2BB4:078A
	db	'You pour the potash mixture onto the mound.',10,0
PUBLIC	C_2BB4_07B7
C_2BB4_07B7	LABEL	BYTE	;2BB4:07B7
	db	'The seed is destroyed by acid.',10,0
PUBLIC	C_2BB4_07D7
C_2BB4_07D7	LABEL	BYTE	;2BB4:07D7
	db	'The seed is poisoned by oil.',10,0
PUBLIC	C_2BB4_07F5
C_2BB4_07F5	LABEL	BYTE	;2BB4:07F5
	db	'There is nothing to drill into.',10,0
PUBLIC	C_2BB4_0816
C_2BB4_0816	LABEL	BYTE	;2BB4:0816
	db	'There is no room left for the ore.',10,0
PUBLIC	C_2BB4_083A
C_2BB4_083A	LABEL	BYTE	;2BB4:083A
	db	'The drill',39,'s power is connected.',10,0
PUBLIC	C_2BB4_085B
C_2BB4_085B	LABEL	BYTE	;2BB4:085B
	db	'You eat a Martian berry. You feel %s strange ',0
PUBLIC	C_2BB4_0889
C_2BB4_0889	LABEL	BYTE	;2BB4:0889
	db	'relationship with things around you.',10,0
PUBLIC	C_2BB4_08AF
C_2BB4_08AF	LABEL	BYTE	;2BB4:08AF
	db	'sensitivity to the feelings of objects around yo','u.',10,0
PUBLIC	C_2BB4_08E3
C_2BB4_08E3	LABEL	BYTE	;2BB4:08E3
	db	'sense of your spatial location.',10,0
PUBLIC	C_2BB4_0904
C_2BB4_0904	LABEL	BYTE	;2BB4:0904
	db	'feelings, like the berries were really odd.',10,0
PUBLIC	C_2BB4_0931
C_2BB4_0931	LABEL	BYTE	;2BB4:0931
	db	'%s enters a battle frenzy!',10,0
PUBLIC	C_2BB4_094D
C_2BB4_094D	LABEL	BYTE	;2BB4:094D
	db	'This switch cannot be fixed.',10,0
PUBLIC	C_2BB4_096B
C_2BB4_096B	LABEL	BYTE	;2BB4:096B
	db	'The switch is fastened.',10,0
PUBLIC	C_2BB4_0984
C_2BB4_0984	LABEL	BYTE	;2BB4:0984
	db	'The switch is loose.',10,0
PUBLIC	C_2BB4_099A
C_2BB4_099A	LABEL	BYTE	;2BB4:099A
	db	'It must be repaired first.',10,0
PUBLIC	C_2BB4_09B6
C_2BB4_09B6	LABEL	BYTE	;2BB4:09B6
	db	'The panel is fastened.',10,0
PUBLIC	C_2BB4_09CE
C_2BB4_09CE	LABEL	BYTE	;2BB4:09CE
	db	'The panel is loose.',10,0
PUBLIC	C_2BB4_09E3
C_2BB4_09E3	LABEL	BYTE	;2BB4:09E3
	db	'The panel is broken.',10,0
PUBLIC	C_2BB4_09F9
C_2BB4_09F9	LABEL	BYTE	;2BB4:09F9
	db	'The panel is not installed.',10,0
PUBLIC	C_2BB4_0A16
C_2BB4_0A16	LABEL	BYTE	;2BB4:0A16
	db	'Panels are only installed onto cabinets.',10,0
PUBLIC	C_2BB4_0A40
C_2BB4_0A40	LABEL	BYTE	;2BB4:0A40
	db	'This cabinet requires a different type of panel.',10,0
PUBLIC	C_2BB4_0A72
C_2BB4_0A72	LABEL	BYTE	;2BB4:0A72
	db	'There is nothing to unload.',10,0
PUBLIC	C_2BB4_0A8F
C_2BB4_0A8F	LABEL	BYTE	;2BB4:0A8F
	db	'There is no more room.',10,0
PUBLIC	C_2BB4_0AA7
C_2BB4_0AA7	LABEL	BYTE	;2BB4:0AA7
	db	'You put the %s into the wheelbarrow.',10,0
PUBLIC	C_2BB4_0ACD
C_2BB4_0ACD	LABEL	BYTE	;2BB4:0ACD
	db	'You can',39,'t mix different things in the same load!',10,0
PUBLIC	C_2BB4_0AFF
C_2BB4_0AFF	LABEL	BYTE	;2BB4:0AFF
	db	'You put the %s into the rail car.',10,0
PUBLIC	C_2BB4_0B22
C_2BB4_0B22	LABEL	BYTE	;2BB4:0B22
	db	'You can',39,'t bury anything here.',10,0
PUBLIC	C_2BB4_0B41
C_2BB4_0B41	LABEL	BYTE	;2BB4:0B41
	db	'You planted the Martian seed.',10,0
PUBLIC	C_2BB4_0B60
C_2BB4_0B60	LABEL	BYTE	;2BB4:0B60
	db	'You filled in the hole.',10,0
PUBLIC	C_2BB4_0B79
C_2BB4_0B79	LABEL	BYTE	;2BB4:0B79
	db	'You can',39,'t dig here.',10,0
PUBLIC	C_2BB4_0B8E
C_2BB4_0B8E	LABEL	BYTE	;2BB4:0B8E
	db	'You uprooted the pod plant.',10,0
PUBLIC	C_2BB4_0BAB
C_2BB4_0BAB	LABEL	BYTE	;2BB4:0BAB
	db	'An unconscious Martian falls out of the pod.',10,0
PUBLIC	C_2BB4_0BD9
C_2BB4_0BD9	LABEL	BYTE	;2BB4:0BD9
	db	'You pried the nails out of the crate.',10,0
PUBLIC	C_2BB4_0C00
C_2BB4_0C00	LABEL	BYTE	;2BB4:0C00
	db	'It is not fully grown.',10,0
PUBLIC	C_2BB4_0C18
C_2BB4_0C18	LABEL	BYTE	;2BB4:0C18
	db	'It is fastened securely.',10,0
PUBLIC	C_2BB4_0C32
C_2BB4_0C32	LABEL	BYTE	;2BB4:0C32
	db	'It turns loosely.',10,0
PUBLIC	C_2BB4_0C45
C_2BB4_0C45	LABEL	BYTE	;2BB4:0C45
	db	'  New setting (R%/S%/C)?',10,0
PUBLIC	C_2BB4_0C5F
C_2BB4_0C5F	LABEL	BYTE	;2BB4:0C5F
	db	'the lever seems stuck.',10,0
PUBLIC	C_2BB4_0C77
C_2BB4_0C77	LABEL	BYTE	;2BB4:0C77
	db	'Push lever to ',0
PUBLIC	C_2BB4_0C86
C_2BB4_0C86	LABEL	BYTE	;2BB4:0C86
	db	'You are somewhere near ',10,'Lat:%d %c',10,'Long:%d %c',10,0
PUBLIC	C_2BB4_0CB4
C_2BB4_0CB4	LABEL	BYTE	;2BB4:0CB4
	db	'You need a cue stick in your hands.',10,0
PUBLIC	C_2BB4_0CD9
C_2BB4_0CD9	LABEL	BYTE	;2BB4:0CD9
	db	'Organ music must be played on a pipe organ.',10,0
PUBLIC	C_2BB4_0D06
C_2BB4_0D06	LABEL	BYTE	;2BB4:0D06
	db	'Which section of rail needs fixing -',10,0
PUBLIC	C_2BB4_0D2C
C_2BB4_0D2C	LABEL	BYTE	;2BB4:0D2C
	db	'Radium has already been installed.',10,0
PUBLIC	C_2BB4_0D50
C_2BB4_0D50	LABEL	BYTE	;2BB4:0D50
	db	'OUCH. It is very hot.',10,0
PUBLIC	C_2BB4_0D67
C_2BB4_0D67	LABEL	BYTE	;2BB4:0D67
	db	'The %s only needed %d radium blocks.',10,0
PUBLIC	C_2BB4_0D8D
C_2BB4_0D8D	LABEL	BYTE	;2BB4:0D8D
	db	'The tongs will now protect your hands.',10,0
PUBLIC	C_2BB4_0DB5
C_2BB4_0DB5	LABEL	BYTE	;2BB4:0DB5
	db	'The radium has been installed.',10,0
PUBLIC	C_2BB4_0DD5
C_2BB4_0DD5	LABEL	BYTE	;2BB4:0DD5
	db	'The headgear is installed.',10,0
PUBLIC	C_2BB4_0DF1
C_2BB4_0DF1	LABEL	BYTE	;2BB4:0DF1
	db	'The machine does not work.',10,0
PUBLIC	C_2BB4_0E0D
C_2BB4_0E0D	LABEL	BYTE	;2BB4:0E0D
	db	'There is nobody sitting in the machine.',10,0
PUBLIC	C_2BB4_0E36
C_2BB4_0E36	LABEL	BYTE	;2BB4:0E36
	db	'The dream machines seem to have ceased functioni','ng.',10,0
PUBLIC	C_2BB4_0E6B
C_2BB4_0E6B	LABEL	BYTE	;2BB4:0E6B
	db	'The sprayer system now has rubber.',10,0
PUBLIC	C_2BB4_0E8F
C_2BB4_0E8F	LABEL	BYTE	;2BB4:0E8F
	db	'The sprayer system now has dye.',10,0
PUBLIC	C_2BB4_0EB0
C_2BB4_0EB0	LABEL	BYTE	;2BB4:0EB0
	db	'Nobody is standing under the sprayer.',10,0
PUBLIC	C_2BB4_0ED7
C_2BB4_0ED7	LABEL	BYTE	;2BB4:0ED7
	db	'You don',39,'t think it would be safe to coat an animal or pl'
	db	'ant with rubber.',10,0
PUBLIC	C_2BB4_0F21
C_2BB4_0F21	LABEL	BYTE	;2BB4:0F21
	db	'Chsheket has already been treated.',10,0
PUBLIC	C_2BB4_0F45
C_2BB4_0F45	LABEL	BYTE	;2BB4:0F45
	db	'There is no rubber in the sprayer system.',10,0
PUBLIC	C_2BB4_0F70
C_2BB4_0F70	LABEL	BYTE	;2BB4:0F70
	db	'The rubber in the sprayer system is not flesh-co','lored.',10,0
PUBLIC	C_2BB4_0FA8
C_2BB4_0FA8	LABEL	BYTE	;2BB4:0FA8
	db	'It is also not flesh-colored.',10,0
PUBLIC	C_2BB4_0FC7
C_2BB4_0FC7	LABEL	BYTE	;2BB4:0FC7
	db	'You killed the plant.',10,0
PUBLIC	C_2BB4_0FDE
C_2BB4_0FDE	LABEL	BYTE	;2BB4:0FDE
	db	'`%s becomes visible!',10,0
PUBLIC	C_2BB4_0FF4
C_2BB4_0FF4	LABEL	BYTE	;2BB4:0FF4
	db	'Ooops. These rollers can never be fixed!',10,0
PUBLIC	C_2BB4_101E
C_2BB4_101E	LABEL	BYTE	;2BB4:101E
	db	'You get two handfuls of oxium from the bin.',10,0
PUBLIC	C_2BB4_104B
C_2BB4_104B	LABEL	BYTE	;2BB4:104B
	db	'This door is not rusted.',10,0
PUBLIC	C_2BB4_1065
C_2BB4_1065	LABEL	BYTE	;2BB4:1065
	db	'The door is unstuck.',10,0
PUBLIC	C_2BB4_107B
C_2BB4_107B	LABEL	BYTE	;2BB4:107B
	db	'You imagine ',0
PUBLIC	C_2BB4_1088
C_2BB4_1088	LABEL	BYTE	;2BB4:1088
	db	'You pull some of the dreamstuff from the hoard.',10,0
PUBLIC	C_2BB4_10B9
C_2BB4_10B9	LABEL	BYTE	;2BB4:10B9
	db	'You lack the mental power to affect the dreamstu','ff hoard.'
	db	10,0
PUBLIC	C_2BB4_10F4
C_2BB4_10F4	LABEL	BYTE	;2BB4:10F4
	db	'Some of the lenses are not responding.',10,0
PUBLIC	C_2BB4_111C
C_2BB4_111C	LABEL	BYTE	;2BB4:111C
	db	'There is no water pressure in the valve system.',10,0
PUBLIC	C_2BB4_114D
C_2BB4_114D	LABEL	BYTE	;2BB4:114D
	db	'No backtalk from you, young %sman!',10,0
PUBLIC	C_2BB4_1171
C_2BB4_1171	LABEL	BYTE	;2BB4:1171
	db	'`(using %s)',0
PUBLIC	C_2BB4_117D
C_2BB4_117D	LABEL	BYTE	;2BB4:117D
	db	'`You fill the bucket with water.',10,0
PUBLIC	C_2BB4_119F
C_2BB4_119F	LABEL	BYTE	;2BB4:119F
	db	'`Dump out the ice? ',0
PUBLIC	C_2BB4_11B3
C_2BB4_11B3	LABEL	BYTE	;2BB4:11B3
	db	'`The cave worm eats the pod plant!',10,0
PUBLIC	C_2BB4_11D7
C_2BB4_11D7	LABEL	BYTE	;2BB4:11D7
	db	'`Don',39,'t bury people on Mars!',10,0
PUBLIC	C_2BB4_11F4
C_2BB4_11F4	LABEL	BYTE	;2BB4:11F4
	db	'`It is automatic and can',39,'t be opened by hand.',10,0
PUBLIC	C_2BB4_1223
C_2BB4_1223	LABEL	BYTE	;2BB4:1223
	db	'It is apparently rusted shut.',10,0
PUBLIC	C_2BB4_1242
C_2BB4_1242	LABEL	BYTE	;2BB4:1242
	db	'It is apparently locked.',10,0
PUBLIC	C_2BB4_125C
C_2BB4_125C	LABEL	BYTE	;2BB4:125C
	db	'It has no power.',10,0
PUBLIC	C_2BB4_126E
C_2BB4_126E	LABEL	BYTE	;2BB4:126E
	db	'The work is too precise to perform telekinetical','ly.',10,0
PUBLIC	C_2BB4_12A3
C_2BB4_12A3	LABEL	BYTE	;2BB4:12A3
	db	'You can',39,'t pile any more dirt there.',10,0
PUBLIC	C_2BB4_12C8
C_2BB4_12C8	LABEL	BYTE	;2BB4:12C8
	db	'The canals are already working.',10,0
PUBLIC	C_2BB4_12E9
C_2BB4_12E9	LABEL	BYTE	;2BB4:12E9
	db	'The ruby lenses are already aligned.',10,0
PUBLIC	C_2BB4_130F
C_2BB4_130F	LABEL	BYTE	;2BB4:130F
	db	'Good shot, old bean!',10,0
PUBLIC	C_2BB4_1325
C_2BB4_1325	LABEL	BYTE	;2BB4:1325
	db	'Wheeeeze',10,0
PUBLIC	C_2BB4_132F
C_2BB4_132F	LABEL	BYTE	;2BB4:132F
	db	'Ching!',10,0
PUBLIC	C_2BB4_1337
C_2BB4_1337	LABEL	BYTE	;2BB4:1337
	db	'Thump! Thump!',10,0
PUBLIC	C_2BB4_1346
C_2BB4_1346	LABEL	BYTE	;2BB4:1346
	db	'Shing!',10,0
PUBLIC	C_2BB4_134E
C_2BB4_134E	LABEL	BYTE	;2BB4:134E
	db	'You hear something to the %s!',10,0
PUBLIC	C_2BB4_136D
C_2BB4_136D	LABEL	BYTE	;2BB4:136D
	db	'`%s is poisoned!',10,0
PUBLIC	C_2BB4_137F
C_2BB4_137F	LABEL	BYTE	;2BB4:137F
	db	'A torch burns out.',10,0
PUBLIC	C_2BB4_1393
C_2BB4_1393	LABEL	BYTE	;2BB4:1393
	db	'`%s attacks!',10,0
PUBLIC	C_2BB4_13A1
C_2BB4_13A1	LABEL	BYTE	;2BB4:13A1
	db	10,'%s is healed.',10,0
PUBLIC	C_2BB4_13B1
C_2BB4_13B1	LABEL	BYTE	;2BB4:13B1
	db	'`%s paralyzed!',10,0
PUBLIC	C_2BB4_13C1
C_2BB4_13C1	LABEL	BYTE	;2BB4:13C1
	db	'`%s fails to respond.',10,0
PUBLIC	C_2BB4_13D8
C_2BB4_13D8	LABEL	BYTE	;2BB4:13D8
	db	'It appears to control a dream machine.',10,0
PUBLIC	C_2BB4_1400
C_2BB4_1400	LABEL	BYTE	;2BB4:1400
	db	'It appears to control a spray system.',10,0
PUBLIC	C_2BB4_1427
C_2BB4_1427	LABEL	BYTE	;2BB4:1427
	db	'It appears to control the ruby lens system.',10,0
PUBLIC	C_2BB4_1454
C_2BB4_1454	LABEL	BYTE	;2BB4:1454
	db	'It appears to control the cistern valves.',10,0
PUBLIC	C_2BB4_147F
C_2BB4_147F	LABEL	BYTE	;2BB4:147F
	db	'These appear to be mechanical controls.',10,0
PUBLIC	C_2BB4_14A8
C_2BB4_14A8	LABEL	BYTE	;2BB4:14A8
	db	'These appear to be valve controls.',10,0
PUBLIC	C_2BB4_14CC
C_2BB4_14CC	LABEL	BYTE	;2BB4:14CC
	db	'These appear to be electrical controls.',10,0
PUBLIC	C_2BB4_14F5
C_2BB4_14F5	LABEL	BYTE	;2BB4:14F5
	db	'You cannot decipher its purpose.',10,0
PUBLIC	C_2BB4_1517
C_2BB4_1517	LABEL	BYTE	;2BB4:1517
	db	'It appears to control this dream machine.',10,0
PUBLIC	C_2BB4_1542
C_2BB4_1542	LABEL	BYTE	;2BB4:1542
	db	'It appears to control this spray system.',10,0
PUBLIC	C_2BB4_156C
C_2BB4_156C	LABEL	BYTE	;2BB4:156C
	db	'It appears to control the ruby lens system.',10,0
PUBLIC	C_2BB4_1599
C_2BB4_1599	LABEL	BYTE	;2BB4:1599
	db	'It appears to control the cistern valves.',10,0
PUBLIC	C_2BB4_15C4
C_2BB4_15C4	LABEL	BYTE	;2BB4:15C4
	db	'It appears to control a mechanical system.',10,0
PUBLIC	C_2BB4_15F0
C_2BB4_15F0	LABEL	BYTE	;2BB4:15F0
	db	'It appears to control a valve system.',10,0
PUBLIC	C_2BB4_1617
C_2BB4_1617	LABEL	BYTE	;2BB4:1617
	db	'It appears to control an electrical system.',10,0
PUBLIC	C_2BB4_1644
C_2BB4_1644	LABEL	BYTE	;2BB4:1644
	db	'You cannot decipher what it controls.',10,0
PUBLIC	C_2BB4_166B
C_2BB4_166B	LABEL	BYTE	;2BB4:166B
	db	'You actuate the mechanism to no apparent effect.',10,0
PUBLIC	C_2BB4_169D
C_2BB4_169D	LABEL	BYTE	;2BB4:169D
	db	'Status lights change but you see no other effect','.',10,0
PUBLIC	C_2BB4_16D0
C_2BB4_16D0	LABEL	BYTE	;2BB4:16D0
	db	'Lights flash and change color but nothing else h','appens.',10
	db	0
PUBLIC	C_2BB4_1709
C_2BB4_1709	LABEL	BYTE	;2BB4:1709
	db	'You are completely unsure what you just did.',10,0
PUBLIC	C_2BB4_1737
C_2BB4_1737	LABEL	BYTE	;2BB4:1737
	db	'Some ice has melted.',10,0
PUBLIC	C_2BB4_174D
C_2BB4_174D	LABEL	BYTE	;2BB4:174D
	db	'You have killed someone whose help you need to w'
	db	'in the game!',10,0
PUBLIC	C_2BB4_178B
C_2BB4_178B	LABEL	BYTE	;2BB4:178B
	db	'One of the important historical figures you were'
	db	' here to rescue is dead!',10,0
PUBLIC	C_2BB4_17D5
C_2BB4_17D5	LABEL	BYTE	;2BB4:17D5
	db	'The ground is not flat enough.',10,0
PUBLIC	C_2BB4_17F5
C_2BB4_17F5	LABEL	BYTE	;2BB4:17F5
	db	'The %s is in the way.',10,0
PUBLIC	C_2BB4_180C
C_2BB4_180C	LABEL	BYTE	;2BB4:180C
	db	'The drill must be installed onto a drill cart.',10,0
PUBLIC	C_2BB4_183C
C_2BB4_183C	LABEL	BYTE	;2BB4:183C
	db	'%s is not strong enough.',10,0
PUBLIC	C_2BB4_1856
C_2BB4_1856	LABEL	BYTE	;2BB4:1856
	db	'The robot already has a heartstone.',10,0
PUBLIC	C_2BB4_187B
C_2BB4_187B	LABEL	BYTE	;2BB4:187B
	db	'The heartstone is installed.',10,0
PUBLIC	C_2BB4_1899
C_2BB4_1899	LABEL	BYTE	;2BB4:1899
	db	'The pod is already open.',10,0
PUBLIC	C_2BB4_18B3
C_2BB4_18B3	LABEL	BYTE	;2BB4:18B3
	db	'You can',39,'t dig any deeper.',10,0
PUBLIC	C_2BB4_18CE
C_2BB4_18CE	LABEL	BYTE	;2BB4:18CE
	db	'It is stuck.',10,0
PUBLIC	C_2BB4_18DC
C_2BB4_18DC	LABEL	BYTE	;2BB4:18DC
	db	'The control panel operates the dream machine.',10,0
PUBLIC	C_2BB4_190B
C_2BB4_190B	LABEL	BYTE	;2BB4:190B
	db	'You need to use pliers to attach the cable.',10,0
PUBLIC	C_2BB4_1938
C_2BB4_1938	LABEL	BYTE	;2BB4:1938
	db	'You will chew it instinctively.',10,0
PUBLIC	C_2BB4_1959
C_2BB4_1959	LABEL	BYTE	;2BB4:1959
	db	'The ground is not clear for digging.',10,0
PUBLIC	C_2BB4_197F
C_2BB4_197F	LABEL	BYTE	;2BB4:197F
	db	'You must bury the %s in a hole.',10,0
PUBLIC	C_2BB4_19A0
C_2BB4_19A0	LABEL	BYTE	;2BB4:19A0
	db	'It is covered with dirt.',10,0
PUBLIC	C_2BB4_19BA
C_2BB4_19BA	LABEL	BYTE	;2BB4:19BA
	db	'They have been covered with dirt.',10,0
PUBLIC	C_2BB4_19DD
C_2BB4_19DD	LABEL	BYTE	;2BB4:19DD
	db	'Funny, no sprayer near sprayer controls!',10,0
PUBLIC	C_2BB4_1A07
C_2BB4_1A07	LABEL	BYTE	;2BB4:1A07
	db	'Alignment coordinate:',10,0
PUBLIC	C_2BB4_1A1E
C_2BB4_1A1E	LABEL	BYTE	;2BB4:1A1E
	db	'You wait until dawn...*',10,0
PUBLIC	C_2BB4_1A37
C_2BB4_1A37	LABEL	BYTE	;2BB4:1A37
	db	'No seed to harvest yet.',10,0
PUBLIC	C_2BB4_1A50
C_2BB4_1A50	LABEL	BYTE	;2BB4:1A50
	db	'Location: ',0
PUBLIC	C_2BB4_1A5B
C_2BB4_1A5B	LABEL	BYTE	;2BB4:1A5B
	db	'Won',39,'t grow there!',10,0
PUBLIC	C_2BB4_1A6E
C_2BB4_1A6E	LABEL	BYTE	;2BB4:1A6E
	db	'Now it just needs watering.',10,0
PUBLIC	C_2BB4_1A8B
C_2BB4_1A8B	LABEL	BYTE	;2BB4:1A8B
	db	'Your mouth is too dry.',10,0
PUBLIC	C_2BB4_1AA3
C_2BB4_1AA3	LABEL	BYTE	;2BB4:1AA3
	db	'You spit into the spittoon.',10,0
PUBLIC	C_2BB4_1AC0
C_2BB4_1AC0	LABEL	BYTE	;2BB4:1AC0
	db	'Click.',10,0
PUBLIC	C_2BB4_1AC8
C_2BB4_1AC8	LABEL	BYTE	;2BB4:1AC8
	db	'You may use the ruby slippers to go home. If you'
	db	' choose to, you may now view the Grand Finale.',10,10,'*',0
PUBLIC	C_2BB4_1B2A
C_2BB4_1B2A	LABEL	BYTE	;2BB4:1B2A
	db	'Afterwards, you will exit to DOS. You will be ab'
	db	'le to resume play from your last saved game.',10,10,'*',0
PUBLIC	C_2BB4_1B8A
C_2BB4_1B8A	LABEL	BYTE	;2BB4:1B8A
	db	'Would you like to see the Grand Finale now? If s'
	db	'o, press Y. To resume play now, hit N: (Y N)',10,0
PUBLIC	C_2BB4_1BE8
C_2BB4_1BE8	LABEL	BYTE	;2BB4:1BE8
	db	'The %s is broken.',10,0
PUBLIC	C_2BB4_1BFB
C_2BB4_1BFB	LABEL	BYTE	;2BB4:1BFB
	db	'You cannot install it there.',10,0
PUBLIC	C_2BB4_1C19
C_2BB4_1C19	LABEL	BYTE	;2BB4:1C19
	db	'%s can',39,'t be used.',10,0
PUBLIC	C_2BB4_1C2C
C_2BB4_1C2C	LABEL	BYTE	;2BB4:1C2C
	db	'%s has no effect on ',0
PUBLIC	C_2BB4_1C41
C_2BB4_1C41	LABEL	BYTE	;2BB4:1C41
	db	'%s has no effect.',10,0
PUBLIC	C_2BB4_1C54
C_2BB4_1C54	LABEL	BYTE	;2BB4:1C54
	db	'It is nailed shut.',10,0
PUBLIC	C_2BB4_1C68
C_2BB4_1C68	LABEL	BYTE	;2BB4:1C68
	db	'              ',10,0
PUBLIC	C_2BB4_1C78
C_2BB4_1C78	LABEL	BYTE	;2BB4:1C78
	db	10,'Searching here, you find nothing.',10,0
PUBLIC	C_2BB4_1C9C
C_2BB4_1C9C	LABEL	BYTE	;2BB4:1C9C
	db	10,'Searching here, you find ',0
PUBLIC	C_2BB4_1CB7
C_2BB4_1CB7	LABEL	BYTE	;2BB4:1CB7
	db	'You see ',0
PUBLIC	C_2BB4_1CC0
C_2BB4_1CC0	LABEL	BYTE	;2BB4:1CC0
	db	'darkness.',10,0
PUBLIC	C_2BB4_1CCB
C_2BB4_1CCB	LABEL	BYTE	;2BB4:1CCB
	db	'Yourself',10,0
PUBLIC	C_2BB4_1CD5
C_2BB4_1CD5	LABEL	BYTE	;2BB4:1CD5
	db	'It is growing.',10,0
PUBLIC	C_2BB4_1CE5
C_2BB4_1CE5	LABEL	BYTE	;2BB4:1CE5
	db	'It is ripe.',10,0
PUBLIC	C_2BB4_1CF2
C_2BB4_1CF2	LABEL	BYTE	;2BB4:1CF2
	db	'It is not growing.',10,0
PUBLIC	C_2BB4_1D06
C_2BB4_1D06	LABEL	BYTE	;2BB4:1D06
	db	'It has a heartstone.',10,0
PUBLIC	C_2BB4_1D1C
C_2BB4_1D1C	LABEL	BYTE	;2BB4:1D1C
	db	'It is empty.',10,0
PUBLIC	C_2BB4_1D2A
C_2BB4_1D2A	LABEL	BYTE	;2BB4:1D2A
	db	'It has %d load\\s of ',0
PUBLIC	C_2BB4_1D40
C_2BB4_1D40	LABEL	BYTE	;2BB4:1D40
	db	10,10,'The time is ',0
PUBLIC	C_2BB4_1D4F
C_2BB4_1D4F	LABEL	BYTE	;2BB4:1D4F
	db	'It is set to ',39,'%s',39,0
PUBLIC	C_2BB4_1D61
C_2BB4_1D61	LABEL	BYTE	;2BB4:1D61
	db	', and has %d charge\\s left.',10,0
PUBLIC	C_2BB4_1D7F
C_2BB4_1D7F	LABEL	BYTE	;2BB4:1D7F
	db	'It appears to be loose.',10,0
PUBLIC	C_2BB4_1D98
C_2BB4_1D98	LABEL	BYTE	;2BB4:1D98
	db	'It appears to be broken.',10,0
PUBLIC	C_2BB4_1DB2
C_2BB4_1DB2	LABEL	BYTE	;2BB4:1DB2
	db	'It',39,'s loaded with %s',0
PUBLIC	C_2BB4_1DC6
C_2BB4_1DC6	LABEL	BYTE	;2BB4:1DC6
	db	'It is ready to use.',10,0
PUBLIC	C_2BB4_1DDB
C_2BB4_1DDB	LABEL	BYTE	;2BB4:1DDB
	db	'The %s cannot be dropped on the ground.',10,0
PUBLIC	C_2BB4_1E04
C_2BB4_1E04	LABEL	BYTE	;2BB4:1E04
	db	10,'Searching the body, you find ',0
PUBLIC	C_2BB4_1E23
C_2BB4_1E23	LABEL	BYTE	;2BB4:1E23
	db	'Won',39,'t budge !',10,0
PUBLIC	C_2BB4_1E32
C_2BB4_1E32	LABEL	BYTE	;2BB4:1E32
	db	'The ice is melting.',10,0
PUBLIC	C_2BB4_1E47
C_2BB4_1E47	LABEL	BYTE	;2BB4:1E47
	db	'The %s cannot be moved onto the ground.',10,0
PUBLIC	C_2BB4_1E70
C_2BB4_1E70	LABEL	BYTE	;2BB4:1E70
	db	10,'That person isn',39,'t here!',10,0
PUBLIC	C_2BB4_1E8A
C_2BB4_1E8A	LABEL	BYTE	;2BB4:1E8A
	db	'%s is carrying too much already.',10,0
PUBLIC	C_2BB4_1EAC
C_2BB4_1EAC	LABEL	BYTE	;2BB4:1EAC
	db	10,10,'You have to unready it first.',10,0
PUBLIC	C_2BB4_1ECD
C_2BB4_1ECD	LABEL	BYTE	;2BB4:1ECD
	db	10,10,'An item can',39,'t be placed inside itself!',10,0
PUBLIC	C_2BB4_1EF7
C_2BB4_1EF7	LABEL	BYTE	;2BB4:1EF7
	db	10,10,'You can',39,'t do that!',10,0
PUBLIC	C_2BB4_1F0D
C_2BB4_1F0D	LABEL	BYTE	;2BB4:1F0D
	db	'That container is full.',10,0
PUBLIC	C_2BB4_1F26
C_2BB4_1F26	LABEL	BYTE	;2BB4:1F26
	db	'A mechanical person can',39,'t eat berries!',10,0
PUBLIC	C_2BB4_1F4E
C_2BB4_1F4E	LABEL	BYTE	;2BB4:1F4E
	db	'The plate in the camera has already been exposed','.',10,0
PUBLIC	C_2BB4_1F81
C_2BB4_1F81	LABEL	BYTE	;2BB4:1F81
	db	'You must use a shovel to move that.',10,0
PUBLIC	C_2BB4_1FA6
C_2BB4_1FA6	LABEL	BYTE	;2BB4:1FA6
	db	'There is already something in the bucket.',10,0
PUBLIC	C_2BB4_1FD1
C_2BB4_1FD1	LABEL	BYTE	;2BB4:1FD1
	db	'You can',39,'t determine how to read the time.',10,0
PUBLIC	C_2BB4_1FFC
C_2BB4_1FFC	LABEL	BYTE	;2BB4:1FFC
	db	'You nail the crate shut.',10,0
PUBLIC	C_2BB4_2016
C_2BB4_2016	LABEL	BYTE	;2BB4:2016
	db	'The spray gun has 10 more charges.',10,0
PUBLIC	C_2BB4_203A
C_2BB4_203A	LABEL	BYTE	;2BB4:203A
	db	'The spray gun now contains 20 charges of green p','aint.',10,0
PUBLIC	C_2BB4_2071
C_2BB4_2071	LABEL	BYTE	;2BB4:2071
	db	'The spray gun now contains 10 charges of weed ki','ller.',10,0
PUBLIC	C_2BB4_20A8
C_2BB4_20A8	LABEL	BYTE	;2BB4:20A8
	db	'There is nothing in the gun.',10,0
PUBLIC	C_2BB4_20C6
C_2BB4_20C6	LABEL	BYTE	;2BB4:20C6
	db	'The ice thaws.',10,0
PUBLIC	C_2BB4_20D6
C_2BB4_20D6	LABEL	BYTE	;2BB4:20D6
	db	'The water freezes.',10,0
PUBLIC	C_2BB4_20EA
C_2BB4_20EA	LABEL	BYTE	;2BB4:20EA
	db	'You can',39,'t bury something using itself!',10,0
PUBLIC	C_2BB4_2112
C_2BB4_2112	LABEL	BYTE	;2BB4:2112
	db	'The cable does not need replacement.',10,0
PUBLIC	C_2BB4_2138
C_2BB4_2138	LABEL	BYTE	;2BB4:2138
	db	'sudden flush, dizziness, and nausea.',10,0
PUBLIC	C_2BB4_215E
C_2BB4_215E	LABEL	BYTE	;2BB4:215E
	db	'No EMS found; no music possible.',10,0
PUBLIC	C_2BB4_2180
C_2BB4_2180	LABEL	BYTE	;2BB4:2180
	db	'Stoker',39,'s path is blocked!',10,0
PUBLIC	C_2BB4_219B
C_2BB4_219B	LABEL	BYTE	;2BB4:219B
	db	'The alien device doesn',39,'t seem to be functioning.',10,0
PUBLIC	C_2BB4_21CD
C_2BB4_21CD	LABEL	BYTE	;2BB4:21CD
	db	'Nothing appears to happen.',10,0
PUBLIC	C_2BB4_21E9
C_2BB4_21E9	LABEL	BYTE	;2BB4:21E9
	db	'A psychic power fades...',10,0
PUBLIC	C_2BB4_2203
C_2BB4_2203	LABEL	BYTE	;2BB4:2203
	db	'The coal begins moving down the belt.',10,0
PUBLIC	C_2BB4_222A
C_2BB4_222A	LABEL	BYTE	;2BB4:222A
	db	'The conveyor belt stops.',10,0
PUBLIC	C_2BB4_2244
C_2BB4_2244	LABEL	BYTE	;2BB4:2244
	db	'The berries are not edible.',10,0
PUBLIC	C_2BB4_2261
C_2BB4_2261	LABEL	BYTE	;2BB4:2261
	db	'There are too many moving trees nearby.',10,0
PUBLIC	C_2BB4_228A
C_2BB4_228A	LABEL	BYTE	;2BB4:228A
	db	'The %s doesn',39,'t need oxygenated air.',10,0
PUBLIC	C_2BB4_22AF
C_2BB4_22AF	LABEL	BYTE	;2BB4:22AF
	db	'%s is beyond help.',10,0
PUBLIC	C_2BB4_22C3
C_2BB4_22C3	LABEL	BYTE	;2BB4:22C3
	db	'%s is revived!',10,0
PUBLIC	C_2BB4_22D3
C_2BB4_22D3	LABEL	BYTE	;2BB4:22D3
	db	'You don',39,'t know how it works.',10,0
PUBLIC	C_2BB4_22F1
C_2BB4_22F1	LABEL	BYTE	;2BB4:22F1
	db	'Aargh!',10,0
PUBLIC	C_2BB4_22F9
C_2BB4_22F9	LABEL	BYTE	;2BB4:22F9
	db	'Version 1.4',10,0

MSGS_DATA	ENDS

END
