//#define __DEBUG_PRINTF__

/*------------------------------------------------------------*/
#include "main.h"
#include "complete.h"
#include "error.h"
//#include "inst.h"
#include "wait.h"
#include "MensajeA.h"
#include "MensajeB.h"
#include "MensajeC.h"
#include "MensajeD.h"
#include "MensajeE.h"
#include "MensajeF.h"
#include "NON_COMPATIBLE.h"
#include "INST_SLOT_2.h"
#include "INST_SLOT_1.h"

enum ICN
{
	SLIMS = 0,	  // fat 0x190 and every 0x2?? ROM
	FATS,		  // 0x110, 0x120, 0x150, 0x160
	FAT170,		  // 0x170
	PROTOKERNELS, // this corresponds to rom 0x100 and 0x101, parrado won´t make the icons, but i will leavi it here in case someone makes it faster than instatuna
	UNSUPPORTED,
};

enum STATE
{
	STATE_MC0,
	STATE_MC1,
	STATE_INSTALL,
	STATE_FINISH,
	STATE_ERROR,
	STATE_BROWSER,
	STATE_UNSUPPORTED,
};

int GetIconType(unsigned long int ROMVERSION)
{
	int icontype = UNSUPPORTED;

	if (ROMVERSION >= 0x190)
		icontype = SLIMS;

	if ((ROMVERSION < 0x190) && (ROMVERSION >= 0x110))
		icontype = FATS;

	if (ROMVERSION == 0x170)
		icontype = FAT170;

	return icontype;
}

//----------------------------------------//
extern u8 opentuna_slims[];
extern int size_opentuna_slims;
//----------------------------------------//
extern u8 opentuna_fats[];
extern int size_opentuna_fats;
//----------------------------------------//
extern u8 opentuna_fat170[];
extern int size_opentuna_fat170;
//----------------------------------------//
extern u8 opentuna_sys[];
extern int size_opentuna_sys;
//----------------------------------------//
extern u8 opl_elf[];
extern int size_opl_elf;
//----------------------------------------//
extern u8 ule_elf[];
extern int size_ule_elf;
//----------------------------------------//
extern u8 apps_icn[];
extern int size_apps_icn;
//----------------------------------------//
extern u8 apps_sys[];
extern int size_apps_sys;

extern unsigned char SIO2MAN_irx[];
extern unsigned int size_SIO2MAN_irx;

extern unsigned char PADMAN_irx[];
extern unsigned int size_PADMAN_irx;

extern unsigned char MCMAN_irx[];
extern unsigned int size_MCMAN_irx;

extern unsigned char MCSERV_irx[];
extern unsigned int size_MCSERV_irx;

static int pad_inited = 0;

//--------------------------------------------------------------
static int file_exists(char *filepath)
{
	int fdn;

	fdn = open(filepath, O_RDONLY);
	if (fdn < 0)
		return 0;

	close(fdn);

	return 1;
}

//--------------------------------------------------------------
static void Reset_IOP(void)
{
	//parrado
	SifInitRpc(0);
	while (!SifIopReset("", 0))
	{
	};
	while (!SifIopSync())
	{
	};
	SifInitRpc(0);
}
//=============================================================
static void display_bmp(u16 W, u16 H, u32 *data)
{
	gs_print_bitmap(
		(gs_get_max_x() - W) / 2, //x
		(gs_get_max_y() - H) / 2, //y
		W,						  //w
		H,						  //h
		data					  //array
	);

#ifdef __DEBUG_PRINTF__
	printf("array displayed\n");
#endif
}
//=============================================================

static void InitPS2(void)
{
	Reset_IOP();
	SifInitIopHeap();
	SifLoadFileInit();
	fioInit();

	sbv_patch_enable_lmb();
	sbv_patch_disable_prefix_check();
	SifExecModuleBuffer(SIO2MAN_irx, size_SIO2MAN_irx, 0, NULL, NULL);
	SifExecModuleBuffer(PADMAN_irx, size_PADMAN_irx, 0, NULL, NULL);
	SifExecModuleBuffer(MCMAN_irx, size_MCMAN_irx, 0, NULL, NULL);
	SifExecModuleBuffer(MCSERV_irx, size_MCSERV_irx, 0, NULL, NULL);
	mcInit(MC_TYPE_XMC);
	PadInitPads();
	

}

//write &embed_file to path
// returns:
// -1 fail to open | -2 failed to write | 0 succes
static int write_embed(void *embed_file, const int embed_size, char *folder, char *filename, int mcport)
{
	int fd, ret;
	char target[MAX_PATH];
	sprintf(target, "mc%u:/%s/%s", mcport, folder, filename);
	if ((ret = open(target, O_RDONLY)) < 0) //if not exist
	{
		if ((fd = open(target, O_CREAT | O_WRONLY | O_TRUNC)) < 0)
		{
			return -1;
		}
		ret = write(fd, embed_file, embed_size);
		if (ret != embed_size)
		{
			return -2;
		}
		close(fd);
	}

#ifdef __DEBUG_PRINTF__
	printf("embed file written: %s\n", target);
#endif
	return 0;
}
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//return 0 = ok, return 1 = error
static int install(int mcport, int icon_variant)
{
	int ret, retorno;
	static int mc_Type, mc_Free, mc_Format;

	mcGetInfo(mcport, 0, &mc_Type, &mc_Free, &mc_Format);
	mcSync(0, NULL, &ret);	

	//If there's no MC, we have an error:
	if (ret != -1)
	{
		return 1;
	}

	//If it is not a PS2 MC, we have an error:
	if (mc_Type != sceMcTypePS2)
	{
		return 2;
	}

	//If there's no free space, we have an error:
	if (mc_Free < 1727)
	{
		return 3;
	}

	//If the files exists, we have an error:
	if (mcport == 0)
	{
		if (file_exists("mc0:/OPENTUNA/icon.icn"))
		{
			return 4;
		}
		if (file_exists("mc0:/OPENTUNA/icon.sys"))
		{
			return 4;
		}
		if (file_exists("mc0:/APPS/icon.sys"))
		{
			return 5;
		}
		if (file_exists("mc0:/APPS/tunacan.icn"))
		{
			return 5;
		}
		if (file_exists("mc0:/APPS/ULE.ELF"))
		{
			return 5;
		}
		if (file_exists("mc0:/APPS/OPNPS2LD.ELF"))
		{
			return 5;
		}
	}
	else
	{
		if (file_exists("mc1:/OPENTUNA/icon.icn"))
		{
			return 4;
		}
		if (file_exists("mc1:/OPENTUNA/icon.sys"))
		{
			return 4;
		}
		if (file_exists("mc1:/APPS/icon.sys"))
		{
			return 5;
		}
		if (file_exists("mc1:/APPS/tunacan.icn"))
		{
			return 5;
		}
		if (file_exists("mc1:/APPS/ULE.ELF"))
		{
			return 5;
		}
		if (file_exists("mc1:/APPS/OPNPS2LD.ELF"))
		{
			return 5;
		}
	}
	ret = mcMkDir(mcport, 0, "OPENTUNA");
	mcSync(0, NULL, &ret);
	ret = mcMkDir(mcport, 0, "APPS");
	mcSync(0, NULL, &ret);
	retorno = -12; ///to ensure installation quits if none of the hacked icons are written
	if (icon_variant == SLIMS)
	{
		retorno = write_embed(&opentuna_slims, size_opentuna_slims, "OPENTUNA", "icon.icn", mcport);
	}
	else if (icon_variant == FATS)
	{
		retorno = write_embed(&opentuna_fats, size_opentuna_fats, "OPENTUNA", "icon.icn", mcport);
	}
	else if (icon_variant == FAT170)
	{
		retorno = write_embed(&opentuna_fat170, size_opentuna_fat170, "OPENTUNA", "icon.icn", mcport);
	}
	if (retorno < 0)
	{
		return 6;
	}
	//<FILES SHARED BY ALL ICONS FROM NOW ON>
	retorno = write_embed(&opentuna_sys, size_opentuna_sys, "OPENTUNA", "icon.sys", mcport);
	if (retorno < 0)
	{
		return 6;
	}
	retorno = write_embed(&apps_sys, size_apps_sys, "APPS", "icon.sys", mcport);
	if (retorno < 0)
	{
		return 6;
	}
	retorno = write_embed(&apps_icn, size_apps_icn, "APPS", "tunacan.icn", mcport);
	if (retorno < 0)
	{
		return 6;
	}
	retorno = write_embed(&ule_elf, size_ule_elf, "APPS", "ULE.ELF", mcport);
	if (retorno < 0)
	{
		return 6;
	}
	retorno = write_embed(&opl_elf, size_opl_elf, "APPS", "OPNPS2LD.ELF", mcport);
	if (retorno < 0)
	{
		return 6;
	}

#ifdef __DEBUG_PRINTF__
	printf("installation finished\n");
#endif

	static sceMcTblGetDir mcDirAAA[64] __attribute__((aligned(64)));
	static sceMcStDateTime maximahora; //Maxium Timestamp, for the ones who does not speak Spanish

	maximahora.Resv2 = 0;
	maximahora.Sec = 59;
	maximahora.Min = 59;
	maximahora.Hour = 23;
	maximahora.Day = 31;
	maximahora.Month = 12;
	maximahora.Year = 2099;
	mcDirAAA->_Modify = maximahora;
	mcDirAAA->_Create = maximahora;
	mcSetFileInfo(mcport, 0, "OPENTUNA", mcDirAAA, 0x02);
	mcSync(0, NULL, &ret);

#ifdef __DEBUG_PRINTF__
	printf("timestamp changed\n");
#endif

	return 0;
}
//--------------------------------------------------------------

static void CleanUp(void) //trimmed from FMCB
{
	if (pad_inited)
	{
		padPortClose(0, 0);
		padPortClose(1, 0);
		padEnd();
	}

	Reset_IOP();

	// Reloads common modules
	SifLoadFileInit();
	sbv_patch_enable_lmb();
	sbv_patch_disable_prefix_check();

	SifLoadModule("rom0:SIO2MAN", 0, 0);
	SifLoadModule("rom0:CDVDFSV", 0, 0);
	SifLoadModule("rom0:CDVDMAN", 0, 0);
	SifLoadModule("rom0:MCMAN", 0, 0);
	SifLoadModule("rom0:MCSERV", 0, 0);
	SifLoadModule("rom0:PADMAN", 0, 0);

	fioExit();
	SifExitIopHeap();
	SifLoadFileExit();
	SifExitRpc();
	SifExitCmd();

	FlushCache(0);
	FlushCache(2);

	// clear the screen
	gs_set_fill_color(0, 0, 0);
	gs_fill_rect(0, 0, gs_get_max_x(), gs_get_max_y());
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//close program and go to browser
static void PS2_browser(void)
{
	CleanUp();
	LoadExecPS2("rom0:OSDSYS", 0, NULL);
}

void error_message(int iz)
{
	display_bmp(640, 448, error);
	switch (iz)
	{
	case 1:
		gs_print_bitmap(192, 343, 256, 40, MensajeA);
		break;
	case 2:
		gs_print_bitmap(192, 343, 256, 40, MensajeB);
		break;
	case 3:
		gs_print_bitmap(192, 343, 256, 40, MensajeC);
		break;
	case 4:
		gs_print_bitmap(192, 343, 256, 40, MensajeD);
		break;
	case 5:
		gs_print_bitmap(192, 343, 256, 40, MensajeE);
		break;

	case 6:
		gs_print_bitmap(192, 343, 256, 40, MensajeF);
		break;
	default:
		break;
	}
}

int wait_key(int key)
{
	int new_pad;
	while (1)
	{
		new_pad = ReadCombinedPadStatus();
		if (new_pad & key)
			return new_pad;
	}
}

int main(int argc, char *argv[])
{
	int fdn, icontype;
	unsigned long int ROM_VERSION;
	char romver[5];
	VMode = NTSC;
	int mcport, state;
	int key;

	// Loads Needed modules
	InitPS2();

	gs_reset();									   // Reset GS
	if ((fdn = open("rom0:ROMVER", O_RDONLY)) > 0) // Reading ROMVER
	{
		read(fdn, romver, 4);
		close(fdn);

		if (romver[4] == 'E')
			VMode = PAL;
		romver[4] = '\0';
		ROM_VERSION = strtoul(romver, NULL, 16); //convert ROM version to unsigned long int for further use on automatic Install
	}

	if (VMode == PAL)
		gs_init(PAL_640_512_32);
	else
		gs_init(NTSC_640_448_32);

	pad_inited = 1;
	icontype = GetIconType(ROM_VERSION);
	int iz = 0;

	if (icontype != UNSUPPORTED)
		state = STATE_MC0;
	else
		state = STATE_UNSUPPORTED;

	while (1)
	{

		switch (state)
		{
		case STATE_MC0:
			mcport = 0;
			display_bmp(640, 448, INST_SLOT_1);

			key = wait_key(-1);

			if ((key & PAD_CROSS) || (key & PAD_CIRCLE))
			{
				state = STATE_INSTALL;
				break;
			}

			if ((key & PAD_SELECT) || (key & PAD_L1))
			{
				state = STATE_MC1;
				break;
			}

			state = STATE_BROWSER;

			break;

		case STATE_MC1:
			mcport = 1;
			display_bmp(640, 448, INST_SLOT_2); //Again, just in case of an old japanese console

			key = wait_key(-1);

			if ((key & PAD_CROSS) || (key & PAD_CIRCLE))
			{
				state = STATE_INSTALL;
				break;
			}

			if ((key & PAD_SELECT) || (key & PAD_L1))
			{
				state = STATE_MC0;
				break;
			}

			state = STATE_BROWSER;

			break;

		case STATE_UNSUPPORTED:

			display_bmp(640, 448, NON_COMPATIBLE); //Again, just in case of an old japanese console
			key = wait_key(PAD_START);
			state = STATE_BROWSER;

			break;

		case STATE_INSTALL:
			display_bmp(640, 448, wait);
			iz = install(mcport, icontype);

			if (iz)
				state = STATE_ERROR;
			else
				state = STATE_FINISH;

			break;

		case STATE_FINISH:
			display_bmp(640, 448, complete);
			key = wait_key(PAD_START);
			state = STATE_BROWSER;

			break;
		case STATE_ERROR:
			error_message(iz);
			key = wait_key(PAD_START);

			state = STATE_BROWSER;

		case STATE_BROWSER:
			PS2_browser();
			break;

		default:
			break;
		}
	}

	return 0;
}
