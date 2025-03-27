#define EXTMEM  0x100000            // 扩展内存起始地址
#define PHYSTOP 0xE000000           // 最大物理地址
#define DEVSPACE 0xFE000000         // 其他设备在更高地址

// Key addresses for address space layout (see kmap in vm.c for layout)
#define KERNBASE 0x80000000         // 内核起始虚拟地址
#define KERNLINK (KERNBASE+EXTMEM)  // 链接内核的地址

#define V2P(a) (((uint) (a)) - KERNBASE) // 虚拟地址'la'转换为物理地址
#define P2V(a) ((void *)(((char *) (a)) + KERNBASE)) // 物理地址'pa'转换为虚拟地址

#define V2P_WO(x) ((x) - KERNBASE)    // 与V2P相同, 但没有强制转换
#define P2V_WO(x) ((x) + KERNBASE)    // 与P2V相同, 但没有强制转换