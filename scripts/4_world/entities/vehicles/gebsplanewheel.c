/*
	One wheel of a GebsPlane.

	Each physics step it casts a ray down from its hub to find the ground. A
	spring and damper push the plane up off the hit, along the ground's normal,
	and the tyre grips: across the wheel it takes out the contact's sideways
	slip (with slipPeak, only as hard as its slip angle allows), along it it
	rolls, drags and brakes - each no harder than the wheel's load times its
	friction. That is the usual raycast suspension of game vehicles. Every force goes in through Transport.ApplyForceAt, the engine's
	deterministic vehicle physics, which the network replays after a
	correction.

	A wheel whose slot is empty does nothing: the plane comes down on the hub
	block of its Geometry LOD. A wheel on a retractable leg (retracts=1) only
	carries the plane with the gear down and locked; folded, it is lifted.
*/
class GebsPlaneWheel
{
	static const float GRAVITY = 9.81;
	static const float GRIP_SHARE = 0.5;		//!< of the slip at its contact each tyre takes out in a step
	static const float RAY_LEAD = 0.5;		//!< m: the ray starts this far above the top of the travel, so a wheel squeezed past it still finds the ground
	static const float BUMP_STOP = 4;		//!< squeezed past its travel, the spring stiffens this many times
	static const float SLIP_FLOOR = 0.5;	//!< m/s: rolling slower than this, a tyre's slip angle is taken against this

	// configuration (GebsFlight >> Wheels >> <name>)
	string m_Name;
	vector m_Hub;				//!< model space: the wheel's centre as modelled, sitting on the ground
	string m_Slot;				//!< the wheel item's slot; "" for the plane's own wheel
	float m_Radius;				//!< m, the plane's own wheel
	float m_Stiffness;			//!< N/m
	float m_Compression;		//!< N per m/s, damping as the spring closes
	float m_Rebound;			//!< N per m/s, damping as it opens
	float m_TravelUp;			//!< m above the modelled height it can be pushed
	float m_TravelDown;			//!< m below it it can hang
	float m_BrakeForce;			//!< N at the tyre, full brake
	float m_Grip;				//!< tyre friction, the plane's own wheel
	float m_LateralGrip;		//!< sideways friction, as a share of the grip
	float m_SlipPeak;			//!< rad: the slip angle by which its side grip is full (0: full at once)
	float m_RollResistance;		//!< share of the load that rolling drags
	float m_Steer;				//!< rad at full right rudder, + points the wheel right
	int m_BrakeSide;			//!< -1 the left rudder pedal also brakes it, 1 the right, 0 neither
	string m_SpinAnim;
	string m_DamperAnim;
	string m_SteerAnim;
	float m_DamperMin;			//!< m the damper animation moves the wheel at phase 0
	float m_DamperMax;			//!< and at phase 1
	bool m_Retracts;			//!< on a leg that folds away with the gear

	// this step
	bool m_Contact;
	vector m_ContactPos;		//!< world space
	vector m_ContactNormal;
	float m_Compressed;			//!< m the spring is in from full extension
	float m_Load;				//!< N it carries
	float m_SteerAngle;			//!< rad, + points it right
	float m_Brake;				//!< 0 .. 1

	// what it looks like (clients)
	float m_RollSpeed;			//!< m/s along the wheel
	float m_SpinAngle;			//!< rad

	static GebsPlaneWheel Load(string path, string name, Object plane)
	{
		string hub;
		g_Game.ConfigGetText(path + " hub", hub);
		if (hub == "" || !plane.MemoryPointExists(hub))
		{
			ErrorEx(string.Format("GebsPlanes: %1 has no memory point '%2' for wheel %3", plane.GetType(), hub, name));
			return null;
		}

		string slot;
		string spinAnim;
		string damperAnim;
		string steerAnim;
		g_Game.ConfigGetText(path + " slot", slot);
		g_Game.ConfigGetText(path + " spinAnim", spinAnim);
		g_Game.ConfigGetText(path + " damperAnim", damperAnim);
		g_Game.ConfigGetText(path + " steerAnim", steerAnim);

		GebsPlaneWheel w = new GebsPlaneWheel();
		w.m_Name = name;
		w.m_Hub = plane.GetMemoryPointPos(hub);
		w.m_Slot = slot;
		w.m_SpinAnim = spinAnim;
		w.m_DamperAnim = damperAnim;
		w.m_SteerAnim = steerAnim;
		w.m_Radius = Math.Max(g_Game.ConfigGetFloat(path + " radius"), 0.05);
		w.m_Stiffness = g_Game.ConfigGetFloat(path + " stiffness");
		w.m_Compression = g_Game.ConfigGetFloat(path + " compression");
		w.m_Rebound = g_Game.ConfigGetFloat(path + " rebound");
		w.m_TravelUp = g_Game.ConfigGetFloat(path + " travelUp");
		w.m_TravelDown = g_Game.ConfigGetFloat(path + " travelDown");
		w.m_BrakeForce = g_Game.ConfigGetFloat(path + " brakeForce");
		w.m_Grip = g_Game.ConfigGetFloat(path + " grip");
		w.m_LateralGrip = g_Game.ConfigGetFloat(path + " lateralGrip");
		w.m_SlipPeak = g_Game.ConfigGetFloat(path + " slipPeak") * Math.DEG2RAD;
		w.m_RollResistance = g_Game.ConfigGetFloat(path + " rollResistance");
		w.m_Steer = g_Game.ConfigGetFloat(path + " steer") * Math.DEG2RAD;
		w.m_BrakeSide = g_Game.ConfigGetInt(path + " brakeSide");
		w.m_DamperMin = g_Game.ConfigGetFloat(path + " damperMin");
		w.m_DamperMax = g_Game.ConfigGetFloat(path + " damperMax");
		w.m_Retracts = g_Game.ConfigGetInt(path + " retracts") != 0;
		return w;
	}

	//! Folded away, or nothing to stand on: no contact.
	void Lift()
	{
		m_Contact = false;
		m_Load = 0;
		m_Compressed = 0;
	}

	//! The wheel item fitted, or null: the plane's own wheel, or an empty slot.
	EntityAI GetItem(EntityAI plane)
	{
		if (m_Slot == "")
			return null;

		return plane.FindAttachmentBySlotName(m_Slot);
	}

	bool IsFitted(EntityAI plane)
	{
		return m_Slot == "" || GetItem(plane) != null;
	}

	//! A fitted wheel's radius shrinks as it is damaged (radiusByDamage).
	float GetRadius(EntityAI plane)
	{
		CarWheel wheel = CarWheel.Cast(GetItem(plane));
		if (wheel)
			return wheel.GetRadius();

		return m_Radius;
	}

	/*!
		One physics step. fwdW, sideW and upW are the plane's axes in world
		space (forward, right, up).
	*/
	void Simulate(GebsPlane plane, float dt, vector fwdW, vector sideW, vector upW)
	{
		Lift();
		if (!IsFitted(plane) || dt <= 0)
			return;

		float radius = GetRadius(plane);
		float grip = m_Grip;
		float lateral = m_LateralGrip;
		float roll = m_RollResistance;
		EntityAI item = GetItem(plane);
		if (item)
		{
			// a fitted wheel brings its own tyre
			grip = item.ConfigGetFloat("tyreGrip");
			lateral = item.ConfigGetFloat("tyreLateralFriction");
			roll = item.ConfigGetFloat("tyreRollResistance");
		}

		vector hubW = plane.SimPoint(m_Hub);
		float length = RAY_LEAD + m_TravelUp + m_TravelDown + radius;
		vector from = hubW + upW * (m_TravelUp + RAY_LEAD);
		vector to = hubW - upW * (m_TravelDown + radius);

		Object hitObject;
		vector hitPos;
		vector hitNormal;
		float hitFraction;
		int layers = PhxInteractionLayers.DEFAULT | PhxInteractionLayers.BUILDING | PhxInteractionLayers.ROADWAY | PhxInteractionLayers.TERRAIN | PhxInteractionLayers.ITEM_LARGE;
		if (!DayZPhysics.RayCastBullet(from, to, layers, plane, hitObject, hitPos, hitNormal, hitFraction))
			return;

		m_Contact = true;
		m_ContactPos = hitPos;
		m_ContactNormal = hitNormal;
		m_Compressed = length * (1 - Math.Clamp(hitFraction, 0, 1));

		// the spring and damper, along the ground's normal. Past its travel it
		// meets the bump stop: it stiffens, so a drop or a bounce never sinks the
		// wheel through the ground and leaves the plane on its Geometry LOD
		// (the Bf 109's tail did, at its spawn: scraping, juddering, and never
		// faster than 95 km/h)
		float closing = -vector.Dot(dBodyGetVelocityAt(plane, hubW), hitNormal);
		float damping = m_Rebound;
		if (closing > 0)
			damping = m_Compression;

		float travel = m_TravelUp + m_TravelDown;
		float spring = m_Stiffness * Math.Min(m_Compressed, travel) + m_Stiffness * BUMP_STOP * Math.Max(m_Compressed - travel, 0);
		m_Load = Math.Max(spring + closing * damping, 0);
		plane.ApplyForceAt(hitPos, hitNormal * m_Load);

		// the tyre, along and across the wheel in the ground's plane
		float cosSteer = Math.Cos(m_SteerAngle);
		float sinSteer = Math.Sin(m_SteerAngle);
		vector along = fwdW * cosSteer + sideW * sinSteer;
		vector across = sideW * cosSteer - fwdW * sinSteer;
		along = along - hitNormal * vector.Dot(along, hitNormal);
		across = across - hitNormal * vector.Dot(across, hitNormal);
		along.Normalize();
		across.Normalize();

		vector slip = dBodyGetVelocityAt(plane, hitPos);
		float vAlong = vector.Dot(slip, along);
		float vAcross = vector.Dot(slip, across);

		// what taking GRIP_SHARE of the slip out in one step takes: the body's
		// effective mass at the contact, each way - its mass, and its inertia about
		// the centre of gravity through the contact's lever arm. (The share of the
		// load's mass used before overshot through that lever arm: the plane rocked
		// from wheel to wheel every step, parked - tools/bf109_sim.py.) Then limited
		// by friction: sideways it grips, along it only rolls and brakes
		vector inv[3];
		dBodyGetInvInertiaTensorWorld(plane, inv);
		float invMass = 1 / Math.Max(dBodyGetMass(plane), 1);
		vector arm = hitPos - plane.SimPoint(dBodyGetCenterOfMass(plane));
		vector rAcross = GebsPlane.Cross(arm, across);
		vector rAlong = GebsPlane.Cross(arm, along);
		vector iAcross = Vector(vector.Dot(inv[0], rAcross), vector.Dot(inv[1], rAcross), vector.Dot(inv[2], rAcross));
		vector iAlong = Vector(vector.Dot(inv[0], rAlong), vector.Dot(inv[1], rAlong), vector.Dot(inv[2], rAlong));
		float stopAcross = GRIP_SHARE / (invMass + vector.Dot(rAcross, iAcross)) / dt;
		float stopAlong = GRIP_SHARE / (invMass + vector.Dot(rAlong, iAlong)) / dt;
		float limit = m_Load * grip;
		float limitAcross = limit * lateral;
		float limitAlong = Math.Min(m_Load * roll + m_BrakeForce * m_Brake, limit);

		// a tyre's side grip builds with its slip angle, full by m_SlipPeak: yawed on its wheels, the
		// plane skids a little rather than turning as on rails. Gripping fully at once, a second of
		// rudder at 95 km/h, with the wing carrying half the weight, rolled the Bf 109 onto a wingtip
		float side = limitAcross;
		if (m_SlipPeak > 0)
		{
			float slipAngle = Math.Atan2(Math.AbsFloat(vAcross), Math.Max(Math.AbsFloat(vAlong), SLIP_FLOOR));
			side = limitAcross * Math.Min(slipAngle / m_SlipPeak, 1);
		}

		float fAcross = Math.Clamp(-vAcross * stopAcross, -side, side);
		float fAlong = Math.Clamp(-vAlong * stopAlong, -limitAlong, limitAlong);

		// one tyre has only so much grip, whichever way it is used
		float total = Math.Sqrt(fAlong * fAlong + fAcross * fAcross);
		float most = Math.Max(limit, limitAcross);
		if (total > most && total > 0)
		{
			fAlong *= most / total;
			fAcross *= most / total;
		}

		plane.ApplyForceAt(hitPos, along * fAlong + across * fAcross);
	}

	/*!
		What it looks like, every frame on clients: the wheel spins with the
		ground going by, rides its spring, and the tailwheel turns with the
		pedals. Where the plane is not simulated (other players' planes) the
		spring stays where it is drawn.
	*/
	void UpdateVisuals(GebsPlane plane, float dt, bool simulated, vector fwdW)
	{
		float radius = GetRadius(plane);
		if (m_Contact || !simulated)
			m_RollSpeed = vector.Dot(GetVelocity(plane), fwdW);
		else
			m_RollSpeed = m_RollSpeed * Math.Clamp(1 - dt * 0.5, 0, 1);	// spinning down in the air

		if (m_SpinAnim != "")
		{
			// a wheel on a leg that is folding or folded rests at its own angle 0: its spin axis is
			// drawn with the leg only down and locked, so spinning on a folding leg could swing it out
			// of its well
			if (m_Retracts && plane.GetGearPhase() > 0)
			{
				m_RollSpeed = 0;
				m_SpinAngle = 0;
			}
			else
			{
				m_SpinAngle += m_RollSpeed / radius * dt;
				m_SpinAngle -= Math.Floor(m_SpinAngle / Math.PI2) * Math.PI2;
			}

			// as turns, 0 .. 1, as the propeller's and every other "user" source's phase here: given radians,
			// a source held to 0 .. 1 turns the wheel 57 degrees and snaps it back, a wobble (model.cfg maxValue 1)
			plane.SetAnimationPhase(m_SpinAnim, m_SpinAngle / Math.PI2);
		}

		if (m_DamperAnim != "" && m_DamperMax > m_DamperMin)
		{
			// up from where it is modelled: the spring settled past travelDown
			float rise = 0;
			if (simulated)
			{
				if (m_Contact)
					rise = m_Compressed - m_TravelDown;
				else
					rise = -m_TravelDown;
			}

			plane.SetAnimationPhase(m_DamperAnim, Math.Clamp((rise - m_DamperMin) / (m_DamperMax - m_DamperMin), 0, 1));
		}

		// -90 .. 90 degrees as 0 .. 1, the middle at 0.5, as the control surfaces' (model.cfg minValue 0, maxValue 1)
		if (m_SteerAnim != "")
			plane.SetAnimationPhase(m_SteerAnim, Math.Clamp(0.5 + m_SteerAngle / Math.PI, 0, 1));
	}
};
