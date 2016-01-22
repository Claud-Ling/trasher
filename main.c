#include <err.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* OP-TEE TEE client API */
#include <tee_client_api.h>

/* To the the UUID */
#include <trasher_ta.h> 

#define READ_ADDR 0x7ff08000

static void read_memory(uint32_t addr)
{
	int   devmem;
	off_t PageOffset, PageAddress;
	const struct timespec ts = {0, 10};

	unsigned long *hw_addr = (unsigned long *)addr;

	devmem = open("/dev/mem", O_RDWR | O_SYNC);
	PageOffset = (off_t) hw_addr % getpagesize();
	PageAddress = (off_t) (hw_addr - PageOffset);
	printf("PageOffset: 0x%08lx\nPageAddress: 0x%08lx\n", PageOffset, PageAddress);
	sleep(3);

	hw_addr = (unsigned long *) mmap(0, 4, PROT_READ|PROT_WRITE, MAP_SHARED, devmem, PageAddress);

	while (1)
	{
		//*hw_addr = 0xbabebabe;
		//printf("Writing to %p : %p\n", hw_addr + PageOffset, *(hw_addr + PageOffset));
		printf("%p: 0x%08lx\n", hw_addr + PageOffset, *(hw_addr + PageOffset));
		nanosleep(&ts, NULL);
	}
}

static void *read_thread(void *arg)
{
	pthread_t id = pthread_self();
	printf("Thread with %d created\n", (int)id);
	read_memory(READ_ADDR);
	return NULL;
}

static void call_ta(void)
{
        TEEC_Result res;
        TEEC_Context ctx;
        TEEC_Session sess;
        TEEC_Operation op;
        TEEC_UUID uuid = TRASHER_TA_UUID;
        uint32_t err_origin;
	uint32_t buf = 0xcafebabe;

        /* Initialize a context connecting us to the TEE */
        res = TEEC_InitializeContext(NULL, &ctx);
        if (res != TEEC_SUCCESS)
                errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

        res = TEEC_OpenSession(&ctx, &sess, &uuid,
                               TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
        if (res != TEEC_SUCCESS)
                errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
                        res, err_origin);

        memset(&op, 0, sizeof(op));
        op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT,
                                         TEEC_NONE,
                                         TEEC_NONE,
                                         TEEC_NONE);

        op.params[0].tmpref.buffer = (void *)&buf;
        op.params[0].tmpref.size = sizeof(uint32_t);

        res = TEEC_InvokeCommand(&sess, TRASH, &op,
                                 &err_origin);
        if (res != TEEC_SUCCESS)
                errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
                        res, err_origin);

        /* We're done with the TA, close the session ... */
        TEEC_CloseSession(&sess);

        /* ... and destroy the context. */
        TEEC_FinalizeContext(&ctx);

        return;
}  

int main() {
#if 0
	pthread_t pt;
	int err;
	err = pthread_create(&pt, NULL, &read_thread, NULL);
	if (err)
		printf("\ncan't create thread :[%s]", (char *)strerror(err));
#endif

	while (1)
		call_ta();

	return 0;
}
