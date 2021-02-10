/* Copyright 1990-2006, Jsoftware Inc.  All rights reserved.               */
/* Licensed use only. Any other use is in violation of copyright.          */
/*                                                                         */
/* Xenos: Miscellaneous                                                    */

#include "j.h"
#include "x.h"

#include "cpuinfo.h"
extern uint64_t g_cpuFeatures;

#include <string.h>
#ifdef _WIN32
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

F1(jtassertq){ASSERTMTV(w); R scb(jt->assert);}

F1(jtasserts){B b; RE(b=b0(w)); jt->assert=b; R mtm;}

F1(jtdirectdefq){ASSERTMTV(w); R scb(jt->directdef);}  // scaf 9!:62

F1(jtdirectdefs){B b; RE(b=b0(w)); jt->directdef=b; R mtm;}  // scaf 9!:63

F1(jtboxq){ASSERTMTV(w); R str(sizeof(jt->bx),jt->bx);}

F1(jtboxs){A x;
 RZ(w=vs(w));
 ASSERT(sizeof(jt->bx)==AS(w)[0],EVLENGTH);
// obsolete  x=jt->bxa; RZ(ras(w)); RZ(jt->bxa=w); jt->bx=CAV(jt->bxa); fa(x);
 MC(jt->bx,CAV(w),sizeof(jt->bx));
 R mtv;
}  // box-display characters

F1(jtctq){ASSERTMTV(w); R scf(1.0-jt->cct);}

F1(jtcts){D d;
 ASSERT(!AR(w),EVRANK);
 RZ(w=cvt(FL,w)); d=DAV(w)[0];
 ASSERT(0<=d,EVDOMAIN); 
 ASSERT(d<=5.820766091e-11,EVDOMAIN);
 jt->cctdefault=jt->cct=1.0-d;
 R mtv;
}

F1(jtdispq){A z; ASSERTMTV(w); GATV0(z,INT,*jt->disp,1); ICPY(AV(z),1+jt->disp,*jt->disp); R z;}

F1(jtdisps){UC n;
 RZ(w=vi(w));
 n=(UC)AN(w);
 ASSERT(1>=AR(w),EVRANK);
 ASSERT(all1(nubsieve(w)),EVDOMAIN);
 ASSERT(all1(eps(w,eval("1 2 4 5 6"))),EVINDEX);
 *jt->disp=n; DO(n, jt->disp[1+i]=(UC)IAV(w)[i];);
 R mtv;
}

F1(jtevmq){ASSERTMTV(w); R behead(jt->evm);}

F1(jtevms){A t,*tv,*wv;
 ARGCHK1(w);
 ASSERT(1==AR(w),EVRANK);
 ASSERT(NEVM==AN(w),EVLENGTH);
 ASSERT(BOX&AT(w),EVDOMAIN);
 GAT0(t,BOX,1+NEVM,1); tv=AAV(t); 
 *tv++=mtv;
 wv=AAV(w);
 DQ(NEVM, RZ(*tv=ca(vs(*wv))); CAV(*tv)[AN(*tv)]=0; ++tv; ++wv;);  // NUL-terminate.  ca to make sure there's room
 ras(t); fa(jt->evm); jt->evm=t;
 R mtv;
}

F1(jtfxx){
 ARGCHK1(w);
 ASSERT(AT(w)&LIT+BOX,EVDOMAIN);
 ASSERT(1>=AR(w),EVRANK);
 R fx(ope(w)); 
}

F1(jtiepdoq){ASSERTMTV(w); R scb(jt->iepdo);}

F1(jtiepdos){B b; RE(b=b0(w)); jt->iepdo=b; R mtm;}

F1(jtiepq){
 ASSERTMTV(w); 
 ASSERT(1==AR(w),EVRANK);
 ASSERT(!AN(w),EVDOMAIN); 
 R jt->iep?jt->iep:mtv;
}

F1(jtieps){
 ARGCHK1(w);
 ASSERT(1>=AR(w),EVRANK);
 ASSERT(!AN(w)||AT(w)&LIT,EVDOMAIN);
 fa(jt->iep);
 RZ(ras(w)); RZ(jt->iep=w); 
 R mtm;
}

F1(jtoutparmq){A z;D*u;I*v;
 ASSERTMTV(w);
 if(IMAX==jt->outmaxlen||IMAX==jt->outmaxbefore||IMAX==jt->outmaxafter){
  GAT0(z,FL, 4,1); u=DAV(z);
  u[0]=(D)jt->outeol;
  u[1]=jt->outmaxlen==IMAX?inf:(D)jt->outmaxlen;
  u[2]=jt->outmaxbefore==IMAX?inf:(D)jt->outmaxbefore;
  u[3]=jt->outmaxafter==IMAX?inf:(D)jt->outmaxafter;
 }else{
  GAT0(z,INT,4,1); v= AV(z);
  v[0]=jt->outeol;
  v[1]=jt->outmaxlen;
  v[2]=jt->outmaxbefore;
  v[3]=jt->outmaxafter;
 }
 RETF(z);
}

F1(jtoutparms){I*v;
 RZ(w=vib(w));
 ASSERT(1==AR(w),EVRANK);
 ASSERT(4==AN(w),EVLENGTH);
 v=AV(w);
 ASSERT(0==v[0]||2==v[0],EVINDEX);
 ASSERT(0<=v[1],EVDOMAIN);
 ASSERT(0<=v[2],EVDOMAIN);
 ASSERT(0<=v[3],EVDOMAIN);
 jt->outeol      =(UC)v[0];
 jt->outmaxlen   =v[1];
 jt->outmaxbefore=v[2];
 jt->outmaxafter =v[3];
 R mtv;
}

F1(jtposq){ASSERTMTV(w); R v2(jt->pos[0],jt->pos[1]);}

F1(jtposs){I n,p,q,*v;
 RZ(w=vi(w));
 n=AN(w); v=AV(w);
 ASSERT(1>=AR(w),EVRANK);
 ASSERT(1==n||2==n,EVLENGTH);
 if(1==n)p=q=*v; else{p=v[0]; q=v[1];} 
 ASSERT(BETWEENC(p,0,2)&&BETWEENC(q,0,2),EVDOMAIN);
 jt->pos[0]=p; jt->pos[1]=q;   
 R mtv;
}

F1(jtppq){
 ASSERTMTV(w);
 R sc(jt->pp);
}

F1(jtpps){I k;
 RE(sc(k=i0(w))); ASSERT(0<k,EVDOMAIN); ASSERT(k==22||k<=NPP,EVLIMIT);
 jt->pp=k;
 R mtv;
}

F1(jtretcommq){ASSERTMTV(w); R scb(jt->retcomm);}

F1(jtretcomms){B b; RE(b=b0(w)); jt->retcomm=b; R mtm;}

F1(jtseclevq){ASSERTMTV(w); R sc(jt->seclev);}

F1(jtseclevs){I k; 
 RE(k=i0(w)); 
 ASSERT(0==k||1==k,EVDOMAIN); 
 if(!jt->seclev&&1==k)jt->seclev=(UC)k;
 R mtm;
}

#if 0  // not implemented
F1(jtsysparmq){I k;
 RE(k=i0(w));
 switch(k){
  default: ASSERT(0,EVINDEX);
  case 0:  R sc(jt->fdepn);
  case 1:  R sc(jt->fdepi);
  case 2:  R sc(jt->fcalln);
  case 3:  R sc(jt->callstacknext);
}}

F1(jtsysparms){A*wv;I k,m;
 ARGCHK1(w);
 ASSERT(BOX&AT(w),EVDOMAIN);
 ASSERT(1==AR(w),EVRANK);
 ASSERT(2==AN(w),EVLENGTH);
 wv=AAV(w); 
 RE(k=i0(wv[0]));
 switch(k){
  default: ASSERT(0,EVINDEX);
  case 0:  RE(m=i0(wv[1])); jt->fdepn =(I4)m; break;
  case 1:  ASSERT(0,EVDOMAIN);  /* jt->fdepi  can not be set */
  case 2:  RE(m=i0(wv[1])); jt->fcalln=(I4)m; break;
  case 3:  ASSERT(0,EVDOMAIN);  /* jt->fcalli can not be set */
 }
 R mtm;
}
#endif

F1(jtsysq){I j;
 ASSERTMTV(w);
 switch(SYS){
  case SYS_PC:        j=0;                break;
  case SYS_PC386:     j=1;                break;
  case SYS_PCWIN:     j=SY_WIN32 ? (SY_WINCE ? 7 : 6) : 2; break;
  case SYS_MACINTOSH: j=3;                break;
  case SYS_OS2:       j=4;                break;
  default:            j=SYS&SYS_UNIX ? 5 : -1;
 }
 R sc(j);
}

F1(jtxepq){
 ASSERTMTV(w); 
 R jt->xep?jt->xep:mtv;
}

F1(jtxeps){
 ARGCHK1(w);
 ASSERT(1>=AR(w),EVRANK);
 ASSERT(!AN(w)||AT(w)&LIT,EVDOMAIN);
 fa(jt->xep);
 RZ(ras(w)); RZ(jt->xep=w); 
 R mtm;
}

F1(jtasgzombq){ASSERTMTV(w); R sc(jt->asgzomblevel);}

F1(jtasgzombs){I k; 
 RE(k=i0(w)); 
 ASSERT(BETWEENC(k,0,2),EVDOMAIN);
 jt->asgzomblevel=(C)k;
 R mtm;
}

#if 0 // obsolete 
// 9!:54/55  undocumented
// unicodex78;       /* 1 iff disallow numeric argument for 7 8 u:      */
F1(jtunicodex78q){
 ASSERTMTV(w);
 R sc(jt->unicodex78);
}

F1(jtunicodex78s){I k;
 RE(k=i0(w));
 ASSERT(BETWEENC(k,0,1),EVDOMAIN);
 jt->unicodex78=(C)k;
 R mtm;
}
#endif

// 9!:56  undocumented
// query/override cpu feature
F1(jtcpufeature){
 ARGCHK1(w);
 ASSERT(AT(w)&LIT,EVDOMAIN);
 ASSERT(AN(w),EVLENGTH);
 ASSERT(1>=AR(w),EVRANK);
 w=str0(w);
 if (!strcasecmp(CAV(w),"CPU")) {
#if defined(__aarch64__)||defined(_M_ARM64)
  R cstr("arm64");
#elif defined(__arm__)||defined(_M_ARM)
  R cstr("arm");
#elif defined(__x86_64__)||defined(_M_X64)
  R cstr("x86_64");
#elif defined(__i386__)||defined(_M_IX86)
  R cstr("x86");
#else
  R cstr("unknown");
#endif
 }
#if defined(__aarch64__)
 if     (!strcasecmp(CAV(w),"FP"      )) R sc(!!(getCpuFeatures()&ARM_HWCAP_FP ));
 else if(!strcasecmp(CAV(w),"ASIMD"   )) R sc(!!(getCpuFeatures()&ARM_HWCAP_ASIMD ));
 else if(!strcasecmp(CAV(w),"EVTSTRM" )) R sc(!!(getCpuFeatures()&ARM_HWCAP_EVTSTRM ));
 else if(!strcasecmp(CAV(w),"AES"     )) R sc(!!(getCpuFeatures()&ARM_HWCAP_AES ));
 else if(!strcasecmp(CAV(w),"PMULL"   )) R sc(!!(getCpuFeatures()&ARM_HWCAP_PMULL ));
 else if(!strcasecmp(CAV(w),"SHA1"    )) R sc(!!(getCpuFeatures()&ARM_HWCAP_SHA1 ));
 else if(!strcasecmp(CAV(w),"SHA2"    )) R sc(!!(getCpuFeatures()&ARM_HWCAP_SHA2 ));
 else if(!strcasecmp(CAV(w),"CRC32"   )) R sc(!!(getCpuFeatures()&ARM_HWCAP_CRC32 ));
 else if(!strcasecmp(CAV(w),"ATOMICS" )) R sc(!!(getCpuFeatures()&ARM_HWCAP_ATOMICS ));
 else if(!strcasecmp(CAV(w),"FPHP"    )) R sc(!!(getCpuFeatures()&ARM_HWCAP_FPHP ));
 else if(!strcasecmp(CAV(w),"ASIMDHP" )) R sc(!!(getCpuFeatures()&ARM_HWCAP_ASIMDHP ));
 else if(!strcasecmp(CAV(w),"CPUID"   )) R sc(!!(getCpuFeatures()&ARM_HWCAP_CPUID ));
 else if(!strcasecmp(CAV(w),"ASIMDRDM")) R sc(!!(getCpuFeatures()&ARM_HWCAP_ASIMDRDM ));
 else if(!strcasecmp(CAV(w),"JSCVT"   )) R sc(!!(getCpuFeatures()&ARM_HWCAP_JSCVT ));
 else if(!strcasecmp(CAV(w),"FCMA"    )) R sc(!!(getCpuFeatures()&ARM_HWCAP_FCMA ));
 else if(!strcasecmp(CAV(w),"LRCPC"   )) R sc(!!(getCpuFeatures()&ARM_HWCAP_LRCPC ));
 else if(!strcasecmp(CAV(w),"DCPOP"   )) R sc(!!(getCpuFeatures()&ARM_HWCAP_DCPOP ));
 else if(!strcasecmp(CAV(w),"SHA3"    )) R sc(!!(getCpuFeatures()&ARM_HWCAP_SHA3 ));
 else if(!strcasecmp(CAV(w),"SM3"     )) R sc(!!(getCpuFeatures()&ARM_HWCAP_SM3 ));
 else if(!strcasecmp(CAV(w),"SM4"     )) R sc(!!(getCpuFeatures()&ARM_HWCAP_SM4 ));
 else if(!strcasecmp(CAV(w),"ASIMDDP" )) R sc(!!(getCpuFeatures()&ARM_HWCAP_ASIMDDP ));
 else if(!strcasecmp(CAV(w),"SHA512"  )) R sc(!!(getCpuFeatures()&ARM_HWCAP_SHA512 ));
 else if(!strcasecmp(CAV(w),"SVE"     )) R sc(!!(getCpuFeatures()&ARM_HWCAP_SVE ));
 else if(!strcasecmp(CAV(w),"ASIMDFHM")) R sc(!!(getCpuFeatures()&ARM_HWCAP_ASIMDFHM ));
 else if(!strcasecmp(CAV(w),"DIT"     )) R sc(!!(getCpuFeatures()&ARM_HWCAP_DIT ));
 else if(!strcasecmp(CAV(w),"USCAT"   )) R sc(!!(getCpuFeatures()&ARM_HWCAP_USCAT ));
 else if(!strcasecmp(CAV(w),"ILRCPC"  )) R sc(!!(getCpuFeatures()&ARM_HWCAP_ILRCPC ));
 else if(!strcasecmp(CAV(w),"FLAGM"   )) R sc(!!(getCpuFeatures()&ARM_HWCAP_FLAGM ));
 else R sc(0);
#elif defined(__x86_64__)||defined(__i386__)||defined(_M_X64)||defined(_M_IX86)
 if     (!strcasecmp(CAV(w),"SSSE3"   )) R sc(!!(getCpuFeatures()&CPU_X86_FEATURE_SSSE3 ));
 else if(!strcasecmp(CAV(w),"POPCNT"  )) R sc(!!(getCpuFeatures()&CPU_X86_FEATURE_POPCNT ));
 else if(!strcasecmp(CAV(w),"MOVBE"   )) R sc(!!(getCpuFeatures()&CPU_X86_FEATURE_MOVBE ));
 else if(!strcasecmp(CAV(w),"SSE4_1"  )) R sc(!!(getCpuFeatures()&CPU_X86_FEATURE_SSE4_1 ));
 else if(!strcasecmp(CAV(w),"SSE4_2"  )) R sc(!!(getCpuFeatures()&CPU_X86_FEATURE_SSE4_2 ));
 else if(!strcasecmp(CAV(w),"AES_NI"  )) R sc(!!(getCpuFeatures()&CPU_X86_FEATURE_AES_NI ));
 else if(!strcasecmp(CAV(w),"AVX"     )) R sc(!!(getCpuFeatures()&CPU_X86_FEATURE_AVX ));
 else if(!strcasecmp(CAV(w),"RDRAND"  )) R sc(!!(getCpuFeatures()&CPU_X86_FEATURE_RDRAND ));
 else if(!strcasecmp(CAV(w),"AVX2"    )) R sc(!!(getCpuFeatures()&CPU_X86_FEATURE_AVX2 ));
 else if(!strcasecmp(CAV(w),"SHA_NI"  )) R sc(!!(getCpuFeatures()&CPU_X86_FEATURE_SHA_NI ));
 else if(!strcasecmp(CAV(w),"FMA"     )) R sc(!!(getCpuFeatures()&CPU_X86_FEATURE_FMA ));
 else if(!strcasecmp(CAV(w),"RDSEED"  )) R sc(!!(getCpuFeatures()&CPU_X86_FEATURE_RDSEED ));
 else R sc(0);
#else
 R sc(0);
#endif
}

// thread unsafe
F2(jtcpufeature2){I k;
 ARGCHK2(a,w);
 ASSERT(AT(w)&LIT,EVDOMAIN);
 ASSERT(AN(w),EVLENGTH);
 ASSERT(1>=AR(w),EVRANK);
 RE(k=i0(a));  // get arg
 ASSERT(k==0||k==1,EVDOMAIN);
 w=str0(w);
 if(k){
#if defined(__aarch64__)
 if     (!strcasecmp(CAV(w),"FP"      )) g_cpuFeatures |= ARM_HWCAP_FP ;
 else if(!strcasecmp(CAV(w),"ASIMD"   )) g_cpuFeatures |= ARM_HWCAP_ASIMD ;
 else if(!strcasecmp(CAV(w),"EVTSTRM" )) g_cpuFeatures |= ARM_HWCAP_EVTSTRM ;
 else if(!strcasecmp(CAV(w),"AES"     )) g_cpuFeatures |= ARM_HWCAP_AES ;
 else if(!strcasecmp(CAV(w),"PMULL"   )) g_cpuFeatures |= ARM_HWCAP_PMULL ;
 else if(!strcasecmp(CAV(w),"SHA1"    )) g_cpuFeatures |= ARM_HWCAP_SHA1 ;
 else if(!strcasecmp(CAV(w),"SHA2"    )) g_cpuFeatures |= ARM_HWCAP_SHA2 ;
 else if(!strcasecmp(CAV(w),"CRC32"   )) g_cpuFeatures |= ARM_HWCAP_CRC32 ;
 else if(!strcasecmp(CAV(w),"ATOMICS" )) g_cpuFeatures |= ARM_HWCAP_ATOMICS ;
 else if(!strcasecmp(CAV(w),"FPHP"    )) g_cpuFeatures |= ARM_HWCAP_FPHP ;
 else if(!strcasecmp(CAV(w),"ASIMDHP" )) g_cpuFeatures |= ARM_HWCAP_ASIMDHP ;
 else if(!strcasecmp(CAV(w),"CPUID"   )) g_cpuFeatures |= ARM_HWCAP_CPUID ;
 else if(!strcasecmp(CAV(w),"ASIMDRDM")) g_cpuFeatures |= ARM_HWCAP_ASIMDRDM ;
 else if(!strcasecmp(CAV(w),"JSCVT"   )) g_cpuFeatures |= ARM_HWCAP_JSCVT ;
 else if(!strcasecmp(CAV(w),"FCMA"    )) g_cpuFeatures |= ARM_HWCAP_FCMA ;
 else if(!strcasecmp(CAV(w),"LRCPC"   )) g_cpuFeatures |= ARM_HWCAP_LRCPC ;
 else if(!strcasecmp(CAV(w),"DCPOP"   )) g_cpuFeatures |= ARM_HWCAP_DCPOP ;
 else if(!strcasecmp(CAV(w),"SHA3"    )) g_cpuFeatures |= ARM_HWCAP_SHA3 ;
 else if(!strcasecmp(CAV(w),"SM3"     )) g_cpuFeatures |= ARM_HWCAP_SM3 ;
 else if(!strcasecmp(CAV(w),"SM4"     )) g_cpuFeatures |= ARM_HWCAP_SM4 ;
 else if(!strcasecmp(CAV(w),"ASIMDDP" )) g_cpuFeatures |= ARM_HWCAP_ASIMDDP ;
 else if(!strcasecmp(CAV(w),"SHA512"  )) g_cpuFeatures |= ARM_HWCAP_SHA512 ;
 else if(!strcasecmp(CAV(w),"SVE"     )) g_cpuFeatures |= ARM_HWCAP_SVE ;
 else if(!strcasecmp(CAV(w),"ASIMDFHM")) g_cpuFeatures |= ARM_HWCAP_ASIMDFHM ;
 else if(!strcasecmp(CAV(w),"DIT"     )) g_cpuFeatures |= ARM_HWCAP_DIT ;
 else if(!strcasecmp(CAV(w),"USCAT"   )) g_cpuFeatures |= ARM_HWCAP_USCAT ;
 else if(!strcasecmp(CAV(w),"ILRCPC"  )) g_cpuFeatures |= ARM_HWCAP_ILRCPC ;
 else if(!strcasecmp(CAV(w),"FLAGM"   )) g_cpuFeatures |= ARM_HWCAP_FLAGM ;
#elif defined(__x86_64__)||defined(__i386__)||defined(_M_X64)||defined(_M_IX86)
 if     (!strcasecmp(CAV(w),"SSSE3"   )) g_cpuFeatures |= CPU_X86_FEATURE_SSSE3 ;
 else if(!strcasecmp(CAV(w),"POPCNT"  )) g_cpuFeatures |= CPU_X86_FEATURE_POPCNT ;
 else if(!strcasecmp(CAV(w),"MOVBE"   )) g_cpuFeatures |= CPU_X86_FEATURE_MOVBE ;
 else if(!strcasecmp(CAV(w),"SSE4_1"  )) g_cpuFeatures |= CPU_X86_FEATURE_SSE4_1 ;
 else if(!strcasecmp(CAV(w),"SSE4_2"  )) g_cpuFeatures |= CPU_X86_FEATURE_SSE4_2 ;
 else if(!strcasecmp(CAV(w),"AES_NI"  )) g_cpuFeatures |= CPU_X86_FEATURE_AES_NI ;
 else if(!strcasecmp(CAV(w),"AVX"     )) g_cpuFeatures |= CPU_X86_FEATURE_AVX ;
 else if(!strcasecmp(CAV(w),"RDRAND"  )) g_cpuFeatures |= CPU_X86_FEATURE_RDRAND ;
 else if(!strcasecmp(CAV(w),"AVX2"    )) g_cpuFeatures |= CPU_X86_FEATURE_AVX2 ;
 else if(!strcasecmp(CAV(w),"SHA_NI"  )) g_cpuFeatures |= CPU_X86_FEATURE_SHA_NI ;
 else if(!strcasecmp(CAV(w),"FMA"     )) g_cpuFeatures |= CPU_X86_FEATURE_FMA ;
 else if(!strcasecmp(CAV(w),"RDSEED"  )) g_cpuFeatures |= CPU_X86_FEATURE_RDSEED ;
#endif
 } else {
#if defined(__aarch64__)
 if     (!strcasecmp(CAV(w),"FP"      )) g_cpuFeatures &= ~ARM_HWCAP_FP ;
 else if(!strcasecmp(CAV(w),"ASIMD"   )) g_cpuFeatures &= ~ARM_HWCAP_ASIMD ;
 else if(!strcasecmp(CAV(w),"EVTSTRM" )) g_cpuFeatures &= ~ARM_HWCAP_EVTSTRM ;
 else if(!strcasecmp(CAV(w),"AES"     )) g_cpuFeatures &= ~ARM_HWCAP_AES ;
 else if(!strcasecmp(CAV(w),"PMULL"   )) g_cpuFeatures &= ~ARM_HWCAP_PMULL ;
 else if(!strcasecmp(CAV(w),"SHA1"    )) g_cpuFeatures &= ~ARM_HWCAP_SHA1 ;
 else if(!strcasecmp(CAV(w),"SHA2"    )) g_cpuFeatures &= ~ARM_HWCAP_SHA2 ;
 else if(!strcasecmp(CAV(w),"CRC32"   )) g_cpuFeatures &= ~ARM_HWCAP_CRC32 ;
 else if(!strcasecmp(CAV(w),"ATOMICS" )) g_cpuFeatures &= ~ARM_HWCAP_ATOMICS ;
 else if(!strcasecmp(CAV(w),"FPHP"    )) g_cpuFeatures &= ~ARM_HWCAP_FPHP ;
 else if(!strcasecmp(CAV(w),"ASIMDHP" )) g_cpuFeatures &= ~ARM_HWCAP_ASIMDHP ;
 else if(!strcasecmp(CAV(w),"CPUID"   )) g_cpuFeatures &= ~ARM_HWCAP_CPUID ;
 else if(!strcasecmp(CAV(w),"ASIMDRDM")) g_cpuFeatures &= ~ARM_HWCAP_ASIMDRDM ;
 else if(!strcasecmp(CAV(w),"JSCVT"   )) g_cpuFeatures &= ~ARM_HWCAP_JSCVT ;
 else if(!strcasecmp(CAV(w),"FCMA"    )) g_cpuFeatures &= ~ARM_HWCAP_FCMA ;
 else if(!strcasecmp(CAV(w),"LRCPC"   )) g_cpuFeatures &= ~ARM_HWCAP_LRCPC ;
 else if(!strcasecmp(CAV(w),"DCPOP"   )) g_cpuFeatures &= ~ARM_HWCAP_DCPOP ;
 else if(!strcasecmp(CAV(w),"SHA3"    )) g_cpuFeatures &= ~ARM_HWCAP_SHA3 ;
 else if(!strcasecmp(CAV(w),"SM3"     )) g_cpuFeatures &= ~ARM_HWCAP_SM3 ;
 else if(!strcasecmp(CAV(w),"SM4"     )) g_cpuFeatures &= ~ARM_HWCAP_SM4 ;
 else if(!strcasecmp(CAV(w),"ASIMDDP" )) g_cpuFeatures &= ~ARM_HWCAP_ASIMDDP ;
 else if(!strcasecmp(CAV(w),"SHA512"  )) g_cpuFeatures &= ~ARM_HWCAP_SHA512 ;
 else if(!strcasecmp(CAV(w),"SVE"     )) g_cpuFeatures &= ~ARM_HWCAP_SVE ;
 else if(!strcasecmp(CAV(w),"ASIMDFHM")) g_cpuFeatures &= ~ARM_HWCAP_ASIMDFHM ;
 else if(!strcasecmp(CAV(w),"DIT"     )) g_cpuFeatures &= ~ARM_HWCAP_DIT ;
 else if(!strcasecmp(CAV(w),"USCAT"   )) g_cpuFeatures &= ~ARM_HWCAP_USCAT ;
 else if(!strcasecmp(CAV(w),"ILRCPC"  )) g_cpuFeatures &= ~ARM_HWCAP_ILRCPC ;
 else if(!strcasecmp(CAV(w),"FLAGM"   )) g_cpuFeatures &= ~ARM_HWCAP_FLAGM ;
#elif defined(__x86_64__)||defined(__i386__)||defined(_M_X64)||defined(_M_IX86)
 if     (!strcasecmp(CAV(w),"SSSE3"   )) g_cpuFeatures &= ~CPU_X86_FEATURE_SSSE3 ;
 else if(!strcasecmp(CAV(w),"POPCNT"  )) g_cpuFeatures &= ~CPU_X86_FEATURE_POPCNT ;
 else if(!strcasecmp(CAV(w),"MOVBE"   )) g_cpuFeatures &= ~CPU_X86_FEATURE_MOVBE ;
 else if(!strcasecmp(CAV(w),"SSE4_1"  )) g_cpuFeatures &= ~CPU_X86_FEATURE_SSE4_1 ;
 else if(!strcasecmp(CAV(w),"SSE4_2"  )) g_cpuFeatures &= ~CPU_X86_FEATURE_SSE4_2 ;
 else if(!strcasecmp(CAV(w),"AES_NI"  )) g_cpuFeatures &= ~CPU_X86_FEATURE_AES_NI ;
 else if(!strcasecmp(CAV(w),"AVX"     )) g_cpuFeatures &= ~CPU_X86_FEATURE_AVX ;
 else if(!strcasecmp(CAV(w),"RDRAND"  )) g_cpuFeatures &= ~CPU_X86_FEATURE_RDRAND ;
 else if(!strcasecmp(CAV(w),"AVX2"    )) g_cpuFeatures &= ~CPU_X86_FEATURE_AVX2 ;
 else if(!strcasecmp(CAV(w),"SHA_NI"  )) g_cpuFeatures &= ~CPU_X86_FEATURE_SHA_NI ;
 else if(!strcasecmp(CAV(w),"FMA"     )) g_cpuFeatures &= ~CPU_X86_FEATURE_FMA ;
 else if(!strcasecmp(CAV(w),"RDSEED"  )) g_cpuFeatures &= ~CPU_X86_FEATURE_RDSEED ;
#endif
}
OPENSSL_setcap();
#if C_AVX && !defined(ANDROID)
 hwfma=(getCpuFeatures()&CPU_X86_FEATURE_FMA)?1:0;
#endif
R mtm;
}

// 9!:58  undocumented
// query/set gemm threshold
// 0 igemm_thres  integer threshold
// 1 dgemm_thres  real threshold
// 2 zgemm_thres  complex threshold

F1(jtgemmtune){I k;
 ARGCHK1(w);
 ASSERT(AT(w)&(B01+INT),EVDOMAIN);
 ASSERT(1==AN(w),EVLENGTH);
 ASSERT(1>=AR(w),EVRANK);
 RE(k=i0(w));  // get arg
 ASSERT(k==0||k==1||k==2,EVDOMAIN);
 R sc((0==k)?jt->igemm_thres:(1==k)?jt->dgemm_thres:jt->zgemm_thres);
}

F2(jtgemmtune2){I j,k;
 ARGCHK2(a,w);
 ASSERT(AT(a)&(B01+INT),EVDOMAIN);
 ASSERT(1==AN(a),EVLENGTH);
 ASSERT(1>=AR(a),EVRANK);
 ASSERT(AT(w)&(B01+INT),EVDOMAIN);
 ASSERT(1==AN(w),EVLENGTH);
 ASSERT(1>=AR(w),EVRANK);
 RE(j=i0(a));  // get arg
 RE(k=i0(w));  // get arg
 ASSERT(j>=-1,EVDOMAIN);
 ASSERT(k==0||k==1||k==2,EVDOMAIN);
 if(k==0) jt->igemm_thres=j;
 else if(k==1) jt->dgemm_thres=j;
 else jt->zgemm_thres=j;
 R sc(1);
}

// enable/disable tstack auditing, since some testcases run too long with it enabled
// bit 0 is set to disable, bit 1 is a one-shot to ask for an audit
// result is old value
F1(jtaudittdisab){
#if MEMAUDIT&2
 I k,oldval;
 oldval = jt->audittstackdisabled;  // return value
 RE(k=i0(w));  // get arg
 if(k&2){jt->audittstackdisabled=0; audittstack(jt);}  // run once if so requested
 jt->audittstackdisabled=k;
 R sc(oldval);
#else
 R sc(0);
#endif
}
