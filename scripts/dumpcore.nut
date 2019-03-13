mega <- 0x20000;
function dump(d, mappernum, increase_cpu, increase_ppu)
{
	local vram = board.vram_mirrorfind == true ? 1 : 0;
	if(mappernum == -1){
		mappernum = board.mappernum;
	}
	memory_new(d, board.cpu_romsize * increase_cpu, board.ppu_romsize * increase_ppu);
	cpu_dump(d, board.cpu_romsize * increase_cpu / board.cpu_banksize, board.cpu_banksize);
	if(board.ppu_ramfind == true){
		if(ppu_ramfind(d) == true){
			nesfile_save(d, mappernum, vram);
			return;
		}
	}
	if(board.ppu_romsize != 0){
		ppu_dump(d, board.ppu_romsize * increase_ppu / board.ppu_banksize, board.ppu_banksize);
	}
	nesfile_save(d, mappernum, vram);
	return;
}
