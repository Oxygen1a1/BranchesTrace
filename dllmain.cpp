
#define  _CRT_SECURE_NO_WARNINGS 
#include <stdio.h>
#include <Windows.h>
#include <math.h>
#include "Plugin.h"
#include "Anencephalic_patch.h"
//宏
#define  JCCINIT 0
#define  ABOUT 1


//OD主界面句柄 很多函数需要 在Plugininit中参数
HWND g_hWnd;
ULONG_PTR g_uEndTrace;
BOOL bIsTracing;
//存放模块基质和大小
ULONG_PTR uModuleBase,uModuleSize;
char szText[100];//输入
t_module* tracemodule;//防止步入非本模块
BOOL bIsPatching;
//无脑Patch
Patch patch;

ULONG_PTR GetEndTraceAddress();
BOOL StrBeginWith(const char* szStr1, const char* szStr2);

//======================================必须导出的函数=======================================================

//************************************
// Method:菜单中显示的插件名
// Description: 必须的导出函数
//************************************
extern "C" __declspec(dllexport) cdecl int ODBG_Plugindata(char shortname[32]) {
	const char* szPlugname = "OxygenPlugin_v1.0";
	strcpy(shortname, szPlugname);
	return PLUGIN_VERSION;
}
//************************************
// Method:插件初始化，用于判断当前OD版本号和插件所支持的版本是否一致
// Description:必须的导出函数 
//************************************
extern "C" __declspec(dllexport) cdecl int  ODBG_Plugininit(int ollydbgversion, HWND hw, ulong * features) {
	if (ollydbgversion<PLUGIN_VERSION) {
		MessageBoxA(0, "Ollydbg Version Error!", "Error", MB_OK | MB_ICONERROR);
		return -1;
	}
	g_hWnd = hw;
	return 0;
}
//======================================重要的导出函数=======================================================


//************************************
// Method:显示菜单项
// Description:显示对应的菜单选项
//************************************
extern "C" __declspec(dllexport) cdecl int ODBG_Pluginmenu(int origin, char data[4096], void* item) {
	//origin是指在哪显示 比如PM_MAIN 在最上面的
	//PM_DISASM 在反汇编
	//所以应该是不止一次调用这个回调函数
	switch (origin) {
		case PM_MAIN: {
			const char* szMenu = "BranchesTrace{0&TraceInit,2&ForceStop,3&Anencephalic patch,1&About}";
			strcpy(data, szMenu);
			break;
		}
		case PM_DISASM: {
			const char* szMenu = "BranchesTrace{0&TraceInit,2&ForceStop,3&Anencephalic patch,1&About}";
			strcpy(data, szMenu);
			break;
		}

	}

	return 1;
}


//************************************
// Method:菜单选项的回调
// Description:回调对应选项
//************************************
extern "C" __declspec(dllexport) cdecl void ODBG_Pluginaction(int origin, int action, void* item) {
	switch (action)
	{
	case JCCINIT: {
		//开始追踪
		bIsTracing = true;
		t_dump* t_cpudisam = (t_dump*)_Plugingetvalue(VAL_CPUDASM);
		
		g_uEndTrace = GetEndTraceAddress();

		//初始化无脑Patch对象
	
		patch.fn_init(t_cpudisam->sel0, g_uEndTrace);
		//初始化module
		tracemodule = _Findmodule(g_uEndTrace);
		_Go(0,0,STEP_IN,0,0);
		break;
	}
	case ABOUT: {
		MessageBoxA(0, "OxygenPlugin_v1.0\nsomething sundry func\nBy:Oxygen E-Mail:304914289@qq.com\n", "Info", MB_OK | MB_ICONINFORMATION);
		break;
	}
	case 2: {
		//强制停止
		bIsTracing = 0;
		bIsPatching = 0;
		break;
	}
	case 3: {
		//无脑Patch
		bIsPatching = 1;
		bIsTracing = 0;
		_Go(0, 0, STEP_IN, 0, 0);
		break;
	}
	default:
		break;
	}

}

//OD各种操作所进行的回调
extern "C" __declspec(dllexport) int cdecl ODBG_Pausedex(int reasonex, int dummy, t_reg * reg, DEBUG_EVENT * debugevent) {
	
	

	
	
	
	if (!bIsTracing && !bIsPatching) return 1;

	//正在追踪
	//获取当前选择的反汇编行
	t_dump* t_cpudisam = (t_dump*)_Plugingetvalue(VAL_CPUDASM);
	t_disasm t_dis;
	UCHAR uOpcode[MAXCMDSIZE];
	ulong uSelLen = t_cpudisam->sel1 - t_cpudisam->sel0;
	


	_Readmemory(uOpcode, t_cpudisam->sel0, uSelLen, MM_RESILENT);
	_Disasm(uOpcode, uSelLen, t_cpudisam->sel0, 0, &t_dis, DISASM_ALL, _Plugingetvalue(VAL_MAINTHREADID));

	if(bIsPatching){
		if (patch.PatchData[patch.PatchCurrent].uPatchAddress == t_cpudisam->sel0) {
			//是Patch的地方
			patch.fn_patch_data();
			_Go(0, 0, STEP_IN, 0, 0);
			return 1;

		}

	}
	if (t_cpudisam->sel0 == g_uEndTrace) {
		//结束追踪
		MessageBoxA(0, "Trace end!", "Oxygen", MB_OK | MB_ICONINFORMATION);

		//记录和Patch都给弄掉
		bIsTracing = 0;
		bIsPatching = 0;
		return 0;
	}



	//先判断Call Jmp的地址是否属于自身的模块
	//只要Call Jmp 都会有jmpaddr
	if (!t_dis.jmpaddr) {
		//如果为0 没必要记录
		_Go(0, 0, STEP_IN, 0, 0);
		return 1;
	}

	auto test=_Findmodule(t_dis.jmpaddr);
	if (test) {
		if (test->base != tracemodule->base) {
			_Go(0, 0, STEP_OVER, 0, 0);
			return 1;
		}

	}
	//就算等于也未必是本模块
	//有可能是跳转表
	UCHAR uTmp[2];
	_Readmemory(uTmp, t_dis.jmpaddr, 2, MM_RESILENT);

	if (uTmp[0] == 0xff && uTmp[1] == 0x25) {
		//跳转表 大概率是非本模块 直接步过
		_Go(0, 0, STEP_OVER, 0, 0);
		return 1;
	}


	//可能是int3 也可能是单步异常
	if (reasonex == PP_SINGLESTEP || reasonex == PP_INT3BREAK) {
		if (StrBeginWith(t_dis.result, "j")) {

			if (StrBeginWith(t_dis.result, "jmp")) {
				_Go(0, 0, STEP_IN, 0, 0);
				return 1;
			}
			

			//记录
			if (StrBeginWith("跳转已实现", t_dis.opinfo[1])) {

				patch.fn_add_patch((ULONG_PTR)t_cpudisam->sel0,(int)uSelLen, 1,(ULONG_PTR)t_dis.jmpaddr);
			}
			else {
				//未实现跳转
				patch.fn_add_patch((ULONG_PTR)t_cpudisam->sel0, (int)uSelLen, 0, 0);
			}
			_Addtolist(t_cpudisam->sel0, 1, t_dis.opinfo[1]);
		}
		
	}

	//继续单步
	_Go(0, 0, STEP_IN, 0, 0);
	return 1;

}

ULONG_PTR GetEndTraceAddress()
{
	MessageBoxA(0, "Please input address format be like '4010A0'", "Warning", MB_OK | MB_ICONINFORMATION);
	ULONG_PTR uRet=0;
	//置0
	unsigned int nStrlen = 0;
	char* szTitle = new char[100];

	strcpy(szTitle, "[BranchesTrace]:please input endtrace address");
	if (_Gettext(szTitle, szText, 0, NM_NONAME, 0) == 0) return 0;

	//转换
	nStrlen = strlen(szText);
	if (!nStrlen) return 0;

	for (int i = nStrlen - 1; i >= 0; i--) {
		if(szText[i]>= '0' && szText[i]<='9')
		uRet += (szText[i]-48) * (unsigned int)pow(0x10, nStrlen - i - 1);
		if(szText[i]>='A' && szText[i]<='F')
		uRet += (szText[i] - 55) * (unsigned int)pow(0x10, nStrlen - i - 1);
	}

	
	delete szTitle;

	return uRet;

}

BOOL StrBeginWith(const char* szStr1,const char* szStr2)
{
	int nStrlen = (strlen(szStr1) > strlen(szStr2)) ? strlen(szStr2) : strlen(szStr1);

	return strncmp(szStr1, szStr2, nStrlen) == 0;

}


