/*
	A plane's looping sounds, on clients: the engine's while it runs, the
	wind's while it moves.

	A car's engine sound is run by the car simulation, which feeds its sound
	sets' controllers (rpm, thrust, ...). A plane has no car underneath, so it
	plays the same kind of looping sound sets itself and feeds them: each set
	gets a sound object built with those controllers as variables, updated
	every frame. The sets crossfade among themselves by rpm in their shaders.

	Each loop keeps its own slot. When the game stops a loop's wave (opening
	the UFO's canopy with the drive on cut its hum off, Cole 2026-10-28), the
	wave's stopped or ended event empties its slot and the next Update starts
	it again, no sooner than RETRY_TIME after the last start - as Alien
	Invasion's UFO does, whose loops this follows.
*/
class GebsPlaneEngineSound : Managed
{
	static const float RETRY_TIME = 0.25;	//!< s between starts of the same loop

	//! The controllers the shaders read, in the order Update() fills m_Values.
	static const ref array<string> CONTROLLERS = {"rpm", "thrust", "speed", "engineOn", "doors", "campos"};

	protected ref array<ref SoundParams> m_Params = {};
	protected ref array<ref SoundObjectBuilder> m_Builders = {};
	protected ref array<ref SoundObject> m_Objects = {};
	protected ref array<AbstractWave> m_Waves = {};		//!< one per sound object, null while it isn't playing
	protected ref array<float> m_StartedAt = {};		//!< tick time each slot last started
	protected ref array<float> m_Values = {0, 0, 0, 0, 0, 1};
	protected bool m_Running;
	protected int m_Restarts;

	void GebsPlaneEngineSound(Object owner, vector position, TStringArray soundSets)
	{
		foreach (string soundSet : soundSets)
		{
			SoundParams params = new SoundParams(soundSet);
			if (!params.IsValid())
				continue;

			SoundObjectBuilder builder = new SoundObjectBuilder(params);
			foreach (string controller : CONTROLLERS)
				builder.AddVariable(controller, 0);

			SoundObject sound = builder.BuildSoundObject();
			if (!sound)
				continue;

			sound.SetParent(owner);
			sound.SetPosition(position);
			m_Params.Insert(params);
			m_Builders.Insert(builder);
			m_Objects.Insert(sound);
			m_Waves.Insert(null);
			m_StartedAt.Insert(-RETRY_TIME);
		}
	}

	void ~GebsPlaneEngineSound()
	{
		Stop();
	}

	bool IsPlaying()
	{
		return m_Running;
	}

	//! Loops the game stopped and this started again since it was made
	int Restarts()
	{
		return m_Restarts;
	}

	/*!
		rpm on the sound's scale, thrust 0 .. 1, speed in km/h, doors 0 shut ..
		1 open, campos 1 with the camera outside.
	*/
	void Update(bool running, float rpm, float thrust, float speed, float doors, float campos)
	{
		if (!running)
		{
			Stop();
			return;
		}

		m_Values[0] = rpm;
		m_Values[1] = thrust;
		m_Values[2] = speed;
		m_Values[3] = 1;
		m_Values[4] = doors;
		m_Values[5] = campos;
		foreach (SoundObject sound : m_Objects)
			sound.UpdateVariables(m_Values);

		float now = g_Game.GetTickTime();
		for (int i = 0; i < m_Objects.Count(); i++)
		{
			if (m_Waves[i] || now - m_StartedAt[i] < RETRY_TIME)
				continue;

			AbstractWave wave = g_Game.GetSoundScene().Play3D(m_Objects[i], m_Builders[i]);
			if (!wave)
				continue;

			wave.Loop(true);
			AbstractWaveEvents events = wave.GetEvents();
			if (events)
			{
				events.Event_OnSoundWaveStopped.Insert(OnWaveStopped);
				events.Event_OnSoundWaveEnded.Insert(OnWaveStopped);
			}

			if (m_Running)
				m_Restarts++;
			m_Waves[i] = wave;
			m_StartedAt[i] = now;
		}
		m_Running = true;
	}

	//! A loop's wave stopped or ended: empty its slot, so Update starts it again while it should play
	protected void OnWaveStopped(AbstractWave wave)
	{
		int i = m_Waves.Find(wave);
		if (i != -1)
			m_Waves[i] = null;
	}

	void Stop()
	{
		m_Running = false;
		for (int i = 0; i < m_Waves.Count(); i++)
		{
			AbstractWave wave = m_Waves[i];
			m_Waves[i] = null;
			if (wave)
				wave.Stop();
		}
	}
};
