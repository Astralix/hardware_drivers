#include <common.h>

#ifdef DW_UBOOT_NAND_SUPPORT

#include <command.h>
#include <asm/byteorder.h>
#include <asm/io.h>

#include <image.h>
#include <u-boot/zlib.h>

#include <jffs2/jffs2.h>
#include <nand.h>

//test kernel uImage integrity
extern int image_info (ulong addr);
extern int mtdparts_init( void );
extern int find_dev_and_part(const char *id, struct mtd_device **dev,
                             u8 *part_num, struct part_info **part);
int board_nand_init(struct nand_chip *nand)
{
	return 0;
}

static int set_mtdparts(int activeKernel, int activeRootfs);

  /*----------------------------------*
   *|  Safe boot from nand support   |*
   *----------------------------------*/

#define DW_BUFFER	(TEXT_BASE + 0x2000000)

#define UBOOT_NAME	"u-boot"
#define KERNEL1_NAME	"kernel"
#define KERNEL2_NAME	"kernel2"
#define JFFS2_NAME	"jffs2"

#define SAFE_MTDPARTS		"mtdparts=dwflash.0:512k@0x0(u-boot);dwnand:%dk@0x%x(kernel),%dk@0x%x(rootfs),%dk@0x%x(jffs2),%dk@0x%x(rootfs2)"

int do_dw_nand_safe_bootm ( cmd_tbl_t *pCmdTbl, int Flag, int Argc, char *pArgv[] );
int dw_nand_check_kernel(nand_info_t *nand, char *partName);
int dw_nand_check_rootfs(nand_info_t *nand, char *partName);
int rootfs_crc( uLong Address, char *pRootfsSize,char *pRootfsCrc);
int get_off_size(const char *part_name, ulong *off, ulong *size);
int dw_copy_nand_to_ram(nand_info_t *nand, ulong buffer, ulong off, ulong size);

static ulong kernelLoadAddress;

U_BOOT_CMD(
	nand_safe_bootm, 2, 0, do_dw_nand_safe_bootm,
	"nand_safe_bootm   - safe boot from nand flash\n",
	" <load_addr>      - verify kernel + rootfs, and load kernel to address load_addr\n"
);

int do_dw_nand_safe_bootm ( cmd_tbl_t *pCmdTbl, int flag, int argc, char *argv[] )
{
	char *pVerify, *pActive;
	int activeKernel, activeRootfs;

	nand_info_t *nand;
	int ret;

	nand = &nand_info[nand_curr_device];

	if (argc < 2 || argv[1] == NULL) {
		printf(" you must specify load address\n");
		return -1;
	}

	kernelLoadAddress = simple_strtoul(argv[1], NULL, 16);

	// Set 'activeRootfs' to be the first good rootfs
	// Check rootfs first so the kernel will be in RAM after the verification.
	pActive = getenv(DSPG_DW_NOR_ROOTFS_ACTIVE_ENV_NAME);
	pVerify = getenv(DSPG_DW_ENV_ROOTFS_VERIFY_NAME);
	if (pVerify != NULL && *pVerify != '1') {
		if (pActive != NULL && *pActive == '1') {
			printf("verify rootfs 1\n");
			if ((ret = dw_nand_check_rootfs(nand, DSPG_DW_ENV_ROOTFS_NAME)) != 0) {
				printf(" error: %d\n", ret);
				printf("verify rootfs 2\n");
				if ((ret = dw_nand_check_rootfs(nand, DSPG_DW_ENV_ROOTFS2_NAME)) != 0) {
					printf(" error: %d\n", ret);
					return ret;
				}
				else activeRootfs = 2;
			}
			else activeRootfs = 1;
		}
		else {
			printf("verify rootfs 2\n");
			if ((ret = dw_nand_check_rootfs(nand, DSPG_DW_ENV_ROOTFS2_NAME)) != 0) {
				printf(" error: %d\n", ret);
				printf("verify rootfs 1\n");
				if ((ret = dw_nand_check_rootfs(nand, DSPG_DW_ENV_ROOTFS_NAME)) != 0) {
					printf(" error: %d\n", ret);
					return ret;
				}
				else activeRootfs = 1;
			}
			else activeRootfs = 2;
		}
	}
	else {	// Do not check filesystem integrity (rootfs_verify = 1)
		if (pActive != NULL && *pActive == '1') activeRootfs = 1;
		else activeRootfs = 2;
	}

	// Set 'activeKernel' to be the first good kernel
	pActive = getenv(DSPG_DW_NOR_KEREL_ACTIVE_ENV_NAME);
	if (pActive != NULL && *pActive == '1') {
		printf("verify kernel 1\n");
		if ((ret = dw_nand_check_kernel(nand, KERNEL1_NAME)) != 0) {
			printf(" error: %d\n", ret);
			printf("verify kernel 2\n");
			if ((ret = dw_nand_check_kernel(nand, KERNEL2_NAME)) != 0) {
				printf(" error: %d\n", ret);
				return ret;
			}
			else {
				activeKernel = 2;
				setenv(DSPG_DW_NOR_KEREL_ACTIVE_ENV_NAME, "2");
				saveenv();
			}
		}
		else activeKernel = 1;
	}
	else {
		printf("verify kernel 2\n");
		if ((ret = dw_nand_check_kernel(nand, KERNEL2_NAME)) != 0) {
			printf(" error: %d\n", ret);
			printf("verify kernel 1\n");
			if ((ret = dw_nand_check_kernel(nand, KERNEL1_NAME)) != 0) {
				printf(" error: %d\n", ret);
				return ret;
			}
			else {
				activeKernel = 1;
				setenv(DSPG_DW_NOR_KEREL_ACTIVE_ENV_NAME, "1");
				saveenv();
			}
		}
		else activeKernel = 2;
	}
	
	// Note that right now the last checked kernel (the good kernel) is located in 'kernelLoadAddress'
	//printf("active kernel = %d\n", activeKernel);
	//printf("active rootfs = %d\n", activeRootfs);
	set_mtdparts(activeKernel, activeRootfs);

	return 0;
}

extern int dspg_dw_set_root_block( char *pRootfs );
/* Set the nand flash partitions (variable 'mtdparts'):
 * 
 *  0 - u-boot partition
 *  1 - non active kernel partition
 *  2 - active root filesystem partition
 *  3 - jffs2 data partition
 *  4 - non active root filesystem partition
 */
int set_mtdparts(int activeKernel, int activeRootfs)
{
/*
	char mtdparts_str[300];
	ulong kernel_off, kernel_size, rootfs_off, rootfs_size, rootfs2_off, rootfs2_size, jffs2_off, jffs2_size;

	if (activeKernel==1) {
		if (get_off_size(KERNEL2_NAME, &kernel_off, &kernel_size) != 0) return -1;
	}
	else if (get_off_size(KERNEL1_NAME, &kernel_off, &kernel_size) != 0) return -1;

	if (activeRootfs==1) {
		if (get_off_size(DSPG_DW_ENV_ROOTFS_NAME, &rootfs_off, &rootfs_size) != 0) return -1;
		if (get_off_size(DSPG_DW_ENV_ROOTFS2_NAME, &rootfs2_off, &rootfs2_size) != 0) return -1;
	}
	else {
		if (get_off_size(DSPG_DW_ENV_ROOTFS2_NAME, &rootfs_off, &rootfs_size) != 0) return -1;
		if (get_off_size(DSPG_DW_ENV_ROOTFS_NAME, &rootfs2_off, &rootfs2_size) != 0) return -1;
	}

	if (get_off_size(JFFS2_NAME, &jffs2_off, &jffs2_size) != 0) return -1;

	// Set mtdparts variable
	sprintf(mtdparts_str, SAFE_MTDPARTS, kernel_size/1024, kernel_off, rootfs_size/1024, rootfs_off, jffs2_size/1024, jffs2_off, rootfs2_size/1024, rootfs2_off);

	setenv(DSPG_DW_ENV_MTDPARTS_NAME, mtdparts_str);
*/
    if( 1 == activeRootfs )
    {
        dspg_dw_set_root_block(DSPG_DW_ENV_ROOTFS_NAME);
    }
    else
    {
        dspg_dw_set_root_block(DSPG_DW_ENV_ROOTFS2_NAME);
    }
    return 0;
}

/* dw_nand_check_kernel: Check the kernel for integrity
 *
 *	@nand		[in]	pointer to current nand device
 *	@partName	[in]	partition name to check
 *
 *	@return 	0 ok / -1 failure / -2 no such partition
 */
int dw_nand_check_kernel(nand_info_t *nand, char *partName)
{
	ulong off, size;
	char size_var[30];
	char *pTemp;

	if (get_off_size(partName, &off, &size) != 0) return -2;

	sprintf(size_var,"%s_size", partName);

	// If we can find the size of the kernel load only its size and not the whole partition.
	pTemp = getenv(size_var);
	if (pTemp != NULL) {
		size = simple_strtoul(pTemp, NULL, 16);
	}

	if (dw_copy_nand_to_ram(nand, kernelLoadAddress, off, size) != 0) return -1;
	
	return image_info(kernelLoadAddress);
}

/* dw_nand_check_rootfs: Check the rootfs for integrity
 *
 *	@nand		[in]	pointer to current nand device
 *	@partName	[in]	partition name to check
 *
 *	@return		0 ok / -1 failure / -2 no such partition
 */
int dw_nand_check_rootfs(nand_info_t *nand, char *partName)
{
	ulong off, size;
	char rootfs_size_var[20];
	char rootfs_crc_var[20];
	char *pTemp;

	if (get_off_size(partName, &off, &size) != 0) return -2;

	sprintf(rootfs_size_var,"%s_size", partName);
	sprintf(rootfs_crc_var,"%s_crc", partName);

	// If we can find the size of the rootfs load only its size and not the whole partition.
	pTemp = getenv(rootfs_size_var);
	if (pTemp != NULL) {
		size = simple_strtoul(pTemp, NULL, 16);
	}
	
	if (dw_copy_nand_to_ram(nand, DW_BUFFER, off, size) != 0) return -1;

	if (rootfs_crc(DW_BUFFER, rootfs_size_var, rootfs_crc_var) != 0) return -1;
	
        setenv(DSPG_DW_ENV_ROOTFS_VERIFY_NAME, "1");
        saveenv();

	return 0;
}

/* dw_copy_nand_to_ram:	Copy data from nand to RAM
 *
 * 	@nand		[in]	pointer to current nand device
 * 	@buffer		[in]	target offset in RAM
 * 	@off		[in]	source offset in nand
 * 	@size		[in]	size in bytes to copy
 *
 *	@return 	0 ok / -1 failure
 */
int dw_copy_nand_to_ram(nand_info_t *nand, ulong buffer, ulong off, ulong size)
{
	//nand_read_options_t opts;
	int ret;
	size_t length = size;

	/*memset(&opts, 0, sizeof(opts));
	opts.buffer	= (u_char*) buffer;
	opts.length	= size;
	opts.offset	= off;
	opts.quiet      = 1;*/
	
	//ret = nand_read_opts(nand, &opts);
	ret = nand_read_skip_bad(nand, off, &length, (u_char *)buffer);

	if (ret != 0) {
		printf("Error reading from address: 0x%lx, size: 0x%lx\n", off, size);
		return -1;
	}
	return 0;
}

///
///	@brief		Calculate CRC32 for specified rootfs start address
///
///	@Address        [in]	start address for crc calculation
///	@pRootfsSize    [in]	pointer to uboot env string containing rootfs size
///	@pRootfsCrc     [in]	pointer to uboot env string containing rootfs crc
///
///	@return		1 - main rootfs/2 - second rootfs/ -1 failure
///
int rootfs_crc( uLong Address, char *pRootfsSize,char *pRootfsCrc)
{
    char *pSize;
    char *pCRC;

    pSize = getenv (pRootfsSize);
    pCRC = getenv (pRootfsCrc);

    if(pSize != NULL && pCRC != NULL)
    {
        unsigned long Crc;
        unsigned int Size;

        Size = simple_strtoul(pSize,NULL,16);
        Crc = simple_strtoul(pCRC,NULL,16);

        if (crc32(0,(const uchar *)Address,Size) != Crc)
        {
            printf("WRONG CRC on %8lxlx rootfs\n",Address);
            return -1;
        }
    }

    return 0;
}

/* get_off_size: Get offset and size for the partition name
 * 
 * 	@part_name	[in]	partition name to check
 * 	@off		[out]	offset of the partition in the nand device
 * 	@size		[out]	size of the partition in the nand device
 *
 *	@return 	0 ok / -1 failure
 */
int get_off_size(const char *part_name, ulong *off, ulong *size)
{
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;

	if ((mtdparts_init() == 0) && (find_dev_and_part(part_name, &dev, &pnum, &part) == 0)) {
		if (dev->id->type != MTD_DEV_TYPE_NAND) {
			puts("not a NAND device\n");
			return -1;
		}
		*off = part->offset;
		*size = part->size;
		return 0;
	}
	return -1;
}
#endif
