/*
 *  cpu.c
 *  Contains APEX cpu pipeline implementation
 *
 *  Author :
 *  Ulugbek Ergashev (uergash1@binghamton.edu)
 *  State University of New York, Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"

/* Flag to enable debug messages */
int ENABLE_DEBUG_MESSAGES;

/* Flag to enable to display register and memory values */
int ENABLE_DISPLAY;

/* Flag to enable counting code_memory_size by the implemented logic */
int ENABLE_COUNTING;

/*
 * This function creates and initializes APEX cpu.
 *
 * Note : You are free to edit this function according to your
 * 				implementation
 */
APEX_CPU*
APEX_cpu_init(const char* filename, const char* function, const int cycles)
{
  if (!filename && !function && !cycles) {
    return NULL;
  }

  APEX_CPU* cpu = malloc(sizeof(*cpu));
  if (!cpu) {
    return NULL;
  }

  if (strcmp(function, "simulate") == 0) {
    ENABLE_DEBUG_MESSAGES = 0;
    ENABLE_DISPLAY = 1;
  }
  else {
    ENABLE_DEBUG_MESSAGES = 1;
    ENABLE_DISPLAY = 1;
  }

  /* Initialize PC, Registers and all pipeline stages */
  cpu->pc = 4000;
  memset(cpu->regs, 0, sizeof(int) * 16);
  for (int i = 0; i < 16; i++) {
    cpu->regs_valid[i] = 1;
  }
  /*memset(cpu->regs_valid, 1, sizeof(int) * 16);*/
  memset(cpu->stage, 0, sizeof(CPU_Stage) * NUM_STAGES);
  memset(cpu->data_memory, 0, sizeof(int) * 4000);

  /* Parse input file and create code memory */
  cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);

  /* Making Z flag invalid for the first branch instruction */
  cpu->z_flag_valid = 1;

  cpu->clock = 1;

  if (!cpu->code_memory) {
    free(cpu);
    return NULL;
  }

  if (ENABLE_DEBUG_MESSAGES) {
    fprintf(stderr,
            "APEX_CPU : Initialized APEX CPU, loaded %d instructions\n",
            cpu->code_memory_size);
    fprintf(stderr, "APEX_CPU : Printing Code Memory\n");
    printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode", "rd", "rs1", "rs2", "imm");

    for (int i = 0; i < cpu->code_memory_size; ++i) {
      printf("%-9s %-9d %-9d %-9d %-9d\n",
             cpu->code_memory[i].opcode,
             cpu->code_memory[i].rd,
             cpu->code_memory[i].rs1,
             cpu->code_memory[i].rs2,
             cpu->code_memory[i].imm);
    }
  }

  /* Make all stages busy except Fetch stage, initally to start the pipeline */
  for (int i = 1; i < NUM_STAGES; ++i) {
    cpu->stage[i].busy = 1;
  }

  ENABLE_COUNTING = 0;
  cpu->code_memory_size = cycles;

  return cpu;
}

/*
 * This function de-allocates APEX cpu.
 *
 * Note : You are free to edit this function according to your
 * 				implementation
 */
void
APEX_cpu_stop(APEX_CPU* cpu)
{
  free(cpu->code_memory);
  free(cpu);
}

/* Converts the PC(4000 series) into
 * array index for code memory
 *
 * Note : You are not supposed to edit this function
 *
 */
int
get_code_index(int pc)
{
  return (pc - 4000) / 4;
}

static void
print_instruction(CPU_Stage* stage)
{
  if (strcmp(stage->opcode, "STORE") == 0) {
    printf(
      "%s,R%d,R%d,#%d ", stage->opcode, stage->rs1, stage->rs2, stage->imm);
  }

  if (strcmp(stage->opcode, "LOAD") == 0) {
    printf(
      "%s,R%d,R%d,#%d ", stage->opcode, stage->rd, stage->rs1, stage->imm);
  }

  if (strcmp(stage->opcode, "MOVC") == 0) {
    printf("%s,R%d,#%d ", stage->opcode, stage->rd, stage->imm);
  }

  if (strcmp(stage->opcode, "ADD") == 0 ||
      strcmp(stage->opcode, "SUB") == 0 ||
      strcmp(stage->opcode, "AND") == 0 ||
      strcmp(stage->opcode, "OR") == 0 ||
      strcmp(stage->opcode, "EX-OR") == 0 ||
      strcmp(stage->opcode, "MUL") == 0) {

    printf("%s,R%d,R%d,R%d ", stage->opcode, stage->rd, stage->rs1, stage->rs2);
  }

  if (strcmp(stage->opcode, "BZ") == 0 ||
      strcmp(stage->opcode, "BNZ") == 0) {
    printf("%s,#%d ", stage->opcode, stage->imm);
  }

  if (strcmp(stage->opcode, "JUMP") == 0) {
    printf("%s,R%d,#%d ", stage->opcode, stage->rs1, stage->imm);
  }

  if (strcmp(stage->opcode, "BUBBLE") == 0) {
    printf("%s", stage->opcode);
  }

  if (strcmp(stage->opcode, "HALT") == 0) {
    printf("%s", stage->opcode);
  }
}

/* Debug function which dumps the cpu stage
 * content
 *
 * Note : You are not supposed to edit this function
 *
 */
static void
print_stage_content(char* name, CPU_Stage* stage)
{
  printf("%-15s: pc(%d) ", name, stage->pc);
  print_instruction(stage);
  printf("\n");
}

/* Display function to display the content of reginsters and memory */
static void
display(APEX_CPU* cpu)
{
  printf("\n\n============== STATE OF ARCHITECTURAL REGISTER FILE ==============\n\n");
  for(int i = 0; i < 16; i++) {
    printf("|\tREG[%d]\t|\tValue = %d\t|\tStatus = %d\t|\n", i, cpu->regs[i], cpu->regs_valid[i]);
  }

  printf("\n\n========== STATE OF DATA MEMORY ==========\n\n");
  for(int i = 0; i < 100; i++) {
    printf("|\tMEM[%d]\t|\tData Value = %d\t|\n", i, cpu->data_memory[i]);
  }
}

/*  Exception handler
 *  Key 0 - computed effective memory address is not in the range of 0 - 4096
 *  Key 1 - Invalid input register
 */
int
exception_handler(int code, char* opcode)
{
  switch(code) {
    case 0: printf("ERROR >> Computed effective memory address for %s is out of 4096 memory range size\n", opcode);
            exit(1);
            break;

    case 1: printf("ERROR >> Invalid register input for %s (Register range is within 0-15)\n", opcode);
            exit(1);
            break;
  }
  return 0;
}

/*  Handles control flow for BZ, BNZ and JUMP
 *  Takes branch by introducing two BUBBLEs and computes the value for code_memory_size
 */
int
control_flow(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[EX];
  int difference = stage->buffer - stage->pc;

  strcpy(cpu->stage[DRF].opcode, "BUBBLE");
  cpu->stage[DRF].pc = 0000;

  strcpy(cpu->stage[F].opcode, "BUBBLE");
  cpu->stage[F].pc = 0000;
  cpu->stage[F].stalled = 1;

  cpu->pc = stage->buffer;

  if (ENABLE_COUNTING) {
    if (difference == 4) {
      cpu->code_memory_size += 2;
    }

    if (difference == 8) {
      cpu->code_memory_size++;
    }

    if (difference > 12) {
      int temp = (difference - 12) / 4;
      cpu->code_memory_size -= temp;
    }

    if (stage->imm < 0) {
      difference = abs(difference);
      int temp = difference / 4;
      cpu->code_memory_size = cpu->code_memory_size + temp + 3;
    }
  }
  return 0;
}

/*
 *  Fetch Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
fetch(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[F];
  if (!stage->busy && !stage->stalled) {

    /* Store current PC in fetch latch */
    stage->pc = cpu->pc;

    /* Index into code memory using this pc and copy all instruction fields into
     * fetch latch
     */
    APEX_Instruction* current_ins = &cpu->code_memory[get_code_index(cpu->pc)];
    strcpy(stage->opcode, current_ins->opcode);
    stage->rd = current_ins->rd;
    stage->rs1 = current_ins->rs1;
    stage->rs2 = current_ins->rs2;
    stage->imm = current_ins->imm;
    stage->rd = current_ins->rd;

    /* Update PC for next instruction */
    cpu->pc += 4;

    /* Copy data from fetch latch to decode latch */
    if (!cpu->stage[DRF].stalled) {
      cpu->stage[DRF] = cpu->stage[F];
    }
    else {
      stage->stalled = 1;
    }

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Fetch", stage);
    }
  }
  else {

    /* If HALT reached into WB stage, then finish program */
    if (strcmp(cpu->stage[WB].opcode, "HALT") == 0) {
      cpu->code_memory_size = cpu->clock + 1;
    }

    /* If current stage contains BUBBLE, then stop stalling the stage and show the content of fetch stage
     * The only instruction that introduces BUBBLE in fetch stage is BRANCH
     */
    if (strcmp(stage->opcode, "BUBBLE") == 0) {
      stage->stalled = 0;
      if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Fetch", stage);
      }
    }

    /* If DRF stage is not stalled, and does npt contain BUBBLE because of taken branch, then
     * stop stalling fetch stage, and copy data into the next stage.
     */
    if (!cpu->stage[DRF].stalled && strcmp(cpu->stage[DRF].opcode, "BUBBLE") != 0) {
      stage->stalled = 0;
      cpu->stage[DRF] = cpu->stage[F];
      if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Fetch", stage);
      }
    }

    /* Show if next stage is not HALT */
    if (cpu->stage[DRF].stalled && strcmp(cpu->stage[DRF].opcode, "HALT") != 0) {
      if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Fetch", stage);
      }
    }
  }
  return 0;
}

/*
 *  Decode Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
decode(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[DRF];
  if (!stage->busy && !stage->stalled) {

    /* Read data from register file for store */
    if (strcmp(stage->opcode, "STORE") == 0) {
      if (cpu->regs_valid[stage->rs1] && cpu->regs_valid[stage->rs2]) {
        stage->rs1_value = cpu->regs[stage->rs1];
        stage->rs2_value = cpu->regs[stage->rs2];
      }
      else{
        stage->stalled = 1;
      }
    }

    /* Read data from register file for LOAD */
    if (strcmp(stage->opcode, "LOAD") == 0) {
      if (cpu->regs_valid[stage->rs1]) {
        stage->rs1_value = cpu->regs[stage->rs1];
        cpu->regs_valid[stage->rd] = 0;
      }
      else {
        stage->stalled = 1;
      }
    }

    /* No Register file read needed for MOVC */
    if (strcmp(stage->opcode, "MOVC") == 0) {
      cpu->regs_valid[stage->rd] = 0;
    }

    /* Read data from register file for ADD, SUB, AND, OR, EX-OR */
    if (strcmp(stage->opcode, "ADD") == 0 ||
        strcmp(stage->opcode, "SUB") == 0 ||
        strcmp(stage->opcode, "AND") == 0 ||
        strcmp(stage->opcode, "OR") == 0 ||
        strcmp(stage->opcode, "EX-OR") == 0 ||
        strcmp(stage->opcode, "MUL") == 0) {

      if (cpu->regs_valid[stage->rs1] && cpu->regs_valid[stage->rs2]) {
        cpu->regs_valid[stage->rd] = 0;
        stage->rs1_value = cpu->regs[stage->rs1];
        stage->rs2_value = cpu->regs[stage->rs2];
      }
      else {
        stage->stalled = 1;
      }
    }

    /* Make Z_flag invalid when arithmetic operation comes in */
    if (strcmp(stage->opcode, "ADD") == 0 ||
        strcmp(stage->opcode, "SUB") == 0 ||
        strcmp(stage->opcode, "MUL") == 0) {

      cpu->z_flag_valid = 0;
    }

    /* Check whether Z_flag is valid for BZ and BNZ */
    if (strcmp(stage->opcode, "BZ") == 0 ||
        strcmp(stage->opcode, "BNZ") == 0) {

      if (!cpu->z_flag_valid) {
        stage->stalled = 1;
      }
    }

    /* Read data from register file for JUMP */
    if (strcmp(stage->opcode, "JUMP") == 0) {
      if (cpu->regs_valid[stage->rs1]) {
        stage->rs1_value = cpu->regs[stage->rs1];
      }
      else {
        stage->stalled = 1;
      }
    }

    /* Copy data from decode latch to execute latch
       if decode stage is not stalled
       else transfer bubble*/
    if (!stage->stalled) {
      cpu->stage[EX] = cpu->stage[DRF];
    }
    else {
      strcpy(cpu->stage[EX].opcode, "BUBBLE");
      cpu->stage[EX].pc = 0000;
    }

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Decode/RF", stage);
    }

    if (strcmp(stage->opcode, "HALT") == 0) {
      cpu->stage[F].stalled = 1;
      stage->stalled = 1;
    }
  }
  else {
    /* If current stage does not contain HALT, and next stage is not stalled, then
     * current stage is stalled becuase of dependency between source and destination registers
     */
    if (stage->stalled && strcmp(stage->opcode, "HALT") != 0 && !cpu->stage[EX].stalled) {
      if (ENABLE_COUNTING) {
        cpu->code_memory_size++;
      }

      if (strcmp(stage->opcode, "ADD") == 0 ||
          strcmp(stage->opcode, "SUB") == 0 ||
          strcmp(stage->opcode, "AND") == 0 ||
          strcmp(stage->opcode, "OR") == 0 ||
          strcmp(stage->opcode, "EX-OR") == 0 ||
          strcmp(stage->opcode, "MUL") == 0 ||
          strcmp(stage->opcode, "STORE") == 0) {

        if (cpu->regs_valid[stage->rs1] && cpu->regs_valid[stage->rs2]) {

          /* If it is STORE, do not make rd invalid */
          if (strcmp(stage->opcode, "STORE") != 0) {
            cpu->regs_valid[stage->rd] = 0;
          }

          stage->rs1_value = cpu->regs[stage->rs1];
          stage->rs2_value = cpu->regs[stage->rs2];
          stage->stalled = 0;
          cpu->stage[EX] = cpu->stage[DRF];
        }
      }

      if (strcmp(stage->opcode, "LOAD") == 0) {
        if (cpu->regs_valid[stage->rs1]) {
          cpu->regs_valid[stage->rd] = 0;
          stage->rs1_value = cpu->regs[stage->rs1];
          stage->stalled = 0;
          cpu->stage[EX] = cpu->stage[DRF];
        }
      }

      if (strcmp(stage->opcode, "JUMP") == 0) {
        if (cpu->regs_valid[stage->rs1]) {
          stage->rs1_value = cpu->regs[stage->rs1];
          stage->stalled = 0;
          cpu->stage[EX] = cpu->stage[DRF];
        }
      }

      if (strcmp(stage->opcode, "BZ") == 0 ||
          strcmp(stage->opcode, "BNZ") == 0) {

        if (cpu->z_flag_valid) {
          stage->stalled = 0;
          cpu->stage[EX] = cpu->stage[DRF];
        }
      }

      if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Decode/RF", stage);
      }
    }

    if (cpu->stage[EX].stalled && strcmp(cpu->stage[EX].opcode, "HALT") != 0) {
      if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Decode/RF", stage);
      }
    }
  }
  return 0;
}

/*
 *  Execute Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
execute(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[EX];
  if (!stage->busy && !stage->stalled) {

    /* STORE */
    if (strcmp(stage->opcode, "STORE") == 0) {
      stage->mem_address = stage->rs2_value + stage->imm;
      if (stage->mem_address > 4096 || stage->mem_address < 0) {
        exception_handler(0, stage->opcode);
      }
    }

     /* LOAD */
    if (strcmp(stage->opcode, "LOAD") == 0) {
      stage->mem_address = stage->rs1_value + stage->imm;
      if (stage->mem_address > 4096 || stage->mem_address < 0) {
        exception_handler(0, stage->opcode);
      }
    }

    /* MOVC */
    if (strcmp(stage->opcode, "MOVC") == 0) {
      stage->buffer = stage->imm + 0;
    }

    /* ADD */
    if (strcmp(stage->opcode, "ADD") == 0) {
      stage->buffer = stage->rs1_value + stage->rs2_value;
    }

    /* SUB */
    if (strcmp(stage->opcode, "SUB") == 0) {
      stage->buffer = stage->rs1_value - stage->rs2_value;
    }

    /* MUL - stall the stage for one cycle */
    if (strcmp(stage->opcode, "MUL") == 0) {
      stage->buffer = stage->rs1_value * stage->rs2_value;
      stage->stalled = 1;
    }

    /* AND */
    if (strcmp(stage->opcode, "AND") == 0) {
      stage->buffer = stage->rs1_value & stage->rs2_value;
    }

    /* OR */
    if (strcmp(stage->opcode, "OR") == 0) {
      stage->buffer = stage->rs1_value | stage->rs2_value;
    }

    /* EX-OR */
    if (strcmp(stage->opcode, "EX-OR") == 0) {
      stage->buffer = stage->rs1_value ^ stage->rs2_value;
    }

    if (strcmp(stage->opcode, "JUMP") == 0) {
      stage->buffer = stage->rs1_value + stage->imm;
      control_flow(cpu);
    }

    if (strcmp(stage->opcode, "BNZ") == 0) {
      if (!cpu->z_flag) {
        stage->buffer = stage->pc + stage->imm;
        control_flow(cpu);
      }
    }

    if (strcmp(stage->opcode, "BZ") == 0) {
      if (cpu->z_flag) {
        stage->buffer = stage->pc + stage->imm;
        control_flow(cpu);
      }
    }

    /* Copy data from Execute latch to Memory latch*/
    if (!stage->stalled) {
      cpu->stage[MEM] = cpu->stage[EX];
    }
    else {
      cpu->stage[DRF].stalled = 1;
      strcpy(cpu->stage[MEM].opcode, "BUBBLE");
      cpu->stage[MEM].pc = 0000;
    }

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Execute", stage);
    }

    if (strcmp(stage->opcode, "HALT") == 0) {
      stage->stalled = 1;
    }
  }
  else {
    /* If current stage is stalled then it is because of MUL
     * stop stalling and copy data into the next stage
     */
    if (stage->stalled && strcmp(stage->opcode, "MUL") == 0) {
      if (ENABLE_COUNTING) {
        cpu->code_memory_size++;
      }
      stage->stalled = 0;
      cpu->stage[DRF].stalled = 0;
      cpu->stage[MEM] = cpu->stage[EX];

      if (ENABLE_DEBUG_MESSAGES) {
        print_stage_content("Execute", stage);
      }
    }
  }
  return 0;
}

/*
 *  Memory Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
memory(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[MEM];
  if (!stage->busy && !stage->stalled) {

    /* STORE */
    if (strcmp(stage->opcode, "STORE") == 0) {
      cpu->data_memory[stage->mem_address] = stage->rs1_value;
    }

    /* LOAD */
    if (strcmp(stage->opcode, "LOAD") == 0) {
      stage->buffer = cpu->data_memory[stage->mem_address];
    }

    /* Copy data from decode latch to execute latch*/
    cpu->stage[WB] = cpu->stage[MEM];

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Memory", stage);
    }

    if (strcmp(stage->opcode, "HALT") == 0) {
      stage->stalled = 1;
    }
  }
  return 0;
}

/*
 *  Writeback Stage of APEX Pipeline
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
writeback(APEX_CPU* cpu)
{
  CPU_Stage* stage = &cpu->stage[WB];
  if (!stage->busy && !stage->stalled) {

    /* Update register file */
    if (strcmp(stage->opcode, "MOVC") == 0 ||
        strcmp(stage->opcode, "SUB") == 0 ||
        strcmp(stage->opcode, "ADD") == 0 ||
        strcmp(stage->opcode, "AND") == 0 ||
        strcmp(stage->opcode, "OR") == 0 ||
        strcmp(stage->opcode, "EX-OR") == 0 ||
        strcmp(stage->opcode, "LOAD") == 0 ||
        strcmp(stage->opcode, "MUL") == 0) {
      printf("%d", stage->buffer);
      cpu->regs[stage->rd] = stage->buffer;

      /*  Check whether there is an instruction in EX or MEM stage using the same destination register
       *  If so, do NOT make the register valid
       */
      if ((stage->rd != cpu->stage[MEM].rd ||
          strcmp(cpu->stage[MEM].opcode, "STORE") == 0 ||
          strcmp(cpu->stage[MEM].opcode, "BZ") == 0 ||
          strcmp(cpu->stage[MEM].opcode, "BNZ") == 0 ||
          strcmp(cpu->stage[MEM].opcode, "JUMP") == 0 ||
          strcmp(cpu->stage[MEM].opcode, "BUBBLE") == 0) &&
          (stage->rd != cpu->stage[EX].rd ||
          strcmp(cpu->stage[EX].opcode, "STORE") == 0 ||
          strcmp(cpu->stage[EX].opcode, "BZ") == 0 ||
          strcmp(cpu->stage[EX].opcode, "BNZ") == 0 ||
          strcmp(cpu->stage[EX].opcode, "JUMP") == 0 ||
          strcmp(cpu->stage[EX].opcode, "BUBBLE") == 0)) {

        cpu->regs_valid[stage->rd] = 1;
      }
    }

    /* Update Z-flag */
    if (strcmp(stage->opcode, "SUB") == 0 ||
        strcmp(stage->opcode, "ADD") == 0 ||
        strcmp(stage->opcode, "MUL") == 0) {

      /*  Check whether there is an instruction (ADD or SUB or MUL) in EX or MEM stage
       *  If so, do NOT make z_flag valid
       */
      if (strcmp(cpu->stage[MEM].opcode, "ADD") != 0 &&
          strcmp(cpu->stage[MEM].opcode, "SUB") != 0 &&
          strcmp(cpu->stage[MEM].opcode, "MUL") != 0 &&
          strcmp(cpu->stage[EX].opcode, "ADD") != 0 &&
          strcmp(cpu->stage[EX].opcode, "SUB") != 0 &&
          strcmp(cpu->stage[EX].opcode, "MUL") != 0) {

        cpu->z_flag_valid = 1;
        if (stage->buffer == 0) {
          cpu->z_flag = 1;
        }
        else {
          cpu->z_flag = 0;
        }
      }
    }

    cpu->ins_completed++;

    if (ENABLE_DEBUG_MESSAGES) {
      print_stage_content("Writeback", stage);
    }

    if (strcmp(stage->opcode, "HALT") == 0) {
      strcpy(stage->opcode, "");
    }
  }
  return 0;
}

/*
 *  APEX CPU simulation loop
 *
 *  Note : You are free to edit this function according to your
 * 				 implementation
 */
int
APEX_cpu_run(APEX_CPU* cpu)
{
  while (cpu->clock <= cpu->code_memory_size) {
    if (ENABLE_COUNTING) {
      /* All the instructions committed, so exit */
      if (cpu->ins_completed == cpu->code_memory_size) {
        printf("(apex) >> Simulation Complete\n");
        break;
      }
    }

    if (ENABLE_DEBUG_MESSAGES) {
      printf("--------------------------------\n");
      printf("Clock Cycle #: %d\n", cpu->clock);
      printf("--------------------------------\n");
    }

    writeback(cpu);
    memory(cpu);
    execute(cpu);
    decode(cpu);
    fetch(cpu);
    cpu->clock++;
  }

  if (ENABLE_DISPLAY) {
    display(cpu);
  }

  return 0;
}
