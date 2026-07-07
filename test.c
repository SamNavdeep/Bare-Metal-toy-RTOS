
#include <stdint.h>
#include "ARMCM4.h"

#define DELAY_LEN 500000 
#define STACK_SIZE 256

volatile uint32_t current_task __attribute__((used)) = 0;
uint32_t x = 0;
uint32_t y = 0;

void scheduler(void){
    current_task = (current_task + 1) % 2;
}

void delay(){
    volatile uint32_t i;
    for(i = 0; i < DELAY_LEN; i++){}
}

void counter1(uint32_t* a){
    while(1){
        delay();
        *a += 1;
    }
}

void counter2(uint32_t* b){
    while(1){
        delay();
        *b += 1;
    }
}

struct task_info{
    uint32_t *sp;
    uint8_t id;
};

uint32_t task1[STACK_SIZE];
uint32_t task2[STACK_SIZE];
struct task_info tasks[2] __attribute__((used)) = {{task1, 1}, {task2, 2}};

void SysTick_Handler(void){
    static volatile uint32_t hardware_ticks = 0;
    hardware_ticks += 1;
    
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	
}

__attribute__((naked)) void PendSV_Handler(void) {
    __asm volatile(
        "CPSID I\n"              // Disable interrupts
        
        "PUSH {R4-R11}\n"        // Push remaining 8 registers
        
        "LDR R0, =current_task\n"
        "LDR R1, [R0]\n"         
        "CBZ R1, skip_save_2\n"  
        
        "LDR R2, =tasks\n"       
        "ADD R2, #8\n"           
        "STR SP, [R2]\n"         // Store SP into tasks[1]
        "B next_task\n"
        
        "skip_save_2:\n"
        "LDR R2, =tasks\n"
        "STR SP, [R2]\n"         // Store SP into tasks[0]
        
        "next_task:\n"
        "PUSH {LR}\n"            // Save the magic EXC_RETURN number
        "BL scheduler\n"         // Call our C scheduler
        "POP {LR}\n"             // Restore the magic number
        
        "LDR R0, =current_task\n"
        "LDR R1, [R0]\n"         
        "LDR R2, =tasks\n"
        "CBZ R1, load_task1\n"
        "ADD R2, #8\n"           
        
        "load_task1:\n"
        "LDR SP, [R2]\n"         // Load new task's SP 
        
        "POP {R4-R11}\n"         // Pop the 8 manual registers
        
        "CPSIE I\n"              // Re-enable interrupts
        "BX LR\n"                // Hardware auto-pops and resumes task!
    );
}

void os_init(void){
    // --- Task 1 Setup ---
    tasks[0].sp = &task1[STACK_SIZE - 16]; 
    task1[STACK_SIZE - 1] = 0x01000000;    
    task1[STACK_SIZE - 2] = ((uint32_t)counter1); 
    
    task1[STACK_SIZE - 8] = (uint32_t)&x;  

    // --- Task 2 Setup ---
    tasks[1].sp = &task2[STACK_SIZE - 16]; 
    task2[STACK_SIZE - 1] = 0x01000000;
    task2[STACK_SIZE - 2] = ((uint32_t)counter2); 
    
    task2[STACK_SIZE - 8] = (uint32_t)&y;  
}

// The OS Bootloader
__attribute__((naked)) void os_start(void) {
    __asm volatile(
        "LDR R0, =tasks\n"
        "LDR SP, [R0]\n"         // 1. Point SP to Task 1 stack
        "POP {R4-R11}\n"         // 2. Pop the 8 registers
        "POP {R0-R3, R12, LR}\n" // 3. Pop the hardware registers (R0 gets &x)
        "POP {R1}\n"             // 4. Pop the PC (counter1 address) into R1
        "POP {R2}\n"             // 5. Pop xPSR into R2 (discard it for now)
        "CPSIE I\n"              // 6. Enable interrupts safely
        "BX R1\n"                // 7. Jump straight into counter1
    );
}

int main(){
    os_init();
    SysTick_Config(SystemCoreClock / 1000);
    NVIC_SetPriority(PendSV_IRQn, 255);
    os_start(); 
    
    return 0; 
}