/********************************************************************************
 * Copyright (c) 2016-2019 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ********************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <stdint.h>
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include "cl_err.h"

static void print_help(const char * const description)
{
    fprintf(stderr, "clEnqueueCopyBufferP2PAMD Test App -- %s\n", description);
    fprintf(stderr, "Command line parameters:\n");
    fprintf(stderr, "   -h, --help: Print this help menu.\n");
    fprintf(stderr, "The following parameters are optional:\n");
    fprintf(stderr, "   -p, --platform: Choose OpenCL platform (default 0)\n");
    fprintf(stderr, "   -f, --first_device: Choose 1st device (default 0)\n");
    fprintf(stderr, "   -s, --second_device: Choose 2nd device (dfeault 1)\n");
}

void check_opts(const int argc, char** argv, const char *description,
        uint32_t * const platform, uint32_t * const first_device,
        uint32_t * const second_device)
{
    const char* const opts = "hs:p:f:s:";
    const struct option long_opts[] = {
            {"help", 0, NULL, 'h'},
            {"size", 1, NULL, 's'},
            {"platform", 1, NULL, 'p'},
            {"first_device", 1, NULL, 'f'},
            {"second_device", 1, NULL, 's'},
            {NULL, 0, NULL, 0}
    };

    if (argv == NULL || description == NULL || platform == NULL ||
            first_device == NULL || second_device == NULL)
    {
        fprintf(stderr, "Incorrectly passing arguments to check_opts\n");
        fprintf(stderr, "Pointers were: %p %p %p %p %p\n", (void*)argv,
                (void*)description, (void*)platform, (void*)first_device,
                (void*)second_device);
        fprintf(stderr, "Error at %s:%d\n", __FILE__, __LINE__);
        exit(-1);
    }

    *platform = 0;
    *first_device = 0;
    *second_device = 1;

    while (1)
    {
        int retval = getopt_long(argc, argv, opts, long_opts, NULL);
        if (retval == -1)
            return;
        switch (retval)
        {
            case 'p':
                *platform = (uint32_t)atoi(optarg);
                break;
            case 'f':
                *first_device = (uint32_t)atoi(optarg);
                break;
            case 's':
                *second_device = (uint32_t)atoi(optarg);
            case 'h':
            case '?':
            default:
                print_help(description);
                exit(-1);
        }
    }
}

cl_platform_id setup_platform(const uint32_t platform_to_use)
{
    cl_int cl_err;
    cl_uint num_platforms;

    printf("Searching for platforms...\n");

    cl_err = clGetPlatformIDs(0, NULL, &num_platforms);
    check_cl_error(__FILE__, __LINE__, cl_err);

    if (num_platforms <= platform_to_use)
    {
        fprintf(stderr, "Requested to use platform %u\n", platform_to_use);
        fprintf(stderr, "But there are only %u platforms in the system!\n",
                num_platforms);
        fprintf(stderr, "Quitting in error. %s:%d\n", __FILE__, __LINE__);
        exit(-1);
    }

    cl_platform_id *platform_ids;
    platform_ids= calloc(num_platforms, sizeof(cl_platform_id));
    if (platform_ids == NULL)
    {
        fprintf(stderr, "Unable to calloc(%u, %zu) at %s:%d\n", num_platforms,
                sizeof(cl_platform_id), __FILE__, __LINE__);
        exit(-1);
    }
    cl_err = clGetPlatformIDs(num_platforms, platform_ids, NULL);
    check_cl_error(__FILE__, __LINE__, cl_err);

    cl_platform_id plat_to_return = platform_ids[platform_to_use];

    size_t platform_name_len = 0;
    cl_err = clGetPlatformInfo(plat_to_return, CL_PLATFORM_NAME, 0, NULL,
            &platform_name_len);
    check_cl_error(__FILE__, __LINE__, cl_err);

    char *platform_name = calloc(platform_name_len, sizeof(char));
    if (platform_name == NULL)
    {
        fprintf(stderr, "Unable to calloc(%zu) at %s:%d\n",
                platform_name_len, __FILE__, __LINE__);
        exit(-1);
    }
    cl_err = clGetPlatformInfo(plat_to_return, CL_PLATFORM_NAME,
            platform_name_len, platform_name, NULL);
    check_cl_error(__FILE__, __LINE__, cl_err);
    printf("    Using platform: %s\n", platform_name);

    free(platform_ids);
    free(platform_name);
    return plat_to_return;
}

cl_device_id setup_device(const uint32_t device_to_use,
        const uint32_t platform_to_use, const cl_platform_id platform,
        const cl_device_type dev_type)
{
    cl_int cl_err;
    cl_uint num_devices;

    printf("Searching for devices...\n");

    cl_err = clGetDeviceIDs(platform, dev_type, 0, NULL,
            &num_devices);
    check_cl_error(__FILE__, __LINE__, cl_err);

    if (num_devices <= device_to_use)
    {
        fprintf(stderr, "Requested to use device %u on platform %u\n",
                device_to_use, platform_to_use);
        fprintf(stderr, "But there are only %u GPU devices on this platform.\n",
                num_devices);
        fprintf(stderr, "Quitting in error. %s:%d\n", __FILE__, __LINE__);
        exit(-1);
    }

    cl_device_id *device_ids = calloc(num_devices, sizeof(cl_device_id));
    if (device_ids == NULL)
    {
        fprintf(stderr, "Unable to calloc(%u, %zu) at %s:%d\n", num_devices,
                sizeof(cl_device_id), __FILE__, __LINE__);
        exit(-1);
    }
    cl_err = clGetDeviceIDs(platform, dev_type, num_devices,
            device_ids, NULL);
    check_cl_error(__FILE__, __LINE__, cl_err);

    cl_device_id dev_to_return = device_ids[device_to_use];

    size_t device_name_len = 0;
    cl_err = clGetDeviceInfo(dev_to_return, CL_DEVICE_NAME, 0, NULL,
            &device_name_len);
    check_cl_error(__FILE__, __LINE__, cl_err);

    char *device_name = calloc(device_name_len, sizeof(char));
    if (device_name == NULL)
    {
        fprintf(stderr, "Unable to calloc(%zu) at %s:%d\n",
                device_name_len, __FILE__, __LINE__);
        exit(-1);
    }
    cl_err = clGetDeviceInfo(dev_to_return, CL_DEVICE_NAME, device_name_len,
            device_name, NULL);
    check_cl_error(__FILE__, __LINE__, cl_err);
    printf("    Using device: %s\n", device_name);

    free(device_ids);
    free(device_name);
    return dev_to_return;
}

cl_context setup_context(const cl_platform_id platform,
        const cl_device_id device)
{
    cl_int cl_err;
    cl_context ctxt_to_return;

    cl_context_properties properties[] = { CL_CONTEXT_PLATFORM,
        (cl_context_properties)platform, 0 };
    ctxt_to_return = clCreateContext(properties, 1, &device, NULL, NULL,
            &cl_err);
    check_cl_error(__FILE__, __LINE__, cl_err);
    return ctxt_to_return;
}

cl_command_queue setup_cmd_queue(const cl_context context,
        const cl_device_id device)
{
    cl_int cl_err;
    cl_command_queue queue_to_return;
    queue_to_return = clCreateCommandQueueWithProperties(context, device, NULL,
            &cl_err);
    check_cl_error(__FILE__, __LINE__, cl_err);
    return queue_to_return;
}

int main(int argc, char** argv)
{
    cl_int cl_err;
    uint32_t platform_to_use = 0;
    uint32_t device_to_use[2];
    cl_device_type dev_type = CL_DEVICE_TYPE_GPU;
    uint64_t buffer_size = 1024 * 1024 * 1024;

    // Check input options.
    check_opts(argc, argv, "clEnqueueCopyBufferP2PAMD Test",
            &platform_to_use, &device_to_use[0], &device_to_use[1]);

    // Set up the OpenCL environment.
    cl_platform_id platform = setup_platform(platform_to_use);

    cl_device_id devices[2];
    cl_context contexts[2];
    cl_command_queue cmd_queues[2];
    cl_mem buffers[2];
    // Set up GPUs devices, contexts, command queues, and buffers.
    // We have two GPUs.
    for (int i = 0; i < 2; i++)
    {
        devices[i] = setup_device(device_to_use[i], platform_to_use,
                platform, dev_type);

        // Set up contexts on both of the GPUs
        // For this P2P transfer to work, the devices must be in separate
        // contexts. And only one GPU can be in that context.
        contexts[i] = setup_context(platform, devices[i]);

        // Set up command queues for the GPUs
        cmd_queues[i] = setup_cmd_queue(contexts[i], devices[i]);

        // Allocate buffers on both devices
        buffers[i] = clCreateBuffer(contexts[i], CL_MEM_READ_WRITE,
                buffer_size,  NULL, &cl_err);
        check_cl_error(__FILE__, __LINE__, cl_err);

        // Print out information about the P2P capabilities
        // Number of devices that are P2P with your chosen GPU
        cl_uint num_p2p_devs;
        cl_err = clGetDeviceInfo(devices[i], CL_DEVICE_NUM_P2P_DEVICES_AMD,
                sizeof(cl_uint), &num_p2p_devs, NULL);
        check_cl_error(__FILE__, __LINE__, cl_err);
        printf("Number of P2P devices that can be seen from device #%u: %u\n",
                device_to_use[i], num_p2p_devs);

        // Print out info about any neighboring devices.
        cl_device_id *p2p_devs;
        p2p_devs = malloc(num_p2p_devs * sizeof(cl_device_id));
        cl_err = clGetDeviceInfo(devices[i], CL_DEVICE_P2P_DEVICES_AMD,
                num_p2p_devs * sizeof(cl_device_id), p2p_devs, NULL);
        check_cl_error(__FILE__, __LINE__, cl_err);

        cl_device_topology_amd topology;
        cl_err = clGetDeviceInfo(devices[i], CL_DEVICE_TOPOLOGY_AMD,
                sizeof(cl_device_topology_amd), &topology, NULL);
        check_cl_error(__FILE__, __LINE__, cl_err);
        fprintf(stderr, "PCIe Topology of device %u: %x:%x.%x\n",
                device_to_use[i], topology.pcie.bus, topology.pcie.device,
                topology.pcie.function);

        for (int j = 0; j < num_p2p_devs; j++)
        {
            cl_err = clGetDeviceInfo(p2p_devs[j], CL_DEVICE_TOPOLOGY_AMD,
                    sizeof(cl_device_topology_amd), &topology, NULL);
            check_cl_error(__FILE__, __LINE__, cl_err);
            fprintf(stderr, "    Topology of neighbor #%u: %x:%x.%x\n", j,
                    topology.pcie.bus, topology.pcie.device,
                    topology.pcie.function);
        }
        free(p2p_devs);
    }

    // Verify that device 0 can see device 1 for a P2P transfer
    cl_device_topology_amd target_dev_topology;
    cl_err = clGetDeviceInfo(devices[1], CL_DEVICE_TOPOLOGY_AMD,
                    sizeof(cl_device_topology_amd), &target_dev_topology,
                    NULL);
    check_cl_error(__FILE__, __LINE__, cl_err);

    cl_uint num_p2p_devs;
    cl_err = clGetDeviceInfo(devices[0], CL_DEVICE_NUM_P2P_DEVICES_AMD,
            sizeof(cl_uint), &num_p2p_devs, NULL);
    check_cl_error(__FILE__, __LINE__, cl_err);
    cl_device_id *p2p_devs;
    p2p_devs = malloc(num_p2p_devs * sizeof(cl_device_id));
    cl_err = clGetDeviceInfo(devices[0], CL_DEVICE_P2P_DEVICES_AMD,
            num_p2p_devs * sizeof(cl_device_id), p2p_devs, NULL);
    check_cl_error(__FILE__, __LINE__, cl_err);
    int is_neighbor = 0;
    for (int i = 0; i < num_p2p_devs; i++)
    {
        cl_device_topology_amd temp_topology;
        cl_err = clGetDeviceInfo(p2p_devs[i], CL_DEVICE_TOPOLOGY_AMD,
        sizeof(cl_device_topology_amd), &temp_topology,
            NULL);
        check_cl_error(__FILE__, __LINE__, cl_err);
        if (temp_topology.pcie.bus == target_dev_topology.pcie.bus &&
                        temp_topology.pcie.device == target_dev_topology.pcie.device &&
                        temp_topology.pcie.function == target_dev_topology.pcie.function)
            is_neighbor = 1;
    }
    free(p2p_devs);
    if (!is_neighbor)
    {
        fprintf(stderr, "Device %u and device %u are not P2P neighbors\n",
                device_to_use[0], device_to_use[1]);
        fprintf(stderr, "They cannot be used to test P2P transfers.\n");
        fprintf(stderr, "Exiting!\n");
        return -1;
    }

    for (int i = 0; i < 2; i++)
    {
        printf("Moving buffers %d to Device %u\n", i, device_to_use[i]);
        cl_err = clEnqueueMigrateMemObjects(cmd_queues[i], 1, &buffers[i], 0,
                0, NULL, NULL);
        check_cl_error(__FILE__, __LINE__, cl_err);
        cl_err = clFinish(cmd_queues[i]);
        check_cl_error(__FILE__, __LINE__, cl_err);
        printf("Done moving buffer\n");

        cl_uint pattern;
        if (i == 0)
        {
            printf("Filling buffer 0 with '0x1234567'\n");
            pattern=0x12345678;
        }
        else
        {
            printf("Filling buffer 1 with '0'\n");
            pattern=0;
        }
        cl_err = clEnqueueFillBuffer(cmd_queues[i], buffers[i], &pattern,
                sizeof(cl_uint), 0, buffer_size, 0, NULL, NULL);
        check_cl_error(__FILE__, __LINE__, cl_err);
        cl_err = clFinish(cmd_queues[i]);
        check_cl_error(__FILE__, __LINE__, cl_err);
        printf("Done filling buffer.\n");

        printf("\n");
    }

    printf("\n\nTransferring buffer 0 to buffer 1.\n");
    // Set up function pointer to our P2P extension function
    clEnqueueCopyBufferP2PAMD_fn call_this =
        clGetExtensionFunctionAddressForPlatform(platform,
                "clEnqueueCopyBufferP2PAMD");
    // Use the function pointer to call that function
    // For this function, the command queue must point to the source
    // device.
    cl_err = call_this(cmd_queues[0], buffers[0], buffers[1], 0, 0,
            buffer_size, 0, NULL, NULL);
    check_cl_error(__FILE__, __LINE__, cl_err);
    cl_err = clFinish(cmd_queues[0]);
    check_cl_error(__FILE__, __LINE__, cl_err);
    printf("Done transferring buffer 0 to buffer 1.\n");

    printf("Reading buffer 1. It should contain '0x12345678'\n");
    cl_uint *read_buffer;
    read_buffer = calloc(buffer_size, 1);
    cl_err = clEnqueueReadBuffer(cmd_queues[1], buffers[1], CL_BLOCKING, 0,
            buffer_size, read_buffer, 0, 0, NULL);
    check_cl_error(__FILE__, __LINE__, cl_err);

    printf("First few values: %x %x %x\n", read_buffer[0], read_buffer[1],
            read_buffer[2]);

    if (read_buffer[0] != 0x12345678 || read_buffer[1] != 0x12345678 ||
            read_buffer[2] != 0x12345678)
    {
        printf("ERROR appears to have happened!\n");
        printf("It looks like the P2P transfer failed!\n");
        return -1;
    }
    else
    {
        printf("Done Running clEnqueueCopyBufferP2PAMD Test.\n");
        return 0;
    }
}
