/*
	Piper PA-18 Super Cub.

	A two-seat tandem taildragger: the pilot sits in front, the passenger behind,
	and the one door is on the right. How it flies and rolls is all in
	data/config.cpp, class GebsFlight; this class only says how the crew sits
	and where the camera goes.
*/
class GebsPA18_ColorBase : GebsPlane
{
	// the Gunter 2's seating: low, legs stretched out ahead, as in a Cub's cockpit
	// (the Niva's sits too tall for it, and puts the legs through the floor)
	override int GetAnimInstance()
	{
		return VehicleAnimInstances.GOLF;
	}

	// The seat's animation decides which side you climb out: a car's driver gets
	// out to the left. The pilot's seat has to keep the driver's animation all the
	// same - the game hands the controls and the HUD only to a driver's seat -
	// so GebsPlane.PlaceAtDoor moves the pilot to the door on the right once out.
	// The passenger behind takes the right-hand passenger's, which gets out right.
	override int GetSeatAnimationType(int posIdx)
	{
		switch (posIdx)
		{
			case 0:
				return DayZPlayerConstants.VEHICLESEAT_DRIVER;
			case 1:
				return DayZPlayerConstants.VEHICLESEAT_PASSENGER_R;
		}

		return 0;
	}

	// 10.7 m of wing: stand well back
	override float GetTransportCameraDistance()
	{
		return 9.0;
	}

	override vector GetTransportCameraOffset()
	{
		return "0 1.6 0";
	}

	override void OnDebugSpawn()
	{
		super.OnDebugSpawn();

		// the float plane has no wheels to fit
		if (HasFloats())
			return;

		GetInventory().CreateInInventory("GebsPA18_Wheel");
		GetInventory().CreateInInventory("GebsPA18_Wheel");
	}
};

class GebsPA18_Yellow : GebsPA18_ColorBase {};
class GebsPA18_Winter : GebsPA18_ColorBase {};
class GebsPA18_Desert : GebsPA18_ColorBase {};

//! The Super Cub on floats, flown arcade (data/config.cpp, GebsPA18F_ColorBase).
class GebsPA18F_ColorBase : GebsPA18_ColorBase {};
class GebsPA18F_Yellow : GebsPA18F_ColorBase {};
class GebsPA18F_Winter : GebsPA18F_ColorBase {};
class GebsPA18F_Desert : GebsPA18F_ColorBase {};
