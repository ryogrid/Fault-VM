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
#define INJECT_BITS_END 4000
#define BITS_SLICE 200
#define EX_COUNT 1
#define LOG_FILENAME "./experiment.log"
#define SLEEP_SECONDS 50
#define GUEST_IP_ADDR "172.16.100.101"
#define GUEST_PORT 8888
#define DESTROY_WAIT_SECONDS 2
#define WAIT_TO_ALIVE_CHECK_SECONDS 5
#define GUEST_NAME "centos"
#define GUEST_FILE_PATH "/root/work/xen/centos_hvm/centos_hvm.hvm"
#define SNAPSHOT_PATH "/root/work/xen/centos_hvm/save.snapshot"


int init(uint32_t dom_id,xa_instance_t *xai);
int inject_byte(uint32_t addr,xa_instance_t *xai);
int inject_page(uint32_t addr,xa_instance_t *xai);
int is_guest_dead(xa_instance_t *xai);
int cleanup(xa_instance_t *xai);
int create_domain_with_xc();
int destroy_domain_with_xc();
int restore_domain_with_xc();

int main(int argc, char **argv)
{
    int i,is_init;
    xa_instance_t xai;
    uint32_t dom_id = atoi(argv[1]);
    uint32_t addr;
    int current_bits = 0,injected_bits,attack_count,rc,ex_count;
    FILE *fp;
    char *key_buf;
    apr_hash_index_t *hi;

    apr_pool_t *mp;
    apr_hash_t *ht;
    
    /* file open */
    fp = file_open(LOG_FILENAME);
    
    /* initialize for hash table */
    apr_initialize();
    
    /* do experiment that injects some bits each time decided */
    for(current_bits = BITS_SLICE*1;current_bits < INJECT_BITS_END;current_bits+=BITS_SLICE){
      /* inject faults EX_COUNT times per BITS_SLICE period */
      for(ex_count=0;ex_count < EX_COUNT;ex_count++){
	/* create hash table */
	apr_pool_create(&mp, NULL);
	ht = apr_hash_make(mp);

	/* initialize */
	is_init = -1;
	if((is_init = init(dom_id++, &xai)) == 0){
	  printf("initialize failed\n");
	  ex_count--;
	  goto create_guest_again;
	}
      
	attack_count = 0;
	srand(dom_id);
	rc = 0;
      
	/* pause domain */
	xc_domain_pause(xai.xc_handle,dom_id-1);

	for(injected_bits=0;injected_bits < current_bits;injected_bits++){
	  addr = rand() % MEMORY_SIZE;
	
	  /* addr hit VGA area*/
	  if((addr >= 0xa0000) && (addr < 0xc0000)){
	    printf("VGA region passed\n");
	    continue;		/* skip */
	  }

	  if(inject_bit(addr,&xai)){
	    printf("inject failed. so go out loop\n");
	    break;
	  }
	  attack_count++;
/* 	  if((attack_count % 1000) == 0){ */
	    printf("inject 0x%x as %utimes\n",addr,attack_count);
/* 	  } */
	
	  /* check if current address had already attaced */
	  key_buf = malloc(sizeof(char)*9); /* 8 charactors and a NULL charactor */
	  /* convert to hex str */
	  sprintf(key_buf,"%0.8x",addr);
	  if((apr_hash_get(ht,key_buf,APR_HASH_KEY_STRING))!=NULL){
	    printf("same address(%0.8x) passed\n",addr);
	    continue;		/* skip */
	  }else{
	    apr_hash_set(ht,key_buf,APR_HASH_KEY_STRING,key_buf);
	  }
	}
	
	/* unpause guest */
	xc_domain_unpause(xai.xc_handle,dom_id-1);
	sleep(WAIT_TO_ALIVE_CHECK_SECONDS);

      create_guest_again:
	if((rc = is_guest_dead(&xai))){
	  break;
	  
	  /* output to experiment log as guest dead*/
	  if((is_init != 0)){
	    fprintf(fp,"%u\n",attack_count);
	    fflush(fp);
	    printf("guest dead %d times\n",ex_count);
	  }
	}

	if(cleanup(&xai) == XA_FAILURE){
	  printf("cleanup of XA failed\n");
	}
	if(destroy_domain_with_xc()){
	  printf("destroy domain failed.\n");
	  fflush(stdout);
	  goto create_guest_again;
	}
	sleep(DESTROY_WAIT_SECONDS);

	/* create new domain. we can assume that domain ID of new domain is same as dom_id*/
	printf("creating new domain\n");
	if(restore_domain_with_xc()){
	  printf("create domain failed.\n");
	  fflush(stdout);
	  goto create_guest_again;
	}
	sleep(SLEEP_SECONDS);	/* wait until new domain finishes boot */
	ex_count++;

	/* cleanup for hash table */
	for (hi = apr_hash_first(NULL, ht); hi; hi = apr_hash_next(hi)) {
	  const char *k;
	  const char *v;

	  apr_hash_this(hi, (const void**)&k, NULL, (void**)&v);
	  apr_hash_set(ht,k,APR_HASH_KEY_STRING,NULL);
	  free(k);
	}
	apr_pool_destroy(mp);

      }
    }

    file_close(fp);
    return 0;

    apr_terminate();
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

int restore_domain_with_xc(){
  int i;
  char snapshot_path[50];

  if ((i = fork()) == 0){
    execlp("xm","xm","restore",SNAPSHOT_PATH,(char *) 0);
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
