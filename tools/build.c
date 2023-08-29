// 将 os_system.o 文件 做成可执行 img 文件


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "../include/fs/elf.h"

#define ELF_HEADER_SIZE 52


/*
    [ELF 头文件]
    [程序头部表]
      Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
      // 其实这个段是 elf_Header+ph_header = 244 字节
      LOAD           0x000000 0x08048000 0x08048000 0x000f4 0x000f4 R   0x1000

       // .text 的段 = 45371 = b138 4字节对齐
      LOAD           0x001000 0x08049000 0x08049000 0x0b13b 0x0b13b R E 0x1000   

      // .rodata+..eh_frame 段的结合
      LOAD           0x00d000 0x08055000 0x08055000 0x026dc 0x026dc R   0x1000

      // .got.plt + .data .bss = + data.rel.ro
      LOAD           0x00fea0 0x08058ea0 0x08058ea0 0x00d84 0x480d8 RW  0x1000
      

     .data.rel.ro =  154+c+c04+47338  4809C
    【段 LOAD1】    244字节
    【段 LOAD2】    45371 字节
    【段 LOAD3】
    【段 LOAD4】
    【段 STACK】
    【段 RELRO】

    [节区头部表]

     Section to Segment mapping:
      段节...
      00     
      01     .text 
      02     .rodata .eh_frame 
      03     .data.rel.ro .got.plt .data .bss 
      04     
      05     .data.rel.ro 



    // 49467
*/

char *os_system = "/media/lonely/新加卷/工程实践/OS/tools/system";
char *tar_img = "/media/lonely/新加卷/工程实践/OS/tools/system.bin";
char buf[1024];

struct elf32_header *elf_head;
struct elf32_program_header_table *ph_tables[4];

static unsigned long to_long_bit(int index1,int index2,int index3,int index4)
{
  return (buf[index1] & 0xff) + ((buf[index2] << 8) & 0xff00) + 
        ((buf[index3] << 16) & 0xff0000) + ((buf[index4] << 24) & 0xff000000);
}

// gcc -m32 build.c -o build
int main()
{
  int i;
  char *img_vaddr;
  FILE *fd;
  FILE *tar_file;

  unsigned long ph_in_file_sizes[4];
  unsigned long ph_offset_in_file[4];

  unsigned long ph_start;
  unsigned short ph_size;
  unsigned short ph_num;
  unsigned long ph_total_size;

  tar_file = fopen(tar_img, "w");
  fd = fopen(os_system, "r");

  if(fd <= 0)
  {
    printf("没有 可执行文件!");
    return 0;
  }
  
  printf("fd= %p \n",fd);

  size_t rc = fread(buf, 1024, 1, fd);
  if(rc != 1)
  {
     printf("read elf_header error \n",rc);
     return 0;
  }

  ph_start = buf[28];
  ph_size  = buf[42];
  ph_num   = buf[44];

  printf("ph_start =%d, ph_size=%d, ph_num=%d \n", ph_start,  ph_size,  ph_num);

  // 在文件中的 offset
  ph_offset_in_file[0] = to_long_bit(56, 57, 58, 59);
  ph_offset_in_file[1] = to_long_bit(88, 89, 90, 91);
  ph_offset_in_file[2] = to_long_bit(120, 121, 122, 123);
  ph_offset_in_file[3] = to_long_bit(152, 153, 154, 155);

  // 段在文件中的大小
  ph_in_file_sizes[0] = to_long_bit(68, 69, 70, 71);
  ph_in_file_sizes[1] = to_long_bit(100, 101, 102, 103);    // 100
  ph_in_file_sizes[2] = to_long_bit(132, 133, 134, 135);    // 132
  ph_in_file_sizes[3] = to_long_bit(164, 165, 166, 167);    // 164

  printf("ph_offset_in_file[0] 文件中的offset =%x,Size=%x \n",ph_offset_in_file[0], ph_in_file_sizes[0]);
  printf("ph_offset_in_file[1] 文件中的offset =%x,Size=%x \n",ph_offset_in_file[1], ph_in_file_sizes[1]);
  printf("ph_offset_in_file[2] 文件中的offset =%x,Size=%x \n",ph_offset_in_file[2], ph_in_file_sizes[2]);

  ph_total_size =  ph_offset_in_file[2] + ph_in_file_sizes[2];
  
  printf("总大小 =%x \n",  ph_total_size);

  img_vaddr =  (char*)malloc(ph_total_size);
  
  fseek(fd, ph_offset_in_file[0], SEEK_SET);
  rc = fread(img_vaddr, ph_total_size, 1, fd);

  if(rc != 1)
    printf("读取错误！=%d \n",rc);

  rc = fwrite(img_vaddr, ph_total_size, 1, tar_file);

  printf("关闭文件! =%d \n",rc);

  fclose(fd);
  fclose(tar_file);


  return 0;
}