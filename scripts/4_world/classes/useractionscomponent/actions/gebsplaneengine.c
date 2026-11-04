/*
	Starting and stopping a plane's engine, from the pilot's seat.

	The engine is the plane's own (GebsPlane.SimulateEngine), and so are its
	switches. These actions throw them on the server and on the pilot's
	client both (OnStart, OnFinishProgress, OnEnd and OnExecute run on both),
	as vanilla's engine actions do for a vehicle without moves, and as Alien
	Invasion's UFO powers up: so starting never waits on a network move.

	Start is held, as in a car. The seated start animation ends after about a
	second whatever the action asks for, so the action is as short as a car's
	start (UATimeSpent.START_ENGINE); held that long, the starter latches and
	the engine turns over and fires by itself - after an inertia starter's
	flywheel has wound up, on the Bf 109 - or, with no fuel or a ruined engine,
	never: the starter runs down, and the script log says why. Let go sooner,
	and it stops cranking.
*/
class GebsActionStartPlaneEngineCB : ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(UATimeSpent.START_ENGINE);
	}
};

//! The plane the player is flying, or null.
class GebsPlaneActions
{
	static GebsPlane GetPilotedPlane(PlayerBase player)
	{
		if (!player)
			return null;

		HumanCommandVehicle vehCommand = player.GetCommand_Vehicle();
		if (!vehCommand)
			return null;

		GebsPlane plane = GebsPlane.Cast(vehCommand.GetTransport());
		if (!plane || plane.CrewDriver() != player)
			return null;

		return plane;
	}
};

class GebsActionStartPlaneEngine : ActionContinuousBase
{
	void GebsActionStartPlaneEngine()
	{
		m_CallbackClass = GebsActionStartPlaneEngineCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_STARTENGINE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ALL;
		m_LockTargetOnUse = false;
		m_Text = "#STR_GebsPlanes_StartEngine";
	}

	override void CreateConditionComponents()
	{
		m_ConditionTarget = new CCTNone();
		m_ConditionItem = new CCINone();
	}

	//! Not while the engine runs, cranks, or turns over by itself after a start.
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		GebsPlane plane = GebsPlaneActions.GetPilotedPlane(player);
		return plane && !plane.IsStarting();
	}

	//! Re-checked every step, the start condition would cut the action off as
	//! soon as the engine began to crank, let go of the starter and start again
	//! while the button is held, round and round (the UFO's first test)
	override bool ActionConditionContinue(ActionData action_data)
	{
		return GebsPlaneActions.GetPilotedPlane(action_data.m_Player) != null;
	}

	override void OnStart(ActionData action_data)
	{
		super.OnStart(action_data);

		GebsPlane plane = GebsPlaneActions.GetPilotedPlane(action_data.m_Player);
		if (plane)
			plane.HoldStarter(true);
	}

	//! Held to the end: the engine turns over and fires by itself from here.
	override void OnFinishProgress(ActionData action_data)
	{
		super.OnFinishProgress(action_data);

		GebsPlane plane = GebsPlaneActions.GetPilotedPlane(action_data.m_Player);
		if (plane)
			plane.LatchStarter();
	}

	override void OnEnd(ActionData action_data)
	{
		super.OnEnd(action_data);

		GebsPlane plane = GebsPlaneActions.GetPilotedPlane(action_data.m_Player);
		if (!plane)
			return;

		plane.HoldStarter(false);
		if (!plane.IsStarting() || !g_Game.IsServer())
			return;

		string why = plane.WhyEngineWontRun();
		if (why != "")
			Print(string.Format("GebsPlanes: %1 won't start:%2 (health level %3, engine %4, fuel %5 L)", plane.GetType(), why, plane.GetHealthLevel(""), plane.GetHealthLevel("Engine"), plane.GetFuel()));
	}

	override bool CanBeUsedInVehicle()
	{
		return true;
	}

	override bool HasTarget()
	{
		return false;
	}

	override bool UseMainItem()
	{
		return false;
	}
};

class GebsActionStopPlaneEngine : ActionSingleUseBase
{
	void GebsActionStopPlaneEngine()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_STOPENGINE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ALL;
		m_Text = "#stop_engine";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone();
		m_ConditionTarget = new CCTNone();
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		GebsPlane plane = GebsPlaneActions.GetPilotedPlane(player);
		return plane && plane.IsStarting();
	}

	//! On the server and the pilot's client both, as starting.
	override void OnExecute(ActionData action_data)
	{
		super.OnExecute(action_data);

		GebsPlane plane = GebsPlaneActions.GetPilotedPlane(action_data.m_Player);
		if (plane)
			plane.SwitchOff();
	}

	override bool CanBeUsedInVehicle()
	{
		return true;
	}

	override bool HasTarget()
	{
		return false;
	}

	override bool UseMainItem()
	{
		return false;
	}
};
