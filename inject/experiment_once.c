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

/* for experiment */
#include "experiment_base.h"

int main(int argc, char **argv)
{
    int i,is_init,inject_bits,is_pause=FALSE,wait_before_check=0,is_dead=FALSE;
    xa_instance_t xai;
    uint32_t dom_id = atoi(argv[2]);
    uint32_t addr;
    unsigned int attack_count,rc;
    FILE *fp;
    char *key_buf;
    char guest_name[20],log_file_name[30];
    apr_hash_index_t *hi;

    apr_pool_t *mp;
    apr_hash_t *ht;
    
    if(!((argc == 6)||(argc == 5))){
      printf("usage: ./test_random_access.out <guest_name> <dom_id> <ip_addr> <inject_bits> <wait_before_check>\n");
      exit(0);
    }
    
    strcpy(guest_name,argv[1]);
    strcpy(ip_addr,argv[3]);
    printf("passed guest IP address is %s\n",ip_addr);
    
    inject_bits=atoi(argv[4]);
    if(argc == 6){
      is_pause=TRUE;
      wait_before_check=atoi(argv[5]);
    }

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
      goto create_guest_again;
    }
    
    if(is_pause){
      xc_domain_pause(xai.xc_handle,dom_id);
    }
      
    attack_count = 0;
    srand(dom_id);
    rc = 0;
    /* inject faults inject_bits times */
    while(attack_count < inject_bits){
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
      
      if(is_pause == FALSE){
	/* for simple algorithm */
	if((rc = is_guest_dead(&xai))){
	  printf("guest dead\n");
	  is_dead=TRUE;
	  break;
	}
      }
    }

 create_guest_again:
 re_destroy:

      /* for with_pause algorithm */
      if(is_pause == TRUE){
	/* for simple algorithm */
	xc_domain_unpause(xai.xc_handle,dom_id);
	sleep(wait_before_check);
	if((rc = is_guest_dead(&xai))){
	  is_dead=TRUE;
	}
      }    

/*     /\* destroy two times just in case*\/ */
/*     if(destroy_domain_with_xc(guest_name)){ */
/*       printf("destroy domain failed.\n"); */
/*       fflush(stdout); */
/*       goto create_guest_again; */
/*     } */
    if(destroy_domain_with_xc(guest_name)){
      printf("destroy domain failed.\n");
      fflush(stdout);
      goto create_guest_again;
    }
    sleep(DESTROY_WAIT_SECONDS);

    /* if sleep to wait finish of destroy was short */
    xc_domain_getinfo(xai.xc_handle,xai.domain_id,1,&(xai.info));
    if(xai.info.running == 1){
      goto re_destroy;
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
    /* create new domain. we can assume that domain ID of new domain is same as dom_id*/
    printf("resuming new domain\n");
    if(restore_domain_with_xc(guest_name)){
      printf("resume domain failed.\n");
      fflush(stdout);
      goto create_guest_again;
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
