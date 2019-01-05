=== Test Application for clEnqueueCopyBufferP2PAMD ===

Sample code to demonstrate the function `clEnqueueCopyBufferP2PAMD()`, which is
enabled by the `cl_amd_copy_buffer_p2p` extension in the AMD OpenCL runtime.

This function will move `cl_mem` buffers between two GPUs that are in separate
contexts. (In fact, they must be in separate contexts, and each context must
have a single GPU in it).

This extension also extends `clDeviceInfo()` to show information about which
other devices can take part in a particular P2P transfer. In other words,
it lets you enumerate what "neighbors" can take part in this. The code here
demonstrates this functionality as well.

This sample has been tested on ROCm 2.0 on a system with multiple GPUs.
The GPUs must have PCIe Large BAR support (primarily enabled by workstation
and server cards) and the motherboard SBIOS must support it as well. Large
BAR support is often called "Above 4G Decoding" in system BIOS settings.
