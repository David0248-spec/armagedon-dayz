// ARMA-GEDON: balans obrazen amunicji (przebicie kamizelek / helmow, obrazenia)
class CfgPatches
{
	class AGA_AmmoBalance
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Weapons_Projectiles","DZ_Sounds_Effects","VBB_Weapons"};
	};
};
class CfgAmmo {
  class Bullet_Base;
  class Bullet_12GaugeSlug : Bullet_Base {
    airFriction = -0.0025;
    unconRefillModifier = 2.5;
  };
  class Bullet_12GaugeRubberSlug : Bullet_12GaugeSlug {
    initSpeed = 380;
    typicalSpeed = 420;
    airFriction = -0.01;
  };
  class Bullet_556x45 : Bullet_Base {
    airFriction = -0.0009;
    unconRefillModifier = 1.5;
    damageBarrel = 250;
    damageBarrelDestroyed = 250;
    class DamageApplied {
      defaultDamageOverride[] = {{1.1,1}};
      class Health {
        damage = 120;
      };
      class Blood {
        damage = 100;
      };
      class Shock {
        damage = 105;
      };
    };
  };
  class Bullet_545x39 : Bullet_Base {
    airFriction = -0.0009;
    unconRefillModifier = 1.5;
    damageBarrel = 215;
    damageBarrelDestroyed = 215;
    class DamageApplied {
      defaultDamageOverride[] = {{1.2,1}};
      class Health {
        damage = 120;
      };
      class Blood {
        damage = 100;
      };
      class Shock {
        damage = 75;
      };
    };
  };
  class Bullet_762x54 : Bullet_Base {
    airFriction = -0.0002;
    unconRefillModifier = 1.5;
    impactBehaviour = 0;
    class DamageApplied {
      defaultDamageOverride[] = {{0.99,1}};
      class Health {
        damage = 170;
      };
      class Blood {
        damage = 100;
      };
      class Shock {
        damage = 105;
      };
    };
    soundSetSuperSonic[] = {"AGA_SuperSonicCrack_SoundSet","AGA_SuperSonicCrackTail_SoundSet"};
  };
  class Bullet_308Win : Bullet_Base {
    airFriction = -0.0002;
    unconRefillModifier = 1.5;
    impactBehaviour = 0;
    class DamageApplied {
      defaultDamageOverride[] = {{1.05,1}};
      class Health {
        damage = 182;
      };
      class Blood {
        damage = 100;
      };
      class Shock {
        damage = 110;
      };
    };
    soundSetSuperSonic[] = {"AGA_SuperSonicCrack_SoundSet","AGA_SuperSonicCrackTail_SoundSet"};
  };
  class Bullet_762x39 : Bullet_Base {
    airFriction = -0.0011;
    unconRefillModifier = 1.5;
    damageBarrel = 500;
    damageBarrelDestroyed = 500;
    class DamageApplied {
      defaultDamageOverride[] = {{1.2,1}};
      class Health {
        damage = 130;
      };
      class Blood {
        damage = 100;
      };
      class Shock {
        damage = 75;
      };
    };
  };
  class Bullet_45ACP : Bullet_Base {
    initSpeed = 290;
    airFriction = -0.0007;
    unconRefillModifier = 4.5;
    class DamageApplied {
      class Health {
        damage = 45;
      };
      class Blood {
        damage = 100;
      };
      class Shock {
        damage = 45;
        damageOverride[] = {{0.9,1}};
      };
    };
  };
  class Bullet_357 : Bullet_Base {
    unconRefillModifier = 4.5;
    class DamageApplied {
      defaultDamageOverride[] = {{0.85,1}};
      class Health {
        damage = 70;
      };
      class Blood {
        damage = 100;
      };
      class Shock {
        damage = 90;
      };
    };
  };
  class Bullet_9x19 : Bullet_Base {
    initSpeed = 380;
    airFriction = -0.0016;
    unconRefillModifier = 4.5;
  };
  class Bullet_380 : Bullet_Base {
    unconRefillModifier = 4.5;
  };
  class Bullet_22 : Bullet_Base {
    unconRefillModifier = 4.5;
  };
  class Bullet_9x39AP : Bullet_Base {
    typicalSpeed = 420;
    initSpeed = 420;
    unconRefillModifier = 1.5;
    class DamageApplied {
      defaultDamageOverride[] = {{0.95,1}};
      class Health {
        damage = 75;
        armorDamage = 2.5;
      };
      class Blood {
        damage = 100;
      };
      class Shock {
        damage = 65;
      };
    };
  };
  class Bullet_9x39 : Bullet_Base {
    typicalSpeed = 380;
    initSpeed = 380;
    unconRefillModifier = 1.5;
    class DamageApplied {
      class Health {
        damage = 75;
      };
      class Blood {
        damage = 100;
      };
      class Shock {
        damage = 65;
        damageOverride[] = {{0.9,1}};
      };
    };
  };
  class Bolt_Base;
  class Bullet_HuntingBolt : Bolt_Base {
    unconRefillModifier = 1.5;
    airFriction = -0.001;
    class DamageApplied {
      defaultDamageOverride[] = {{0.98,0.96},{0.961,0.96},{0.935,0.925},{0.905,0.71},{0.87,0.56}};
      class Health {
        damage = 149;
      };
      class Blood {
        damage = 100;
      };
      class Shock {
        damage = 95;
      };
    };
  };
  class Bullet_ImprovisedBolt_1 : Bolt_Base {
    unconRefillModifier = 1.5;
    airFriction = -0.0074;
    class DamageApplied {
      defaultDamageOverride[] = {{0.9,0.96},{0.78,0.8},{0.61,0.52},{0.5,0.1},{0.4,0}};
      class Health {
        damage = 133;
      };
      class Blood {
        damage = 100;
      };
      class Shock {
        damage = 85;
      };
    };
  };
  class Bullet_ImprovisedBolt_2 : Bolt_Base {
    unconRefillModifier = 1.5;
    airFriction = -0.003;
    class DamageApplied {
      defaultDamageOverride[] = {{0.965,0.96},{0.905,0.81},{0.8,0.65},{0.746,0.6},{0.67,0.36}};
      class Health {
        damage = 141;
      };
      class Blood {
        damage = 100;
      };
      class Shock {
        damage = 85;
      };
    };
  };
  class Bullet_338 : Bullet_Base {
    airFriction = -0.0002;
    initSpeed = 785;
    typicalSpeed = 865;
    unconRefillModifier = 1.0;
    deflecting = 5;
    class DamageApplied {
      defaultDamageOverride[] = {{0.95,1}};
      class Health {
        damage = 260;
      };
      class Blood {
        damage = 125;
      };
      class Shock {
        damage = 185;
      };
    };
    soundSetSuperSonic[] = {"AGA_SuperSonicCrack_SoundSet","AGA_SuperSonicCrackTail_SoundSet"};
  };
};
class CfgSoundSets {
  class AGA_SuperSonicCrack_SoundSet {
    soundShaders[] = {"SuperSonicCrack_Close_SoundShader","sonicCrack_Close_interior_SoundShader"};
    volumeFactor = 0.6;
    volumeCurve = "InverseSquare1Curve";
    spatial = 1;
    doppler = 0;
    loop = 0;
    sound3DProcessingType = "WeaponMediumShotTail3DProcessingType";
    soundObjectsLimit = 1;
  };
  class AGA_SuperSonicCrackTail_SoundSet {
    soundShaders[] = {"SuperSonicCrack_Dist_SoundShader"};
    volumeFactor = 1.2;
    volumeCurve = "InverseSquare1Curve";
    spatial = 1;
    doppler = 0;
    loop = 0;
    sound3DProcessingType = "WeaponMediumShotTail3DProcessingType";
    soundObjectsLimit = 1;
  };
};

