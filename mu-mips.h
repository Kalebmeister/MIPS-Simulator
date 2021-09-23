#include <stdint.h>

#define FALSE 0
#define TRUE  1

/******************************************************************************/
/* MIPS memory layout                                                                                                                                      */
/******************************************************************************/
#define MEM_TEXT_BEGIN  0x00400000
#define MEM_TEXT_END      0x0FFFFFFF
/*Memory address 0x10000000 to 0x1000FFFF access by $gp*/
#define MEM_DATA_BEGIN  0x10010000
#define MEM_DATA_END   0x7FFFFFFF

#define MEM_KTEXT_BEGIN 0x80000000
#define MEM_KTEXT_END  0x8FFFFFFF

#define MEM_KDATA_BEGIN 0x90000000
#define MEM_KDATA_END  0xFFFEFFFF

/*stack and data segments occupy the same memory space. Stack grows backward (from higher address to lower address) */
#define MEM_STACK_BEGIN 0x7FFFFFFF
#define MEM_STACK_END  0x10010000

typedef struct {
	uint32_t begin, end;
	uint8_t *mem;
} mem_region_t;

/* memory will be dynamically allocated at initialization */
mem_region_t MEM_REGIONS[] = {
	{ MEM_TEXT_BEGIN, MEM_TEXT_END, NULL },
	{ MEM_DATA_BEGIN, MEM_DATA_END, NULL },
	{ MEM_KDATA_BEGIN, MEM_KDATA_END, NULL },
	{ MEM_KTEXT_BEGIN, MEM_KTEXT_END, NULL }
};

#define NUM_MEM_REGION 4
#define MIPS_REGS 32

typedef struct CPU_State_Struct {

  uint32_t PC;		                   /* program counter */
  uint32_t REGS[MIPS_REGS]; /* register file. */
  uint32_t HI, LO;                          /* special regs for mult/div. */
} CPU_State;



/***************************************************************/
/* CPU State info.                                                                                                               */
/***************************************************************/

CPU_State CURRENT_STATE, NEXT_STATE;
int RUN_FLAG;	/* run flag*/
uint32_t INSTRUCTION_COUNT;
uint32_t PROGRAM_SIZE; /*in words*/

char prog_file[32];


/***************************************************************/
/* global variables
/***************************************************************/

//string R[2][32];

/***************************************************************/
/* Function Declerations.                                                                                                */
/***************************************************************/
void help();
uint32_t mem_read_32(uint32_t address);
void mem_write_32(uint32_t address, uint32_t value);
void cycle();
void run(int num_cycles);
void runAll();
void mdump(uint32_t start, uint32_t stop) ;
void rdump();
void handle_command();
void reset();
void init_memory();
void load_program();
void handle_instruction(); /*IMPLEMENT THIS*/
void initialize();
void print_program(); /*IMPLEMENT THIS*/
void print_instruction(uint32_t);
void ADD(int rs, int rt, int rd);
void ADDU(int rs, int rt, int rd);
void ADDI(int rs, int rt, uint32_t address);
void ADDUI(int rs, int rt, uint32_t address);
void SUB(int rs, int rt, int rd);
void SUBU(int rs, int rt, int rd);
void MULT(int rs, int rt, int rd);
void MULTU(int rs, int rt, int rd);
void DIV(int rs, int rt, int rd);
void DIVU(int rs, int rt, int rd);
void AND(int rs, int rt, int rd);
void ANDI(int rs, int rt, uint32_t address);
void OR(int rs, int rt, int rd);
void ORI(int rs, int rt, uint32_t address);
void XOR(int rs, int rt, int rd);
void XORI(int rs, int rt, uint32_t address);
void NOR(int rs, int rt, int rd);
void SLT(int rs, int rt, int rd);
void SLTI(int rs, int rt, uint32_t address);
void SLL(int rs, int rt, int shamt);
void SRL(int rs, int rt, int rd);
void SRA(int rs, int rt, int shamt);
void J(uint32_t j_address);
void JR(uint32_t j_address, int rs);
void JAL(uint32_t j_address);
void JALR(uint32_t j_address, int rs);
