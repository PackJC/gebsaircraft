/*
	gebsaircraft - fixed-wing flight for DayZ.

	DayZ has no aircraft. A GebsPlane is a bare Transport - no car underneath -
	and this script simulates all of it, every physics step:

	- its wheels (GebsPlaneWheel): each casts a ray to the ground, rides a
	  spring and damper, grips, brakes and, the tailwheel, steers. A plane with
	  retractable gear (the Bf 109) folds its legs away and back down;
	- a floatplane's floats (GebsPlaneFloats, the Bf 109 W): buoyancy, planing
	  on the step, the water's drag and grip, the water rudders;
	- its engine: off, cranking on the starter, running on its fuel, with a
	  fixed-pitch propeller (the Cub's) or a constant-speed one whose governor
	  holds the revs the throttle sets (the Bf 109's), and a supercharger that
	  keeps its power up to its rated altitude;
	- the air: the airflow over each wing panel and tail surface from that
	  point's own velocity - so rolling, pitching and yawing damp themselves,
	  sideslip weathervanes the nose, and a panel that stalls first drops and
	  can spin the plane - with lift and drag there; leading-edge slats that
	  open by themselves at high angles of attack and put the stall off; a
	  propeller with its thrust, torque, slipstream and gyroscopic kick.

	What a plane is made of lives in its config, class GebsFlight (numbers),
	and its model (memory points for every surface, wheel, the propeller and
	the door).

	Multiplayer: as with every DayZ vehicle, the pilot's own client simulates
	the plane, and the server simulates it too, from the same moves, and
	corrects the pilot's machine wherever the two part company - a jolt each
	time. So the simulation has to come out the same on both: the pilot's
	controls and gear lever ride in the network move (GebsPlaneMove); the
	engine's, propeller's, gear's and slats' state is in the owner state that a
	correction rewinds; every force goes in through Transport.ApplyForceAt, the
	engine's deterministic vehicle physics that a correction replays; and
	nothing reads what each machine keeps its own copy of, such as the
	weather's wind.

	The engine's switches are the exception, as Alien Invasion's UFO has them,
	whose foundation this is: the start and stop actions throw them on the
	server and on the pilot's machine alike, as vanilla's engine actions do for
	a vehicle without moves, so starting never waits on a move getting through.
	A seated start animation lasts about a second, so once the action has run
	its course the starter latches and the engine turns over and fires by
	itself.

	A body that has sat still falls asleep, and asleep it gets no simulation
	steps at all: nothing would start the engine, fly the plane or land it. So
	the plane keeps itself awake while someone is at the controls, the engine
	is on or it is still moving, and checks every frame, since the game puts it
	to sleep itself when the plane changes hands.
*/

//! What the pilot is doing. Everything the simulation depends on that is not
//! already in the physics state.
class GebsPlaneControls
{
	float m_Elevator;		//!< -1 stick forward (nose down) .. 1 stick back (nose up)
	float m_Aileron;		//!< -1 roll left .. 1 roll right
	float m_Rudder;			//!< -1 yaw left .. 1 yaw right
	float m_Throttle;		//!< 0 .. 1, the lever
	float m_Flaps;			//!< 0 .. 1, where the flaps are (they move slowly)
	float m_Trim;			//!< -1 nose down .. 1 nose up
	float m_Brake;			//!< 0 .. 1
	bool m_GearDown = true;	//!< the gear lever (planes with retractable gear)
	bool m_Ignition;		//!< the magnetos: on to run (thrown by the engine actions on each machine, not in the move)
	bool m_Starter;			//!< the starter, while the start action is held (likewise)
	bool m_StarterLatched;	//!< the start action ran its course: the engine turns over and fires by itself

	//! The pilot gone: stick and pedals centred, lever shut, brakes off.
	void ResetSticks()
	{
		m_Elevator = 0;
		m_Aileron = 0;
		m_Rudder = 0;
		m_Throttle = 0;
		m_Brake = 0;
	}

	void SwitchOff()
	{
		m_Ignition = false;
		m_Starter = false;
		m_StarterLatched = false;
	}
};

//! The vehicle's network move, with the pilot's controls added (the engine's switches are not in it).
class GebsPlaneMove : TransportMove
{
	float m_Elevator;
	float m_Aileron;
	float m_Rudder;
	float m_Throttle;
	float m_Flaps;
	float m_Trim;
	float m_Brake;
	bool m_GearDown;

	void Store(GebsPlaneControls c)
	{
		m_Elevator = c.m_Elevator;
		m_Aileron = c.m_Aileron;
		m_Rudder = c.m_Rudder;
		m_Throttle = c.m_Throttle;
		m_Flaps = c.m_Flaps;
		m_Trim = c.m_Trim;
		m_Brake = c.m_Brake;
		m_GearDown = c.m_GearDown;
	}

	void Restore(GebsPlaneControls c)
	{
		c.m_Elevator = m_Elevator;
		c.m_Aileron = m_Aileron;
		c.m_Rudder = m_Rudder;
		c.m_Throttle = m_Throttle;
		c.m_Flaps = m_Flaps;
		c.m_Trim = m_Trim;
		c.m_Brake = m_Brake;
		c.m_GearDown = m_GearDown;
	}

	protected override event void Write(PawnMoveWriter ctx, PawnMove prev)
	{
		super.Write(ctx, prev);

		ctx.Write(m_Elevator);
		ctx.Write(m_Aileron);
		ctx.Write(m_Rudder);
		ctx.Write(m_Throttle);
		ctx.Write(m_Flaps);
		ctx.Write(m_Trim);
		ctx.Write(m_Brake);
		ctx.Write(m_GearDown);
	}

	protected override event void Read(PawnMoveReader ctx, PawnMove prev)
	{
		super.Read(ctx, prev);

		ctx.Read(m_Elevator);
		ctx.Read(m_Aileron);
		ctx.Read(m_Rudder);
		ctx.Read(m_Throttle);
		ctx.Read(m_Flaps);
		ctx.Read(m_Trim);
		ctx.Read(m_Brake);
		ctx.Read(m_GearDown);
	}

	protected override event int EstimateMaximumSize()
	{
		// seven floats and a flag, with room for the serializer's own framing
		return super.EstimateMaximumSize() + 64;
	}
};

//! Simulation state a network correction has to rewind along with the body.
class GebsPlaneOwnerState : TransportOwnerState
{
	int m_EngineState;
	float m_CrankTime;
	float m_EnginePower;
	float m_WingCL;
	float m_PropRpm;
	float m_GearPhase;
	float m_SlatLeft;
	float m_SlatRight;

	protected override event void Write(PawnStateWriter ctx)
	{
		super.Write(ctx);

		ctx.Write(m_EngineState);
		ctx.Write(m_CrankTime);
		ctx.Write(m_EnginePower);
		ctx.Write(m_WingCL);
		ctx.Write(m_PropRpm);
		ctx.Write(m_GearPhase);
		ctx.Write(m_SlatLeft);
		ctx.Write(m_SlatRight);
	}

	protected override event void Read(PawnStateReader ctx)
	{
		super.Read(ctx);

		ctx.Read(m_EngineState);
		ctx.Read(m_CrankTime);
		ctx.Read(m_EnginePower);
		ctx.Read(m_WingCL);
		ctx.Read(m_PropRpm);
		ctx.Read(m_GearPhase);
		ctx.Read(m_SlatLeft);
		ctx.Read(m_SlatRight);
	}

	protected override event int EstimateMaximumSize()
	{
		return super.EstimateMaximumSize() + 56;
	}
};

enum GebsEngineState
{
	OFF,
	CRANKING,	//!< on the starter
	RUNNING
};

enum GebsAeroControl
{
	NONE,
	AILERON_LEFT,	//!< more lift for roll right
	AILERON_RIGHT,	//!< less lift for roll right
	ELEVATOR,		//!< less lift (tail down, nose up) for stick back; carries the trim too
	RUDDER			//!< pushes the tail left for right rudder
};

/*!
	One lifting surface: a wing panel, the tailplane, the fin. It acts at one
	point, its quarter chord, and gets its own airflow there.

	Below the stall the lift is the finite-wing lift slope from the aspect ratio
	(2 pi AR / (AR + 2)) and the drag is profile drag plus induced drag. Past the
	stall angle both blend over a few degrees into a flat plate's, which is what
	any wing becomes well beyond the stall and in reverse flow.
*/
class GebsAeroSurface
{
	static const float STALL_BLEND = 0.09;			//!< rad over which a stalled surface becomes a flat plate
	static const float FLAP_STALL_SHIFT = 0.035;	//!< rad: full flaps stall the panel this much earlier

	string m_Name;
	vector m_Point;			//!< model space
	vector m_Normal;		//!< model space: up for a horizontal surface, right for the fin
	vector m_Span;			//!< model space
	float m_Area;			//!< m2
	float m_Aspect;
	float m_Chord;			//!< m
	float m_Incidence;		//!< rad, to the fuselage datum
	float m_Alpha0;			//!< rad, angle of zero lift (negative for a cambered wing)
	float m_CLa;			//!< lift slope, per rad
	float m_StallPos;		//!< rad
	float m_StallNeg;		//!< rad
	float m_CD0;
	float m_Oswald;
	float m_Cm0;			//!< camber pitching moment coefficient
	int m_Control;			//!< GebsAeroControl
	float m_ControlGain;	//!< rad of effective angle for full control
	float m_TrimGain;		//!< rad for full trim
	float m_FlapCL;			//!< extra lift coefficient with full flaps
	float m_FlapCD;			//!< extra drag coefficient with full flaps
	float m_FlapCm;			//!< extra pitching moment coefficient with full flaps
	float m_Slipstream;		//!< share of the propeller's slipstream this surface sits in
	bool m_Swirl;			//!< whether the slipstream's corkscrew hits it (the fin)
	bool m_Downwash;		//!< whether it flies in the wing's downwash (the tailplane)
	bool m_Wing;			//!< a main wing panel: feeds the downwash, feels ground effect
	int m_SlatGroup;		//!< 0 none, 1 the left wing's slats, 2 the right's
	float m_Alpha;			//!< rad, its angle of attack last step (for the slats)

	static GebsAeroSurface Load(string path, string name, Object plane, vector up, vector side, vector fwd)
	{
		string point;
		g_Game.ConfigGetText(path + " point", point);
		if (point == "" || !plane.MemoryPointExists(point))
		{
			ErrorEx(string.Format("GebsPlanes: %1 has no memory point '%2' for surface %3", plane.GetType(), point, name));
			return null;
		}

		GebsAeroSurface s = new GebsAeroSurface();
		s.m_Name = name;
		s.m_Point = plane.GetMemoryPointPos(point);

		string type;
		g_Game.ConfigGetText(path + " type", type);
		if (type == "vertical")
		{
			s.m_Normal = side;
			s.m_Span = up;
		}
		else
		{
			// dihedral: the panel rises towards its tip, so its normal leans in towards the fuselage.
			// In a sideslip the wing on the windward side then meets the air harder and lifts, rolling the
			// plane away from the slip: what keeps a plane's wings level by itself
			float dihedral = g_Game.ConfigGetFloat(path + " dihedral") * Math.DEG2RAD;
			float outboard = 1;
			if (vector.Dot(s.m_Point, side) < 0)
				outboard = -1;
			s.m_Normal = up * Math.Cos(dihedral) - side * (outboard * Math.Sin(dihedral));
			s.m_Span = side * Math.Cos(dihedral) + up * (outboard * Math.Sin(dihedral));
		}

		s.m_Area = g_Game.ConfigGetFloat(path + " area");
		s.m_Aspect = Math.Max(g_Game.ConfigGetFloat(path + " aspect"), 0.5);
		s.m_Chord = g_Game.ConfigGetFloat(path + " chord");
		s.m_Incidence = g_Game.ConfigGetFloat(path + " incidence") * Math.DEG2RAD;
		s.m_Alpha0 = g_Game.ConfigGetFloat(path + " alpha0") * Math.DEG2RAD;
		s.m_CLa = 2 * Math.PI * s.m_Aspect / (s.m_Aspect + 2);
		s.m_StallPos = g_Game.ConfigGetFloat(path + " stall") * Math.DEG2RAD;
		s.m_StallNeg = g_Game.ConfigGetFloat(path + " stallNeg") * Math.DEG2RAD;
		s.m_CD0 = g_Game.ConfigGetFloat(path + " cd0");
		s.m_Oswald = Math.Max(g_Game.ConfigGetFloat(path + " oswald"), 0.3);
		s.m_Cm0 = g_Game.ConfigGetFloat(path + " cm0");
		s.m_ControlGain = g_Game.ConfigGetFloat(path + " controlGain") * Math.DEG2RAD;
		s.m_TrimGain = g_Game.ConfigGetFloat(path + " trimGain") * Math.DEG2RAD;
		s.m_FlapCL = g_Game.ConfigGetFloat(path + " flapCL");
		s.m_FlapCD = g_Game.ConfigGetFloat(path + " flapCD");
		s.m_FlapCm = g_Game.ConfigGetFloat(path + " flapCm");
		s.m_Slipstream = g_Game.ConfigGetFloat(path + " slipstream");
		s.m_Swirl = g_Game.ConfigGetInt(path + " swirl") != 0;
		s.m_Downwash = g_Game.ConfigGetInt(path + " downwash") != 0;
		s.m_Wing = g_Game.ConfigGetInt(path + " wing") != 0;

		string slats;
		g_Game.ConfigGetText(path + " slats", slats);
		if (slats == "left")
			s.m_SlatGroup = 1;
		else if (slats == "right")
			s.m_SlatGroup = 2;

		string control;
		g_Game.ConfigGetText(path + " control", control);
		switch (control)
		{
			case "aileronLeft":
				s.m_Control = GebsAeroControl.AILERON_LEFT;
				break;
			case "aileronRight":
				s.m_Control = GebsAeroControl.AILERON_RIGHT;
				break;
			case "elevator":
				s.m_Control = GebsAeroControl.ELEVATOR;
				break;
			case "rudder":
				s.m_Control = GebsAeroControl.RUDDER;
				break;
			default:
				s.m_Control = GebsAeroControl.NONE;
				break;
		}

		return s;
	}

	/*!
		Lift and drag coefficients at angle of attack alpha (rad, -pi..pi).
		inducedScale < 1 in ground effect; slatStall (rad) and slatCL are what
		the open slats add.
	*/
	void Coefficients(float alpha, float flaps, float inducedScale, float slatStall, float slatCL, out float cl, out float cd)
	{
		float stallPos = m_StallPos - flaps * FLAP_STALL_SHIFT + slatStall;

		float clLin = m_CLa * (alpha - m_Alpha0) + flaps * m_FlapCL + slatCL;
		float cdLin = m_CD0 + flaps * m_FlapCD + inducedScale * clLin * clLin / (Math.PI * m_Oswald * m_Aspect);

		float sinA = Math.Sin(alpha);
		float clPlate = 1.1 * Math.Sin(2 * alpha);
		float cdPlate = m_CD0 + flaps * m_FlapCD + 1.3 * sinA * sinA;

		float blend = 0;
		if (alpha > stallPos)
			blend = (alpha - stallPos) / STALL_BLEND;
		else if (alpha < m_StallNeg)
			blend = (m_StallNeg - alpha) / STALL_BLEND;
		blend = Math.Clamp(blend, 0, 1);
		blend = blend * blend * (3 - 2 * blend);

		cl = Math.Lerp(clLin, clPlate, blend);
		cd = Math.Lerp(cdLin, cdPlate, blend);
	}
};

/*!
	One gauge on the panel: a needle (an animation source of the model) and
	what it shows (GebsPlane.GaugeValue), from GebsFlight >> Gauges.
*/
class GebsPlaneGauge
{
	string m_Source;
	string m_Value;
	float m_Min;
	float m_Max;
	bool m_Wrap;		//!< goes round and round (an altimeter's hands, a clock's)
	float m_Shown;		//!< the phase last set
	float m_Lag;		//!< s for the needle to follow (0: at once)

	static GebsPlaneGauge Load(string path)
	{
		// read into locals first: an object's field as a config read's out parameter is a trap
		string source;
		string value;
		g_Game.ConfigGetText(path + " source", source);
		g_Game.ConfigGetText(path + " value", value);
		GebsPlaneGauge g = new GebsPlaneGauge();
		g.m_Source = source;
		g.m_Value = value;
		g.m_Min = g_Game.ConfigGetFloat(path + " min");
		g.m_Max = g_Game.ConfigGetFloat(path + " max");
		g.m_Wrap = g_Game.ConfigGetInt(path + " wrap") != 0;
		g.m_Lag = g_Game.ConfigGetFloat(path + " lag");
		g.m_Shown = -1;
		if (g.m_Source == "" || g.m_Value == "" || g.m_Max == g.m_Min)
			return null;

		return g;
	}

	float Phase(float value)
	{
		float t = (value - m_Min) / (m_Max - m_Min);
		if (m_Wrap)
			return t - Math.Floor(t);

		return Math.Clamp(t, 0, 1);
	}
};

/*!
	Base class for every plane. A plane's class only has to say how its crew
	sits; everything else comes from its config's GebsFlight class.
*/
class GebsPlane : Transport
{
	static const float CONTROL_RATE = 3.0;		//!< rudder travel per second towards where it is pushed
	static const float AILERON_RATE = 2.0;		//!< stick, side to side
	static const float ELEVATOR_RATE = 1.2;		//!< stick, fore and aft: on a key, a tap is a nudge, not a yank into the stall
	static const float CONTROL_RETURN = 4.0;	//!< and back to the middle when let go
	static const float ELEVATOR_RETURN = 1.5;	//!< the stick, fore and aft, let go: slower, so a pull let go doesn't drop the nose
	// a held pull's hand (PullRoom): tuned in tools/bf109_sim.py, where a tighter band or a longer look ahead made the stick saw
	static const float PULL_LEAD = 0.1;			//!< s: it reads the wing's angle of attack and the g this far ahead
	static const float PULL_BAND = 0.3;			//!< rad short of the stall margin over which the pull slows to a stop
	static const float PULL_BAND_G = 2.0;		//!< g short of the g limit likewise
	static const float PULL_BACK = 2.0;			//!< inside either, the stick comes forward at up to this many times ELEVATOR_RATE
	static const float ALPHA_RATE_TIME = 0.1;	//!< s the wing's angle-of-attack rate is smoothed over
	static const float ARCADE_STALL_BAND = 0.1;	//!< rad short of the arcade assist's stall margin over which a pull slows
	static const float ARCADE_BANK_BAND = 0.25;	//!< rad short of its bank limit over which the stick's roll slows
	static const float THROTTLE_RATE = 0.5;		//!< lever travel per second
	static const float TRIM_RATE = 0.25;
	static const float TAIL_STEER_FULL = 15;	//!< km/h up to which the tailwheel takes its full swing
	static const float SYNC_INTERVAL = 0.1;		//!< s between updates of what other players see
	static const float SYNC_STEP = 0.02;		//!< smallest change worth sending
	static const float FUEL_SYNC_STEP = 0.1;	//!< L
	static const float SEA_LEVEL_DENSITY = 1.225;
	static const float PROP_BLUR_RPM = 450;		//!< propeller rpm above which the blades give way to a blurred disc
	static const float RPM_TO_RAD = 0.10472;	//!< 2 pi / 60
	static const float AWAKE_SPEED = 0.5;		//!< m/s: still moving, so kept awake even with nobody aboard
	static const float GAUGE_RANGE_SQ = 400;	//!< (m) the panel's needles only move with a camera within 20 m
	static const int STORE_VERSION = 2;
	static const float BOARD_HEIGHT = 3.0;		//!< (m) climbing in from no further above or below the boarding point than this (GebsFlight boardHeight)
	static const float SPLASH_SINK = 1.5;		//!< m/s: a float coming down on the water faster than this splashes

	protected bool m_FlightLoaded;
	protected ref array<ref GebsAeroSurface> m_FlightSurfaces;
	protected vector m_FlightFwd;		//!< model axes
	protected vector m_FlightUp;
	protected vector m_FlightSide;		//!< to the right
	protected vector m_PropHub;
	protected vector m_TailPoint;		//!< for the turn rates: as far back as the tail
	protected vector m_BodyPoint;
	protected string m_DoorPoint;
	protected string m_DoorSource;
	protected string m_BoardPoint;		//!< climb in or work the door from within m_BoardReach of it
	protected float m_BoardReach;		//!< m, across the ground
	protected float m_BoardHeight;		//!< m above or below the boarding point

	// propeller and engine
	protected bool m_ConstantSpeed;		//!< a governed propeller (otherwise fixed pitch)
	protected float m_StaticThrust;		//!< N at full power, standing still, sea level
	protected float m_PropPitchSpeed;	//!< fixed pitch: m/s at which the propeller stops pulling
	protected float m_PropEfficiency;	//!< constant speed: of the shaft power, what becomes thrust at speed
	protected float m_PropDiameter;		//!< m
	protected float m_PropDragArea;		//!< m2 of drag from a dead propeller
	protected float m_PropInertia;		//!< kg m2: its spin's gyroscopic kick when the nose swings
	protected int m_PropRotation;		//!< 1 clockwise seen from the cockpit, -1 the other way
	protected float m_GearRatio;		//!< engine revs per propeller rev
	protected float m_RatedPower;		//!< W
	protected float m_CriticalAltitude;	//!< m up to which the supercharger holds full power (0: none)
	protected float m_IdlePower;		//!< share of full power at idle
	protected float m_PowerLag;			//!< s for the engine to answer the throttle
	protected float m_RpmIdle;			//!< engine rpm
	protected float m_RpmStatic;		//!< fixed pitch: full throttle, standing still
	protected float m_RpmMax;			//!< constant speed: the governor's rpm at full throttle
	protected float m_RpmLag;			//!< constant speed: s for the revs to follow
	protected float m_RpmPerSpeed;		//!< propeller rpm the windmilling airflow adds per m/s
	protected float m_RpmOverspeed;		//!< engine rpm above which the engine is damaged
	protected float m_StarterRpm;		//!< engine rpm on the starter
	protected float m_StarterWindTime;	//!< s an inertia starter's flywheel spins up before it engages
	protected float m_CatchTime;		//!< s from the start before it fires
	protected float m_CrankGiveUp;		//!< s more of cranking an engine that won't fire before the starter gives up
	protected float m_BoostIdle;		//!< ata, the manifold pressure gauge at idle
	protected float m_BoostMax;			//!< ata, at full throttle
	protected float m_SoundRpmScale;	//!< maps engine rpm onto the engine sound's rpm range
	protected float m_SlipstreamGain;	//!< share of the far-wake slipstream that reaches the tail
	protected float m_SwirlGain;		//!< sideways share of the slipstream at the fin
	protected float m_TorqueGain;		//!< share of the engine torque the airframe feels
	protected float m_FuelBurnIdle;		//!< L/h
	protected float m_FuelBurnFull;		//!< L/h

	// airframe
	protected float m_BodyCdaFwd;		//!< m2, drag area of fuselage, gear, struts head-on
	protected float m_BodyCdaSide;		//!< m2, side-on
	protected float m_BodyCdaUp;		//!< m2, top-on
	protected float m_GearCda;			//!< m2 more head-on while the gear is down
	protected float m_Span;				//!< m
	protected float m_DownwashGain;		//!< rad of downwash at the tail per unit wing lift coefficient
	protected bool m_GroundEffect;
	protected float m_WindScale;
	protected float m_FlapNotches;
	protected float m_FlapRate;			//!< flap travel per second
	protected float m_FlapDegrees;		//!< for the HUD: how far full flaps go down (0: UP / HALF / FULL)
	protected float m_SlatAlpha;		//!< rad: the slats start to open
	protected float m_SlatFull;			//!< rad: and are fully out
	protected float m_SlatStall;		//!< rad they put the stall off by
	protected float m_SlatCL;			//!< lift coefficient they add
	protected float m_SlatTime;			//!< s they take to move
	protected float m_CrashImpulse;		//!< N s a knock has to pass to do damage
	protected float m_CrashDamage;		//!< health per N s past it
	protected float m_AileronRig;		//!< the ailerons' fixed tab: what holds the wings level against the torque at cruise
	protected float m_PullMargin;		//!< rad: a held pull key stops this far short of the stall (0: no stop)
	protected float m_PullG;			//!< and short of this many g (0: no stop)

	// arcade flying (GebsFlight Arcade, SimulateArcade): the stick asks for turn rates and the plane flies itself
	protected bool m_Arcade;
	protected float m_ArcadeRollRate;		//!< rad/s rolled at full stick
	protected float m_ArcadePitchRate;		//!< rad/s pitched at full stick
	protected float m_ArcadeYawRate;		//!< rad/s yawed at full pedal, flying
	protected float m_ArcadeWaterYawRate;	//!< rad/s yawed at full pedal on the water at a taxi
	protected float m_ArcadeLevelRoll;		//!< 1/s: the stick let go, the wings roll level at this times the bank
	protected float m_ArcadeLevelRollMax;	//!< rad/s at most
	protected float m_ArcadeLevelPitch;		//!< 1/s: the stick let go, a climb or a dive eases level at this times its angle
	protected float m_ArcadeLevelPitchMax;	//!< rad/s at most
	protected float m_ArcadeBankLimit;		//!< rad: the stick banks no further (0: no limit)
	protected float m_ArcadeGain;			//!< 1/s: how hard the turn rates chase the ones asked for
	protected float m_ArcadeAccel;			//!< rad/s2 at most
	protected float m_ArcadeSlip;			//!< 1/s: the nose turns into a sideslip (the feet a pilot doesn't have to use)
	protected float m_ArcadeStallMargin;	//!< rad: the nose is held this far below the stall
	protected float m_ArcadeStallGain;		//!< 1/s: and comes down at this times how far it is inside that
	protected float m_ArcadeSpeedMin;		//!< m/s: below this the plane is left to itself
	protected float m_ArcadeSpeedFull;		//!< m/s: from this the assist flies it fully
	protected float m_ArcadeTakeOver;		//!< share of the air's own turning moments the assist takes over
	protected float m_ArcadeWaterPitch;		//!< rad: afloat, the attitude it holds the floats at with the stick let go
	protected float m_ArcadeWaterPitchMax;	//!< rad: and with it full back
	protected float m_ArcadeWaterPitchGain;	//!< 1/s: how fast it goes there
	protected float m_ArcadeClimbMax;		//!< rad: the steepest climb, from climbSpeed
	protected float m_ArcadeClimbSpeed;		//!< m/s
	protected float m_ArcadeClimbSpeedMin;	//!< m/s: no climb at all this slow
	protected float m_ArcadeDiveMax;		//!< rad: the steepest dive
	protected float m_ArcadeClimbGain;		//!< 1/s: how hard the climb is held to that

	// this step: where the centre of gravity is, and the air's turning moments about it (AirForceAt)
	protected vector m_AirCom;
	protected vector m_AirFwd;
	protected vector m_AirUp;
	protected vector m_AirSide;
	protected float m_AirPitch;			//!< N m, nose up +
	protected float m_AirYaw;			//!< N m, nose right +
	protected float m_AirRoll;			//!< N m, right wing down +

	// on the ground
	protected ref array<ref GebsPlaneWheel> m_Wheels;
	protected float m_ParkHoldSpeed;	//!< m/s
	protected float m_DiffBrake;
	protected float m_DiffBrakeFade;	//!< km/h
	protected float m_TailSteerFade;	//!< km/h
	protected float m_TailSteerMin;
	protected float m_GearUpTime;		//!< s to retract (0: fixed gear)
	protected float m_GearDownTime;		//!< s to extend
	protected bool m_GearNeedsEngine;	//!< hydraulic: up only with the engine running, down slower without it

	// on the water (GebsFlight Floats: a floatplane)
	protected ref GebsPlaneFloats m_FloatGear;
	protected bool m_Drowned;			//!< the propeller's hub went under with the engine on (the server damages it once)

	// fuel: the server's, sent to everyone
	protected float m_FuelCapacity;		//!< L
	protected float m_Fuel;				//!< L
	protected float m_FuelSent;
	protected float m_FuelToBurn;

	// state
	protected ref GebsPlaneControls m_FlightControls;
	protected int m_EngineState;		//!< GebsEngineState
	protected float m_CrankTime;
	protected float m_EnginePower;		//!< 0 .. 1, what the engine is giving
	protected float m_PropRpm;			//!< the propeller's (engine rpm / m_GearRatio)
	protected float m_PropTurns;		//!< for the spinning blades
	protected float m_WingCL;			//!< the wing's lift coefficient last step, for the downwash
	protected float m_Slipstream;		//!< m/s the propeller adds over the tail
	protected float m_GearPhase;		//!< 0 down and locked .. 1 up
	protected float m_SlatLeft;			//!< 0 shut .. 1 out
	protected float m_SlatRight;
	protected float m_StallMargin = 1;	//!< rad: how far below its stall the wing panel nearest it was, last step
	protected float m_LoadFactor = 1;	//!< g the lift was pulling, last step
	protected float m_LoadPerRad;		//!< and how many more g a radian more angle of attack would pull
	protected float m_WingAlpha;		//!< rad, the wing's average angle of attack last step
	protected float m_AlphaRate;		//!< rad/s it is rising at, smoothed
	protected bool m_HasWingAlpha;
	protected float m_GroundLoad;		//!< N the retracting wheels carry, this step
	protected vector m_SimTm[4];		//!< the physics body's world transform this step (SimPoint, SimDir)
	protected float m_SyncTimer;
	protected bool m_KeptAwake;			//!< the body held awake here (UpdateSleep)
	protected bool m_TookOver;			//!< this client is the pilot and has taken the plane over (TakeOver)

	// pilot input
	protected int m_FlapNotch;
	protected bool m_FlapsDownHeld;
	protected bool m_FlapsUpHeld;
	protected bool m_GearKeyHeld;
	protected ref map<string, UAInput> m_FlightInputs;

	// what other players see and hear (server -> clients)
	protected float m_NetElevator;
	protected float m_NetAileron;
	protected float m_NetRudder;
	protected float m_NetFlaps;
	protected float m_NetTrim;
	protected float m_NetThrottle;
	protected float m_NetPower;
	protected float m_NetRpm;			//!< engine rpm, 0 .. 4000
	protected float m_NetGear;
	protected bool m_NetGearDown = true;
	protected bool m_NetSwitch;			//!< the ignition (GebsPlaneControls.m_Ignition)
	protected float m_NetSlatLeft;
	protected float m_NetSlatRight;
	protected int m_NetEngineState;

	// the cockpit (clients)
	protected bool m_HasCockpit;		//!< stick, pedals, throttle, flap wheel and gear lever move (GebsFlight cockpit=1)
	protected bool m_HasTrimAnim;		//!< the tailplane itself turns with the trim (the Bf 109's)
	protected ref array<ref GebsPlaneGauge> m_Gauges;
	protected float m_Vario;			//!< m/s, smoothed as a variometer's capsule does
	protected float m_TurnRate;			//!< deg/s, smoothed
	protected float m_Slip;				//!< the slip ball, -1 .. 1
	protected float m_OilTemp = 15;		//!< deg C, warming up and cooling down here only (they are for show)
	protected float m_CoolantTemp = 15;
	protected vector m_LastVelocity;

	// sound (clients)
	protected vector m_SoundPos;
	protected ref TStringArray m_EngineSoundSets;
	protected ref TStringArray m_AirSoundSets;
	protected string m_EngineStartSound;
	protected string m_EngineCatchSound;
	protected string m_EngineFailSound;
	protected string m_EngineStopSound;
	protected string m_DoorOpenSound;
	protected string m_DoorCloseSound;
	protected string m_GearMoveSound;
	protected string m_GearLockSound;
	protected string m_FlapSound;
	protected string m_SplashSound;
	protected ref TStringArray m_WaterSoundSets;
	protected ref GebsPlaneEngineSound m_EngineSound;
	protected ref GebsPlaneEngineSound m_AirSound;
	protected ref GebsPlaneEngineSound m_WaterSound;
	protected float m_HeardWet;			//!< the floats' share in the water, last frame
	protected int m_HeardEngineState;
	protected float m_HeardGear = -1;
	protected bool m_HeardGearMoving;
	protected float m_HeardFlaps = -1;
	protected float m_FlapSoundTime;

	// vehicle actions (get in, the door), shared by every plane of a type
	static ref map<typename, ref TInputActionMap> s_GebsPlaneActionMaps = new map<typename, ref TInputActionMap>();
	protected TInputActionMap m_InputActionMap;
	protected bool m_ActionsInitialized;

	// test readout (GebsFlight debugLog): a line a second in the pilot's and the server's script logs
	protected bool m_DebugLog;
	protected float m_DebugTimer;
	protected int m_DebugSteps;			//!< simulation steps since the last line
	protected int m_DebugMovesOut;		//!< moves this machine sent (the pilot's)
	protected int m_DebugMovesIn;		//!< moves this machine flew from the pilot (the server)
	protected float m_DebugAirspeed;	//!< m/s along the nose
	protected float m_DebugThrust;		//!< N
	protected float m_DebugLift;		//!< N, straight up, all surfaces
	protected float m_DebugAlpha;		//!< deg, the wing's average angle of attack
	protected float m_DebugPullRoom = 1;	//!< what a held pull key had left (PullRoom), last input
	protected float m_DebugArcadeAuthority;	//!< the arcade assist, last step: how much it flew the plane
	protected vector m_DebugArcadeWant;		//!< the roll, pitch and yaw rates it asked for (deg/s)
	protected vector m_DebugArcadeRate;		//!< and the plane's (deg/s)
	protected float m_DebugArcadeBank;		//!< deg, right wing down +
	protected float m_DebugArcadeClimb;		//!< deg, the flight path's
	protected int m_DebugRewinds;		//!< corrections from the server this machine rewound to (the pilot's)

	void GebsPlane()
	{
		// no car simulation makes the body: the plane makes its own, from its
		// Geometry LOD's components and masses
		CreateDynamicPhysics(PhxInteractionLayers.VEHICLE);
		dBodyActive(this, ActiveState.ACTIVE);
		NoBodyDamping();

		SetEventMask(EntityEvent.SIMULATE);
		SetEventMask(EntityEvent.POSTSIMULATE);
		SetEventMask(EntityEvent.FRAME);
		SetEventMask(EntityEvent.CONTACT);

		m_FlightSurfaces = new array<ref GebsAeroSurface>();
		m_Wheels = new array<ref GebsPlaneWheel>();
		m_Gauges = new array<ref GebsPlaneGauge>();
		m_FlightControls = new GebsPlaneControls();
		m_FlightInputs = new map<string, UAInput>();
		m_EngineSoundSets = new TStringArray();
		m_AirSoundSets = new TStringArray();
		m_WaterSoundSets = new TStringArray();

		// a plane comes with some fuel; a saved one gets its own back in OnStoreLoad.
		// (An object's own ConfigGetFloat reads only its top-level entries: the
		// GebsFlight one needs the full path, or a new plane came with none.)
		m_FuelCapacity = Math.Max(ConfigGetFloat("fuelCapacity"), 1);
		m_Fuel = m_FuelCapacity * Math.Clamp(g_Game.ConfigGetFloat("CfgVehicles " + GetType() + " GebsFlight spawnFuel"), 0, 1);
		m_FuelSent = m_Fuel;

		RegisterNetSyncVariableFloat("m_NetElevator", -1, 1, 2);
		RegisterNetSyncVariableFloat("m_NetAileron", -1, 1, 2);
		RegisterNetSyncVariableFloat("m_NetRudder", -1, 1, 2);
		RegisterNetSyncVariableFloat("m_NetFlaps", 0, 1, 2);
		RegisterNetSyncVariableFloat("m_NetTrim", -1, 1, 2);
		RegisterNetSyncVariableFloat("m_NetThrottle", 0, 1, 2);
		RegisterNetSyncVariableFloat("m_NetPower", 0, 1, 2);
		RegisterNetSyncVariableFloat("m_NetRpm", 0, 4000, 0);
		RegisterNetSyncVariableFloat("m_NetGear", 0, 1, 2);
		RegisterNetSyncVariableBool("m_NetGearDown");
		RegisterNetSyncVariableBool("m_NetSwitch");
		RegisterNetSyncVariableFloat("m_NetSlatLeft", 0, 1, 1);
		RegisterNetSyncVariableFloat("m_NetSlatRight", 0, 1, 1);
		RegisterNetSyncVariableInt("m_NetEngineState", 0, 2);
		RegisterNetSyncVariableFloat("m_Fuel", 0, m_FuelCapacity, 1);
	}

	override void EEInit()
	{
		super.EEInit();

		NoBodyDamping();
		LoadFlightModel();
	}

	/*!
		A new dynamic body comes with the engine's own damping, a drag of about
		0.12 x mass x speed (measured off the Bf 109's first runs: at full power
		it held the plane to 100 km/h on the runway, and it would cap it in the
		air too). The plane's drag is all its own (the surfaces, the body, the
		tyres), so the body gets none. On every machine that simulates it.
	*/
	protected void NoBodyDamping()
	{
		if (dBodyIsSet(this))
			dBodySetDamping(this, 0, 0);
	}

	override void EEDelete(EntityAI parent)
	{
		if (m_EngineSound)
			m_EngineSound.Stop();
		if (m_AirSound)
			m_AirSound.Stop();
		if (m_WaterSound)
			m_WaterSound.Stop();
		if (m_FloatGear)
			m_FloatGear.StopEffects();

		super.EEDelete(parent);
	}

	//--------------------------------------------------------------------------
	// config and model
	//--------------------------------------------------------------------------

	protected string FlightPath()
	{
		return "CfgVehicles " + GetType() + " GebsFlight";
	}

	protected float FlightFloat(string key)
	{
		return g_Game.ConfigGetFloat(FlightPath() + " " + key);
	}

	protected string FlightText(string key)
	{
		string value;
		g_Game.ConfigGetText(FlightPath() + " " + key, value);
		return value;
	}

	//! A number from a config class, or fallback when it isn't there.
	protected static float GebsConfigFloat(string path, string key, float fallback)
	{
		if (!g_Game.ConfigIsExisting(path + " " + key))
			return fallback;

		return g_Game.ConfigGetFloat(path + " " + key);
	}

	protected vector FlightPoint(string key)
	{
		string point = FlightText(key);
		if (point == "" || !MemoryPointExists(point))
		{
			ErrorEx(string.Format("GebsPlanes: %1 has no memory point '%2' (%3)", GetType(), point, key));
			return vector.Zero;
		}

		return GetMemoryPointPos(point);
	}

	protected void LoadFlightModel()
	{
		if (m_FlightLoaded)
			return;

		m_FlightLoaded = true;

		string path = FlightPath();
		if (!g_Game.ConfigIsExisting(path))
		{
			ErrorEx("GebsPlanes: " + GetType() + " has no GebsFlight config class");
			return;
		}

		// The propeller's axis says which way is forward; up is the model's up.
		m_PropHub = FlightPoint("propHub");
		vector front = FlightPoint("propFront");
		m_FlightUp = "0 1 0";
		m_FlightFwd = front - m_PropHub;
		m_FlightFwd[1] = 0;
		m_FlightFwd.Normalize();
		m_FlightSide = Cross(m_FlightUp, m_FlightFwd);
		m_BodyPoint = FlightPoint("bodyPoint");
		m_DoorPoint = FlightText("doorPoint");
		m_DoorSource = FlightText("doorSource");
		m_BoardPoint = FlightText("boardPoint");
		if (m_BoardPoint == "")
			m_BoardPoint = m_DoorPoint;
		m_BoardReach = FlightFloat("boardReach");
		if (m_BoardReach <= 0)
			m_BoardReach = 3.5;
		m_BoardHeight = FlightFloat("boardHeight");
		if (m_BoardHeight <= 0)
			m_BoardHeight = BOARD_HEIGHT;

		string prop = FlightText("propType");
		m_ConstantSpeed = prop == "constantSpeed";
		m_StaticThrust = Math.Max(FlightFloat("staticThrust"), 1);
		m_PropPitchSpeed = Math.Max(FlightFloat("propPitchSpeed"), 1);
		m_PropEfficiency = Math.Clamp(FlightFloat("propEfficiency"), 0.3, 0.95);
		m_PropDiameter = Math.Max(FlightFloat("propDiameter"), 0.1);
		m_PropDragArea = FlightFloat("propDragArea");
		m_PropInertia = FlightFloat("propInertia");
		m_PropRotation = g_Game.ConfigGetInt(path + " propRotation");
		m_GearRatio = FlightFloat("gearRatio");
		if (m_GearRatio <= 0)
			m_GearRatio = 1;
		m_RatedPower = FlightFloat("ratedPower");
		m_CriticalAltitude = FlightFloat("criticalAltitude");
		m_IdlePower = FlightFloat("idlePower");
		m_PowerLag = Math.Max(FlightFloat("powerLag"), 0.05);
		m_RpmIdle = FlightFloat("rpmIdle");
		m_RpmStatic = FlightFloat("rpmStatic");
		m_RpmMax = FlightFloat("rpmMax");
		m_RpmLag = Math.Max(FlightFloat("rpmLag"), 0.05);
		m_RpmPerSpeed = FlightFloat("rpmPerSpeed");
		m_RpmOverspeed = FlightFloat("rpmOverspeed");
		m_StarterRpm = FlightFloat("starterRpm");
		m_StarterWindTime = FlightFloat("starterWindTime");
		m_CatchTime = FlightFloat("catchTime");
		m_CrankGiveUp = FlightFloat("crankGiveUp");
		if (m_CrankGiveUp <= 0)
			m_CrankGiveUp = 3;
		m_BoostIdle = FlightFloat("boostIdle");
		m_BoostMax = FlightFloat("boostMax");
		m_SoundRpmScale = FlightFloat("soundRpmScale");
		m_SlipstreamGain = FlightFloat("slipstream");
		m_SwirlGain = FlightFloat("swirl");
		m_TorqueGain = FlightFloat("torque");
		m_FuelBurnIdle = FlightFloat("fuelBurnIdle");
		m_FuelBurnFull = FlightFloat("fuelBurnFull");

		m_BodyCdaFwd = FlightFloat("bodyCdaForward");
		m_BodyCdaSide = FlightFloat("bodyCdaSide");
		m_BodyCdaUp = FlightFloat("bodyCdaUp");
		m_GearCda = FlightFloat("gearCda");
		m_Span = FlightFloat("span");
		m_DownwashGain = FlightFloat("downwash");
		m_GroundEffect = g_Game.ConfigGetInt(path + " groundEffect") != 0;
		m_WindScale = FlightFloat("windScale");
		m_FlapNotches = Math.Max(g_Game.ConfigGetInt(path + " flapNotches"), 1);
		m_FlapRate = FlightFloat("flapRate");
		if (m_FlapRate <= 0)
			m_FlapRate = 0.2;
		m_FlapDegrees = FlightFloat("flapDegrees");
		m_SlatAlpha = FlightFloat("slatAlpha") * Math.DEG2RAD;
		m_SlatFull = Math.Max(FlightFloat("slatFull") * Math.DEG2RAD, m_SlatAlpha + 0.01);
		m_SlatStall = FlightFloat("slatStall") * Math.DEG2RAD;
		m_SlatCL = FlightFloat("slatCL");
		m_SlatTime = Math.Max(FlightFloat("slatTime"), 0.05);
		m_CrashImpulse = FlightFloat("crashImpulse");
		m_CrashDamage = FlightFloat("crashDamage");
		m_AileronRig = FlightFloat("aileronRig");
		m_PullMargin = FlightFloat("stallMargin") * Math.DEG2RAD;
		m_PullG = FlightFloat("pullG");
		m_DebugLog = g_Game.ConfigGetInt(path + " debugLog") != 0;
		m_HasCockpit = g_Game.ConfigGetInt(path + " cockpit") != 0;
		m_HasTrimAnim = g_Game.ConfigGetInt(path + " trimAnim") != 0;

		m_ParkHoldSpeed = FlightFloat("parkHoldSpeed");
		m_DiffBrake = FlightFloat("diffBrake");
		m_DiffBrakeFade = Math.Max(FlightFloat("diffBrakeFade"), 1);
		m_TailSteerFade = Math.Max(FlightFloat("tailSteerFade"), 1);
		m_TailSteerMin = FlightFloat("tailSteerMin");
		m_GearUpTime = FlightFloat("gearUpTime");
		m_GearDownTime = Math.Max(FlightFloat("gearDownTime"), 0.1);
		m_GearNeedsEngine = g_Game.ConfigGetInt(path + " gearNeedsEngine") != 0;

		string soundPoint = FlightText("soundPoint");
		if (soundPoint != "" && MemoryPointExists(soundPoint))
			m_SoundPos = GetMemoryPointPos(soundPoint);
		g_Game.ConfigGetTextArray(path + " engineSounds", m_EngineSoundSets);
		g_Game.ConfigGetTextArray(path + " airSounds", m_AirSoundSets);
		m_EngineStartSound = FlightText("engineStartSound");
		m_EngineCatchSound = FlightText("engineCatchSound");
		m_EngineFailSound = FlightText("engineFailSound");
		m_EngineStopSound = FlightText("engineStopSound");
		m_DoorOpenSound = FlightText("doorOpenSound");
		m_DoorCloseSound = FlightText("doorCloseSound");
		m_GearMoveSound = FlightText("gearMoveSound");
		m_GearLockSound = FlightText("gearLockSound");
		m_FlapSound = FlightText("flapSound");
		m_SplashSound = FlightText("splashSound");
		g_Game.ConfigGetTextArray(path + " waterSounds", m_WaterSoundSets);

		// a floatplane's floats (gebsplanefloat.c)
		m_FloatGear = GebsPlaneFloats.Load(path + " Floats", this);

		// arcade flying (SimulateArcade)
		string arcade = path + " Arcade";
		m_Arcade = g_Game.ConfigIsExisting(arcade) && GebsConfigFloat(arcade, "enabled", 1) != 0;
		m_ArcadeRollRate = GebsConfigFloat(arcade, "rollRate", 60) * Math.DEG2RAD;
		m_ArcadePitchRate = GebsConfigFloat(arcade, "pitchRate", 25) * Math.DEG2RAD;
		m_ArcadeYawRate = GebsConfigFloat(arcade, "yawRate", 15) * Math.DEG2RAD;
		m_ArcadeWaterYawRate = GebsConfigFloat(arcade, "waterYawRate", 30) * Math.DEG2RAD;
		m_ArcadeLevelRoll = GebsConfigFloat(arcade, "levelRoll", 1.5);
		m_ArcadeLevelRollMax = GebsConfigFloat(arcade, "levelRollMax", 40) * Math.DEG2RAD;
		m_ArcadeLevelPitch = GebsConfigFloat(arcade, "levelPitch", 1);
		m_ArcadeLevelPitchMax = GebsConfigFloat(arcade, "levelPitchMax", 8) * Math.DEG2RAD;
		m_ArcadeBankLimit = GebsConfigFloat(arcade, "bankLimit", 0) * Math.DEG2RAD;
		m_ArcadeGain = GebsConfigFloat(arcade, "rateGain", 5);
		m_ArcadeAccel = GebsConfigFloat(arcade, "maxAccel", 3);
		m_ArcadeSlip = GebsConfigFloat(arcade, "slipGain", 2);
		m_ArcadeStallMargin = GebsConfigFloat(arcade, "stallMargin", 3) * Math.DEG2RAD;
		m_ArcadeStallGain = GebsConfigFloat(arcade, "stallGain", 3);
		m_ArcadeSpeedMin = GebsConfigFloat(arcade, "speedMin", 25) / 3.6;
		m_ArcadeSpeedFull = Math.Max(GebsConfigFloat(arcade, "speedFull", 55) / 3.6, m_ArcadeSpeedMin + 0.5);
		m_ArcadeTakeOver = GebsConfigFloat(arcade, "takeOver", 1);
		m_ArcadeWaterPitch = GebsConfigFloat(arcade, "waterPitch", 6) * Math.DEG2RAD;
		m_ArcadeWaterPitchMax = GebsConfigFloat(arcade, "waterPitchMax", 8) * Math.DEG2RAD;
		m_ArcadeWaterPitchGain = GebsConfigFloat(arcade, "waterPitchGain", 2);
		m_ArcadeClimbMax = GebsConfigFloat(arcade, "climbMax", 30) * Math.DEG2RAD;
		m_ArcadeClimbSpeedMin = GebsConfigFloat(arcade, "climbSpeedMin", 80) / 3.6;
		m_ArcadeClimbSpeed = Math.Max(GebsConfigFloat(arcade, "climbSpeed", 120) / 3.6, m_ArcadeClimbSpeedMin + 0.5);
		m_ArcadeDiveMax = GebsConfigFloat(arcade, "diveMax", 45) * Math.DEG2RAD;
		m_ArcadeClimbGain = GebsConfigFloat(arcade, "climbGain", 1.5);

		string surfaces = path + " Surfaces";
		int count = g_Game.ConfigGetChildrenCount(surfaces);
		float tailBack = 0;
		for (int i = 0; i < count; i++)
		{
			string name;
			g_Game.ConfigGetChildName(surfaces, i, name);
			GebsAeroSurface surface = GebsAeroSurface.Load(surfaces + " " + name, name, this, m_FlightUp, m_FlightSide, m_FlightFwd);
			if (!surface)
				continue;

			m_FlightSurfaces.Insert(surface);
			// the furthest-back surface (the tail) and the propeller are what the turn rates are read between
			float back = -vector.Dot(surface.m_Point - m_PropHub, m_FlightFwd);
			if (back > tailBack)
			{
				tailBack = back;
				m_TailPoint = surface.m_Point;
			}
		}
		if (tailBack <= 0)
			m_TailPoint = m_PropHub - m_FlightFwd * 4;

		string wheels = path + " Wheels";
		count = g_Game.ConfigGetChildrenCount(wheels);
		for (i = 0; i < count; i++)
		{
			string wheelName;
			g_Game.ConfigGetChildName(wheels, i, wheelName);
			GebsPlaneWheel wheel = GebsPlaneWheel.Load(wheels + " " + wheelName, wheelName, this);
			if (wheel)
				m_Wheels.Insert(wheel);
		}

		string gauges = path + " Gauges";
		count = g_Game.ConfigGetChildrenCount(gauges);
		for (i = 0; i < count; i++)
		{
			string gaugeName;
			g_Game.ConfigGetChildName(gauges, i, gaugeName);
			GebsPlaneGauge gauge = GebsPlaneGauge.Load(gauges + " " + gaugeName);
			if (gauge)
				m_Gauges.Insert(gauge);
		}
	}

	//! A model-space point in the world, as the physics body stands this step (m_SimTm, see EOnSimulate).
	vector SimPoint(vector modelPos)
	{
		return m_SimTm[3] + SimDir(modelPos);
	}

	//! A model-space direction in the world, as the physics body stands this step.
	vector SimDir(vector modelDir)
	{
		return m_SimTm[0] * modelDir[0] + m_SimTm[1] * modelDir[1] + m_SimTm[2] * modelDir[2];
	}

	//! The body's velocity (the entity's own can lag it, as its transform does in a replayed step).
	vector SimVelocity()
	{
		return dBodyGetVelocityAt(this, SimPoint(dBodyGetCenterOfMass(this)));
	}

	static vector Cross(vector a, vector b)
	{
		return Vector(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
	}

	static float WrapAngle(float a)
	{
		while (a > Math.PI)
			a -= 2 * Math.PI;
		while (a < -Math.PI)
			a += 2 * Math.PI;
		return a;
	}

	//! International standard atmosphere, by height above sea level.
	static float AirDensity(float altitude)
	{
		float t = 1 - 0.0000225577 * Math.Clamp(altitude, -500, 11000);
		return GebsPlane.SEA_LEVEL_DENSITY * Math.Pow(t, 4.2559);
	}

	//! Share of its sea-level power the engine gives at this height: a
	//! supercharger holds it up to its rated altitude, then it falls with the air.
	float AltitudePower(float altitude)
	{
		if (m_CriticalAltitude > 0)
		{
			if (altitude <= m_CriticalAltitude)
				return 1;

			return AirDensity(altitude) / AirDensity(m_CriticalAltitude);
		}

		return AirDensity(altitude) / SEA_LEVEL_DENSITY;
	}

	//--------------------------------------------------------------------------
	// what a Transport has to say for itself
	//--------------------------------------------------------------------------

	override string GetVehicleType()
	{
		return "VehicleTypeGebsPlane";
	}

	override int Get3rdPersonCameraType()
	{
		return DayZPlayerCameras.DAYZCAMERA_3RD_VEHICLE;
	}

	//--------------------------------------------------------------------------
	// pilot input (the pilot's client)
	//--------------------------------------------------------------------------

	bool IsLocalPilot()
	{
		Human driver = CrewDriver();
		return driver && driver == g_Game.GetPlayer() && driver.IsControllingVehicle();
	}

	protected float InputValue(string name)
	{
		UAInput input;
		if (!m_FlightInputs.Find(name, input))
		{
			input = GetUApi().GetInputByName(name);
			m_FlightInputs.Insert(name, input);
		}

		if (!input)
			return 0;

		return input.LocalValue();
	}

	//! Keyboard stick: moves towards where it is pushed, springs back (at back) when let go.
	protected static float MoveStick(float current, float target, float dt, float rate, float back = 4.0)
	{
		if (target == 0 || target * current < 0)
			rate = back;

		float step = rate * dt;
		if (Math.AbsFloat(target - current) <= step)
			return target;
		if (target > current)
			return current + step;
		return current - step;
	}

	/*!
		How much room a held pull key has left: 1 all of it, 0 at the limit,
		below 0 inside it. The limits are the wing panel nearest its stall
		against stallMargin, and the g against pullG, each read PULL_LEAD ahead
		at the rate the wing's angle of attack is rising (SimulateAir keeps
		them, on the pilot's machine as on the server).
	*/
	protected float PullRoom()
	{
		float rising = Math.Max(m_AlphaRate, 0) * PULL_LEAD;
		float room = 1;
		if (m_PullMargin > 0)
			room = Math.Min(room, (m_StallMargin - rising - m_PullMargin) / PULL_BAND);
		if (m_PullG > 0)
			room = Math.Min(room, (m_PullG - m_LoadFactor - rising * m_LoadPerRad) / PULL_BAND_G);
		return room;
	}

	protected void ReadPilotInput(float dt)
	{
		GebsPlaneControls c = m_FlightControls;

		float pitch = Math.Clamp(InputValue("UAGebsPlanePitchUp") - InputValue("UAGebsPlanePitchDown"), -1, 1);
		float roll = InputValue("UAGebsPlaneRollRight") - InputValue("UAGebsPlaneRollLeft");
		float yaw = InputValue("UAGebsPlaneRudderRight") - InputValue("UAGebsPlaneRudderLeft");

		// the stick back: a held pull slows as the room before the stall or the g limit runs out and
		// stops there, as a pilot's hand does at the first buffet; inside either, it eases forward
		// again (never past the middle). A key held into the stall dropped the Bf 109's wing
		float before = c.m_Elevator;
		float room = 1;
		if (m_Arcade)
		{
			// arcade: the stick asks for a pitch rate (SimulateArcade), which keeps off the stall by itself
			c.m_Elevator = MoveStick(before, pitch, dt, CONTROL_RATE, CONTROL_RETURN);
		}
		else
		{
			float want = pitch;
			// on floats, up on the step, a held pull eases to the planing attitude's (Floats stepPull): full back
			// stick there drags the afterbody, and the moment the floats come off throws the nose up into the stall
			if (pitch > 0 && m_FloatGear && m_FloatGear.IsWet() && m_DebugAirspeed > 2 * m_FloatGear.m_HumpSpeed)
				want = Math.Min(pitch, m_FloatGear.m_StepPull);
			float elevator = MoveStick(before, want, dt, ELEVATOR_RATE, ELEVATOR_RETURN);
			if (pitch > 0 && before >= 0)
			{
				room = PullRoom();
				if (room < 0)
					elevator = Math.Max(Math.Min(elevator, before + ELEVATOR_RATE * Math.Max(room, -PULL_BACK) * dt), 0);
				else if (room < 1 && elevator > before)
					elevator = before + (elevator - before) * room;
			}
			c.m_Elevator = elevator;
		}
		m_DebugPullRoom = room;

		c.m_Aileron = MoveStick(c.m_Aileron, Math.Clamp(roll, -1, 1), dt, AILERON_RATE, CONTROL_RETURN);
		c.m_Rudder = MoveStick(c.m_Rudder, Math.Clamp(yaw, -1, 1), dt, CONTROL_RATE, CONTROL_RETURN);

		float lever = InputValue("UAGebsPlaneThrottleUp") - InputValue("UAGebsPlaneThrottleDown");
		c.m_Throttle = Math.Clamp(c.m_Throttle + lever * THROTTLE_RATE * dt, 0, 1);

		float trim = InputValue("UAGebsPlaneTrimUp") - InputValue("UAGebsPlaneTrimDown");
		c.m_Trim = Math.Clamp(c.m_Trim + trim * TRIM_RATE * dt, -1, 1);

		// one notch per press, however many input steps the press lasts
		bool flapsDown = InputValue("UAGebsPlaneFlapsExtend") > 0.5;
		bool flapsUp = InputValue("UAGebsPlaneFlapsRetract") > 0.5;
		if (flapsDown && !m_FlapsDownHeld && m_FlapNotch < m_FlapNotches)
			m_FlapNotch++;
		if (flapsUp && !m_FlapsUpHeld && m_FlapNotch > 0)
			m_FlapNotch--;
		m_FlapsDownHeld = flapsDown;
		m_FlapsUpHeld = flapsUp;

		float flapTarget = m_FlapNotch / m_FlapNotches;
		float flapStep = m_FlapRate * dt;
		if (Math.AbsFloat(flapTarget - c.m_Flaps) <= flapStep)
			c.m_Flaps = flapTarget;
		else if (flapTarget > c.m_Flaps)
			c.m_Flaps = c.m_Flaps + flapStep;
		else
			c.m_Flaps = c.m_Flaps - flapStep;

		c.m_Brake = Math.Clamp(InputValue("UAGebsPlaneBrake"), 0, 1);

		// the gear lever: one throw per press. The simulation won't fold a leg that carries the plane
		if (m_GearUpTime > 0)
		{
			bool gearKey = InputValue("UAGebsPlaneGear") > 0.5;
			if (gearKey && !m_GearKeyHeld)
				c.m_GearDown = !c.m_GearDown;
			m_GearKeyHeld = gearKey;
		}
	}

	override void OnInput(float dt)
	{
		super.OnInput(dt);

		if (IsLocalPilot())
			ReadPilotInput(dt);
	}

	/*!
		The engine actions (gebsplaneengine.c), on the server and on the pilot's
		client both. Holding the start action switches the magnetos on and the
		starter in; SimulateEngine cranks and fires the engine. The action is as
		short as a car's start (a seated animation won't last a start), so once
		it has run its course the starter latches and the engine turns over and
		fires by itself; let go before that and it stops cranking.
	*/
	void HoldStarter(bool held)
	{
		GebsPlaneControls c = m_FlightControls;
		if (held)
		{
			c.m_Ignition = true;
			c.m_Starter = true;
			WakeUp();
			DebugNote("starter held");
			return;
		}

		if (!c.m_StarterLatched)
			c.m_Starter = false;

		DebugNote("starter let go, latched " + YesNo(c.m_StarterLatched) + ", engine " + typename.EnumToString(GebsEngineState, m_EngineState));
	}

	void LatchStarter()
	{
		if (m_FlightControls.m_Starter)
			m_FlightControls.m_StarterLatched = true;

		DebugNote("starter latched");
	}

	//! The engine is running, or turning over by itself after a start.
	bool IsStarting()
	{
		return GetEngineState() != GebsEngineState.OFF || m_FlightControls.m_StarterLatched;
	}

	void SwitchOff()
	{
		m_FlightControls.SwitchOff();
		DebugNote("switched off");
	}

	// kept for the engine actions of v0.2
	void PilotStarter(bool held)
	{
		HoldStarter(held);
	}

	void PilotIgnition(bool on)
	{
		if (!on)
			SwitchOff();
	}

	//! The pilot gone: lever shut, switches off, the brakes hold it.
	override void OnDriverExit(Human player)
	{
		super.OnDriverExit(player);

		m_FlightControls.ResetSticks();
		SwitchOff();
	}

	//--------------------------------------------------------------------------
	// network move: the controls go with the vehicle's own inputs
	//--------------------------------------------------------------------------

	protected override event typename GetMoveType()
	{
		return GebsPlaneMove;
	}

	protected override event typename GetOwnerStateType()
	{
		return GebsPlaneOwnerState;
	}

	protected override event void ObtainMove(/*inout*/ PawnMove pMove)
	{
		super.ObtainMove(pMove);

		GebsPlaneMove move = GebsPlaneMove.Cast(pMove);
		if (move)
			move.Store(m_FlightControls);
		m_DebugMovesOut++;
	}

	protected override event void ConsumeMove(PawnMove pMove)
	{
		super.ConsumeMove(pMove);

		GebsPlaneMove move = GebsPlaneMove.Cast(pMove);
		if (move)
			move.Restore(m_FlightControls);
		m_DebugMovesIn++;
	}

	protected override event bool ReplayMove(PawnMove pMove)
	{
		GebsPlaneMove move = GebsPlaneMove.Cast(pMove);
		if (move)
			move.Restore(m_FlightControls);

		return super.ReplayMove(pMove);
	}

	//! After a correction the pilot's machine re-runs its moves since; each step
	//! of that has to fly on the controls it was first flown on
	protected override event void SimulateMove(PawnMove pMove)
	{
		super.SimulateMove(pMove);

		GebsPlaneMove move = GebsPlaneMove.Cast(pMove);
		if (move)
			move.Restore(m_FlightControls);
	}

	protected override event void ObtainState(/*inout*/ PawnOwnerState pState)
	{
		super.ObtainState(pState);

		GebsPlaneOwnerState state = GebsPlaneOwnerState.Cast(pState);
		if (state)
		{
			state.m_EngineState = m_EngineState;
			state.m_CrankTime = m_CrankTime;
			state.m_EnginePower = m_EnginePower;
			state.m_WingCL = m_WingCL;
			state.m_PropRpm = m_PropRpm;
			state.m_GearPhase = m_GearPhase;
			state.m_SlatLeft = m_SlatLeft;
			state.m_SlatRight = m_SlatRight;
		}
	}

	protected override event void RewindState(PawnOwnerState pState, /*inout*/ PawnMove pMove, inout NetworkRewindType pRewindType)
	{
		super.RewindState(pState, pMove, pRewindType);
		m_DebugRewinds++;

		// an additive rewind keeps the simulation where it is and adds the
		// difference, as the player's own does: the state stays
		if (pRewindType == NetworkRewindType.ADDITIVE)
			return;

		GebsPlaneOwnerState state = GebsPlaneOwnerState.Cast(pState);
		if (state)
		{
			m_EngineState = state.m_EngineState;
			m_CrankTime = state.m_CrankTime;
			m_EnginePower = state.m_EnginePower;
			m_WingCL = state.m_WingCL;
			m_PropRpm = state.m_PropRpm;
			m_GearPhase = state.m_GearPhase;
			m_SlatLeft = state.m_SlatLeft;
			m_SlatRight = state.m_SlatRight;
		}
	}

	//--------------------------------------------------------------------------
	// simulation (the server and the pilot's client)
	//
	// Both simulate the plane from the same moves, and the server corrects the
	// pilot's machine wherever the two part: every correction is a jolt. So
	// everything here has to come out the same on both. The forces go in
	// through Transport.ApplyForceAt, the engine's deterministic physics that a
	// correction replays, not straight onto the body (dBodyApplyForceAt), and
	// nothing reads what each machine has its own copy of, such as the
	// weather's wind (GebsFlight windScale=0).
	//--------------------------------------------------------------------------

	override void EOnSimulate(IEntity other, float dt)
	{
		if (IsProxy() || dt <= 0)
			return;

		if (!m_FlightLoaded)
			LoadFlightModel();

		m_DebugSteps++;

		// where the body is this step: everything below works from this, not from the entity's own
		// transform (GetPosition, ModelToWorld, VectorToParent). When the server corrects the pilot's
		// machine, it replays several steps inside one frame; if the entity's transform stays where it
		// was before the replay, every wheel's ray, every force's point and every wing's angle comes out
		// of the past, and each correction is a jolt. (2026-11-03: two or three corrections a second,
		// the server's plane steady on the taxiway, its tailwheel at 2-3 kN, while the pilot's bounced
		// on its tail, 0-35 kN, and skipped about in the air.) Vanilla's boat reads its position in
		// EOnSimulate the same way
		dBodyGetWorldTransform(this, m_SimTm);

		if (!CrewDriver())
			m_FlightControls.ResetSticks();

		SimulateEngine(dt);
		SimulateGear(dt);
		SimulateGround(dt);
		BeginAirMoments();
		SimulateFloats(dt);
		if (m_FlightSurfaces.Count() > 0)
			SimulateAir(dt);
		if (m_Arcade)
			SimulateArcade(dt);
	}

	//--------------------------------------------------------------------------
	// on the water (a floatplane)
	//--------------------------------------------------------------------------

	bool HasFloats()
	{
		return m_FloatGear != null;
	}

	//! Whether any float is in the water (this machine's last step; other players' planes: as drawn).
	bool IsOnWater()
	{
		if (!m_FloatGear)
			return false;
		if (IsProxy())
			return m_FloatGear.m_WetShare > 0;

		return m_FloatGear.IsWet();
	}

	/*!
		The floats in the water (GebsPlaneFloats.Simulate). With the
		propeller's hub under the water the engine drowns: it stops, and the
		server takes a quarter of its health. Coming down harder than slamLoad g
		damages the airframe, crashDamage per N s past it, as a knock on land
		does (EOnContact).
	*/
	protected void SimulateFloats(float dt)
	{
		if (!m_FloatGear)
			return;

		m_FloatGear.Simulate(this, dt, SimDir(m_FlightFwd), SimDir(m_FlightSide), SimDir(m_FlightUp), m_FlightControls.m_Rudder, IsDamageDestroyed());

		bool hubUnder = GebsPlaneFloats.WaterDepthAt(SimPoint(m_PropHub)) > 0;
		if (hubUnder && m_EngineState != GebsEngineState.OFF)
		{
			m_EngineState = GebsEngineState.OFF;
			DebugNote("the propeller went under the water: the engine drowned");
			if (g_Game.IsServer() && !m_Drowned)
				AddHealth("Engine", "Health", -GetMaxHealth("Engine", "") * 0.25);
			m_Drowned = true;
		}
		else if (!hubUnder)
		{
			m_Drowned = false;
		}

		if (g_Game.IsServer() && m_FloatGear.m_SlamLoad > 0 && m_CrashDamage > 0)
		{
			float limit = m_FloatGear.m_SlamLoad * dBodyGetMass(this) * 9.81;
			if (m_FloatGear.m_Load > limit)
				ProcessDirectDamage(DamageType.CUSTOM, null, "Chassis", "EnviroDmg", m_BodyPoint, (m_FloatGear.m_Load - limit) * dt * m_CrashDamage);
		}
	}

	//! Whether the engine can run: fuel, and an engine that isn't ruined.
	bool CanEngineRun()
	{
		return m_Fuel > 0 && !IsDamageDestroyed() && GetHealthLevel("Engine") < GameConstants.STATE_RUINED;
	}

	//! Why it can't, for the script log: "" if it can.
	string WhyEngineWontRun()
	{
		string why;
		if (IsDamageDestroyed())
			why += " the plane is ruined;";
		if (GetHealthLevel("Engine") >= GameConstants.STATE_RUINED)
			why += " the engine is ruined;";
		if (m_Fuel <= 0)
			why += " no fuel;";
		return why;
	}

	/*!
		The engine: off; cranking while the starter is in (an inertia starter
		spins its flywheel up first), firing at catchTime if it can run, the
		starter giving up crankGiveUp later if it can't; running on the ignition
		until it's switched off or runs dry. Its power follows the throttle with
		a lag, and a constant-speed propeller's governor brings the revs to what
		the throttle sets.
	*/
	protected void SimulateEngine(float dt)
	{
		GebsPlaneControls c = m_FlightControls;

		// nobody at the controls (the pilot got out, died or left the game): off,
		// whatever this machine heard of the switches
		if (!CrewDriver() && (c.m_Ignition || c.m_Starter || c.m_StarterLatched))
		{
			c.SwitchOff();
			DebugNote("nobody at the controls: switched off");
		}

		bool starting = c.m_Starter || c.m_StarterLatched;
		switch (m_EngineState)
		{
			case GebsEngineState.OFF:
				if (c.m_Ignition && starting && !IsDamageDestroyed())
				{
					m_EngineState = GebsEngineState.CRANKING;
					m_CrankTime = 0;
				}
				break;

			case GebsEngineState.CRANKING:
				if (!c.m_Ignition || !starting)
				{
					m_EngineState = GebsEngineState.OFF;
					c.m_Starter = false;
					c.m_StarterLatched = false;
					break;
				}

				m_CrankTime += dt;
				if (m_CrankTime >= m_CatchTime && CanEngineRun())
				{
					// it fires: the starter has done its job
					m_EngineState = GebsEngineState.RUNNING;
					c.m_Starter = false;
					c.m_StarterLatched = false;
				}
				else if (m_CrankTime >= m_CatchTime + m_CrankGiveUp)
				{
					// it won't fire (no fuel, a ruined engine): the starter runs down
					m_EngineState = GebsEngineState.OFF;
					c.m_Starter = false;
					c.m_StarterLatched = false;
				}
				break;

			case GebsEngineState.RUNNING:
				if (!c.m_Ignition || !CanEngineRun())
					m_EngineState = GebsEngineState.OFF;
				break;
		}

		float target = 0;
		if (m_EngineState == GebsEngineState.RUNNING)
			target = m_IdlePower + (1 - m_IdlePower) * c.m_Throttle;

		m_EnginePower += (target - m_EnginePower) * Math.Clamp(dt / m_PowerLag, 0, 1);
		if (target == 0 && m_EnginePower < 0.002)
			m_EnginePower = 0;

		if (m_ConstantSpeed)
		{
			float speedFwd = vector.Dot(SimVelocity(), SimDir(m_FlightFwd));
			float rpmTarget = GovernedPropRpm(c.m_Throttle, speedFwd, m_EngineState, m_CrankTime);
			m_PropRpm += (rpmTarget - m_PropRpm) * Math.Clamp(dt / m_RpmLag, 0, 1);
			if (rpmTarget == 0 && m_PropRpm < 1)
				m_PropRpm = 0;
		}
	}

	/*!
		A constant-speed propeller's revs (the propeller's, not the engine's):
		running, the governor holds what the throttle sets, from idle to rpmMax;
		on the starter, the starter's (nothing while an inertia starter's
		flywheel spins up); dead, it windmills on the airflow.
	*/
	float GovernedPropRpm(float throttle, float speedFwd, int engineState, float crankTime)
	{
		float windmill = Math.Max(speedFwd, 0) * m_RpmPerSpeed;
		if (engineState == GebsEngineState.RUNNING)
		{
			float governed = m_RpmIdle + (m_RpmMax - m_RpmIdle) * Math.Pow(Math.Clamp(throttle, 0, 1), 0.6);
			return Math.Max(governed / m_GearRatio, windmill * 0.5);
		}

		if (engineState == GebsEngineState.CRANKING)
		{
			if (crankTime < m_StarterWindTime)
				return windmill;

			return Math.Max(m_StarterRpm / m_GearRatio, windmill);
		}

		return windmill;
	}

	//! The engine's state here: simulated, or as the server sends it.
	int GetEngineState()
	{
		if (IsProxy())
			return m_NetEngineState;

		return m_EngineState;
	}

	bool EngineIsOn()
	{
		return GetEngineState() == GebsEngineState.RUNNING;
	}

	//--------------------------------------------------------------------------
	// the gear
	//--------------------------------------------------------------------------

	bool HasRetractableGear()
	{
		return m_GearUpTime > 0;
	}

	/*!
		The legs fold up and down at a steady rate, worked by the engine's
		hydraulics: up only with the engine running (and never while the wheels
		carry the plane: the squat switch), down slower without it, on the
		emergency release.
	*/
	protected void SimulateGear(float dt)
	{
		if (m_GearUpTime <= 0)
			return;

		float target = 1;
		if (m_FlightControls.m_GearDown)
			target = 0;

		bool hydraulics = !m_GearNeedsEngine || m_EngineState == GebsEngineState.RUNNING;
		if (target > m_GearPhase && (m_GroundLoad > 500 || !hydraulics))
			target = m_GearPhase;

		float time = m_GearUpTime;
		if (target < m_GearPhase)
		{
			time = m_GearDownTime;
			if (!hydraulics)
				time *= 2;
		}

		float step = dt / time;
		if (Math.AbsFloat(target - m_GearPhase) <= step)
			m_GearPhase = target;
		else if (target > m_GearPhase)
			m_GearPhase += step;
		else
			m_GearPhase -= step;
	}

	float GetGearPhase()
	{
		if (IsProxy())
			return m_NetGear;

		return m_GearPhase;
	}

	//! Down and locked.
	bool IsGearDown()
	{
		return GetGearPhase() < 0.02;
	}

	//! Where the lever is.
	bool IsGearSetDown()
	{
		if (IsProxy())
			return m_NetGearDown;

		return m_FlightControls.m_GearDown;
	}

	/*!
		The wheels. The rudder steers the tailwheel - its full swing at a crawl,
		less and less as the speed builds, since at takeoff speed a full-lock
		tailwheel would swap the plane end for end - and, at a crawl, also
		brakes the main wheel on its side, as the toe brakes on the pedals do
		for the tight turns. Standing still at idle with the pedals centred, or
		with nobody at the controls, the brakes hold it. A folding wheel only
		carries the plane down and locked.
	*/
	protected void SimulateGround(float dt)
	{
		GebsPlaneControls c = m_FlightControls;
		vector fwdW = SimDir(m_FlightFwd);
		vector sideW = SimDir(m_FlightSide);
		vector upW = SimDir(m_FlightUp);

		float speed = SimVelocity().Length();
		float kmh = speed * 3.6;
		float swing = Math.Clamp(1 - (kmh - TAIL_STEER_FULL) / m_TailSteerFade, m_TailSteerMin, 1);

		float brake = c.m_Brake;
		bool holding = speed < m_ParkHoldSpeed && c.m_Throttle < 0.05 && Math.AbsFloat(c.m_Rudder) < 0.1;
		if (holding || !CrewDriver())
			brake = 1;

		float toeBrake = m_DiffBrake * Math.Clamp(1 - kmh / m_DiffBrakeFade, 0, 1) * Math.AbsFloat(c.m_Rudder);
		bool locked = m_GearPhase < 0.02;

		m_GroundLoad = 0;
		foreach (GebsPlaneWheel wheel : m_Wheels)
		{
			wheel.m_SteerAngle = wheel.m_Steer * c.m_Rudder * swing;
			wheel.m_Brake = brake;
			if (wheel.m_BrakeSide != 0 && c.m_Rudder * wheel.m_BrakeSide > 0)
				wheel.m_Brake = Math.Max(brake, toeBrake);

			if (wheel.m_Retracts && !locked)
			{
				wheel.Lift();
				continue;
			}

			wheel.Simulate(this, dt, fwdW, sideW, upW);
			if (wheel.m_Retracts)
				m_GroundLoad += wheel.m_Load;
		}
	}

	//! Whether any wheel is on the ground.
	bool IsOnGround()
	{
		foreach (GebsPlaneWheel wheel : m_Wheels)
		{
			if (wheel.m_Contact)
				return true;
		}

		return false;
	}

	//! Standing on its wheels, barely moving, nothing driving the propeller:
	//! the air is left alone, so a parked plane sits still instead of being
	//! nudged every step by the last of the engine's power.
	protected bool IsParked()
	{
		if (m_EnginePower > 0 || SimVelocity().LengthSq() > 2.25)
			return false;

		return IsOnGround();
	}

	//! Fixed pitch: propeller speed from the engine's power and the airspeed
	//! along the axis. It turns faster as the plane speeds up, the starter
	//! turns it over slowly, and a dead engine's propeller windmills.
	float PropRpmFor(float power, float speedFwd, int engineState)
	{
		float windmill = Math.Max(speedFwd, 0) * m_RpmPerSpeed;
		if (engineState == GebsEngineState.CRANKING)
			return Math.Max(m_StarterRpm, windmill - m_RpmIdle);
		if (engineState != GebsEngineState.RUNNING)
			return Math.Max(windmill - m_RpmIdle, 0);

		float open = Math.Clamp((power - m_IdlePower) / Math.Max(1 - m_IdlePower, 0.01), 0, 1);
		return m_RpmIdle + (m_RpmStatic - m_RpmIdle) * Math.Sqrt(open) + windmill * (0.25 + 0.75 * open);
	}

	/*!
		Thrust along the propeller axis, N. A fixed-pitch propeller's falls off
		with speed and goes negative past its pitch speed or with the engine
		dead. A constant-speed one turns the engine's power into thrust at its
		efficiency, up to its static thrust standing still. Above its rated
		altitude the engine gives less.
	*/
	protected float PropThrust(float speedFwd, float rho, float altitude)
	{
		if (m_EnginePower > 0.001)
		{
			float v = Math.Max(speedFwd, 0);
			if (m_ConstantSpeed)
			{
				float shaft = m_EnginePower * m_RatedPower * AltitudePower(altitude);
				float v0 = m_RatedPower * m_PropEfficiency / m_StaticThrust;
				return shaft * m_PropEfficiency / Math.Sqrt(v * v + v0 * v0);
			}

			return m_EnginePower * (rho / SEA_LEVEL_DENSITY) * m_StaticThrust * (1 - v / m_PropPitchSpeed);
		}

		if (speedFwd > 0)
			return -0.5 * rho * speedFwd * speedFwd * m_PropDragArea;

		return 0;
	}

	protected float ControlAngle(GebsAeroSurface s)
	{
		// flown arcade, the assist turns the plane (SimulateArcade): the surfaces only show the stick
		if (m_Arcade)
			return 0;

		GebsPlaneControls c = m_FlightControls;
		switch (s.m_Control)
		{
			case GebsAeroControl.AILERON_LEFT:
				return s.m_ControlGain * (c.m_Aileron + m_AileronRig);
			case GebsAeroControl.AILERON_RIGHT:
				return -s.m_ControlGain * (c.m_Aileron + m_AileronRig);
			case GebsAeroControl.ELEVATOR:
				return -(s.m_ControlGain * c.m_Elevator + s.m_TrimGain * c.m_Trim);
			case GebsAeroControl.RUDDER:
				return -s.m_ControlGain * c.m_Rudder;
		}

		return 0;
	}

	//! A force couple turning the plane by moment (N m) about the axis that
	//! carries the nose along dir: pushes the propeller hub along dir and the
	//! tail the other way. Forces at points, so no handedness to get wrong.
	//! air: one of the air's own (AirForceAt), which the arcade assist takes over.
	protected void ApplyNoseCouple(vector dir, float moment, bool air = true)
	{
		vector nose = SimPoint(m_PropHub);
		vector tail = SimPoint(m_TailPoint);
		float arm = vector.Distance(nose, tail);
		if (arm < 0.5 || moment == 0)
			return;

		vector f = dir * (moment / arm);
		if (air)
		{
			AirForceAt(nose, f);
			AirForceAt(tail, f * -1);
		}
		else
		{
			ApplyForceAt(nose, f);
			ApplyForceAt(tail, f * -1);
		}
	}

	//! This step's centre of gravity and axes, and no turning moments from the air yet (AirForceAt).
	protected void BeginAirMoments()
	{
		m_AirCom = SimPoint(dBodyGetCenterOfMass(this));
		m_AirFwd = SimDir(m_FlightFwd);
		m_AirUp = SimDir(m_FlightUp);
		m_AirSide = SimDir(m_FlightSide);
		m_AirPitch = 0;
		m_AirYaw = 0;
		m_AirRoll = 0;
	}

	/*!
		A force from the air (the propeller, the airframe, a surface): applied,
		and flown arcade, its turning moment about the centre of gravity kept
		for the assist, along the plane's own axes. Each read off as a pilot
		feels it, not by a cross product: a push up ahead of the centre of
		gravity raises the nose, a push forward above it lowers it; a push to
		the right ahead of it swings the nose right, a push forward on the
		right swings it left; a push to the right above it rolls the right wing
		down, a push up on the right rolls it up.
	*/
	void AirForceAt(vector pos, vector force)
	{
		ApplyForceAt(pos, force);
		if (!m_Arcade)
			return;

		vector r = pos - m_AirCom;
		float rf = vector.Dot(r, m_AirFwd);
		float ru = vector.Dot(r, m_AirUp);
		float rs = vector.Dot(r, m_AirSide);
		float ff = vector.Dot(force, m_AirFwd);
		float fu = vector.Dot(force, m_AirUp);
		float fs = vector.Dot(force, m_AirSide);
		m_AirPitch += fu * rf - ff * ru;
		m_AirYaw += fs * rf - ff * rs;
		m_AirRoll += fs * ru - fu * rs;
	}

	//! The nose's pitch rate (up +) and yaw rate (right +), rad/s, from how
	//! fast the propeller hub moves round the tail.
	void TurnRates(out float pitch, out float yaw, bool sim = false)
	{
		vector nose;
		vector tail;
		vector upW;
		vector sideW;
		if (sim)
		{
			nose = SimPoint(m_PropHub);
			tail = SimPoint(m_TailPoint);
			upW = SimDir(m_FlightUp);
			sideW = SimDir(m_FlightSide);
		}
		else
		{
			nose = ModelToWorld(m_PropHub);
			tail = ModelToWorld(m_TailPoint);
			upW = VectorToParent(m_FlightUp);
			sideW = VectorToParent(m_FlightSide);
		}
		float arm = vector.Distance(nose, tail);
		pitch = 0;
		yaw = 0;
		if (arm < 0.5)
			return;

		vector rel = (dBodyGetVelocityAt(this, nose) - dBodyGetVelocityAt(this, tail)) * (1 / arm);
		pitch = vector.Dot(rel, upW);
		yaw = vector.Dot(rel, sideW);
	}

	protected void SimulateAir(float dt)
	{
		vector position = m_SimTm[3];
		float rho = AirDensity(position[1]);

		vector wind = vector.Zero;
		if (m_WindScale > 0 && g_Game.GetWeather())
			wind = g_Game.GetWeather().GetWind() * m_WindScale;

		vector fwdW = SimDir(m_FlightFwd);
		vector upW = SimDir(m_FlightUp);
		vector sideW = SimDir(m_FlightSide);

		// the plane's velocity through the air
		vector air = SimVelocity() - wind;
		float speedFwd = vector.Dot(air, fwdW);

		if (!m_ConstantSpeed)
			m_PropRpm = PropRpmFor(m_EnginePower, speedFwd, m_EngineState);
		m_DebugAirspeed = speedFwd;

		if (IsParked())
		{
			m_Slipstream = 0;
			m_DebugThrust = 0;
			m_DebugLift = 0;
			m_StallMargin = 1;
			m_LoadFactor = 1;
			m_LoadPerRad = 0;
			m_AlphaRate = 0;
			m_HasWingAlpha = false;
			return;
		}

		// propeller: thrust, the slipstream it throws back, its torque
		float thrust = PropThrust(speedFwd, rho, position[1]);
		m_DebugThrust = thrust;
		m_Slipstream = 0;
		if (thrust > 0)
		{
			float disc = 0.25 * Math.PI * m_PropDiameter * m_PropDiameter;
			float v0 = Math.Max(speedFwd, 0);
			m_Slipstream = (Math.Sqrt(v0 * v0 + 2 * thrust / (rho * disc)) - v0) * m_SlipstreamGain;
		}

		AirForceAt(SimPoint(m_PropHub), fwdW * thrust);

		float omega = Math.Max(m_PropRpm * RPM_TO_RAD, 30);
		if (m_TorqueGain > 0 && m_EnginePower > 0.001)
		{
			// rolls the airframe against the propeller: a couple across the cabin
			float shaft = m_EnginePower * m_RatedPower;
			if (m_ConstantSpeed)
				shaft *= AltitudePower(position[1]);
			float torque = shaft / omega * m_TorqueGain * m_PropRotation;
			vector torquePoint = SimPoint(m_BodyPoint);
			vector rollCouple = upW * (torque * 0.5);
			AirForceAt(torquePoint - sideW, rollCouple * -1);
			AirForceAt(torquePoint + sideW, rollCouple);
		}

		// the spinning propeller resists the nose swinging and kicks it round 90
		// degrees on: with one turning clockwise from the cockpit, raising the tail
		// yaws the nose left, a yaw to the right pitches it down
		if (m_PropInertia > 0 && m_PropRpm > 1)
		{
			float spin = m_PropInertia * m_PropRpm * RPM_TO_RAD * m_PropRotation;
			float pitchRate;
			float yawRate;
			TurnRates(pitchRate, yawRate, true);
			ApplyNoseCouple(sideW, spin * pitchRate);
			ApplyNoseCouple(upW, -spin * yawRate);
		}

		// fuselage, gear and struts: drag along each axis
		vector bodyPos = SimPoint(m_BodyPoint);
		vector bodyAir = dBodyGetVelocityAt(this, bodyPos) - wind;
		float bu = vector.Dot(bodyAir, fwdW);
		float bs = vector.Dot(bodyAir, sideW);
		float bn = vector.Dot(bodyAir, upW);
		float cdaFwd = m_BodyCdaFwd + m_GearCda * (1 - m_GearPhase);
		vector bodyDrag = fwdW * (bu * Math.AbsFloat(bu) * cdaFwd) + sideW * (bs * Math.AbsFloat(bs) * m_BodyCdaSide) + upW * (bn * Math.AbsFloat(bn) * m_BodyCdaUp);
		AirForceAt(bodyPos, bodyDrag * (-0.5 * rho));

		// ground effect: less induced drag within a span of the ground, or of the water over it
		float inducedScale = 1;
		if (m_GroundEffect && m_Span > 0)
		{
			float ground = Math.Max(g_Game.SurfaceY(position[0], position[2]), position[1] + GebsPlaneFloats.WaterDepthAt(position));
			float height = position[1] - ground;
			float r = 16 * Math.Max(height, 0.1) / m_Span;
			inducedScale = r * r / (1 + r * r);
		}

		float wingLift = 0;
		float wingArea = 0;
		float wingAlpha = 0;
		float liftUp = 0;
		float slatAlphaLeft = -1;
		float slatAlphaRight = -1;
		// for a held pull key (PullRoom): the wing panel nearest its stall, the g the lift pulls, and how
		// many more g a radian more would pull
		float stallMargin = 1;
		float liftNormal = 0;
		float liftSlope = 0;

		foreach (GebsAeroSurface s : m_FlightSurfaces)
		{
			vector pointW = SimPoint(s.m_Point);
			vector normalW = SimDir(s.m_Normal);
			vector spanW = SimDir(s.m_Span);

			vector v = dBodyGetVelocityAt(this, pointW) - wind;
			if (s.m_Slipstream > 0)
				v = v + fwdW * (m_Slipstream * s.m_Slipstream);
			if (s.m_Swirl)
				v = v - sideW * (m_Slipstream * m_SwirlGain * m_PropRotation);

			// only the flow across the span lifts
			v = v - spanW * vector.Dot(v, spanW);
			float v2 = v.LengthSq();
			if (v2 < 0.25)
				continue;

			float speed = Math.Sqrt(v2);
			vector dir = v * (1 / speed);
			float u = vector.Dot(dir, fwdW);
			float w = vector.Dot(dir, normalW);

			float alpha = Math.Atan2(-w, u) + s.m_Incidence + ControlAngle(s);
			if (s.m_Downwash)
				alpha -= m_WingCL * m_DownwashGain;
			alpha = WrapAngle(alpha);
			s.m_Alpha = alpha;

			float flaps = 0;
			if (s.m_FlapCL != 0)
				flaps = m_FlightControls.m_Flaps;

			float induced = 1;
			if (s.m_Wing)
				induced = inducedScale;

			float slat = 0;
			if (s.m_SlatGroup == 1)
			{
				slat = m_SlatLeft;
				slatAlphaLeft = Math.Max(slatAlphaLeft, alpha);
			}
			else if (s.m_SlatGroup == 2)
			{
				slat = m_SlatRight;
				slatAlphaRight = Math.Max(slatAlphaRight, alpha);
			}

			float cl;
			float cd;
			s.Coefficients(alpha, flaps, induced, slat * m_SlatStall, slat * m_SlatCL, cl, cd);

			float q = 0.5 * rho * v2 * s.m_Area;
			vector liftDir = normalW * u - fwdW * w;
			vector liftForce = liftDir * (q * cl);
			AirForceAt(pointW, liftForce - dir * (q * cd));
			liftUp += liftForce[1];
			liftNormal += vector.Dot(liftForce, upW);

			// the airfoil's own pitching moment, as a couple along the chord
			float cm = s.m_Cm0 + flaps * s.m_FlapCm;
			if (cm != 0 && s.m_Chord > 0 && Math.AbsFloat(alpha) < Math.PI * 0.5)
			{
				float pitchCouple = q * cm;	// moment / chord
				vector half = fwdW * (s.m_Chord * 0.5);
				AirForceAt(pointW + half, normalW * pitchCouple);
				AirForceAt(pointW - half, normalW * -pitchCouple);
			}

			if (s.m_Wing)
			{
				wingLift += cl * s.m_Area;
				wingArea += s.m_Area;
				wingAlpha += alpha * s.m_Area;
				float panelStall = s.m_StallPos - flaps * GebsAeroSurface.FLAP_STALL_SHIFT + slat * m_SlatStall;
				stallMargin = Math.Min(stallMargin, panelStall - alpha);
				liftSlope += q * s.m_CLa;
			}
		}

		float weight = Math.Max(dBodyGetMass(this), 1) * 9.81;
		m_StallMargin = stallMargin;
		m_LoadFactor = liftNormal / weight;
		m_LoadPerRad = liftSlope / weight;

		// the slats: pulled out by the airflow round the leading edge as the angle
		// of attack rises, each wing's on its own (so one wing's can open in a
		// turn or a yaw while the other's stay shut); they act from the next step
		if (m_SlatStall > 0 || m_SlatCL > 0)
		{
			float k = Math.Clamp(dt / m_SlatTime, 0, 1);
			m_SlatLeft += (SlatTarget(slatAlphaLeft) - m_SlatLeft) * k;
			m_SlatRight += (SlatTarget(slatAlphaRight) - m_SlatRight) * k;
		}

		if (wingArea > 0)
		{
			m_WingCL = wingLift / wingArea;
			m_DebugAlpha = wingAlpha / wingArea * Math.RAD2DEG;

			// how fast the wing's angle of attack is rising, smoothed
			float wingAlphaNow = wingAlpha / wingArea;
			if (m_HasWingAlpha && dt > 0)
				m_AlphaRate += ((wingAlphaNow - m_WingAlpha) / dt - m_AlphaRate) * Math.Clamp(dt / ALPHA_RATE_TIME, 0, 1);
			m_WingAlpha = wingAlphaNow;
			m_HasWingAlpha = true;
		}
		m_DebugLift = liftUp;
	}

	//! kg m2 about an axis through the centre of gravity, from the body's own inertia (world space this step).
	//! (inv0..inv2: the rows of dBodyGetInvInertiaTensorWorld)
	protected static float AxisInertia(vector inv0, vector inv1, vector inv2, vector axis)
	{
		vector ia = Vector(vector.Dot(inv0, axis), vector.Dot(inv1, axis), vector.Dot(inv2, axis));
		return 1 / Math.Max(vector.Dot(axis, ia), 0.000001);
	}

	/*!
		Arcade flying (GebsFlight Arcade): the stick and pedals ask for turn
		rates, and the assist turns the plane at them, as Alien Invasion's UFO's
		drive does, taking over the air's own turning moments (AirForceAt) while
		the air still lifts and drags. So the nose goes where it is pointed and
		the wings lift the plane along after it.
		- Stick let go, the wings roll level and a climb or a dive eases level:
		  hands off, it flies straight and level.
		- Banked, the nose follows the turn round and keeps up (a level turn
		  needs no pull), and it turns into any sideslip by itself.
		- It won't stall: closing on the stall the nose comes down, so a held
		  pull at low speed mushes along instead of dropping a wing.
		- It flies the plane from speedMin, fully from speedFull; below that the
		  floats or wheels have it. On the water the pedals turn it at any speed
		  and the stick doesn't roll it.
		Everything from this step's body and the pilot's controls, on the
		server and the pilot's machine alike, through ApplyForceAt couples.
	*/
	protected void SimulateArcade(float dt)
	{
		m_DebugArcadeAuthority = 0;
		if (!CrewDriver() || IsDamageDestroyed())
			return;

		GebsPlaneControls c = m_FlightControls;
		vector fwdW = m_AirFwd;
		vector upW = m_AirUp;
		vector sideW = m_AirSide;
		vector vel = SimVelocity();
		float speed = vel.Length();
		float airspeed = vector.Dot(vel, fwdW);

		float authority = Math.Clamp((airspeed - m_ArcadeSpeedMin) / (m_ArcadeSpeedFull - m_ArcadeSpeedMin), 0, 1);
		bool afloat = m_FloatGear && m_FloatGear.IsWet();
		bool down = afloat || IsOnGround();

		// the attitude: the bank (right wing down +), the flight path's climb, the sideslip (the air from the right +)
		float bank = Math.Atan2(-sideW[1], upW[1]);
		float climb = 0;
		float slip = 0;
		if (speed > 1)
		{
			climb = Math.Asin(Math.Clamp(vel[1] / speed, -1, 1));
			slip = Math.Atan2(vector.Dot(vel, sideW), Math.Max(airspeed, 1));
		}

		// a level turn at this bank: its rate round the vertical, as the nose pitches and yaws with it
		float pitchTurn = 0;
		float yawTurn = 0;
		if (upW[1] > 0.2)
		{
			float turn = 9.81 * Math.Tan(Math.Clamp(bank, -1.3, 1.3)) / Math.Max(speed, m_ArcadeSpeedFull);
			pitchTurn = turn * Math.Sin(bank);
			yawTurn = turn * Math.Cos(bank);
		}

		// roll: the stick at its rate, slowing to a stop at the bank limit; let go, back to wings level. Afloat or on
		// the ground, the stick doesn't roll it
		float aileron = c.m_Aileron;
		if (down)
			aileron = 0;
		float rollWant = aileron * m_ArcadeRollRate;
		if (m_ArcadeBankLimit > 0 && aileron * bank > 0)
			rollWant *= Math.Clamp((m_ArcadeBankLimit - Math.AbsFloat(bank)) / ARCADE_BANK_BAND, 0, 1);
		rollWant += (1 - Math.AbsFloat(aileron)) * Math.Clamp(-m_ArcadeLevelRoll * bank, -m_ArcadeLevelRollMax, m_ArcadeLevelRollMax);

		// pitch: the stick at its rate; let go, the flight path eases level; both on top of a turn's. Never into the
		// stall: short of the margin a pull slows, inside it the nose comes down
		float elevator = c.m_Elevator;
		float pitchWant = elevator * m_ArcadePitchRate + (1 - Math.AbsFloat(elevator)) * Math.Clamp(-m_ArcadeLevelPitch * climb, -m_ArcadeLevelPitchMax, m_ArcadeLevelPitchMax) + pitchTurn;
		// afloat, it holds the floats at their planing attitude instead (waterPitch), and the stick tips it between
		// level and the most the afterbody allows (waterPitchMax): so the throttle alone gets it up on the step and
		// off, and a held pull can't drag the floats' tails
		if (afloat)
		{
			float attitude = Math.Asin(Math.Clamp(fwdW[1], -1, 1));
			float waterWant = m_ArcadeWaterPitch + Math.Max(elevator, 0) * (m_ArcadeWaterPitchMax - m_ArcadeWaterPitch) + Math.Min(elevator, 0) * m_ArcadeWaterPitch;
			pitchWant = Math.Clamp(m_ArcadeWaterPitchGain * (waterWant - attitude), -m_ArcadePitchRate, m_ArcadePitchRate);
		}
		else
		{
			// flying, the flight path climbs no steeper than the speed can keep up (climbMax from climbSpeed, none by
			// climbSpeedMin), so a held pull settles into a steady climb instead of zooming up, slowing and dropping
			// the nose over and over; and dives no steeper than diveMax
			float climbRoom = m_ArcadeClimbMax * Math.Clamp((airspeed - m_ArcadeClimbSpeedMin) / (m_ArcadeClimbSpeed - m_ArcadeClimbSpeedMin), 0, 1);
			pitchWant = Math.Min(pitchWant, pitchTurn + m_ArcadeClimbGain * (climbRoom - climb));
			pitchWant = Math.Max(pitchWant, pitchTurn + m_ArcadeClimbGain * (-m_ArcadeDiveMax - climb));
		}
		float stallRoom = m_StallMargin - m_ArcadeStallMargin;
		if (stallRoom < 0)
			pitchWant = Math.Min(pitchWant, m_ArcadeStallGain * stallRoom);
		else if (pitchWant > 0 && stallRoom < ARCADE_STALL_BAND)
			pitchWant *= stallRoom / ARCADE_STALL_BAND;

		// yaw: the pedals, the turn's, and into any sideslip. On the water the pedals have it at any speed
		float yawWant = authority * (yawTurn + m_ArcadeSlip * slip) + c.m_Rudder * m_ArcadeYawRate;
		float yawAuthority = authority;
		if (afloat)
		{
			yawWant = authority * (yawTurn + m_ArcadeSlip * slip) + c.m_Rudder * Math.Lerp(m_ArcadeWaterYawRate, m_ArcadeYawRate, authority);
			yawAuthority = 1;
		}

		// the plane's turn rates now
		float pitchRate;
		float yawRate;
		TurnRates(pitchRate, yawRate, true);
		float rollRate = vector.Dot(dBodyGetVelocityAt(this, m_AirCom - sideW) - dBodyGetVelocityAt(this, m_AirCom + sideW), upW) * 0.5;

		vector inv[3];
		dBodyGetInvInertiaTensorWorld(this, inv);
		float rollAccel = Math.Clamp((rollWant - rollRate) * m_ArcadeGain, -m_ArcadeAccel, m_ArcadeAccel);
		float pitchAccel = Math.Clamp((pitchWant - pitchRate) * m_ArcadeGain, -m_ArcadeAccel, m_ArcadeAccel);
		float yawAccel = Math.Clamp((yawWant - yawRate) * m_ArcadeGain, -m_ArcadeAccel, m_ArcadeAccel);
		float rollMoment = (AxisInertia(inv[0], inv[1], inv[2], fwdW) * rollAccel - m_ArcadeTakeOver * m_AirRoll) * authority;
		float pitchMoment = (AxisInertia(inv[0], inv[1], inv[2], sideW) * pitchAccel - m_ArcadeTakeOver * m_AirPitch) * authority;
		float yawMoment = (AxisInertia(inv[0], inv[1], inv[2], upW) * yawAccel - m_ArcadeTakeOver * m_AirYaw) * yawAuthority;

		ApplyNoseCouple(upW, pitchMoment, false);
		ApplyNoseCouple(sideW, yawMoment, false);
		vector rollCouple = upW * (rollMoment * 0.5);
		ApplyForceAt(m_AirCom - sideW, rollCouple);
		ApplyForceAt(m_AirCom + sideW, rollCouple * -1);

		m_DebugArcadeAuthority = authority;
		m_DebugArcadeWant = Vector(rollWant, pitchWant, yawWant) * Math.RAD2DEG;
		m_DebugArcadeRate = Vector(rollRate, pitchRate, yawRate) * Math.RAD2DEG;
		m_DebugArcadeBank = bank * Math.RAD2DEG;
		m_DebugArcadeClimb = climb * Math.RAD2DEG;
	}

	protected float SlatTarget(float alpha)
	{
		if (alpha <= m_SlatAlpha)
			return 0;

		float t = Math.Clamp((alpha - m_SlatAlpha) / (m_SlatFull - m_SlatAlpha), 0, 1);
		return t * t * (3 - 2 * t);
	}

	//--------------------------------------------------------------------------
	// server: fuel, damage, what other players see
	//--------------------------------------------------------------------------

	override void EOnPostSimulate(IEntity other, float timeSlice)
	{
		super.EOnPostSimulate(other, timeSlice);

		if (!g_Game.IsServer())
			return;

		if (m_EngineState == GebsEngineState.RUNNING)
		{
			float open = Math.Clamp((m_EnginePower - m_IdlePower) / Math.Max(1 - m_IdlePower, 0.01), 0, 1);
			m_FuelToBurn += (m_FuelBurnIdle + (m_FuelBurnFull - m_FuelBurnIdle) * open) / 3600 * timeSlice;
			if (m_FuelToBurn >= 0.01)
			{
				m_Fuel = Math.Max(m_Fuel - m_FuelToBurn, 0);
				m_FuelToBurn = 0;
			}
		}

		if (m_RpmOverspeed > 0 && GetEngineRpm() > m_RpmOverspeed)
			AddHealth("Engine", "Health", -GetMaxHealth("Engine", "") * 0.02 * timeSlice);

		m_SyncTimer += timeSlice;
		if (m_SyncTimer >= SYNC_INTERVAL)
		{
			m_SyncTimer = 0;
			SyncState();
		}
	}

	//! Whether a value has moved far enough from what was last sent to send it again (or come to rest at 0).
	protected static bool Moved(float sent, float value, float step)
	{
		return Math.AbsFloat(sent - value) >= step || (value == 0 && sent != 0);
	}

	protected void SyncState()
	{
		GebsPlaneControls c = m_FlightControls;
		bool dirty = false;

		if (Moved(m_NetElevator, c.m_Elevator, SYNC_STEP))
		{
			m_NetElevator = c.m_Elevator;
			dirty = true;
		}
		if (Moved(m_NetAileron, c.m_Aileron, SYNC_STEP))
		{
			m_NetAileron = c.m_Aileron;
			dirty = true;
		}
		if (Moved(m_NetRudder, c.m_Rudder, SYNC_STEP))
		{
			m_NetRudder = c.m_Rudder;
			dirty = true;
		}
		if (Moved(m_NetFlaps, c.m_Flaps, SYNC_STEP))
		{
			m_NetFlaps = c.m_Flaps;
			dirty = true;
		}
		if (Moved(m_NetTrim, c.m_Trim, SYNC_STEP))
		{
			m_NetTrim = c.m_Trim;
			dirty = true;
		}
		if (Moved(m_NetThrottle, c.m_Throttle, SYNC_STEP))
		{
			m_NetThrottle = c.m_Throttle;
			dirty = true;
		}
		if (Moved(m_NetPower, m_EnginePower, SYNC_STEP))
		{
			m_NetPower = m_EnginePower;
			dirty = true;
		}
		float rpm = GetEngineRpm();
		if (Moved(m_NetRpm, rpm, 20))
		{
			m_NetRpm = rpm;
			dirty = true;
		}
		if (Moved(m_NetGear, m_GearPhase, SYNC_STEP) || ((m_GearPhase == 0 || m_GearPhase == 1) && m_NetGear != m_GearPhase))
		{
			m_NetGear = m_GearPhase;
			dirty = true;
		}
		if (Moved(m_NetSlatLeft, m_SlatLeft, 0.05))
		{
			m_NetSlatLeft = m_SlatLeft;
			dirty = true;
		}
		if (Moved(m_NetSlatRight, m_SlatRight, 0.05))
		{
			m_NetSlatRight = m_SlatRight;
			dirty = true;
		}
		if (m_NetGearDown != c.m_GearDown)
		{
			m_NetGearDown = c.m_GearDown;
			dirty = true;
		}
		if (m_NetSwitch != c.m_Ignition)
		{
			m_NetSwitch = c.m_Ignition;
			dirty = true;
		}
		if (m_NetEngineState != m_EngineState)
		{
			m_NetEngineState = m_EngineState;
			dirty = true;
		}
		if (Math.AbsFloat(m_FuelSent - m_Fuel) >= FUEL_SYNC_STEP || (m_Fuel == 0 && m_FuelSent != 0))
		{
			m_FuelSent = m_Fuel;
			dirty = true;
		}

		if (dirty)
			SetSynchDirty();
	}

	//! A knock harder than crashImpulse damages the part it hits: the engine
	//! near the nose, the airframe anywhere else.
	override protected void EOnContact(IEntity other, Contact extra)
	{
		super.EOnContact(other, extra);

		if (!g_Game.IsServer() || m_CrashDamage <= 0 || extra.Impulse < m_CrashImpulse)
			return;

		vector hitPos = WorldToModel(extra.Position);
		string zone = "Chassis";
		if (MemoryPointExists("dmgZone_engine") && vector.Distance(hitPos, GetMemoryPointPos("dmgZone_engine")) < 1.0)
			zone = "Engine";

		ProcessDirectDamage(DamageType.CUSTOM, null, zone, "EnviroDmg", hitPos, (extra.Impulse - m_CrashImpulse) * m_CrashDamage);
		dBodyActive(this, ActiveState.ACTIVE);
	}

	//--------------------------------------------------------------------------
	// fuel: its own tank, filled from a jerrycan at the "refill" point by
	// vanilla's refuel action, which asks any Transport through these
	//--------------------------------------------------------------------------

	float GetFuel()
	{
		return m_Fuel;
	}

	//! The filler is high on the airframe: the refuel action measures from the
	//! player's feet, so it reaches further than a car's.
	override float GetActionDistanceFuel()
	{
		return 2.5;
	}

	float GetFuelCapacity()
	{
		return m_FuelCapacity;
	}

	override int GetFluidCapacityScript(ETransportFluid fluidType)
	{
		if (fluidType == ETransportFluid.FUEL)
			return ConfigGetInt("fuelCapacity");

		return 0;
	}

	override float GetFluidFractionScript(ETransportFluid fluidType)
	{
		if (fluidType == ETransportFluid.FUEL)
			return m_Fuel / m_FuelCapacity;

		return 0;
	}

	override void FillScript(ETransportFluid fluidType, float amount)
	{
		if (fluidType != ETransportFluid.FUEL || !g_Game.IsServer())
			return;

		m_Fuel = Math.Clamp(m_Fuel + amount, 0, m_FuelCapacity);
		m_FuelSent = m_Fuel;
		SetSynchDirty();
	}

	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);

		ctx.Write(STORE_VERSION);
		ctx.Write(m_Fuel);
		ctx.Write(m_FlightControls.m_GearDown);
		float door = 0;
		if (m_DoorSource != "")
			door = GetAnimationPhase(m_DoorSource);
		ctx.Write(door);
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version))
			return false;

		int mine;
		if (!ctx.Read(mine))
			return false;
		if (!ctx.Read(m_Fuel))
			return false;

		bool gearDown = true;
		float door = 0;
		if (mine >= 2)
		{
			if (!ctx.Read(gearDown))
				return false;
			if (!ctx.Read(door))
				return false;
		}

		LoadFlightModel();
		m_Fuel = Math.Clamp(m_Fuel, 0, m_FuelCapacity);
		m_FuelSent = m_Fuel;
		m_FlightControls.m_GearDown = gearDown;
		m_GearPhase = 1;
		if (gearDown || m_GearUpTime <= 0)
			m_GearPhase = 0;
		if (m_DoorSource != "")
			SetAnimationPhase(m_DoorSource, door);
		return true;
	}

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();

		m_NetGear = m_GearPhase;
		m_NetGearDown = m_FlightControls.m_GearDown;
		SetSynchDirty();
	}

	//--------------------------------------------------------------------------
	// for the instrument panel (scripts/5_mission/gui/gebsplanehud.c)
	//--------------------------------------------------------------------------

	float GetAirspeed()
	{
		return vector.Dot(GetVelocity(this), VectorToParent(m_FlightFwd));
	}

	//! The propeller's rpm here.
	float GetPropRpm()
	{
		if (IsProxy())
			return m_NetRpm / m_GearRatio;

		return m_PropRpm;
	}

	//! The engine's rpm here (the propeller's times the reduction gear).
	float GetEngineRpm()
	{
		if (IsProxy())
			return m_NetRpm;

		return m_PropRpm * m_GearRatio;
	}

	float GetThrottle()
	{
		if (IsProxy())
			return m_NetThrottle;

		return m_FlightControls.m_Throttle;
	}

	float GetFlaps()
	{
		if (IsProxy())
			return m_NetFlaps;

		return m_FlightControls.m_Flaps;
	}

	float GetFlapDegrees()
	{
		return m_FlapDegrees;
	}

	float GetTrim()
	{
		if (IsProxy())
			return m_NetTrim;

		return m_FlightControls.m_Trim;
	}

	//! The manifold pressure, ata: what the throttle opens, held by the
	//! supercharger up to its rated altitude. 0 for a plane without a gauge for it.
	float GetBoost()
	{
		if (m_BoostMax <= 0)
			return 0;

		float ambient = AirDensity(GetPosition()[1]) / SEA_LEVEL_DENSITY;
		if (GetEngineState() != GebsEngineState.RUNNING)
			return ambient;

		float boost = m_BoostIdle + (m_BoostMax - m_BoostIdle) * GetThrottle();
		return boost * Math.Min(AltitudePower(GetPosition()[1]), 1);
	}

	bool IsBraking()
	{
		foreach (GebsPlaneWheel wheel : m_Wheels)
		{
			if (wheel.m_Brake > 0.5 && wheel.m_Contact)
				return true;
		}

		return false;
	}

	//--------------------------------------------------------------------------
	// test readout (GebsFlight debugLog=1)
	//--------------------------------------------------------------------------

	protected static string Num(float value, int decimals = 0)
	{
		float scale = Math.Pow(10, decimals);
		float rounded = Math.Round(value * scale) / scale;
		return rounded.ToString();
	}

	protected static string YesNo(bool value)
	{
		if (value)
			return "yes";

		return "no";
	}

	//! Which machine a test line comes from.
	protected string DebugMachine()
	{
		if (g_Game.IsServer())
			return "server";

		return "pilot";
	}

	//! A test line when something happens to the engine (the engine actions).
	void DebugNote(string what)
	{
		if (m_DebugLog)
			Print("GebsPlanes " + GetType() + " (" + DebugMachine() + "): " + what);
	}

	/*!
		Once a second while someone is at the controls or the engine is on, two
		lines in the server's and the pilot's script logs: who simulates the
		plane and how (steps and moves in the last second, awake), the switches;
		then what it is doing - engine, throttle, power and revs; airspeed and
		ground speed; thrust, and lift against weight with the wing's angle of
		attack and the slats; pitch and compass heading; each wheel and the gear;
		and the controls.
	*/
	protected void DebugReadout(float dt)
	{
		m_DebugTimer += dt;
		if (m_DebugTimer < 1)
			return;

		m_DebugTimer = 0;
		int steps = m_DebugSteps;
		int movesOut = m_DebugMovesOut;
		int movesIn = m_DebugMovesIn;
		int rewinds = m_DebugRewinds;
		m_DebugSteps = 0;
		m_DebugMovesOut = 0;
		m_DebugMovesIn = 0;
		m_DebugRewinds = 0;

		if (!g_Game.IsServer() && !IsLocalPilot())
			return;
		if (!CrewDriver() && m_EngineState == GebsEngineState.OFF)
			return;

		string net = "GebsPlanes " + GetType() + " (" + DebugMachine() + "): owner " + YesNo(IsOwner()) + ", proxy " + YesNo(IsProxy());
		net += ", strategy " + typename.EnumToString(NetworkMoveStrategy, GetNetworkMoveStrategy()) + ", awake " + YesNo(dBodyIsActive(this));
		// corrections: the server put the pilot's plane back where its own copy was (each one a jump)
		net += " | last second: " + steps + " steps, " + movesOut + " moves sent, " + movesIn + " moves flown, " + rewinds + " corrections";
		net += " | ignition " + YesNo(m_FlightControls.m_Ignition) + ", starter " + YesNo(m_FlightControls.m_Starter) + ", latched " + YesNo(m_FlightControls.m_StarterLatched);
		Print(net);

		string wheelState;
		foreach (GebsPlaneWheel wheel : m_Wheels)
		{
			if (wheelState != "")
				wheelState += ", ";
			wheelState += wheel.m_Name + " ";
			if (!wheel.IsFitted(this))
				wheelState += "missing";
			else if (wheel.m_Contact)
				wheelState += Num(wheel.m_Load) + " N";
			else
				wheelState += "up";
			if (wheel.m_Brake > 0)
				wheelState += " brake " + Num(wheel.m_Brake, 2);
		}

		vector dir = GetDirection();
		float heading = Math.Atan2(dir[0], dir[2]) * Math.RAD2DEG;
		if (heading < 0)
			heading += 360;

		string engine = typename.EnumToString(GebsEngineState, m_EngineState);
		string line = "GebsPlanes " + GetType() + " (" + DebugMachine() + "): engine " + engine + " " + Num(m_CrankTime, 1) + " s, throttle " + Num(m_FlightControls.m_Throttle * 100) + "%, power " + Num(m_EnginePower * 100) + "%, " + Num(GetEngineRpm()) + " rpm, fuel " + Num(m_Fuel, 1) + " L";
		line += " | airspeed " + Num(m_DebugAirspeed * 3.6) + " km/h, ground speed " + Num(GetVelocity(this).Length() * 3.6) + " km/h, height " + Num(GetPosition()[1] - g_Game.SurfaceY(GetPosition()[0], GetPosition()[2]), 1) + " m";
		line += " | thrust " + Num(m_DebugThrust) + " N, lift " + Num(m_DebugLift) + " of " + Num(dBodyGetMass(this) * 9.81) + " N, wing AoA " + Num(m_DebugAlpha, 1) + ", slats " + Num(m_SlatLeft, 2) + "/" + Num(m_SlatRight, 2);
		line += ", stall " + Num(m_StallMargin * Math.RAD2DEG, 1) + " deg off, " + Num(m_LoadFactor, 1) + " g";
		if (IsLocalPilot())
			line += ", pull room " + Num(m_DebugPullRoom, 2);
		line += " | pitch " + Num(GetOrientation()[1], 1) + ", roll " + Num(GetOrientation()[2], 1) + ", heading " + Num(heading);
		line += " | gear " + Num(m_GearPhase, 2) + " (lever " + YesNo(m_FlightControls.m_GearDown) + "), wheels " + wheelState;
		if (m_FloatGear)
		{
			// each float's slices in the water and what the water pushes it up with; the water's drag; how far
			// the origin is above the water, and how this machine's GetWaterDepth reads
			line += " | floats";
			foreach (GebsPlaneFloat fl : m_FloatGear.m_Floats)
				line += " " + fl.m_Name + " " + fl.m_Wet + " wet " + Num(fl.m_Load) + " N";
			line += ", water drag " + Num(m_FloatGear.m_Drag) + " N, " + Num(-GebsPlaneFloats.WaterDepthAt(GetPosition()), 2) + " m above the water (" + GebsPlaneFloats.DepthModeName() + ")";
		}
		if (m_Arcade)
		{
			// the assist: how much it flies the plane, the roll, pitch and yaw rates it asks for and the plane's
			vector want = m_DebugArcadeWant;
			vector rate = m_DebugArcadeRate;
			line += " | arcade " + Num(m_DebugArcadeAuthority, 2) + ", bank " + Num(m_DebugArcadeBank) + ", climb " + Num(m_DebugArcadeClimb, 1);
			line += ", roll " + Num(want[0]) + "/" + Num(rate[0]) + ", pitch " + Num(want[1]) + "/" + Num(rate[1]) + ", yaw " + Num(want[2]) + "/" + Num(rate[2]) + " deg/s";
		}
		line += " | elevator " + Num(m_FlightControls.m_Elevator, 2) + ", aileron " + Num(m_FlightControls.m_Aileron, 2) + ", rudder " + Num(m_FlightControls.m_Rudder, 2) + ", trim " + Num(m_FlightControls.m_Trim, 2) + ", flaps " + Num(m_FlightControls.m_Flaps, 2) + ", brake " + Num(m_FlightControls.m_Brake, 2);
		Print(line);
	}

	//--------------------------------------------------------------------------
	// every frame: awake, a new pilot, what you see and hear
	//--------------------------------------------------------------------------

	override void EOnFrame(IEntity other, float timeSlice)
	{
		if (!g_Game.IsServer())
		{
			bool pilot = IsLocalPilot();
			if (pilot && !m_TookOver)
				TakeOver();
			m_TookOver = pilot;
		}

		// every frame, asleep or not: a sleeping body gets no simulation steps to do this in
		UpdateSleep();
		if (m_DebugLog)
			DebugReadout(timeSlice);

		#ifndef SERVER
		UpdateVisuals(timeSlice);
		UpdateCockpit(timeSlice);
		UpdateSound(timeSlice);
		#endif
	}

	/*!
		This client has just taken the seat. Until now the server flew the
		plane (it sat where it was left, or someone else had it), so this
		machine's engine, propeller, gear and controls are stale: start from the
		server's. The ignition matters most, as it isn't in the move.
	*/
	protected void TakeOver()
	{
		GebsPlaneControls c = m_FlightControls;
		c.m_Ignition = m_NetSwitch;
		c.m_GearDown = m_NetGearDown;
		c.m_Flaps = m_NetFlaps;
		c.m_Trim = m_NetTrim;
		c.m_Throttle = m_NetThrottle;
		m_FlapNotch = Math.Round(m_NetFlaps * m_FlapNotches);
		m_EngineState = m_NetEngineState;
		m_EnginePower = m_NetPower;
		m_PropRpm = m_NetRpm / m_GearRatio;
		m_GearPhase = m_NetGear;
		m_SlatLeft = m_NetSlatLeft;
		m_SlatRight = m_NetSlatRight;
		c.m_Starter = false;
		c.m_StarterLatched = false;
		m_CrankTime = 0;

		DebugNote("took over, engine " + typename.EnumToString(GebsEngineState, m_EngineState) + ", ignition " + YesNo(c.m_Ignition));
	}

	/*!
		A body that has sat still for a moment falls asleep, and asleep it gets
		no simulation steps: the engine wouldn't start nor the plane fly. On the
		machines that simulate it (the server, the pilot's), it is held awake
		while someone is at the controls, the engine is on or it is still
		moving, and may sleep again once it's parked and empty. Checked every
		frame, not just when it should change: the game puts the body to sleep
		itself when the plane changes hands (the UFO's server stopped simulating
		and taking the pilot's moves when he took the seat back, Cole
		2026-10-28).
	*/
	protected void UpdateSleep()
	{
		bool simulates = g_Game.IsServer() || IsLocalPilot();
		bool moving = GetVelocity(this).LengthSq() > AWAKE_SPEED * AWAKE_SPEED;
		bool awake = simulates && (CrewDriver() != null || m_EngineState != GebsEngineState.OFF || moving);
		if (awake)
		{
			if (!m_KeptAwake || !dBodyIsActive(this))
				WakeUp();
			return;
		}

		if (m_KeptAwake)
		{
			m_KeptAwake = false;
			dBodyActive(this, ActiveState.ACTIVE);
		}
	}

	protected void WakeUp()
	{
		m_KeptAwake = true;
		dBodyActive(this, ActiveState.ALWAYS_ACTIVE);
	}

	protected void SetPhase(string source, float phase)
	{
		if (Math.AbsFloat(GetAnimationPhase(source) - phase) > 0.001)
			SetAnimationPhase(source, phase);
	}

	protected void UpdateVisuals(float dt)
	{
		if (!m_FlightLoaded)
			return;

		float elevator = m_NetElevator;
		float aileron = m_NetAileron;
		float rudder = m_NetRudder;
		float flaps = m_NetFlaps;
		float power = m_NetPower;
		float slatLeft = m_NetSlatLeft;
		float slatRight = m_NetSlatRight;
		if (!IsProxy())
		{
			elevator = m_FlightControls.m_Elevator;
			aileron = m_FlightControls.m_Aileron;
			rudder = m_FlightControls.m_Rudder;
			flaps = m_FlightControls.m_Flaps;
			power = m_EnginePower;
			slatLeft = m_SlatLeft;
			slatRight = m_SlatRight;
		}

		// the controls: 0.5 is neutral
		SetPhase("elevator", 0.5 + 0.5 * elevator);
		SetPhase("aileron", 0.5 + 0.5 * aileron);
		SetPhase("rudder", 0.5 + 0.5 * rudder);
		SetPhase("flaps", flaps);
		if (m_HasTrimAnim)
			SetPhase("trim", 0.5 + 0.5 * GetTrim());
		if (m_SlatStall > 0)
		{
			SetPhase("slat_l", slatLeft);
			SetPhase("slat_r", slatRight);
		}
		if (m_GearUpTime > 0)
			SetPhase("gear", GetGearPhase());

		vector fwdW = VectorToParent(m_FlightFwd);
		float speedFwd = vector.Dot(GetVelocity(this), fwdW);
		if (IsProxy())
		{
			if (m_ConstantSpeed)
				m_PropRpm = m_NetRpm / m_GearRatio;
			else
				m_PropRpm = PropRpmFor(power, speedFwd, m_NetEngineState);
		}

		// the blades turn until they blur into a disc
		m_PropTurns += m_PropRpm / 60 * dt;
		m_PropTurns -= Math.Floor(m_PropTurns);
		if (m_PropRpm > PROP_BLUR_RPM)
		{
			SetPhase("prop_blur", 1);	// hides the blades
			SetPhase("prop_still", 0);	// shows the disc
		}
		else
		{
			SetPhase("prop_blur", 0);
			SetPhase("prop_still", 1);
			SetAnimationPhase("propeller", m_PropTurns);
		}

		// the wheels; other players' planes steer the tailwheel from the rudder they're sent
		float kmh = GetVelocity(this).Length() * 3.6;
		float swing = Math.Clamp(1 - (kmh - TAIL_STEER_FULL) / m_TailSteerFade, m_TailSteerMin, 1);
		foreach (GebsPlaneWheel wheel : m_Wheels)
		{
			if (IsProxy())
				wheel.m_SteerAngle = wheel.m_Steer * rudder * swing;
			wheel.UpdateVisuals(this, dt, !IsProxy(), fwdW);
		}

		// the floats' spray (the water rudders turn with the "rudder" source above)
		if (m_FloatGear)
			m_FloatGear.UpdateEffects(this, dt);
	}

	//! A client is near enough to see the panel: the pilot, or a camera within 20 m.
	protected bool IsPanelSeen()
	{
		if (IsLocalPilot())
			return true;

		return vector.DistanceSq(g_Game.GetCurrentCameraPosition(), GetPosition()) < GAUGE_RANGE_SQ;
	}

	/*!
		The cockpit (GebsFlight cockpit=1): the stick, pedals, throttle lever,
		flap handwheel and gear lever move with the controls, the gear lamps
		show green down and locked and red up, the radiator flaps open as the
		engine works hard and slow, and every gauge (GebsFlight Gauges) shows
		what it reads.
	*/
	protected void UpdateCockpit(float dt)
	{
		if (!m_FlightLoaded)
			return;

		bool running = GetEngineState() == GebsEngineState.RUNNING;
		float power = m_NetPower;
		if (!IsProxy())
			power = m_EnginePower;

		// temperatures, warming and cooling here (for show): oil and coolant run
		// towards their working heat with the engine on, back to the air's off
		float oilWant = 15;
		float coolWant = 15;
		if (running)
		{
			oilWant = 55 + 25 * power;
			coolWant = 70 + 22 * power;
		}
		m_OilTemp += (oilWant - m_OilTemp) * Math.Clamp(dt / 150, 0, 1);
		m_CoolantTemp += (coolWant - m_CoolantTemp) * Math.Clamp(dt / 90, 0, 1);

		if (m_HasCockpit)
		{
			float elevator = m_NetElevator;
			float aileron = m_NetAileron;
			float rudder = m_NetRudder;
			if (!IsProxy())
			{
				elevator = m_FlightControls.m_Elevator;
				aileron = m_FlightControls.m_Aileron;
				rudder = m_FlightControls.m_Rudder;
			}

			SetPhase("stick_pitch", 0.5 + 0.5 * elevator);
			SetPhase("stick_roll", 0.5 + 0.5 * aileron);
			SetPhase("pedals", 0.5 + 0.5 * rudder);
			SetPhase("throttle", GetThrottle());
			SetPhase("flapwheel", GetFlaps());
			float lever = 1;
			if (IsGearSetDown())
				lever = 0;
			SetPhase("gearlever", lever);

			// the gear lamps: each lit face hides at 1 (green down and locked, red up); with no gear to
			// work (the floatplane's stays folded) both stay dark
			float gear = GetGearPhase();
			float greenOff = 1;
			float redOff = 1;
			if (m_GearUpTime > 0)
			{
				if (gear < 0.02)
					greenOff = 0;
				if (gear > 0.98)
					redOff = 0;
			}
			SetPhase("lamp_down_off", greenOff);
			SetPhase("lamp_up_off", redOff);

			float radiator = 0.15;
			if (running)
				radiator = Math.Clamp(0.25 + 0.6 * power - Math.Max(GetAirspeed(), 0) / 160, 0.15, 1);
			SetPhase("radiator", radiator);
		}

		if (m_Gauges.Count() == 0 || !IsPanelSeen())
			return;

		// the instruments' own lags: a variometer's capsule, a turn indicator's gyro, the ball
		vector vel = GetVelocity(this);
		float k = Math.Clamp(dt / 0.8, 0, 1);
		m_Vario += (vel[1] - m_Vario) * k;
		float pitchRate;
		float yawRate;
		TurnRates(pitchRate, yawRate);
		m_TurnRate += (yawRate * Math.RAD2DEG - m_TurnRate) * Math.Clamp(dt / 0.4, 0, 1);
		if (dt > 0)
		{
			vector accel = (vel - m_LastVelocity) * (1 / dt);
			// the ball: sideways specific force, gravity's and the turn's, against the airframe's side
			vector specific = accel + Vector(0, 9.81, 0);
			float side = vector.Dot(specific, VectorToParent(m_FlightSide)) / 9.81;
			m_Slip += (Math.Clamp(side, -1, 1) - m_Slip) * Math.Clamp(dt / 0.3, 0, 1);
		}
		m_LastVelocity = vel;

		foreach (GebsPlaneGauge g : m_Gauges)
		{
			float phase = g.Phase(GaugeValue(g.m_Value));
			if (g.m_Lag > 0 && g.m_Shown >= 0 && !g.m_Wrap)
				phase = g.m_Shown + (phase - g.m_Shown) * Math.Clamp(dt / g.m_Lag, 0, 1);
			if (Math.AbsFloat(phase - g.m_Shown) > 0.0005)
			{
				g.m_Shown = phase;
				SetAnimationPhase(g.m_Source, phase);
			}
		}
	}

	/*!
		What a gauge reads (GebsFlight Gauges value=):
		ias (km/h, indicated: falls off with the air's density), altitude (m),
		vario (m/s), turn (deg/s, right +), slip (-1 .. 1, ball right +),
		heading (deg), rpm (engine), boost (ata), pitch (the propeller's blade
		angle, deg), clockHours / clockMinutes (the time of day), fuel (L),
		fuelPressure and oilPressure (kg/cm2), oilTemp and coolantTemp (deg C),
		amps, brakeLeft / brakeRight (0 .. 1), flaps (0 .. 1), trim (-1 .. 1).
	*/
	float GaugeValue(string what)
	{
		vector pos = GetPosition();
		bool running = GetEngineState() == GebsEngineState.RUNNING;
		float rpm = GetEngineRpm();
		vector dir;
		float heading;
		float tip;
		int year;
		int month;
		int day;
		int hour;
		int minute;
		switch (what)
		{
			case "ias":
				return Math.Max(GetAirspeed(), 0) * 3.6 * Math.Sqrt(AirDensity(pos[1]) / SEA_LEVEL_DENSITY);
			case "altitude":
				return pos[1];
			case "vario":
				return m_Vario;
			case "turn":
				return m_TurnRate;
			case "slip":
				return m_Slip;
			case "heading":
				dir = VectorToParent(m_FlightFwd);
				heading = Math.Atan2(dir[0], dir[2]) * Math.RAD2DEG;
				if (heading < 0)
					heading += 360;
				return heading;
			case "rpm":
				return rpm;
			case "boost":
				return GetBoost();
			case "pitch":
				// the blade angle at three quarters of the radius, from the airspeed
				// and the revs: fine standing still, coarser as it speeds up
				tip = Math.Max(GetPropRpm(), 1) * RPM_TO_RAD * 0.375 * m_PropDiameter;
				return Math.Atan2(Math.Max(GetAirspeed(), 0) + 8, tip) * Math.RAD2DEG + 12;
			case "clockHours":
				g_Game.GetWorld().GetDate(year, month, day, hour, minute);
				return hour + minute / 60.0;
			case "clockMinutes":
				g_Game.GetWorld().GetDate(year, month, day, hour, minute);
				return minute;
			case "fuel":
				return GetFuel();
			case "fuelPressure":
				if (running)
					return 1.2 + 0.25 * GetThrottle();
				if (GetEngineState() == GebsEngineState.CRANKING)
					return 0.4;
				return 0;
			case "oilPressure":
				if (rpm <= 0)
					return 0;
				return Math.Clamp(rpm / 2400, 0, 1.15) * 7.5;
			case "oilTemp":
				return m_OilTemp;
			case "coolantTemp":
				return m_CoolantTemp;
			case "amps":
				if (running && rpm > 1100)
					return 12;
				if (m_NetSwitch || m_FlightControls.m_Ignition)
					return -3;
				return 0;
			case "brakeLeft":
				return WheelBrake(-1);
			case "brakeRight":
				return WheelBrake(1);
			case "flaps":
				return GetFlaps();
			case "trim":
				return GetTrim();
		}

		return 0;
	}

	//! How hard the brakes on one side are on (-1 left, 1 right), 0 .. 1.
	protected float WheelBrake(int side)
	{
		float brake = 0;
		foreach (GebsPlaneWheel wheel : m_Wheels)
		{
			if (wheel.m_BrakeSide == side)
				brake = Math.Max(brake, wheel.m_Brake);
		}

		return brake;
	}

	/*!
		The sounds: the starter (an inertia starter's flywheel winding up, then
		the catch, or without it when the engine can't run), the engine's loops
		while it runs, the wind's while it moves, the stop; the gear's
		hydraulics while the legs move and their thump as they lock; the flap
		handwheel.
	*/
	protected void UpdateSound(float dt)
	{
		if (!m_FlightLoaded)
			return;

		int state = GetEngineState();
		if (state != m_HeardEngineState)
		{
			if (state == GebsEngineState.CRANKING)
			{
				if (CanEngineRun())
					PlayOnce(m_EngineStartSound);
				else
					PlayOnce(m_EngineFailSound);
			}
			else if (state == GebsEngineState.RUNNING)
			{
				PlayOnce(m_EngineCatchSound);
			}
			else if (state == GebsEngineState.OFF && m_HeardEngineState == GebsEngineState.RUNNING)
			{
				PlayOnce(m_EngineStopSound);
			}

			m_HeardEngineState = state;
		}

		if (m_GearUpTime > 0)
		{
			float gear = GetGearPhase();
			if (m_HeardGear >= 0)
			{
				bool moving = Math.AbsFloat(gear - m_HeardGear) > 0.0001;
				if (moving && !m_HeardGearMoving)
					PlayOnce(m_GearMoveSound);
				if (!moving && m_HeardGearMoving && (gear < 0.02 || gear > 0.98))
					PlayOnce(m_GearLockSound);
				m_HeardGearMoving = moving;
			}
			m_HeardGear = gear;
		}

		float flaps = GetFlaps();
		m_FlapSoundTime -= dt;
		if (m_HeardFlaps >= 0 && Math.AbsFloat(flaps - m_HeardFlaps) > 0.0001 && m_FlapSoundTime <= 0)
		{
			PlayOnce(m_FlapSound);
			m_FlapSoundTime = 0.9;
		}
		m_HeardFlaps = flaps;

		float campos = 1;
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (player && player.GetParent() == this && player.IsCameraInsideVehicle())
			campos = 0;

		float doors = 0;
		if (IsDoorOpen())
			doors = 1;

		float power = m_NetPower;
		if (!IsProxy())
			power = m_EnginePower;
		float kmh = GetVelocity(this).Length() * 3.6;

		if (!m_EngineSound && m_EngineSoundSets.Count() > 0 && state == GebsEngineState.RUNNING)
			m_EngineSound = new GebsPlaneEngineSound(this, m_SoundPos, m_EngineSoundSets);
		if (m_EngineSound)
			m_EngineSound.Update(state == GebsEngineState.RUNNING, GetEngineRpm() * m_SoundRpmScale, power, kmh, doors, campos);

		bool flying = kmh > 25;
		if (!m_AirSound && m_AirSoundSets.Count() > 0 && flying)
			m_AirSound = new GebsPlaneEngineSound(this, vector.Zero, m_AirSoundSets);
		if (m_AirSound)
			m_AirSound.Update(flying, GetEngineRpm() * m_SoundRpmScale, power, kmh, doors, campos);

		// the floats in the water: its lapping and rushing (their shaders read the share in the water as
		// "thrust"), and a splash as they come down on it
		if (m_FloatGear)
		{
			float wet = m_FloatGear.m_WetShare;
			bool lapping = wet > 0;
			if (!m_WaterSound && m_WaterSoundSets.Count() > 0 && lapping)
				m_WaterSound = new GebsPlaneEngineSound(this, vector.Zero, m_WaterSoundSets);
			if (m_WaterSound)
				m_WaterSound.Update(lapping, 0, wet, kmh, doors, campos);
			if (wet > 0 && m_HeardWet == 0 && (GetVelocity(this)[1] < -SPLASH_SINK || kmh > 60))
				PlayOnce(m_SplashSound);
			m_HeardWet = wet;
		}
	}

	protected void PlayOnce(string soundSet)
	{
		if (soundSet != "")
			SEffectManager.PlaySoundOnObject(soundSet, this);
	}

	//! The door's sound, as a car's: opening from shut, closing from open.
	override void OnAnimationPhaseStarted(string animSource, float phase)
	{
		super.OnAnimationPhaseStarted(animSource, phase);

		#ifndef SERVER
		string door = m_DoorSource;
		door.ToLower();
		animSource.ToLower();
		if (door == "" || animSource != door)
			return;

		if (phase == 0)
			PlayOnce(m_DoorOpenSound);
		else
			PlayOnce(m_DoorCloseSound);
		#endif
	}

	//--------------------------------------------------------------------------
	// the cabin door or canopy: one, animated, not an attachment
	//--------------------------------------------------------------------------

	string GetDoorAnimSource()
	{
		return m_DoorSource;
	}

	bool IsDoorOpen()
	{
		return m_DoorSource != "" && GetAnimationPhase(m_DoorSource) > 0.5;
	}

	//! km/h, as vanilla's car: faster than this nobody climbs out; with the door or canopy open, they eject
	static const float CREW_EJECT_SPEED = 8.0;

	//! The plane's speed in km/h, as someone leaving it meets it.
	float GetCrewExitSpeed()
	{
		return GetVelocity(this).Length() * 3.6;
	}

	//! Moving: Get out isn't offered, only Eject (GebsActionEjectPlane).
	bool IsTooFastToGetOut()
	{
		return GetCrewExitSpeed() > CREW_EJECT_SPEED;
	}

	//! Ejecting needs the way out open: the door or canopy, or a cockpit without one.
	bool IsEjectWayOpen()
	{
		return m_DoorSource == "" || IsDoorOpen();
	}

	/*!
		Whether someone standing at pos is near enough to climb in, or to work the door from outside: within
		boardReach of the boarding point (the cockpit, or the door) across the ground, from any side, on the
		ground or up on the wing.
	*/
	bool IsInBoardingReach(vector pos)
	{
		vector board = GetPosition();
		if (m_BoardPoint != "" && MemoryPointExists(m_BoardPoint))
			board = ModelToWorld(GetMemoryPointPos(m_BoardPoint));

		if (Math.AbsFloat(pos[1] - board[1]) > m_BoardHeight)
			return false;

		board[1] = 0;
		pos[1] = 0;
		return vector.Distance(board, pos) <= m_BoardReach;
	}

	//! The seat someone climbing in takes: the one they look at if it is free, else the first free one, the pilot's first.
	int GetBoardingSeat(int componentIndex)
	{
		int looked = CrewPositionIndex(componentIndex);
		if (looked >= 0 && !CrewMember(looked))
			return looked;

		for (int i = 0; i < CrewSize(); i++)
		{
			if (!CrewMember(i))
				return i;
		}

		return -1;
	}

	//! The door or canopy opens by itself for someone climbing in or out. On the server: the phase syncs.
	void OpenDoorForCrew()
	{
		if (m_DoorSource != "" && !IsDoorOpen())
			SetAnimationPhase(m_DoorSource, 1);
	}

	/*!
		Puts someone who has just got out where the door is. The pilot's seat
		has to use a car driver's animations, and a car driver climbs out to the
		left, wherever the door is; this moves them round to it. On the ground
		they stand on the ground (or the wing root, for a low-wing plane);
		jumping out in the air, they fall from beside the door.
	*/
	void PlaceAtDoor(Human player)
	{
		if (!player || m_DoorPoint == "" || !MemoryPointExists(m_DoorPoint))
			return;

		vector pos = ModelToWorld(GetMemoryPointPos(m_DoorPoint));
		float ground = g_Game.SurfaceRoadY(pos[0], pos[2]);
		if (pos[1] - ground < 1.5)
			pos[1] = ground;

		player.SetPosition(pos);
	}

	/*!
		Server: someone who has ejected at kmh. In the air the fall does the
		rest, and a floatplane's crew lands in the water. From a plane rolling
		on its wheels they are hurt as a car's crew jumping out at that speed
		(vanilla's numbers, ActionGetOutTransport): shock, a bleed, their legs,
		and at speed their health.
	*/
	void OnCrewEjectedServer(PlayerBase player, float kmh)
	{
		if (!player || !player.IsAlive() || !IsOnGround())
			return;

		float shock = kmh * kmh / ActionGetOutTransport.DMG_FACTOR;
		if (HelmetBase.Cast(player.GetItemOnHead()))
			shock *= 0.5;
		player.GiveShock(-shock);

		array<string> bleeds = PlayerBase.m_BleedingSourcesLow;
		if (kmh >= ActionGetOutTransport.LOW_SPEED_VALUE)
			bleeds = PlayerBase.m_BleedingSourcesUp;
		if (player.m_BleedingManagerServer && bleeds && bleeds.Count() > 0)
			player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection(bleeds.Get(Math.RandomInt(0, bleeds.Count())));

		player.DamageAllLegs(kmh * kmh / ActionGetOutTransport.SHOCK_FACTOR);

		float health = Math.Clamp(Math.InverseLerp(ActionGetOutTransport.HEALTH_LOW_SPEED_VALUE, ActionGetOutTransport.HEALTH_HIGH_SPEED_VALUE, kmh), 0, 1);
		if (health > 0)
			player.ProcessDirectDamage(DamageType.CUSTOM, player, "", "FallDamageHealth", player.WorldToModel(player.GetPosition()), health);
	}

	override string GetAnimSourceFromSelection(string selection)
	{
		switch (selection)
		{
			case "doors_driver":
			case "doors_driver_upper":
			case "canopy":
				return m_DoorSource;
		}

		return super.GetAnimSourceFromSelection(selection);
	}

	//! In and out whether the door is open or not: it opens by itself (OpenDoorForCrew).
	override bool CrewCanGetThrough(int posIdx)
	{
		return true;
	}

	override bool CanReachSeatFromDoors(string pSeatSelection, vector pFromPos, float pDistance = 1.0)
	{
		return IsInBoardingReach(pFromPos);
	}

	override bool CanReachDoorsFromSeat(string pDoorsSelection, int pCurrentSeat)
	{
		return true;
	}

	//! Tandem seats: nobody climbs over the pilot.
	override bool CanReachSeatFromSeat(int currentSeat, int nextSeat)
	{
		return false;
	}

	//--------------------------------------------------------------------------
	// actions on the plane (the ones from the seat are the player's:
	// scripts/4_world/entities/manbase/gebsplanecrew.c)
	//--------------------------------------------------------------------------

	void InitializeActions()
	{
		m_InputActionMap = s_GebsPlaneActionMaps.Get(Type());
		if (!m_InputActionMap)
		{
			// the shared map owns it; the plane only keeps a pointer, as CarScript does
			TInputActionMap actionMap = new TInputActionMap();
			s_GebsPlaneActionMaps.Insert(Type(), actionMap);
			m_InputActionMap = actionMap;
			SetActions();
		}
	}

	override void GetActions(typename action_input_type, out array<ActionBase_Basic> actions)
	{
		if (!m_ActionsInitialized)
		{
			m_ActionsInitialized = true;
			InitializeActions();
		}

		actions = m_InputActionMap.Get(action_input_type);
		if (actions)
			return;

		// Vanilla asks a vehicle you sit in for its actions of every input type
		// and counts them without checking for none (a car always has its lights
		// action). So every type gets a list, if only an empty one; the shared map
		// owns it
		array<ActionBase_Basic> none = new array<ActionBase_Basic>();
		m_InputActionMap.Insert(action_input_type, none);
		actions = none;
	}

	void SetActions()
	{
		AddAction(GebsActionGetInPlane);
		AddAction(GebsActionOpenPlaneDoor);
		AddAction(GebsActionClosePlaneDoor);
	}

	void AddAction(typename actionName)
	{
		ActionBase action = ActionManagerBase.GetAction(actionName);
		if (!action)
		{
			Debug.LogError("Action " + actionName + " doesn't exist!");
			return;
		}

		typename inputType = action.GetInputType();
		if (!inputType)
		{
			m_ActionsInitialized = false;
			return;
		}

		array<ActionBase_Basic> actions = m_InputActionMap.Get(inputType);
		if (!actions)
		{
			actions = new array<ActionBase_Basic>();
			m_InputActionMap.Insert(inputType, actions);
		}

		actions.Insert(action);
	}

	//! Admin-tool spawn: full tanks and the door open, ready to climb in. The
	//! wheels come from the plane's own class.
	override void OnDebugSpawn()
	{
		m_Fuel = m_FuelCapacity;
		m_FuelSent = m_Fuel;
		SetSynchDirty();
		if (m_DoorSource != "")
			SetAnimationPhase(m_DoorSource, 1);
	}
};
