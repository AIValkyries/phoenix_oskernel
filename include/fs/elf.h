#ifndef _ELF_H
#define _ELF_H

// 编码信息 
#define ELF_32  0x01
#define ELF_64  0x02
#define ELFDATA2LSB 0x01    // 小端编码
#define ELFDATA2MSB 0x02    // 大端编码

#define EV_CURRENT 1        // 当前版本号

// 文件类型
#define ET_NONE 0
#define ET_REL 0x01  // 可重定位文件
#define ET_EXEC 0x02 // 可执行文件
#define ET_DYN  0x03  // 动态链接库
#define ET_CORE 0x04  // core 文件


// 适用的处理器体系结构
#define EM_NONE 0  
#define EM_M32  1    // AT&T WE 32100 
#define EM_SPARC 2
#define EM_386 3        // Intel Architecture 
#define EM_X86_X64 62   // 新版本有很多个处理器体系，忽略掉    


/* ----------------------------程序头表的信息--------------------------  */
// 段的类型
#define PT_NULL 0
#define PT_LOAD 1       // 可装载的段
#define PT_DYNAMIC 2    // 动态链接的信息
#define PT_INTERP  3    // 这种段类型只对可执行程序有意义，当它出现在共享目标文件中时，是一个无意义的多余项。
#define PT_NOTE    4
#define PT_SHLIB   5    // 该段类型是保留的
#define PT_PHDR    6

// 段的权限
#define PF_X    0x01    // 可执行
#define PF_W    0x02    // 只写
#define PF_R    0x04    // 只读
#define PF_RE   0x05
#define PF_RW   0x06
#define PF_MASKPROC 0xf0000000  // 未指定


/* ----------------------------符号表的信息--------------------------  */

// 符号绑定
#define STB_LOCAL   0       // 局部符号
#define STB_GLOBAL  1       // 全局符号
#define STB_WEAK    2       // 弱引用
#define STB_LOPROC  13
#define STB_HIPROC  15

// 符号类型
#define STT_NOTYPE  0
#define STT_OBJECT  1       // 对象，数组，变量
#define STT_FUNC    2       // 函数
#define STT_SECTION 3
#define STT_FILE    4
#define STT_LOPROC  13
#define STT_HIPROC  15


// 不需要附加的重定位项
struct elf32_rel
{
    unsigned long offset;
    unsigned long info;
};

// 需要附加的重定位项
struct elf32_rela
{
    unsigned long offset;
    unsigned long info;
    unsigned long addend;
};

// 文件头 52 字节
struct elf32_header
{
    /*
        Magic       ：7f 45 4c 46 01 01 01 【00 00 00 00 00 00 00 00 00】
        Class       : elf32 文件类型
        Data        : 2s complement, little endian （小端） 编码格式
        Version     : 1 (current)
        OS/ABI      : UNIX-System V
        ABI Version : 0

        魔数4字节
        {
            ELFMAG0 0x7f e_ident[EI_MAG0]
            ELFMAG1 ‘E’  e_ident[EI_MAG1]
            ELFMAG2 ‘L’  e_ident[EI_MAG2] 
            ELFMAG3 ‘F’  e_ident[EI_MAG3]
        }
        EI_CLASS
        {
            ELFCLASSNONE 0 非法目标文件
            ELFCLASS32 1 32 位目标文件
            ELFCLASS64 2 64 位目标文件
        }
        EI_DATA
        {
            ELFDATANONE 0 非法编码格式
            ELFDATA2LSB 1 LSB 编码(小端编码) 
            ELFDATA2MSB 2 MSB 编码(大端编码)

            ## Intel 架构中， e_ident[EI_DATA] 取值为 ELFDATA2LSB 。
        }
        EI_VERSION     : 1 (current)
        从 e_ident[EI_PAD] 到 e_ident[EI_NIDENT-1] 之间的 9 个字节目前暂时不使用
    */
    unsigned char ident[16];

    unsigned short type;
    unsigned short machine;

    unsigned long version;
    unsigned long entry;      // 入口地址
    unsigned long phoff;      // 程序头表在文件中的 offset 32
    unsigned long shoff;      // 段表在文件中的 offset
    unsigned long flags;        // 对于 intel 来说，这个值为0

    unsigned short ehsize;     // elf 文件头的大小 40
    unsigned short phentsize;  // 程序头表项的大小, 字节
    unsigned short phnum;      // 程序头表的项数
    
    unsigned short shentsize;  // 段表项的大小, 字节
    unsigned short shnum;      // 段表的项数

    unsigned short shstrndx;
};


/*
    程序头表 [56/2 字节]
*/
struct elf32_program_header_table
{
    unsigned long type;         // 52~56
    // 本段在文件中的偏移量（开始位置）
    unsigned long offset;        // 56~60

    unsigned long vaddr;         // 本段在内存中的虚拟起始地址          60~64
    unsigned long paddr;         // 物理地址是相对寻址的系统上,保留不用  64~68
    unsigned long filesz;        // 本段在文件中的大小                68~72
    unsigned long memsz;         // 本段在内存镜像中的字节大小

    unsigned long flags;
    unsigned long align;        // 对齐
};

/*
    段表
*/
struct elf32_section_header_table
{
    unsigned long name_index;   // 字符串 在.shstrtab 的偏移量

    /*
        SHT_NULL
        SHT_PROGBITS
        SHT_SYMTAB   符号表
        SHT_STRTAB   字符串表
        SHT_RELA     重定位表
        SHT_HASH     符号表的哈希表
        SHT_DYNAMIC  动态链接信心
        SHT_NOTE     提示性信息
        SHT_NOBITS
        SHT_REL     重定位信息包含了
        SHT_SHLIB   保留
        SHT_DNYSYM  动态链接的符号表
    */
    unsigned long type;         // 段的类型

    /*
        SHF_WRITE
        SHF_ALLOC  
        SHF_EXECINSTR
    */
    unsigned long flags;        // 段的标志
    unsigned long addr;         // 段虚拟地址
    unsigned long offset;       // 段偏移
    unsigned long size;         // 段的长度
    unsigned long link;         // 段链接信息
    unsigned long info;         // 
    unsigned long addralign;    // 段地址对齐
    unsigned long entsize;      // 项的长度
};

/*
    符号表 = symbol的数组
    每个 elf32_symbol_table 对应一个符号
    数组[0] = 未定义符号
*/
struct elf32_symbol_table
{
    unsigned long name_index;  // 在字符串中的索引
    unsigned long value;       // 符号相对应的值，也可能是一个地址符号
    unsigned long size;        // 符号的大小

    unsigned char info;         // 符号类型和绑定信息
    unsigned char other;        // 
    unsigned long shndx;        // 符号所在的段
};





#endif