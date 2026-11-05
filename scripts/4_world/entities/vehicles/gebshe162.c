/*
    Heinkel He 162 A-2: the common aircraft's controls, gear, fuel, damage,
    persistence and deterministic network simulation, with BMW 003 jet thrust.
    The inherited governed shaft state carries turbine rpm through corrections.
    There is no propeller slipstream, propeller torque or afterburner.
*/
class GebsHe162_Base : GebsPlane
{
    protected Particle m_JetExhaust;
    protected GebsHe162ExhaustLight m_JetHeat;

    override int GetAnimInstance()
    {
        return VehicleAnimInstances.GOLF;
    }

    override int GetSeatAnimationType(int posIdx)
    {
        return DayZPlayerConstants.VEHICLESEAT_DRIVER;
    }

    override float GetTransportCameraDistance()
    {
        return 9.0;
    }

    override vector GetTransportCameraOffset()
    {
        return "0 1.2 0";
    }

    /*
        A turbojet delivers momentum thrust, rather than shaft power divided
        by airspeed. The modest ram/altitude correction is an approximation,
        not a compressor map. Spool lag remains in the shared owner state.
    */
    override protected float PropThrust(float speedFwd, float rho, float altitude)
    {
        float density = Math.Pow(Math.Max(rho / SEA_LEVEL_DENSITY, 0.05), 0.75);
        float ram = 1 - 0.12 * Math.Clamp(Math.Max(speedFwd, 0) / 240, 0, 1);
        return Math.Max(m_EnginePower, 0) * m_StaticThrust * density * ram;
    }

    override float GovernedPropRpm(float throttle, float speedFwd, int engineState, float crankTime)
    {
        if (engineState == GebsEngineState.RUNNING)
        {
            float spool = Math.Clamp((m_EnginePower - m_IdlePower) / Math.Max(1 - m_IdlePower, 0.01), 0, 1);
            return (m_RpmIdle + (m_RpmMax - m_RpmIdle) * Math.Sqrt(spool)) / m_GearRatio;
        }

        if (engineState == GebsEngineState.CRANKING)
        {
            // The Riedel starter brings the compressor up before light-off.
            float start = Math.Clamp(crankTime / Math.Max(m_CatchTime, 0.1), 0, 1);
            return m_StarterRpm * start / m_GearRatio;
        }

        // The small freewheeling airflow does not drive a propeller disc.
        return Math.Max(speedFwd, 0) * m_RpmPerSpeed;
    }

    override float GaugeValue(string what)
    {
        float spool = Math.Clamp(GetEngineRpm() / Math.Max(m_RpmMax, 1), 0, 1);
        if (what == "egt")
        {
            if (GetEngineState() == GebsEngineState.RUNNING)
                return 350 + 410 * spool * spool;
            return 15 + 300 * spool;
        }

        if (what == "oilPressure")
            return 7.5 * spool;

        return super.GaugeValue(what);
    }

    override void EOnFrame(IEntity other, float timeSlice)
    {
        super.EOnFrame(other, timeSlice);

        #ifndef SERVER
        UpdateJetExhaust();
        #endif
    }

    protected void UpdateJetExhaust()
    {
        if (!m_FlightLoaded || !MemoryPointExists("exhaust") || !MemoryPointExists("exhaust_dir"))
            return;

        // The model's flat red nozzle disc stays hidden: the flame particle replaces it.
        SetPhase("jet_exhaust_off", 1);

        float spool = Math.Clamp(GetEngineRpm() / Math.Max(m_RpmMax, 1), 0, 1);
        bool running = GetEngineState() == GebsEngineState.RUNNING;
        bool starting = GetEngineState() == GebsEngineState.CRANKING;
        float heat = Math.Clamp((spool - 0.15) / 0.85, 0, 1);
        if (running)
            heat = Math.Max(heat, 0.2);

        if ((!running && !starting && spool < 0.08) || IsDamageDestroyed())
        {
            StopJetExhaust();
            return;
        }

        vector nozzle = GetMemoryPointPos("exhaust");
        vector aft = GetMemoryPointPos("exhaust_dir") - nozzle;
        aft.Normalize();

        if (!m_JetExhaust)
            m_JetExhaust = Particle.PlayOnObject(ParticleList.GEBS_HE162_JETFLAME, this, nozzle, JetFlameOrientation(aft));

        if (m_JetExhaust)
        {
            // Starting: a faint flicker. Idle: a short orange tongue. Full power: a long, dense jet.
            float flame = heat;
            if (!running)
                flame = 0.1 * spool;
            m_JetExhaust.ScaleParticleParamFromOriginal(EmitorParam.BIRTH_RATE, 0.15 + 0.85 * flame);
            m_JetExhaust.ScaleParticleParamFromOriginal(EmitorParam.SIZE, 0.6 + 0.6 * flame);
            m_JetExhaust.ScaleParticleParamFromOriginal(EmitorParam.VELOCITY, 0.35 + 0.9 * flame);
            m_JetExhaust.ScaleParticleParamFromOriginal(EmitorParam.LIFETIME, 0.6 + 0.8 * flame);
        }

        if (!m_JetHeat)
        {
            m_JetHeat = GebsHe162ExhaustLight.Cast(ScriptedLightBase.CreateLight(GebsHe162ExhaustLight, ModelToWorld(nozzle)));
            if (m_JetHeat)
                m_JetHeat.AttachOnObject(this, nozzle + aft * 0.35);
        }

        if (m_JetHeat)
            m_JetHeat.SetSpool(heat, running);
    }

    //! The flame particle shoots along its own +Y: this turns +Y onto the nozzle's aft axis (model space).
    protected vector JetFlameOrientation(vector aft)
    {
        vector side = "0 1 0";
        if (Math.AbsFloat(vector.Dot(side, aft)) > 0.95)
            side = "0 0 1";
        vector dir = side - aft * vector.Dot(side, aft);
        dir.Normalize();

        vector mat[4];
        Math3D.DirectionAndUpMatrix(dir, aft, mat);
        vector rot[3];
        rot[0] = mat[0];
        rot[1] = mat[1];
        rot[2] = mat[2];
        return Math3D.MatrixToAngles(rot);
    }

    protected void StopJetExhaust()
    {
        if (m_JetExhaust)
        {
            m_JetExhaust.Stop();
            m_JetExhaust = null;
        }
        if (m_JetHeat)
        {
            m_JetHeat.FadeOut(0.2);
            m_JetHeat = null;
        }
    }

    override void EEDelete(EntityAI parent)
    {
        StopJetExhaust();
        super.EEDelete(parent);
    }
};

class GebsHe162A2 : GebsHe162_Base
{
    override void OnDebugSpawn()
    {
        super.OnDebugSpawn();
        GetInventory().CreateInInventory("GebsHe162_Wheel");
        GetInventory().CreateInInventory("GebsHe162_Wheel");
        GetInventory().CreateInInventory("GebsHe162_NoseWheel");
    }
};

/* The orange glow the flame casts on the tail, with the heat distortion behind the nozzle. */
class GebsHe162ExhaustLight : PointLightBase
{
    void GebsHe162ExhaustLight()
    {
        SetVisibleDuringDaylight(true);
        SetCastShadow(false);
        SetRadiusTo(2.5);
        SetBrightnessTo(0.02);
        SetDiffuseColor(1.0, 0.45, 0.1);
        SetAmbientColor(0.15, 0.06, 0.01);
        SetFlareVisible(false);
        EnableHeatHaze(true);
        SetHeatHazeRadius(0.18);
        SetHeatHazePower(0.006);
        SetFadeOutTime(0.2);
    }

    void SetSpool(float heat, bool burning)
    {
        SetHeatHazeRadius(0.12 + 0.20 * heat);
        SetHeatHazePower(0.003 + 0.010 * heat);
        float glow = 0;
        if (burning)
            glow = 0.15 + 0.6 * heat;
        SetBrightnessTo(glow);
    }
};
