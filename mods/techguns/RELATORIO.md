# Relatorio de conversao: techguns

Handler: `generic`

- abertos: 1
- abertos_por_mecanica: 0
- acoes_com_argumento: 1
- armaduras: 56
- armaduras_com_textura: 44
- armas_por_classe: 0
- baus_container: 1
- blocos: 334
- blocos_com_estados: 37
- blocos_tecnicos_ocultos: 1
- caixas: 464
- chamadas_forge: 1931
- classes_com_comportamento: 24
- classes_java: 1026
- colocadores: 9
- conexoes: 1
- depende_de: chisel(opt),ftblib(opt),patchouli(opt)
- drops: 61
- drops_por_codigo: 1
- espadas_ferramentas: 5
- eventos_forge: 39
- ganchos_hud: 5
- gui_vanilla_substituida: 0
- itens_com_luz: 0
- itens_com_mecanica: 3
- maquinas_ini: 0
- mod_id: techguns
- mod_nome: Techguns
- mod_versao: 1.12.2-2.0.2.0
- oredict_nomes: 59
- receitas: 306
- receitas_ini: 0
- segundos: 3.9
- sons: 399
- sons_ogg: 405
- srg_com_nome: 156/1216
- telas_gui: 45
- texturas: 284
- texturas_gui: 42
- vanilla_embutida: 1

## Lacuna: Forge: aproximado (260 tipos)
- `mc/item/ItemStack.isEmpty [func_190926_b]` x105
- `mc/item/ItemStack.getItem [func_77973_b]` x82
- `mc/item/ItemStack.EMPTY [field_190927_a]` x62
- `mc/item/ItemStack.<init>` x58
- `mc/item/ItemStack.copy [func_77946_l]` x45
- `mc/block/state/IBlockState.withProperty [func_177226_a]` x42
- `mc/block/state/IBlockState.getBlock [func_177230_c]` x41
- `mc/item/ItemStack.getItemDamage [func_77952_i]` x39
- `mc/item/ItemStack.getCount [func_190916_E]` x38
- `mc/block/state/IBlockState.getValue [func_177229_b]` x37
- `mc/util/math/AxisAlignedBB.<init>` x26
- `mc/item/ItemStack.func_190920_e` x24
- `mc/item/ItemStack.getTagCompound [func_77978_p]` x18
- `mc/item/ItemStack.setItemDamage [func_77964_b]` x16
- `mc/block/Block.getDefaultState [func_176223_P]` x16
- `mc/block/state/BlockStateContainer.func_177621_b` x14
- `mc/item/ItemStack.setTagCompound [func_77982_d]` x13
- `mc/item/ItemStack.func_77958_k` x11
- `mc/block/state/BlockStateContainer$Builder.build` x10
- `mc/block/state/BlockStateContainer$Builder.add` x10
- `mc/block/state/BlockStateContainer$Builder.<init>` x10
- `forge/oredict/OreDictionary.itemMatches` x10
- `mc/block/state/BlockFaceShape.UNDEFINED` x9
- `mc/block/properties/PropertyEnum.func_177709_a` x9
- `mc/item/ItemStack.getMetadata [func_77960_j]` x9
- `mc/block/BlockHorizontal.field_185512_D` x9
- `mc/block/Block.field_185505_j` x7
- `mc/block/material/Material.Material.IRON [field_151573_f]` x7
- `mc/block/Block.getRegistryName` x7
- `mc/item/ItemStack.func_77977_a` x7
- `mc/block/state/BlockFaceShape.SOLID` x6
- `mc/block/properties/PropertyBool.func_177716_a` x6
- `mc/util/BlockRenderLayer.SOLID` x6
- `mc/item/ItemArmor.field_77881_a` x6
- `mc/item/ItemStack.shrink [func_190918_g]` x6
- `forge/oredict/OreDictionary.getOres` x6
- `mc/block/Block.getStateFromMeta [func_176203_a]` x6
- `mc/block/state/BlockStateContainer.<init>` x5
- `mc/util/BlockRenderLayer.CUTOUT` x5
- `mc/block/state/IBlockState.getMaterial [func_185904_a]` x5

## Lacuna: Forge: sem equivalente (786 tipos)
- `mc/world/World.isRemote [field_72995_K]` x82
- `mc/world/World.getBlockState [func_180495_p]` x36
- `mc/entity/player/EntityPlayer.field_70170_p` x35
- `forge/fml/common/network/simpleimpl/SimpleNetworkWrapper.sendToAllAround` x33
- `mc/world/World.setBlockState [func_180501_a]` x29
- `mc/world/World.getTileEntity [func_175625_s]` x29
- `mc/world/World.spawnEntity [func_72838_d]` x26
- `mc/inventory/EntityEquipmentSlot.LEGS` x24
- `mc/world/World.rand [field_73012_v]` x23
- `mc/inventory/EntityEquipmentSlot.FEET` x23
- `mc/inventory/EntityEquipmentSlot.HEAD` x23
- `mc/inventory/EntityEquipmentSlot.CHEST` x23
- `mc/entity/ai/attributes/IAttributeInstance.func_111128_a` x23
- `forge/client/model/ModelLoader.setCustomModelResourceLocation` x22
- `mc/entity/SharedMonsterAttributes.field_111263_d` x22
- `mc/entity/SharedMonsterAttributes.field_111267_a` x22
- `forge/fml/common/network/simpleimpl/IMessage.<extends>` x22
- `mc/entity/SharedMonsterAttributes.field_111265_b` x21
- `mc/entity/player/EntityPlayer.field_70161_v` x20
- `mc/entity/player/EntityPlayer.field_70165_t` x20
- `mc/entity/SharedMonsterAttributes.field_111264_e` x20
- `mc/entity/player/EntityPlayer.field_70163_u` x19
- `mc/inventory/EntityEquipmentSlot.MAINHAND` x19
- `mc/world/World.provider [field_73011_w]` x19
- `mc/world/WorldProvider.getDimension` x19
- `mc/entity/player/EntityPlayer.getHeldItem [func_184586_b]` x18
- `mc/world/World.setBlockToAir [func_175698_g]` x17
- `mc/world/World.func_184134_a` x16
- `mc/world/World.playSound [func_184148_a]` x16
- `mc/entity/Entity.field_70161_v` x16
- `mc/entity/Entity.field_70165_t` x16
- `mc/inventory/Slot.func_190901_a` x16
- `forge/items/CapabilityItemHandler.ITEM_HANDLER_CAPABILITY` x16
- `mc/entity/Entity.field_70163_u` x15
- `forge/fml/common/FMLCommonHandler.instance` x15
- `mc/world/World.func_73045_a` x14
- `mc/inventory/Slot.func_75211_c` x14
- `mc/inventory/Slot.func_75218_e` x14
- `mc/inventory/Slot.func_75215_d` x14
- `mc/inventory/Slot.func_75220_a` x14

## Lacuna: HUD por evento do Forge (sem codigo no Blockland) (5 tipos)
- `TabRegistry` x1
- `TGGuiEvents` x1
- `RadRegenerationPotion` x1
- `RadResistancePotion` x1
- `RadiationPotion` x1

## Lacuna: dependencia opcional ausente (2 tipos)
- `ftblib` x1
- `patchouli` x1

## Lacuna: desenhado por codigo (TESR, sem modelo JSON) (10 tipos)
- `RenderBlastfurnace` x1
- `RenderChargingStation` x1
- `RenderDungeonGenerator` x1
- `RenderDungeonScanner` x1
- `RenderFabricator` x1
- `RenderGrinder` x1
- `RenderMachine` x1
- `RenderOreDrill` x1
- `RenderReactionChamber` x1
- `RenderTurret` x1

## Lacuna: drop por codigo nao traduzido: codigo proprio (1 tipos)
- `door3x3` x1

## Lacuna: drop por codigo nao traduzido: estado (1 tipos)
- `bunkerdoor` x1

## Lacuna: entidade propria (sem equivalente) (30 tipos)
- `AbstractBeamProjectile` x1
- `AdvancedBulletProjectile` x1
- `AlienBlasterProjectile` x1
- `BioGunProjectile` x1
- `BlasterProjectile` x1
- `ChainsawProjectile` x1
- `CyberdemonBlasterProjectile` x1
- `DeatomizerProjectile` x1
- `EntityRadiation` x1
- `FlamethrowerProjectile` x1
- `FlyingGibs` x1
- `FragGrenadeProjectile` x1
- `GaussProjectile` x1
- `GenericProjectile` x1
- `GenericProjectileExplosive` x1
- `GenericProjectileIncendiary` x1
- `Grenade40mmProjectile` x1
- `GrenadeProjectile` x1
- `GuidedMissileProjectile` x1
- `GuidedMissileProjectileHV` x1
- `LaserProjectile` x1
- `NDRProjectile` x1
- `PowerHammerProjectile` x1
- `RocketProjectile` x1
- `RocketProjectileHV` x1
- `RocketProjectileNuke` x1
- `SonicShotgunProjectile` x1
- `StoneBulletProjectile` x1
- `TFGProjectile` x1
- `TeslaProjectile` x1

## Lacuna: mod sem perfil em db/mods (so analise automatica) (1 tipos)
- `techguns` x1

## Lacuna: modelo OBJ aproximado por caixa (4 tipos)
- `gaussrifle` x1
- `grenade40mm` x1
- `grenadelauncher` x1
- `grenadelauncher_1` x1

## Lacuna: modelo OBJ sem textura (3 tipos)
- `techguns:npc/helicopter0.obj` x1
- `techguns:npc/helicopter1.obj` x1
- `techguns:npc/helicopter2.obj` x1

## Lacuna: modelo de item sem elementos (1 tipos)
- `basicmachine` x1

## Lacuna: modelo sem elementos (6 tipos)
- `basicmachine` x1
- `door3x3` x1
- `fluid` x1
- `helicopter0` x1
- `helicopter1` x1
- `helicopter2` x1

## Lacuna: modelo sem textura resolvida (6 tipos)
- `basicmachine` x1
- `door3x3` x1
- `fluid` x1
- `helicopter0` x1
- `helicopter1` x1
- `helicopter2` x1

## Lacuna: parent de modelo desconhecido (2 tipos)
- `block/thin_block` x2
- `builtin/entity` x1

## Lacuna: receita de tipo desconhecido (3 tipos)
- `ammo_change_crafting -> AmmoSwitchRecipeFactory` x42
- `miningtool_upgrade -> MiningToolUpgradeHeadRecipeFactory` x5
- `ammo_sum_recipe -> AmmoSumRecipeFactory` x1

## Lacuna: rotacao de UV de face ignorada (3 tipos)
- `lamp0` x1
- `slimyladder` x1
- `tg_spawner` x1

## Lacuna: textura so por nome (vanilla/ausente) (31 tipos)
- `techguns:textures/entity/launchergrenade` x1
- `techguns:items/dummy` x1
- `techguns:items/as50magazine` x1
- `techguns:items/as50magazineempty` x1
- `techguns:items/as50magazine_explosive` x1
- `techguns:items/as50magazine_incendiary` x1
- `techguns:items/assaultriflemagazine` x1
- `techguns:items/assaultriflemagazineempty` x1
- `techguns:items/assaultriflemagazine_incendiary` x1
- `techguns:items/door3x3` x1
- `techguns:items/lmgmagazine` x1
- `techguns:items/lmgmagazineempty` x1
- `techguns:items/lmgmagazine_incendiary` x1
- `techguns:items/minigundrumepmty` x1
- `techguns:items/plasticbag` x1
- `techguns:items/rocket` x1
- `techguns:items/rocket_high_velocity` x1
- `techguns:items/rocket_nuke` x1
- `techguns:items/steam_boots` x1
- `techguns:items/steam_chestplate` x1
- `techguns:items/steam_helmet` x1
- `techguns:items/steam_leggings` x1
- `techguns:items/steelbarrel` x1
- `techguns:items/t3_power_boots` x1
- `techguns:items/t3_power_chestplate` x1
- `techguns:items/t3_power_helmet` x1
- `techguns:items/t3_power_leggings` x1
- `techguns:items/t4_power_boots` x1
- `techguns:items/t4_power_chestplate` x1
- `techguns:items/t4_power_helmet` x1
- `techguns:items/t4_power_leggings` x1
