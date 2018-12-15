#ifndef _HARD_ONAJIMI_H_
#define _HARD_ONAJIMI_H_
/*
STROB: CPU/PPU ADDRESS BUS control
L Address reset, address = 0
H Address Enable
*/
enum{
	ADDRESS_RESET = 1,
	ADDRESS_ENABLE = 0
};
/*
D0: CPU/PPU ADDRESS BUS increment
D1: CPU/PPU DATA SHIFT
D2: CPU/PPU DATA WRITE DATA
D3: CPU/PPU DATA DIRECTION
D4: PPU /WR + CPU φ2
D5: PPU /CS
D6: CPU ROM area /CS
D7: CPU /WR
*/
enum BITNUM{
	BITNUM_ADDRESS_INCREMENT = 0,
	BITNUM_DATA_SHIFT_RIGHT,
	BITNUM_DATA_WRITE_DATA,
	BITNUM_DATA_DIRECTION,
	BITNUM_CPU_M2,
	BITNUM_PPU_SELECT,
	BITNUM_CPU_RAMROM_SELECT,
	BITNUM_CPU_RW,
	BITNUM_PPU_RW = BITNUM_CPU_M2
};
/*
D0: CPU/PPU ADDRESS BUS increment
H->L address += 1

D1: CPU/PPU DATA SHIFT (バスの接続が反転している)
L->H D01234567

D2: CPU/PPU DATA WRITE DATA
LSB->MSB 最下位bitから順に。
*/
enum{
	ADDRESS_NOP = 1 << BITNUM_ADDRESS_INCREMENT,
	DATA_SHIFT_NOP = 0 << BITNUM_DATA_SHIFT_RIGHT
};
/*
D3: CPU/PPU DATA DIRECTION
H FC read
L FC write
*/
enum{
	DATA_DIRECTION_WRITE = 0,
	DATA_DIRECTION_READ = 1
};
/*
D4: PPU /WE + CPU M2
H PPU read + CPU bus enable (for MMC5)
L PPU write + CPU bus disable
*/
enum{
	PPU_WRITE__CPU_DISABLE = 0,
	PPU_READ__CPU_ENABLE
};
/*
D5: PPU /RD + PPU A13
H disable
L enable
*/
enum{
	PPU_ENABLE = 0,
	PPU_DISABLE
};
/*
D6: CPU ROM select (~A15)
H RAM IO select, use $0000-$7fff
L ROM select, use $8000-$ffff
*/
enum{
	CPU_ROM_SELECT = 0,
	CPU_RAM_SELECT
};
/*
D7: CPU W/~R
L write
H read
*/
enum{
	CPU_WRITE = 0,
	CPU_READ
};
/*
BUSY: CPU/PPU DATA READ DATA
LSB->MSB 最下位bitから順に。
*/
#endif
