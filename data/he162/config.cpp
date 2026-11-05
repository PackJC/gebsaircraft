/* Heinkel He 162 A-2, BMW 003E-1 turbojet. See integration-contract.md.
   Flight coefficients are a first simulation approximation pending DayZ flight tests. */
class CfgPatches {
    class GebsPlanes_He162 {
        units[]={"GebsHe162A2","GebsHe162_Wheel","GebsHe162_NoseWheel"};
        weapons[]={};
        requiredVersion=0.1;
        requiredAddons[]={"GebsPlanes","GebsPlanes_Data","DZ_Data","DZ_Vehicles_Wheeled","DZ_Sounds_Effects"};
    };
};
class CfgSlots {
    class Slot_GebsHe162_Wheel_1_2 {
        name="GebsHe162_Wheel_1_2"; displayName="Left main wheel";
        selection="wheel_1_2"; ghostIcon="set:dayz_inventory image:wheel";
    };
    class Slot_GebsHe162_Wheel_2_2 {
        name="GebsHe162_Wheel_2_2"; displayName="Right main wheel";
        selection="wheel_2_2"; ghostIcon="set:dayz_inventory image:wheel";
    };
    class Slot_GebsHe162_Wheel_1_1 {
        name="GebsHe162_Wheel_1_1"; displayName="Nose wheel";
        selection="wheel_1_1"; ghostIcon="set:dayz_inventory image:wheel";
    };
};
class CfgNonAIVehicles {
    class ProxyAttachment;
    class ProxyGebsHe162_Wheel: ProxyAttachment {
        scope=2; inventorySlot[]={"GebsHe162_Wheel_1_2","GebsHe162_Wheel_2_2"};
        model="\gebsaircraft\data\he162\proxy\gebshe162_wheel.p3d";
    };
    class ProxyGebsHe162_NoseWheel: ProxyAttachment {
        scope=2; inventorySlot[]={"GebsHe162_Wheel_1_1"};
        model="\gebsaircraft\data\he162\proxy\gebshe162_nosewheel.p3d";
    };
};
class CfgVehicles {
    class CarWheel;
    class GebsPlane_Base;
    class GebsHe162_Wheel: CarWheel {
        scope=2; displayName="He 162 main wheel";
        descriptionShort="Main landing wheel for the Heinkel He 162.";
        model="\gebsaircraft\data\he162\proxy\gebshe162_wheel.p3d";
        weight=16000; inventorySlot[]={"GebsHe162_Wheel_1_2","GebsHe162_Wheel_2_2"};
        rotationFlags=4; repairableWithKits[]={6}; repairCosts[]={30};
        radiusByDamage[]={0,0.3274,0.3,0.285,0.9998,0.245,0.9999,0.19};
        radius=0.3274; width=0.2152;
        tyreType=0; tyreOffroadResistance=0.85; tyreRollResistance=0.015;
        tyreGrip=0.85; tyreRoughness=1; tyreLongitudinalFriction=0.9; tyreLateralFriction=0.9;
    };
    class GebsHe162_Wheel_Ruined: GebsHe162_Wheel {
        radius=0.21; width=0.14; tyreOffroadResistance=0.1;
        tyreRollResistance=0.3; tyreGrip=0.4; tyreLongitudinalFriction=0.4; tyreLateralFriction=0.4;
    };
    class GebsHe162_NoseWheel: GebsHe162_Wheel {
        displayName="He 162 nose wheel";
        descriptionShort="Nose landing wheel for the Heinkel He 162.";
        model="\gebsaircraft\data\he162\proxy\gebshe162_nosewheel.p3d";
        weight=9000; inventorySlot[]={"GebsHe162_Wheel_1_1"};
        radiusByDamage[]={0,0.188,0.3,0.165,0.9998,0.14,0.9999,0.115};
        radius=0.188; width=0.1322;
    };
    class GebsHe162_NoseWheel_Ruined: GebsHe162_NoseWheel {
        radius=0.14; width=0.10; tyreOffroadResistance=0.1;
        tyreRollResistance=0.3; tyreGrip=0.4; tyreLongitudinalFriction=0.4; tyreLateralFriction=0.4;
    };
    class GebsHe162_Base: GebsPlane_Base {
        scope=0; displayName="Heinkel He 162 A-2";
        descriptionShort="Single-seat He 162 with a BMW 003 turbojet, retractable tricycle gear and working cockpit.";
        model="\gebsaircraft\data\he162\gebshe162.p3d";
        fuelCapacity=695;
        attachments[]={"GebsHe162_Wheel_1_2","GebsHe162_Wheel_2_2","GebsHe162_Wheel_1_1"};
        class GUIInventoryAttachmentsProps {
            class Chassis {
                name="$STR_attachment_Chassis0"; description="";
                icon="set:dayz_inventory image:cat_vehicle_chassis";
                attachmentSlots[]={"GebsHe162_Wheel_1_2","GebsHe162_Wheel_2_2","GebsHe162_Wheel_1_1"};
            };
        };
        hiddenSelections[]={"camo","camo_wing"};
        hiddenSelectionsTextures[]={"gebsaircraft\data\he162\data\gebshe162_body_co.paa","gebsaircraft\data\he162\data\gebshe162_wing_co.paa"};
        hiddenSelectionsMaterials[]={"gebsaircraft\data\he162\data\gebshe162_body_paint.rvmat","gebsaircraft\data\he162\data\gebshe162_wing_paint.rvmat"};
        class Crew {
            class Driver {
                actionSel="seat_driver"; proxyPos="crewDriver";
                getInPos="pos_driver"; getInDir="pos_driver_dir"; isDriver=1;
            };
        };
        class AnimationSources {
            class DoorsDriver { source="user"; initPhase=0; animPeriod=1.4; };
            class elevator { source="user"; initPhase=0.5; animPeriod=0.0001; };
            class aileron: elevator {}; class rudder: elevator {}; class trim: elevator {};
            class stick_pitch: elevator {}; class stick_roll: elevator {}; class pedals: elevator {};
            class flaps { source="user"; initPhase=0; animPeriod=0.0001; };
            class gear: flaps {}; class radiator: flaps {}; class throttle: flaps {};
            class flapwheel: flaps {}; class gearlever: flaps {};
            // These harmless common sources keep the shared visual updater complete.
            class propeller: flaps {}; class prop_blur: flaps {};
            class prop_still { source="user"; initPhase=1; animPeriod=0.0001; };
            class lamp_down_off: prop_still {}; class lamp_up_off: prop_still {};
            class jet_exhaust_off: prop_still {};
            class wheelbackleft: flaps {}; class wheelbackright: flaps {}; class wheelfrontleft: flaps {};
            class turnfrontleft: elevator {};
            class gauge_ias: flaps {};
            class gauge_alt_m: flaps {};
            class gauge_alt_km: flaps {};
            class gauge_vario: flaps {};
            class gauge_heading: flaps {};
            class gauge_rpm: flaps {};
            class gauge_fuel: flaps {};
            class gauge_egt: flaps {};
            class gauge_pressure: flaps {};
        };
        class DamageSystem {
            class GlobalHealth { class Health { hitpoints=2500; healthLevels[]={{1,{}},{0.7,{}},{0.5,{}},{0.3,{}},{0,{}}}; }; };
            class DamageZones {
                class Chassis {
                    displayName="$STR_CfgVehicleDmg_Chassis0"; fatalInjuryCoef=-1;
                    componentNames[]={"dmgZone_chassis"}; inventorySlots[]={};
                    class Health { hitpoints=3500; transferToGlobalCoef=0.5; healthLevels[]={{1,{}},{0.7,{}},{0.5,{}},{0.3,{}},{0,{}}}; };
                };
                class Engine {
                    displayName="$STR_CfgVehicleDmg_Engine0"; fatalInjuryCoef=0.001;
                    memoryPoints[]={"dmgZone_engine"}; componentNames[]={"dmgZone_engine"}; inventorySlots[]={};
                    class Health { hitpoints=1500; transferToGlobalCoef=1; healthLevels[]={{1,{}},{0.7,{}},{0.5,{}},{0.3,{}},{0,{}}}; };
                };
                class FuelTank {
                    displayName="$STR_CfgVehicleDmg_FuelTank0"; fatalInjuryCoef=-1;
                    componentNames[]={"dmgZone_fuelTank"}; inventorySlots[]={};
                    class Health { hitpoints=800; transferToGlobalCoef=0; healthLevels[]={{1,{}},{0.7,{}},{0.5,{}},{0.3,{}},{0,{}}}; };
                };
            };
        };
        class GebsFlight {
            // Axis points retain the shared names, but are on the jet's thrust line.
            propHub="thrust_origin"; propFront="thrust_front"; bodyPoint="aero_body";
            doorPoint="pos_door"; boardPoint="pos_cockpit"; boardReach=3.5; boardHeight=3;
            doorSource="DoorsDriver"; doorOpenSound="GebsHe162_canopy_open_SoundSet";
            doorCloseSound="GebsHe162_canopy_close_SoundSet"; spawnFuel=0.5;
            crashImpulse=9000; crashDamage=0.06;
            cockpit=1; trimAnim=0; stallMargin=1.5; pullG=6;
            // The shared governed state carries shaft rpm; subclass supplies jet thrust.
            // Gameplay tuning for DayZ's short airfields (real BMW 003: about 7,845 N). With the
            // thrust put through the centre of gravity, more flap lift and a stronger elevator,
            // it rotates at about 130 km/h and leaves the ground in roughly 400 m instead of
            // being pinned on its nose wheel by the high-mounted engine.
            thrustAtCom=1;
            propType="constantSpeed"; staticThrust=11500; ratedPower=0;
            criticalAltitude=0; propEfficiency=0.8; propDiameter=0.69;
            propDragArea=0; propInertia=0; propRotation=0; gearRatio=1;
            idlePower=0.05; powerLag=1.8; rpmIdle=3500; rpmMax=9500; rpmLag=1.2;
            rpmPerSpeed=2; rpmOverspeed=10200; boostIdle=0; boostMax=0; soundRpmScale=1;
            slipstream=0; swirl=0; torque=0; aileronRig=0;
            fuelBurnIdle=180; fuelBurnFull=1550;
            starterRpm=2800; starterWindTime=0; catchTime=9; crankGiveUp=3;
            bodyCdaForward=0.12; bodyCdaSide=3.5; bodyCdaUp=4.2; gearCda=0.20;
            span=7.20; downwash=0.137; groundEffect=1; windScale=0;
            flapNotches=2; flapRate=0.16; flapDegrees=45;
            slatAlpha=0; slatFull=1; slatStall=0; slatCL=0; slatTime=0.3; debugLog=0;
            soundPoint="sound_engine";
            engineSounds[]={"GebsHe162_Engine_Idle_SoundSet","GebsHe162_Engine_Low_SoundSet","GebsHe162_Engine_Mid_SoundSet","GebsHe162_Engine_High_SoundSet","GebsHe162_Engine_Idle_Int_SoundSet","GebsHe162_Engine_Low_Int_SoundSet","GebsHe162_Engine_Mid_Int_SoundSet","GebsHe162_Engine_High_Int_SoundSet"};
            airSounds[]={"GebsHe162_Wind_SoundSet"};
            engineStartSound="GebsHe162_starter_SoundSet"; engineCatchSound="GebsHe162_catch_SoundSet";
            engineFailSound="GebsHe162_starter_fail_SoundSet"; engineStopSound="GebsHe162_stop_SoundSet";
            gearMoveSound="GebsHe162_gear_move_SoundSet"; gearLockSound="GebsHe162_gear_lock_SoundSet";
            flapSound="GebsHe162_flaps_SoundSet";
            gearUpTime=6; gearDownTime=5; gearNeedsEngine=1;
            parkHoldSpeed=0.5; diffBrake=0.7; diffBrakeFade=40; tailSteerFade=45; tailSteerMin=0.1;
            class Wheels {
                class MainLeft {
                    hub="wheel_1_2_hub"; slot="GebsHe162_Wheel_1_2"; radius=0.3274;
                    stiffness=200000; compression=14500; rebound=18500;
                    travelUp=0.14; travelDown=0.06; brakeForce=6500;
                    grip=0.9; lateralGrip=1; slipPeak=8; rollResistance=0.015;
                    steer=0; brakeSide=-1; retracts=1; spinAnim="wheelbackleft";
                };
                class MainRight: MainLeft {
                    hub="wheel_2_2_hub"; slot="GebsHe162_Wheel_2_2";
                    brakeSide=1; spinAnim="wheelbackright";
                };
                class Nose {
                    hub="wheel_1_1_hub"; slot="GebsHe162_Wheel_1_1"; radius=0.188;
                    stiffness=110000; compression=6000; rebound=8000;
                    travelUp=0.12; travelDown=0.04; brakeForce=0;
                    grip=0.85; lateralGrip=1.2; slipPeak=8; rollResistance=0.02;
                    steer=25; brakeSide=0; retracts=1;
                    spinAnim="wheelfrontleft"; steerAnim="turnfrontleft";
                };
            };
            class Surfaces {
                class WingLeftInner {
                    point="aero_wing_l_in"; type="horizontal";
                    area=3.6; aspect=4.65; chord=1.72; incidence=3; alpha0=-1.5;
                    stall=15; stallNeg=-13; cd0=0.009; oswald=0.8; cm0=-0.025; dihedral=3;
                    flapCL=1.1; flapCD=0.10; flapCm=-0.04; slipstream=0; wing=1;
                };
                class WingRightInner: WingLeftInner { point="aero_wing_r_in"; };
                class WingLeftOuter: WingLeftInner {
                    point="aero_wing_l_out"; area=1.98; chord=1.25;
                    incidence=2; stall=14.5; flapCL=0; flapCD=0; flapCm=0;
                    control="aileronLeft"; controlGain=6;
                };
                class WingRightOuter: WingLeftOuter { point="aero_wing_r_out"; control="aileronRight"; };
                class Tailplane {
                    point="aero_htail"; type="horizontal";
                    area=2.3; aspect=3.7; chord=0.72; incidence=1; alpha0=0;
                    stall=17; stallNeg=-17; cd0=0.008; oswald=0.75;
                    control="elevator"; controlGain=18; trimGain=4; downwash=1; slipstream=0;
                };
                class FinLeft {
                    point="aero_vtail_l"; type="vertical";
                    area=0.65; aspect=1.4; chord=0.7; incidence=0; alpha0=0;
                    stall=18; stallNeg=-18; cd0=0.008; oswald=0.7;
                    control="rudder"; controlGain=16; slipstream=0; swirl=0;
                };
                class FinRight: FinLeft { point="aero_vtail_r"; };
            };
            class Gauges {
                class ias { source="gauge_ias"; value="ias"; min=0; max=1000; wrap=0; lag=0.25; };
                class alt_m { source="gauge_alt_m"; value="altitude"; min=0; max=1000; wrap=1; lag=0; };
                class alt_km { source="gauge_alt_km"; value="altitude"; min=0; max=20000; wrap=0; lag=0.3; };
                class vario { source="gauge_vario"; value="vario"; min=-30; max=30; wrap=0; lag=0.4; };
                class heading { source="gauge_heading"; value="heading"; min=0; max=360; wrap=1; lag=0; };
                class rpm { source="gauge_rpm"; value="rpm"; min=0; max=12000; wrap=0; lag=0.3; };
                class fuel { source="gauge_fuel"; value="fuel"; min=0; max=700; wrap=0; lag=2; };
                class egt { source="gauge_egt"; value="egt"; min=0; max=1000; wrap=0; lag=0.7; };
                class pressure { source="gauge_pressure"; value="oilPressure"; min=0; max=10; wrap=0; lag=0.4; };
            };
        };
    };
    class GebsHe162A2: GebsHe162_Base { scope=2; };
};

// Procedural jet loops; mechanical samples reuse the project's own sounds.
class CfgSoundShaders {
    class GebsHe162_Engine_Idle_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_engine_idle",1}};
        frequency="rpm / 3500";
        volume="engineOn * campos * (1 - (rpm factor [4300,5100])) * (0.7 + 0.3 * thrust)"; range=1400;
    };
    class GebsHe162_Engine_Idle_Int_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_engine_idle_int",1}};
        frequency="rpm / 3500";
        volume="engineOn * (1 - campos) * (1 - (rpm factor [4300,5100])) * (0.7 + 0.3 * thrust)"; range=1400;
    };
    class GebsHe162_Engine_Low_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_engine_low",1}};
        frequency="rpm / 5500";
        volume="engineOn * campos * (rpm factor [4300,5100]) * (1 - (rpm factor [6200,7000])) * (0.7 + 0.3 * thrust)"; range=1400;
    };
    class GebsHe162_Engine_Low_Int_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_engine_low_int",1}};
        frequency="rpm / 5500";
        volume="engineOn * (1 - campos) * (rpm factor [4300,5100]) * (1 - (rpm factor [6200,7000])) * (0.7 + 0.3 * thrust)"; range=1400;
    };
    class GebsHe162_Engine_Mid_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_engine_mid",1}};
        frequency="rpm / 7500";
        volume="engineOn * campos * (rpm factor [6200,7000]) * (1 - (rpm factor [8200,9000])) * (0.7 + 0.3 * thrust)"; range=1400;
    };
    class GebsHe162_Engine_Mid_Int_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_engine_mid_int",1}};
        frequency="rpm / 7500";
        volume="engineOn * (1 - campos) * (rpm factor [6200,7000]) * (1 - (rpm factor [8200,9000])) * (0.7 + 0.3 * thrust)"; range=1400;
    };
    class GebsHe162_Engine_High_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_engine_high",1}};
        frequency="rpm / 9500";
        volume="engineOn * campos * (rpm factor [8200,9000]) * (0.7 + 0.3 * thrust)"; range=1400;
    };
    class GebsHe162_Engine_High_Int_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_engine_high_int",1}};
        frequency="rpm / 9500";
        volume="engineOn * (1 - campos) * (rpm factor [8200,9000]) * (0.7 + 0.3 * thrust)"; range=1400;
    };
    class GebsHe162_Wind_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_wind",1}};
        frequency="0.75 + 0.5 * (speed factor [40,850])";
        volume="(speed factor [30,650]) * (0.55 + 0.45 * campos) * (1 - 0.4 * (1 - campos) * (1 - doors))";
        range=250;
    };
    class GebsHe162_starter_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_starter",1}};
        volume=0.8; range=300;
    };
    class GebsHe162_starter_fail_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_starter_fail",1}};
        volume=0.8; range=300;
    };
    class GebsHe162_catch_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_catch",1}};
        volume=0.8; range=700;
    };
    class GebsHe162_stop_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_stop",1}};
        volume=0.8; range=600;
    };
    class GebsHe162_gear_move_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_gear_move",1}};
        volume=0.8; range=60;
    };
    class GebsHe162_gear_lock_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_gear_lock",1}};
        volume=0.8; range=60;
    };
    class GebsHe162_flaps_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_flaps",1}};
        volume=0.8; range=15;
    };
    class GebsHe162_canopy_open_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_canopy_open",1}};
        volume=0.8; range=25;
    };
    class GebsHe162_canopy_close_SoundShader {
        samples[]={{"gebsaircraft\data\he162\sounds\gebshe162_canopy_close",1}};
        volume=0.8; range=25;
    };
};
class CfgSoundSets {
    class baseVehicles_SoundSet;
    class GebsHe162_Engine_Idle_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_Engine_Idle_SoundShader"}; };
    class GebsHe162_Engine_Idle_Int_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_Engine_Idle_Int_SoundShader"}; };
    class GebsHe162_Engine_Low_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_Engine_Low_SoundShader"}; };
    class GebsHe162_Engine_Low_Int_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_Engine_Low_Int_SoundShader"}; };
    class GebsHe162_Engine_Mid_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_Engine_Mid_SoundShader"}; };
    class GebsHe162_Engine_Mid_Int_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_Engine_Mid_Int_SoundShader"}; };
    class GebsHe162_Engine_High_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_Engine_High_SoundShader"}; };
    class GebsHe162_Engine_High_Int_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_Engine_High_Int_SoundShader"}; };
    class GebsHe162_Wind_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_Wind_SoundShader"}; };
    class GebsHe162_starter_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_starter_SoundShader"}; };
    class GebsHe162_starter_fail_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_starter_fail_SoundShader"}; };
    class GebsHe162_catch_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_catch_SoundShader"}; };
    class GebsHe162_stop_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_stop_SoundShader"}; };
    class GebsHe162_gear_move_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_gear_move_SoundShader"}; };
    class GebsHe162_gear_lock_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_gear_lock_SoundShader"}; };
    class GebsHe162_flaps_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_flaps_SoundShader"}; };
    class GebsHe162_canopy_open_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_canopy_open_SoundShader"}; };
    class GebsHe162_canopy_close_SoundSet: baseVehicles_SoundSet { soundShaders[]={"GebsHe162_canopy_close_SoundShader"}; };
};
