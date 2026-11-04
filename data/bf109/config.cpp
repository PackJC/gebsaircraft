/*

	gebsaircraft - the Messerschmitt Bf 109 E-3

	A bare Transport like every GebsPlane (scripts/4_world/entities/vehicles/
	gebsplane.c simulates it all: wheels, engine, fuel and the air). Everything
	about how it flies and rolls is in its GebsFlight class; the positions of
	its wings, tail, propeller, wheels and canopy are memory points in its model,
	which tools/build_bf109.py builds from the CadNav model.

	Bf 109 E-3, the numbers it is built on:
		span 9.87 m, wing area 16.35 m2, length 8.64 m
		Daimler-Benz DB 601 A: 1,175 PS (864 kW) for takeoff at 2,480 rpm, its
		supercharger holding full power to about 4,000 m; a VDM constant-speed
		propeller of 3.1 m, geared 1:1.55
		empty 2,010 kg, loaded about 2,500 kg; one 400 L fuel tank behind the seat
		top speed 467 km/h at sea level, 560 at 4,400 m; climb 15-17 m/s
		stall about 150 km/h clean; leading-edge slats, plain flaps to 42 degrees
		with the ailerons drooping with them; the whole tailplane trims
		retractable main gear, folding outward into the wing, hydraulic
	With the numbers below the script works out to (tools/bf109_sim.py): see
	CHANGELOG.md and the README's check list.

*/

class CfgPatches {
	class GebsPlanes_BF109 {
		units[]= {
			"GebsBF109E3",
			"GebsBF109E3_Winter",
			"GebsBF109E3_Desert",
			"GebsBF109W",
			"GebsBF109W_Winter",
			"GebsBF109W_Desert",
			"GebsBF109_Wheel"
		};
		weapons[]= {};
		requiredVersion=0.1;
		requiredAddons[]= {
			"GebsPlanes",
			"GebsPlanes_Data",
			"DZ_Data",
			"DZ_Vehicles_Wheeled",
			"DZ_Sounds_Effects"
		};
	};
};

/*
	Wheel slots. The main wheels come off like any car wheel; the plane's own
	wheel simulation reads their size and condition. The tailwheel is part of
	the plane.
*/
class CfgSlots {
	class Slot_GebsBF109_Wheel_1_2 {
		name="GebsBF109_Wheel_1_2";
		displayName="$STR_GebsPlanes_WheelLeft";
		selection="wheel_1_2";
		ghostIcon="set:dayz_inventory image:wheel";
	};
	class Slot_GebsBF109_Wheel_2_2 {
		name="GebsBF109_Wheel_2_2";
		displayName="$STR_GebsPlanes_WheelRight";
		selection="wheel_2_2";
		ghostIcon="set:dayz_inventory image:wheel";
	};
};

//! the main wheel model, shown on its proxies in the plane (they ride on the folding legs)
class CfgNonAIVehicles {
	class ProxyAttachment;
	class ProxyGebsBF109_Wheel: ProxyAttachment {
		scope=2;
		inventorySlot[]= {
			"GebsBF109_Wheel_1_2",
			"GebsBF109_Wheel_2_2"
		};
		model="\gebsaircraft\data\bf109\proxy\gebsbf109_wheel.p3d";
	};
};

class CfgVehicles {
	class CarWheel;
	class GebsPlane_Base;

	/*
		The main wheel: a 650 x 150 mm tyre on a light alloy rim. An ordinary car
		wheel item: the plane's wheel simulation takes the fitted wheel's radius
		(which shrinks as it is damaged) and its tyre's grip and rolling
		resistance from here, so a ruined wheel drags.
	*/
	class GebsBF109_Wheel: CarWheel {
		scope=2;
		displayName="$STR_GebsPlanes_BF109_Wheel0";
		descriptionShort="$STR_GebsPlanes_BF109_Wheel1";
		model="\gebsaircraft\data\bf109\proxy\gebsbf109_wheel.p3d";
		weight=16000;
		inventorySlot[]= {
			"GebsBF109_Wheel_1_2",
			"GebsBF109_Wheel_2_2"
		};
		rotationFlags=4;
		repairableWithKits[]= {6};
		repairCosts[]= {30};
		radiusByDamage[]= {0,0.313,0.3,0.27,0.9998,0.23,0.9999,0.19};
		radius=0.313;
		width=0.16;
		// a high-pressure fighter tyre: good on a hard strip, it ruts and drags in soft ground
		tyreType=0;
		tyreOffroadResistance=0.85;
		tyreRollResistance=0.015;
		tyreGrip=0.85;
		tyreRoughness=1;
		tyreLongitudinalFriction=0.9;
		tyreLateralFriction=0.9;
	};
	class GebsBF109_Wheel_Ruined: GebsBF109_Wheel {
		radius=0.22;
		width=0.14;
		tyreOffroadResistance=0.1;
		tyreRollResistance=0.3;
		tyreGrip=0.4;
		tyreLongitudinalFriction=0.4;
		tyreLateralFriction=0.4;
	};

	/*
		Messerschmitt Bf 109 E-3
	*/
	class GebsBF109_Base: GebsPlane_Base {
		scope=0;
		displayName="$STR_GebsPlanes_BF109E3_0";
		descriptionShort="$STR_GebsPlanes_BF109E3_1";
		model="\gebsaircraft\data\bf109\gebsbf109.p3d";

		// one seat, under the canopy; boarded from the left wing root
		class Crew {
			class Driver {
				actionSel="seat_driver";
				proxyPos="crewDriver";
				getInPos="pos_driver";
				getInDir="pos_driver_dir";
				isDriver=1;
			};
		};

		/*
			Everything that moves is a "user" source: the script sets the controls,
			the gear, the propeller, the cockpit and every needle each frame on each
			client, so they answer at once (tiny animPeriod). The wheels keep the
			names a car's animations use. The canopy is set on the server, like a
			car door, and swings in its own time.
		*/
		class AnimationSources {
			class DoorsDriver {
				source="user";
				initPhase=0;
				animPeriod=1.4;
			};
			class elevator {
				source="user";
				initPhase=0.5;
				animPeriod=0.0001;
			};
			class aileron: elevator {};
			class rudder: elevator {};
			class trim: elevator {};
			class stick_pitch: elevator {};
			class stick_roll: elevator {};
			class pedals: elevator {};
			class flaps {
				source="user";
				initPhase=0;
				animPeriod=0.0001;
			};
			class gear: flaps {};
			class radiator: flaps {};
			class throttle: flaps {};
			class flapwheel: flaps {};
			class gearlever: flaps {};
			class propeller: flaps {};
			// the blades hide when prop_blur is 1, the blurred disc when prop_still is 1
			class prop_blur: flaps {};
			class prop_still {
				source="user";
				initPhase=1;
				animPeriod=0.0001;
			};
			// the gear lamps' lit faces hide at 1
			class lamp_down_off: prop_still {};
			class lamp_up_off: prop_still {};
			// wheels: spin in turns (0..1), the tailwheel's castor -90..90 deg as 0..1, straight at 0.5
			class wheelbackleft: flaps {};
			class wheelbackright: flaps {};
			class wheelfrontleft: flaps {};
			class turnfrontleft: elevator {};
			// the panel's needles (GebsFlight Gauges)
			class gauge_ias: flaps {};
			class gauge_alt_m: flaps {};
			class gauge_alt_km: flaps {};
			class gauge_turn: elevator {};
			class gauge_slip: elevator {};
			class gauge_heading: flaps {};
			class gauge_vario: elevator {};
			class gauge_rpm: flaps {};
			class gauge_boost: flaps {};
			class gauge_pitch_h: flaps {};
			class gauge_pitch_m: flaps {};
			class gauge_clock_h: flaps {};
			class gauge_clock_m: flaps {};
			class gauge_fuel: flaps {};
			class gauge_fuelpress: flaps {};
			class gauge_oilpress: flaps {};
			class gauge_brake_l: flaps {};
			class gauge_brake_r: flaps {};
			class gauge_coolant: flaps {};
			class gauge_oiltemp: flaps {};
			class gauge_amps: flaps {};
		};

		attachments[]= {
			"GebsBF109_Wheel_1_2",
			"GebsBF109_Wheel_2_2"
		};
		class GUIInventoryAttachmentsProps {
			class Chassis {
				name="$STR_attachment_Chassis0";
				description="";
				icon="set:dayz_inventory image:cat_vehicle_chassis";
				attachmentSlots[]= {
					"GebsBF109_Wheel_1_2",
					"GebsBF109_Wheel_2_2"
				};
			};
		};

		/*
			The paint, on two texture sheets painted by tools/paint_bf109.py: camo is
			the fuselage, cowling, tail and canopy frames, camo_wing the wing with its
			flaps and ailerons. Another livery is two more sheets swapped in here.
		*/
		hiddenSelections[]= {
			"camo",
			"camo_wing"
		};
		hiddenSelectionsTextures[]= {
			"gebsaircraft\data\bf109\data\gebsbf109_body_co.paa",
			"gebsaircraft\data\bf109\data\gebsbf109_wing_co.paa"
		};
		hiddenSelectionsMaterials[]= {
			"gebsaircraft\data\bf109\data\gebsbf109_body_paint.rvmat",
			"gebsaircraft\data\bf109\data\gebsbf109_wing_paint.rvmat"
		};

		// the L-shaped tank under and behind the seat (GebsPlane: its own tank,
		// filled from a jerrycan at the "refill" point, the filler on the left side)
		fuelCapacity=400;

		/*
			Every Health class needs its healthLevels, as vanilla's vehicles have
			them: health levels are read through them, and a zone without them reads
			as ruined. A ruined Engine zone, or no fuel, and the engine won't run;
			crashes damage the zone they hit (GebsPlane.EOnContact).
		*/
		class DamageSystem {
			class GlobalHealth {
				class Health {
					hitpoints=2500;
					healthLevels[]= {{1,{}},{0.7,{}},{0.5,{}},{0.3,{}},{0,{}}};
				};
			};
			class DamageZones {
				class Chassis {
					displayName="$STR_CfgVehicleDmg_Chassis0";
					fatalInjuryCoef=-1;
					componentNames[]= {
						"dmgZone_chassis"
					};
					class Health {
						hitpoints=3500;
						transferToGlobalCoef=0.5;
						healthLevels[]= {{1,{}},{0.7,{}},{0.5,{}},{0.3,{}},{0,{}}};
					};
					inventorySlots[]= {};
				};
				class Engine {
					displayName="$STR_CfgVehicleDmg_Engine0";
					fatalInjuryCoef=0.001;
					memoryPoints[]= {
						"dmgZone_engine"
					};
					componentNames[]= {
						"dmgZone_engine"
					};
					class Health {
						hitpoints=1600;
						transferToGlobalCoef=1;
						healthLevels[]= {{1,{}},{0.7,{}},{0.5,{}},{0.3,{}},{0,{}}};
					};
					inventorySlots[]= {};
				};
				class FuelTank {
					displayName="$STR_CfgVehicleDmg_FuelTank0";
					fatalInjuryCoef=-1;
					componentNames[]= {
						"dmgZone_fuelTank"
					};
					class Health {
						hitpoints=800;
						transferToGlobalCoef=0;
						healthLevels[]= {{1,{}},{0.7,{}},{0.5,{}},{0.3,{}},{0,{}}};
					};
					inventorySlots[]= {};
				};
			};
		};

		/*
			How it flies (scripts/4_world/entities/vehicles/gebsplane.c).
			Angles in degrees, lengths in metres, forces in newtons.
		*/
		class GebsFlight {
			// memory points
			propHub="prop_hub";				// where the thrust acts
			propFront="prop_front";			// ahead of the hub on the propeller axis: says which way is forward
			bodyPoint="aero_body";			// where fuselage drag acts
			doorPoint="pos_door";			// where the pilot is put down after getting out: left, behind the wing root
			boardPoint="pos_cockpit";		// get in, or work the canopy from outside, within boardReach of here
			boardReach=3.5;
			doorSource="DoorsDriver";
			doorOpenSound="GebsBF109_canopy_open_SoundSet";
			doorCloseSound="GebsBF109_canopy_close_SoundSet";
			spawnFuel=0.5;					// share of a full tank a new plane comes with
			// a knock harder than this (N s; a landing hard enough to bottom the
			// springs is about 8,000) damages the part it hits, by crashDamage
			// health per N s past it: the engine near the nose, the airframe elsewhere
			crashImpulse=9000;
			crashDamage=0.06;

			// the cockpit's stick, pedals, throttle, flap wheel, gear lever, lamps and
			// radiator flaps move (GebsPlane.UpdateCockpit), and the tailplane trims
			cockpit=1;
			trimAnim=1;
			// the keys: a held pull slows as the wing panel nearest its stall comes within stallMargin
			// degrees of it, or the g nears pullG, and stops there, as a pilot's hand does at the first
			// buffet (GebsPlane.PullRoom). 0 turns either off: the keys could then pull it into the stall,
			// where it drops a wing (Cole's run, 2026-11-03)
			stallMargin=1.5;
			pullG=6;

			// propeller and engine: the DB 601 A on a VDM constant-speed propeller,
			// turning clockwise seen from the cockpit; the governor holds the revs the
			// throttle sets, from idle to 2,400
			propType="constantSpeed";
			ratedPower=864000;		// W, 1,175 PS
			criticalAltitude=4000;	// m: the supercharger holds full power up to here
			propEfficiency=0.82;	// of the shaft power, what becomes thrust at speed
			staticThrust=9500;		// N standing still at full power
			propDiameter=3.1;
			propDragArea=0.35;		// a dead, windmilling propeller
			propInertia=30;			// kg m2: its spin kicks the nose round when it pitches or yaws
			propRotation=1;
			gearRatio=1.55;			// engine revs per propeller rev
			idlePower=0.06;
			powerLag=0.5;			// s
			rpmIdle=650;			// engine rpm
			rpmMax=2400;
			rpmLag=0.8;				// s for the governor to settle
			rpmPerSpeed=7;			// propeller rpm a dead engine windmills at, per m/s
			rpmOverspeed=2900;		// above this, a dive damages the engine
			boostIdle=0.6;			// ata on the manifold pressure gauge
			boostMax=1.35;
			soundRpmScale=1;
			slipstream=0.45;		// share of the far-wake slipstream that reaches the tail
			swirl=0.12;				// its corkscrew on the fin: right rudder on the takeoff run
			// the engine's torque rolls the airframe left. Its slipstream's swirl, striking the wing
			// roots the other way, gives much of that back. At 0.55 it rolled the plane over at
			// lift-off unless the pilot held the tail up and the wings level just so
			// (tools/bf109_sim.py); at 0.3 a held-back stick or a hands-off run climbs away
			torque=0.3;
			// the ailerons' fixed tab, set on the ground so that at cruise (about 350 km/h,
			// half power) the wings stay level against that torque with the stick let go
			aileronRig=0.025;
			fuelBurnIdle=40;		// L/h
			fuelBurnFull=400;		// L/h: a full tank lasts an hour flat out

			// engine start: the inertia starter's flywheel winds up for starterWindTime,
			// then engages and turns the engine over, and with fuel and a sound engine
			// it fires at catchTime; one that won't fire gives up crankGiveUp later
			starterRpm=140;
			starterWindTime=3.0;	// s
			catchTime=4.4;			// s
			crankGiveUp=3;			// s

			// airframe
			bodyCdaForward=0.27;	// fuselage, cowling, radiators and tail struts head-on (m2)
			bodyCdaSide=4.5;
			bodyCdaUp=6.0;
			gearCda=0.2;			// more head-on with the gear down
			span=9.87;
			downwash=0.106;			// rad at the tail per unit of wing lift coefficient: 2 / (pi * aspect)
			groundEffect=1;
			// DayZ's wind on the plane: off, as the server and the pilot's machine each
			// have their own copy of the weather (see the README)
			windScale=0;
			// the flaps: a handwheel beside the seat, wound down notch by notch
			// (0, 10, 20, 30, 42 degrees) and slowly
			flapNotches=4;
			flapRate=0.12;			// share of full travel per second: 8 s all the way
			flapDegrees=42;
			// the slats, on the outer wing: they slide out by themselves as the angle of
			// attack climbs past slatAlpha and are fully out by slatFull, putting the
			// outer wing's stall off by slatStall
			slatAlpha=10;
			slatFull=15;
			slatStall=6;
			slatCL=0.12;
			slatTime=0.3;			// s
			// while testing: once a second with someone in the seat, two lines in the
			// pilot's and the server's script logs. 0 turns it off
			debugLog=1;

			// engine sound: the DB 601's own loops (tools/sounds_bf109.py), fed the
			// engine's rpm, its power as their "thrust" and the airspeed; the wind's
			// while it moves
			soundPoint="sound_engine";
			engineSounds[]= {
				"GebsBF109_Engine_Idle_SoundSet",
				"GebsBF109_Engine_Low_SoundSet",
				"GebsBF109_Engine_Mid_SoundSet",
				"GebsBF109_Engine_High_SoundSet",
				"GebsBF109_Engine_Idle_Int_SoundSet",
				"GebsBF109_Engine_Low_Int_SoundSet",
				"GebsBF109_Engine_Mid_Int_SoundSet",
				"GebsBF109_Engine_High_Int_SoundSet"
			};
			airSounds[]= {
				"GebsBF109_Wind_SoundSet"
			};
			engineStartSound="GebsBF109_starter_SoundSet";
			engineCatchSound="GebsBF109_catch_SoundSet";
			engineFailSound="GebsBF109_starter_fail_SoundSet";
			engineStopSound="GebsBF109_stop_SoundSet";
			gearMoveSound="GebsBF109_gear_move_SoundSet";
			gearLockSound="GebsBF109_gear_lock_SoundSet";
			flapSound="GebsBF109_flaps_SoundSet";

			// the gear: hydraulic, worked by the engine's pump. Up only with the engine
			// running and no weight on the wheels; down slower without the engine
			gearUpTime=6;			// s
			gearDownTime=5;
			gearNeedsEngine=1;

			// on the ground. Standing still at idle with the pedals centred, the
			// brakes hold it. Rudder at a crawl also brakes the main wheel on that
			// side, as the toe brakes do: on the 109's narrow gear, a steady hand
			parkHoldSpeed=0.5;		// m/s
			diffBrake=0.7;			// share of full brake on the inside wheel at full rudder
			diffBrakeFade=40;		// km/h by which it is gone
			// the tailwheel's swing narrows as the speed builds: full to 15 km/h,
			// down to tailSteerMin of it by 15 + tailSteerFade
			tailSteerFade=45;		// km/h
			tailSteerMin=0.1;

			/*
				The wheels (GebsPlaneWheel): each casts a ray down from its hub, a
				spring and damper hold the plane up off the hit, and the tyre grips up
				to what its load allows, sideways and along, with the brakes. The mains
				fold away with the gear (retracts=1). A spring settles by load /
				stiffness, travelDown of it from the modelled height.
			*/
			class Wheels {
				class MainLeft {
					hub="wheel_1_2_hub";
					slot="GebsBF109_Wheel_1_2";
					radius=0.313;
					stiffness=190000;		// about 11 kN on each main wheel: it settles 6 cm
					compression=14000;		// N per m/s, damping as it compresses
					rebound=18000;			// and as it extends
					travelUp=0.12;
					travelDown=0.06;
					brakeForce=6500;		// N at the tyre, full brake: hydraulic, strong enough to nose it over
					grip=0.9;
					lateralGrip=1;
					// the tyre's side grip builds with its slip angle, full by 8 deg, as a real tyre's does.
					// Gripping fully at once, as on rails, a second of rudder at 95 km/h, with the wing
					// carrying half the weight, rolled the 109 onto its wingtip (Cole's run, 2026-11-03)
					slipPeak=8;
					rollResistance=0.015;
					steer=0;
					brakeSide=-1;			// left
					retracts=1;
					spinAnim="wheelbackleft";
				};
				class MainRight: MainLeft {
					hub="wheel_2_2_hub";
					slot="GebsBF109_Wheel_2_2";
					brakeSide=1;
					spinAnim="wheelbackright";
				};
				// the tailwheel castors on its fork; here the pedals swing it, less as
				// the speed builds, and its sideways grip keeps a taildragger straight
				class Tail {
					hub="tailwheel_hub";
					slot="";
					radius=0.135;
					stiffness=70000;		// about 3.4 kN on it: it settles 5 cm
					compression=3000;
					rebound=4000;
					travelUp=0.12;
					travelDown=0.04;
					brakeForce=0;
					grip=0.9;
					lateralGrip=1.4;
					rollResistance=0.02;
					steer=-25;
					brakeSide=0;
					spinAnim="wheelfrontleft";
					steerAnim="turnfrontleft";
				};
			};

			/*
				The lifting surfaces, each at its own memory point (its quarter chord).
				The wing is four panels so that rolling, and a stall that starts on one
				side, come out of the airflow at each one: the flaps are on the inner
				panels, the ailerons (drooping with the flaps) and the slats on the
				outer. NACA 2R1 sections: zero lift at -1.5 deg, Cm -0.025.
			*/
			class Surfaces {
				class WingLeftInner {
					point="aero_wing_l_in";
					type="horizontal";
					area=5.7;
					aspect=6.0;		// the whole wing's
					chord=1.75;
					// rigged 2 deg lower against the fuselage than the drawings' 1 deg (the tailplane with it,
					// so it trims the same): in the three-point attitude the inner wing sat at its flapped
					// stall, and any roll at lift-off dropped a wing. It flies 2 deg nose higher
					incidence=-1;
					alpha0=-1.5;
					stall=15.5;
					stallNeg=-13;
					cd0=0.0075;
					oswald=0.8;
					cm0=-0.025;
					dihedral=6.5;	// the wing rises 6.5 deg to its tips: what keeps its wings level
					flapCL=0.9;
					flapCD=0.09;
					flapCm=-0.1;
					slipstream=0.3;
					wing=1;
				};
				class WingRightInner: WingLeftInner {
					point="aero_wing_r_in";
				};
				class WingLeftOuter: WingLeftInner {
					point="aero_wing_l_out";
					area=2.5;
					chord=1.3;
					stall=13.5;
					// the drooping aileron
					flapCL=0.25;
					flapCD=0.02;
					flapCm=-0.02;
					slipstream=0;
					control="aileronLeft";
					controlGain=5.5;
					slats="left";
				};
				class WingRightOuter: WingLeftOuter {
					point="aero_wing_r_out";
					control="aileronRight";
					slats="right";
				};
				class Tailplane {
					point="aero_htail";
					type="horizontal";
					area=2.7;
					aspect=3.5;
					chord=0.85;
					incidence=-3;
					alpha0=0;
					stall=17;
					stallNeg=-17;
					cd0=0.008;
					oswald=0.75;
					control="elevator";
					controlGain=10;		// full back stick lifts it off without pivoting it on its tailwheel into the stall
					trimGain=4;
					downwash=1;
					slipstream=0.7;
				};
				class Fin {
					point="aero_vtail";
					type="vertical";
					area=1.6;
					aspect=1.5;
					chord=0.95;
					incidence=0;
					alpha0=0;
					stall=18;
					stallNeg=-18;
					cd0=0.008;
					oswald=0.7;
					control="rudder";
					controlGain=16;
					slipstream=0.7;
					swirl=1;
				};
			};

			// --- gauges: begin ---
			// written by tools/build_bf109.py from bf109_scene.GAUGE_DEFS: what each needle reads, from its
			// value at the start of its sweep to the end (model.cfg turns it from angle0 to angle1)
			class Gauges
			{
				class Ias { source="gauge_ias"; value="ias"; min=0; max=750; wrap=0; lag=0.25; };
				class Alt_m { source="gauge_alt_m"; value="altitude"; min=0; max=1000; wrap=1; lag=0; };
				class Alt_km { source="gauge_alt_km"; value="altitude"; min=0; max=10000; wrap=0; lag=0.3; };
				class Turn { source="gauge_turn"; value="turn"; min=-6; max=6; wrap=0; lag=0.4; };
				class Slip { source="gauge_slip"; value="slip"; min=-1; max=1; wrap=0; lag=0.4; };
				class Heading { source="gauge_heading"; value="heading"; min=0; max=360; wrap=1; lag=0; };
				class Vario { source="gauge_vario"; value="vario"; min=-30; max=30; wrap=0; lag=0.4; };
				class Rpm { source="gauge_rpm"; value="rpm"; min=0; max=3600; wrap=0; lag=0.3; };
				class Boost { source="gauge_boost"; value="boost"; min=0.6; max=1.8; wrap=0; lag=0.3; };
				class Pitch_h { source="gauge_pitch_h"; value="pitch"; min=12; max=72; wrap=0; lag=0.4; };
				class Pitch_m { source="gauge_pitch_m"; value="pitch"; min=12; max=17; wrap=1; lag=0; };
				class Clock_h { source="gauge_clock_h"; value="clockHours"; min=0; max=12; wrap=1; lag=0; };
				class Clock_m { source="gauge_clock_m"; value="clockMinutes"; min=0; max=60; wrap=1; lag=0; };
				class Fuel { source="gauge_fuel"; value="fuel"; min=0; max=400; wrap=0; lag=2; };
				class Fuelpress { source="gauge_fuelpress"; value="fuelPressure"; min=0; max=3; wrap=0; lag=0.4; };
				class Oilpress { source="gauge_oilpress"; value="oilPressure"; min=0; max=10; wrap=0; lag=0.4; };
				class Brake_l { source="gauge_brake_l"; value="brakeLeft"; min=0; max=1; wrap=0; lag=0.4; };
				class Brake_r { source="gauge_brake_r"; value="brakeRight"; min=0; max=1; wrap=0; lag=0.4; };
				class Coolant { source="gauge_coolant"; value="coolantTemp"; min=0; max=130; wrap=0; lag=1; };
				class Oiltemp { source="gauge_oiltemp"; value="oilTemp"; min=0; max=120; wrap=0; lag=1; };
				class Amps { source="gauge_amps"; value="amps"; min=-20; max=40; wrap=0; lag=0.4; };
			};
			// --- gauges: end ---
		};
	};

	class GebsBF109E3: GebsBF109_Base {
		scope=2;
	};
	/*
		Camouflage (tools/paint_bf109.py --scheme): winter, the uppers whitewashed in
		the field round the markings, the yellow cowling, tail and wingtips left;
		desert, RLM 79 sand with RLM 80 mottles over RLM 78, a white fuselage band
		and wingtips. Only the painted faces' colour sheets change.
	*/
	class GebsBF109E3_Winter: GebsBF109E3 {
		displayName="$STR_GebsPlanes_BF109E3_Winter0";
		hiddenSelectionsTextures[]= {
			"gebsaircraft\data\bf109\data\gebsbf109_body_winter_co.paa",
			"gebsaircraft\data\bf109\data\gebsbf109_wing_winter_co.paa"
		};
	};
	class GebsBF109E3_Desert: GebsBF109E3 {
		displayName="$STR_GebsPlanes_BF109E3_Desert0";
		hiddenSelectionsTextures[]= {
			"gebsaircraft\data\bf109\data\gebsbf109_body_desert_co.paa",
			"gebsaircraft\data\bf109\data\gebsbf109_wing_desert_co.paa"
		};
	};

	/*
		The Bf 109 on floats (the W, for "Wasser"): the E-3 with its gear folded
		away for good and twin floats on struts under the wing, after the photo
		Cole sent of a 109 on floats. It takes off from and lands on the sea and
		the lakes and can't use a runway; on land it rests on its floats.

		Everything not said here is the E-3's: how it flies, its engine, cockpit,
		sounds and paint. The floats are simulated in slices along each one
		(GebsPlaneFloat, scripts/4_world/entities/vehicles/gebsplanefloat.c):
		buoyancy, the bottom's planing lift and drag, the water's grip sideways
		and the water rudders on the pedals. Tuned in tools/bf109_sim.py --plane w.
	*/
	class GebsBF109W: GebsBF109_Base {
		scope=2;
		displayName="$STR_GebsPlanes_BF109W_0";
		descriptionShort="$STR_GebsPlanes_BF109W_1";
		model="\gebsaircraft\data\bf109\gebsbf109w.p3d";
		// no wheels to fit
		attachments[]= {};
		class GUIInventoryAttachmentsProps {};
		// the E-3's paint, and the floats' (tools/paint_floats.py) in camo_floats, which a livery repaints
		hiddenSelections[]= {
			"camo",
			"camo_wing",
			"camo_floats"
		};
		hiddenSelectionsTextures[]= {
			"gebsaircraft\data\bf109\data\gebsbf109_body_co.paa",
			"gebsaircraft\data\bf109\data\gebsbf109_wing_co.paa",
			"gebsaircraft\data\bf109\data\gebsbf109_floats_co.paa"
		};
		hiddenSelectionsMaterials[]= {
			"gebsaircraft\data\bf109\data\gebsbf109_body_paint.rvmat",
			"gebsaircraft\data\bf109\data\gebsbf109_wing_paint.rvmat",
			"gebsaircraft\data\bf109\data\gebsbf109_floats.rvmat"
		};

		class GebsFlight: GebsFlight {
			// the gear stays folded in its wells (the model has it so); nothing to work
			gearUpTime=0;
			gearCda=0;
			// the floats' own drag is theirs (Floats, below); their struts and fittings here
			bodyCdaForward=0.31;
			// a swimmer climbs in from the water, well below the cockpit
			boardHeight=4.5;
			// its own wheels: none (an empty class, so the E-3's aren't inherited)
			class Wheels {};
			// the water's sounds, from vanilla's boat: lapping at a taxi, rushing at speed, a splash as a
			// float comes down on it
			waterSounds[]= {
				"GebsBF109W_Water_Light_SoundSet",
				"GebsBF109W_Water_Strong_SoundSet"
			};
			splashSound="boat_01_splash_SoundSet";

			/*
				The floats. Each is cut into slices along it, each slice a vee
				bottom with upright sides. Under the water a slice is buoyed up by
				what it displaces; moving, its bottom planes - the water pushes on it
				as hard as the bottom meets the water (planing, per radian of the
				angle and square metre of wetted bottom), so up on the step it
				carries the plane on a little of its length; displacing water it
				makes waves (humpDrag of the water it displaces at humpSpeed, less
				either side: the hump the takeoff run has to get over); its skin
				drags (skinFriction); sideways the floats grip like keels (sideDrag),
				and going up and down they are damped (heaveDamping, N per m/s per
				square metre of waterline).
			*/
			class Floats {
				waterDensity=1000;		// kg/m3: fresh water; the sea is a little denser
				planing=0.5;
				skinFriction=0.0032;
				humpSpeed=8;			// m/s
				humpDrag=0.15;
				sideDrag=0.8;
				heaveDamping=1100;
				// the water rudders on the transoms, on the pedals: of use at a taxi, lifted by
				// rudderFade km/h (a pilot cranks them up for the takeoff)
				rudderArea=0.12;		// m2 each
				rudderAngle=30;			// deg at full pedal
				rudderLift=2.5;			// per rad
				rudderStall=28;			// deg
				rudderFade=45;			// km/h
				// each float's own air drag (m2), at its middle
				airCdaForward=0.10;
				airCdaSide=1.6;
				airCdaUp=2.5;
				// hitting the water harder than this many g damages the airframe, crashDamage per
				// N s past it; the engine drowns with the propeller's hub under water
				slamLoad=5;
				// on the keys, up on the step (past twice humpSpeed) a held pull eases to this much stick: the
				// planing attitude. Full back there drags the afterbody, and as the floats come off the nose is
				// thrown up into the stall (tools/bf109_sim.py: a wing dropped 41 deg); over the hump it's full
				stepPull=0.4;
				rudderDepth=0.30;		// m of water rudder blade
				// a wrecked plane's floats flood: the share of their buoyancy left (it settles and sinks)
				ruinedBuoyancy=0.35;

				// --- floats: begin ---
				// written by tools/build_bf109.py from tools/bf109_floats.py: each float's slices, bow to stern,
				// each at the keel's memory point in its middle: its length, the half beam at its chines, the
				// vee's depth and the deck's height above the keel (m), the bottom's slope (deg, + rising to the bow)
				class Left {
					slices[]= {"float_l_00", "float_l_01", "float_l_02", "float_l_03", "float_l_04", "float_l_05", "float_l_06", "float_l_07", "float_l_08", "float_l_09", "float_l_10", "float_l_11"};
					length[]= {0.600, 0.600, 0.600, 0.600, 0.600, 0.600, 0.550, 0.550, 0.550, 0.550, 0.550, 0.550};
					halfBeam[]= {0.209, 0.370, 0.443, 0.460, 0.460, 0.460, 0.450, 0.412, 0.363, 0.304, 0.239, 0.167};
					chine[]= {0.165, 0.271, 0.296, 0.279, 0.252, 0.227, 0.210, 0.192, 0.169, 0.142, 0.111, 0.070};
					deck[]= {0.367, 0.680, 0.857, 0.900, 0.900, 0.900, 0.797, 0.680, 0.555, 0.426, 0.293, 0.157};
					slope[]= {23.196, 15.402, 6.980, 0.000, 0.000, 0.000, -9.000, -9.000, -9.000, -9.000, -9.000, -9.000};
					rudder="float_l_rudder";
					centre="float_l_centre";
					sprayFront="float_l_spray_front";
					spraySide="float_l_spray_side";
				};
				class Right: Left {
					slices[]= {"float_r_00", "float_r_01", "float_r_02", "float_r_03", "float_r_04", "float_r_05", "float_r_06", "float_r_07", "float_r_08", "float_r_09", "float_r_10", "float_r_11"};
					rudder="float_r_rudder";
					centre="float_r_centre";
					sprayFront="float_r_spray_front";
					spraySide="float_r_spray_side";
				};
				// --- floats: end ---
			};
		};
	};
	// camouflage, the floats painted to match (tools/paint_floats.py --scheme)
	class GebsBF109W_Winter: GebsBF109W {
		displayName="$STR_GebsPlanes_BF109W_Winter0";
		hiddenSelectionsTextures[]= {
			"gebsaircraft\data\bf109\data\gebsbf109_body_winter_co.paa",
			"gebsaircraft\data\bf109\data\gebsbf109_wing_winter_co.paa",
			"gebsaircraft\data\bf109\data\gebsbf109_floats_winter_co.paa"
		};
	};
	class GebsBF109W_Desert: GebsBF109W {
		displayName="$STR_GebsPlanes_BF109W_Desert0";
		hiddenSelectionsTextures[]= {
			"gebsaircraft\data\bf109\data\gebsbf109_body_desert_co.paa",
			"gebsaircraft\data\bf109\data\gebsbf109_wing_desert_co.paa",
			"gebsaircraft\data\bf109\data\gebsbf109_floats_desert_co.paa"
		};
	};
};

/*
	The sounds (tools/sounds_bf109.py writes the files). The engine's loops are
	each made at one rpm and pitched from there by the shader, crossfading by
	rpm; the outside set plays with the camera outside, the inside set (lower,
	duller, with the airframe's rattle) in the cockpit.
*/
class CfgSoundShaders {
	class baseVehicles_SoundShader;
	class GebsBF109_Engine_Base_SoundShader {
		range=1200;
	};
	class GebsBF109_Engine_Idle_SoundShader: GebsBF109_Engine_Base_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_engine_idle",1}};
		frequency="rpm / 650";
		volume="engineOn * campos * (1 - (rpm factor [900,1250])) * (0.75 + 0.25 * thrust)";
	};
	class GebsBF109_Engine_Low_SoundShader: GebsBF109_Engine_Base_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_engine_low",1}};
		frequency="rpm / 1200";
		volume="engineOn * campos * (rpm factor [900,1250]) * (1 - (rpm factor [1500,1850])) * (0.7 + 0.3 * thrust)";
	};
	class GebsBF109_Engine_Mid_SoundShader: GebsBF109_Engine_Base_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_engine_mid",1}};
		frequency="rpm / 1800";
		volume="engineOn * campos * (rpm factor [1500,1850]) * (1 - (rpm factor [2050,2300])) * (0.65 + 0.35 * thrust)";
	};
	class GebsBF109_Engine_High_SoundShader: GebsBF109_Engine_Base_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_engine_high",1}};
		frequency="rpm / 2400";
		volume="engineOn * campos * (rpm factor [2050,2300]) * (0.6 + 0.4 * thrust)";
	};
	class GebsBF109_Engine_Idle_Int_SoundShader: GebsBF109_Engine_Idle_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_engine_idle_int",1}};
		volume="engineOn * (1 - campos) * (1 - (rpm factor [900,1250])) * (0.75 + 0.25 * thrust)";
	};
	class GebsBF109_Engine_Low_Int_SoundShader: GebsBF109_Engine_Low_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_engine_low_int",1}};
		volume="engineOn * (1 - campos) * (rpm factor [900,1250]) * (1 - (rpm factor [1500,1850])) * (0.7 + 0.3 * thrust)";
	};
	class GebsBF109_Engine_Mid_Int_SoundShader: GebsBF109_Engine_Mid_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_engine_mid_int",1}};
		volume="engineOn * (1 - campos) * (rpm factor [1500,1850]) * (1 - (rpm factor [2050,2300])) * (0.65 + 0.35 * thrust)";
	};
	class GebsBF109_Engine_High_Int_SoundShader: GebsBF109_Engine_High_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_engine_high_int",1}};
		volume="engineOn * (1 - campos) * (rpm factor [2050,2300]) * (0.6 + 0.4 * thrust)";
	};
	class GebsBF109_Wind_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_wind",1}};
		frequency="0.75 + 0.5 * (speed factor [40,600])";
		volume="(speed factor [30,450]) * (0.55 + 0.45 * campos) * (1 - 0.4 * (1 - campos) * (1 - doors))";
		range=250;
	};
	class GebsBF109_OneShot_SoundShader {
		volume=1;
		range=500;
	};
	class GebsBF109_starter_SoundShader: GebsBF109_OneShot_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_starter",1}};
		range=250;
	};
	class GebsBF109_starter_fail_SoundShader: GebsBF109_OneShot_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_starter_fail",1}};
		range=250;
	};
	class GebsBF109_catch_SoundShader: GebsBF109_OneShot_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_catch",1}};
		range=900;
	};
	class GebsBF109_stop_SoundShader: GebsBF109_OneShot_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_stop",1}};
		range=600;
	};
	class GebsBF109_gear_move_SoundShader: GebsBF109_OneShot_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_gear_move",1}};
		volume=0.7;
		range=60;
	};
	class GebsBF109_gear_lock_SoundShader: GebsBF109_OneShot_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_gear_lock",1}};
		volume=0.8;
		range=60;
	};
	class GebsBF109_flaps_SoundShader: GebsBF109_OneShot_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_flaps",1}};
		volume=0.45;
		range=15;
	};
	class GebsBF109_canopy_open_SoundShader: GebsBF109_OneShot_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_canopy_open",1}};
		volume=0.5;
		range=25;
	};
	class GebsBF109_canopy_close_SoundShader: GebsBF109_OneShot_SoundShader {
		samples[]= {{"gebsaircraft\data\bf109\sounds\gebsbf109_canopy_close",1}};
		volume=0.6;
		range=25;
	};
	// the floatplane's water, vanilla's boat's loops: here "thrust" is the share of the floats in the water
	// (GebsPlane.UpdateSound), "speed" km/h; quieter in the cockpit
	class GebsBF109W_Water_Light_SoundShader {
		samples[]= {{"\DZ\sounds\vehicles\boat_01\water\floating_water_light_loop",1}};
		volume="thrust * (1 - (speed factor [15,30])) * (0.5 + 0.5 * campos)";
		range=40;
	};
	class GebsBF109W_Water_Strong_SoundShader {
		samples[]= {{"\DZ\sounds\vehicles\boat_01\water\floating_water_strong_loop",1}};
		volume="1.6 * thrust * (speed factor [15,30]) * (0.45 + 0.55 * campos)";
		frequency="0.9 + 0.25 * (speed factor [30,180])";
		range=150;
	};
};

class CfgSoundSets {
	class baseVehicles_SoundSet;
	class GebsBF109_Engine_Idle_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_Engine_Idle_SoundShader"};
	};
	class GebsBF109_Engine_Low_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_Engine_Low_SoundShader"};
	};
	class GebsBF109_Engine_Mid_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_Engine_Mid_SoundShader"};
	};
	class GebsBF109_Engine_High_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_Engine_High_SoundShader"};
	};
	class GebsBF109_Engine_Idle_Int_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_Engine_Idle_Int_SoundShader"};
	};
	class GebsBF109_Engine_Low_Int_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_Engine_Low_Int_SoundShader"};
	};
	class GebsBF109_Engine_Mid_Int_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_Engine_Mid_Int_SoundShader"};
	};
	class GebsBF109_Engine_High_Int_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_Engine_High_Int_SoundShader"};
	};
	class GebsBF109_Wind_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_Wind_SoundShader"};
	};
	class GebsBF109_starter_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_starter_SoundShader"};
	};
	class GebsBF109_starter_fail_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_starter_fail_SoundShader"};
	};
	class GebsBF109_catch_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_catch_SoundShader"};
	};
	class GebsBF109_stop_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_stop_SoundShader"};
	};
	class GebsBF109_gear_move_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_gear_move_SoundShader"};
	};
	class GebsBF109_gear_lock_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_gear_lock_SoundShader"};
	};
	class GebsBF109_flaps_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_flaps_SoundShader"};
	};
	class GebsBF109_canopy_open_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_canopy_open_SoundShader"};
	};
	class GebsBF109_canopy_close_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109_canopy_close_SoundShader"};
		// as it shuts, near the end of the canopy's 1.4 s swing
		startDelay=1.1;
	};
	class GebsBF109W_Water_Light_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109W_Water_Light_SoundShader"};
	};
	class GebsBF109W_Water_Strong_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsBF109W_Water_Strong_SoundShader"};
	};
};
