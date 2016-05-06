/*
	SuperCollider real time audio synthesis system
	Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com
	
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
 
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//RedInfo by redFrik 090305

//--changes 091027:
//added RedInfoTmp
//--changes 090308:
//changed RedInfoLmu to use IOConnectMethodScalarIScalarO to make it all run on 10.4
//added RedInfoSms2 that outputs scaled values instead and flips the x on macbooks
//added RedInfoSms3 that outputs raw values

//add CoreFoundation.framework and IOKit.framework to the xcode project
#import <CoreFoundation/CoreFoundation.h>
#import <IOKit/ps/IOPowerSources.h>
#import <IOKit/ps/IOPSKeys.h>

#include <IOKit/IOKitLib.h>
#include <unistd.h>

//add unimotion.c to the xcode project
#include "unimotion.h"	//UniMotion by Lincoln Ramsay

#include "SC_PlugIn.h"
static InterfaceTable *ft;

struct RedInfoBatGlobalState {
	float current, maximum;
	bool flag;
} gRedInfoBatGlobals;

struct RedInfoLmuGlobalState {
	int left, right;
	bool flag;
} gRedInfoLmuGlobals;

struct RedInfoSmsGlobalState {
	double x, y, z;
	bool flag;
} gRedInfoSmsGlobals;

struct RedInfoSms2GlobalState {
	int x, y, z;
	bool flag;
} gRedInfoSms2Globals;

struct RedInfoSms3GlobalState {
	int x, y, z;
	bool flag;
} gRedInfoSms3Globals;

struct RedInfoTmpGlobalState {
	float temp;
	bool flag;
} gRedInfoTmpGlobals;

struct RedInfoBat : public Unit {
	RedInfoBatGlobalState *bat_gstate;
	float prev;
};

struct RedInfoLmu : public Unit {
	RedInfoLmuGlobalState *lmu_gstate;
	float prev;
};

struct RedInfoSms : public Unit {
	RedInfoSmsGlobalState *sms_gstate;
	float prev;
};

struct RedInfoSms2 : public Unit {
	RedInfoSms2GlobalState *sms2_gstate;
	float prev;
};

struct RedInfoSms3 : public Unit {
	RedInfoSms3GlobalState *sms3_gstate;
	float prev;
};

struct RedInfoTmp : public Unit {
	RedInfoTmpGlobalState *tmp_gstate;
	float prev;
};

void *bat_gstate_update_func(void *arg) {
	RedInfoBatGlobalState *bat_gstate= &gRedInfoBatGlobals;
	bat_gstate->flag= true;
	for(;;) {
		if(bat_gstate->flag) {
			bat_gstate->flag= false;
			CFTypeRef info= IOPSCopyPowerSourcesInfo();
			CFArrayRef list= IOPSCopyPowerSourcesList(info);
			if(CFArrayGetCount(list)>0) {
				CFDictionaryRef batt= IOPSGetPowerSourceDescription(info, CFArrayGetValueAtIndex(list, 0));
				const void *pval= (CFStringRef)CFDictionaryGetValue(batt, CFSTR(kIOPSNameKey));
				pval= CFDictionaryGetValue(batt, CFSTR(kIOPSCurrentCapacityKey));
				CFNumberGetValue((CFNumberRef)pval, kCFNumberFloat32Type, &bat_gstate->current);
				pval= CFDictionaryGetValue(batt, CFSTR(kIOPSMaxCapacityKey));
				CFNumberGetValue((CFNumberRef)pval, kCFNumberFloat32Type, &bat_gstate->maximum);
			} else {
				bat_gstate->current= -100.f;
				bat_gstate->maximum= 100.f;
			}
			CFRelease(list);
			CFRelease(info);
		}
		usleep(500000);					//minimum update rate 0.5sec
	}
}

void *lmu_gstate_update_func(void *arg) {
	RedInfoLmuGlobalState *lmu_gstate= &gRedInfoLmuGlobals;
	kern_return_t kr;
	io_service_t serviceObject;
	io_connect_t dataPort= 0;
	lmu_gstate->flag= true;
	
	serviceObject= IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("AppleLMUController"));
	if(serviceObject) {
		kr= IOServiceOpen(serviceObject, mach_task_self(), 0, &dataPort);
		IOObjectRelease(serviceObject);
		if(kr==KERN_SUCCESS) {
			
			uint32_t outputCnt= 2;
			uint64_t scalarI_64[2];
			scalarI_64[0]= 0;
			scalarI_64[1]= 0;
			uint32_t left, right;
			for(;;) {
				if(lmu_gstate->flag) {
					lmu_gstate->flag= false;
#if defined(__LP64__)		//check if 10.5 api available
					kr= IOConnectCallScalarMethod(dataPort, 0, NULL, 0, scalarI_64, &outputCnt);
					if(kr==KERN_SUCCESS) {
						lmu_gstate->left= scalarI_64[0];
						lmu_gstate->right= scalarI_64[1];
					}					
#else						//use 10.4 api
					kr= IOConnectMethodScalarIScalarO(dataPort, 0, 0, 2, &left, &right);
					if(kr==KERN_SUCCESS) {
						lmu_gstate->left= left;
						lmu_gstate->right= right;
					}
#endif
				}
				usleep(17000);					//minimum update rate 17ms
			}
		}
	}
	return 0;
}

void *sms_gstate_update_func(void *arg) {
	RedInfoSmsGlobalState *sms_gstate= &gRedInfoSmsGlobals;
	int type= detect_sms();
	double x, y, z;
	sms_gstate->flag= true;
	for(;;) {
		if(sms_gstate->flag) {
			sms_gstate->flag= false;	
			if(read_sms_real(type, &x, &y, &z)==1) {
				sms_gstate->x= x;
				sms_gstate->y= y;
				sms_gstate->z= z;
			} else {
				sms_gstate->x= 0.f;
				sms_gstate->y= 0.f;
				sms_gstate->z= 0.f;
			}
		}
		usleep(17000);					//minimum update rate 17ms
	}
}

void *sms2_gstate_update_func(void *arg) {
	RedInfoSms2GlobalState *sms2_gstate= &gRedInfoSms2Globals;
	int type= detect_sms();
	int x, y, z;
	sms2_gstate->flag= true;
	for(;;) {
		if(sms2_gstate->flag) {
			sms2_gstate->flag= false;	
			if(read_sms_scaled(type, &x, &y, &z)==1) {
				sms2_gstate->x= x;
				sms2_gstate->y= y;
				sms2_gstate->z= z;
			} else {
				sms2_gstate->x= 0;
				sms2_gstate->y= 0;
				sms2_gstate->z= 0;
			}
		}
		usleep(17000);					//minimum update rate 17ms
	}
}

void *sms3_gstate_update_func(void *arg) {
	RedInfoSms3GlobalState *sms3_gstate= &gRedInfoSms3Globals;
	int type= detect_sms();
	int x, y, z;
	sms3_gstate->flag= true;
	for(;;) {
		if(sms3_gstate->flag) {
			sms3_gstate->flag= false;	
			if(read_sms(type, &x, &y, &z)==1) {
				sms3_gstate->x= x;
				sms3_gstate->y= y;
				sms3_gstate->z= z;
			} else {
				sms3_gstate->x= 0;
				sms3_gstate->y= 0;
				sms3_gstate->z= 0;
			}
		}
		usleep(17000);					//minimum update rate 17ms
	}
}

void *tmp_gstate_update_func(void *arg) {
	RedInfoTmpGlobalState *tmp_gstate= &gRedInfoTmpGlobals;
	kern_return_t kr;
	io_service_t serviceObject;
	io_connect_t dataPort= 0;
	tmp_gstate->flag= true;
	
	serviceObject= IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("AppleSMC"));
	if(serviceObject) {
		kr= IOServiceOpen(serviceObject, mach_task_self(), 0, &dataPort);
		IOObjectRelease(serviceObject);
		if(kr==KERN_SUCCESS) {
			
			uint32_t outputCnt= 2;
			uint64_t scalarI_64[2];
			scalarI_64[0]= 0;
			scalarI_64[1]= 0;
			uint32_t left, right;
			for(;;) {
				if(tmp_gstate->flag) {
					tmp_gstate->flag= false;
#if defined(__LP64__)		//check if 10.5 api available
					kr= IOConnectCallScalarMethod(dataPort, 0, NULL, 0, scalarI_64, &outputCnt);
					if(kr==KERN_SUCCESS) {
						tmp_gstate->temp= scalarI_64[0];
					}					
#else						//use 10.4 api
					kr= IOConnectMethodScalarIScalarO(dataPort, 0, 0, 2, &left, &right);
					if(kr==KERN_SUCCESS) {
						tmp_gstate->temp= left;
					}
#endif
				}
				usleep(500000);					//minimum update rate 0.5sec	???
			}
		}
	}
}

//--

extern "C" {
	void load(InterfaceTable *inTable);
	void RedInfoBat_Ctor(RedInfoBat *unit);
	void RedInfoBat_next(RedInfoBat *unit, int inNumSamples);
	void RedInfoLmu_Ctor(RedInfoLmu *unit);
	void RedInfoLmu_next(RedInfoLmu *unit, int inNumSamples);
	void RedInfoSms_Ctor(RedInfoSms *unit);
	void RedInfoSms_next(RedInfoSms *unit, int inNumSamples);
	void RedInfoSms2_Ctor(RedInfoSms2 *unit);
	void RedInfoSms2_next(RedInfoSms2 *unit, int inNumSamples);
	void RedInfoSms3_Ctor(RedInfoSms3 *unit);
	void RedInfoSms3_next(RedInfoSms3 *unit, int inNumSamples);
	void RedInfoTmp_Ctor(RedInfoTmp *unit);
	void RedInfoTmp_next(RedInfoTmp *unit, int inNumSamples);
}

//--

void RedInfoBat_Ctor(RedInfoBat *unit) {
	SETCALC(RedInfoBat_next);
	unit->bat_gstate= &gRedInfoBatGlobals;
	unit->prev= 0.f;
	RedInfoBat_next(unit, 1);
}

void RedInfoBat_next(RedInfoBat *unit, int inNumSamples) {
	if((unit->prev<=0.f)&&(ZIN0(0)>0.f)) {
		unit->bat_gstate->flag= true;
	}
	unit->prev= ZIN0(0);
	ZOUT0(0)= unit->bat_gstate->current / unit->bat_gstate->maximum;
}

//--

void RedInfoLmu_Ctor(RedInfoLmu *unit) {
	SETCALC(RedInfoLmu_next);
	unit->lmu_gstate= &gRedInfoLmuGlobals;
	unit->prev= 0.f;
	RedInfoLmu_next(unit, 1);
}

void RedInfoLmu_next(RedInfoLmu *unit, int inNumSamples) {
	if((unit->prev<=0.f)&&(ZIN0(0)>0.f)) {
		unit->lmu_gstate->flag= true;
	}
	unit->prev= ZIN0(0);
	ZOUT0(0)= unit->lmu_gstate->left;
	ZOUT0(1)= unit->lmu_gstate->right;
}

//--

void RedInfoSms_Ctor(RedInfoSms *unit) {
	SETCALC(RedInfoSms_next);
	unit->sms_gstate= &gRedInfoSmsGlobals;
	unit->prev= 0.f;
	RedInfoSms_next(unit, 1);
}

void RedInfoSms_next(RedInfoSms *unit, int inNumSamples) {
	if((unit->prev<=0.f)&&(ZIN0(0)>0.f)) {
		unit->sms_gstate->flag= true;
	}
	unit->prev= ZIN0(0);
	ZOUT0(0)= unit->sms_gstate->x;
	ZOUT0(1)= unit->sms_gstate->y;
	ZOUT0(2)= unit->sms_gstate->z;
}

//--

void RedInfoSms2_Ctor(RedInfoSms2 *unit) {
	SETCALC(RedInfoSms2_next);
	unit->sms2_gstate= &gRedInfoSms2Globals;
	unit->prev= 0.f;
	RedInfoSms2_next(unit, 1);
}

void RedInfoSms2_next(RedInfoSms2 *unit, int inNumSamples) {
	if((unit->prev<=0.f)&&(ZIN0(0)>0.f)) {
		unit->sms2_gstate->flag= true;
	}
	unit->prev= ZIN0(0);
	ZOUT0(0)= unit->sms2_gstate->x;
	ZOUT0(1)= unit->sms2_gstate->y;
	ZOUT0(2)= unit->sms2_gstate->z;
}

//--

void RedInfoSms3_Ctor(RedInfoSms3 *unit) {
	SETCALC(RedInfoSms3_next);
	unit->sms3_gstate= &gRedInfoSms3Globals;
	unit->prev= 0.f;
	RedInfoSms3_next(unit, 1);
}

void RedInfoSms3_next(RedInfoSms3 *unit, int inNumSamples) {
	if((unit->prev<=0.f)&&(ZIN0(0)>0.f)) {
		unit->sms3_gstate->flag= true;
	}
	unit->prev= ZIN0(0);
	ZOUT0(0)= unit->sms3_gstate->x;
	ZOUT0(1)= unit->sms3_gstate->y;
	ZOUT0(2)= unit->sms3_gstate->z;
}

//--

void RedInfoTmp_Ctor(RedInfoTmp *unit) {
	SETCALC(RedInfoTmp_next);
	unit->tmp_gstate= &gRedInfoTmpGlobals;
	unit->prev= 0.f;
	RedInfoTmp_next(unit, 1);
}

void RedInfoTmp_next(RedInfoTmp *unit, int inNumSamples) {
	if((unit->prev<=0.f)&&(ZIN0(0)>0.f)) {
		unit->tmp_gstate->flag= true;
	}
	unit->prev= ZIN0(0);
	ZOUT0(0)= unit->tmp_gstate->temp;
}

//--

PluginLoad(InterfaceTable *inTable) {
	ft= inTable;
	pthread_t batListenThread;
	pthread_create(&batListenThread, NULL, bat_gstate_update_func, (void*)0);
	pthread_t lmuListenThread;
	pthread_create(&lmuListenThread, NULL, lmu_gstate_update_func, (void*)0);
	pthread_t smsListenThread;
	pthread_create(&smsListenThread, NULL, sms_gstate_update_func, (void*)0);
	pthread_t sms2ListenThread;
	pthread_create(&sms2ListenThread, NULL, sms2_gstate_update_func, (void*)0);
	pthread_t sms3ListenThread;
	pthread_create(&sms3ListenThread, NULL, sms3_gstate_update_func, (void*)0);
	pthread_t tmpListenThread;
	pthread_create(&tmpListenThread, NULL, tmp_gstate_update_func, (void*)0);
	DefineSimpleUnit(RedInfoBat);
	DefineSimpleUnit(RedInfoLmu);
	DefineSimpleUnit(RedInfoSms);
	DefineSimpleUnit(RedInfoSms2);
	DefineSimpleUnit(RedInfoSms3);
	DefineSimpleUnit(RedInfoTmp);
}
