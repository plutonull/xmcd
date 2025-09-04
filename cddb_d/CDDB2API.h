/*
 * @(#)CDDB2API.h	7.4 01/04/13
 * from cddb2sdk-1.0.R3
 *
 * CDDB2API.h Copyright (C) 2000 CDDB, Inc. All Rights Reserved.
 *
 * This file defines the interface to the CDDB shared library.
 *
 * This file is published to 3rd party developers so that they may call the SDK.
 */
#ifndef CDDB2API_H
#define CDDB2API_H

/* Only include the enums if the main IDL generated header file has not been included. */
#ifndef __CDDBControl_h__
#ifdef XMCD_CDDB
#include "cddb_d/CDDBdefs.h"
#else
#include "CDDBEnums.h"
#endif
#endif

/* Error codes */
/* TBD REC: Define strings for these errors. */
#define _CddbResult_TYPEDEF_(_sc) ((CddbResult)_sc)
#define Cddb_OK                             ((CddbResult)0x00000000L)
#define Cddb_FALSE                          ((CddbResult)0x00000001L)
#define Cddb_E_UNEXPECTED                     _CddbResult_TYPEDEF_(0x8000FFFFL)
#define Cddb_E_NOTIMPL                        _CddbResult_TYPEDEF_(0x80004001L)
#define Cddb_E_OUTOFMEMORY                    _CddbResult_TYPEDEF_(0x8007000EL)
#define Cddb_E_INVALIDARG                     _CddbResult_TYPEDEF_(0x80070057L)
#define Cddb_E_NOINTERFACE                    _CddbResult_TYPEDEF_(0x80004002L)
#define Cddb_E_POINTER                        _CddbResult_TYPEDEF_(0x80004003L)
#define Cddb_E_HANDLE                         _CddbResult_TYPEDEF_(0x80070006L)
#define Cddb_E_ABORT                          _CddbResult_TYPEDEF_(0x80004004L)
#define Cddb_E_FAIL                           _CddbResult_TYPEDEF_(0x80004005L)
#define Cddb_E_ACCESSDENIED                   _CddbResult_TYPEDEF_(0x80070005L)
#define Cddb_E_PENDING                        _CddbResult_TYPEDEF_(0x8000000AL)

typedef unsigned char CddbBoolean;
typedef long		  CddbResult;

/* Null terminated C string */
typedef char *		CddbStr;
typedef const char*	CddbConstStr;

/* The following definitions are opaque object pointers. */
typedef struct OpaqueCddbControl*		CddbControlPtr;
typedef struct OpaqueCddbDiscs*			CddbDiscsPtr;
typedef struct OpaqueCddbDisc*			CddbDiscPtr;
typedef struct OpaqueCddbTrack*			CddbTrackPtr;
typedef struct OpaqueCddbTracks*		CddbTracksPtr;
typedef struct OpaqueCddbGenre*			CddbGenrePtr;
typedef struct OpaqueCddbGenreList*		CddbGenreListPtr;
typedef struct OpaqueCddbRole*			CddbRolePtr;
typedef struct OpaqueCddbLanguage*		CddbLanguagePtr;
typedef struct OpaqueCddbRegion*		CddbRegionPtr;
/* CddbFieldPtr has been deprecated. */
/*typedef struct OpaqueCddbField*			CddbFieldPtr;*/
typedef struct OpaqueCddbSegment*		CddbSegmentPtr;
typedef struct OpaqueCddbURL*			CddbURLPtr;
typedef struct OpaqueCddbFullName*		CddbFullNamePtr;
typedef struct OpaqueCddbCredit*		CddbCreditPtr;
typedef struct OpaqueCddbSegments*		CddbSegmentsPtr;
typedef struct OpaqueCddbGenreTree*		CddbGenreTreePtr;
typedef struct OpaqueCddbCredits*		CddbCreditsPtr;
typedef struct OpaqueCddbURLList*		CddbURLListPtr;
typedef struct OpaqueCddbURLTree*		CddbURLTreePtr;
/* CddbFieldListPtr has been deprecated. */
/*typedef struct OpaqueCddbFieldList*		CddbFieldListPtr;*/
typedef struct OpaqueCddbLanguageList*	CddbLanguageListPtr;
typedef struct OpaqueCddbRegionList*	CddbRegionListPtr;
typedef struct OpaqueCddbRoleList*		CddbRoleListPtr;
typedef struct OpaqueCddbUserInfo*		CddbUserInfoPtr;
typedef struct OpaqueCddbOptions*		CddbOptionsPtr;
typedef struct OpaqueCddbRoleTree*		CddbRoleTreePtr;
typedef struct OpaqueCddbLanguages*		CddbLanguagesPtr;
typedef struct OpaqueCddbURLManager*	CddbURLManagerPtr;
typedef struct OpaqueCddbID3TagManager*	CddbID3TagManagerPtr;
typedef struct OpaqueCddbID3Tag*		CddbID3TagPtr;
typedef struct OpaqueCddbInfoWindow*	CddbInfoWindowPtr;

/*
 * These are the type codes that you can pass to CddbCreateObject. Currently, the only type
 * codes defined are for the CddbFullName, CddbURL, and CddbID3Tag objects. This is because
 * no other APIs exist for creating objects of these type. Other objects you might want to
 * create, such as CddbDisc, CddbTrack, CddbCredit, and CddbSegment are created automatically
 * when you call CddbControl_GetSubmitDisc, CddbDisc_AddTrack, CddbDisc_AddCredit,
 * CddbDisc_AddSegment, etc.
 */
typedef enum CddbObjectType
{
	CddbFullNameType	= 1,
	CddbURLType			= 2,
	CddbID3TagType		= 3
} CddbObjectType;

#ifdef Cddb_Build
/* Defined if building the Cddb shared library. Don't define this in a client application. */
#pragma export on
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* --- General Functions --------------------------------------------------- */
	
/*
 * On the Mac, CddbInitialize has been deprecated. Use CddbMacInitialize 
 * (declared in CddbMacExtras.h) instead.
 */
	CddbResult CddbInitialize(CddbControlPtr *cddb);
	
	CddbResult CddbTerminate(CddbControlPtr cddb);
	
	void CddbGetErrorString(CddbResult code, char* buffer, int bufferSize);
	
	void CddbAlert(CddbResult code);
	
	void* CddbCreateObject(CddbObjectType type);
	
	int CddbAddRefObject(void* pObj);
	
	int CddbReleaseObject(void* pObj);

/* --- Callbacks --------------------------------------------------- */
	
	typedef void (*CddbCommandCompletedCallback)(long commandCode, CddbResult commandResult, long value, long userData);
	typedef void (*CddbLogMessageCallback)(CddbStr message, long userData);
	typedef void (*CddbServerMessageCallback)(long messageCode, long messageAction, CddbStr messageData, long userData);
	typedef void (*CddbCommandProgressCallback)(long commandCode, long progressCode, long bytesDone, long bytesTotal, long userData);
	
	void CddbSetCommandCompletedCallback(CddbCommandCompletedCallback callback, long userData);
	void CddbSetLogMessageCallback(CddbLogMessageCallback callback, long userData);
	void CddbSetServerMessageCallback(CddbServerMessageCallback callback, long userData);
	void CddbSetCommandProgressCallback(CddbCommandProgressCallback callback, long userData);
	
/* --- CddbControl --------------------------------------------------- */
	
	CddbResult CddbControl_GetVersion(			CddbControlPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbControl_IsRegistered(		CddbControlPtr that,
												CddbBoolean InvokeDialog,
							/* [retval][out] */ CddbBoolean *Registered);
	
	CddbResult CddbControl_SetClientInfo(		CddbControlPtr that,
												CddbConstStr ClientId,
												CddbConstStr ClientTag,
												CddbConstStr ClientVersion,
												CddbConstStr ClientRegString);
	
	CddbResult CddbControl_GetUserInfo(			CddbControlPtr that,
							/* [retval][out] */ CddbUserInfoPtr *pVal);
	
	CddbResult CddbControl_SetUserInfo(			CddbControlPtr that,
												CddbUserInfoPtr UserInfo);
	
	CddbResult CddbControl_GetOptions(			CddbControlPtr that,
							/* [retval][out] */ CddbOptionsPtr *pVal);
	
	CddbResult CddbControl_SetOptions(			CddbControlPtr that,
												CddbOptionsPtr Options);
	
	CddbResult CddbControl_GetMediaToc(			CddbControlPtr that,
												long unitNumber,
							/* [retval][out] */ CddbStr *MediaToc);
	
	CddbResult CddbControl_LookupMediaByToc(	CddbControlPtr that,
												CddbConstStr MediaToc,
												CddbBoolean EventOnCompletion,
							/* [retval][out] */ CDDBMatchCode *pMatchCode);
	
#if 0
/* GetMatchedDiscInfo has been deprecated. Use GetMatchedDisc and GetMatchedDiscs instead. */
	CddbResult CddbControl_GetMatchedDiscInfo(	CddbControlPtr that,
									/* [out] */ CddbDiscPtr *ppDisc,
									/* [out] */ CddbDiscsPtr *ppDiscs);
#endif
	
	CddbResult CddbControl_GetMatchedDisc(		CddbControlPtr that,
							/* [retval][out] */ CddbDiscPtr *ppDisc);
	
	CddbResult CddbControl_GetMatchedDiscs(		CddbControlPtr that,
							/* [retval][out] */ CddbDiscsPtr *ppDiscs);
	
	CddbResult CddbControl_Initialize(			CddbControlPtr that,
												long ParentHWND,	/* ignored */
												CDDBCacheFlags Flags);

	CddbResult CddbControl_Shutdown(			CddbControlPtr that);
        
#if 0
/* CddbControl_SetClientInfoFromHost has been deprecated. */
	CddbResult CddbControl_SetClientInfoFromHost(	CddbControlPtr that,
													CddbConstStr Target,
													CddbConstStr User,
													CddbConstStr Password);
#endif
        
	CddbResult CddbControl_InvokeFuzzyMatchDialog(	CddbControlPtr that,
													CddbDiscsPtr Discs,
								/* [retval][out] */ CddbDiscPtr *pVal);
	
	CddbResult CddbControl_GetFullDiscInfo(		CddbControlPtr that,
												CddbDiscPtr Disc,
												CddbBoolean EventOnCompletion,
							/* [retval][out] */ CddbDiscPtr *pVal);
	
	CddbResult CddbControl_GetDiscInfo(			CddbControlPtr that,
												CddbConstStr MediaId,
												CddbConstStr MuiId,
												CddbConstStr RevisionId,
												CddbConstStr RevisionTag,
												CddbBoolean EventOnCompletion,
									/* [out] */ CddbDiscPtr *ppDisc,
									/* [out] */ CddbDiscsPtr *ppDiscs);
	
	CddbResult CddbControl_InvokeDiscInfo(		CddbControlPtr that,
												CddbConstStr MediaToc,
												CddbConstStr MediaId,
												CddbConstStr MuiId,
												long unitNumber,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbControl_DisplayDiscInfo(		CddbControlPtr that,
												CddbDiscPtr Disc,
												CDDBUIFlags Flags,
							/* [retval][out] */ CDDBUIFlags *pVal);
	
	CddbResult CddbControl_GetSubmitDisc(		CddbControlPtr that,
												CddbConstStr MediaToc,
												CddbConstStr MediaId,
												CddbConstStr MuiId,
							/* [retval][out] */ CddbDiscPtr *pVal);
	
	CddbResult CddbControl_SubmitDisc(			CddbControlPtr that,
												CddbDiscPtr Disc,
												CddbBoolean EventOnCompletion,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbControl_InvokeSubmitDisc(	CddbControlPtr that,
												CddbConstStr MediaToc,
												long unitNumber,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbControl_GetGenreList(		CddbControlPtr that,
												CddbBoolean EventOnCompletion,
							/* [retval][out] */ CddbGenreListPtr *pVal);
	
	CddbResult CddbControl_GetGenreTree(		CddbControlPtr that,
												CddbBoolean EventOnCompletion,
							/* [retval][out] */ CddbGenreTreePtr *pVal);
	
	CddbResult CddbControl_GetGenreInfo(		CddbControlPtr that,
												CddbConstStr GenreId,
							/* [retval][out] */ CddbGenrePtr *pVal);
	
	CddbResult CddbControl_GetRegionList(		CddbControlPtr that,
												CddbBoolean EventOnCompletion,
							/* [retval][out] */ CddbRegionListPtr *pVal);
	
	CddbResult CddbControl_GetRegionInfo(		CddbControlPtr that,
												CddbConstStr RegionId,
							/* [retval][out] */ CddbRegionPtr *pVal);
	
	CddbResult CddbControl_GetRoleList(			CddbControlPtr that,
												CddbBoolean EventOnCompletion,
							/* [retval][out] */ CddbRoleListPtr *pVal);
	
	CddbResult CddbControl_GetRoleTree(			CddbControlPtr that,
												CddbBoolean EventOnCompletion,
							/* [retval][out] */ CddbRoleTreePtr *pVal);

	CddbResult CddbControl_GetRoleInfo(			CddbControlPtr that,
												CddbConstStr RoleId,
							/* [retval][out] */ CddbRolePtr *pVal);
	
	CddbResult CddbControl_GetLanguageList(		CddbControlPtr that,
												CddbBoolean EventOnCompletion,
							/* [retval][out] */ CddbLanguageListPtr *pVal);
	
	CddbResult CddbControl_GetLanguageInfo(		CddbControlPtr that,
												CddbConstStr LanguageId,
							/* [retval][out] */ CddbLanguagePtr *pVal);
	
#if 0
/* CddbControl_GetFieldList has been deprecated. */
	CddbResult CddbControl_GetFieldList(		CddbControlPtr that,
												CddbConstStr Target,
												CddbBoolean EventOnCompletion,
							/* [retval][out] */ CddbFieldListPtr *pVal);
	
/* CddbControl_GetFieldInfo has been deprecated. */
	CddbResult CddbControl_GetFieldInfo(		CddbControlPtr that,
												CddbConstStr Target,
												CddbConstStr Name,
							/* [retval][out] */ CddbFieldPtr *pVal);
#endif
	
	CddbResult CddbControl_GetURLList(			CddbControlPtr that,
												CddbDiscPtr Disc,
												CddbBoolean EventOnCompletion,
							/* [retval][out] */ CddbURLListPtr *pVal);
	
	CddbResult CddbControl_GetURLManager(		CddbControlPtr that,
							/* [retval][out] */ CddbURLManagerPtr *pVal);
	
	CddbResult CddbControl_UpdateControl(		CddbControlPtr that,
												CddbBoolean UpdateNow,
												CddbBoolean EventOnCompletion,
							/* [retval][out] */	CddbBoolean *pVal);
	
	CddbResult CddbControl_GetServiceStatusURL(	CddbControlPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbControl_GetServiceStatus(	CddbControlPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
#if 0
/* CddbControl_GetCoverURL has not been implemented. */
	CddbResult CddbControl_GetCoverURL(			CddbControlPtr that,
												CddbDiscPtr Disc,
												CddbBoolean EventOnCompletion,
							/* [retval][out] */ CddbURLPtr *pVal);
#endif
	
	CddbResult CddbControl_GetUpdateURL(		CddbControlPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbControl_InvokeAboutCddbDialog(	CddbControlPtr that);
	
	CddbResult CddbControl_InvokePopupMenu(		CddbControlPtr that,
												CDDBUIFlags Flags);
	
	CddbResult CddbControl_InvokeUserRegDialog(	CddbControlPtr that,
												CddbUserInfoPtr UserInfo,
												CDDBUIFlags Flags,
							/* [retval][out] */ CDDBUIFlags *pVal);

	CddbResult CddbControl_InvokeOptionsDialog(	CddbControlPtr that,
												CddbOptionsPtr Options,
							/* [retval][out] */ CDDBUIFlags *pVal);

	CddbResult CddbControl_InvokeNoMatchDialog(	CddbControlPtr that,
							/* [retval][out] */ CDDBUIFlags *pVal);

	CddbResult CddbControl_GetDiscTagId(		CddbControlPtr that,
												CddbDiscPtr Disc,
												long TrackNum,
							/* [retval][out] */ CddbStr *pVal);

	CddbResult CddbControl_LookupMediaByTagId(	CddbControlPtr that,
												CddbConstStr TagId,
												CddbBoolean EventOnCompletion,
							/* out */			long* TrackNum,
							/* [retval][out] */ CddbDiscPtr *pVal);

	CddbResult CddbControl_GetTagManager(		CddbControlPtr that,
							/* [retval][out] */ CddbID3TagManagerPtr *pVal);

	CddbResult CddbControl_HttpPost(			CddbControlPtr that,
												CddbConstStr URL,
												CddbConstStr Header,
												void *PostData,
												long PostDataSize,
									/* [out] */ void **Data,
									/* [out] */ long *DataSize);

	CddbResult CddbControl_HttpGet(				CddbControlPtr that,
												CddbConstStr URL,
												CddbConstStr Header,
												void *GetData,
												long GetDataSize,
									/* [out] */ void **Data,
									/* [out] */ long *DataSize);

	CddbResult CddbControl_HttpGetFile(			CddbControlPtr that,
												CddbConstStr URL,
												CddbConstStr Header,
												void *GetData,
												long GetDataSize,
												CddbConstStr File);

	CddbResult CddbControl_InvokeInfoBrowser(	CddbControlPtr that,
												CddbDiscPtr Disc,
												CddbURLPtr URL,
												CDDBUIFlags Flags);
        
	CddbResult CddbControl_Cancel(				CddbControlPtr that,
							/* [retval][out] */ long *pVal);

	CddbResult CddbControl_FlushLocalCache(		CddbControlPtr that,
												CDDBFlushFlags Flags);
	
	CddbResult CddbControl_Status(				CddbControlPtr that);
	
	CddbResult CddbControl_ServerNoop(			CddbControlPtr that,
												CddbBoolean EventOnCompletion);
	
/* --- CddbDiscs --------------------------------------------------- */
	
	CddbResult CddbDiscs_GetCount(				CddbDiscsPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbDiscs_GetDisc(				CddbDiscsPtr that,
												long lIndex,
							/* [retval][out] */ CddbDiscPtr *pVal);
	
/* --- CddbDisc ---------------------------------------------------- */

	CddbResult CddbDisc_GetToc(					CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutToc(					CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetTitle(				CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutTitle(				CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetArtist(				CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutArtist(				CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetLabel(				CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutLabel(				CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetYear(				CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutYear(				CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetNumTracks(			CddbDiscPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbDisc_GetTrackTitle(			CddbDiscPtr that,
												long Number,
							/* [retval][out] */ CddbStr *Title);
	
	CddbResult CddbDisc_AddTrack(				CddbDiscPtr that,
												long Number,
												CddbConstStr Title,
							/* [retval][out] */ CddbTrackPtr *pVal);
	
	CddbResult CddbDisc_GetMediaId(				CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_GetMuiId(				CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_GetNotes(				CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutNotes(				CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetGenreId(				CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutGenreId(				CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetSecondaryGenreId(	CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutSecondaryGenreId(	CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetRegionId(			CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutRegionId(			CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetRevision(			CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutRevision(			CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetTotalInSet(			CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutTotalInSet(			CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetNumberInSet(			CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutNumberInSet(			CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetTitleUId(			CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_GetCertifier(			CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_GetTitleSort(			CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutTitleSort(			CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetTitleThe(			CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutTitleThe(			CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetTracks(				CddbDiscPtr that,
							/* [retval][out] */ CddbTracksPtr *pVal);
	
	CddbResult CddbDisc_GetSegments(			CddbDiscPtr that,
							/* [retval][out] */ CddbSegmentsPtr *pVal);
	
	CddbResult CddbDisc_GetCredits(				CddbDiscPtr that,
							/* [retval][out] */ CddbCreditsPtr *pVal);
	
	CddbResult CddbDisc_GetArtistFullName(		CddbDiscPtr that,
							/* [retval][out] */ CddbFullNamePtr *pVal);
	
	CddbResult CddbDisc_PutArtistFullName(		CddbDiscPtr that,
												const CddbFullNamePtr newVal);
	
#if 0
	/* Deprecated. Use CddbControl_GetURLList followed by CddbURLList_CreateURLTree instead. */
	CddbResult CddbDisc_GetURLTree(				CddbDiscPtr that,
							/* [retval][out] */ CddbURLTreePtr *pVal);
#endif
	
	CddbResult CddbDisc_AddSegment(				CddbDiscPtr that,
												CddbConstStr Name,
							/* [retval][out] */ CddbSegmentPtr *pVal);
	
	CddbResult CddbDisc_RemoveSegment(			CddbDiscPtr that,
												CddbConstStr Name);
	
	CddbResult CddbDisc_AddCredit(				CddbDiscPtr that,
												CddbConstStr RoleId,
												CddbConstStr Name,
							/* [retval][out] */ CddbCreditPtr *pVal);
	
	CddbResult CddbDisc_RemoveCredit(			CddbDiscPtr that,
												CddbCreditPtr credit);
	
	CddbResult CddbDisc_GetLanguageId(			CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutLanguageId(			CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetRevisionTag(			CddbDiscPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbDisc_PutRevisionTag(			CddbDiscPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbDisc_GetCompilation(			CddbDiscPtr that,
							/* [retval][out] */ CddbBoolean *pVal);
	
	CddbResult CddbDisc_PutCompilation(			CddbDiscPtr that,
												CddbBoolean newVal);
	
	CddbResult CddbDisc_GetLanguages(			CddbDiscPtr that,
							/* [retval][out] */ CddbLanguagesPtr *pVal);
	
	CddbResult CddbDisc_GetTrack(				CddbDiscPtr that,
												long Number,
							/* [retval][out] */ CddbTrackPtr *pVal);
	
	CddbResult CddbDisc_GetSegment(				CddbDiscPtr that,
												long SegmentIndex,
							/* [retval][out] */ CddbSegmentPtr *pVal);
	
	CddbResult CddbDisc_GetNumCredits(			CddbDiscPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbDisc_GetNumSegments(			CddbDiscPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbDisc_GetCredit(				CddbDiscPtr that,
												long CreditIndex,
							/* [retval][out] */ CddbCreditPtr *pVal);
	
	CddbResult CddbDisc_IsSubmit(				CddbDiscPtr that,
							/* [retval][out] */ CddbBoolean *pVal);

	CddbResult CddbDisc_IsPropertyCertified(	CddbDiscPtr that,
												CDDBProperty Property,
							/* [retval][out] */ CddbBoolean *pVal);
	
	CddbResult CddbDisc_RemoveSegmentObject(	CddbDiscPtr that,
												CddbSegmentPtr Segment);
	
/* --- CddbTracks --------------------------------------------------- */

	CddbResult CddbTracks_GetCount(				CddbTracksPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbTracks_GetTrack(				CddbTracksPtr that,
												long lIndex,
							/* [retval][out] */ CddbTrackPtr *pVal);
	
/* --- CddbTrack --------------------------------------------------- */
	
	CddbResult CddbTrack_GetTitle(				CddbTrackPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbTrack_PutTitle(				CddbTrackPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbTrack_GetArtist(				CddbTrackPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbTrack_PutArtist(				CddbTrackPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbTrack_GetYear(				CddbTrackPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbTrack_PutYear(				CddbTrackPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbTrack_GetLabel(				CddbTrackPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbTrack_PutLabel(				CddbTrackPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbTrack_GetNotes(				CddbTrackPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbTrack_PutNotes(				CddbTrackPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbTrack_GetGenreId(			CddbTrackPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbTrack_PutGenreId(			CddbTrackPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbTrack_GetSecondaryGenreId(	CddbTrackPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbTrack_PutSecondaryGenreId(	CddbTrackPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbTrack_GetLyrics(				CddbTrackPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbTrack_PutLyrics(				CddbTrackPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbTrack_GetBeatsPerMinute(		CddbTrackPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbTrack_PutBeatsPerMinute(		CddbTrackPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbTrack_GetISRC(				CddbTrackPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbTrack_PutISRC(				CddbTrackPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbTrack_GetTitleSort(			CddbTrackPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbTrack_PutTitleSort(			CddbTrackPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbTrack_GetTitleThe(			CddbTrackPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbTrack_PutTitleThe(			CddbTrackPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbTrack_GetArtistFullName(		CddbTrackPtr that,
							/* [retval][out] */ CddbFullNamePtr *pVal);
	
	CddbResult CddbTrack_PutArtistFullName(		CddbTrackPtr that,
												const CddbFullNamePtr newVal);
	
	CddbResult CddbTrack_AddCredit(				CddbTrackPtr that,
												CddbConstStr RoleId,
												CddbConstStr Name,
							/* [retval][out] */ CddbCreditPtr *pVal);
	
	CddbResult CddbTrack_RemoveCredit(			CddbTrackPtr that,
												CddbCreditPtr Credit);
	
	CddbResult CddbTrack_GetCredits(			CddbTrackPtr that,
							/* [retval][out] */ CddbCreditsPtr *pVal);
	
	CddbResult CddbTrack_GetCredit(				CddbTrackPtr that,
												long CreditIndex,
							/* [retval][out] */ CddbCreditPtr *pVal);
	
	CddbResult CddbTrack_GetNumCredits(			CddbTrackPtr that,
							/* [retval][out] */ long *pVal);

	CddbResult CddbTrack_IsPropertyCertified(	CddbTrackPtr that,
												CDDBProperty Property,
							/* [retval][out] */ CddbBoolean *pVal);
	
/* --- CddbGenre --------------------------------------------------- */
	
	CddbResult CddbGenre_GetId(					CddbGenrePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbGenre_GetName(				CddbGenrePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbGenre_GetDescription(		CddbGenrePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
#if 0
/* CddbGenre_GetType has been deprecated. */
	CddbResult CddbGenre_GetType(				CddbGenrePtr that,
							/* [retval][out] */ CddbStr *pVal);
#endif
	
	CddbResult CddbGenre_GetParent(				CddbGenrePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
/* --- CddbGenreList --------------------------------------------------- */
	
	CddbResult CddbGenreList_GetCount(			CddbGenreListPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbGenreList_GetGenreName(		CddbGenreListPtr that,
												CddbConstStr GenreId,
							/* [retval][out] */ CddbStr *GenreName);
	
	CddbResult CddbGenreList_GetMetaGenre(		CddbGenreListPtr that,
							/* [retval][out] */ CddbGenrePtr *pVal);
	
	CddbResult CddbGenreList_GetGenre(			CddbGenreListPtr that,
												long lIndex,
							/* [retval][out] */ CddbGenrePtr *pVal);
	
	CddbResult CddbGenreList_GetGenreInfo(		CddbGenreListPtr that,
												CddbConstStr GenreId,
							/* [retval][out] */ CddbGenrePtr *pVal);
	
/* --- CddbRole --------------------------------------------------- */
	
	CddbResult CddbRole_GetId(					CddbRolePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbRole_GetName(				CddbRolePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbRole_GetDescription(			CddbRolePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbRole_GetParent(				CddbRolePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
/* --- CddbLanguage --------------------------------------------------- */
	
	CddbResult CddbLanguage_GetId(				CddbLanguagePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbLanguage_GetName(			CddbLanguagePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbLanguage_GetDescription(		CddbLanguagePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
/* --- CddbRegion --------------------------------------------------- */
	
	CddbResult CddbRegion_GetId(				CddbRegionPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbRegion_GetName(				CddbRegionPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbRegion_GetDescription(		CddbRegionPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
#if 0
/* CddbField has been deprecated. */
/* --- CddbField --------------------------------------------------- */
	
	CddbResult CddbField_GetDisplayName(		CddbFieldPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbField_GetName(				CddbFieldPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbField_GetDescription(		CddbFieldPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbField_GetType(				CddbFieldPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbField_GetLength(				CddbFieldPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbField_GetEnumCount(			CddbFieldPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbField_GetTable(				CddbFieldPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbField_GetTarget(				CddbFieldPtr that,
							/* [retval][out] */ CddbStr *pVal);
#endif
	
/* --- CddbSegment --------------------------------------------------- */
	
	CddbResult CddbSegment_GetName(				CddbSegmentPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbSegment_PutName(				CddbSegmentPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbSegment_GetNotes(			CddbSegmentPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbSegment_PutNotes(			CddbSegmentPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbSegment_GetStartTrack(		CddbSegmentPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbSegment_PutStartTrack(		CddbSegmentPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbSegment_GetStartFrame(		CddbSegmentPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbSegment_PutStartFrame(		CddbSegmentPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbSegment_GetEndTrack(			CddbSegmentPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbSegment_PutEndTrack(			CddbSegmentPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbSegment_GetEndFrame(			CddbSegmentPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbSegment_PutEndFrame(			CddbSegmentPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbSegment_GetCredits(			CddbSegmentPtr that,
							/* [retval][out] */ CddbCreditsPtr *pVal);
	
	CddbResult CddbSegment_AddCredit(			CddbSegmentPtr that,
												CddbConstStr RoleId,
												CddbConstStr Name,
							/* [retval][out] */ CddbCreditPtr *pVal);
	
	CddbResult CddbSegment_RemoveCredit(		CddbSegmentPtr that,
												CddbCreditPtr Credit);
	
	CddbResult CddbSegment_GetCredit(			CddbSegmentPtr that,
												long CreditIndex,
							/* [retval][out] */ CddbCreditPtr *pVal);
	
	CddbResult CddbSegment_GetNumCredits(		CddbSegmentPtr that,
							/* [retval][out] */ long *pVal);

	CddbResult CddbSegment_IsPropertyCertified(	CddbSegmentPtr that,
												CDDBProperty Property,
							/* [retval][out] */ CddbBoolean *pVal);
	
/* --- CddbURL --------------------------------------------------- */

	CddbResult CddbURL_GetHref(					CddbURLPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbURL_PutHref(					CddbURLPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbURL_GetDisplayLink(			CddbURLPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbURL_PutDisplayLink(			CddbURLPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbURL_GetDescription(			CddbURLPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbURL_PutDescription(			CddbURLPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbURL_GetType(					CddbURLPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbURL_PutType(					CddbURLPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbURL_GetCategory(				CddbURLPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbURL_PutCategory(				CddbURLPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbURL_GetWeight(				CddbURLPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbURL_PutWeight(				CddbURLPtr that,
												CddbConstStr newVal);

	CddbResult CddbURL_GetSize(					CddbURLPtr that,
							/* [retval][out] */ CddbStr *pVal);
							
	CddbResult CddbURL_PutSize(					CddbURLPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbURL_GetDisplayText(			CddbURLPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbURL_PutDisplayText(			CddbURLPtr that,
												CddbConstStr newVal);

	CddbResult CddbURL_IsPropertyCertified(		CddbURLPtr that,
												CDDBProperty Property,
							/* [retval][out] */ CddbBoolean *pVal);

/* --- CddbFullName --------------------------------------------------- */
	
	CddbResult CddbFullName_GetName(			CddbFullNamePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbFullName_PutName(			CddbFullNamePtr that,
												CddbConstStr newVal);
	
	CddbResult CddbFullName_GetFirstName(		CddbFullNamePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbFullName_PutFirstName(		CddbFullNamePtr that,
												CddbConstStr newVal);
	
	CddbResult CddbFullName_GetLastName(		CddbFullNamePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbFullName_PutLastName(		CddbFullNamePtr that,
												CddbConstStr newVal);
	
	CddbResult CddbFullName_GetThe(				CddbFullNamePtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbFullName_PutThe(				CddbFullNamePtr that,
												CddbConstStr newVal);
	
/* --- CddbCredit --------------------------------------------------- */
	
	CddbResult CddbCredit_GetId(				CddbCreditPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbCredit_PutId(				CddbCreditPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbCredit_GetName(				CddbCreditPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbCredit_PutName(				CddbCreditPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbCredit_GetNotes(				CddbCreditPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbCredit_PutNotes(				CddbCreditPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbCredit_GetFullName(			CddbCreditPtr that,
							/* [retval][out] */ CddbFullNamePtr *pVal);
	
	CddbResult CddbCredit_PutFullName(			CddbCreditPtr that,
												const CddbFullNamePtr newVal);

	CddbResult CddbCredit_IsPropertyCertified(	CddbCreditPtr that,
												CDDBProperty Property,
							/* [retval][out] */ CddbBoolean *pVal);
	
/* --- CddbSegments --------------------------------------------------- */
	
	CddbResult CddbSegments_GetCount(			CddbSegmentsPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbSegments_GetSegment(			CddbSegmentsPtr that,
												long lIndex,
							/* [retval][out] */ CddbSegmentPtr *pVal);
	
/* --- CddbGenreTree --------------------------------------------------- */
	
	CddbResult CddbGenreTree_GetCount(			CddbGenreTreePtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbGenreTree_GetSubGenreList(	CddbGenreTreePtr that,
												CddbConstStr GenreId,
							/* [retval][out] */ CddbGenreListPtr *pVal);
	
	CddbResult CddbGenreTree_GetMetaGenre(		CddbGenreTreePtr that,
												long genreIndex,
							/* [retval][out] */ CddbGenrePtr *pVal);
		
	CddbResult CddbGenreTree_GetItem(			CddbGenreTreePtr that,
												long lIndex,
							/* [retval][out] */ CddbGenreListPtr *pVal);
	
/* --- CddbCredits --------------------------------------------------- */
	
	CddbResult CddbCredits_GetCount(			CddbCreditsPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbCredits_GetCredit(			CddbCreditsPtr that,
												long lIndex,
							/* [retval][out] */ CddbCreditPtr *pVal);
	
/* --- CddbURLList --------------------------------------------------- */
	
	CddbResult CddbURLList_GetCount(			CddbURLListPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbURLList_GetCategory(			CddbURLListPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbURLList_GetURL(				CddbURLListPtr that,
												long lIndex,
							/* [retval][out] */ CddbURLPtr *pVal);
	
	CddbResult CddbURLList_CreateURLTree(		CddbURLListPtr that,
							/* [retval][out] */ CddbURLTreePtr *pVal);
	
/* --- CddbURLTree --------------------------------------------------- */
	
	CddbResult CddbURLTree_GetCount(			CddbURLTreePtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbURLTree_CategoryList(		CddbURLTreePtr that,
												CddbConstStr Category,
							/* [retval][out] */ CddbURLListPtr *pVal);
	
	CddbResult CddbURLTree_GetURLList(			CddbURLTreePtr that,
												long lIndex,
							/* [retval][out] */ CddbURLListPtr *pVal);
	
#if 0
/* CddbFieldList has been deprecated. */
/* --- CddbFieldList --------------------------------------------------- */
	
	CddbResult CddbFieldList_GetCount(			CddbFieldListPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbFieldList_GetFieldName(		CddbFieldListPtr that,
												CddbConstStr Name,
												CddbConstStr Target,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbFieldList_GetItem(			CddbFieldListPtr that,
												long lIndex,
							/* [retval][out] */ CddbFieldPtr *pVal);
#endif
	
/* --- CddbLanguageList --------------------------------------------------- */
	
	CddbResult CddbLanguageList_GetCount(		CddbLanguageListPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbLanguageList_GetLanguageName(CddbLanguageListPtr that,
												CddbConstStr Id,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbLanguageList_GetLanguage(	CddbLanguageListPtr that,
												long lIndex,
							/* [retval][out] */ CddbLanguagePtr *pVal);
	
	CddbResult CddbLanguageList_GetLanguageInfo(CddbLanguageListPtr that,
												CddbConstStr LanguageId,
							/* [retval][out] */ CddbLanguagePtr *pVal);
	
/* --- CddbRegionList --------------------------------------------------- */
	
	CddbResult CddbRegionList_GetCount(			CddbRegionListPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbRegionList_GetRegionName(	CddbRegionListPtr that,
												CddbConstStr Id,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbRegionList_GetRegion(		CddbRegionListPtr that,
												long lIndex,
							/* [retval][out] */ CddbRegionPtr *pVal);
	
	CddbResult CddbRegionList_GetRegionInfo(	CddbRegionListPtr that,
												CddbConstStr RegionId,
							/* [retval][out] */ CddbRegionPtr *pVal);
	
/* --- CddbRoleList --------------------------------------------------- */
	
	CddbResult CddbRoleList_GetCount(			CddbRoleListPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbRoleList_GetRoleName(		CddbRoleListPtr that,
												CddbConstStr Id,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbRoleList_GetRole(			CddbRoleListPtr that,
												long lIndex,
							/* [retval][out] */ CddbRolePtr *pVal);

	CddbResult CddbRoleList_GetCategory(		CddbRoleListPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbRoleList_GetCategoryRole(	CddbRoleListPtr that,
							/* [retval][out] */ CddbRolePtr *pVal);
	
	CddbResult CddbRoleList_GetRoleInfo(		CddbRoleListPtr that,
												CddbConstStr RoleId,
							/* [retval][out] */ CddbRolePtr *pVal);
	
/* --- CddbUserInfo --------------------------------------------------- */
	
	CddbResult CddbUserInfo_GetEmailAddress(	CddbUserInfoPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbUserInfo_PutEmailAddress(	CddbUserInfoPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbUserInfo_GetUserHandle(		CddbUserInfoPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbUserInfo_PutUserHandle(		CddbUserInfoPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbUserInfo_GetPassword(		CddbUserInfoPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbUserInfo_PutPassword(		CddbUserInfoPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbUserInfo_GetPasswordHint(	CddbUserInfoPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbUserInfo_PutPasswordHint(	CddbUserInfoPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbUserInfo_GetRegionId(		CddbUserInfoPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbUserInfo_PutRegionId(		CddbUserInfoPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbUserInfo_GetPostalCode(		CddbUserInfoPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbUserInfo_PutPostalCode(		CddbUserInfoPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbUserInfo_GetAge(				CddbUserInfoPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbUserInfo_PutAge(				CddbUserInfoPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbUserInfo_GetSex(				CddbUserInfoPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbUserInfo_PutSex(				CddbUserInfoPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbUserInfo_GetAllowEmail(		CddbUserInfoPtr that,
							/* [retval][out] */ CddbBoolean *pVal);
	
	CddbResult CddbUserInfo_PutAllowEmail(		CddbUserInfoPtr that,
												CddbBoolean newVal);
	
	CddbResult CddbUserInfo_GetAllowStats(		CddbUserInfoPtr that,
							/* [retval][out] */ CddbBoolean *pVal);
	
	CddbResult CddbUserInfo_PutAllowStats(		CddbUserInfoPtr that,
												CddbBoolean newVal);

/* --- CddbOptions --------------------------------------------------- */
		
	CddbResult CddbOptions_GetLanguage(			CddbOptionsPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbOptions_PutLanguage(			CddbOptionsPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbOptions_GetLocalCachePath(	CddbOptionsPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbOptions_PutLocalCachePath(	CddbOptionsPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbOptions_GetLocalCacheSize(	CddbOptionsPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbOptions_PutLocalCacheSize(	CddbOptionsPtr that,
												long newVal);
	
	CddbResult CddbOptions_GetLogFile(			CddbOptionsPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbOptions_PutLogFile(			CddbOptionsPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbOptions_GetLogFlags(			CddbOptionsPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbOptions_PutLogFlags(			CddbOptionsPtr that,
												long newVal);
	
#if 0
/* CddbOptions_GetLogWindow and CddbOptions_PutLogWindow have been deprecated. */
	CddbResult CddbOptions_GetLogWindow(		CddbOptionsPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbOptions_PutLogWindow(		CddbOptionsPtr that,
												long newVal);
#endif
	
	CddbResult CddbOptions_GetProxyServer(		CddbOptionsPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbOptions_PutProxyServer(		CddbOptionsPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbOptions_GetProxyServerPort(	CddbOptionsPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbOptions_PutProxyServerPort(	CddbOptionsPtr that,
												long newVal);
	
#if 0
/* CddbOptions_GetResourceModule and CddbOptions_PutResourceModule have been deprecated. */
	CddbResult CddbOptions_GetResourceModule(	CddbOptionsPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbOptions_PutResourceModule(	CddbOptionsPtr that,
												long newVal);
#endif
	
	CddbResult CddbOptions_GetServerTimeout(	CddbOptionsPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbOptions_PutServerTimeout(	CddbOptionsPtr that,
												long newVal);
	
#if 0
/* CddbOptions_GetAsyncCompletion and CddbOptions_PutAsyncCompletion have been deprecated. */
	CddbResult CddbOptions_GetAsyncCompletion(	CddbOptionsPtr that,
							/* [retval][out] */ CddbBoolean *pVal);
	
	CddbResult CddbOptions_PutAsyncCompletion(	CddbOptionsPtr that,
												CddbBoolean newVal);
#endif
	
	CddbResult CddbOptions_GetAutoDownloadURLs(	CddbOptionsPtr that,
							/* [retval][out] */ CddbBoolean *pVal);
	
	CddbResult CddbOptions_PutAutoDownloadURLs(	CddbOptionsPtr that,
												CddbBoolean newVal);
	
	CddbResult CddbOptions_GetAutoDownloadTargetedURLs(	CddbOptionsPtr that,
									/* [retval][out] */ CddbBoolean *pVal);
	
	CddbResult CddbOptions_PutAutoDownloadTargetedURLs(	CddbOptionsPtr that,
														CddbBoolean newVal);
	
	CddbResult CddbOptions_GetProxyUserName(	CddbOptionsPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbOptions_PutProxyUserName(	CddbOptionsPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbOptions_GetProxyPassword(	CddbOptionsPtr that,
							/* [retval][out] */ CddbStr *pVal);
	
	CddbResult CddbOptions_PutProxyPassword(	CddbOptionsPtr that,
												CddbConstStr newVal);
	
	CddbResult CddbOptions_GetProgressEvents(	CddbOptionsPtr that,
							/* [retval][out] */ CddbBoolean *pVal);
	
	CddbResult CddbOptions_PutProgressEvents(	CddbOptionsPtr that,
												CddbBoolean newVal);

	CddbResult CddbOptions_GetLocalCacheTimeout(CddbOptionsPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbOptions_PutLocalCacheTimeout(CddbOptionsPtr that,
												long newVal);
	
	CddbResult CddbOptions_GetLocalCacheFlags(	CddbOptionsPtr that,
							/* [retval][out] */ long *pVal);
	
	CddbResult CddbOptions_PutLocalCacheFlags(	CddbOptionsPtr that,
												long newVal);
	
	CddbResult CddbOptions_GetTestSubmitMode(	CddbOptionsPtr that,
							/* [retval][out] */ CddbBoolean *pVal);
	
	CddbResult CddbOptions_PutTestSubmitMode(	CddbOptionsPtr that,
												CddbBoolean newVal);
	
/* --- CddbRoleTree --------------------------------------------------- */
		
	CddbResult CddbRoleTree_GetCount(			CddbRoleTreePtr that,
				 			/* [retval][out] */ long *pVal);
	
	CddbResult CddbRoleTree_GetCategoryRoleList(CddbRoleTreePtr that,
												CddbConstStr RoleId,
							/* [retval][out] */ CddbRoleListPtr *pVal);
	
	CddbResult CddbRoleTree_GetRoleList(		CddbRoleTreePtr that,
												long ListIndex,
							/* [retval][out] */ CddbRoleListPtr *pVal);
	
	
/* --- CddbLanguages --------------------------------------------------- */
		
	
/* --- CddbURLManager --------------------------------------------------- */
		
	CddbResult CddbURLManager_GetCoverURLs(		CddbURLManagerPtr that,
												CddbDiscPtr Disc,
							/* [retval][out] */ CddbURLListPtr* pVal);
        
	CddbResult CddbURLManager_GetInfoURLs(		CddbURLManagerPtr that,
												CddbDiscPtr Disc,
							/* [retval][out] */ CddbURLListPtr* pVal);
        
	CddbResult CddbURLManager_GetAdURLs(		CddbURLManagerPtr that,
												CddbDiscPtr Disc,
							/* [retval][out] */ CddbURLListPtr* pVal);
        
	CddbResult CddbURLManager_GetMenuURLs(		CddbURLManagerPtr that,
												CddbDiscPtr Disc,
							/* [retval][out] */ CddbURLListPtr* pVal);
        
	CddbResult CddbURLManager_GetSkinURLs(		CddbURLManagerPtr that,
												CddbDiscPtr Disc,
							/* [retval][out] */ CddbURLListPtr* pVal);
        
	CddbResult CddbURLManager_GetPluginURLs(	CddbURLManagerPtr that,
												CddbDiscPtr Disc,
							/* [retval][out] */ CddbURLListPtr* pVal);
        
	CddbResult CddbURLManager_GetTypedURLs(		CddbURLManagerPtr that,
												CddbDiscPtr Disc,
												CddbConstStr Type,
							/* [retval][out] */ CddbURLListPtr* pVal);
        
	CddbResult CddbURLManager_GetAssociatedURLs(CddbURLManagerPtr that,
												CddbDiscPtr Disc,
												CddbConstStr Type,
												CddbConstStr Property,
												CddbConstStr Value,
							/* [retval][out] */ CddbURLListPtr* pVal);
        
	CddbResult CddbURLManager_GetURLAssociations(CddbURLManagerPtr that,
							/* [retval][out] */ CddbURLListPtr* pVal);
        
	CddbResult CddbURLManager_GetCurrentURL(	CddbURLManagerPtr that,
												CddbDiscPtr Disc,
												CddbConstStr URL,
							/* [retval][out] */ CddbStr* pVal);
        
	CddbResult CddbURLManager_SubmitURL(		CddbURLManagerPtr that,
												CddbDiscPtr Disc,
												CddbURLPtr URL);
        
	CddbResult CddbURLManager_DeleteURL(		CddbURLManagerPtr that,
												CddbURLPtr URL);

	CddbResult CddbURLManager_GotoURL(			CddbURLManagerPtr that,
												CddbURLPtr URL,
												CddbConstStr RawURL);
	
/* --- CddbID3TagManager --------------------------------------------------- */
		
	CddbResult CddbID3TagManager_InitTagFromDisc(	CddbID3TagManagerPtr that,
													CddbDiscPtr Disc,
													long TrackNum,
								/* [retval][out] */ CddbID3TagPtr *pVal);
        
	CddbResult CddbID3TagManager_MergeToFile(	CddbID3TagManagerPtr that,
												CddbID3TagPtr Tag,
												CddbConstStr Filename,
												long Flags);
        
	CddbResult CddbID3TagManager_GetID3TagFileId(	CddbID3TagManagerPtr that,
													CddbDiscPtr Disc,
													long TrackNum,
								/* [retval][out] */ CddbStr *pVal);

	CddbResult CddbID3TagManager_GetCddbOwnerId(	CddbID3TagManagerPtr that,
								/* [retval][out] */ CddbStr *pVal);

	CddbResult CddbID3TagManager_GetCddbCommentString(	CddbID3TagManagerPtr that,
									/* [retval][out] */ CddbStr *pVal);

	CddbResult CddbID3TagManager_LookupMediaByFile(	CddbID3TagManagerPtr that,
													CddbConstStr Filename,
													CddbBoolean EventOnCompletion,
										/* [out] */ long* TrackNum,
								/* [retval][out] */ CddbDiscPtr* pVal);
	
/* --- CddbID3Tag --------------------------------------------------- */
		
	CddbResult CddbID3Tag_GetAlbum(				CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutAlbum(				CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_GetMovie(				CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutMovie(				CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_GetTitle(				CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutTitle(				CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_GetCopyrightYear(		CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutCopyrightYear(		CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_GetCopyrightHolder(	CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutCopyrightHolder(	CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_GetComments(			CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutComments(			CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_GetLabel(				CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutLabel(				CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_GetBeatsPerMinute(	CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutBeatsPerMinute(	CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_GetLeadArtist(		CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutLeadArtist(		CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_GetPartOfSet(			CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutPartOfSet(			CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_GetTrackPosition(		CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutTrackPosition(		CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_GetYear(				CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutYear(				CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_GetGenre(				CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutGenre(				CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_GetFileId(			CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutFileId(			CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_GetISRC(				CddbID3TagPtr that,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_PutISRC(				CddbID3TagPtr that,
												CddbConstStr newVal);
        
	CddbResult CddbID3Tag_LoadFromFile(			CddbID3TagPtr that,
												CddbConstStr Filename,
												CddbBoolean Readonly);
        
	CddbResult CddbID3Tag_BindToFile(			CddbID3TagPtr that,
												CddbConstStr Filename,
												CddbBoolean Readonly);
        
	CddbResult CddbID3Tag_SaveToFile(			CddbID3TagPtr that,
												CddbConstStr Filename);
        
	CddbResult CddbID3Tag_Commit(				CddbID3TagPtr that);
        
	CddbResult CddbID3Tag_Clear(				CddbID3TagPtr that);
        
	CddbResult CddbID3Tag_LoadFromBuffer(		CddbID3TagPtr that,
												unsigned char* Buffer,
												long BufferSize);
        
	CddbResult CddbID3Tag_GetBufferSize(		CddbID3TagPtr that,
							/* [retval][out] */ long *pVal);
        
	CddbResult CddbID3Tag_SaveToBuffer(			CddbID3TagPtr that,
												unsigned char* Buffer,
												long BufferSize);
        
	CddbResult CddbID3Tag_GetTextFrame(			CddbID3TagPtr that,
												CddbConstStr frame,
							/* [retval][out] */ CddbStr *pVal);
        
	CddbResult CddbID3Tag_SetTextFrame(			CddbID3TagPtr that,
												CddbConstStr frame,
												CddbConstStr newVal);
	
/* --- CddbInfoWindow --------------------------------------------------- */
#if 0
/* Though you can't create or use CddbInfoWindow objects, */
/* you can call CddbControl_InvokeInfoBrowser to display  */
/* disc-related URLs in the default browser.              */

/* TBD REC: Wrap this class. */		
	CddbResult CddbInfoWindow_Init(				CddbInfoWindowPtr that
												long hWnd,
												long left,
												long top,
												long right,
												long bottom);
        
	CddbResult CddbInfoWindow_SetRawURL(		CddbInfoWindowPtr that
												CddbConstStr RawURL);
        
	CddbResult CddbInfoWindow_GetHwnd(			CddbInfoWindowPtr that
									/* [out] */ long* phwnd);
        
	CddbResult CddbInfoWindow_SetDisc(			CddbInfoWindowPtr that
												CddbDiscPtr Disc);
        
	CddbResult CddbInfoWindow_SetURL(			CddbInfoWindowPtr that
												CddbURLPtr URL);
        
	CddbResult CddbInfoWindow_Refresh(			CddbInfoWindowPtr that);
        
	CddbResult CddbInfoWindow_Shutdown(			CddbInfoWindowPtr that);
        
	CddbResult CddbInfoWindow_SetAdPosition(	CddbInfoWindowPtr that
												CddbConstStr Position);
#endif
	
/* --- CddbUIOptions --------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#ifdef Cddb_Build
#pragma export off
#endif

#endif
