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

#define MEMORY_SIZE 0x20000000U
#define ATTACABLE_BYTES (MEMORY_SIZE - 0x20000U)
#define EX_COUNT 1
#define LOG_FILENAME_BASE "./experiment_"
#define SLEEP_SECONDS 15
/* #define GUEST_IP_ADDR "172.16.100.101" */
/* #define GUEST_PORT 8888 */ /* for centos*/
#define GUEST_PORT 12345 /* for winxp*/
#define DESTROY_WAIT_SECONDS 5
#define GUEST_NAME "centos"
#define GUEST_FILE_PATH "/root/work/xen/centos_hvm/centos_hvm.hvm"
#define SNAPSHOT_BASE "/var/xen/"

char ip_addr[30];

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
  printf("connect to %s\n",ip_addr);
  sock = tcp_connect(ip_addr,GUEST_PORT);
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

int destroy_domain_with_xc(char *guest_name){
  int i;

  if ((i = fork()) == 0){
    execlp("xm","xm","destroy",guest_name,(char *) 0);
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

int restore_domain_with_xc(char *guest_name){
  int i;
  char snapshot_path[50];

  strcpy(snapshot_path,SNAPSHOT_BASE);
  strcat(snapshot_path,guest_name);
  strcat(snapshot_path,".snapshot");
  printf("try to restore %s\n",snapshot_path);

  if ((i = fork()) == 0){
    execlp("xm","xm","restore",snapshot_path,(char *) 0);
    fprintf(stderr,"execution of xm for resume is failed.\n");
    fflush(stderr);
    exit(1);
  }else{
    if(i != -1){
      printf("wait until execution of xm for restore finishes\n");
      wait((int *)0);
      return 0;
    }else{
      return 1;
    }
  }
}
