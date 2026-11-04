/*
	The plane's instrument panel on the HUD, in place of a car's speedometer.

	Vanilla's HUD keeps one vehicle HUD per vehicle type (GetVehicleType) and
	shows it to whoever takes the driver's seat; a plane's type is
	"VehicleTypeGebsPlane", and this is its HUD: airspeed, altitude and height
	above the ground, climb, throttle and engine rpm (and the manifold pressure
	for a plane with a boost gauge), fuel, flaps and trim, the gear for a plane
	whose gear folds, and the engine and brakes.
*/
class GebsPlaneHud : VehicleHudBase
{
	protected GebsPlane m_Plane;
	protected TextWidget m_Speed;
	protected TextWidget m_Altitude;
	protected TextWidget m_Climb;
	protected TextWidget m_Power;
	protected TextWidget m_Fuel;
	protected TextWidget m_Setup;
	protected TextWidget m_State;
	protected float m_ClimbShown;		//!< m/s, smoothed

	override void Init(Widget vehicleHudPanels)
	{
		super.Init(vehicleHudPanels);

		m_VehiclePanel = g_Game.GetWorkspace().CreateWidgets("gebsaircraft/data/gui/gebsplanehud.layout", vehicleHudPanels);
		if (!m_VehiclePanel)
			return;

		m_Speed = TextWidget.Cast(m_VehiclePanel.FindAnyWidget("Speed"));
		m_Altitude = TextWidget.Cast(m_VehiclePanel.FindAnyWidget("Altitude"));
		m_Climb = TextWidget.Cast(m_VehiclePanel.FindAnyWidget("Climb"));
		m_Power = TextWidget.Cast(m_VehiclePanel.FindAnyWidget("Power"));
		m_Fuel = TextWidget.Cast(m_VehiclePanel.FindAnyWidget("Fuel"));
		m_Setup = TextWidget.Cast(m_VehiclePanel.FindAnyWidget("Setup"));
		m_State = TextWidget.Cast(m_VehiclePanel.FindAnyWidget("State"));
	}

	override void ShowVehicleInfo(PlayerBase player)
	{
		m_Plane = null;
		m_ClimbShown = 0;
		if (player && player.GetCommand_Vehicle())
			m_Plane = GebsPlane.Cast(player.GetCommand_Vehicle().GetTransport());
	}

	override void HideVehicleInfo()
	{
		m_Plane = null;
	}

	protected static string Whole(float value)
	{
		float rounded = Math.Round(value);
		return rounded.ToString();
	}

	protected static string Tenths(float value)
	{
		float rounded = Math.Round(value * 10) / 10;
		string text = rounded.ToString();
		if (text.IndexOf(".") < 0)
			text += ".0";
		return text;
	}

	protected static string Hundredths(float value)
	{
		float rounded = Math.Round(value * 100) / 100;
		string text = rounded.ToString();
		if (text.IndexOf(".") < 0)
			text += ".00";
		else if (text.IndexOf(".") == text.Length() - 2)
			text += "0";
		return text;
	}

	override void RefreshVehicleHud(float timeslice)
	{
		if (!m_Plane || !m_VehiclePanel || !m_Speed)
			return;

		vector position = m_Plane.GetPosition();
		float ground = g_Game.SurfaceY(position[0], position[2]);
		float climb = GetVelocity(m_Plane)[1];
		m_ClimbShown += (climb - m_ClimbShown) * Math.Clamp(timeslice * 3, 0, 1);

		m_Speed.SetText(Whole(Math.Max(m_Plane.GetAirspeed(), 0) * 3.6) + " km/h");
		m_Altitude.SetText("ALT " + Whole(position[1]) + " m   AGL " + Whole(Math.Max(position[1] - ground, 0)) + " m");

		string climbText = Tenths(m_ClimbShown);
		if (m_ClimbShown >= 0.05)
			climbText = "+" + climbText;
		m_Climb.SetText("CLIMB " + climbText + " m/s");

		string power = "THROTTLE " + Whole(m_Plane.GetThrottle() * 100) + "%   " + Whole(m_Plane.GetEngineRpm()) + " RPM";
		float boost = m_Plane.GetBoost();
		if (boost > 0)
			power += "   " + Hundredths(boost) + " ATA";
		m_Power.SetText(power);
		m_Fuel.SetText("FUEL " + Whole(m_Plane.GetFuel()) + " / " + Whole(m_Plane.GetFuelCapacity()) + " L");

		string flaps = "UP";
		float flapSetting = m_Plane.GetFlaps();
		float flapDegrees = m_Plane.GetFlapDegrees();
		if (flapDegrees > 0)
		{
			if (flapSetting > 0.01)
				flaps = Whole(flapSetting * flapDegrees) + "°";
		}
		else if (flapSetting > 0.75)
		{
			flaps = "FULL";
		}
		else if (flapSetting > 0.25)
		{
			flaps = "HALF";
		}
		string trim = Whole(m_Plane.GetTrim() * 100);
		if (m_Plane.GetTrim() > 0.005)
			trim = "+" + trim;
		string setup = "FLAPS " + flaps + "   TRIM " + trim;
		if (m_Plane.HasRetractableGear())
		{
			float gear = m_Plane.GetGearPhase();
			string gearText = "MOVING";
			if (gear < 0.02)
				gearText = "DOWN";
			else if (gear > 0.98)
				gearText = "UP";
			setup += "   GEAR " + gearText;
		}
		m_Setup.SetText(setup);

		string state = "ENGINE OFF";
		switch (m_Plane.GetEngineState())
		{
			case GebsEngineState.CRANKING:
				state = "STARTING";
				break;
			case GebsEngineState.RUNNING:
				state = "ENGINE ON";
				break;
		}
		if (m_Plane.IsBraking())
			state += "   BRAKES";
		m_State.SetText(state);
	}
};

//! Hand the HUD its plane panel, alongside vanilla's car, boat and motorbike ones.
modded class IngameHud
{
	override void Init(Widget hud_panel_widget)
	{
		super.Init(hud_panel_widget);

		GebsPlaneHud planeHud = new GebsPlaneHud();
		m_VehicleHudMap.Insert("VehicleTypeGebsPlane", planeHud);
		planeHud.Init(m_VehicleHudPanels);
		planeHud.HidePanel();
	}
};
