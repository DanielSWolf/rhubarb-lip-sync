/*-
 * Copyright (c) 1998 John D. Polstra.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/sys/elf_common.h,v 1.14 2003/06/18 16:38:22 kan Exp $
 */

#ifndef _SYS_ELF_COMMON_H_
#define _SYS_ELF_COMMON_H_ 1

/*
 * ELF definitions that are independent of architecture or word size.
 */

/*
 * Note header.  The ".note" section contains an array of notes.  Each
 * begins with this header, aligned to a word boundary.  Immediately
 * following the note header is n_namesz bytes of name, padded to the
 * next word boundary.  Then comes n_descsz bytes of descriptor, again
 * padded to a word boundary.  The values of n_namesz and n_descsz do
 * not include the padding.
 */

typedef struct {
	uint32_t	n_namesz;	/* Length of name. */
	uint32_t	n_descsz;	/* Length of descriptor. */
	uint32_t	n_type;		/* Type of this note. */
} Elf_Note;

/* Indexes into the e_ident array.  Keep synced with 
   http://www.sco.com/developer/gabi/ch4.eheader.html */
#define EI_MAG0		0	/* Magic number, byte 0. */
#define EI_MAG1		1	/* Magic number, byte 1. */
#define EI_MAG2		2	/* Magic number, byte 2. */
#define EI_MAG3		3	/* Magic number, byte 3. */
#define EI_CLASS	4	/* Class of machine. */
#define EI_DATA		5	/* Data format. */
#define EI_VERSION	6	/* ELF format version. */
#define EI_OSABI	7	/* Operating system / ABI identification */
#define EI_ABIVERSION	8	/* ABI version */
#define OLD_EI_BRAND	8	/* Start of architecture identification. */
#define EI_PAD		9	/* Start of padding (per SVR4 ABI). */
#define EI_NIDENT	16	/* Size of e_ident array. */

/* Values for the magic number bytes. */
#define ELFMAG0		0x7f
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'
#define ELFMAG		"\177ELF"	/* magic string */
#define SELFMAG		4		/* magic string size */

/* Values for e_ident[EI_VERSION] and e_version. */
#define EV_NONE		0
#define EV_CURRENT	1

/* Values for e_ident[EI_CLASS]. */
#define ELFCLASSNONE	0	/* Unknown class. */
#define ELFCLASS32	1	/* 32-bit architecture. */
#define ELFCLASS64	2	/* 64-bit architecture. */

/* Values for e_ident[EI_DATA]. */
#define ELFDATANONE	0	/* Unknown data format. */
#define ELFDATA2LSB	1	/* 2's complement little-endian. */
#define ELFDATA2MSB	2	/* 2's complement big-endian. */

/* Values for e_ident[EI_OSABI]. */
#define ELFOSABI_SYSV		0	/* UNIX System V ABI */
#define ELFOSABI_NONE		ELFOSABI_SYSV	/* symbol used in old spec */
#define ELFOSABI_HPUX		1	/* HP-UX operating system */
#define ELFOSABI_NETBSD		2	/* NetBSD */
#define ELFOSABI_LINUX		3	/* GNU/Linux */
#define ELFOSABI_HURD		4	/* GNU/Hurd */
#define ELFOSABI_86OPEN		5	/* 86Open common IA32 ABI */
#define ELFOSABI_SOLARIS	6	/* Solaris */
#define ELFOSABI_MONTEREY	7	/* Monterey */
#define ELFOSABI_IRIX		8	/* IRIX */
#define ELFOSABI_FREEBSD	9	/* FreeBSD */
#define ELFOSABI_TRU64		10	/* TRU64 UNIX */
#define ELFOSABI_MODESTO	11	/* Novell Modesto */
#define ELFOSABI_OPENBSD	12	/* OpenBSD */
#define ELFOSABI_ARM		97	/* ARM */
#define ELFOSABI_STANDALONE	255	/* Standalone (embedded) application */

/* e_ident */
#define IS_ELF(ehdr)	((ehdr).e_ident[EI_MAG0] == ELFMAG0 && \
			 (ehdr).e_ident[EI_MAG1] == ELFMAG1 && \
			 (ehdr).e_ident[EI_MAG2] == ELFMAG2 && \
			 (ehdr).e_ident[EI_MAG3] == ELFMAG3)

/* Values for e_type. */
#define ET_NONE		0	/* Unknown type. */
#define ET_REL		1	/* Relocatable. */
#define ET_EXEC		2	/* Executable. */
#define ET_DYN		3	/* Shared object. */
#define ET_CORE		4	/* Core file. */

/* Values for e_machine. */
#define EM_NONE		0	/* Unknown machine. */
#define EM_M32		1	/* AT&T WE32100. */
#define EM_SPARC	2	/* Sun SPARC. */
#define EM_386		3	/* Intel i386. */
#define EM_68K		4	/* Motorola 68000. */
#define EM_88K		5	/* Motorola 88000. */
#define EM_486		6	/* Intel i486. */
#define EM_860		7	/* Intel i860. */
#define EM_MIPS		8	/* MIPS R3000 Big-Endian only */

/* Extensions.  This list is not complete. */
#define EM_S370		9	/* IBM System/370 */
#define EM_MIPS_RS4_BE	10	/* MIPS R4000 Big-Endian */ /* Depreciated */
#define EM_PARISC	15	/* HPPA */
#define EM_SPARC32PLUS	18	/* SPARC v8plus */
#define EM_PPC		20	/* PowerPC 32-bit */
#define EM_PPC64	21	/* PowerPC 64-bit */
#define EM_ARM		40	/* ARM */
#define EM_SPARCV9	43	/* SPARC v9 64-bit */
#define EM_IA_64	50	/* Intel IA-46 Processor */
#define EM_X86_64	62	/* Advanced Micro Devices x86-64 */
#define EM_ALPHA	0x9026	/* Alpha (written in the absence of an ABI */

/* Special section indexes. */
#define SHN_UNDEF	     0		/* Undefined, missing, irrelevant. */
#define SHN_LORESERVE	0xff00		/* First of reserved range. */
#define SHN_LOPROC	0xff00		/* First processor-specific. */
#define SHN_HIPROC	0xff1f		/* Last processor-specific. */
#define SHN_ABS		0xfff1		/* Absolute values. */
#define SHN_COMMON	0xfff2		/* Common data. */
#define SHN_HIRESERVE	0xffff		/* Last of reserved range. */

/* sh_type */
#define SHT_NULL	0		/* inactive */
#define SHT_PROGBITS	1		/* program defined information */
#define SHT_SYMTAB	2		/* symbol table section */
#define SHT_STRTAB	3		/* string table section */
#define SHT_RELA	4		/* relocation section with addends */
#define SHT_HASH	5		/* symbol hash table section */
#define SHT_DYNAMIC	6		/* dynamic section */ 
#define SHT_NOTE	7		/* note section */
#define SHT_NOBITS	8		/* no space section */
#define SHT_REL		9		/* relocation section - no addends */
#define SHT_SHLIB	10		/* reserved - purpose unknown */
#define SHT_DYNSYM	11		/* dynamic symbol table section */ 
#define SHT_NUM		12		/* number of section types */
#define SHT_LOOS	0x60000000	/* First of OS specific semantics */
#define SHT_HIOS	0x6fffffff	/* Last of OS specific semantics */
#define SHT_LOPROC	0x70000000	/* reserved range for processor */
#define SHT_HIPROC	0x7fffffff	/* specific section header types */
#define SHT_LOUSER	0x80000000	/* reserved range for application */
#define SHT_HIUSER	0xffffffff	/* specific indexes */

/* Flags for sh_flags. */
#define SHF_WRITE	0x1		/* Section contains writable data. */
#define SHF_ALLOC	0x2		/* Section occupies memory. */
#define SHF_EXECINSTR	0x4		/* Section contains instructions. */
#define SHF_TLS		0x400		/* Section contains TLS data. */
#define SHF_MASKPROC	0xf0000000	/* Reserved for processor-specific. */

/* Values for p_type. */
#define PT_NULL		0	/* Unused entry. */
#define PT_LOAD		1	/* Loadable segment. */
#define PT_DYNAMIC	2	/* Dynamic linking information segment. */
#define PT_INTERP	3	/* Pathname of interpreter. */
#define PT_NOTE		4	/* Auxiliary information. */
#define PT_SHLIB	5	/* Reserved (not used). */
#define PT_PHDR		6	/* Location of program header itself. */
#define	PT_TLS		7	/* Thread local storage segment */

#define PT_COUNT	8	/* Number of defined p_type values. */

#define	PT_LOOS		0x60000000	/* OS-specific */
#define	PT_HIOS		0x6fffffff	/* OS-specific */
#define PT_LOPROC	0x70000000	/* First processor-specific type. */
#define PT_HIPROC	0x7fffffff	/* Last processor-specific type. */

/* Values for p_flags. */
#define PF_X		0x1	/* Executable. */
#define PF_W		0x2	/* Writable. */
#define PF_R		0x4	/* Readable. */

/* Values for d_tag. */
#define DT_NULL		0	/* Terminating entry. */
#define DT_NEEDED	1	/* String table offset of a needed shared
				   library. */
#define DT_PLTRELSZ	2	/* Total size in bytes of PLT relocations. */
#define DT_PLTGOT	3	/* Processor-dependent address. */
#define DT_HASH		4	/* Address of symbol hash table. */
#define DT_STRTAB	5	/* Address of string table. */
#define DT_SYMTAB	6	/* Address of symbol table. */
#define DT_RELA		7	/* Address of ElfNN_Rela relocations. */
#define DT_RELASZ	8	/* Total size of ElfNN_Rela relocations. */
#define DT_RELAENT	9	/* Size of each ElfNN_Rela relocation entry. */
#define DT_STRSZ	10	/* Size of string table. */
#define DT_SYMENT	11	/* Size of each symbol table entry. */
#define DT_INIT		12	/* Address of initialization function. */
#define DT_FINI		13	/* Address of finalization function. */
#define DT_SONAME	14	/* String table offset of shared object
				   name. */
#define DT_RPATH	15	/* String table offset of library path. [sup] */
#define DT_SYMBOLIC	16	/* Indicates "symbolic" linking. [sup] */
#define DT_REL		17	/* Address of ElfNN_Rel relocations. */
#define DT_RELSZ	18	/* Total size of ElfNN_Rel relocations. */
#define DT_RELENT	19	/* Size of each ElfNN_Rel relocation. */
#define DT_PLTREL	20	/* Type of relocation used for PLT. */
#define DT_DEBUG	21	/* Reserved (not used). */
#define DT_TEXTREL	22	/* Indicates there may be relocations in
				   non-writable segments. [sup] */
#define DT_JMPREL	23	/* Address of PLT relocations. */
#define	DT_BIND_NOW	24	/* [sup] */
#define	DT_INIT_ARRAY	25	/* Address of the array of pointers to
				   initialization functions */
#define	DT_FINI_ARRAY	26	/* Address of the array of pointers to
				   termination functions */
#define	DT_INIT_ARRAYSZ	27	/* Size in bytes of the array of
				   initialization functions. */
#define	DT_FINI_ARRAYSZ	28	/* Size in bytes of the array of
				   terminationfunctions. */
#define	DT_RUNPATH	29	/* String table offset of a null-terminated
				   library search path string. */
#define	DT_FLAGS	30	/* Object specific flag values. */
#define	DT_ENCODING	32	/* Values greater than or equal to DT_ENCODING
				   and less than DT_LOOS follow the rules for
				   the interpretation of the d_un union
				   as follows: even == 'd_ptr', even == 'd_val'
				   or none */
#define	DT_PREINIT_ARRAY 32	/* Address of the array of pointers to
				   pre-initialization functions. */
#define	DT_PREINIT_ARRAYSZ 33	/* Size in bytes of the array of
				   pre-initialization functions. */

#define	DT_COUNT	33	/* Number of defined d_tag values. */

#define	DT_LOOS		0x6000000d	/* First OS-specific */
#define	DT_HIOS		0x6fff0000	/* Last OS-specific */
#define	DT_LOPROC	0x70000000	/* First processor-specific type. */
#define	DT_HIPROC	0x7fffffff	/* Last processor-specific type. */

/* Values for DT_FLAGS */
#define	DF_ORIGIN	0x0001	/* Indicates that the object being loaded may
				   make reference to the $ORIGIN substitution
				   string */
#define	DF_SYMBOLIC	0x0002	/* Indicates "symbolic" linking. */
#define	DF_TEXTREL	0x0004	/* Indicates there may be relocations in
				   non-writable segments. */
#define	DF_BIND_NOW	0x0008	/* Indicates that the dynamic linker should
				   process all relocations for the object
				   containing this entry before transferring
				   control to the program. */
#define	DF_STATIC_TLS	0x0010	/* Indicates that the shared object or
				   executable contains code using a static
				   thread-local storage scheme. */

/* Values for n_type.  Used in core files. */
#define NT_PRSTATUS	1	/* Process status. */
#define NT_FPREGSET	2	/* Floating point registers. */
#define NT_PRPSINFO	3	/* Process state info. */

/* Symbol Binding - ELFNN_ST_BIND - st_info */
#define STB_LOCAL	0	/* Local symbol */
#define STB_GLOBAL	1	/* Global symbol */
#define STB_WEAK	2	/* like global - lower precedence */
#define STB_LOPROC	13	/* reserved range for processor */
#define STB_HIPROC	15	/*  specific symbol bindings */

/* Symbol type - ELFNN_ST_TYPE - st_info */
#define STT_NOTYPE	0	/* Unspecified type. */
#define STT_OBJECT	1	/* Data object. */
#define STT_FUNC	2	/* Function. */
#define STT_SECTION	3	/* Section. */
#define STT_FILE	4	/* Source file. */
#define STT_TLS		6	/* TLS object. */
#define STT_LOPROC	13	/* reserved range for processor */
#define STT_HIPROC	15	/*  specific symbol types */

/* Special symbol table indexes. */
#define STN_UNDEF	0	/* Undefined symbol index. */

#endif /* !_SYS_ELF_COMMON_H_ */
