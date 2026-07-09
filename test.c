#include <stdint.h>
#include "ARMCM4.h"

#define DELAY_LEN 500000 
#define STACK_SIZE 256

volatile uint32_t current_task __attribute__((used)) = 0;
uint32_t x = 0;
uint32_t y = 0;
uint32_t z = 0; 

void scheduler(void){

    current_task = (current_task + 1) % 3;
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
        *b += 2;
    }
}

void counter3(uint32_t* a){
    while(1){
        delay();
        *a += 3;
    }
}

struct task_info{
    uint32_t *sp;
    uint8_t id;
};

uint32_t task1[STACK_SIZE];
uint32_t task2[STACK_SIZE];
uint32_t task3[STACK_SIZE];


struct task_info tasks[3] __attribute__((used)) = {
    {task1, 1}, 
    {task2, 2}, 
    {task3, 3}
};

void SysTick_Handler(void){
    static volatile uint32_t hardware_ticks = 0;
    hardware_ticks += 1;
    
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

__attribute__((naked)) void PendSV_Handler(void) {
    __asm volatile(
        "CPSID I\n"              // Disable interrupts
        
        "PUSH {R4-R11}\n"        
        "LDR R0, =current_task\n"
        "LDR R1, [R0]\n"         // R1 = current_task index (0, 1, or 2)
        "LDR R2, =tasks\n"
        "LSL R1, R1, #3\n"       // R1 = current_task * 8 (sizeof struct is 8 bytes)
        "ADD R2, R2, R1\n"       // R2 = &tasks[current_task]
        "STR SP, [R2]\n"         // Save current SP into tasks[current_task].sp
        

        "PUSH {LR}\n"            // Save the EXC_RETURN number
        "BL scheduler\n"         // Call our C scheduler
        "POP {LR}\n"             // Restore the number
   
        "LDR R0, =current_task\n"
        "LDR R1, [R0]\n"         // R1 = new current_task index
        "LDR R2, =tasks\n"
        "LSL R1, R1, #3\n"       // R1 = new current_task * 8
        "ADD R2, R2, R1\n"       // R2 = &tasks[new current_task]
        "LDR SP, [R2]\n"         // Load new task's SP 
        
        "POP {R4-R11}\n"      
        
        "CPSIE I\n"              // Re-enable interrupts
        "BX LR\n"                // Hardware auto-pops 
    );
}

void os_init(void){

    tasks[0].sp = &task1[STACK_SIZE - 16]; 
    task1[STACK_SIZE - 1] = 0x01000000;    
    task1[STACK_SIZE - 2] = ((uint32_t)counter1); 
    task1[STACK_SIZE - 8] = (uint32_t)&x;  


    tasks[1].sp = &task2[STACK_SIZE - 16]; 
    task2[STACK_SIZE - 1] = 0x01000000;
    task2[STACK_SIZE - 2] = ((uint32_t)counter2); 
    task2[STACK_SIZE - 8] = (uint32_t)&y;  


    tasks[2].sp = &task3[STACK_SIZE - 16]; 
    task3[STACK_SIZE - 1] = 0x01000000;
    task3[STACK_SIZE - 2] = ((uint32_t)counter3); 
    task3[STACK_SIZE - 8] = (uint32_t)&z;  
}

// The OS Bootloader
__attribute__((naked)) void os_start(void) {
    __asm volatile(
        "LDR R0, =tasks\n"
        "LDR SP, [R0]\n"         // 1. Point SP to Task 1 stack
        "POP {R4-R11}\n"         // 2. Pop the 8 registers
        "POP {R0-R3, R12, LR}\n" // 3. Pop the hardware registers 
        "POP {R1}\n"             // 4. Pop the PC into R1
        "POP {R2}\n"             // 5. Pop xPSR into R2 
        "CPSIE I\n"              // 6. Enable interrupt
        "BX R1\n"                // 7. Jump to counter1
    );
}

int main(){
    os_init();
    SysTick_Config(SystemCoreClock / 1000);
    NVIC_SetPriority(PendSV_IRQn, 255);
    os_start(); 
    
    return 0; 
}
