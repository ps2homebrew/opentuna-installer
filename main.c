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
#include "welcome.h"
#include "MANUAL_INST.h"
#include "MANUAL_INST_FAT170.h"
#include "MANUAL_INST_FATS.h"
#include "MANUAL_INST_SLIMS.h"
//*/
#define ASK_MCPORT(mcport) \
display_bmp(640, 448, MCPORT_QUERY); \
readPad(); \
while(1){ if ((new_pad & PAD_L1) || (new_pad & PAD_R1)){ if (new_pad & PAD_L1) {mcport = 0;} else {mcport = 1;} }}

enum ICN
{
	SLIMS = 0,// fat 0x190 and every 0x2?? ROM
	FATS,// 0x110, 0x120, 0x150, 0x160
	FAT170,// 0x170
	PROTOKERNELS, // this corresponds to rom 0x100 and 0x101, parrado won´t make the icons, but i will leavi it here in case someone makes it faster than instatuna
	UNSUPPORTED,
};

int GetIconType(unsigned long int ROMVERSION {
	int icontype;
	if (ROMVERSION >= 0x190) icontype = SLIMS;
	
	else if ( (ROMVERSION < 0x190) && (ROMVERSION >= 0x110) icontype = FATS;
	
	else if (ROMVERSION == 0x170) icontype = FAT170;
	
	else icontype = UNSUPPORTED;
	
	return icontype;
}

//----------------------------------------//
extern u8 opentuna_icn[];
extern int size_opentuna_icn;
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
	(gs_get_max_x() - W) / 2,   //x
	(gs_get_max_y() - H) / 2,   //y
	W,                          //w
	H,                          //h
	data                        //array
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

	sbv_patch_disable_prefix_check();
	SifLoadModule("rom0:SIO2MAN", 0, NULL);
	SifLoadModule("rom0:MCMAN", 0, NULL);
	SifLoadModule("rom0:MCSERV", 0, NULL);
	SifLoadModule("rom0:PADMAN", 0, NULL);

	//Faltaba iniciar la MC (alexparrado)
	mcInit(MC_TYPE_MC);

	setupPad();
	waitAnyPadReady();
}

//write &embed_file to path
// returns:
// -1 fail to open | -2 failed to write | 0 succes
static int write_embed(void *embed_file, const int embed_size, char* folder, char* filename, int mcport)
{
	int fd, ret;
	char target[MAX_PATH];
	sprintf(target, "mc%u:/%s/%s", mcport, folder, filename);
	if ((ret = open(target, O_RDONLY)) < 0)  //if not exist
	{
		if ((fd = open(target, O_CREAT | O_WRONLY | O_TRUNC)) < 0) {
			return -1;
		}
		ret = write(fd, embed_file, embed_size);
		if (ret != embed_size) {
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
	int ret, retorno, mcport = 0;
	static int mc_Type, mc_Free, mc_Format;

	mcGetInfo(mcport, 0, &mc_Type, &mc_Free, &mc_Format);
	mcSync(0, NULL, &ret);

	//If there's no MC, we have an error:
	if (ret != -1){return 1;}

	//If it is not a PS2 MC, we have an error:
	if (mc_Type != 2){return 2;}

	//If there's no free space, we have an error:
	if (mc_Free < 1727){return 3;}

	//If the files exists, we have an error:
	if ( mcport == 0){
	if (file_exists("mc0:/OPENTUNA/icon.icn")) {return 4;}
	if (file_exists("mc0:/OPENTUNA/icon.sys")) {return 4;}
	if (file_exists("mc0:/APPS/icon.sys")) {return 5;}
	if (file_exists("mc0:/APPS/tunacan.icn")) {return 5;}
	if (file_exists("mc0:/APPS/ULE.ELF")) {return 5;}
	if (file_exists("mc0:/APPS/OPNPS2LD.ELF")) {return 5;}
	} else {
	if (file_exists("mc1:/OPENTUNA/icon.icn")) {return 4;}
	if (file_exists("mc1:/OPENTUNA/icon.sys")) {return 4;}
	if (file_exists("mc1:/APPS/icon.sys")) {return 5;}
	if (file_exists("mc1:/APPS/tunacan.icn")) {return 5;}
	if (file_exists("mc1:/APPS/ULE.ELF")) {return 5;}
	if (file_exists("mc1:/APPS/OPNPS2LD.ELF")) {return 5;}
	}
	ret = mcMkDir(mcport, 0, "OPENTUNA");
	mcSync(0, NULL, &ret);
	ret = mcMkDir(mcport, 0, "APPS");
	mcSync(0, NULL, &ret);
	retorno = -12;///to ensure installation quits if none of the hacked icons are written
	       if (icon_variant ==  SLIMS) {
		retorno = write_embed(&opentuna_icn, size_opentuna_icn, "OPENTUNA","icon.icn",mcport);
	} else if (icon_variant ==   FATS) {
		retorno = write_embed(&opentuna_fats, size_opentuna_fats, "OPENTUNA", "icon.icn", mcport);
	} else if (icon_variant == FAT170) {
		retorno = write_embed(&opentuna_fat170, size_opentuna_fat170, "OPENTUNA","icon.icn",mcport);
	}
	if (retorno < 0) {return 6;}
	//<FILES SHARED BY ALL ICONS FROM NOW ON>
	retorno = write_embed(&opentuna_sys, size_opentuna_sys, "OPENTUNA","icon.sys",mcport);
	if (retorno < 0) {return 6;}
	retorno = write_embed(&apps_sys, size_apps_sys, "APPS","icon.sys",mcport);
	if (retorno < 0) {return 6;}
	retorno = write_embed(&apps_icn, size_apps_icn, "APPS","tunacan.icn",mcport);
	if (retorno < 0) {return 6;}
	retorno = write_embed(&ule_elf, size_ule_elf, "APPS","ULE.ELF",mcport);
	if (retorno < 0) {return 6;}
	retorno = write_embed(&opl_elf, size_opl_elf, "APPS","OPNPS2LD.ELF",mcport);
	if (retorno < 0) {return 6;}

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
	if (pad_inited) {
		padPortClose(0,0);
		padPortClose(1,0);
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

int main (int argc, char *argv[])
{
	int fdn, icontype;
	unsigned long int ROM_VERSION;
	char romver[5];
	VMode = NTSC;

	// Loads Needed modules
	InitPS2();

	gs_reset(); // Reset GS
	if((fdn = open("rom0:ROMVER", O_RDONLY)) > 0) // Reading ROMVER
	{
		read(fdn, romver, 4);
		close(fdn);

		if (romver[4] == 'E')
			VMode = PAL;
		romver[4] = '\0';
		strtoul(ROM_VERSION_STR, romver, 4); //convert ROM version to unsigned long int for further use on automatic Install
	}

	if (VMode == PAL)
		gs_init(PAL_640_512_32);
	else
		gs_init(NTSC_640_448_32);

	display_bmp(640, 448, WELCOME);

	waitAnyPadReady();
	pad_inited = 1;
	icontype = GetIconType(ROM_VERSION);
	int iz = 1;
	int menuactual = 101;//101: Initial Menu, 102: Installing (not needed), 103: Error, 104: Done

	display_bmp(640, 448, WELCOME);//Again, just in case of an old japanese console
	while (1) {
		readPad();

		if (((new_pad & PAD_L1) && (menuactual == 101)) || ((new_pad & PAD_R1) && (menuactual == 101))) {
			menuactual = 102;
			if (new_pad & PAD_L1)// manual install
			{
				ASK_MCPORT(mcport)
				display_bmp(640, 448, wait);
				iz = install(mcport);
			} else {// auto install (R1)
				ASK_MCPORT(mcport)
				display_bmp(640, 448, wait);
				iz = install(mcport,icontype);
			}
			if(iz == 0){
				menuactual = 104;
				display_bmp(640, 448, complete);
			}
			else {
				menuactual = 103;
				display_bmp(640, 448, error);
				if(iz == 1){gs_print_bitmap(192, 343, 256, 40, MensajeA);}
				else if(iz == 2){gs_print_bitmap(192, 343, 256, 40, MensajeB);}
				else if(iz == 3){gs_print_bitmap(192, 343, 256, 40, MensajeC);}
				else if(iz == 4){gs_print_bitmap(192, 343, 256, 40, MensajeD);}
				else if(iz == 5){gs_print_bitmap(192, 343, 256, 40, MensajeE);}
				else if(iz == 6){gs_print_bitmap(192, 343, 256, 40, MensajeF);}
			}
		}

		else if ((new_pad & PAD_START) && (menuactual == 103)) {PS2_browser();}
		else if ((new_pad & PAD_START) && (menuactual == 104)) {PS2_browser();}
	}

	return 0;
}
