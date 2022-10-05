#include <Windows.h>
#include "Plugin.h"
#include "Anencephalic_patch.h"
void Patch::fn_init(ULONG_PTR uStartAddress, ULONG_PTR uEndAddress) {
	this->uEndAddress = uEndAddress;
	this->uStartAddress = uStartAddress;
	this->PatchData = new PatchInfo[0x500];
	this->PatchCount = 0;
	this->PatchCurrent = 0;
}

Patch::~Patch()
{
	if(this->PatchData)
	delete this->PatchData;

}

void Patch::fn_add_patch(ULONG_PTR uPatchAddress,int PatchSize,BOOL IsJmp, ULONG_PTR uJmpAddress)
{
	if (PatchCount >= 500) {
		MessageBoxA(0, "Can not Log too many jcc!","Error",MB_OK|MB_ICONERROR);
		return;
	}
		
	this->PatchData[PatchCount].IsJmp = IsJmp;
	this->PatchData[PatchCount].uPatchAddress = uPatchAddress;
	this->PatchData[PatchCount].uJmpAddress = uJmpAddress;
	this->PatchData[PatchCount].PatchSize = PatchSize;

	PatchCount++;
}

void Patch::fn_patch_data()
{
	if (PatchCount == PatchCurrent) {
		MessageBoxA(0, "Patch Over!", "Info", MB_OK | MB_ICONINFORMATION);
		return;
	}
	UCHAR e9Jmp = 0xe9;
	UCHAR ebJmp = 0xeb;
	//�㹻�� ����Patchʱ���е���
	UCHAR noparry[] = { 0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90 };



	if (PatchData[PatchCurrent].IsJmp) {
		
		//�޸���ת
		//�ĳ�Jmp
		if (PatchData[PatchCurrent].PatchSize == 2) {
			//˵���ǽ���ת
			BYTE offset = PatchData[PatchCurrent].uJmpAddress - PatchData[PatchCurrent].uPatchAddress - PatchData[PatchCurrent].PatchSize;
			_Writememory((void*)&ebJmp,PatchData[PatchCurrent].uPatchAddress, 1, MM_RESTORE);
			_Writememory((void*)&offset, PatchData[PatchCurrent].uPatchAddress+1, 1, MM_RESTORE);
		}
		else {
			//Զ��ת ���ֽ�
			int offset= PatchData[PatchCurrent].uJmpAddress - PatchData[PatchCurrent].uPatchAddress - PatchData[PatchCurrent].PatchSize;
			_Writememory((void*)&e9Jmp, PatchData[PatchCurrent].uPatchAddress, 1, MM_RESTORE);
			_Writememory((void*)&offset, PatchData[PatchCurrent].uPatchAddress + 1, 4, MM_RESTORE);
		}
		

	}
	else {
		//����ת ֱ�Ӹĳ�nop����
		_Writememory(noparry,PatchData[PatchCurrent].uPatchAddress,  PatchData[PatchCurrent].PatchSize, MM_RESTORE);
	}
	PatchCurrent++;

}
