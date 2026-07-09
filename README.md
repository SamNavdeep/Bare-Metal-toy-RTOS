# Brief Summary of the project

This is a project to implement the round robin scheduler in baremetal C in Arm cortex M processor. The primary goal of this project was to understand hardware rules of the Cortex-M processor, specifically focusing on deterministic exception handling, raw memory manipulation, and the physics of hardware-level context switching.

# What I Learned & Implemented

Preemptive Round-Robin Scheduling: I implemented a basic scheduler that uses the hardware SysTick timer to interrupt tasks every millisecond. This helped me understand the difference between cooperative multitasking and true preemptive scheduling.

Context Switching with PendSV: This was the hardest part to get right. I learned that doing a context switch directly inside the SysTick handler can cause fatal nesting faults. I implemented the PendSV exception and set it to the lowest priority (255) to ensure stack swapping only happens when the CPU is in a safe state.

Forging a Stack Frame: To start a task, I had to learn how the CPU actually reads memory. I bypassed the standard C runtime and manually initialized dummy stack frames in RAM. This included setting up the initial Program Counter (PC), configuring the xPSR register to keep the CPU in Thumb instruction mode, and passing arguments via the R0 register.

Inline Assembly & AAPCS Rules: I wrote custom naked functions (__attribute__((naked))) to manually push and pop the software registers (R4-R11). During this, I ran into several HardFaults and had to learn about the ARM Architecture Procedure Call Standard (AAPCS)—specifically how to fix stack misalignment by pushing dummy registers to maintain the strict 8-byte boundary.

# Output
<img width="1877" height="976" alt="image" src="https://github.com/user-attachments/assets/ec06ea2f-7a5a-490a-8015-bd6afa22f6c6" />


You can see that the global variables x,y,z are incrementing together, indicating the context switching.
