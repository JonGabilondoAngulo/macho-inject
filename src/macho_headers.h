//
//  macho_headers.h
//  macho-inject
//
//  Created by Jon Gabilondo on 2/27/17.
//

#ifndef macho_headers_h
#define macho_headers_h

#include <stdio.h>

#endif /* macho_headers_h */

#ifndef _UINT32_T
#define _UINT32_T
typedef unsigned int uint32_t;
#endif /* _UINT32_T */

/* Constant for the magic field of the mach_header_64 (64-bit architectures) */
#define MH_MAGIC_64 0xfeedfacf /* the 64-bit mach magic number */
#define MH_CIGAM_64 0xcffaedfe /* NXSwapInt(MH_MAGIC_64) */
#define	LC_LOAD_DYLIB	0xc	/* load a dynamically linked shared library */
#define LC_CODE_SIGNATURE 0x1d	/* local of code signature */
#define FAT_MAGIC	0xcafebabe
#define FAT_CIGAM	0xbebafeca	/* NXSwapLong(FAT_MAGIC) */

/*
 * The support for the 64-bit fat file format described here is a work in
 * progress and not yet fully supported in all the Apple Developer Tools.
 *
 * When a slice is greater than 4mb or an offset to a slice is greater than 4mb
 * then the 64-bit fat file format is used.
 */
#define FAT_MAGIC_64	0xcafebabf
#define FAT_CIGAM_64	0xbfbafeca	/* NXSwapLong(FAT_MAGIC_64) */

/*
 * A variable length string in a load command is represented by an lc_str
 * union.  The strings are stored just after the load command structure and
 * the offset is from the start of the load command structure.  The size
 * of the string is reflected in the cmdsize field of the load command.
 * Once again any padded bytes to bring the cmdsize field to a multiple
 * of 4 bytes must be zero.
 */
union lc_str {
    uint32_t	offset;	/* offset to the string */
#ifndef __LP64__
    char		*ptr;	/* pointer to the string */
#endif
};

struct fat_header {
    uint32_t	magic;		/* FAT_MAGIC or FAT_MAGIC_64 */
    uint32_t	nfat_arch;	/* number of structs that follow */
};

typedef int			integer_t;
typedef integer_t	cpu_type_t;
typedef integer_t	cpu_subtype_t;

struct fat_arch {
    cpu_type_t	cputype;	/* cpu specifier (int) */
    cpu_subtype_t	cpusubtype;	/* machine specifier (int) */
    uint32_t	offset;		/* file offset to this object file */
    uint32_t	size;		/* size of this object file */
    uint32_t	align;		/* alignment as a power of 2 */
};

/*
 * Dynamicly linked shared libraries are identified by two things.  The
 * pathname (the name of the library as found for execution), and the
 * compatibility version number.  The pathname must match and the compatibility
 * number in the user of the library must be greater than or equal to the
 * library being used.  The time stamp is used to record the time a library was
 * built and copied into user so it can be use to determined if the library used
 * at runtime is exactly the same as used to built the program.
 */
struct dylib {
    union lc_str  name;			/* library's path name */
    uint32_t timestamp;			/* library's build time stamp */
    uint32_t current_version;		/* library's current version number */
    uint32_t compatibility_version;	/* library's compatibility vers number*/
};

/*
 * The 32-bit mach header appears at the very beginning of the object file for
 * 32-bit architectures.
 */
struct mach_header {
    uint32_t	magic;		/* mach magic number identifier */
    cpu_type_t	cputype;	/* cpu specifier */
    cpu_subtype_t	cpusubtype;	/* machine specifier */
    uint32_t	filetype;	/* type of file */
    uint32_t	ncmds;		/* number of load commands */
    uint32_t	sizeofcmds;	/* the size of all the load commands */
    uint32_t	flags;		/* flags */
};

/*
 * The 64-bit mach header appears at the very beginning of object files for
 * 64-bit architectures.
 */
struct mach_header_64 {
    uint32_t	magic;		/* mach magic number identifier */
    cpu_type_t	cputype;	/* cpu specifier */
    cpu_subtype_t	cpusubtype;	/* machine specifier */
    uint32_t	filetype;	/* type of file */
    uint32_t	ncmds;		/* number of load commands */
    uint32_t	sizeofcmds;	/* the size of all the load commands */
    uint32_t	flags;		/* flags */
    uint32_t	reserved;	/* reserved */
};

/*
 * A dynamically linked shared library (filetype == MH_DYLIB in the mach header)
 * contains a dylib_command (cmd == LC_ID_DYLIB) to identify the library.
 * An object that uses a dynamically linked shared library also contains a
 * dylib_command (cmd == LC_LOAD_DYLIB, LC_LOAD_WEAK_DYLIB, or
 * LC_REEXPORT_DYLIB) for each library it uses.
 */
struct dylib_command {
    uint32_t	cmd;		/* LC_ID_DYLIB, LC_LOAD_{,WEAK_}DYLIB,
                             LC_REEXPORT_DYLIB */
    uint32_t	cmdsize;	/* includes pathname string */
    struct dylib	dylib;		/* the library identification */
};

/*
 * The linkedit_data_command contains the offsets and sizes of a blob
 * of data in the __LINKEDIT segment.
 */
struct linkedit_data_command {
    uint32_t	cmd;		/* LC_CODE_SIGNATURE or LC_SEGMENT_SPLIT_INFO */
    uint32_t	cmdsize;	/* sizeof(struct linkedit_data_command) */
    uint32_t	dataoff;	/* file offset of data in __LINKEDIT segment */
    uint32_t	datasize;	/* file size of data in __LINKEDIT segment  */
};


enum {
    noErr                         = 0
};
