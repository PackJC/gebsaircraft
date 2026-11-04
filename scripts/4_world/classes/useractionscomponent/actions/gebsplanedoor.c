/*
	Opening and closing a plane's cabin door, from outside and from the seat.

	The door is an animation on the plane, not a door item, and a plane is not
	a car, so vanilla's car door actions never see it. Outside, these are the
	plane's own actions (GebsPlane.SetActions); from the seat they are the
	player's (gebsplanecrew.c).
*/
class GebsActionPlaneDoorBase : ActionInteractBase
{
	protected bool m_IsOpening = true;

	void GebsActionPlaneDoorBase()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_OPENDOORFW;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
		m_LockTargetOnUse = false;
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone();
		m_ConditionTarget = new CCTNone();
	}

	protected GebsPlane GetPlane(ActionTarget target)
	{
		if (!target)
			return null;

		GebsPlane plane = GebsPlane.Cast(target.GetObject());
		if (!plane)
			plane = GebsPlane.Cast(target.GetParent());

		return plane;
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		// from the seat, the inside actions below
		if (player.GetCommand_Vehicle())
			return false;

		GebsPlane plane = GetPlane(target);
		if (!plane || plane.GetDoorAnimSource() == "")
			return false;

		if (!plane.IsInBoardingReach(player.GetPosition()))
			return false;

		return plane.IsDoorOpen() != m_IsOpening;
	}

	override void OnStartServer(ActionData action_data)
	{
		super.OnStartServer(action_data);

		SwingDoor(GetPlane(action_data.m_Target));
	}

	protected void SwingDoor(GebsPlane plane)
	{
		if (!plane)
			return;

		float phase = 0;
		if (m_IsOpening)
			phase = 1;

		plane.SetAnimationPhase(plane.GetDoorAnimSource(), phase);
	}
};

class GebsActionOpenPlaneDoor : GebsActionPlaneDoorBase
{
	void GebsActionOpenPlaneDoor()
	{
		m_IsOpening = true;
		m_Text = "#open";
	}
};

class GebsActionClosePlaneDoor : GebsActionPlaneDoorBase
{
	void GebsActionClosePlaneDoor()
	{
		m_IsOpening = false;
		m_Text = "#close";
	}
};

/*!
	From either seat: a Cub's door is in reach of both. Seated, the player
	plays a car's seated door animation - a standing one never finishes in a
	seat, and the action hangs with everything else behind it. Like vanilla's
	car door actions, it takes the driver's for even seats and the co-driver's
	for odd ones.
*/
class GebsActionPlaneDoorInsideBase : GebsActionPlaneDoorBase
{
	protected int m_SeatCommandEven;
	protected int m_SeatCommandOdd;

	void GebsActionPlaneDoorInsideBase()
	{
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ALL;
	}

	protected GebsPlane GetRiddenPlane(PlayerBase player)
	{
		HumanCommandVehicle vehCommand = player.GetCommand_Vehicle();
		if (!vehCommand || vehCommand.IsGettingIn() || vehCommand.IsGettingOut())
			return null;

		return GebsPlane.Cast(vehCommand.GetTransport());
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		GebsPlane plane = GetRiddenPlane(player);
		if (!plane || plane.GetDoorAnimSource() == "")
			return false;

		int crewIdx = plane.CrewMemberIndex(player);
		if (crewIdx < 0)
			return false;

		if (crewIdx % 2 == 0)
			m_CommandUID = m_SeatCommandEven;
		else
			m_CommandUID = m_SeatCommandOdd;

		return plane.IsDoorOpen() != m_IsOpening;
	}

	override void OnStartServer(ActionData action_data)
	{
		// the outside action's swing finds no plane at a target from the seat
		super.OnStartServer(action_data);

		SwingDoor(GetRiddenPlane(action_data.m_Player));
	}

	override bool CanBeUsedInVehicle()
	{
		return true;
	}

	override bool HasTarget()
	{
		return false;
	}
};

class GebsActionOpenPlaneDoorInside : GebsActionPlaneDoorInsideBase
{
	void GebsActionOpenPlaneDoorInside()
	{
		m_IsOpening = true;
		m_SeatCommandEven = DayZPlayerConstants.CMD_ACTIONMOD_DRIVER_DOOR_OPEN;
		m_SeatCommandOdd = DayZPlayerConstants.CMD_ACTIONMOD_CODRIVER_DOOROPEN;
		m_CommandUID = m_SeatCommandEven;
		m_Text = "#open";
	}
};

class GebsActionClosePlaneDoorInside : GebsActionPlaneDoorInsideBase
{
	void GebsActionClosePlaneDoorInside()
	{
		m_IsOpening = false;
		m_SeatCommandEven = DayZPlayerConstants.CMD_ACTIONMOD_DRIVER_DOOR_CLOSE;
		m_SeatCommandOdd = DayZPlayerConstants.CMD_ACTIONMOD_CODRIVER_DOORCLOSE;
		m_CommandUID = m_SeatCommandEven;
		m_Text = "#close";
	}
};

//! Every action class has to be registered before anything can use it.
modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);

		actions.Insert(GebsActionGetInPlane);
		actions.Insert(GebsActionOpenPlaneDoor);
		actions.Insert(GebsActionClosePlaneDoor);
		actions.Insert(GebsActionOpenPlaneDoorInside);
		actions.Insert(GebsActionClosePlaneDoorInside);
		actions.Insert(GebsActionEjectPlane);
		actions.Insert(GebsActionStartPlaneEngine);
		actions.Insert(GebsActionStopPlaneEngine);
	}
};
