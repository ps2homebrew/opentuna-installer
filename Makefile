EE_BIN = Installer.elf
EE_BIN_PACKED = OpenTuna_Installer.elf
EE_BIN_STRIPPED = stripped.elf
EE_OBJS = main.o gs.o pad.o  gs_asm.o ps2_asm.o dma_asm.o
EE_OBJS += opentuna_icn.o opentuna_sys.o opl_elf.o ule_elf.o apps_icn.o apps_sys.o OpenTuna_FAT-170.o OpenTuna_FAT-110-120-150-160.o
EE_SRC =   opentuna_icn.s opentuna_sys.s opl_elf.s ule_elf.s apps_icn.s apps_sys.s OpenTuna_FAT-170.s OpenTuna_FAT-110-120-150-160.s
EE_LIBS = -ldebug -lcdvd -lpatches -lpad -lmc

all:
	$(MAKE) $(EE_BIN_PACKED)

opentuna_icn.s:
	bin2s INSTALL/OPENTUNA/icon.icn opentuna_icn.s opentuna_icn

opentuna_sys.s:
	bin2s INSTALL/OPENTUNA/icon.sys opentuna_sys.s opentuna_sys

opl_elf.s:
	bin2s INSTALL/APPS/OPNPS2LD.ELF opl_elf.s opl_elf

ule_elf.s:
	bin2s INSTALL/APPS/ULE.ELF ule_elf.s ule_elf

apps_icn.s:
	bin2s INSTALL/APPS/tunacan.icn apps_icn.s apps_icn

apps_sys.s:
	bin2s INSTALL/APPS/icon.sys apps_sys.s apps_sys
	
OpenTuna_FAT-110-120-150-160.s:
	bin2s INSTALL/OPENTUNA/OpenTuna_FAT-110-120-150-160.bin OpenTuna_FAT-110-120-150-160.s opentuna_fats
OpenTuna_FAT-170.s:
	bin2s INSTALL/OPENTUNA/OpenTuna_FAT-170.bin OpenTuna_FAT-170.s opentuna_fat170
	
clean:
	rm -fr *.o $(EE_BIN_PACKED) $(EE_BIN_STRIPPED) $(EE_BIN) opentuna_icn.* opentuna_sys.* opl_elf.* ule_elf.* apps_icn.* apps_sys.* OpenTuna_FAT-110-120-150-160.* OpenTuna_FAT-170.*

$(EE_BIN_STRIPPED): $(EE_BIN)
	$(EE_STRIP) -o $@ $<

$(EE_BIN_PACKED): $(EE_BIN_STRIPPED)
	ps2-packer $< $@ > /dev/null

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
