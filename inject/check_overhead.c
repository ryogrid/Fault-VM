#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h> 		/* for execlp */
#include <xenaccess/xenaccess.h>
#include <xenaccess/xa_private.h>
#include <time.h> 		/* for gettimeofday etc.. */

#include "file_util.h"
#include "tcp_util.h"

/* for hash table */
#include <apr_general.h>
#include <apr_hash.h>
#include <apr_strings.h>

/* for experiment */
#include "experiment_base.h"

#define BIT_SLEEP_SEC 0.001

double get_sec()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + (double)tv.tv_usec*1e-6;
}

int main(int argc, char **argv)
{
    int i,is_init,inject_bits,is_pause=FALSE,bits_per_once,wait_sec_between_inject=0,is_dead=FALSE;
    xa_instance_t xai;
    uint32_t dom_id = atoi(argv[2]);
    uint32_t addr;
    unsigned int attack_count,rc;
    FILE *fp;
    char *key_buf;
    char guest_name[20],log_file_name[30];
    apr_hash_index_t *hi;
    double start_sec,now_sec,time_count;

    struct timespec to_nano;

    apr_pool_t *mp;
    apr_hash_t *ht;
    
    if(!(argc == 6)){
      printf("usage: ./check_overhead.out <experiment_count>\n");
      exit(0);
    }
    
    strcpy(guest_name,argv[1]);
    
    inject_bits=atoi(argv[3]);
    bits_per_once=atoi(argv[4]);
    wait_sec_between_inject=atoi(argv[5]);

    /* log file open */
    strcpy(log_file_name,LOG_FILENAME_BASE);
    strcat(log_file_name,guest_name);
    strcat(log_file_name,".log");
    fp = file_open(log_file_name);
    
    /* initialize for hash table */
    apr_initialize();

    /* create hash table */
    apr_pool_create(&mp, NULL);
    ht = apr_hash_make(mp);

    /* initialize */
    is_init = -1;
    printf("try to initialize %s[%d]\n",guest_name,dom_id);
    if((is_init = init(dom_id++, &xai)) == 0){
      printf("initialize failed\n");
      exit(1);
    }
      
    attack_count = 0;
    srand(dom_id);
    rc = 0;
    time_count=0.0;
    start_sec = get_sec();
    /* inject faults inject_bits times */
    while(attack_count < inject_bits){

      for(i=0;i<bits_per_once;i++){
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
      }

      attack_count+=bits_per_once;
      if((attack_count % 1000) == 0){
	printf("inject 0x%x as %utimes\n",addr,attack_count);
      }
	
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
      
      /* at least, sleep over wait_sec_between_inject sec */
      while(((now_sec = get_sec()) - start_sec) < time_count){
	to_nano.tv_sec = 0;
	to_nano.tv_nsec = 1000;
	nanosleep(&to_nano,NULL);
      }
      time_count += wait_sec_between_inject;
    }

    if(cleanup(&xai) == XA_FAILURE){
      printf("cleanup of XA failed\n");
    }

    /* output to experiment log */
    if(((is_init != 0)&&(rc != 0))||(is_dead == TRUE)){
      fprintf(fp,"%u\n",attack_count);
      fflush(fp);
      printf("guest dead\n");
    }

    /* cleanup for hash table */
    for (hi = apr_hash_first(NULL, ht); hi; hi = apr_hash_next(hi)) {
      const char *k;
      const char *v;

      apr_hash_this(hi, (const void**)&k, NULL, (void**)&v);
      apr_hash_set(ht,k,APR_HASH_KEY_STRING,NULL);
      free(k);
    }
    apr_pool_destroy(mp);   

    file_close(fp);
    apr_terminate();

    return 0;
}
