#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <xenaccess/xenaccess.h>
#include <xenaccess/xa_private.h>
#include "file_util.h"

#define MEMORY_SIZE 0x40000000U
#define ATTACABLE_BYTES (MEMORY_SIZE - 0x20000U)
#define EX_COUNT 500
#define LOG_FILENAME "./experiment.log"
#define SLEEP_SECONDS 60

int init(uint32_t dom_id,xa_instance_t *xai);
int inject_byte(uint32_t addr,xa_instance_t *xai);
int inject_page(uint32_t addr,xa_instance_t *xai);
int is_guest_dead(xa_instance_t *xai);
int cleanup(xa_instance_t *xai);

int main(int argc, char **argv)
{
    int i;
    xa_instance_t xai;
    uint32_t dom_id = atoi(argv[1]);
    uint32_t addr;
    unsigned int ex_count = 0,attack_count;
    FILE *fp;    
    
    /* file open */
    fp = file_open(LOG_FILENAME);
    /*  xc_domain_pause(xai.xc_handle,dom); */
    /* inject faults */

    while(ex_count < EX_COUNT){
      /* initialize */
      if(init(dom_id++, &xai) == 0){
	printf("initialize failed\n");
	exit(1);
      }

      attack_count = 0;
/*       for(addr = (0x40000000 - xai.page_size);addr >= 0x00000000 ;addr -= xai.page_size){ */
      for(addr = 0x00000000;addr < 0x40000000 ;addr += xai.page_size){
	inject_page(addr,&xai);
	attack_count++;

/* 	if((attack_count % 1000) == 0){ */
	  printf("inject 0x%x as %utimes\n",addr,attack_count);
/* 	} */

	if(is_guest_dead(&xai)){
	  fprintf(fp,"%u\n",attack_count);
	  printf("guest dead %d times\n",ex_count);
	  sleep(SLEEP_SECONDS);
	  break;
	}
      }
      cleanup(&xai);
      ex_count++;
    }

    /*  xc_domain_unpause(xai.xc_handle,dom); */
    file_close(fp);
    return 0;
}

/* initialize xai and return page size */
int init(uint32_t dom_id,xa_instance_t *xai){
  unsigned char *memory;
  uint32_t offset = 0x0;

  xa_init(dom_id,xai);
  memory = xa_access_physical_address(xai,0x0,&offset);
  if (memory == NULL){
    perror("failed to get page size");
    return 0;
  }else{
    return xai->page_size;
  }
}

int inject_bit(uint32_t addr,xa_instance_t *xai){
  uint32_t offset = 0;
  int digit;

  unsigned char *memory = xa_access_physical_address(xai,addr,&offset);
  if(memory == NULL){
    return 0;
  }else{
    /* bit flip */
    digit = rand() % 8;
    memory[0] = memory[0] ^ (1UL << digit);
    munmap(memory,xai->page_size);
    return 1;
  }
}

int inject_byte(uint32_t addr,xa_instance_t *xai){
  uint32_t offset = 0;

  unsigned char *memory = xa_access_physical_address(xai,addr,&offset);
  if(memory == NULL){
    return 0;
  }else{
    memory[0] = ~memory[0];
    munmap(memory,xai->page_size);
    return 1;
  }
}

int inject_page(uint32_t addr,xa_instance_t *xai){
  uint32_t offset = 0;
  int i;

  unsigned char *memory = xa_access_physical_address(xai,addr,&offset);
  if(memory == NULL){
    return 0;
  }else{
    for(i = 0;i < xai->page_size ;i++){
       memory[i] = ~memory[i];
    }
    munmap(memory,xai->page_size);
    return 1;
  }
}

int is_guest_dead(xa_instance_t *xai){
  if (xc_domain_getinfo(xai->xc_handle,xai->domain_id,1,&(xai->info)) != 1){
    return 1;
  }
  if((xai->info.crashed == 1) || (xai->info.dying == 1)){
    return 1;
  }
  
  return 0;
}

int cleanup(xa_instance_t *xai){
  xa_destroy(xai);
}
