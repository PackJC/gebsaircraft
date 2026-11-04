/*
	Climbing into a plane and out of it.

	Vanilla's get-in action wants the player at a car's door, looking at the
	seat's own block in the View Geometry, with the door already open. In a
	plane's cockpit that block hides behind the cockpit and canopy round it,
	so nothing offered to get in. This one takes a look at any part of the
	plane from anywhere within its boardReach of the cockpit
	(GebsPlane.IsInBoardingReach), puts the player in the seat they look at
	if it is free, or else the first free one, the pilot's first, and opens
	the door or canopy on the way in. Getting out is vanilla's, and the door
	opens for that too, but only at a standstill: moving, the crew ejects
	instead, through a door they have opened. A floatplane is boarded from the
	water as well, by a swimmer alongside.
*/
class GebsActionGetInPlane : ActionBase
{
	void GebsActionGetInPlane()
	{
		m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
		m_Text = "#get_in_vehicle";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone();
		m_ConditionTarget = new CCTNone();
	}

	override typename GetInputType()
	{
		return ContinuousInteractActionInput;
	}

	override bool HasProgress()
	{
		return false;
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
		if (player.GetCommand_Vehicle())
			return false;

		if (player.GetItemInHands() && player.GetItemInHands().IsHeavyBehaviour())
			return false;

		GebsPlane plane = GetPlane(target);
		if (!plane || !plane.IsInBoardingReach(player.GetPosition()))
			return false;

		// a swimmer climbs up into a floatplane only, from beside its float
		if (player.GetCommand_Swim() && !plane.HasFloats())
			return false;

		return plane.GetBoardingSeat(target.GetComponentIndex()) >= 0;
	}

	override void Start(ActionData action_data)
	{
		super.Start(action_data);

		GebsPlane plane = GetPlane(action_data.m_Target);
		if (!plane)
			return;

		int seatIdx = plane.GetBoardingSeat(action_data.m_Target.GetComponentIndex());
		if (seatIdx < 0)
			return;

		HumanCommandVehicle vehCommand = action_data.m_Player.StartCommand_Vehicle(plane, seatIdx, plane.GetSeatAnimationType(seatIdx));
		if (vehCommand)
		{
			vehCommand.SetVehicleType(plane.GetAnimInstance());

			GetDayZGame().GetBacklit().OnEnterCar();
		}
	}

	override void OnStartServer(ActionData action_data)
	{
		super.OnStartServer(action_data);

		GebsPlane plane = GetPlane(action_data.m_Target);
		if (plane)
			plane.OpenDoorForCrew();
	}

	override bool CanBeUsedInRestrain()
	{
		return true;
	}

	//! A floatplane is boarded from the water too: a swimmer alongside climbs in (ActionCondition: floatplanes only)
	override bool CanBeUsedSwimming()
	{
		return true;
	}

	override void OnUpdate(ActionData action_data)
	{
		if (action_data.m_State == UA_START)
		{
			if (!action_data.m_Player.GetCommand_Vehicle() || !action_data.m_Player.GetCommand_Vehicle().IsGettingIn())
				End(action_data);
		}
	}

	override int GetActionCategory()
	{
		return AC_INTERACT;
	}

	//! Holds the seat while the player climbs in, as vanilla's get-in does, by the seat Start takes.
	override bool AddActionJuncture(ActionData action_data)
	{
		GebsPlane plane = GetPlane(action_data.m_Target);
		if (!plane)
			return false;

		int seatIdx = plane.GetBoardingSeat(action_data.m_Target.GetComponentIndex());
		if (seatIdx < 0)
			return false;

		InventoryLocation il = new InventoryLocation;
		il.SetVehicle(plane, action_data.m_Player, seatIdx);
		if (!g_Game.AddInventoryJunctureEx(action_data.m_Player, action_data.m_Player, il, true, 10000, action_data))
			return false;

		action_data.m_ReservedInventoryLocations.Insert(il);
		return true;
	}
};

/*!
	Getting out of a plane: only while it is all but standing still
	(GebsPlane.IsTooFastToGetOut); moving, the crew can only eject, below. The
	door or canopy opens by itself, so nobody climbs out through the glass.
*/
modded class ActionGetOutTransport
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!super.ActionCondition(player, target, item))
			return false;

		HumanCommandVehicle vehCommand = player.GetCommand_Vehicle();
		if (!vehCommand)
			return true;

		GebsPlane plane = GebsPlane.Cast(vehCommand.GetTransport());
		return !plane || !plane.IsTooFastToGetOut();
	}

	override void OnStartServer(ActionData action_data)
	{
		HumanCommandVehicle vehCommand = action_data.m_Player.GetCommand_Vehicle();
		if (vehCommand)
		{
			GebsPlane plane = GebsPlane.Cast(vehCommand.GetTransport());
			if (plane)
				plane.OpenDoorForCrew();
		}

		super.OnStartServer(action_data);
	}
};

/*!
	Ejecting: leaving a plane that is moving, when Get out isn't offered, and
	only once the door or canopy is open (the seat's open-door action; it
	doesn't open by itself for this). The player jumps out as from a moving
	car and is put beside the door (PlayerBase.OnCommandVehicleFinish): in the
	air they fall from there, and from a plane rolling on the ground they are
	hurt as jumping out of a car (GebsPlane.OnCrewEjectedServer).
*/
class GebsActionEjectPlane : ActionGetOutTransport
{
	void GebsActionEjectPlane()
	{
		m_Text = "#STR_GebsPlanes_Eject";
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		HumanCommandVehicle vehCommand = player.GetCommand_Vehicle();
		if (!vehCommand || vehCommand.IsGettingIn() || vehCommand.IsGettingOut())
			return false;

		GebsPlane plane = GebsPlane.Cast(vehCommand.GetTransport());
		if (!plane || plane.CrewMemberIndex(player) < 0)
			return false;

		return plane.IsTooFastToGetOut() && plane.IsEjectWayOpen();
	}

	//! Always the jump out, at the plane's speed: vanilla counts a speed only for cars and boats.
	override void ProcessGetOutTransportActionData(Transport veh, GetOutTransportActionData got_action_data)
	{
		super.ProcessGetOutTransportActionData(veh, got_action_data);

		GebsPlane plane = GebsPlane.Cast(veh);
		if (!plane)
			return;

		got_action_data.m_JumpingOutThreshold = GebsPlane.CREW_EJECT_SPEED;
		got_action_data.m_Speed = plane.GetCrewExitSpeed();
		got_action_data.m_WasJumpingOut = true;
		// which animation is the player's own machine's choice, sent to the server with the action
		if (got_action_data.m_Player.IsOwner())
			got_action_data.m_WasJumpingOutAnim = true;
	}

	override void ApplyJumpOutDmg(ActionData action_data)
	{
		super.ApplyJumpOutDmg(action_data);

		GetOutTransportActionData gotActionData = GetOutTransportActionData.Cast(action_data);
		if (!gotActionData)
			return;

		GebsPlane plane = GebsPlane.Cast(gotActionData.m_Vehicle);
		if (plane)
			plane.OnCrewEjectedServer(gotActionData.m_Player, gotActionData.m_Speed);
	}
};
