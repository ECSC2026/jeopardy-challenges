# run this from inside volshell: rs("dump_chacha20.py")
# or from the CLI: --script dump_chacha20.py [--script-only]

from volatility3.framework.symbols import intermed, linux

kernel_layer = self.context.layers['layer_name']
kernel = self.context.modules[self.config['kernel']]

super_blocks = kernel.object_from_symbol("super_blocks")

super_blocks_list = super_blocks.to_list(
    symbol_type=f"{kernel.symbol_table_name}!super_block",
    member="s_list"
)

sb_bch = None

for sb in super_blocks_list:
    sb_name = sb.get_type()
    if sb_name == "bcachefs":
        sb_bch = sb

assert sb_bch is not None

bch_uuid = sb_bch.uuid
print(f"Found UUID device: {bch_uuid}")

bch_table_name = intermed.IntermediateSymbolTable.create(
    context=self.context,
    config_path="symbols.bcachefs",
    sub_path="",
    filename="bcachefs",
)

bch_fs = self.context.object(
    object_type=f"{bch_table_name}!bch_fs",
    layer_name="layer_name",
    offset=sb_bch.s_fs_info
)

chacha20_key = kernel_layer.read(bch_fs.chacha20_key.vol.offset, 32)

print(f"chacha20_key: {chacha20_key.hex()}")
