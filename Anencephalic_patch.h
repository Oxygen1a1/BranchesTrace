#pragma once

struct PatchInfo
{
	ULONG_PTR uPatchAddress;
	BOOL IsJmp;
	ULONG_PTR uJmpAddress;
	int PatchSize;
};


class Patch {
	ULONG_PTR uStartAddress;
	ULONG_PTR uEndAddress;
public:
	PatchInfo* PatchData;
	int PatchCurrent;
	int PatchCount;
public:
	void fn_init(ULONG_PTR uStartAddress, ULONG_PTR uEndAddress);
	~Patch();
	void fn_add_patch(ULONG_PTR uPatchAddress,int PatchSize,BOOL IsJmp=0,ULONG_PTR uJmpAddress=0);
	void fn_patch_data();
};