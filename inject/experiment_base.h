extern int init(uint32_t dom_id,xa_instance_t *xai);
extern int inject_byte(uint32_t addr,xa_instance_t *xai);
extern int inject_page(uint32_t addr,xa_instance_t *xai);
extern int is_guest_dead(xa_instance_t *xai);
extern int cleanup(xa_instance_t *xai);
extern int create_domain_with_xc();
extern int destroy_domain_with_xc(char *guest_name);
extern int restore_domain_with_xc(char *guest_name);

#define MEMORY_SIZE 0x20000000U
#define ATTACABLE_BYTES (MEMORY_SIZE - 0x20000U)
#define EX_COUNT 1
#define LOG_FILENAME_BASE "./experiment_"
#define SLEEP_SECONDS 15
/* #define GUEST_IP_ADDR "172.16.100.101" */
#define GUEST_PORT 8888
#define DESTROY_WAIT_SECONDS 2
#define GUEST_NAME "centos"
#define GUEST_FILE_PATH "/root/work/xen/centos_hvm/centos_hvm.hvm"
#define SNAPSHOT_BASE "/root/work/xen/centos_hvm/"

extern char ip_addr[30];
