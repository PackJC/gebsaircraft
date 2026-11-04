/*
	The crew of a plane.

	What they can do from the seat - start and stop the engine, work the door,
	eject - are the player's actions, as vanilla's start-engine and car door actions
	are.

	Getting out: which side a player climbs out is the seat animation's, and
	the pilot's is a car driver's, who gets out to the left; the plane's door
	can be on the other side. When someone's time in a plane ends, put them at
	its door (GebsPlane.PlaceAtDoor). Done on the server and on the player's own
	machine, the same moment on both, so the move doesn't come back as a
	network correction. Not when they leave unconscious or dead: those stay
	where the game puts them.
*/
modded class PlayerBase
{
	protected GebsPlane m_GebsPlaneRidden;

	override void SetActions(out TInputActionMap InputActionMap)
	{
		super.SetActions(InputActionMap);

		AddAction(GebsActionStartPlaneEngine, InputActionMap);
		AddAction(GebsActionStopPlaneEngine, InputActionMap);
		AddAction(GebsActionOpenPlaneDoorInside, InputActionMap);
		AddAction(GebsActionClosePlaneDoorInside, InputActionMap);
		AddAction(GebsActionEjectPlane, InputActionMap);
	}

	//! every step in a vehicle: remember whether it is a plane
	override bool ModCommandHandlerInside(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
	{
		if (pCurrentCommandID == DayZPlayerConstants.COMMANDID_VEHICLE)
		{
			HumanCommandVehicle vehCommand = GetCommand_Vehicle();
			if (vehCommand)
				m_GebsPlaneRidden = GebsPlane.Cast(vehCommand.GetTransport());
		}

		return super.ModCommandHandlerInside(pDt, pCurrentCommandID, pCurrentCommandFinished);
	}

	override void OnCommandVehicleFinish()
	{
		super.OnCommandVehicleFinish();

		GebsPlane plane = m_GebsPlaneRidden;
		m_GebsPlaneRidden = null;

		if (!plane || !IsAlive() || IsUnconscious() || m_ShouldBeUnconscious)
			return;

		if (GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_REMOTE)
			return;

		plane.PlaceAtDoor(this);
	}
};
