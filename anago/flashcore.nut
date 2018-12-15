mega <- 0x20000;
function loopsize_get(t, trans, image_size, device_size)
{
	local trans_full = 3, trans_top = 1, trans_bottom = 2; //header.h enum transtype
	local loop = {start = 0, end = 0};
	switch(trans){
	case trans_full:{
		local size = device_size < t.maxsize ? device_size : t.maxsize;
		loop.end = size / t.banksize;
		}break;
	case trans_top:
		loop.end = image_size / t.banksize;
		break;
	case trans_bottom:
		loop.start = (t.maxsize - image_size) / t.banksize;
		loop.end = t.maxsize / t.banksize;
		break;
	default:
		loop.start = 0;
		loop.end = 0;
		break;
	}
	return loop;
}
function program(
	d, mapper, 
	cpu_trans, cpu_image_size, cpu_device_size,
	ppu_trans, ppu_image_size, ppu_device_size
)
{
	local trans_empty = 0;
	if(board.mappernum != mapper){
		print("mapper number not connected\n");
		return;
	}
	local cpu_loop = loopsize_get(board.cpu, cpu_trans, cpu_image_size, cpu_device_size);
	local ppu_loop = loopsize_get(board.ppu, ppu_trans, ppu_image_size, ppu_device_size);
	local co_cpu = newthread(cpu_transfer);
	local co_ppu = newthread(ppu_transfer);
	if(board.vram_mirrorfind == true){
		vram_mirrorfind(d);
	}
	initalize(d, board.cpu.banksize, board.ppu.banksize);
	if(cpu_trans != trans_empty){
		cpu_erase(d);
	}
	if(ppu_trans != trans_empty){
		ppu_erase(d);
	}
	erase_wait(d);
	if(cpu_trans != trans_empty){
		//cpu_transfer(d, cpu_loop.start, cpu_loop.end, board.cpu.banksize);
		co_cpu.call(d, cpu_loop.start, cpu_loop.end, board.cpu.banksize);
	}
	if(ppu_trans != trans_empty){
		co_ppu.call(d, ppu_loop.start, ppu_loop.end, board.ppu.banksize);
	}
	program_main(d, co_cpu, co_ppu)
}

