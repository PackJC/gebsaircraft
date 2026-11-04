# Changelog

## v0.3.0

**Renamed gebsaircraft.** The mod's folder, PBO prefix and every path in it are now `gebsaircraft` (it was GebsPlanes): pack it under that prefix into `@gebsaircraft`. The classes, keybinds and saved planes are the same.

### New Aircraft

- **Messerschmitt Bf 109 E-3.** The single-seat fighter of 1940, in a Channel-front scheme: RLM 71/02 splinter over RLM 65, yellow nose, rudder and wingtips, the red "5" and the ace of hearts. No guns fitted yet
  - **A DB 601 on a constant-speed propeller.** 1,175 PS. The governor holds the revs the throttle sets, from 650 to 2,400, and the supercharger keeps full power up to about 4,000 m
  - **Real handling.** About 480 km/h flat out low down, 16 m/s climb, a stall at about 145 km/h clean and 130 with full flaps, and 110° a second of roll at 400 km/h. The leading-edge slats slide out by themselves near the stall. The torque, the slipstream and the propeller's gyroscopic kick all swing and roll it, and a fixed aileron tab holds the wings level at cruise
  - **Retractable gear (G).** Hydraulic: the legs fold outward into the wing in 6 seconds with the engine running. They won't fold with weight on the wheels, and they come down without the engine, slowly. Green and red lamps on the panel
  - **Flaps** on a handwheel, wound down in four steps to 42°. The ailerons droop with them. The whole tailplane trims, as on the real one
  - **A working cockpit.** Fifteen German instruments with moving needles: airspeed, altimeter (two hands), turn and slip, compass, variometer, rpm, boost, propeller pitch clock, clock, fuel, fuel and oil pressure, brake pressure, coolant and oil temperature, ammeter. The stick, rudder pedals, throttle lever, flap wheel and gear lever move with the controls, and the placards are in German
  - **The canopy** is hinged on its right and swings over
- **Getting in is easy.** Hold F anywhere near the cockpit while looking at the plane; the door or canopy opens by itself, getting in and getting out
  - **The inertia starter.** Hold the start action and the flywheel winds up, engages and turns the engine over, and it fires about four seconds later
  - **Its own sounds,** made from how a DB 601 sounds: the V12's firing note and crackling stubs, the propeller's beat, the supercharger's whine, muffled in the cockpit. Also the starter, the catch, the shutdown, the gear's hydraulics and lock, the flap wheel, the canopy and the wind
- **Bf 109 W on floats.** The E-3 on twin floats, from the sea and the lakes: its gear folded away for good, a float under each wing on one broad strut, braced forward to the engine
  - **The floats float.** Each is simulated in twelve slices: buoyancy, the bottom planing on the step, the waves it makes (the hump of the takeoff run), skin friction, a keel's grip sideways and the step's hollow that leaves the afterbody dry. It rests half a metre deep, the propeller well clear, and rights itself if tipped
  - **A floatplane's takeoff.** Stick back over the hump, then on the step from about 60 km/h, off at about 170 km/h after some 850 m. On keys, S held eases to the planing attitude on the step by itself, so the floats coming off don't throw the nose up into a stall
  - **Water rudders** on the pedals for taxiing, up to about 45 km/h. No brakes on the water: it creeps at about 6 km/h at idle
  - **About 400 km/h flat out and 9 m/s climb**: the floats' drag, and 290 kg more
  - **Landing on the water** in the planing attitude runs on for 20 s. Too hard (over 5 g) damages it, a propeller in the water drowns the engine, and a wreck's floats flood and it sinks
  - **Spray and sounds** from vanilla's boat: spray at each float's bow and step, the water lapping and rushing, a splash on touchdown
  - **Getting in from the water:** a swimmer alongside can climb in
  - Its floats painted to match: RLM 71/02 decks, RLM 65 below, panel lines, rivets and a waterline stain
- **Super Cub on floats, flown arcade.** The easy plane: the Cub's landing gear off, aluminium floats on struts from the gear's fittings with spreader bars across, water rudders, and a 180 hp engine
  - **It flies itself.** The keys ask for turn rates and an assist turns the plane at them. Let go and the wings roll level and it flies straight and level, at any throttle. Hold A or D and it banks to 60° and turns about 20° a second without losing height or needing rudder; hold S and it settles into a steady 5 m/s climb; hold W and it dives no steeper than 40°, pulling out when you let go
  - **It can't stall or spin.** Near the stall the nose comes down by itself, and a held pull climbs only as steeply as the speed keeps up
  - **Off the water by itself.** Full throttle and hands off: up on the step and off at about 110 km/h in 18 s. With S held, off at 100 km/h after 200 m
  - **Lands by itself.** Throttle closed and hands off, it settles onto the water at about 1.5 m/s
  - **Turns on the spot on the water** with Z and C, about 30° a second
  - About 200 km/h flat out
- **Winter and desert camouflage for every plane.** Each plane comes in two more paint schemes, its floats painted to match:
  - **Bf 109 E-3 and W, winter:** whitewashed in the field over the splinter and the fuselage mottle, brushed on and thin in streaks, the crosses, the red 5 and the ace of hearts masked round, the yellow cowling, tail and wingtips and the light blue undersides left
  - **Bf 109 E-3 and W, desert:** a 1941 tropical scheme, RLM 79 sand with soft RLM 80 olive mottles over RLM 78, the cowling and tail sand too, a white band round the rear fuselage and white wingtips
  - **Super Cub and Cub on floats:** white with grey disruptive patches, or sand with brown, without the stripe and the registration

### Every Plane

- **Eject from a moving plane.** Once a plane is moving faster than a walk (8 km/h), Get out is no longer offered. Open the door or canopy from the seat (F) and Eject appears instead: hold F. You jump out beside the door, falling from there in the air; jumping from a plane rolling along the ground hurts as jumping out of a moving car does. The door doesn't open by itself for this
- **The Cub's tail no longer bounces on the takeoff run.** Its tailwheel spring ran out of travel under the tail's weight, so the tail sat on its hard collision block, bounced off it and lifted at 16 km/h, and the plane pitched up and down until it went over on its nose. The tailwheel now rides a longer, stiffer, damped spring with the block well clear. Held S also stops short of the stall now, as on the Bf 109: with it held down the runway the Cub lifts off at about 90 km/h and climbs away
- **Arcade flying for any plane** whose config has an `Arcade` class in its `GebsFlight`: the assist the Cub on floats flies with, its rates, limits and the water attitude all set there. It works from the plane's own state and the controls in the network move, so the server and the pilot's machine fly it alike
- **The floats are shared.** The float shapes, their painter and their simulation now serve any floatplane: the Bf 109 W's floats come out exactly as before

- **Ground effect over water.** The wing's ground effect now takes its height from the water's surface too, not the sea bed under it
- **Built on Alien Invasion's UFO, which flies:**
  - The engine's switches are thrown on the server and the pilot's machine alike, so starting never waits on the network.
  - Once the start action has run its course, the starter latches and the engine turns over by itself. The seated animation is too short for a whole start: that is why the Cub wouldn't start in its first v0.2 test.
  - The plane keeps itself awake while crewed, running or moving, and checks every frame.
  - A pilot taking the seat picks up the engine, gear and controls as they are.
  - Engine sounds that the game stops are started again.
- **Smooth on the pilot's machine.** The server corrects the pilot's plane two or three times a second, as it does every vehicle, and the pilot's machine replays its last moves within one frame. The plane worked its forces out from where the game drew it, which doesn't move during a replay, so every replay came out wrong and every correction was a jump: the tail bouncing on the taxiway while the server's plane sat still, and the plane skipping about in the air. The simulation now works from the physics body's own position, as vanilla's boat does
- **The Bf 109's gear folds into its wells.** It turned 85° about a fore-and-aft hinge and left the wheels lying on top of the wing. The hinge axis and angle are now searched for against the wing itself: the wheels end up in the model's wells, at most 5 cm out above and 8 cm below
- **Wheels spin true.** Their animation phases now run 0 to 1, as every other source's does (in radians a wheel could turn 57° and snap back), a wheel's spin axis moves with its leg or castor, and a wheel stops spinning while its leg folds. The tailwheel's swing shows both ways. Both planes
- **The Bf 109 no longer rocks onto a wingtip on the takeoff run.** Between 90 and 130 km/h the wing carries half the weight, and its tyres gripped sideways as on rails, so a second of rudder rolled it onto one wheel and then a wingtip, bouncing it from wheel to wheel. A tyre's side grip now builds with its slip angle, full by 8°, as a real tyre's does (`slipPeak`). The Cub's tyres are as they were
- **S held no longer pulls the Bf 109 into the stall.** On keys the stick went all the way back, far past the stall, and a wing dropped. A held pull now slows as the wing nears its stall or the g nears 6, and stops there, as a pilot's hand stops at the first shudder (`stallMargin`, `pullG`). Let go, the stick comes back to the middle more gently, so the nose doesn't drop at once. Every plane's stick comes back that way; the stop is the Bf 109's only
- **The test readout** counts the server's corrections each second on the pilot's machine, and shows how near the wing is to its stall, the g and how much pull is left
- **Planes are no longer held back by an invisible drag.** A new physics body comes with the engine's own damping, about 0.12 x mass x speed: at full power it held the Bf 109 to 100 km/h on the runway, short of flying speed, and it slowed the Cub too. The planes now turn it off, and all their drag is their own
- **The wheels can't sink through the ground any more.** Squeezed past its travel, a wheel now meets a stiff bump stop, and its ray starts higher, so it always finds the ground. Before, the Bf 109's tail sank through when it was spawned and the plane scraped along on its body: juddering, wheels wobbling, and never faster than 95 km/h
- **The tyres no longer rock a parked plane.** Each tyre now stops its slip with the plane's real effective mass at the contact. Before, a parked plane rocked from wheel to wheel every physics step (found flying the Bf 109 in a simulator of the same code)
- **Dihedral.** A wing panel can now rise towards its tip, as real wings do. In a sideslip it rolls the plane level, the spiral stability every plane has. Without it a plane let go slowly rolled into a dive
- **The test readout** now writes a line on the server as well as the pilot's machine: who simulates the plane, steps and moves a second, awake, the switches. The flight line adds rpm, height, the slats, roll and the gear
- The HUD shows the gear and the boost for planes that have them, and the flaps in degrees
- A saved plane now keeps its gear lever and its door or canopy. Planes saved by v0.2 still load

## v0.2.0

**Delete every plane on the server before updating.** Planes are saved differently now, and an old one won't load.

### Rebuilt Without a Car Underneath

The plane flew, but jolted in the air: the server was correcting it up to 30 times a second. It is rebuilt as a bare vehicle with its own simulation, with no car physics fighting the flight model.

- **Its own wheels.** Each wheel finds the ground with a ray, rides a spring and damper and grips up to what its load allows. The tailwheel steers on the rudder pedals and narrows its swing as the speed builds. At a crawl the rudder also brakes the main wheel on its side for tight turns, as a Cub pilot's toe brakes do. The main wheels are still removable wheel items: a damaged one is smaller and grips less
- **Its own engine.** Hold the start action and the starter cranks it until it fires. It runs on its fuel until it's switched off, runs dry or is ruined. There are no gears any more: standing still with the throttle shut and the pedals centred, the brakes hold it
- **The same on both machines.** The ignition and the starter ride in the network move with the controls, so the engine starts on the same step on the pilot's machine and the server, and every force goes in through the engine's deterministic vehicle physics
- **Its own fuel tank,** filled from a jerrycan under the left wing root with the usual refuel action. A new plane comes with half tanks
- **An instrument panel on the HUD:** airspeed, altitude and height above the ground, climb, throttle and propeller rpm, fuel, flaps, trim, and the engine and brakes
- **Crashes do damage.** A hard knock damages the engine if it hits near the nose, the airframe anywhere else
- **The engine sounds** come from the plane itself now, following the propeller's rpm, with the starter's crank and the shutdown
- **The door** opens and closes from either seat as well as from outside
- **The tailwheel is part of the plane.** Its two hidden half-wheel items are gone

## v0.1.0

### New Aircraft

- **Piper PA-18 Super Cub.** The 150 hp two-seat tandem bush plane on fat tundra tyres. It takes off from a field in about 70 metres, climbs at about 5 m/s and tops out around 200 km/h. It stalls at 74 km/h clean and 69 km/h with full flaps, numbers taken from the real aeroplane
- **It flies like a plane.** Each wing panel and tail surface gets its own airflow, so a wing that stalls first drops and can spin the plane. The engine's torque and the propeller's slipstream swing it on the takeoff run, the wing's downwash and ground effect are there, and DayZ's wind blows on it
- **Full controls.** Stick, rudder, a throttle lever that stays put, three flap settings, trim and wheel brakes, on keys you can rebind under Settings > Controls > Gebs Planes. The rudder steers the tailwheel on the ground
- **Moving parts.** The elevator, rudder, ailerons and flaps move with the controls, for the pilot and for everyone watching. The propeller spins and blurs into a disc at speed. The tailwheel steers and both main wheels turn on their suspension
- **A door that opens.** The Cub's split door folds open: the lower half drops and the window swings up under the wing. Open it from outside or from the seat, and get in only through an open door
- **A Cub that has been flown.** Painted from scratch, since the model came without textures:
  - **Livery:** Cub Yellow dope with a black lightning stripe down each side and the registration N7842B.
  - **Fabric:** rib tapes and stitching over every rib, and the fabric sagging between them.
  - **Propeller:** polished aluminium in front, black behind, yellow tips.
  - **Panel:** five gauges on the instrument panel.
  - **Wear:** the top faded by the sun, small paint chips on the leading edges and round the door, bugs on the wing, exhaust soot and oil streaked down the belly, dust and mud low down and behind the wheels.
- **One plane, two wheels to lose.** No battery, spark plug or radiator. The main wheels come off like car wheels, and the tailwheel is part of the plane
- **Multiplayer.** The flight controls travel in the vehicle's network move, so the server flies the same plane as the pilot

### Fixed After the First Test Flight

- **The plane was a mirror image.** The door was on the left, the registration read backwards and the aileron inputs went to the wrong wings. It is now built the way vanilla's cars are: nose forward, left side at +X
- **You could see through the cabin.** The fuselage skin and doors now have an inside, the colour of the fabric's back
- **You sat under the plane.** The crew positions are now proper seats at hip height, with a low-seat car pose that fits a Cub's cockpit
- **You only got the passenger seat.** Look at the front seat through the open door to get in as pilot, the back seat for passenger
- **The engine would not start.** The engine's idle and minimum revs were the wrong way round, which the physics refuses
- **Spawning it threw a script error.** The hidden tailwheel had no ruined version, and no proxy for the physics to find

### Fixed After the Second Test

- **The door was far too loud.** Opening and closing it now makes a quiet latch click instead of a car door slam
- **The engine cranked but would not start.** It was too weak to hold its own idle and died straight away. It now has a car engine's torque, idling at 900 rpm
- **You couldn't change gear.** The plane now has neutral and one forward gear, on the car keys: E into gear, Q back to neutral, and no reverse. In neutral the engine idles, the propeller gives no thrust and the parking brake holds the plane. The rudder moved to Z and C to make room
- **It twitched when parked.** Standing on its wheels with the propeller not pulling, it no longer gets the wind and the last of the engine's power, so it sits still
- **The tailwheel sank into the ground.** The visible tailwheel is now fixed to the airframe, with a stiffer spring and a slightly larger physics wheel under it, so the tyre sits on the ground

### Fixed After the Third and Fourth Tests

- **The engine started, then died a moment later.** The plane's damage zones had no health levels, which every vanilla car has. Without them the car code read the engine as ruined once a second and switched it off. (One test plane had also been spawned ruined, with COT's health field at 0.) A refused start now writes the reason to the script log: ruined or out of fuel, with its health and fuel
- **You got out on the wrong side.** A car driver's animation climbs out to the left, and the pilot's seat has to keep it: the game gives the controls and the car HUD only to a driver's seat (trying another took both away). Whoever gets out is now put at the door on the right, and the passenger's seat climbs out to the right anyway
- **The door still sounded awful.** It now has a quiet sound of its own: a light car door at a third of the volume, heard only close by

### Fixed After the Sixth Test

- **The rudder was still on Q and E, and Z and C did nothing.** A saved key profile keeps the keys it has for an input over a mod's new defaults, so the rudder's inputs have new names and Z and C now take hold. While the rudder was on Q/E, every touch of left rudder also shifted down to neutral and cut the propeller, so the plane could not build speed to take off
- **Taxiing was hard.** The tailwheel swings further (35°) and faster, and grips sideways hard enough to keep the plane running straight. The main tyres roll more freely
- **A test readout.** While the engine runs, the pilot's script log gets a line a second: gear, throttle, airspeed, thrust, lift against weight, the wing's angle of attack, pitch, heading, wheels on the ground and steering. `debugLog=0` in `GebsFlight` turns it off

### Fixed After the Seventh Test

The readout showed the plane does lift off, at about 55 km/h, but it took 20 seconds of full power to get there, steered barely at all on the ground and swung round under power.

- **The takeoff run dragged.** The main gear's spring sagged 9.6 of its 10 cm under the plane's weight, so the hubs scraped the ground on every bump. The spring is now stiff enough to settle about 4.5 cm, and the hubs sit further inside the tyres
- **The rudder pedals hardly steered it on the ground, and it swung round under power.** Below 50 km/h the pedals now turn the plane at the rate they ask for, up to 35° a second, and hold it straight when centred, as the steerable tailwheel and toe brakes do for a Cub pilot. Above that the rudder in the slipstream takes over
- **A tap of S yanked it into a stall.** On the keyboard the stick now moves fore and aft at a steadier pace, so a tap is a nudge
- **The flaps were on V, which is third person, and B, which opens COT's map editor.** They are now on X (down) and R (up), which do nothing else in the pilot's seat
- **The readout** now also shows each wheel (on the ground, and how far its spring has settled), the turn rate, and the elevator, trim and flaps

### Fixed After the Eighth Test

It flew, but jolted about in the air. The server log showed it correcting the pilot's plane up to 30 times a second.

- **It lagged and jolted in the air.** The flight forces went straight onto the physics body, which the network's corrections don't replay, and they used the weather's wind, which the server and the pilot's machine each keep their own copy of. The forces now go in through the engine's deterministic vehicle physics, the wind is left out, replayed moves carry their controls and additive rewinds leave the flight state alone
- **The tailwheel was never there.** The physics finds each wheel through a bone named after its slot, and the tailwheel's two halves shared the tailwheel's bone, so the game made no wheels for them and the tail slid along on its hub block. That was the tailwheel sinking into the ground, the rudder that barely steered on the ground, and much of the slow takeoff run. They have bones of their own now
- **A full-lock tailwheel at speed.** Now that it steers, its swing narrows as the speed builds, to 15% of full lock by 65 km/h
