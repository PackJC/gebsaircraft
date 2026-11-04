/*
	A GebsPlane's floats (GebsFlight Floats: the Bf 109 W).

	Each float is cut into slices along it (tools/bf109_floats.py shapes them
	and the build writes them into the config), each at the keel's memory
	point in its middle, each a vee bottom (halfBeam at the chines, chine above
	the keel) with upright sides up to its deck. Every physics step each slice
	with its keel under the water:
	- is buoyed up by the water it displaces;
	- planes: moving into the water, its bottom is pushed back out as hard as
	  it meets it (planing, per radian of the angle and square metre of wetted
	  bottom), so up on the step a little of each float carries the plane;
	- is damped going up and down (heaveDamping);
	- along its keel, makes waves as it displaces water (humpDrag of its
	  displacement at humpSpeed, less either side: the hump of the takeoff run)
	  and drags with its skin (skinFriction);
	- across, grips like a keel (sideDrag).
	Up on the step the water breaks away from it and leaves a hollow behind:
	the afterbody runs over it dry. A water rudder hangs on each transom, on
	the pedals, of use at a taxi; each float has its own air drag.

	Every force goes in through Transport.ApplyForceAt at points from the
	physics body's own transform this step (GebsPlane.SimPoint), as the
	wheels' do, so a replayed step comes out the same. tools/bf109_sim.py
	(simulate_floats) mirrors it line for line; it was tuned there.
*/
class GebsPlaneFloatSlice
{
	vector m_Keel;			//!< model space: the keel in the slice's middle
	float m_Length;			//!< m along the keel
	float m_HalfBeam;		//!< m, at the chines
	float m_Chine;			//!< m the chines are above the keel: the vee's depth
	float m_Deck;			//!< m the deck is above the keel
	float m_SlopeCos;		//!< the bottom's slope to the datum (+ rising to the bow)
	float m_SlopeSin;
	bool m_Aft;				//!< behind the step: its bottom rises aft
};

class GebsPlaneFloat
{
	string m_Name;
	ref array<ref GebsPlaneFloatSlice> m_Slices = new array<ref GebsPlaneFloatSlice>();
	vector m_StepKeel;		//!< model space: the keel at the step (the forebody's last slice)
	vector m_Rudder;		//!< where its water rudder's force acts
	vector m_Centre;		//!< where its own air drag acts
	vector m_SprayFront;	//!< on the keel where the forebody meets the water
	vector m_SpraySide;		//!< on the keel at the step

	// this step
	int m_Wet;				//!< slices in the water
	float m_Load;			//!< N the water pushes it up with, along the plane's up

	// what it looks like (clients)
	ref GebsFloatSpray m_FrontSpray;
	ref GebsFloatSpray m_SideSpray;

	static GebsPlaneFloat Load(string path, string name, Object plane)
	{
		TStringArray points = new TStringArray();
		TFloatArray length = new TFloatArray();
		TFloatArray halfBeam = new TFloatArray();
		TFloatArray chine = new TFloatArray();
		TFloatArray deck = new TFloatArray();
		TFloatArray slope = new TFloatArray();
		g_Game.ConfigGetTextArray(path + " slices", points);
		g_Game.ConfigGetFloatArray(path + " length", length);
		g_Game.ConfigGetFloatArray(path + " halfBeam", halfBeam);
		g_Game.ConfigGetFloatArray(path + " chine", chine);
		g_Game.ConfigGetFloatArray(path + " deck", deck);
		g_Game.ConfigGetFloatArray(path + " slope", slope);

		int n = points.Count();
		if (n == 0 || length.Count() < n || halfBeam.Count() < n || chine.Count() < n || deck.Count() < n || slope.Count() < n)
		{
			ErrorEx(string.Format("GebsPlanes: %1's float %2 has %3 slices but not their sizes for each", plane.GetType(), name, n));
			return null;
		}

		GebsPlaneFloat f = new GebsPlaneFloat();
		f.m_Name = name;
		for (int i = 0; i < n; i++)
		{
			if (!plane.MemoryPointExists(points[i]))
			{
				ErrorEx(string.Format("GebsPlanes: %1 has no memory point '%2' for float %3", plane.GetType(), points[i], name));
				return null;
			}

			GebsPlaneFloatSlice s = new GebsPlaneFloatSlice();
			s.m_Keel = plane.GetMemoryPointPos(points[i]);
			s.m_Length = length[i];
			s.m_HalfBeam = halfBeam[i];
			s.m_Chine = Math.Max(chine[i], 0.01);
			s.m_Deck = deck[i];
			float angle = slope[i] * Math.DEG2RAD;
			s.m_SlopeCos = Math.Cos(angle);
			s.m_SlopeSin = Math.Sin(angle);
			s.m_Aft = angle < 0;
			f.m_Slices.Insert(s);
			if (!s.m_Aft)
				f.m_StepKeel = s.m_Keel;
		}

		f.m_Rudder = Point(plane, path, "rudder", f.m_Slices[n - 1].m_Keel);
		f.m_Centre = Point(plane, path, "centre", f.m_StepKeel);
		f.m_SprayFront = Point(plane, path, "sprayFront", f.m_Slices[0].m_Keel);
		f.m_SpraySide = Point(plane, path, "spraySide", f.m_StepKeel);
		return f;
	}

	//! A memory point the config names, or where it falls back to.
	protected static vector Point(Object plane, string path, string key, vector fallback)
	{
		string name;
		g_Game.ConfigGetText(path + " " + key, name);
		if (name == "" || !plane.MemoryPointExists(name))
			return fallback;

		return plane.GetMemoryPointPos(name);
	}
};

/*!
	The floats together: what they're made of (GebsFlight Floats), the water's
	forces on them each step, and on clients their spray.
*/
class GebsPlaneFloats
{
	static const float GRAVITY = 9.81;
	static const float SPRAY_KMH = 4;		//!< km/h: slower than this a float throws up no spray
	static const float DEPTH_CHECK_RANGE = 300;	//!< m: GetWaterDepth's reading is checked (WaterDepthAt) only from one this near the water

	//! How GetWaterDepth reads (WaterDepthAt): 0 not known yet, 1 a point's depth under the surface, 2 the water's depth there
	static int s_DepthMode;

	ref array<ref GebsPlaneFloat> m_Floats = new array<ref GebsPlaneFloat>();
	float m_WaterDensity;	//!< kg/m3
	float m_Planing;		//!< the bottom's push per radian of its angle to the water, per square metre wetted, of 1/2 rho v^2
	float m_SkinFriction;
	float m_HumpSpeed;		//!< m/s
	float m_HumpDrag;		//!< of the displaced water's weight, at the hump
	float m_SideDrag;
	float m_HeaveDamping;	//!< N per m/s per m2 of waterline
	float m_RudderArea;		//!< m2, each water rudder
	float m_RudderAngle;	//!< rad at full pedal
	float m_RudderLift;		//!< per rad
	float m_RudderStall;	//!< rad
	float m_RudderFade;		//!< km/h by which they're lifted
	float m_RudderDepth;	//!< m of blade
	float m_AirCdaFwd;		//!< m2, each float's air drag head-on
	float m_AirCdaSide;
	float m_AirCdaUp;
	float m_SlamLoad;		//!< g from the water past which it damages the airframe
	float m_RuinedBuoyancy;	//!< share of the buoyancy a wreck's flooded floats keep
	float m_StepPull;		//!< the keys' held pull up on the step (GebsPlane.ReadPilotInput)

	// this step
	float m_Load;			//!< N the water pushes the plane up with, along its up
	float m_Drag;			//!< N the water holds it back with
	int m_Wet;				//!< slices in the water, all floats

	// clients
	float m_WetShare;		//!< 0 .. 1: the floats in the water, as seen here (for the sounds)

	static GebsPlaneFloats Load(string path, Object plane)
	{
		if (!g_Game.ConfigIsExisting(path))
			return null;

		GebsPlaneFloats fl = new GebsPlaneFloats();
		fl.m_WaterDensity = Value(path, "waterDensity", 1000);
		fl.m_Planing = Value(path, "planing", 0.5);
		fl.m_SkinFriction = Value(path, "skinFriction", 0.003);
		fl.m_HumpSpeed = Math.Max(Value(path, "humpSpeed", 8), 0.5);
		fl.m_HumpDrag = Value(path, "humpDrag", 0.15);
		fl.m_SideDrag = Value(path, "sideDrag", 1);
		fl.m_HeaveDamping = Value(path, "heaveDamping", 1000);
		fl.m_RudderArea = Value(path, "rudderArea", 0);
		fl.m_RudderAngle = Value(path, "rudderAngle", 25) * Math.DEG2RAD;
		fl.m_RudderLift = Value(path, "rudderLift", 2.5);
		fl.m_RudderStall = Value(path, "rudderStall", 20) * Math.DEG2RAD;
		fl.m_RudderFade = Math.Max(Value(path, "rudderFade", 45), 1);
		fl.m_RudderDepth = Math.Max(Value(path, "rudderDepth", 0.3), 0.05);
		fl.m_AirCdaFwd = Value(path, "airCdaForward", 0);
		fl.m_AirCdaSide = Value(path, "airCdaSide", 0);
		fl.m_AirCdaUp = Value(path, "airCdaUp", 0);
		fl.m_SlamLoad = Value(path, "slamLoad", 0);
		fl.m_RuinedBuoyancy = Value(path, "ruinedBuoyancy", 1);
		fl.m_StepPull = Value(path, "stepPull", 1);

		int count = g_Game.ConfigGetChildrenCount(path);
		for (int i = 0; i < count; i++)
		{
			string name;
			g_Game.ConfigGetChildName(path, i, name);
			GebsPlaneFloat f = GebsPlaneFloat.Load(path + " " + name, name, plane);
			if (f)
				fl.m_Floats.Insert(f);
		}

		if (fl.m_Floats.Count() == 0)
			return null;

		return fl;
	}

	protected static float Value(string path, string key, float fallback)
	{
		if (!g_Game.ConfigIsExisting(path + " " + key))
			return fallback;

		return g_Game.ConfigGetFloat(path + " " + key);
	}

	/*!
		How deep a world point is under the water (the sea or a pond), m:
		negative above it. GetWaterDepth is read as a point's depth under the
		surface, as vanilla's boat reads it at its propeller; the first call
		checks that by asking a metre higher too, and should it give the water's
		depth at that spot instead, takes the surface from the ground under it.
		Each machine finds the same, so the floats come out the same on both.
	*/
	static float WaterDepthAt(vector p)
	{
		float depth = g_Game.GetWaterDepth(p);
		// decided once, from a reading that looks like water within reach (a placeholder far from any
		// water, the same at both heights whichever way it reads, could mislead it)
		if (s_DepthMode == 0 && Math.AbsFloat(depth) < DEPTH_CHECK_RANGE)
		{
			float higher = g_Game.GetWaterDepth(p + Vector(0, 1, 0));
			if (depth - higher > 0.5)
				s_DepthMode = 1;
			else
				s_DepthMode = 2;
			Print(string.Format("GebsPlanes: GetWaterDepth reads %1 (%2 at %3, %4 a metre above)", DepthModeName(), depth, p, higher));
		}

		if (s_DepthMode == 2)
			return depth + g_Game.SurfaceY(p[0], p[2]) - p[1];

		return depth;
	}

	static string DepthModeName()
	{
		if (s_DepthMode == 1)
			return "a point's depth under the surface";
		if (s_DepthMode == 2)
			return "the water's depth at a spot";

		return "unknown";
	}

	bool IsWet()
	{
		return m_Wet > 0;
	}

	//! One physics step. fwdW, sideW and upW are the plane's axes in the world (SimDir); rudder the pedals, -1 .. 1.
	void Simulate(GebsPlane plane, float dt, vector fwdW, vector sideW, vector upW, float rudder, bool ruined)
	{
		m_Load = 0;
		m_Drag = 0;
		m_Wet = 0;
		if (dt <= 0)
			return;

		float rho = m_WaterDensity;
		vector worldUp = "0 1 0";
		float buoyancy = 1;
		if (ruined)
			buoyancy = m_RuinedBuoyancy;

		vector vel = plane.SimVelocity();
		vector flow = vel * (1 / Math.Max(vel.Length(), 0.1));
		// up on the step the water breaks away from it and leaves a hollow behind: the afterbody runs over it
		// dry, the water there as low as the step's keel, fully so from twice the hump's speed
		float vent = Math.Clamp((vector.Dot(vel, fwdW) - m_HumpSpeed) / m_HumpSpeed, 0, 1);
		float rhoAir = GebsPlane.AirDensity(plane.SimPoint(vector.Zero)[1]);

		foreach (GebsPlaneFloat f : m_Floats)
		{
			f.m_Wet = 0;
			f.m_Load = 0;
			float stepDepth = Math.Max(WaterDepthAt(plane.SimPoint(f.m_StepKeel)), 0) * vent;

			foreach (GebsPlaneFloatSlice s : f.m_Slices)
			{
				vector keel = plane.SimPoint(s.m_Keel);
				float depth = WaterDepthAt(keel);
				if (s.m_Aft)
					depth -= stepDepth;
				if (depth <= 0)
					continue;

				// the section under the water: the vee, then the upright sides
				float d = Math.Min(depth, s.m_Deck);
				float w = s.m_HalfBeam;
				float hc = s.m_Chine;
				float area;
				float beam;
				float girth;
				if (d <= hc)
				{
					float half = w * d / hc;
					area = w * d * d / hc;
					beam = 2 * half;
					girth = 2 * Math.Sqrt(half * half + d * d);
				}
				else
				{
					area = w * hc + 2 * w * (d - hc);
					beam = 2 * w;
					girth = 2 * Math.Sqrt(w * w + hc * hc) + 2 * (d - hc);
				}

				vector point = keel + upW * (0.5 * d);
				vector v = dBodyGetVelocityAt(plane, point);
				vector normal = upW * (-s.m_SlopeCos) + fwdW * s.m_SlopeSin;	// the bottom's, outward
				vector along = fwdW * s.m_SlopeCos + upW * s.m_SlopeSin;		// along the bottom, to the bow

				// buoyed up by what it displaces
				vector force = worldUp * (rho * GRAVITY * area * s.m_Length * buoyancy);
				// the bottom meeting the water is pushed back out: planing
				float into = vector.Dot(v, normal);
				if (into > 0)
					force = force - normal * (0.5 * rho * m_Planing * beam * s.m_Length * v.Length() * into);
				// damped going up and down
				force = force - worldUp * (m_HeaveDamping * beam * s.m_Length * v[1]);
				// along the keel: the waves it makes, its skin
				float vt = vector.Dot(v, along);
				float x = Math.AbsFloat(vt) / m_HumpSpeed;
				float resist = rho * GRAVITY * area * s.m_Length * m_HumpDrag * 2 * x * x / (1 + x * x * x * x);
				resist += 0.5 * rho * m_SkinFriction * girth * s.m_Length * vt * vt;
				if (vt < 0)
					resist = -resist;
				force = force - along * resist;
				// across: a keel's grip
				float vs = vector.Dot(v, sideW);
				force = force - sideW * (0.5 * rho * m_SideDrag * d * s.m_Length * vs * Math.AbsFloat(vs));

				plane.ApplyForceAt(point, force);
				f.m_Wet++;
				f.m_Load += vector.Dot(force, upW);
				m_Drag -= vector.Dot(force, flow);
			}

			m_Wet += f.m_Wet;
			m_Load += f.m_Load;

			// the water rudder: its blade's share under the water, of use at a taxi, lifted by m_RudderFade
			if (m_RudderArea > 0)
			{
				vector rudderPoint = plane.SimPoint(f.m_Rudder);
				float share = Math.Clamp(0.5 + WaterDepthAt(rudderPoint) / m_RudderDepth, 0, 1);
				if (share > 0)
				{
					vector rv = dBodyGetVelocityAt(plane, rudderPoint);
					float vf = vector.Dot(rv, fwdW);
					float vsr = vector.Dot(rv, sideW);
					float fade = Math.Clamp(1 - Math.AbsFloat(vf) * 3.6 / m_RudderFade, 0, 1);
					if (fade > 0)
					{
						float a = m_RudderAngle * rudder + Math.Atan2(vsr, Math.Max(Math.AbsFloat(vf), 0.3));
						a = Math.Clamp(a, -m_RudderStall, m_RudderStall);
						float q = 0.5 * rho * (vf * vf + vsr * vsr) * m_RudderArea * share * fade;
						vector rudderForce = sideW * (-q * m_RudderLift * a);
						float sp = Math.Sqrt(vf * vf + vsr * vsr);
						if (sp > 0.05)
							rudderForce = rudderForce - (fwdW * vf + sideW * vsr) * (q * (0.03 + 0.5 * a * a) / sp);
						plane.ApplyForceAt(rudderPoint, rudderForce);
					}
				}
			}

			// the float's own air drag, at its middle: one of the air's forces, which a plane flown arcade takes over
			vector centre = plane.SimPoint(f.m_Centre);
			vector cv = dBodyGetVelocityAt(plane, centre);
			float cu = vector.Dot(cv, fwdW);
			float cs = vector.Dot(cv, sideW);
			float cn = vector.Dot(cv, upW);
			vector drag = fwdW * (cu * Math.AbsFloat(cu) * m_AirCdaFwd) + sideW * (cs * Math.AbsFloat(cs) * m_AirCdaSide) + upW * (cn * Math.AbsFloat(cn) * m_AirCdaUp);
			plane.AirForceAt(centre, drag * (-0.5 * rhoAir));
		}
	}

	/*!
		Every frame on clients: each float's spray where its forebody meets the
		water and at its step, by its speed (vanilla's boat's own spray), and
		how much of the floats is in the water, for the sounds. Worked out
		here from where the plane is drawn, so other players' planes spray too.
	*/
	void UpdateEffects(GebsPlane plane, float dt)
	{
		float kmh = GetVelocity(plane).Length() * 3.6;
		int wet = 0;
		foreach (GebsPlaneFloat f : m_Floats)
		{
			if (!f.m_FrontSpray)
			{
				f.m_FrontSpray = new GebsFloatSpray(true);
				f.m_FrontSpray.Attach(plane, f.m_SprayFront);
				f.m_SideSpray = new GebsFloatSpray(false);
				f.m_SideSpray.Attach(plane, f.m_SpraySide);
			}

			if (f.m_SideSpray.Update(plane, dt, kmh))
				wet++;
			f.m_FrontSpray.Update(plane, dt, kmh);
		}

		m_WetShare = wet / Math.Max(m_Floats.Count(), 1);
	}

	void StopEffects()
	{
		foreach (GebsPlaneFloat f : m_Floats)
		{
			if (f.m_FrontSpray)
				f.m_FrontSpray.Finish();
			if (f.m_SideSpray)
				f.m_SideSpray.Finish();
		}
	}
};

/*!
	One float's spray: vanilla's boat's own (its bow's, or its side's wake),
	on while the float there is in the water and moving, its emitters set by
	the speed as the boat's are, and kept on the water's surface.
*/
class GebsFloatSpray : EffectParticle
{
	static const float POS_UPDATE = 0.2;	//!< s between moves to the water's surface (a reattach)

	protected bool m_Front;
	protected int m_EmitorCount;
	protected int m_Band = -1;			//!< speed band the emitters were last set for
	protected float m_PosTimer;
	protected vector m_Point;			//!< model space: on the keel

	void GebsFloatSpray(bool front)
	{
		m_Front = front;
		if (front)
		{
			m_EmitorCount = 13;
			SetParticleID(ParticleList.BOAT_WATER_FRONT);
		}
		else
		{
			m_EmitorCount = 5;
			SetParticleID(ParticleList.BOAT_WATER_SIDE);
		}
	}

	void Attach(Object plane, vector point)
	{
		m_Point = point;
		AttachTo(plane, point);
	}

	protected void Emit(Particle ptc, int id, bool on)
	{
		if (on)
		{
			ptc.SetParameter(id, EmitorParam.BIRTH_RATE, ptc.GetParameterOriginal(id, EmitorParam.BIRTH_RATE));
			ptc.SetParameter(id, EmitorParam.BIRTH_RATE_RND, ptc.GetParameterOriginal(id, EmitorParam.BIRTH_RATE_RND));
		}
		else
		{
			ptc.SetParameter(id, EmitorParam.BIRTH_RATE, 0);
			ptc.SetParameter(id, EmitorParam.BIRTH_RATE_RND, 0);
		}
	}

	//! Every frame; whether the float is in the water here.
	bool Update(Object plane, float dt, float kmh)
	{
		vector world = plane.ModelToWorld(m_Point);
		float depth = GebsPlaneFloats.WaterDepthAt(world);
		bool wet = depth > -0.1;
		if (!wet || kmh < GebsPlaneFloats.SPRAY_KMH)
		{
			Finish();
			return wet;
		}

		if (!IsPlaying())
		{
			m_Band = -1;
			m_PosTimer = POS_UPDATE;
			Start();
		}

		Particle ptc = GetParticle();
		if (!ptc)
			return wet;

		// the speed bands, as vanilla's boat's: slowest, slow (5 km/h), medium (20), fast (40)
		int band = 0;
		if (kmh > 40)
			band = 3;
		else if (kmh > 20)
			band = 2;
		else if (kmh > 5)
			band = 1;

		if (band != m_Band)
		{
			m_Band = band;
			for (int i = 0; i < m_EmitorCount; i++)
			{
				if (m_Front)
				{
					if (i == 5 || i == 6)
						Emit(ptc, i, band == 3);	// fast drops
					else if (i == 9 || i == 10)
						Emit(ptc, i, band == 2);	// slow drops
					else if (i < 5 || i == 7 || i == 8)
						Emit(ptc, i, band >= 2);	// the bow's splashes
				}
				else if (i < 3)
				{
					Emit(ptc, i, band >= 2);		// the side's splashes
				}
			}
		}

		float lerp = Math.Clamp(kmh / 40, 0, 1);
		float big = Math.Clamp((kmh - 15) / 35, 0, 1);
		for (int k = 0; k < m_EmitorCount; k++)
		{
			if (m_Front)
			{
				if (k == 11 || k == 12)
					ptc.SetParameter(k, EmitorParam.VELOCITY, ptc.GetParameterOriginal(k, EmitorParam.VELOCITY) * lerp);
				else if (band >= 2 && (k < 5 || k == 7 || k == 8))
					ptc.SetParameter(k, EmitorParam.SIZE, ptc.GetParameterOriginal(k, EmitorParam.SIZE) * big);
			}
			else
			{
				ptc.SetParameter(k, EmitorParam.SIZE, ptc.GetParameterOriginal(k, EmitorParam.SIZE) * lerp);
				if (k >= 3)
					ptc.SetParameter(k, EmitorParam.VELOCITY, ptc.GetParameterOriginal(k, EmitorParam.VELOCITY) * lerp);
			}
		}

		// on the water's surface, a little under it, as the boat keeps its own
		m_PosTimer += dt;
		if (m_PosTimer >= POS_UPDATE)
		{
			m_PosTimer = 0;
			vector surface = world;
			surface[1] = world[1] + depth - 0.2;
			SetCurrentLocalPosition(plane.WorldToModel(surface));
		}

		return wet;
	}

	//! Off: no more water thrown up, what's in the air falls.
	void Finish()
	{
		if (!IsPlaying())
			return;

		Particle ptc = GetParticle();
		if (ptc)
		{
			ptc.SetParticleParam(EmitorParam.BIRTH_RATE, 0);
			ptc.SetParticleParam(EmitorParam.BIRTH_RATE_RND, 0);
		}
		Stop();
	}
};
