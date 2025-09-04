/*
 * @(#)CDDBEnums.h	7.1 01/01/19
 * from cddb2sdk-1.0.R3
 *
 * CDDBEnums.h Copyright (C) 2000 CDDB, Inc. All Rights Reserved.
 *
 * Enums used by CDDBControl
 *
 * This file is published to 3rd party developers so that they may call the SDK.
 */
#ifndef CDDBENUMS_H
#define CDDBENUMS_H


typedef enum CDDBErrors
{
	ERR_DomainMask			= 0x7ff0000,
	ERR_DomainTransport		= 0x2fa0000,
	ERR_DomainService		= 0x2fb0000,
	ERR_DomainControl		= 0x2fc0000,
	ERR_CodeMask			= 0xffff,
	ERR_Busy				= 0x1,
	ERR_NotRegistered		= 0x2,
	ERR_HandleUsed			= 0x3,
	ERR_InvalidParameter	= 0x4,
	ERR_MissingField		= 0x5,
	ERR_MissingProperty		= 0x6,
	ERR_NoCommand			= 0x7,
	ERR_NoClientInfo		= 0x8,
	ERR_NotInitialized		= 0x9,
	ERR_InvalidTagId		= 0xA,
	ERR_Disabled			= 0xB,

/*
 *	NOTE:	The CDDBTRN enums must be kept consistent with
 *			those defined in cddbslib.h.
 */
	CDDBTRNOutOfMemory			= 0x82FA0001,	/* couldn't allocate memory */
	CDDBTRNBadPointer			= 0x82FA0002,	/* invalid pointer */
	CDDBTRNOutOfRange			= 0x82FA0003,	/* argument out of range */
	CDDBTRNCorruptedData		= 0x82FA0004,	/* data malformed or corrupted */
	CDDBTRNFieldNotFound		= 0x82FA0005,	/* named field not found */
	CDDBTRNUnknownEncoding		= 0x82FA0006,	/* data from server encoded in unknown format */
	CDDBTRNSockInitErr			= 0x82FA0007,	/* couldn't initialize network communications */
	CDDBTRNHostNotFound			= 0x82FA0008,	/* CDDB server can't be found */
	CDDBTRNSockCreateErr		= 0x82FA0009,	/* socket create failed */
	CDDBTRNSockOpenErr			= 0x82FA000A,	/* socket open failed */
	CDDBTRNSendFailed			= 0x82FA000B,	/* send to server failed */
	CDDBTRNRecvFailed			= 0x82FA000C,	/* recv of server data failed */
	CDDBTRNNoEvent				= 0x82FA000D,	/* no event available */
	CDDBTRNNoUserInfo			= 0x82FA000E,	/* user/client info has not been set */
	CDDBTRNBatchNest			= 0x82FA000F,	/* batch nesting not permitted */
	CDDBTRNBatchNotOpen			= 0x82FA0010,	/* must open before closing */
	CDDBTRNBadResponseSyntax	= 0x82FA0011,	/* response from CDDB server had bad syntax */
	CDDBTRNUnknownCompression	= 0x82FA0012,	/* unsupported compression scheme requested */
	CDDBTRNTooManyRetries		= 0x82FA0013,	/* retry limit exceeded when communicating with server */
	CDDBTRNRecordNotFound		= 0x82FA0014,	/* record not found in data store */
	CDDBTRNKeyTooLong			= 0x82FA0015,	/* datastore key exceeds maximum allowed length */
	CDDBTRNURLNotFound			= 0x82FA0016,	/* URL response not found in datastore */
	CDDBTRNBadArgument			= 0x82FA0017,	/* invalid argument passed to interface function */
	CDDBTRNUnflattenFailed		= 0x82FA0018,	/* object stored in the datastore is corrupt */
	CDDBTRNTokenTooLong			= 0x82FA0019,	/* protocol parser error: token exceeds max size */
	CDDBTRNTokenInvalid			= 0x82FA001A,	/* protocol parser error: token malformed */
	CDDBTRNCannotCreateFile		= 0x82FA001B,	/* unable to create file in file system */
	CDDBTRNBadClientCommand		= 0x82FA001C,	/* malformed command sent from server to client */
	CDDBTRNSendStatsFailed		= 0x82FA001D,	/* sending statistics to server failed */
	CDDBTRNUnknownEncryption	= 0x82FA001E,	/* unsupported encryption scheme requested */
	CDDBTRNProtocolVersion		= 0x82FA001F,	/* server using newer version of protocol */
	CDDBTRNDataStoreVersion		= 0x82FA0020,	/* unknown version of datastore file */
	CDDBTRNDataStoreInitFail	= 0x82FA0021,	/* can't initialize datastore */
	CDDBTRNDataStoreNotCached	= 0x82FA0022,	/* passed query not in local datastore */
	CDDBTRNCancelled			= 0x82FA0023,	/* command cancelled before completion */
	CDDBTRNServerTimeout		= 0x82FA0024,	/* timed-out waiting for data from server */
	CDDBTRNInvalidURL			= 0x82FA0025,	/* passed URL has bad format */
	CDDBTRNHTTPError			= 0x82FA0026,	/* generic (non-proxy) HTTP processing error */
	CDDBTRNFileWriteError		= 0x82FA0027,	/* cannot write to file */
	CDDBTRNFileDeleteError		= 0x82FA0028,	/* cannot delete file */
	CDDBTRNIDInvalidated		= 0x82FA0029,	/* previously cached ID has been invalidated */
	CDDBTRNHTTPProxyError		= 0x82FA002A,	/* HTTP proxy error */

	CDDBSVCServiceError			= 0x82FB0000,	/*	unspecified service error */
	CDDBSVCHandleUsed			= 0x82FB0001,	/* handle is taken */
	CDDBSVCNoEmail				= 0x82FB0002,	/* no email address was available */
	CDDBSVCNoHint				= 0x82FB0003,	/* no password hint was available */
	CDDBSVCUnknownHandle		= 0x82FB0004,	/* handle is unknown */
	CDDBSVCInvalidField			= 0x82FB0064,	/* request contains invalid field */
	CDDBSVCMissingField			= 0x82FB0065,	/* request does not include required field */
	CDDBSVCLimitReached			= 0x82FB0066,	/* disc query limit reached */

	CDDBCTLBusy					= 0x82FC0001,	/* control is busy */
	CDDBCTLNotRegistered		= 0x82FC0002,	/* user is not registered */
	CDDBCTLHandleUsed			= 0x82FC0003,	/* requested user "nickname" is used or the password being supplied is invalid */
	CDDBCTLInvalidParameter		= 0x82FC0004,	/* required field is invalid */
	CDDBCTLMissingField			= 0x82FC0005,	/* required field is missing */
	CDDBCTLMissingProperty		= 0x82FC0006,	/* required property is missing */
	CDDBCTLNoCommand			= 0x82FC0007,	/* no active command */
	CDDBCTLNoClientInfo			= 0x82FC0008,	/* client info has not been set */
	CDDBCTLNotInitialized		= 0x82FC0009,	/* control hasn't been initialized */
	CDDBCTLInvalidTagId			= 0x82FC000A,	/* passed Tag Id isn't valid */
	CDDBCTLDisabled				= 0x82FC000B,	/* the application or control has been disabled */

	CDDBCTL_ID3TagNoMemory		= 0x82FC1000,	/* No available memory */
	CDDBCTL_ID3TagNoData		= 0x82FC1001,	/* No data to parse */
	CDDBCTL_ID3TagBadData		= 0x82FC1002,	/* Improperly formatted data */
	CDDBCTL_ID3TagNoBuffer		= 0x82FC1003,	/* No buffer to write to */
	CDDBCTL_ID3TagSmallBuffer	= 0x82FC1004,	/* Buffer is too small */
	CDDBCTL_ID3TagInvalidFrameID	= 0x82FC1005,	/* Invalid frame id */
	CDDBCTL_ID3TagFieldNotFound		= 0x82FC1006,	/* Requested field not found */
	CDDBCTL_ID3TagUnknownFieldType	= 0x82FC1007,	/* Unknown field type */
	CDDBCTL_ID3TagAlreadyAttached	= 0x82FC1008,	/* Tag is already attached to a file */
	CDDBCTL_ID3TagInvalidVersion	= 0x82FC1009,	/* Invalid tag version */
	CDDBCTL_ID3TagNoFile		= 0x82FC100A,	/* No file to parse */
	CDDBCTL_ID3TagReadonly		= 0x82FC100B,	/* Attempting to write to a read-only file */
	CDDBCTL_ID3TagzlibError		= 0x82FC100C	/* Error in compression/decompression */

} CDDBErrors;

typedef enum CDDBCommands
{
	CMD_None					= 0,
	CMD_Invalid					= -1,
	PROP_Version				= 1,
	PROP_ServiceStatusURL		= 2,
	CMD_IsRegistered			= 3,
	CMD_SetClientInfo			= 4,
	CMD_GetUserInfo				= 5,
	CMD_SetUserInfo				= 6,
	CMD_GetOptions				= 7,
	CMD_SetOptions				= 8,
	CMD_GetMediaToc				= 9,
	CMD_LookupMediaByToc		= 10,
	CMD_GetMatchedDiscInfo		= 11,
	CMD_InvokeFuzzyMatchDialog	= 12,
	CMD_GetFullDiscInfo			= 13,
	CMD_GetDiscInfo				= 14,
	CMD_InvokeDiscInfo			= 15,
	CMD_DisplayDiscInfo			= 16,
	CMD_GetSubmitDisc			= 17,
	CMD_SubmitDisc				= 18,
	CMD_InvokeSubmitDisc		= 19,
	CMD_GetMediaTagId			= 100,
	CMD_LookupMediaByTagId		= 101,
	CMD_LookupMediaByFile		= 102,
	CMD_GetGenreList			= 20,
	CMD_GetGenreTree			= 21,
	CMD_GetGenreInfo			= 22,
	CMD_GetRegionList			= 23,
	CMD_GetRegionInfo			= 24,
	CMD_GetRoleList				= 25,
	CMD_GetRoleTree				= 26,
	CMD_GetRoleInfo				= 27,
	CMD_GetLanguageList			= 28,
	CMD_GetLanguageInfo			= 29,
	CMD_GetFieldList			= 30,
	CMD_GetFieldInfo			= 31,
	CMD_GetURLList				= 32,
	CMD_GetCoverURL				= 33,
	CMD_GetURLManager			= 34,
	CMD_Cancel					= 35,
	CMD_Status					= 36,
	CMD_GetServiceStatus		= 37,
	CMD_ServerNoop				= 38,
	CMD_FlushLocalCache			= 39,
	CMD_UpdateControl			= 40
} CDDBCommands;

typedef enum CDDBLogFlags
{
	LOG_DEFAULT					= 0,
	LOG_OFF						= 0x1,
	LOG_WINDOW					= 0x2,
	LOG_FILE					= 0x4,
	LOG_APP						= 0x8,
	LOG_DEST_MASK				= 0xff,
	LOG_EVENT_MASK				= 0xffffff00,
	LOG_EVENT_UI				= 0x100,
	LOG_EVENT_DEVICE			= 0x200,
	LOG_EVENT_SERVER			= 0x400,
	LOG_EVENT_EVENTS			= 0x800,
	LOG_EVENT_COMPLETION		= 0x1000,
	LOG_EVENT_ERROR				= 0x10000000
} CDDBLogFlags;

typedef enum CDDBUIFlags
{
	UI_NONE						= 0,
	UI_READONLY					= 0x1,
	UI_EDITMODE					= 0x2,
	UI_SUBMITNEW				= 0x8,
	UI_OK						= 0x100,
	UI_CANCEL					= 0x200,
	UI_DATA_CHANGED				= 0x400,
	UI_FULL						= 0x1000,
	UI_SHORT					= 0x2000,
	UI_DISP_PROGRESS			= 0x10000,
	UI_DISP_STATIC				= 0x20000,
	UI_DISP_BONUS				= 0x40000
} CDDBUIFlags;

typedef enum CDDBTagFlags
{
	TAG_MERGE_DEFAULT			= 0,
	TAG_MERGE_ALL				= 0x1,
	TAG_MERGE_ID_ONLY			= 0x2
} CDDBTagFlags;

typedef enum CDDBEventCodes
{
	EVENT_COMMAND_COMPLETED		= 1,
	EVENT_LOG_MESSAGE			= 2,
	EVENT_SERVER_MESSAGE		= 3,
	EVENT_COMMAND_PROGRESS		= 4
} CDDBEventCodes;

typedef enum CDDBProperty
{
	PROP_Default			= 0x00000000,
	PROP_DomainMask			= 0xff000000,
	PROP_Title				= 0x1,
	PROP_Artist				= 0x2,
	PROP_Label				= 0x4,
	PROP_Year				= 0x8,
	PROP_Notes				= 0x10,
	PROP_GenreId			= 0x20,
	PROP_SecondaryGenreId	= 0x40,
	PROP_RegionId			= 0x80,
	PROP_TotalInSet			= 0x100,
	PROP_NumberInSet		= 0x200,
	PROP_Certifier			= 0x400,
	PROP_TitleSort			= 0x800,
	PROP_TitleThe			= 0x1000,
	PROP_ArtistFirstName	= 0x2000,
	PROP_ArtistLastName		= 0x4000,
	PROP_ArtistThe			= 0x8000,
	PROP_Lyrics				= 0x10000,
	PROP_BeatsPerMinute		= 0x20000,
	PROP_ISRC				= 0x40000,
	PROP_Name				= 0x80000,
	PROP_StartTrack			= 0x100000,
	PROP_StartFrame			= 0x200000,
	PROP_EndTrack			= 0x400000,
	PROP_EndFrame			= 0x800000,
	PROP_Href				= 0x1000001,
	PROP_DisplayLink		= 0x1000002,
	PROP_Description		= 0x1000004,
	PROP_Category			= 0x1000008,
	PROP_Size				= 0x1000010,
	PROP_DisplayText		= 0x1000020,
	PROP_Id					= 0x1000040,
	PROP_Compilation		= 0x1000080
} CDDBProperty;

/*
 *	NOTE:	The following enums must be kept consistent with
 *			those defined in cddbslib.h.
 */
typedef enum CDDBMatchCode
{
	MATCH_NONE					= 1,
	MATCH_MULTIPLE				= 2,
	MATCH_EXACT					= 3,
	MATCH_CODE_SIZER			= 0xFFFFFFFF	/* force the size of a CDDBMatchCode to 32 bits */
} CDDBMatchCode;

typedef enum CDDBProgressCodes
{
	CMD_CONNECTING				= 1,
	CMD_SENDING					= 2,
	CMD_RECEIVING				= 3,
	CMD_CANCELLED				= 4,
	CMD_WAITING					= 5,
	CMD_COMPLETED				= 6
} CDDBProgressCodes;

typedef enum CDDBCacheFlags
{
	CACHE_DEFAULT				= 0,
	CACHE_DONT_CONNECT			= 0x1,
	CACHE_DONT_CREATE			= 0x2,
	CACHE_DONT_WRITE_MEDIA		= 0x10,
	CACHE_DONT_WRITE_LIST		= 0x20,
	CACHE_DONT_WRITE_URL		= 0x40,
	CACHE_DONT_WRITE_ANY		= 0xF0,
	CACHE_NO_LOOKUP_MEDIA		= 0x100,
	CACHE_NO_LOOKUP_LIST		= 0x200,
	CACHE_NO_LOOKUP_URL			= 0x400,
	CACHE_NO_LOOKUP_ANY			= 0xF00,
	CACHE_SUBMIT_UPDATE			= 0x1000,
	CACHE_SUBMIT_NEW			= 0x2000,
	CACHE_SUBMIT_OFFLINE		= 0x4000,
	CACHE_SUBMIT_ALL			= 0xF000,
	CACHE_UPDATE_FUZZY			= 0x10000,
	CACHE_UPDATE_CHECK			= 0x20000

} CDDBCacheFlags;

typedef enum CDDBFlushFlags
{
	FLUSH_DEFAULT				= 0,
	FLUSH_USERINFO				= 0x1,
	FLUSH_CLIENTINFO			= 0x2,
	FLUSH_REGISTRY				= 0x4,
	FLUSH_CACHE_MEDIA			= 0x10,
	FLUSH_CACHE_LISTS			= 0x20,
	FLUSH_CACHE_URLS			= 0x40,
	FLUSH_MEMORY_MEDIA			= 0x100,
	FLUSH_MEMORY_LISTS			= 0x200,
	FLUSH_MEMORY_URLS			= 0x400
} CDDBFlushFlags;

typedef enum CDDBServerMessageCodes
{
	MSG_UPDATE					= 1,
	MSG_ENABLE					= 2,
	MSG_DISABLE					= 3,
	MSG_MESSAGE					= 4
} CDDBServerMessageCodes;

typedef enum CDDBServerMessageActions
{
	ACTION_NONE					= 1,
	ACTION_RECOMMENDED			= 2,
	ACTION_REQUIRED				= 3
} CDDBServerMessageActions;

#endif
