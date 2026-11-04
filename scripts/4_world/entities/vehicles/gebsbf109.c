/*
	Messerschmitt Bf 109 E-3, and the Bf 109 W on floats.

	A single-seat fighter on a narrow-track, retractable undercarriage: the
	pilot sits under a canopy hinged on its right, and climbs in and out over
	the left wing root. How it flies and rolls is all in data/bf109/config.cpp,
	class GebsFlight; this class only says how the pilot sits and where the
	camera goes. The W is the same airframe, its gear folded away for good, on
	twin floats (GebsFlight Floats).
*/
class GebsBF109_Base : GebsPlane
{
	// the Gunter 2's seating: low, legs stretched out ahead under the panel, as
	// in the 109's cockpit, where the pilot sits almost on the floor
	override int GetAnimInstance()
	{
		return VehicleAnimInstances.GOLF;
	}

	// The seat's animation decides which side you climb out: a car's driver gets
	// out to the left, and the 109 is boarded from the left wing root anyway. The
	// pilot's seat has to keep the driver's animation: the game hands the
	// controls and the HUD only to a driver's seat.
	override int GetSeatAnimationType(int posIdx)
	{
		return DayZPlayerConstants.VEHICLESEAT_DRIVER;
	}

	// 9.9 m of wing: stand well back
	override float GetTransportCameraDistance()
	{
		return 10.0;
	}

	override vector GetTransportCameraOffset()
	{
		return "0 1.5 0";
	}
};

class GebsBF109E3 : GebsBF109_Base
{
	override void OnDebugSpawn()
	{
		super.OnDebugSpawn();

		GetInventory().CreateInInventory("GebsBF109_Wheel");
		GetInventory().CreateInInventory("GebsBF109_Wheel");
	}
};

class GebsBF109E3_Winter : GebsBF109E3 {};
class GebsBF109E3_Desert : GebsBF109E3 {};

//! On floats: no wheels to fit.
class GebsBF109W : GebsBF109_Base {};
class GebsBF109W_Winter : GebsBF109W {};
class GebsBF109W_Desert : GebsBF109W {};
