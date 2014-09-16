#include <ppapi.h>
#include <dwl.h>

#define ALIGN_CROP_SIZE		8
#define ALIGN_CROP_START	16

class PostProcessor
{
	PPInst ppInst;
//	int memallocFd;
//	int memFd;
	PPConfig ppConf;
	DWLLinearMem_t inputBuffer;
	DWLLinearMem_t outputBuffer;
	bool hasSettings;

public:
	PostProcessor();
	~PostProcessor();

	int init();

	/* allocInputFrame
	 *
	 * allocate input frame buffer (if needed) and set it as the input buffer
	 * of the post-processor (will call setInputFrame).
	 */ 
	int allocInputFrame(unsigned int format, unsigned int width, unsigned int height);


	/* allocOuputFrame
	 *
	 * allocate output frame buffer (if needed) and set it as the output buffer
	 * of the post-processor (will call setInputFrame).
	 */ 
	int allocOutputFrame(unsigned int format, unsigned int width, unsigned int height);

	// these functions will set the input and output without allocating the buffers
	int setInputFrame(unsigned int inputFrameBusAddr, unsigned int format, unsigned int width, unsigned int height);
	int setOutputFrame(unsigned int outputFrameBusAddr, unsigned int format, unsigned int width, unsigned int height);

    // apply transformations
    int setHorizontalFlip(int doFlip);
    int setVerticalFlip(int doFlip);

	void* getInputFrameVirtAddr();
	void* getOutputFrameVirtAddr();
	uint32_t getOutputFrameBusAddr();

	int commitSettings();

	int execute();

private:
	int setInputCropParamsForFill();

	int initMemAlloc();
	int allocBuffer(DWLLinearMem_t *params, unsigned int size);
	int freeBuffer(DWLLinearMem_t *params);
};

