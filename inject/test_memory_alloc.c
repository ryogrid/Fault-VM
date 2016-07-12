#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h> 		/* for execlp */
#include <xenaccess/xenaccess.h>
#include <xenaccess/xa_private.h>

#include "file_util.h"
#include "tcp_util.h"

/* for hash table */
#include <apr_general.h>
#include <apr_hash.h>
#include <apr_strings.h>

#define MEMORY_SIZE 0x20000000U
#define ATTACABLE_BYTES (MEMORY_SIZE - 0x20000U)
#define EX_COUNT 30
#define LOG_FILENAME "./experiment.log"
#define SLEEP_SECONDS 50
#define GUEST_IP_ADDR "172.16.100.101"
#define GUEST_PORT 8888
#define DESTROY_WAIT_SECONDS 2
#define GUEST_NAME "centos"
#define GUEST_FILE_PATH "/root/work/xen/centos_hvm/centos_hvm.hvm"

int init(uint32_t dom_id,xa_instance_t *xai);
int inject_byte(uint32_t addr,xa_instance_t *xai);
int inject_page(uint32_t addr,xa_instance_t *xai);
int is_guest_dead(xa_instance_t *xai);
int cleanup(xa_instance_t *xai);
int create_domain_with_xc();
int destroy_domain_with_xc();

int main(int argc, char **argv)
{
    int i,is_init;
    xa_instance_t xai;
    uint32_t dom_id = atoi(argv[1]);
    uint32_t addr;
    unsigned int ex_count = 0,attack_count,rc;
    FILE *fp;
    
    file open
    fp = file_open(LOG_FILENAME);

    inject faults
    while(ex_count < EX_COUNT){
      initialize
      is_init = -1;
      if((is_init = init(dom_id++, &xai)) == 0){
	printf("initialize failed\n");
	ex_count--;
      }
      
      attack_count = 0;
      srand(rand());
      rc = 0;

      addr = rand() % MEMORY_SIZE;
      if(inject_bit(addr,&xai)){
	printf("inject failed. so go out loop\n");
	break;
      }

      if(cleanup(&xai) == XA_FAILURE){
	printf("cleanup of XA failed\n");
      }

      if(destroy_domain_with_xc()){
	printf("destroy domain failed.\n");
	fflush(stdout);
      }
      sleep(DESTROY_WAIT_SECONDS);

      /* output to experiment log */
      fprintf(fp,"%u\n",attack_count);
      fflush(fp);
      printf("guest dead %d times\n",ex_count);

      /* create new domain. we can assume that domain ID of new domain is same as dom_id*/
      printf("creating new domain\n");
      if(create_domain_with_xc()){
	printf("create domain failed.\n");
	fflush(stdout);
      }
      sleep(SLEEP_SECONDS);	/* wait until new domain finishes boot */
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
    return 1;
  }else{
    /* bit flip */
    digit = rand() % 8;
    memory[0] = memory[0] ^ (1UL << digit);
    munmap(memory,xai->page_size);
    return 0;
  }
}

int inject_byte(uint32_t addr,xa_instance_t *xai){
  uint32_t offset = 0;

  unsigned char *memory = xa_access_physical_address(xai,addr,&offset);
  if(memory == NULL){
    return 1;
  }else{
    memory[0] = ~memory[0];
    munmap(memory,xai->page_size);
    return 0;
  }
}

int inject_page(uint32_t addr,xa_instance_t *xai){
  uint32_t offset = 0;
  int i;

  unsigned char *memory = xa_access_physical_address(xai,addr,&offset);
  if(memory == NULL){
    return 1;
  }else{
    for(i = 0;i < xai->page_size ;i++){
       memory[i] = ~memory[i];
    }
    munmap(memory,xai->page_size);
    return 0;
  }
}

int is_guest_dead(xa_instance_t *xai){
  int sock,send_value,send_num;
  FILE *in,*out;
  char rbuf[BUFFERSIZE];
  char sbuf[BUFFERSIZE];

  if (xc_domain_getinfo(xai->xc_handle,xai->domain_id,1,&(xai->info)) != 1){
    return 1;
  }
  if((xai->info.crashed == 1) || (xai->info.dying == 1)){
    return 1;
  }
  
  /* try to heart beat */
  sock = tcp_connect(GUEST_IP_ADDR,GUEST_PORT);
  if(sock < 0){
    printf("connect to guest failed\n");
    return 2;
  }
  tcp_set_timeout(sock,3);
  if(tcp_open_stream(sock,&in,&out) < 0){
    printf("open stream failed\n");
    return 2;
  }

  send_value=rand();
  send_num=sprintf(sbuf,"%d",send_value);
  fprintf(out,"%d\n",send_value);
  if(fgets(rbuf,send_num + 1,in) <= 0){
    printf("disconnected\n");
    fclose(in);
    fclose(out);
    return 2;    
  }
  if(memcmp(sbuf,rbuf,send_num) != 0){
    printf("heart beat failed\n");
    fclose(in);
    fclose(out);
    return 2;
  }

  printf("heart beat succeeded[%s]\n",rbuf);
  fflush(stdout);
  fclose(in);
  fclose(out);

  return 0;
}

int cleanup(xa_instance_t *xai){
  return xa_destroy(xai);
}

int create_domain_with_xc(){
  int i;

  if ((i = fork()) == 0){
    execlp("xm","xm","create",GUEST_FILE_PATH,(char *) 0);
    fprintf(stderr,"execution of xm for create is failed.\n");
    fflush(stderr);
    exit(1);
  }else{
    if(i != -1){
      printf("wait until execution of xm for create finishes\n");
      wait((int *)0);
      return 0;
    }else{
      return 1;
    }
  }
}

int destroy_domain_with_xc(){
  int i;

  if ((i = fork()) == 0){
    execlp("xm","xm","destroy",GUEST_NAME,(char *) 0);
    fprintf(stderr,"execution of xm for destroy is failed.\n");
    fflush(stderr);
    exit(1);
  }else{
    if(i != -1){
      printf("wait until execution of xm for destroy finishes\n");
      wait((int *)0);
      return 0;
    }else{
      return 1;
    }
  }
}
