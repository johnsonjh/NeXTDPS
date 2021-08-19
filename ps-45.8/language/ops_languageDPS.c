/*
  ops_languageDPS.c

THIS IS A DERIVED FILE -- DO NOT EDIT IT

This is the program fragment for registering the operator set
named "ops_languageDPS". It was generated automatically,
using information derived from the languagenames.h file.
*/

{
  extern void PSAbs();
  extern void PSAdd();
  extern void PSALoad();
  extern void PSAnchorSearch();
  extern void PSAnd();
  extern void PSArray();
  extern void PSAStore();
  extern void PSATan();
  extern void PSBegin();
  extern void PSBind();
  extern void PSBitShift();
  extern void PSBytesAvailable();
  extern void PSCeiling();
  extern void PSClear();
  extern void PSClearDictStack();
  extern void PSClrToMrk();
  extern void PSCloseFile();
  extern void PSCopy();
  extern void PSCos();
  extern void PSCount();
  extern void PSCntDictStack();
  extern void PSCntExecStack();
  extern void PSCntToMark();
  extern void PSCrDict();
  extern void PSCrFile();
  extern void PSCrObjFormat();
  extern void PSCrPacking();
  extern void PSCvI();
  extern void PSCvLit();
  extern void PSCVN();
  extern void PSCvR();
  extern void PSCVRS();
  extern void PSCVS();
  extern void PSCvX();
  extern void PSDef();
  extern void PSDefUserName();
  extern void PSDefUserObj();
  extern void PSDeleteFile();
  extern void PSDevDisMount();
  extern void PSDevForAll();
  extern void PSDevFormat();
  extern void PSDevMount();
  extern void PSDevStatus();
  extern void PSDict();
  extern void PSDictStack();
  extern void PSDiv();
  extern void PSDup();
  extern void PSEExec();
  extern void PSEnd();
  extern void PSEq();
  extern void PSExch();
  extern void PSExec();
  extern void PSExecUserObj();
  extern void PSExecStack();
  extern void PSXctOnly();
  extern void PSExit();
  extern void PSExp();
  extern void PSFile();
  extern void PSFileNameForAll();
  extern void PSFilPos();
  extern void PSFloor();
  extern void PSFls();
  extern void PSFlsFile();
  extern void PSFor();
  extern void PSForAll();
  extern void PSGe();
  extern void PSGet();
  extern void PSGetInterval();
  extern void PSGt();
  extern void PSIDiv();
  extern void PSIf();
  extern void PSIfElse();
  extern void PSIndex();
  extern void PSKnown();
  extern void PSLe();
  extern void PSLength();
  extern void PSLn();
  extern void PSLoad();
  extern void PSLog();
  extern void PSLoop();
  extern void PSLt();
  extern void PSMark();
  extern void PSMaxLength();
  extern void PSMod();
  extern void PSMul();
  extern void PSNe();
  extern void PSNeg();
  extern void PSNoAccess();
  extern void PSNot();
  extern void PSOr();
  extern void PSPkdary();
  extern void PSPop();
  extern void PSPrint();
  extern void PSPrObject();
  extern void PSPut();
  extern void PSPutInterval();
  extern void PSRand();
  extern void PSRCheck();
  extern void PSRead();
  extern void PSReadHexString();
  extern void PSReadLine();
  extern void PSReadOnly();
  extern void PSReadString();
  extern void PSRenameFile();
  extern void PSRepeat();
  extern void PSResFile();
  extern void PSRoll();
  extern void PSRound();
  extern void PSRRand();
  extern void PSSCheck();
  extern void PSSearch();
  extern void PSStFilPos();
  extern void PSStObjFormat();
  extern void PSStPacking();
  extern void PSSin();
  extern void PSSqRt();
  extern void PSSRand();
  extern void PSStatus();
  extern void PSStop();
  extern void PSStore();
  extern void PSString();
  extern void PSSub();
  extern void PSToken();
  extern void PSTruncate();
  extern void PSType();
  extern void PSUnDef();
  extern void PSUndefUserObj();
  extern void PSWCheck();
  extern void PSWhere();
  extern void PSWrite();
  extern void PSWrtHexString();
  extern void PSWrObject();
  extern void PSWrtString();
  extern void PSXCheck();
  extern void PSXor();
#if (OS == os_mpw)
  void (*ops_languageDPS[135])();
  register void (**p)() = ops_languageDPS;
  *p++ = PSAbs;
  *p++ = PSAdd;
  *p++ = PSALoad;
  *p++ = PSAnchorSearch;
  *p++ = PSAnd;
  *p++ = PSArray;
  *p++ = PSAStore;
  *p++ = PSATan;
  *p++ = PSBegin;
  *p++ = PSBind;
  *p++ = PSBitShift;
  *p++ = PSBytesAvailable;
  *p++ = PSCeiling;
  *p++ = PSClear;
  *p++ = PSClearDictStack;
  *p++ = PSClrToMrk;
  *p++ = PSCloseFile;
  *p++ = PSCopy;
  *p++ = PSCos;
  *p++ = PSCount;
  *p++ = PSCntDictStack;
  *p++ = PSCntExecStack;
  *p++ = PSCntToMark;
  *p++ = PSCrDict;
  *p++ = PSCrFile;
  *p++ = PSCrObjFormat;
  *p++ = PSCrPacking;
  *p++ = PSCvI;
  *p++ = PSCvLit;
  *p++ = PSCVN;
  *p++ = PSCvR;
  *p++ = PSCVRS;
  *p++ = PSCVS;
  *p++ = PSCvX;
  *p++ = PSDef;
  *p++ = PSDefUserName;
  *p++ = PSDefUserObj;
  *p++ = PSDeleteFile;
  *p++ = PSDevDisMount;
  *p++ = PSDevForAll;
  *p++ = PSDevFormat;
  *p++ = PSDevMount;
  *p++ = PSDevStatus;
  *p++ = PSDict;
  *p++ = PSDictStack;
  *p++ = PSDiv;
  *p++ = PSDup;
  *p++ = PSEExec;
  *p++ = PSEnd;
  *p++ = PSEq;
  *p++ = PSExch;
  *p++ = PSExec;
  *p++ = PSExecUserObj;
  *p++ = PSExecStack;
  *p++ = PSXctOnly;
  *p++ = PSExit;
  *p++ = PSExp;
  *p++ = PSFile;
  *p++ = PSFileNameForAll;
  *p++ = PSFilPos;
  *p++ = PSFloor;
  *p++ = PSFls;
  *p++ = PSFlsFile;
  *p++ = PSFor;
  *p++ = PSForAll;
  *p++ = PSGe;
  *p++ = PSGet;
  *p++ = PSGetInterval;
  *p++ = PSGt;
  *p++ = PSIDiv;
  *p++ = PSIf;
  *p++ = PSIfElse;
  *p++ = PSIndex;
  *p++ = PSKnown;
  *p++ = PSLe;
  *p++ = PSLength;
  *p++ = PSLn;
  *p++ = PSLoad;
  *p++ = PSLog;
  *p++ = PSLoop;
  *p++ = PSLt;
  *p++ = PSMark;
  *p++ = PSMaxLength;
  *p++ = PSMod;
  *p++ = PSMul;
  *p++ = PSNe;
  *p++ = PSNeg;
  *p++ = PSNoAccess;
  *p++ = PSNot;
  *p++ = PSOr;
  *p++ = PSPkdary;
  *p++ = PSPop;
  *p++ = PSPrint;
  *p++ = PSPrObject;
  *p++ = PSPut;
  *p++ = PSPutInterval;
  *p++ = PSRand;
  *p++ = PSRCheck;
  *p++ = PSRead;
  *p++ = PSReadHexString;
  *p++ = PSReadLine;
  *p++ = PSReadOnly;
  *p++ = PSReadString;
  *p++ = PSRenameFile;
  *p++ = PSRepeat;
  *p++ = PSResFile;
  *p++ = PSRoll;
  *p++ = PSRound;
  *p++ = PSRRand;
  *p++ = PSSCheck;
  *p++ = PSSearch;
  *p++ = PSStFilPos;
  *p++ = PSStObjFormat;
  *p++ = PSStPacking;
  *p++ = PSSin;
  *p++ = PSSqRt;
  *p++ = PSSRand;
  *p++ = PSStatus;
  *p++ = PSStop;
  *p++ = PSStore;
  *p++ = PSString;
  *p++ = PSSub;
  *p++ = PSToken;
  *p++ = PSTruncate;
  *p++ = PSType;
  *p++ = PSUnDef;
  *p++ = PSUndefUserObj;
  *p++ = PSWCheck;
  *p++ = PSWhere;
  *p++ = PSWrite;
  *p++ = PSWrtHexString;
  *p++ = PSWrObject;
  *p++ = PSWrtString;
  *p++ = PSXCheck;
  *p++ = PSXor;
#else (OS == os_mpw)
  static void (*ops_languageDPS[135])() = {
    PSAbs,
    PSAdd,
    PSALoad,
    PSAnchorSearch,
    PSAnd,
    PSArray,
    PSAStore,
    PSATan,
    PSBegin,
    PSBind,
    PSBitShift,
    PSBytesAvailable,
    PSCeiling,
    PSClear,
    PSClearDictStack,
    PSClrToMrk,
    PSCloseFile,
    PSCopy,
    PSCos,
    PSCount,
    PSCntDictStack,
    PSCntExecStack,
    PSCntToMark,
    PSCrDict,
    PSCrFile,
    PSCrObjFormat,
    PSCrPacking,
    PSCvI,
    PSCvLit,
    PSCVN,
    PSCvR,
    PSCVRS,
    PSCVS,
    PSCvX,
    PSDef,
    PSDefUserName,
    PSDefUserObj,
    PSDeleteFile,
    PSDevDisMount,
    PSDevForAll,
    PSDevFormat,
    PSDevMount,
    PSDevStatus,
    PSDict,
    PSDictStack,
    PSDiv,
    PSDup,
    PSEExec,
    PSEnd,
    PSEq,
    PSExch,
    PSExec,
    PSExecUserObj,
    PSExecStack,
    PSXctOnly,
    PSExit,
    PSExp,
    PSFile,
    PSFileNameForAll,
    PSFilPos,
    PSFloor,
    PSFls,
    PSFlsFile,
    PSFor,
    PSForAll,
    PSGe,
    PSGet,
    PSGetInterval,
    PSGt,
    PSIDiv,
    PSIf,
    PSIfElse,
    PSIndex,
    PSKnown,
    PSLe,
    PSLength,
    PSLn,
    PSLoad,
    PSLog,
    PSLoop,
    PSLt,
    PSMark,
    PSMaxLength,
    PSMod,
    PSMul,
    PSNe,
    PSNeg,
    PSNoAccess,
    PSNot,
    PSOr,
    PSPkdary,
    PSPop,
    PSPrint,
    PSPrObject,
    PSPut,
    PSPutInterval,
    PSRand,
    PSRCheck,
    PSRead,
    PSReadHexString,
    PSReadLine,
    PSReadOnly,
    PSReadString,
    PSRenameFile,
    PSRepeat,
    PSResFile,
    PSRoll,
    PSRound,
    PSRRand,
    PSSCheck,
    PSSearch,
    PSStFilPos,
    PSStObjFormat,
    PSStPacking,
    PSSin,
    PSSqRt,
    PSSRand,
    PSStatus,
    PSStop,
    PSStore,
    PSString,
    PSSub,
    PSToken,
    PSTruncate,
    PSType,
    PSUnDef,
    PSUndefUserObj,
    PSWCheck,
    PSWhere,
    PSWrite,
    PSWrtHexString,
    PSWrObject,
    PSWrtString,
    PSXCheck,
    PSXor,
  };
#endif (OS == os_mpw)
  RgstOpSet(ops_languageDPS, 135, 256*PACKAGE_INDEX + 1);
}
