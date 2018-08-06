#include <string.h>

#include <r_lib.h>
#include <r_io.h>
#include <r_types.h>
#include <r_core.h>
#include <r_bin.h>
#include <r_cons.h>

static void __process_section(RCore *core, ut64 base)
{
    ut8 buffer[64];
    ut8 format_buff[256];
    ut64 entry_pc;
    ut64 entry_info;
    ut64 nfunctab;

    r_io_nread_at(core->io, base, buffer, sizeof(buffer));

    ut8 quantum = buffer[6];
    ut8 ptrsize = buffer[7];

    if(ptrsize == 8){
        nfunctab = *(ut64 *)&buffer[8];
    }else{
        nfunctab = *(ut32 *)&buffer[8];
    }

    r_cons_printf("[gorec] Now resolving %u symbols...\n", nfunctab);

    // Looping over our functions
    int i;
    int y;
    ut64 entry_iter = base + 8 + ptrsize;
    ut32 stroffset = 0;

    for(i = 0; i < nfunctab; i++){
        // Part 1: Read entries from functab
        r_io_nread_at(core->io, entry_iter, buffer, sizeof(buffer));

        if(ptrsize == 4){
            entry_pc = (ut32)(*(ut32 *)&buffer[0]);
            entry_info = *(ut32 *)&buffer[ptrsize];
        }else{
            entry_pc = *(ut64 *)&buffer[0];
            entry_info = *(ut64 *)&buffer[ptrsize];
        }

        //r_cons_printf("[gorec] pc: 0x%lx, info: 0x%lx\n", entry_pc, entry_info);
        entry_iter += ptrsize * 2;

        // Part 2: Get symbol name
        r_io_nread_at(core->io, base + entry_info + ptrsize, buffer, sizeof(buffer));
        stroffset = *(ut32 *)&buffer[0];
        r_io_nread_at(core->io, base + stroffset, buffer, sizeof(buffer));

        r_core_cmd(core, "fs symbols", 0);
        snprintf(format_buff, sizeof(format_buff), "f sym.%s @ 0x%lx\n", buffer, entry_pc);

        // Part 3: Cleaning input a bit
        for(y = 0; y < sizeof(format_buff); y++){
            switch(format_buff[y]){
                case '{':
                case '}':
                case ';':
                    format_buff[y] = '_';
            }
        }

        // Part 4: Registering the symbol
        r_core_cmd(core, format_buff, 0);
    }
}

static void __recover_symbols(RCore *core)
{
    RList *sections = r_bin_get_sections(core->bin);
    RListIter *iter;
    RBinSection *section;
    ut64 offset = 0;

    r_list_foreach(sections, iter, section){
        if(strncmp(section->name, ".gopclntab", 10) == 0){
            offset = section->vaddr;
            break;
        }
    }

    if(offset == 0){
        r_cons_printf("[gorec] Couldn't find section, trying fallback method\n");
        r_core_cmd(core, "/x fbffffff0000", 0); // trying to find the pattern

        // Now we need to check that the input data is valid
        ut8 buffer[16];
        RFlagItem *item;

        r_list_foreach(core->flags->flags, iter, item){
            if(strncmp(item->name, "hit", 3) == 0){
                r_io_nread_at(core->io, item->offset, buffer, sizeof(buffer));

                if((buffer[6] == 1 || buffer[6] == 4) && (buffer[7] == 4 || buffer[7] == 8)){
                    offset = item->offset;
                    break;
                }
            }
        }
    }

    if(offset == 0){
        r_cons_printf("[gorec] Couldn't find .gopclntab section, exiting...\n");
        return;
    }

    r_cons_printf("[gorec] Section .gopclntab at: 0x%lx\n", offset);
    __process_section(core, offset);
}


static int __cmd_handler(void *user, const char *input)
{
    RCore *core = (RCore *)user;
    
    if(strncmp(input, "gorec", 5) == 0)
    {
        __recover_symbols(core);
    }

    return 0;
}

RCorePlugin r_core_plugin_gorecover = {
	.name = "gorecover",
	.desc = "Recover stripped symbols in go executables",
	.call = __cmd_handler,
};

#ifndef CORELIB
RLibStruct radare_plugin = {
	.type = R_LIB_TYPE_CORE,
	.data = &r_core_plugin_gorecover,
	.version = R2_VERSION
};
#endif