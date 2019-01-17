#ifndef _APEX_CPU_H_
#define _APEX_CPU_H_
/**
 *  cpu.h
 *  Contains various CPU and Pipeline Data structures
 *
 *  Author :
 *  Ulugbek Ergashev (uergash1@binghamton.edu)
 *  State University of New York, Binghamton
 */

 #define IQ_ENTRIES_NUMBER 16
 #define ROB_ENTRIES_NUMBER 32
 #define LSQ_ENTRIES_NUMBER 20
 #define URF_ENTRIES_NUMBER 40
 #define RAT_ENTRIES_NUMBER 16
 #define RRAT_ENTRIES_NUMBER 16
 #define BIS_ENTRIES_NUMBER 8

enum STAGES
{
  F,
  DRF,
  Int_FU,
  Mul_FU,
  MEM,
  NUM_STAGES
};

/* Format of an APEX instruction  */
typedef struct APEX_Instruction
{
  char opcode[128];	// Operation Code
  int rd;		    // Destination Register Address
  int rs1;		    // Source-1 Register Address
  int rs2;		    // Source-2 Register Address
  int imm;		    // Literal Value
} APEX_Instruction;

/* Model of CPU stage latch */
typedef struct CPU_Stage
{
  int pc;		    // Program Counter
  char opcode[128];	// Operation Code
  int arch_rs1;		    // Source-1 Architectural Register Address
  int arch_rs2;		    // Source-2 Architectural Register Address
  int arch_rd;		    // Destination Architectural Register Address
  int phys_rs1;		    // Source-1 Physical Register Address
  int phys_rs2;		    // Source-2 Physical Register Address
  int phys_rd;		    // Destination Physical Register Address
  int imm;		    // Literal Value
  int rs1_value;	// Source-1 Register Value
  int rs2_value;	// Source-2 Register Value
  int rs1_valid;	// Source-1 Register Value
  int rs2_valid;	// Source-2 Register Value
  int buffer;		// Latch to hold some value
  int mem_address;	// Computed Memory Address
  int busy;		    // Flag to indicate, stage is performing some action
  int stalled;		// Flag to indicate, stage is stalled
  int rob_entry_id;
  int branch_id;
  int LSQ_index;
  int target_address;
} CPU_Stage;

/* Issue Queue entry */
typedef struct ISSUE_QUEUE_Entry
{
  int pc;		    // Program Counter
  char opcode[128];	// Operation Code
  int counter;    // counts the number of cycles of an instruction in Issue Queue
  int free;    // indicates if the entry is allocated or free
  enum STAGES FU_type;    // function unit type
  int imm;    // Literal Value

  /* Source-1 fields */
  int arch_rs1;
  int rs1_ready;    // source-1 ready bit
  int phys_rs1;    // source-1 physical address
  int rs1_value;    // source-1 value

  /* Source-2 fields */
  int arch_rs2;
  int rs2_ready;    // source-2 ready bit
  int phys_rs2;    // source-2 physical address
  int rs2_value;    // source-2 value

  int arch_rd;
  int phys_rd;
  int rob_entry_id;
  int LSQ_index;
  int branch_id;
} ISSUE_QUEUE_Entry;

/* Issue Queue */
typedef struct ISSUE_QUEUE
{
  int free_entry; // points to free entry in Issue Queue
  ISSUE_QUEUE_Entry iq_entry[IQ_ENTRIES_NUMBER];
} ISSUE_QUEUE;

typedef struct ROB_Entry
{
  int free;    // indicates if the entry is allocated or free
  char opcode[128];	// Operation Code
  int pc;    // PC value of an instruction
  int arch_rd;    // Destination architectural address
  int phys_rd;    // Destination physical address
  int status;    // indicates if the result value is valid
  int branch_id;
  int arch_rs1;
  int phys_rs1;    // source-1 physical address
  int arch_rs2;
  int phys_rs2;    // source-2 physical address
  int imm;
} ROB_Entry;

typedef struct ROB
{
  int tail;
  int head;
  ROB_Entry rob_entry[ROB_ENTRIES_NUMBER];
} ROB;

typedef struct UNIFIED_REGISTER_FILE_Entry
{
  int value;    // Value of physical register
  int free;    // Status bit indicating whether physical register is free or allocated
  int valid;    // Valid bit indicating whether physical register holds valid value or not
} UNIFIED_REGISTER_FILE_Entry;

typedef struct RENAME_ALIAS_TABLE_Entry
{
  int phys_reg;   // The most recent physical register for an architectural register in URF
} RENAME_ALIAS_TABLE_Entry;

typedef struct RETIREMENT_RENAME_ALIAS_TABLE_Entry
{
  int commited_phys_reg; // Commited physical register for an architectural register in URF
} RETIREMENT_RENAME_ALIAS_TABLE_Entry;

typedef struct BIS_Entry
{
  int free;    // indicates if the entry is allocated or free
  int phys_src;    // Physical Register Source for branch
} BIS_Entry;

typedef struct BACKUP_Entry
{
  //UNIFIED_REGISTER_FILE_Entry urf[URF_ENTRIES_NUMBER];
  RENAME_ALIAS_TABLE_Entry rat[RAT_ENTRIES_NUMBER];
} BACKUP_Entry;

typedef struct BIS
{
  int tail;  // branch instruction gets BIS id from the tail
  int head;
  BIS_Entry bis_entry[BIS_ENTRIES_NUMBER];
  BACKUP_Entry backup_entry[BIS_ENTRIES_NUMBER];
} BIS;

typedef struct LSQ_Entry
{
  int free;
  char opcode[128];
  int pc;
  int mem_address_valid;
  int mem_address;
  int branch_id;
  int rob_entry_id;

  /* Source-1 fields */
  int arch_rs1;
  int rs1_ready;    // source-1 ready bit
  int phys_rs1;    // source-1 physical address
  int rs1_value;    // source-1 value

  /* Source-2 fields */
  int arch_rs2;
  int phys_rs2;    // source-2 physical address

  int imm;

  int arch_rd;
  int phys_rd;
} LSQ_Entry;

typedef struct LSQ
{
  int tail;
  int head;
  LSQ_Entry lsq_entry[LSQ_ENTRIES_NUMBER];
} LSQ;

typedef struct APEX_CPU
{
  /* Clock cycles elasped */
  int clock;
  int fill_in_rob;

  int mul_cycle;
  int mem_cycle;
  int last_branch_id;
  int last_arith_phys_rd;
  int commitments;

  /* Current program counter */
  int pc;

  UNIFIED_REGISTER_FILE_Entry urf[URF_ENTRIES_NUMBER];

  /* Rename Table for 5 architectural registers */
  RENAME_ALIAS_TABLE_Entry rat[RAT_ENTRIES_NUMBER];

  /* Back-end Register Alias Table - 5 architectural registers point to committed physical registers in URF */
  RETIREMENT_RENAME_ALIAS_TABLE_Entry rrat[RRAT_ENTRIES_NUMBER];

  /* Issue Queue with 2 entries */
  ISSUE_QUEUE iq;

  ROB rob;

  LSQ lsq;

  BIS bis;

  /* Array of 5 CPU_stage */
  CPU_Stage stage[5];

  /* Code Memory where instructions are stored */
  APEX_Instruction* code_memory;
  int code_memory_size;

  /* Data Memory */
  int data_memory[4096];

  /* Some stats */
  int simulation_completed;

} APEX_CPU;

APEX_Instruction*
create_code_memory(const char* filename, int* size);

APEX_CPU*
APEX_cpu_init(const char* filename, const char* function, const int cycles);

int
get_source_values(APEX_CPU* cpu, int rs2_exist);

int
exception_handler(int code, char* opcode);

int
APEX_cpu_run(APEX_CPU* cpu);

void
APEX_cpu_stop(APEX_CPU* cpu);

int
fetch(APEX_CPU* cpu);

int
decode(APEX_CPU* cpu);

int
execute_int(APEX_CPU* cpu);

int
execute_mul(APEX_CPU* cpu);

int
memory(APEX_CPU* cpu);

void
print_instruction(int fetch_decode, CPU_Stage* stage);

#endif
