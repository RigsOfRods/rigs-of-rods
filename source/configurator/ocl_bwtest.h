/*
 * Copyright 1993-2010 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.
 * Any use, reproduction, disclosure, or distribution of this software
 * and related documentation without an express license agreement from
 * NVIDIA Corporation is strictly prohibited.
 *
 * Please refer to the applicable NVIDIA end user license agreement (EULA)
 * associated with this source code for terms and conditions that govern
 * your use of this NVIDIA software.
 *
 */

// *********************************************************************
//
// *********************************************************************

// standard utilities and systems includes
#include <oclUtils.h>

// defines, project
#define MEMCOPY_ITERATIONS  100
#define DEFAULT_SIZE        ( 32 * ( 1 << 20 ) )    //32 M
#define DEFAULT_INCREMENT   (1 << 22)               //4 M
#define CACHE_CLEAR_SIZE    (1 << 24)               //16 M

//shmoo mode defines
#define SHMOO_MEMSIZE_MAX     (1 << 26)         //64 M
#define SHMOO_MEMSIZE_START   (1 << 10)         //1 KB
#define SHMOO_INCREMENT_1KB   (1 << 10)         //1 KB
#define SHMOO_INCREMENT_2KB   (1 << 11)         //2 KB
#define SHMOO_INCREMENT_10KB  (10 * (1 << 10))  //10KB
#define SHMOO_INCREMENT_100KB (100 * (1 << 10)) //100 KB
#define SHMOO_INCREMENT_1MB   (1 << 20)         //1 MB
#define SHMOO_INCREMENT_2MB   (1 << 21)         //2 MB
#define SHMOO_INCREMENT_4MB   (1 << 22)         //4 MB
#define SHMOO_LIMIT_20KB      (20 * (1 << 10))  //20 KB
#define SHMOO_LIMIT_50KB      (50 * (1 << 10))  //50 KB
#define SHMOO_LIMIT_100KB     (100 * (1 << 10)) //100 KB
#define SHMOO_LIMIT_1MB       (1 << 20)         //1 MB
#define SHMOO_LIMIT_16MB      (1 << 24)         //16 MB
#define SHMOO_LIMIT_32MB      (1 << 25)         //32 MB

//enums, project
enum testMode { QUICK_MODE, RANGE_MODE, SHMOO_MODE };
enum memcpyKind { DEVICE_TO_HOST, HOST_TO_DEVICE, DEVICE_TO_DEVICE };
enum memoryMode { PAGEABLE, PINNED };
enum accessMode { MAPPED, DIRECT };

#include <iostream>
using namespace std;

class OpenCLTestBandwidth
{
public:
	OpenCLTestBandwidth(std::ostream &stream);
	~OpenCLTestBandwidth();

protected:
	std::ostream &stream;

	// CL objects
	cl_context cxGPUContext;
	cl_command_queue cqCommandQueue;
	cl_device_id *devices;

	////////////////////////////////////////////////////////////////////////////////
	// declaration, forward
	int runTest(memoryMode memMode = PAGEABLE, accessMode accMode = DIRECT, testMode mode = QUICK_MODE);
	void createQueue(unsigned int device);
	void testBandwidth( unsigned int start, unsigned int end, unsigned int increment,
						testMode mode, memcpyKind kind, accessMode accMode, memoryMode memMode, int startDevice, int endDevice);
	void testBandwidthQuick(unsigned int size, memcpyKind kind, accessMode accMode, memoryMode memMode, int startDevice, int endDevice);
	void testBandwidthRange(unsigned int start, unsigned int end, unsigned int increment,
							memcpyKind kind, accessMode accMode, memoryMode memMode, int startDevice, int endDevice);
	void testBandwidthShmoo(memcpyKind kind, accessMode accMode,  memoryMode memMode, int startDevice, int endDevice);
	double testDeviceToHostTransfer(unsigned int memSize, accessMode accMode, memoryMode memMode);
	double testHostToDeviceTransfer(unsigned int memSize, accessMode accMode, memoryMode memMode);
	double testDeviceToDeviceTransfer(unsigned int memSize);
	void printResultsReadable(unsigned int *memSizes, double* bandwidths, unsigned int count, memcpyKind kind, accessMode accMode, memoryMode memMode, int iNumDevs);

	void ownCheckError(int sample, int ref);
};

// class implementation

OpenCLTestBandwidth::OpenCLTestBandwidth(std::ostream &_stream) : stream(_stream), cqCommandQueue(0), devices(0)
{
    stream << "Starting, this can take a while ..." << endl;
	stream.flush();

    // run the main test
    int iRetVal = runTest();

	stream << endl;

	if(iRetVal == 0)
		stream << "=== PASSED ===" << endl;
	else
		stream << "=== FAILED ===" << endl;
}

OpenCLTestBandwidth::~OpenCLTestBandwidth()
{
}

void OpenCLTestBandwidth::ownCheckError(int sample, int ref)
{
    if(sample != ref)
		stream << " ERROR " << endl;
}

int OpenCLTestBandwidth::runTest(memoryMode memMode, accessMode accMode, testMode mode)
{
    int start = DEFAULT_SIZE;
    int end = DEFAULT_SIZE;
    int startDevice = 0;
    int endDevice = 0;
    int increment = DEFAULT_INCREMENT;
    bool htod = false;
    bool dtoh = false;
    bool dtod = false;

    // Get OpenCL platform ID for NVIDIA if available, otherwise default
    cl_platform_id clSelectedPlatformID = NULL;
    cl_int ciErrNum = oclGetPlatformID (&clSelectedPlatformID);
    ownCheckError(ciErrNum, CL_SUCCESS);

    // Find out how many devices there are
    cl_uint ciDeviceCount;
    ciErrNum = clGetDeviceIDs (clSelectedPlatformID, CL_DEVICE_TYPE_GPU, 0, NULL, &ciDeviceCount);
    if (ciErrNum != CL_SUCCESS)
    {
		stream << "Error in clGetDeviceIDs call: " << ciErrNum << endl;
        return ciErrNum;
    }
    else if (ciDeviceCount == 0)
    {
        stream << "There are no devices supporting OpenCL return code" << ciErrNum << endl;
        return ciErrNum;
    }

	// use all devices
	startDevice = 0;
	endDevice = (int)(ciDeviceCount-1);


    // Get and log the device info
    devices = (cl_device_id*) malloc(sizeof(cl_device_id) * ciDeviceCount);
    ciErrNum = clGetDeviceIDs (clSelectedPlatformID, CL_DEVICE_TYPE_GPU, ciDeviceCount, devices, &ciDeviceCount);
    for(int currentDevice = startDevice; currentDevice <= endDevice; currentDevice++)
    {
		char device_string[1024];
		clGetDeviceInfo(devices[currentDevice], CL_DEVICE_NAME, sizeof(device_string), &device_string, NULL);
        stream << "Running on " << device_string << endl << endl;
    }


	//default:  All tests
	htod = true;
	dtoh = true;
	dtod = true;


    // Create the OpenCL context
    cxGPUContext = clCreateContext(0, ciDeviceCount, devices, NULL, NULL, NULL);
    if (cxGPUContext == (cl_context)0)
    {
        stream << "Failed to create OpenCL context!" << endl;
        return -11000;
    }

    // Run tests
    if(htod)
    {
        testBandwidth((unsigned int)start, (unsigned int)end, (unsigned int)increment,
                      mode, HOST_TO_DEVICE, accMode, memMode, startDevice, endDevice);
    }

	stream << endl;

    if(dtoh)
    {
        testBandwidth((unsigned int)start, (unsigned int)end, (unsigned int)increment,
                      mode, DEVICE_TO_HOST, accMode, memMode, startDevice, endDevice);
    }

	stream << endl;

    if(dtod)
    {
        testBandwidth((unsigned int)start, (unsigned int)end, (unsigned int)increment,
                      mode, DEVICE_TO_DEVICE, accMode, memMode, startDevice, endDevice);
    }

	stream << endl;

	// Clean up
    if(cqCommandQueue)clReleaseCommandQueue(cqCommandQueue);
    if(cxGPUContext)clReleaseContext(cxGPUContext);
    if(devices)free(devices);

    return 0;
}
///////////////////////////////////////////////////////////////////////////////
//  Create command queue for the selected device
///////////////////////////////////////////////////////////////////////////////
void OpenCLTestBandwidth::createQueue(unsigned int device)
{
    // Release if there previous is already one
    if(cqCommandQueue)
    {
        clReleaseCommandQueue(cqCommandQueue);
    }

    cqCommandQueue = clCreateCommandQueue(cxGPUContext, devices[device], CL_QUEUE_PROFILING_ENABLE, NULL);
}

///////////////////////////////////////////////////////////////////////////////
//  Run a bandwidth test
///////////////////////////////////////////////////////////////////////////////
void OpenCLTestBandwidth::testBandwidth(unsigned int start, unsigned int end, unsigned int increment,
              testMode mode, memcpyKind kind, accessMode accMode,
              memoryMode memMode, int startDevice, int endDevice)
{
    switch(mode)
    {
    case QUICK_MODE:
        testBandwidthQuick( DEFAULT_SIZE, kind, accMode, memMode, startDevice, endDevice);
        break;
    case RANGE_MODE:
        testBandwidthRange(start, end, increment, kind, accMode, memMode, startDevice, endDevice);
        break;
    case SHMOO_MODE:
        testBandwidthShmoo(kind, accMode, memMode, startDevice, endDevice);
        break;
    default:
        break;
    }

}
//////////////////////////////////////////////////////////////////////
//  Run a quick mode bandwidth test
//////////////////////////////////////////////////////////////////////
void OpenCLTestBandwidth::testBandwidthQuick(unsigned int size, memcpyKind kind, accessMode accMode,
                   memoryMode memMode, int startDevice, int endDevice)
{
    testBandwidthRange(size, size, DEFAULT_INCREMENT, kind, accMode, memMode, startDevice, endDevice);
}

///////////////////////////////////////////////////////////////////////
//  Run a range mode bandwidth test
//////////////////////////////////////////////////////////////////////
void OpenCLTestBandwidth::testBandwidthRange(unsigned int start, unsigned int end, unsigned int increment,
                   memcpyKind kind, accessMode accMode, memoryMode memMode, int startDevice, int endDevice)
{
    //count the number of copies we're going to run
    unsigned int count = 1 + ((end - start) / increment);

    unsigned int * memSizes = (unsigned int *)malloc(count * sizeof( unsigned int ));
    double* bandwidths = (double*)malloc(count * sizeof(double));

    // Before calculating the cumulative bandwidth, initialize bandwidths array to NULL
    for (unsigned int i = 0; i < count; i++)
        bandwidths[i] = 0.0;

    // Use the device asked by the user
    for (int currentDevice = startDevice; currentDevice <= endDevice; currentDevice++)
    {
        // Allocate command queue for the device (dealloc first if already allocated)
        createQueue(currentDevice);

        //run each of the copies
        for(unsigned int i = 0; i < count; i++)
        {
            memSizes[i] = start + i * increment;
            switch(kind)
            {
            case DEVICE_TO_HOST:    bandwidths[i] += testDeviceToHostTransfer(memSizes[i], accMode, memMode);
                break;
            case HOST_TO_DEVICE:    bandwidths[i] += testHostToDeviceTransfer(memSizes[i], accMode, memMode);
                break;
            case DEVICE_TO_DEVICE:  bandwidths[i] += testDeviceToDeviceTransfer(memSizes[i]);
                break;
            }
        }
    } // Complete the bandwidth computation on all the devices

    //print results
	printResultsReadable(memSizes, bandwidths, count, kind, accMode, memMode, (1 + endDevice - startDevice));

    //clean up
    free(memSizes);
    free(bandwidths);
}

//////////////////////////////////////////////////////////////////////////////
// Intense shmoo mode - covers a large range of values with varying increments
//////////////////////////////////////////////////////////////////////////////
void OpenCLTestBandwidth::testBandwidthShmoo(memcpyKind kind, accessMode accMode,
                   memoryMode memMode, int startDevice, int endDevice)
{
    //count the number of copies to make
    unsigned int count = 1 + (SHMOO_LIMIT_20KB  / SHMOO_INCREMENT_1KB)
        + ((SHMOO_LIMIT_50KB - SHMOO_LIMIT_20KB) / SHMOO_INCREMENT_2KB)
        + ((SHMOO_LIMIT_100KB - SHMOO_LIMIT_50KB) / SHMOO_INCREMENT_10KB)
        + ((SHMOO_LIMIT_1MB - SHMOO_LIMIT_100KB) / SHMOO_INCREMENT_100KB)
        + ((SHMOO_LIMIT_16MB - SHMOO_LIMIT_1MB) / SHMOO_INCREMENT_1MB)
        + ((SHMOO_LIMIT_32MB - SHMOO_LIMIT_16MB) / SHMOO_INCREMENT_2MB)
        + ((SHMOO_MEMSIZE_MAX - SHMOO_LIMIT_32MB) / SHMOO_INCREMENT_4MB);

    unsigned int *memSizes = (unsigned int *)malloc(count * sizeof(unsigned int));
    double* bandwidths = (double*)malloc(count * sizeof(double));

    // Before calculating the cumulative bandwidth, initialize bandwidths array to NULL
    for (unsigned int i = 0; i < count; i++)
        bandwidths[i] = 0.0;

    // Use the device asked by the user
    for (int currentDevice = startDevice; currentDevice <= endDevice; currentDevice++)
    {
        // Allocate command queue for the device (dealloc first if already allocated)
        createQueue(currentDevice);

        //Run the shmoo
        int iteration = 0;
        unsigned int memSize = 0;
        while(memSize <= SHMOO_MEMSIZE_MAX )
        {
            if(memSize < SHMOO_LIMIT_20KB )
            {
                memSize += SHMOO_INCREMENT_1KB;
            }
            else if( memSize < SHMOO_LIMIT_50KB)
            {
                memSize += SHMOO_INCREMENT_2KB;
            }
            else if( memSize < SHMOO_LIMIT_100KB)
            {
                memSize += SHMOO_INCREMENT_10KB;
            }
            else if( memSize < SHMOO_LIMIT_1MB)
            {
                memSize += SHMOO_INCREMENT_100KB;
            }
            else if( memSize < SHMOO_LIMIT_16MB)
            {
                memSize += SHMOO_INCREMENT_1MB;
            }
            else if( memSize < SHMOO_LIMIT_32MB)
            {
                memSize += SHMOO_INCREMENT_2MB;
            }
            else
            {
                memSize += SHMOO_INCREMENT_4MB;
            }

            memSizes[iteration] = memSize;
            switch(kind)
            {
            case DEVICE_TO_HOST:    bandwidths[iteration] += testDeviceToHostTransfer(memSizes[iteration], accMode, memMode);
                break;
            case HOST_TO_DEVICE:    bandwidths[iteration] += testHostToDeviceTransfer(memSizes[iteration], accMode, memMode);
                break;
            case DEVICE_TO_DEVICE:  bandwidths[iteration] += testDeviceToDeviceTransfer(memSizes[iteration]);
                break;
            }
            iteration++;
            stream << ".";
        }
    } // Complete the bandwidth computation on all the devices

    //print results
    stream << endl;
	printResultsReadable(memSizes, bandwidths, count, kind, accMode, memMode, (endDevice - startDevice));

    //clean up
    free(memSizes);
    free(bandwidths);
}

///////////////////////////////////////////////////////////////////////////////
//  test the bandwidth of a device to host memcopy of a specific size
///////////////////////////////////////////////////////////////////////////////
double OpenCLTestBandwidth::testDeviceToHostTransfer(unsigned int memSize, accessMode accMode, memoryMode memMode)
{
    unsigned int timer = 0;
    double elapsedTimeInSec = 0.0;
    double bandwidthInMBs = 0.0;
    unsigned char *h_data = NULL;
    cl_mem cmPinnedData = NULL;
    cl_mem cmDevData = NULL;
    cl_int ciErrNum = CL_SUCCESS;

    //allocate and init host memory, pinned or conventional
    if(memMode == PINNED)
    {
        // Create a host buffer
        cmPinnedData = clCreateBuffer(cxGPUContext, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, memSize, NULL, &ciErrNum);
        ownCheckError(ciErrNum, CL_SUCCESS);

        // Get a mapped pointer
        h_data = (unsigned char*)clEnqueueMapBuffer(cqCommandQueue, cmPinnedData, CL_TRUE, CL_MAP_WRITE, 0, memSize, 0, NULL, NULL, &ciErrNum);
        ownCheckError(ciErrNum, CL_SUCCESS);

        //initialize
        for(unsigned int i = 0; i < memSize/sizeof(unsigned char); i++)
        {
            h_data[i] = (unsigned char)(i & 0xff);
        }

        // unmap and make data in the host buffer valid
        ciErrNum = clEnqueueUnmapMemObject(cqCommandQueue, cmPinnedData, (void*)h_data, 0, NULL, NULL);
        ownCheckError(ciErrNum, CL_SUCCESS);
    }
    else
    {
        // standard host alloc
        h_data = (unsigned char *)malloc(memSize);

        //initialize
        for(unsigned int i = 0; i < memSize/sizeof(unsigned char); i++)
        {
            h_data[i] = (unsigned char)(i & 0xff);
        }
    }

    // allocate device memory
    cmDevData = clCreateBuffer(cxGPUContext, CL_MEM_READ_WRITE, memSize, NULL, &ciErrNum);
    ownCheckError(ciErrNum, CL_SUCCESS);

    // initialize device memory
    if(memMode == PINNED)
    {
	    // Get a mapped pointer
        h_data = (unsigned char*)clEnqueueMapBuffer(cqCommandQueue, cmPinnedData, CL_TRUE, CL_MAP_WRITE, 0, memSize, 0, NULL, NULL, &ciErrNum);	

        ciErrNum = clEnqueueWriteBuffer(cqCommandQueue, cmDevData, CL_FALSE, 0, memSize, h_data, 0, NULL, NULL);
        ownCheckError(ciErrNum, CL_SUCCESS);
    }
    else
    {
        ciErrNum = clEnqueueWriteBuffer(cqCommandQueue, cmDevData, CL_FALSE, 0, memSize, h_data, 0, NULL, NULL);
        ownCheckError(ciErrNum, CL_SUCCESS);
    }
    ownCheckError(ciErrNum, CL_SUCCESS);

    // Sync queue to host, start timer 0, and copy data from GPU to Host
    ciErrNum = clFinish(cqCommandQueue);
    shrDeltaT(0);
    if(accMode == DIRECT)
    {
        // DIRECT:  API access to device buffer
        for(unsigned int i = 0; i < MEMCOPY_ITERATIONS; i++)
        {
            ciErrNum = clEnqueueReadBuffer(cqCommandQueue, cmDevData, CL_FALSE, 0, memSize, h_data, 0, NULL, NULL);
            ownCheckError(ciErrNum, CL_SUCCESS);
        }
        ciErrNum = clFinish(cqCommandQueue);
        ownCheckError(ciErrNum, CL_SUCCESS);
    }
    else
    {
        // MAPPED: mapped pointers to device buffer for conventional pointer access
        void* dm_idata = clEnqueueMapBuffer(cqCommandQueue, cmDevData, CL_TRUE, CL_MAP_WRITE, 0, memSize, 0, NULL, NULL, &ciErrNum);
        ownCheckError(ciErrNum, CL_SUCCESS);
        for(unsigned int i = 0; i < MEMCOPY_ITERATIONS; i++)
        {
            memcpy(h_data, dm_idata, memSize);
        }
        ciErrNum = clEnqueueUnmapMemObject(cqCommandQueue, cmDevData, dm_idata, 0, NULL, NULL);
        ownCheckError(ciErrNum, CL_SUCCESS);
    }

    //get the the elapsed time in seconds
    elapsedTimeInSec = shrDeltaT(0);

    //calculate bandwidth in MB/s
    bandwidthInMBs = ((double)memSize * (double)MEMCOPY_ITERATIONS) / (elapsedTimeInSec * (double)(1 << 20));

    //clean up memory
    if(cmDevData)clReleaseMemObject(cmDevData);
    if(cmPinnedData)
    {
	    clEnqueueUnmapMemObject(cqCommandQueue, cmPinnedData, (void*)h_data, 0, NULL, NULL);	
	    clReleaseMemObject(cmPinnedData);	
    }
    h_data = NULL;

    return bandwidthInMBs;
}
///////////////////////////////////////////////////////////////////////////////
//  test the bandwidth of a device to host memcopy of a specific size
///////////////////////////////////////////////////////////////////////////////
double OpenCLTestBandwidth::testHostToDeviceTransfer(unsigned int memSize, accessMode accMode, memoryMode memMode)
{
    unsigned int timer = 0;
    double elapsedTimeInSec = 0.0;
    double bandwidthInMBs = 0.0;
    unsigned char* h_data = NULL;
    cl_mem cmPinnedData = NULL;
    cl_mem cmDevData = NULL;
    cl_int ciErrNum = CL_SUCCESS;

    // Allocate and init host memory, pinned or conventional
    if(memMode == PINNED)
    {
        // Create a host buffer
        cmPinnedData = clCreateBuffer(cxGPUContext, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, memSize, NULL, &ciErrNum);
        ownCheckError(ciErrNum, CL_SUCCESS);

        // Get a mapped pointer
        h_data = (unsigned char*)clEnqueueMapBuffer(cqCommandQueue, cmPinnedData, CL_TRUE, CL_MAP_WRITE, 0, memSize, 0, NULL, NULL, &ciErrNum);
        ownCheckError(ciErrNum, CL_SUCCESS);

        //initialize
        for(unsigned int i = 0; i < memSize/sizeof(unsigned char); i++)
        {
            h_data[i] = (unsigned char)(i & 0xff);
        }
	
        // unmap and make data in the host buffer valid
        ciErrNum = clEnqueueUnmapMemObject(cqCommandQueue, cmPinnedData, (void*)h_data, 0, NULL, NULL);
        ownCheckError(ciErrNum, CL_SUCCESS);
    }
    else
    {
        // standard host alloc
        h_data = (unsigned char *)malloc(memSize);

        //initialize
        for(unsigned int i = 0; i < memSize/sizeof(unsigned char); i++)
        {
            h_data[i] = (unsigned char)(i & 0xff);
        }
    }

    // allocate device memory
    cmDevData = clCreateBuffer(cxGPUContext, CL_MEM_READ_WRITE, memSize, NULL, &ciErrNum);
    ownCheckError(ciErrNum, CL_SUCCESS);

    // Sync queue to host, start timer 0, and copy data from Host to GPU
    clFinish(cqCommandQueue);
    shrDeltaT(0);
    if(accMode == DIRECT)
    {
	    if(memMode == PINNED)
        {
            // Get a mapped pointer
            h_data = (unsigned char*)clEnqueueMapBuffer(cqCommandQueue, cmPinnedData, CL_TRUE, CL_MAP_WRITE, 0, memSize, 0, NULL, NULL, &ciErrNum);
            ownCheckError(ciErrNum, CL_SUCCESS);
	    }

        // DIRECT:  API access to device buffer
        for(unsigned int i = 0; i < MEMCOPY_ITERATIONS; i++)
        {
                ciErrNum = clEnqueueWriteBuffer(cqCommandQueue, cmDevData, CL_FALSE, 0, memSize, h_data, 0, NULL, NULL);
                ownCheckError(ciErrNum, CL_SUCCESS);
        }
        ciErrNum = clFinish(cqCommandQueue);
        ownCheckError(ciErrNum, CL_SUCCESS);
    }
    else
    {
        // MAPPED: mapped pointers to device buffer and conventional pointer access
        void* dm_idata = clEnqueueMapBuffer(cqCommandQueue, cmDevData, CL_TRUE, CL_MAP_READ, 0, memSize, 0, NULL, NULL, &ciErrNum);
        for(unsigned int i = 0; i < MEMCOPY_ITERATIONS; i++)
        {
            memcpy(dm_idata, h_data, memSize);
        }
        ciErrNum = clEnqueueUnmapMemObject(cqCommandQueue, cmDevData, dm_idata, 0, NULL, NULL);
        ownCheckError(ciErrNum, CL_SUCCESS);
    }

    //get the the elapsed time in seconds
    elapsedTimeInSec = shrDeltaT(0);

    //calculate bandwidth in MB/s
    bandwidthInMBs = ((double)memSize * (double)MEMCOPY_ITERATIONS)/(elapsedTimeInSec * (double)(1 << 20));

    //clean up memory
    if(cmDevData)clReleaseMemObject(cmDevData);
    if(cmPinnedData)
    {
	    clEnqueueUnmapMemObject(cqCommandQueue, cmPinnedData, (void*)h_data, 0, NULL, NULL);
	    clReleaseMemObject(cmPinnedData);
    }
    h_data = NULL;

    return bandwidthInMBs;
}
///////////////////////////////////////////////////////////////////////////////
//  test the bandwidth of a device to host memcopy of a specific size
///////////////////////////////////////////////////////////////////////////////
double OpenCLTestBandwidth::testDeviceToDeviceTransfer(unsigned int memSize)
{
    unsigned int timer = 0;
    double elapsedTimeInSec = 0.0;
    double bandwidthInMBs = 0.0;
    unsigned char* h_idata = NULL;
    cl_int ciErrNum = CL_SUCCESS;

    //allocate host memory
    h_idata = (unsigned char *)malloc( memSize );

    //initialize the memory
    for(unsigned int i = 0; i < memSize/sizeof(unsigned char); i++)
    {
        h_idata[i] = (unsigned char) (i & 0xff);
    }

    // allocate device input and output memory and initialize the device input memory
    cl_mem d_idata = clCreateBuffer(cxGPUContext, CL_MEM_READ_ONLY, memSize, NULL, &ciErrNum);
    ownCheckError(ciErrNum, CL_SUCCESS);
    cl_mem d_odata = clCreateBuffer(cxGPUContext, CL_MEM_WRITE_ONLY, memSize, NULL, &ciErrNum);
    ownCheckError(ciErrNum, CL_SUCCESS);
    ciErrNum = clEnqueueWriteBuffer(cqCommandQueue, d_idata, CL_TRUE, 0, memSize, h_idata, 0, NULL, NULL);
    ownCheckError(ciErrNum, CL_SUCCESS);

    // Sync queue to host, start timer 0, and copy data from one GPU buffer to another GPU bufffer
    clFinish(cqCommandQueue);
    shrDeltaT(0);
    for(unsigned int i = 0; i < MEMCOPY_ITERATIONS; i++)
    {
        ciErrNum = clEnqueueCopyBuffer(cqCommandQueue, d_idata, d_odata, 0, 0, memSize, 0, NULL, NULL);
        ownCheckError(ciErrNum, CL_SUCCESS);
    }

    // Sync with GPU
    clFinish(cqCommandQueue);

    //get the the elapsed time in seconds
    elapsedTimeInSec = shrDeltaT(0);

    // Calculate bandwidth in MB/s
    //      This is for kernels that read and write GMEM simultaneously
    //      Obtained Throughput for unidirectional block copies will be 1/2 of this #
    bandwidthInMBs = 2.0 * ((double)memSize * (double)MEMCOPY_ITERATIONS)/(elapsedTimeInSec * (double)(1 << 20));

    //clean up memory on host and device
    free(h_idata);
    clReleaseMemObject(d_idata);
    clReleaseMemObject(d_odata);

    return bandwidthInMBs;
}

/////////////////////////////////////////////////////////
//print results in an easily read format
////////////////////////////////////////////////////////
void OpenCLTestBandwidth::printResultsReadable(unsigned int *memSizes, double* bandwidths, unsigned int count, memcpyKind kind, accessMode accMode, memoryMode memMode, int iNumDevs)
{
    // log config information
    if (kind == DEVICE_TO_DEVICE)
    {
        stream << "== Device to Device Bandwidth, Device(s) " << iNumDevs << " ==" << endl;
    }
    else
    {
        if (kind == DEVICE_TO_HOST)
        {
            stream << "== Device to Host Bandwidth, Device(s) " << iNumDevs << " ==" << endl;
        }
        else if (kind == HOST_TO_DEVICE)
        {
            stream << "== Host to Device Bandwidth, Device(s) " << iNumDevs << " ==" << endl;
        }
        if(memMode == PAGEABLE)
        {
            stream << "(Paged memory";
        }
        else if (memMode == PINNED)
        {
            stream << "(Pinned memory";
        }
        if(accMode == DIRECT)
        {
            stream << ", direct access)" << endl;
        }
        else if (accMode == MAPPED)
        {
			stream << ", mapped access)" << endl;
        }
    }

	//stream << "> Transfer Size (Bytes) : Bandwidth(MB/s)" << endl;
    unsigned int i;
    for(i = 0; i < count; i++)
    {
		//stream <<  "> " << memSizes[i] << " : " << bandwidths[i] << endl;
		stream <<  "Result: " << bandwidths[i] <<  " MB/s" << endl;
    }
}
