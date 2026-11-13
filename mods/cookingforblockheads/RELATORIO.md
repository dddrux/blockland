# Relatorio de conversao: cookingforblockheads

Handler: `generic`

- abertos_por_mecanica: 0
- acoes_com_argumento: 0
- baus_container: 0
- blocos: 26
- blocos_com_estados: 14
- blocos_tecnicos_ocultos: 0
- caixas: 166
- chamadas_forge: 579
- classes_com_comportamento: 14
- classes_java: 227
- colocadores: 0
- depende_de: crafttweaker(opt),mousetweaks(opt)
- drops: 0
- drops_por_codigo: 0
- eventos_forge: 10
- ganchos_hud: 0
- gui_vanilla_substituida: 0
- itens_com_luz: 0
- itens_com_mecanica: 16
- maquinas_ini: 0
- mod_id: cookingforblockheads
- mod_nome: Cooking for Blockheads
- mod_versao: 6.5.0
- oredict_nomes: 0
- receitas: 19
- receitas_ini: 0
- segundos: 0.7
- sons: 6
- sons_ogg: 6
- srg_com_nome: 94/324
- telas_gui: 6
- texturas: 36
- texturas_gui: 4
- vanilla_embutida: 4

## Lacuna: Forge: aproximado (83 tipos)
- `mc/item/ItemStack.isEmpty [func_190926_b]` x42
- `mc/block/state/IBlockState.getBlock [func_177230_c]` x28
- `mc/item/ItemStack.EMPTY [field_190927_a]` x28
- `mc/item/ItemStack.getItem [func_77973_b]` x24
- `mc/item/ItemStack.<init>` x19
- `mc/block/state/IBlockState.getValue [func_177229_b]` x17
- `mc/block/state/IBlockState.withProperty [func_177226_a]` x14
- `mc/item/ItemStack.copy [func_77946_l]` x12
- `mc/util/math/AxisAlignedBB.<init>` x11
- `mc/block/state/BlockStateContainer.<init>` x11
- `mc/item/ItemStack.shrink [func_190918_g]` x11
- `mc/item/ItemStack.getCount [func_190916_E]` x10
- `mc/item/ItemStack.getItemDamage [func_77952_i]` x7
- `mc/item/ItemStack.func_77979_a` x7
- `forge/oredict/DyeUtils.colorFromStack` x5
- `forge/oredict/DyeUtils.isDye` x5
- `mc/block/material/Material.Material.ROCK [field_151576_e]` x5
- `mc/item/EnumDyeColor.func_176767_b` x5
- `mc/item/EnumDyeColor.func_176766_a` x5
- `mc/item/EnumDyeColor.WHITE` x5
- `mc/block/material/Material.Material.WOOD [field_151575_d]` x4
- `mc/block/properties/PropertyBool.func_177716_a` x4
- `mc/item/crafting/IRecipe.func_77571_b` x4
- `mc/item/Item.<extends>` x4
- `mc/item/Item.addInformation [func_77624_a]` x4
- `mc/item/Item.<init>` x4
- `mc/block/properties/PropertyEnum.func_177709_a` x3
- `mc/util/math/AxisAlignedBB.expand [func_72321_a]` x3
- `mc/util/BlockRenderLayer.CUTOUT` x3
- `mc/block/material/Material.Material.IRON [field_151573_f]` x3
- `mc/item/crafting/IRecipe.func_192400_c` x3
- `mc/item/Item.getRegistryName` x3
- `mc/block/Block.getDefaultState [func_176223_P]` x2
- `mc/item/ItemStack.getMaxStackSize [func_77976_d]` x2
- `mc/block/state/IBlockState.func_177228_b` x2
- `forge/client/model/IModel.bake` x2
- `mc/item/EnumDyeColor.func_193350_e` x2
- `mc/item/EnumDyeColor.func_176765_a` x2
- `mc/block/Block.getActualState [func_176221_a]` x2
- `mc/item/crafting/Ingredient.apply` x2

## Lacuna: Forge: sem equivalente (217 tipos)
- `mc/world/World.getBlockState [func_180495_p]` x23
- `mc/world/World.getTileEntity [func_175625_s]` x16
- `mc/tileentity/TileEntity.getCapability` x16
- `mc/tileentity/TileEntity.writeToNBT [func_189515_b]` x15
- `mc/entity/player/EntityPlayer.getHeldItem [func_184586_b]` x14
- `mc/tileentity/TileEntity.hasCapability` x13
- `mc/entity/player/EntityPlayer.inventory [field_71071_by]` x12
- `mc/world/World.isRemote [field_72995_K]` x12
- `mc/tileentity/TileEntity.<extends>` x12
- `mc/tileentity/TileEntity.<init>` x12
- `mc/entity/player/EntityPlayer.capabilities [field_71075_bZ]` x11
- `mc/entity/player/PlayerCapabilities.field_75098_d` x11
- `mc/tileentity/TileEntity.readFromNBT [func_145839_a]` x11
- `mc/network/play/server/SPacketUpdateTileEntity.<init>` x11
- `mc/network/play/server/SPacketUpdateTileEntity.func_148857_g` x11
- `mc/tileentity/TileEntity.onDataPacket` x11
- `mc/client/renderer/tileentity/TileEntitySpecialRenderer.<extends>` x10
- `mc/client/renderer/tileentity/TileEntitySpecialRenderer.<init>` x10
- `mc/world/World.markAndNotifyBlock` x10
- `mc/world/World.func_175726_f` x10
- `forge/items/IItemHandler.getSlots` x9
- `forge/items/SlotItemHandler.<init>` x9
- `mc/world/IBlockAccess.getTileEntity [func_175625_s]` x8
- `forge/items/CapabilityItemHandler.ITEM_HANDLER_CAPABILITY` x8
- `mc/inventory/Slot.func_75211_c` x8
- `forge/items/ItemStackHandler.<init>` x8
- `mc/entity/player/EntityPlayer.openGui` x7
- `mc/inventory/Slot.func_75216_d` x7
- `mc/inventory/Slot.<init>` x7
- `forge/items/ItemStackHandler.<extends>` x7
- `forge/items/ItemStackHandler.serializeNBT` x7
- `forge/items/ItemStackHandler.deserializeNBT` x7
- `mc/entity/player/InventoryPlayer.func_70441_a` x6
- `mc/entity/player/EntityPlayer.func_184611_a` x6
- `forge/items/ItemStackHandler.getStackInSlot` x6
- `mc/client/gui/inventory/GuiContainer.<extends>` x6
- `mc/client/gui/inventory/GuiContainer.drawScreen [func_73863_a]` x6
- `mc/client/gui/inventory/GuiContainer.<init>` x6
- `mc/inventory/Container.<extends>` x6
- `mc/inventory/Slot.func_75218_e` x6

## Lacuna: dependencia opcional ausente (2 tipos)
- `crafttweaker` x1
- `mousetweaks` x1

## Lacuna: desenhado por codigo (TESR, sem modelo JSON) (12 tipos)
- `CabinetRenderer` x1
- `CookingTableRenderer` x1
- `CounterRenderer` x1
- `CowJarRenderer` x1
- `FridgeRenderer` x1
- `FruitBasketRenderer` x1
- `MilkJarRenderer` x1
- `OvenRenderer` x1
- `SinkRenderer` x1
- `SpiceRackRenderer` x1
- `ToasterRenderer` x1
- `ToolRackRenderer` x1

## Lacuna: mod sem perfil em db/mods (so analise automatica) (1 tipos)
- `cookingforblockheads` x1

## Lacuna: modelo sem elementos (1 tipos)
- `fridge_invisible` x1

## Lacuna: modelo sem textura resolvida (1 tipos)
- `fridge_invisible` x1

## Lacuna: rotacao de UV de face ignorada (3 tipos)
- `corner` x1
- `cutting_board` x1
- `tool_rack` x1
