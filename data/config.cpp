/*

	gebsaircraft - data

	The planes. Each is a bare Transport whose script (scripts/4_world/
	entities/vehicles/gebsplane.c) simulates all of it: wheels, engine, fuel
	and the air. Everything about how one flies and rolls is in its GebsFlight
	class; the positions of its wings, tail, propeller, wheels and door are
	memory points in its model.

	Piper PA-18-150 Super Cub, the numbers it is built on:
		span 10.73 m, wing area 16.58 m2, chord 1.6 m, length 6.88 m
		150 hp Lycoming O-320, fixed-pitch propeller
		empty 422 kg, max 794 kg; flown here at about 680 kg
		stall 43 mph with flaps, 46 without; top speed 130 mph; climb 960 ft/min
		two 18 US gal wing tanks, 136 L
	With the numbers below the script works out to: stall 74 km/h clean,
	69 km/h with full flaps, top speed about 198 km/h (123 mph), climb about
	4.8 m/s (950 ft/min), a takeoff run of roughly 70 m with half flaps.

*/

class CfgPatches {
	class GebsPlanes_Data {
		units[]= {
			"GebsPA18_Yellow",
			"GebsPA18_Winter",
			"GebsPA18_Desert",
			"GebsPA18F_Yellow",
			"GebsPA18F_Winter",
			"GebsPA18F_Desert",
			"GebsPA18_Wheel"
		};
		weapons[]= {};
		requiredVersion=0.1;
		requiredAddons[]= {
			"GebsPlanes",
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
	class Slot_GebsPA18_Wheel_1_2 {
		name="GebsPA18_Wheel_1_2";
		displayName="$STR_GebsPlanes_WheelLeft";
		selection="wheel_1_2";
		ghostIcon="set:dayz_inventory image:wheel";
	};
	class Slot_GebsPA18_Wheel_2_2 {
		name="GebsPA18_Wheel_2_2";
		displayName="$STR_GebsPlanes_WheelRight";
		selection="wheel_2_2";
		ghostIcon="set:dayz_inventory image:wheel";
	};
};

//! the main wheel model, shown on its proxies in the plane
class CfgNonAIVehicles {
	class ProxyAttachment;
	class ProxyGebsPA18_Wheel: ProxyAttachment {
		scope=2;
		inventorySlot[]= {
			"GebsPA18_Wheel_1_2",
			"GebsPA18_Wheel_2_2"
		};
		model="\gebsaircraft\data\pa18\proxy\gebspa18_wheel.p3d";
	};
};

class CfgVehicles {
	class Transport;
	class CarWheel;

	/*
		Wheels. Ordinary car wheel items: the plane's wheel simulation takes the
		fitted wheel's radius (which shrinks as it is damaged) and its tyre's
		grip and rolling resistance from here, so a ruined wheel drags.
	*/
	class GebsPA18_Wheel: CarWheel {
		scope=2;
		displayName="$STR_GebsPlanes_PA18_Wheel0";
		descriptionShort="$STR_GebsPlanes_PA18_Wheel1";
		model="\gebsaircraft\data\pa18\proxy\gebspa18_wheel.p3d";
		weight=9000;
		inventorySlot[]= {
			"GebsPA18_Wheel_1_2",
			"GebsPA18_Wheel_2_2"
		};
		rotationFlags=4;
		repairableWithKits[]= {6};
		repairCosts[]= {30};
		radiusByDamage[]= {0,0.215,0.3,0.19,0.9998,0.16,0.9999,0.13};
		radius=0.215;
		width=0.19;
		// fat, soft, low-pressure bush tyres: good on grass, slow on tarmac
		tyreType=0;
		tyreOffroadResistance=0.95;
		tyreRollResistance=0.012;
		tyreGrip=0.8;
		tyreRoughness=1;
		tyreLongitudinalFriction=0.85;
		tyreLateralFriction=0.95;
	};
	class GebsPA18_Wheel_Ruined: GebsPA18_Wheel {
		radius=0.15;
		width=0.17;
		tyreOffroadResistance=0.1;
		tyreRollResistance=0.3;
		tyreGrip=0.4;
		tyreLongitudinalFriction=0.4;
		tyreLateralFriction=0.4;
	};
	/*
		Every plane. A bare Transport (simulation "transport"): no car underneath.
		Its script (scripts/4_world/entities/vehicles/gebsplane.c) makes the
		physics body and simulates everything - wheels, engine, air - with the
		engine's deterministic vehicle physics.
	*/
	class GebsPlane_Base: Transport {
		scope=0;
		vehicleClass="Transport";
		storageCategory=4;		// saved with the vehicles, as cars are
		insideSoundCoef=0.9;

		class Crew {
			class Driver {
				actionSel="seat_driver";
				proxyPos="crewDriver";
				getInPos="pos_driver";
				getInDir="pos_driver_dir";
				isDriver=1;
			};
			class CoDriver {
				actionSel="seat_coDriver";
				proxyPos="crewCoDriver";
				getInPos="pos_coDriver";
				getInDir="pos_coDriver_dir";
			};
		};

		/*
			Control surfaces, the propeller and the wheels are "user" sources the
			script sets every frame on each client, so they answer at once (tiny
			animPeriod). The wheels keep the names a car's animations use. The door
			is set on the server, like a car door.
		*/
		class AnimationSources {
			class DoorsDriver {
				source="user";
				initPhase=0;
				animPeriod=0.8;
			};
			class elevator {
				source="user";
				initPhase=0.5;
				animPeriod=0.0001;
			};
			class aileron: elevator {};
			class rudder: elevator {};
			class flaps {
				source="user";
				initPhase=0;
				animPeriod=0.0001;
			};
			class propeller: flaps {};
			// the blades hide when prop_blur is 1, the blurred disc when prop_still is 1
			class prop_blur: flaps {};
			class prop_still {
				source="user";
				initPhase=1;
				animPeriod=0.0001;
			};
			class gauge_airspeed: flaps {};
			class gauge_altitude: flaps {};
			class gauge_altitude_k: flaps {};
			class gauge_rpm: flaps {};
			// wheels: spin in turns (0..1), spring (0..1 over its model.cfg travel), tailwheel steering
			// -90..90 deg as 0..1, straight at 0.5
			class wheelbackleft: flaps {};
			class wheelbackright: flaps {};
			class wheelfrontleft: flaps {};
			class damperbackleft: flaps {};
			class damperbackright: flaps {};
			class turnfrontleft: elevator {};
		};
	};

	/*
		Piper PA-18 Super Cub
	*/
	class GebsPA18_ColorBase: GebsPlane_Base {
		scope=0;
		displayName="$STR_GebsPlanes_PA18_0";
		descriptionShort="$STR_GebsPlanes_PA18_1";
		model="\gebsaircraft\data\pa18\gebspa18.p3d";

		attachments[]= {
			"GebsPA18_Wheel_1_2",
			"GebsPA18_Wheel_2_2"
		};
		class GUIInventoryAttachmentsProps {
			class Chassis {
				name="$STR_attachment_Chassis0";
				description="";
				icon="set:dayz_inventory image:cat_vehicle_chassis";
				attachmentSlots[]= {
					"GebsPA18_Wheel_1_2",
					"GebsPA18_Wheel_2_2"
				};
			};
		};

		/*
			The paint, on two texture sheets painted by tools/paint_pa18.py: camo is
			the fuselage, tail and cowling, camo_wing the wing, its flaps, ailerons
			and struts. Another livery is two more sheets swapped in here.
		*/
		hiddenSelections[]= {
			"camo",
			"camo_wing"
		};
		hiddenSelectionsTextures[]= {
			"gebsaircraft\data\pa18\data\gebspa18_body_co.paa",
			"gebsaircraft\data\pa18\data\gebspa18_wing_co.paa"
		};
		hiddenSelectionsMaterials[]= {
			"gebsaircraft\data\pa18\data\gebspa18_body_paint.rvmat",
			"gebsaircraft\data\pa18\data\gebspa18_wing_paint.rvmat"
		};

		// two 18 US gal wing tanks (GebsPlane: its own tank, filled from a jerrycan
		// at the "refill" point like a car's)
		fuelCapacity=136;

		/*
			Every Health class needs its healthLevels, as vanilla's vehicles have
			them: health levels are read through them, and a zone without them
			reads as ruined. A ruined Engine zone, or no fuel, and the engine won't
			run; crashes damage the zone they hit (GebsPlane.EOnContact).
		*/
		class DamageSystem {
			class GlobalHealth {
				class Health {
					hitpoints=1200;
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
						hitpoints=2000;
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
						hitpoints=1000;
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
						hitpoints=600;
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
			doorPoint="pos_door";			// outside the door: where someone getting out is put down
			boardReach=3.5;					// get in, or work the door from outside, within this of boardPoint (default doorPoint)
			doorSource="DoorsDriver";
			doorOpenSound="GebsPA18_door_open_SoundSet";
			doorCloseSound="GebsPA18_door_close_SoundSet";
			spawnFuel=0.5;					// share of a full tank a new plane comes with
			// a knock harder than this (N s; landing hard enough to bottom the
			// springs is about 3,000) damages the part it hits, by crashDamage
			// health per N s past it: the engine near the nose, the airframe elsewhere
			crashImpulse=4000;
			crashDamage=0.15;

			// propeller and engine: O-320, 150 hp, fixed pitch, turning clockwise
			// seen from the cockpit
			staticThrust=2200;
			propPitchSpeed=150;		// thrust falls linearly to nothing at this airspeed (m/s)
			propDiameter=1.88;
			propDragArea=0.25;		// a dead, windmilling propeller
			propRotation=1;
			ratedPower=112000;		// W
			idlePower=0.08;
			powerLag=0.6;			// s
			rpmIdle=650;
			rpmStatic=2300;			// full throttle standing still
			rpmPerSpeed=8;			// the propeller speeds up with the airspeed: ~2,700 flat out
			rpmOverspeed=3000;		// above this, a dive damages the engine
			soundRpmScale=2;
			slipstream=0.5;			// share of the far-wake slipstream that reaches the tail
			swirl=0.12;				// its corkscrew on the fin: right rudder on the takeoff run
			torque=1;				// the engine's torque rolls the airframe left
			fuelBurnIdle=4;			// L/h
			fuelBurnFull=34;		// L/h

			// airframe
			bodyCdaForward=0.45;	// fuselage, gear, struts and wires head-on (m2)
			bodyCdaSide=3.0;
			bodyCdaUp=2.5;
			span=10.7;
			downwash=0.092;			// rad at the tail per unit of wing lift coefficient: 2 / (pi * aspect)
			groundEffect=1;
			// DayZ's wind on the plane. Off: the server and the pilot's machine each
			// have their own copy of the weather, and the plane flown on two winds
			// came apart between them, so the server kept correcting it - a jolt
			// each time.
			windScale=0;
			// the keys: a held pull slows as the wing panel nearest its stall comes within stallMargin
			// degrees of it, or the g nears pullG, and stops there, as a pilot's hand does at the first
			// buffet. S held all the way down the runway then lifts it off at about 92 km/h and climbs
			// away; without the stop it came off at 80 right at the stall, dropped back and struck its
			// tail (tools/bf109_sim.py --plane cub, 2026-11-04). 1.5 deg, the Bf 109's, still struck it
			stallMargin=2.5;
			pullG=4;
			flapNotches=2;			// up, half, full
			gauges=0;				// the panel is one flat mesh: no needles to turn yet
			// while testing: once a second with the engine on, a line in the pilot's
			// script log with throttle, speed, thrust, lift, angle of attack, pitch,
			// heading, the wheels and the controls. 0 turns it off
			debugLog=1;

			// engine start: the starter turns the propeller over, and with fuel and
			// a sound engine it fires after catchTime of cranking
			starterRpm=250;
			catchTime=1.2;			// s
			// engine sound: vanilla's Sarka engine stands in until the plane has
			// recordings of its own. The loops are fed the propeller's rpm x
			// soundRpmScale, the engine's power as their "thrust" and the airspeed
			soundPoint="dmgZone_engine";
			engineSounds[]= {
				"CivilianSedan_Engine_Offload_Ext_Rpm1_SoundSet",
				"CivilianSedan_Engine_Offload_Ext_Rpm2_SoundSet",
				"CivilianSedan_Engine_Offload_Ext_Rpm3_SoundSet",
				"CivilianSedan_Engine_Offload_Ext_Rpm4_SoundSet",
				"CivilianSedan_Engine_Offload_Ext_Rpm5_SoundSet",
				"CivilianSedan_Engine_Ext_Rpm1_SoundSet",
				"CivilianSedan_Engine_Ext_Rpm2_SoundSet",
				"CivilianSedan_Engine_Ext_Rpm3_SoundSet",
				"CivilianSedan_Engine_Ext_Rpm4_SoundSet",
				"CivilianSedan_Engine_Ext_Rpm5_SoundSet"
			};
			engineStartSound="CivilianSedan_engine_start_SoundSet";
			engineFailSound="CivilianSedan_engine_failed_start_fuel_SoundSet";
			engineStopSound="CivilianSedan_engine_stop_SoundSet";

			// on the ground. Standing still at idle with the pedals centred, the
			// brakes hold it (there is no parking gear to shift into). Rudder at a
			// crawl also brakes the main wheel on that side, as a Cub pilot's toe
			// brakes do, for the tight turns
			parkHoldSpeed=0.5;		// m/s
			diffBrake=0.6;			// share of full brake on the inside wheel at full rudder
			diffBrakeFade=25;		// km/h by which it is gone
			// the tailwheel's swing narrows as the speed builds: full to 15 km/h,
			// down to tailSteerMin of it by 15 + tailSteerFade
			tailSteerFade=50;		// km/h
			tailSteerMin=0.15;

			/*
				The wheels (GebsPlaneWheel): each casts a ray down from its hub, a
				spring and damper hold the plane up off the hit, and the tyre grips
				up to what its load allows, sideways and along, with the brakes.
				hub: memory point at the wheel's centre as modelled, sitting on the
				ground. slot: the wheel item fitted there, whose radius and tyre are
				used; none, and it is the plane's own. A spring settles by load /
				stiffness, travelDown of it from the modelled height: set stiffness
				to about load / travelDown and the wheels sit where they are drawn.
				steer: degrees at full right rudder (+ points it right). brakeSide:
				which rudder pedal also brakes it (differential braking).
			*/
			class Wheels {
				class MainLeft {
					hub="wheel_1_2_hub";
					slot="GebsPA18_Wheel_1_2";
					radius=0.215;
					stiffness=62000;		// about 3.1 kN on each main wheel
					compression=3800;		// N per m/s, damping as it compresses
					rebound=5200;			// and as it extends
					// the bump stop from 12 cm in (travelUp + travelDown): before the
					// wheel's collision block, 12.6 cm up at rest and 4 cm nearer than
					// that to the physics (see Tail), lands on the ground. At 0.1 the
					// block met the ground first, rigid, on a hard landing
					travelUp=0.07;
					travelDown=0.05;
					brakeForce=2400;		// N at the tyre, full brake: weak drums; braked hard it still noses over
					grip=0.85;				// tyre friction on the ground
					lateralGrip=1;
					rollResistance=0.012;
					steer=0;
					brakeSide=-1;			// left
					spinAnim="wheelbackleft";
					damperAnim="damperbackleft";
					damperMin=-0.04;		// the model.cfg offsets that damper animation spans
					damperMax=0.1;
				};
				class MainRight: MainLeft {
					hub="wheel_2_2_hub";
					slot="GebsPA18_Wheel_2_2";
					brakeSide=1;
					spinAnim="wheelbackright";
					damperAnim="damperbackright";
				};
				// a small solid tailwheel on a leaf spring, steered by the pedals;
				// behind the CG, so right rudder points it left. Its sideways grip is
				// what keeps a taildragger running straight.
				// It carries about 830 N at rest. The engine's physics meets the
				// Geometry LOD about 4 cm out from its faces (Bullet's collision
				// margin), so the collision block round the hub (6 cm) is on the
				// ground well before it looks it. The physics wheel is larger than the
				// drawn one (0.087), so the tail rides with that block about 9 cm up,
				// and the bump stop (past travelUp + travelDown, 5 cm in) takes over
				// before the block would touch: the spring holds about 4 kN, five
				// times its rest load, first. The drawn tailwheel floats about 6 cm.
				// Cole's tests, 2026-11-04: radius 0.087, 25000, 0.03 up, damping
				// 900/1200 - the tail stood on the block (the spring held 150 of the
				// 830 N), bounced off it and the plane went over on its nose. Radius
				// 0.12, 0.06 up - clear at rest, but with only ~1.5 cm of spring
				// before the block; on a rough field it still hit it (1357 N on the
				// spring a second before the nose dropped): "still bouncy"
				class Tail {
					hub="tailwheel_hub";
					slot="";
					radius=0.15;
					stiffness=40000;		// about 2 cm of it at rest
					compression=2400;		// near critical with the tail's ~115 kg about the main wheels
					rebound=3000;
					travelUp=0.03;
					travelDown=0.02;
					brakeForce=0;
					grip=0.9;
					lateralGrip=1.5;
					rollResistance=0.015;
					steer=-35;
					brakeSide=0;
					spinAnim="wheelfrontleft";
					steerAnim="turnfrontleft";
				};
			};

			/*
				The lifting surfaces, each at its own memory point (its quarter
				chord). The wing is four panels so that rolling, and a stall that
				starts on one side, come out of the airflow at each one. The flaps
				are on the inner panels and the ailerons on the outer, as on the
				real wing. USA 35B section: zero lift at -3 deg, Cm -0.08.
			*/
			class Surfaces {
				class WingLeftInner {
					point="aero_wing_l_in";
					type="horizontal";
					area=3.3;
					aspect=6.9;		// the whole wing's
					chord=1.59;
					incidence=1.5;
					alpha0=-3;
					stall=15;
					stallNeg=-12;
					cd0=0.012;
					oswald=0.75;
					cm0=-0.08;
					flapCL=1.1;
					flapCD=0.1;
					flapCm=-0.12;
					slipstream=0.3;
					wing=1;
				};
				class WingRightInner: WingLeftInner {
					point="aero_wing_r_in";
				};
				class WingLeftOuter: WingLeftInner {
					point="aero_wing_l_out";
					area=5.0;
					flapCL=0;
					flapCD=0;
					flapCm=0;
					slipstream=0;
					control="aileronLeft";
					controlGain=8;
				};
				class WingRightOuter: WingLeftOuter {
					point="aero_wing_r_out";
					control="aileronRight";
				};
				class Tailplane {
					point="aero_htail";
					type="horizontal";
					area=2.9;
					aspect=2.8;
					chord=1.1;
					incidence=-0.5;
					alpha0=0;
					stall=16;
					stallNeg=-16;
					cd0=0.012;
					oswald=0.7;
					control="elevator";
					controlGain=14;
					trimGain=4;
					downwash=1;
					slipstream=0.8;
				};
				class Fin {
					point="aero_vtail";
					type="vertical";
					area=1.3;
					aspect=1.6;
					chord=0.9;
					incidence=0;
					alpha0=0;
					stall=18;
					stallNeg=-18;
					cd0=0.012;
					oswald=0.7;
					control="rudder";
					controlGain=15;
					slipstream=0.8;
					swirl=1;
				};
			};
		};
	};

	class GebsPA18_Yellow: GebsPA18_ColorBase {
		scope=2;
		displayName="$STR_GebsPlanes_PA18_Yellow0";
	};
	/*
		Camouflage: the same plane in other paint (tools/paint_pa18.py --scheme):
		winter white with grey patches, desert sand with brown; no stripe or
		registration. Only the painted faces' colour sheets change.
	*/
	class GebsPA18_Winter: GebsPA18_ColorBase {
		scope=2;
		displayName="$STR_GebsPlanes_PA18_Winter0";
		hiddenSelectionsTextures[]= {
			"gebsaircraft\data\pa18\data\gebspa18_body_winter_co.paa",
			"gebsaircraft\data\pa18\data\gebspa18_wing_winter_co.paa"
		};
	};
	class GebsPA18_Desert: GebsPA18_ColorBase {
		scope=2;
		displayName="$STR_GebsPlanes_PA18_Desert0";
		hiddenSelectionsTextures[]= {
			"gebsaircraft\data\pa18\data\gebspa18_body_desert_co.paa",
			"gebsaircraft\data\pa18\data\gebspa18_wing_desert_co.paa"
		};
	};

	/*
		The Super Cub on floats: the landplane without its landing gear, on
		twin floats on struts from the gear's fittings (tools/pa18_floats.py,
		built by tools/build_pa18.py), and flown arcade: the Arcade class below
		turns the plane at the rates the stick asks for and flies it straight
		and level hands off. The floats wear the camo_floats selection, which a
		livery repaints with the plane.
	*/
	class GebsPA18F_ColorBase: GebsPA18_ColorBase {
		scope=0;
		displayName="$STR_GebsPlanes_PA18F_0";
		descriptionShort="$STR_GebsPlanes_PA18F_1";
		model="\gebsaircraft\data\pa18\gebspa18f.p3d";
		// no wheels to fit
		attachments[]= {};
		class GUIInventoryAttachmentsProps {};
		hiddenSelections[]= {
			"camo",
			"camo_wing",
			"camo_floats"
		};
		hiddenSelectionsTextures[]= {
			"gebsaircraft\data\pa18\data\gebspa18_body_co.paa",
			"gebsaircraft\data\pa18\data\gebspa18_wing_co.paa",
			"gebsaircraft\data\pa18\data\gebspa18_floats_co.paa"
		};
		hiddenSelectionsMaterials[]= {
			"gebsaircraft\data\pa18\data\gebspa18_body_paint.rvmat",
			"gebsaircraft\data\pa18\data\gebspa18_wing_paint.rvmat",
			"gebsaircraft\data\pa18\data\gebspa18_floats.rvmat"
		};

		class GebsFlight: GebsFlight {
			// the floats' own drag is theirs (Floats, below); the struts and spreader bars here, the gear gone
			bodyCdaForward=0.42;
			// a 180 hp Lycoming O-360 conversion, as many float Cubs have: a livelier run off the water
			staticThrust=2800;
			ratedPower=134000;
			fuelBurnFull=40;
			// flown arcade: no torque rolling it or slipstream corkscrew swinging it on the run
			torque=0;
			swirl=0;
			// a swimmer climbs in from the water, below the door
			boardHeight=3.5;
			// its own wheels: none (an empty class, so the landplane's aren't inherited)
			class Wheels {};
			// the water's sounds, from vanilla's boat (the Bf 109 W's sets, data/bf109/config.cpp)
			waterSounds[]= {
				"GebsBF109W_Water_Light_SoundSet",
				"GebsBF109W_Water_Strong_SoundSet"
			};
			splashSound="boat_01_splash_SoundSet";

			/*
				The floats, as the Bf 109 W's (data/bf109/config.cpp tells what
				each number does), sized for the Cub's: 5.7 m long, their hump
				lower and earlier.
			*/
			class Floats {
				waterDensity=1000;
				planing=0.5;
				skinFriction=0.0032;
				humpSpeed=7;			// m/s
				humpDrag=0.15;
				sideDrag=0.8;
				heaveDamping=1100;
				rudderArea=0.05;		// m2 each
				rudderAngle=30;
				rudderLift=2.5;
				rudderStall=28;
				rudderFade=35;			// km/h
				airCdaForward=0.05;
				airCdaSide=0.9;
				airCdaUp=1.6;
				slamLoad=5;
				stepPull=0.5;			// not used flown arcade: the assist keeps it off the stall itself
				rudderDepth=0.22;
				ruinedBuoyancy=0.35;

				// --- floats: begin ---
				// written by tools/build_pa18.py from tools/pa18_floats.py: each float's slices, bow to stern,
				// each at the keel's memory point in its middle: its length, the half beam at its chines, the
				// vee's depth and the deck's height above the keel (m), the bottom's slope (deg, + rising to the bow)
				class Left {
					slices[]= {"float_l_00", "float_l_01", "float_l_02", "float_l_03", "float_l_04", "float_l_05", "float_l_06", "float_l_07", "float_l_08", "float_l_09", "float_l_10", "float_l_11"};
					length[]= {0.542, 0.542, 0.542, 0.542, 0.542, 0.542, 0.408, 0.408, 0.408, 0.408, 0.408, 0.408};
					halfBeam[]= {0.145, 0.257, 0.308, 0.320, 0.320, 0.320, 0.313, 0.288, 0.254, 0.215, 0.170, 0.122};
					chine[]= {0.076, 0.177, 0.206, 0.194, 0.176, 0.158, 0.146, 0.134, 0.119, 0.100, 0.079, 0.057};
					deck[]= {0.168, 0.392, 0.519, 0.550, 0.550, 0.550, 0.479, 0.414, 0.347, 0.279, 0.210, 0.141};
					slope[]= {19.837, 13.057, 5.885, 0.000, 0.000, 0.000, -8.000, -8.000, -8.000, -8.000, -8.000, -8.000};
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

			/*
				Arcade flying (gebsplane.c SimulateArcade). The stick and pedals ask
				for turn rates and the assist turns the plane at them, taking over
				the air's own turning moments while the wings still lift and drag:
				the nose goes where it's pointed and the plane follows it round.
				- let go of the stick, the wings roll level (levelRoll per second of
				  the bank, at most levelRollMax deg/s) and a climb or a dive eases
				  level (levelPitch, levelPitchMax): hands off, straight and level;
				- banked, the nose follows the turn round and keeps up, and it turns
				  into a sideslip by itself (slipGain): no rudder needed in a turn;
				- the stick banks no further than bankLimit;
				- it can't stall: stallMargin short of the stall the nose comes down,
				  so a held pull at low speed mushes along, wings level;
				- it flies the plane from speedMin km/h, fully from speedFull; slower,
				  the floats have it. On the water the pedals turn it at any speed
				  (waterYawRate deg/s) and hold it straight with them centred.
				Rates in deg/s, rateGain per second, maxAccel rad/s2.
			*/
			class Arcade {
				enabled=1;
				rollRate=70;
				pitchRate=30;
				yawRate=20;
				waterYawRate=35;
				levelRoll=1.5;
				levelRollMax=45;
				levelPitch=1.5;
				levelPitchMax=20;
				bankLimit=60;
				rateGain=5;
				maxAccel=3;
				slipGain=2;
				stallMargin=3;
				stallGain=3;
				speedMin=25;
				speedFull=55;
				takeOver=1;
				// afloat: the floats' planing attitude it holds them at, and the most a held pull tips them to
				waterPitch=6;
				waterPitchMax=8;
				waterPitchGain=2;
				// flying: a held pull climbs no steeper than climbMax deg from climbSpeed km/h, less and less
				// slower, none by climbSpeedMin, so it settles into a steady climb; dives no steeper than diveMax
				climbMax=30;
				climbSpeed=120;
				climbSpeedMin=80;
				diveMax=40;
				climbGain=1.5;
			};
		};
	};

	class GebsPA18F_Yellow: GebsPA18F_ColorBase {
		scope=2;
		displayName="$STR_GebsPlanes_PA18F_Yellow0";
	};
	// the floats painted to match (tools/paint_floats.py --plane pa18f --scheme)
	class GebsPA18F_Winter: GebsPA18F_ColorBase {
		scope=2;
		displayName="$STR_GebsPlanes_PA18F_Winter0";
		hiddenSelectionsTextures[]= {
			"gebsaircraft\data\pa18\data\gebspa18_body_winter_co.paa",
			"gebsaircraft\data\pa18\data\gebspa18_wing_winter_co.paa",
			"gebsaircraft\data\pa18\data\gebspa18_floats_winter_co.paa"
		};
	};
	class GebsPA18F_Desert: GebsPA18F_ColorBase {
		scope=2;
		displayName="$STR_GebsPlanes_PA18F_Desert0";
		hiddenSelectionsTextures[]= {
			"gebsaircraft\data\pa18\data\gebspa18_body_desert_co.paa",
			"gebsaircraft\data\pa18\data\gebspa18_wing_desert_co.paa",
			"gebsaircraft\data\pa18\data\gebspa18_floats_desert_co.paa"
		};
	};
};

/*
	The door. A Cub's is a light frame on a latch: a car door slam was far too
	loud for it, and the click of a combination lock grated. This is the Gunter's
	door, the lightest car door in the game, at a third of vanilla's volume and
	heard only close by.
*/
class CfgSoundShaders {
	class baseVehicles_SoundShader;
	class GebsPA18_door_open_SoundShader: baseVehicles_SoundShader {
		samples[]= {{"\DZ\sounds\vehicles\Hatchback\door_open",1}};
		volume=0.1;
		range=25;
	};
	class GebsPA18_door_close_SoundShader: baseVehicles_SoundShader {
		samples[]= {{"\DZ\sounds\vehicles\Hatchback\door_close",1}};
		volume=0.18;
		range=25;
	};
};

class CfgSoundSets {
	class baseVehicles_SoundSet;
	class GebsPA18_door_open_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsPA18_door_open_SoundShader"};
	};
	class GebsPA18_door_close_SoundSet: baseVehicles_SoundSet {
		soundShaders[]= {"GebsPA18_door_close_SoundShader"};
		// as it shuts, near the end of the door's 0.8 s swing
		startDelay=0.6;
	};
};
