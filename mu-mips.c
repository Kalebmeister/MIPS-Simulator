#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <inttypes.h>

#include "mu-mips.h"

/***************************************************************/
/* global variables
***************************************************************/

// array R is holding the data of the temp registers since we dont know the address values the temp registers would have
// {"zero","at","v0","v1","a0","a1","a2","a3","t0","t1","t2","t3","t4","t5","t6","t7","s0","s1","s2","s3","s4","s5","s6","s7","t8","t9","k0","k1","gp","sp","fp","ra"};
int R[32];
char RegNames[32][5]={"zero","at","v0","v1","a0","a1","a2","a3","t0","t1","t2","t3","t4","t5","t6","t7","s0","s1","s2","s3","s4","s5","s6","s7","t8","t9","k0","k1","gp","sp","fp","ra"};

//Opcodes / Funct codes
// ALU
#define ADD 	0x20
#define ADDU	0x21
#define ADDI	0x08
#define	ADDIU	0x09
#define SUB		0x22
#define SUBU	0x23
#define MULT	0x18
#define MULTU	0x19
#define DIV		0x1a
#define DIVU	0x1b
#define AND		0x24
#define ANDI	0x08
#define OR		0x25
#define ORI		0x0d
#define XOR		0x66
#define XORI	0x4e
#define NOR		0x27
#define SLT		0x2a
#define SLTI	0x0a
#define SLL		0x00
#define SRL		0x02
#define SRA		0x03

// Load/Store
#define LW		0x23
#define LB		0x20
#define LH		0x21
#define SW		0x2b
#define SB		0x28
#define SH		0x29
#define MFHI	0x10
#define MFLO	0x12
#define MTHI	0x11
#define MTLO	0x13

// Control Flow

#define BEQ		0x04
#define BNE		0x05
#define BLEZ	0x06
//#define BLTZ	0x
//#define BGEZ	0x do these even exist??
#define BGTZ	0x07
#define J		0x02
#define JR		0x08
#define JAL		0x03
#define JALR	0x08


/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_instruction();
	CURRENT_STATE = NEXT_STATE;
	INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			runAll(); 
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/***********************************************
    ALU functions
	rs rd rt the int that will determine which temp variable value you are using
***********************************************/

void add(int rs, int rt, int rd)
{
	R[rd] = R[rs] + R[rt];
}
void addu(int rs, int rt, int rd)
{
	unsigned int urs = (unsigned int)R[rs];
	unsigned int urt = (unsigned int)R[rt];
	unsigned int urd = (unsigned int)R[rd];
	R[urd] = R[urs] + R[urt];
}
void addi(int rs, int rt, uint32_t address)
{
	int32_t value = mem_read_32(address);
	R[rt] = R[rs] + value; 
}
void addiu(int rs, int rt, uint32_t address)
{
	uint32_t value = mem_read_32(address);
	R[rt] = R[rs] + value;
}
void sub(int rs, int rt, int rd)
{
	R[rd] = R[rs] - R[rt];
}
void subu(int rs, int rt, int rd)
{
	unsigned int urs = (unsigned int)R[rs];
	unsigned int urt = (unsigned int)R[rt];
	unsigned int urd = (unsigned int)R[rd];
	R[urd] = R[urs] - R[urt]; 
}
void mult(int rs, int rt, int rd)
{
	int value = R[rs] * R[rt];
	CURRENT_STATE.HI = 32 >> value;
	CURRENT_STATE.LO = 32 << value;
}
void multu(int rs, int rt, int rd)
{
	unsigned int urt = (unsigned int)R[rt];
	unsigned int urs = (unsigned int)R[rd];
	int value = R[urs] * R[urt];
	CURRENT_STATE.HI = 32 >> value;
	CURRENT_STATE.LO = 32 << value;
}
void div1(int rs, int rt, int rd)
{
	CURRENT_STATE.HI = R[rs] % R[rt];
	CURRENT_STATE.LO = R[rs] / R[rt];
}
void divu(int rs, int rt, int rd)
{
	unsigned int urs = (unsigned int)R[rs];
	unsigned int urt = (unsigned int)R[rt];
	unsigned int urd = (unsigned int)R[rd];
	CURRENT_STATE.HI = R[urs] % R[urt];
	CURRENT_STATE.LO = R[urs] / R[urt];
}
void and(int rs, int rt, int rd)
{
	R[rd] = R[rs] && R[rt];
}
void andi(int rs, int rt, uint32_t address)
{
	int32_t value = mem_read_32(address);
	unsigned int urs = (unsigned int)R[rs];
	unsigned int urt = (unsigned int)R[rt];
	R[urt] = R[urs] && value;
}
void or(int rs, int rt, int rd)
{
	R[rd] = R[rs] || R[rt];
}
void ori(int rs, int rt, uint32_t address)
{
	int32_t value = mem_read_32(address);
	unsigned int urs = (unsigned int)R[rs];
	unsigned int urt = (unsigned int)R[rt];
	R[urt] = R[urs] || value;
}
void xor(int rs, int rt, int rd)
{
	R[rd] = (R[rs] ^ R[rt]);
}
void xori(int rs, int rt, uint32_t address)
{
	int32_t value = mem_read_32(address);
	unsigned int urs = (unsigned int)R[rs];
	unsigned int urt = (unsigned int)R[rt];
	R[urt] = (R[urs] ^ value);
}
void nor(int rs, int rt, int rd)
{
	R[rd] = ~(R[rs] | R[rt]);
}
void slt(int rs, int rt, int rd)
{
	R[rd] = (R[rs] < R[rt]);
}
void slti(int rs, int rt, uint32_t address)
{
	int32_t value = mem_read_32(address);
	unsigned int urs = (unsigned int)R[rs];
	unsigned int urt = (unsigned int)R[rt];
	R[urt] = (R[urs] < value);
}
void sll(int rs, int rt, int shamt)
{
	R[rt] = R[rs] << R[shamt];
}
void srl(int rs, int rt, int rd)
{
	R[rd] = R[rs] << R[rt];
}
void sra(int rs, int rt, int shamt)
{
	R[rt] = R[rs] >> R[shamt];
}

/*****************************************************************/
/* Load and store instructions
*****************************************************************/

void lw(int rs, int rt, uint32_t address)
{
	int32_t value = mem_read_32(address);
	unsigned short SignExtImmm = value & 0xFFFF;
	R[rt] = R[rs] + SignExtImmm;

}
void lb(int rs, int rt, uint32_t address)
{
	 //R[rt-1] = {24'b0, M[R[rs-1] + SignExtImm](7:0)}
	 int32_t value = mem_read_32(address);
	 unsigned short SignExtImmm = value & 0xFFFF;
	 unsigned int urs = (unsigned int)R[rs];
	 unsigned int urt = (unsigned int)R[rt];
	 R[rt] = R[rs] + SignExtImmm;
	 R[rt] >> 24;
}

void lh(int rs, int rt, uint32_t address)
{
	//R[rt-1] = {16'b0, M[R[rs-1] + SignExtImm](15:0)}
	int32_t value = mem_read_32(address);
	unsigned short SignExtImmm = value & 0xFFFF;
	//printf("This is %d\n", SignExtImmm);
	R[rt] = R[rs] + SignExtImmm;
	R[rt] >> 16;
}
void lui(int rt, uint32_t address)
{
	//R[rt-1] = {imm, 16'b0}
	//$1 = 100x2^16
	int32_t value = mem_read_32(address);
	unsigned short SignExtImmm = value & 0xFFFF;
	R[rt] = SignExtImmm >> 16;
}
void sw(int rs, int rt, uint32_t address)
{
	int32_t value = mem_read_32(address);
	unsigned short SignExtImmm = value & 0xFFFF;
	R[rt] = R[rs] + SignExtImmm;
	//M[R[rs-1] + SignExtImm] = R[rt-1]
}
void sb(int rs, int rt, uint32_t address)
{
	//M[R[rs-1]+SignExtImm](7:0) = R[rt-1](7:0)
	int32_t value = mem_read_32(address);
	unsigned short SignExtImmm = value & 0xFFFF;
	R[rt] = R[rs] + SignExtImmm;
}
void sh(int rs, int rt, uint32_t address)
{
	//M[R[rs-1]+SignExtImm](15:0) = R[rt-1](15:0)
	int32_t value = mem_read_32(address);
	unsigned short SignExtImmm = value & 0xFFFF;
	R[rt] = R[rs] + SignExtImmm;
}
void mfhi(int rd)
{
	//R[rd-1] = Hi
	R[rd] = CURRENT_STATE.HI;
}
void mflo(int rd)
{
	//R[rd] = Lo
	
	R[rd] = CURRENT_STATE.LO;
}




/****************************************************************/
/* Control Flow Instructions
****************************************************************/

void beq(int rs, int rt, uint32_t b_address)
{
	if(R[rs] == R[rt])
	{
		CURRENT_STATE.PC = CURRENT_STATE.PC + 4 + b_address;
	}
}
void bne(int rs, int rt, uint32_t b_address)
{
	if(R[rs] != R[rt])
	{
		CURRENT_STATE.PC = CURRENT_STATE.PC + 4 + b_address;

	}
}
void blez(int rs, int rt, int label)
{
	//ble
	if(R[rs] <= R[rt])
	{
		CURRENT_STATE.PC = label;
	}
}
void bltz(int rs, int rt, int label)
{
	//blt
	if(R[rs] < R[rt])
	{
		CURRENT_STATE.PC = label;
	}
}
void j(uint32_t j_address)
{
	CURRENT_STATE.PC = j_address; 
}
void jr(uint32_t j_address, int rs)
{
	CURRENT_STATE.PC = R[rs];
}
void jal(uint32_t j_address)
{
	R[31] = CURRENT_STATE.PC + 8; CURRENT_STATE.PC = j_address;
}
void jalr(uint32_t j_address, int rs)
{
	R[31] = CURRENT_STATE.PC + 8; CURRENT_STATE.PC = R[rs];
}


/***************************************************************/
/* System call
***************************************************************/
//SYSCALL
void syscall()
{
	exit(0);
}

/**************************************************************/
/* Register Selection
**************************************************************/

void fill_reg()
{
	int i = 0;
	while(i < 32)
	{
		R[i] = 0;
		i++;
	}
}

/***********************************************************/
/* bit allocation for instruction set
***********************************************************/

// one is needed for J / I

/***********************************************************/
/* set the temp registers with the 
***********************************************************/





	/*IMPLEMENT THIS*/
	/* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/

void parseInstruction(uint32_t addr)
{
	
	int opCode = mem_read_32(addr) >> 26;
	int data = mem_read_32(addr);

	//R-type format
	if (opCode == 0)
	{	
		//shift and bit mask to get register values

		int rs = (data >> 21) & 0x1f;
		int rt = (data >> 16) & 0x1f;
		int rd = (data >> 11) & 0x1f;
		int sa = (data >> 6) & 0x1f;
		int func = (data >> 0) & 0x1f;
	
		if (func == ADD)
		{
			add(rs,rt,rd);
		}
		else if (func == ADDU)
		{
			addu(rs,rt,rd);

		}
		
		else if (func == SUB)
		{
			sub(rs,rt,rd);

		}
		else if (func == SUBU)
		{
			subu(rs,rt,rd);

		}
		else if (func == MULT)
		{
			mult(rs,rt,rd);

		}
		else if (func == MULTU)
		{
			multu(rs,rt,rd);

		}
		else if (func == DIV)
		{
			div1(rs,rt,rd);

		}
		else if (func == DIVU)
		{
			divu(rs,rt,rd);

		}
		else if (func == AND)
		{
			and(rs,rt,rd);

		}
		else if (func == OR)
		{
			or(rs,rt,rd);

		}
		else if (func == NOR)
		{
			nor(rs,rt,rd);
		}
		else if (func == SLT)
		{
			slt(rs,rt,rd);

		}
		else if (func == SLL)
		{
			sll(rs,rt,sa);

		}
		else if (func == SRL)
		{
			srl(rs,rt,rd);

		}
		else if (func == SRA)
		{
			sra(rs,rt,sa);

		}
		
	}
	
	//I format
	else {
			int rs = (data >> 21) & 0x1f;
			int rt = (data >> 16) & 0x1f;
			uint32_t immediate = data & 0xFFFF;

			if (opCode == ADDI)
			{	
			addi(rs,rt,immediate);

			}
			else if (opCode == ANDI)
			{
			andi(rs,rt,immediate);

			}
			else if (opCode == ORI)
			{
			ori(rs,rt,immediate);

			}
			else if (opCode == XORI)
			{
			xori(rs,rt,immediate);

			}
			else if (opCode == ADDIU)
			{
			addiu(rs,rt,immediate);

			}
			else if (opCode == SLTI)
			{
			slti(rs,rt,immediate);

			}
			else if (opCode == LW)
			{
				lw(rs,rt,immediate);
			}
			else if (opCode == LB)
			{
				lb(rs,rt,immediate);
			}
			else if (opCode == LH)
			{
				lh(rs,rt,immediate);
			}
			else if (opCode == SW)
			{
				sw(rs,rt,immediate);
			}
			else if (opCode == SB)
			{
				sb(rs,rt,immediate);
			}
			else if (opCode == SH)
			{
				sh(rs,rt,immediate);
			}
			else if (opCode == MFHI)
			{
			//	mfhi(rd,HI);
			}
			else if (opCode == MFLO)
			{
			//	mflo(rd);
			}
			else if (opCode == MTHI)
			{
			//	mthi(rs,rt,immediate);
			}
			else if (opCode == MTLO)
			{
			//	mtlo(rs,rt,immediate);
			}
		}
		
}
/************************************************************/
/* decode and execute instruction                                                                     */ 
/************************************************************/
void handle_instruction()
{
	//int i;
	uint32_t addr;
		
	parseInstruction(addr);
	CURRENT_STATE = NEXT_STATE;

}
/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (infMIPS assembly format)    */ 
/************************************************************/
void print_program(){
	int i;
	uint32_t addr = mem_read_32(addr);
	
	for(i=0; i<PROGRAM_SIZE; i++){
		addr = MEM_TEXT_BEGIN + (i*4);
		printf("[0x%x]\t", addr);
		print_instruction(addr);
	}
}

/************************************************************/
/* Print the instruction at given memory address (in MIPS assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){
	
	int opCode = mem_read_32(addr) >> 26;
	int data = mem_read_32(addr);
	//printf("data" "%d" "\n ",data);
	//printf("opcode" "%d" "\n ",opCode);
	//R-type format
	if (opCode == 0)
	{	
		//shift and bit mask to get register values

		int rs = (data >> 21) & 0x1f;
		int rt = (data >> 16) & 0x1f;
		//printf("%d",rs);
		int rd = (data >> 11) & 0x1f;
		int sa = (data >> 6) & 0x1f;
		int func = data & 0x3f;
		//printf("%d\n",data);
		//printf("%d",func);
		//func = func >> 26;
		//printf("function:" "%d" "\n" ,func);
		int i=0;
		if (func == ADD)
		{
			printf("ADD ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);	
		}
		else if (func == ADDU)
		{
			printf("ADDU ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);
			

		}
		
		else if (func == SUB)
		{
			printf("SUB ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);

		}
		else if (func == SUBU)
		{
			printf("SUBU ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);

		}
		else if (func == MULT)
		{
			printf("MULT ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);

		}
		else if (func == MULTU)
		{
			printf("MULTU ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);

		}
		else if (func == DIV)
		{
			printf("DIV ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);

		}
		else if (func == DIVU)
		{
			printf("DIVU ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);	
			

		}
		else if (func == AND)
		{
			printf("AND ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);

		}
		else if (func == OR)
		{
			printf("OR ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);

		}
		else if (func == NOR)
		{
			printf("NOR ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);
		}
		else if (func == SLT)
		{
			printf("SLT ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);
			

		}
		else if (func == SLL)
		{
			printf("SLL ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);

		}
		else if (func == SRL)
		{
			printf("SRL ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);

		}
		else if (func == SRA)
		{
			printf("SRA ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%s\n", RegNames[rd-1]);

		}
		

	}
	//I format
	else {
			int rs = (data >> 21) & 0x1f;
			int rt = (data >> 16) & 0x1f;
			int immediate = data & 0xFFFF;

			if (opCode == ADDI)
			{	
			printf("ADDI ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%04x\n", immediate);

			}
			else if (opCode == ANDI)
			{
			printf("ANDI ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%04x\n", immediate);
				

			}
			else if (opCode == ORI)
			{
			printf("ORI ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%04x\n", immediate);
			}
			else if (opCode == XORI)
			{
			printf("XORI ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%04x\n", immediate);

			}
			else if (opCode == ADDIU)
			{
			printf("ADDIU ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%04x\n", immediate);

			}
			else if (opCode == SLTI)
			{
			printf("SLTI ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%04x\n", immediate);

			}
			else if (opCode == LW)
			{
			printf("LW ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%04x\n", immediate);
			}
			else if (opCode == LB)
			{
			printf("LB ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%04x\n", immediate);
			}
			else if (opCode == LH)
			{
			printf("LH ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%04x\n", immediate);
			}
			else if (opCode == SW)
			{
			printf("SW ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%04x\n", immediate);
			}
			else if (opCode == SB)
			{
			printf("SB ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%04x\n", immediate);
			}
			else if (opCode == SH)
			{
			printf("SH ");
			printf("%s, ", RegNames[rs-1]);
			printf("%s, ", RegNames[rt-1]);
			printf("%04x\n", immediate);
			}
			else if (opCode == MFHI)
			{
			//	mfhi(rd,HI);
			}
			else if (opCode == MFLO)
			{
			//	mflo(rd);
			}
			else if (opCode == MTHI)
			{
			//	mthi(rs,rt,immediate);
			}
			else if (opCode == MTLO)
			{
			//	mtlo(rs,rt,immediate);
			}
		}
	
}
/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
