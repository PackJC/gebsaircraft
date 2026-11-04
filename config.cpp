/*

	gebsaircraft - flyable aircraft for DayZ.

	DayZ has no aircraft. Each plane here is a bare Transport whose script,
	scripts/4_world/entities/vehicles/gebsplane.c, simulates all of it: its
	wheels on their springs, its engine and fuel, and the air - lift and drag on
	every wing panel and tail surface, propeller thrust, torque and slipstream -
	from the stick, rudder, throttle, flaps and trim the pilot works with the
	keys in data/inputs.xml. scripts/5_mission adds its instrument panel to the
	HUD.

	The planes themselves live in data/config.cpp; this file is the mod header.

*/

class CfgPatches {
	class GebsPlanes {
		units[]= {};
		weapons[]= {};
		requiredVersion=0.1;
		requiredAddons[]= {
			"DZ_Data",
			"DZ_Scripts",
			// CarWheel and the crew proxies
			"DZ_Vehicles_Wheeled",
			// the vanilla engine and door sound sets the planes use
			"DZ_Sounds_Effects"
		};
	};
};

class CfgMods {
	class GebsPlanes {
		dir="gebsaircraft";
		picture="gebsaircraft\data\logo.paa";			// picture in expanded description
		logoSmall="gebsaircraft\data\logo.paa";			// icon next to mod name when description is not expanded
		logo="gebsaircraft\data\logo.paa";				// logo below game menu
		logoOver="gebsaircraft\data\logo_hover.paa";	// logo hovered (CfgMods reads logoOver, not logoHover)
		action="https://discord.gg/G8uSGZ8yyf";
		hideName=0;
		hidePicture=0;
		name="Geb's Aircraft";
		overview="Geb's Aircraft: a flyable Super Cub, a Bf 109 fighter and both of them on floats, each in winter and desert camouflage too";	// keep in step with mod.cpp's overview
		author="Geb";
		authorID="0";
		version="0.3.0";
		extra=0;
		type="mod";
		// Without this the engine looks for gebsaircraft/Anims/cfg/skeletons.anim.xml
		// and logs "ANIMATION (E): Can't load ..." on every start; point it at vanilla's.
		skeletonDefinitions="DZ/anims/cfg/skeletons.anim.xml";
		// the flight controls: stick, rudder, throttle, flaps, trim, brakes
		inputs="gebsaircraft/data/inputs.xml";
		dependencies[]= {
			"World",
			"Mission"
		};
		defines[]= {
			"GEBSPLANES"
		};
		class defs {
			class worldScriptModule {
				value="";
				files[]= {
					"gebsaircraft/scripts/4_world"
				};
			};
			class missionScriptModule {
				value="";
				files[]= {
					"gebsaircraft/scripts/5_mission"
				};
			};
		};
	};
};
