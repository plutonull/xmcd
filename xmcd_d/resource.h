/*
 *   xmcd - Motif(R) CD Audio Player/Ripper
 *
 *   Copyright (C) 1993-2004  Ti Kan
 *   E-mail: xmcd@amb.org
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#ifndef lint
static char *_resource_h_ident_ = "@(#)resource.h	7.49 04/02/13";
#endif


/* X resources */
#define XmcdNversion			"version"
#define XmcdCVersion			"Version"
#define XmcdNmainWindowMode		"mainWindowMode"
#define XmcdCMainWindowMode		"MainWindowMode"
#define XmcdNmodeChangeGravity		"modeChangeGravity"
#define XmcdCModeChangeGravity		"ModeChangeGravity"
#define XmcdNnormalMainWidth		"normalMainWidth"
#define XmcdCNormalMainWidth		"NormalMainWidth"
#define XmcdNnormalMainHeight		"normalMainHeight"
#define XmcdCNormalMainHeight		"NormalMainHeight"
#define XmcdNbasicMainWidth		"basicMainWidth"
#define XmcdCBasicMainWidth		"BasicMainWidth"
#define XmcdNbasicMainHeight		"basicMainHeight"
#define XmcdCBasicMainHeight		"BasicMainHeight"
#define XmcdNdisplayBlinkOnInterval	"displayBlinkOnInterval"
#define XmcdCDisplayBlinkOnInterval	"DisplayBlinkOnInterval"
#define XmcdNdisplayBlinkOffInterval	"displayBlinkOffInterval"
#define XmcdCDisplayBlinkOffInterval	"DisplayBlinkOffInterval"
#define XmcdNmainShowFocus		"mainShowFocus"
#define XmcdCMainShowFocus		"MainShowFocus"
#define XmcdNinstallColormap		"installColormap"
#define XmcdCInstallColormap		"InstallColormap"
#define XmcdNremoteMode			"remoteMode"
#define XmcdCRemoteMode			"RemoteMode"
#define XmcdNremoteHost			"remoteHost"
#define XmcdCRemoteHost			"RemoteHost"

/* Common config parameters - these are here because these are
 * modifiable via command line options.
 */
#define XmcdNdevice			"device"
#define XmcdCDevice			"Device"
#define XmcdNoutputPort			"outputPort"
#define XmcdCOutputPort			"OutputPort"
#define XmcdNdebugLevel			"debugLevel"
#define XmcdCDebugLevel			"DebugLevel"

/* Device-specific config parameters - these are here because these are
 * modifiable via command line options.
 */
#define XmcdNexitOnEject		"exitOnEject"
#define XmcdCExitOnEject		"ExitOnEject"

/* Various application message strings */
#define XmcdNmainWindowTitle		"mainWindowTitle"
#define XmcdCMainWindowTitle		"MainWindowTitle"
#define XmcdNlocalMsg			"localMsg"
#define XmcdCLocalMsg			"LocalMsg"
#define XmcdNcddbMsg			"cddbMsg"
#define XmcdCCddbMsg			"CddbMsg"
#define XmcdNcdTextMsg			"cdTextMsg"
#define XmcdCCdTextMsg			"CdTextMsg"
#define XmcdNqueryMsg			"queryMsg"
#define XmcdCQueryMsg			"QueryMsg"
#define XmcdNprogModeMsg		"progModeMsg"
#define XmcdCProgModeMsg		"ProgModeMsg"
#define XmcdNelapseMsg			"elapseMsg"
#define XmcdCElapseMsg			"ElapseMsg"
#define XmcdNelapseSegmentMsg		"elapseSegmentMsg"
#define XmcdCElapseSegmentMsg		"ElapseSegmentMsg"
#define XmcdNelapseDiscMsg		"elapseDiscMsg"
#define XmcdCElapseDiscMsg		"ElapseDiscMsg"
#define XmcdNremainTrackMsg		"remainTrackMsg"
#define XmcdCRemainTrackMsg		"RemainTrackMsg"
#define XmcdNremainSegmentMsg		"remainSegmentMsg"
#define XmcdCRemainSegmentMsg		"RemainSegmentMsg"
#define XmcdNremainDiscMsg		"remainDiscMsg"
#define XmcdCRemainDiscMsg		"RemainDiscMsg"
#define XmcdNplayMsg			"playMsg"
#define XmcdCPlayMsg			"PlayMsg"
#define XmcdNpauseMsg			"pauseMsg"
#define XmcdCPauseMsg			"PauseMsg"
#define XmcdNreadyMsg			"readyMsg"
#define XmcdCReadyMsg			"ReadyMsg"
#define XmcdNsampleMsg			"sampleMsg"
#define XmcdCSampleMsg			"SampleMsg"
#define XmcdNbadOptsMsg			"badOptsMsg"
#define XmcdCBadOptsMsg			"BadOptsMsg"
#define XmcdNnoDiscMsg			"noDiscMsg"
#define XmcdCNoDiscMsg			"NoDiscMsg"
#define XmcdNdevBusyMsg			"devBusyMsg"
#define XmcdCDevBusyMsg			"DevBusyMsg"
#define XmcdNunknownArtistMsg		"unknownArtistMsg"
#define XmcdCUnknownArtistMsg		"UnknownArtistMsg"
#define XmcdNunknownDiscMsg		"unknownDiscMsg"
#define XmcdCUnknownDiscMsg		"UnknownDiscMsg"
#define XmcdNunknownTrackMsg		"unknownTrackMsg"
#define XmcdCUnknownTrackMsg		"UnknownTrackMsg"
#define XmcdNdataMsg			"dataMsg"
#define XmcdCDataMsg			"DataMsg"
#define XmcdNinfoMsg			"infoMsg"
#define XmcdCInfoMsg			"InfoMsg"
#define XmcdNwarningMsg			"warningMsg"
#define XmcdCWarningMsg			"WarningMsg"
#define XmcdNfatalMsg			"fatalMsg"
#define XmcdCFatalMsg			"FatalMsg"
#define XmcdNconfirmMsg			"confirmMsg"
#define XmcdCConfirmMsg			"ConfirmMsg"
#define XmcdNworkingMsg			"workingMsg"
#define XmcdCWorkingMsg			"WorkingMsg"
#define XmcdNaboutMsg			"aboutMsg"
#define XmcdCAboutMsg			"AboutMsg"
#define XmcdNquitMsg			"quitMsg"
#define XmcdCQuitMsg			"QuitMsg"
#define XmcdNaskProceedMsg		"askProceedMsg"
#define XmcdCAskProceedMsg		"AskProceedMsg"
#define XmcdNclearProgramMsg		"clearProgramMsg"
#define XmcdCClearProgramMsg		"ClearProgramMsg"
#define XmcdNcancelSegmentMsg		"cancelSegmentMsg"
#define XmcdCCancelSegmentMsg		"CancelSegmentMsg"
#define XmcdNrestartPlayMsg		"restartPlayMsg"
#define XmcdCRestartPlayMsg		"RestartPlayMsg"
#define XmcdNnoMemMsg			"noMemMsg"
#define XmcdCNoMemMsg			"NoMemMsg"
#define XmcdNtmpdirErrMsg		"tmpdirErrMsg"
#define XmcdCTmpdirErrMsg		"TmpdirErrMsg"
#define XmcdNlibdirErrMsg		"libdirErrMsg"
#define XmcdCLibdirErrMsg		"LibdirErrMsg"
#define XmcdNlongPathErrMsg		"longPathErrMsg"
#define XmcdCLongPathErrMsg		"LongPathErrMsg"
#define XmcdNnoMethodErrMsg		"noMethodErrMsg"
#define XmcdCNoMethodErrMsg		"NoMethodErrMsg"
#define XmcdNnoVuErrMsg			"noVuErrMsg"
#define XmcdCNoVuErrMsg			"NoVuErrMsg"
#define XmcdNnoHelpMsg			"noHelpMsg"
#define XmcdCNoHelpMsg			"NoHelpMsg"
#define XmcdNnoLinkMsg			"noLinkMsg"
#define XmcdCNoLinkMsg			"NoLinkMsg"
#define XmcdNnoDbMsg			"noDbMsg"
#define XmcdCNoDbMsg			"NoDbMsg"
#define XmcdNnoCfgMsg			"noCfgMsg"
#define XmcdCNoCfgMsg			"NoCfgMsg"
#define XmcdNnoInfoMsg			"noInfoMsg"
#define XmcdCNoInfoMsg			"NoInfoMsg"
#define XmcdNnotRomMsg			"notRomMsg"
#define XmcdCNotRomMsg			"NotRomMsg"
#define XmcdNnotScsi2Msg		"notScsi2Msg"
#define XmcdCNotScsi2Msg		"NotScsi2Msg"
#define XmcdNsendConfirmMsg		"sendConfirmMsg"
#define XmcdCSendConfirmMsg		"SendConfirmMsg"
#define XmcdNsubmitCorrectionMsg	"submitCorrectionMsg"
#define XmcdCSubmitCorrectionMsg	"SubmitCorrectionMsg"
#define XmcdNsubmitErrMsg		"submitErrMsg"
#define XmcdCSubmitErrMsg		"SubmitErrMsg"
#define XmcdNsubmitOkMsg		"submitOkMsg"
#define XmcdCSubmitOkMsg		"SubmitOkMsg"
#define XmcdNmodeErrMsg			"modeErrMsg"
#define XmcdCModeErrMsg			"ModeErrMsg"
#define XmcdNstatErrMsg			"statErrMsg"
#define XmcdCStatErrMsg			"StatErrMsg"
#define XmcdNnodeErrMsg			"nodeErrMsg"
#define XmcdCNodeErrMsg			"NodeErrMsg"
#define XmcdNdbIncmplErrMsg		"dbIncmplErrMsg"
#define XmcdCDbIncmplErrMsg		"DbIncmplErrMsg"
#define XmcdNseqFmtErrMsg		"seqFmtErrMsg"
#define XmcdCSeqFmtErrMsg		"SeqFmtErrMsg"
#define XmcdNinvPgmTrkMsg		"invPgmTrkMsg"
#define XmcdCInvPgmTrkMsg		"InvPgmTrkMsg"
#define XmcdNrecovErrMsg		"recovErrMsg"
#define XmcdCRecovErrMsg		"RecovErrMsg"
#define XmcdNmaxErrMsg			"maxErrMsg"
#define XmcdCMaxErrMsg			"MaxErrMsg"
#define XmcdNsavErrForkMsg		"savErrForkMsg"
#define XmcdCSavErrForkMsg		"SavErrForkMsg"
#define XmcdNsavErrSuidMsg		"savErrSuidMsg"
#define XmcdCSavErrSuidMsg		"SavErrSuidMsg"
#define XmcdNsavErrOpenMsg		"savErrOpenMsg"
#define XmcdCSavErrOpenMsg		"SavErrOpenMsg"
#define XmcdNsavErrCloseMsg		"savErrCloseMsg"
#define XmcdCSavErrCloseMsg		"SavErrCloseMsg"
#define XmcdNsavErrWriteMsg		"savErrWriteMsg"
#define XmcdCSavErrWriteMsg		"SavErrWriteMsg"
#define XmcdNsavErrKilledMsg		"savErrKilledMsg"
#define XmcdCSavErrKilledMsg		"SavErrKilledMsg"
#define XmcdNsearchDbMsg		"searchDbMsg"
#define XmcdCSearchDbMsg		"SearchDbMsg"
#define XmcdNchangeSaveMsg		"changeSaveMsg"
#define XmcdCChangeSaveMsg		"ChangeSaveMsg"
#define XmcdNdevlistUndefMsg		"devlistUndefMsg"
#define XmcdCDevlistUndefMsg		"DevlistUndefMsg"
#define XmcdNdevlistCountMsg		"devlistCountMsg"
#define XmcdCDevlistCountMsg		"DevlistCountMsg"
#define XmcdNchangerInitErrMsg		"changerInitErrMsg"
#define XmcdCChangerInitErrMsg		"ChangerInitErrMsg"
#define XmcdNproxyAuthFailMsg		"proxyAuthFailMsg"
#define XmcdCProxyAuthFailMsg		"ProxyAuthFailMsg"
#define XmcdNnoClientMsg		"noClientMsg"
#define XmcdCNoClientMsg		"NoClientMsg"
#define XmcdNunsuppCmdMsg		"unsuppCmdMsg"
#define XmcdCUnsuppCmdMsg		"UnsuppCmdMsg"
#define XmcdNbadArgMsg			"badArgMsg"
#define XmcdCBadArgMsg			"BadArgMsg"
#define XmcdNinvalidCmdMsg		"invalidCmdMsg"
#define XmcdCInvalidCmdMsg		"InvalidCmdMsg"
#define XmcdNcommandFailMsg		"commandFailMsg"
#define XmcdCCommandFailMsg		"CommandFailMsg"
#define XmcdNremoteNotEnabledMsg	"remoteNotEnabledMsg"
#define XmcdCRemoteNotEnabledMsg	"RemoteNotEnabledMsg"
#define XmcdNremoteNoCmdMsg		"remoteNoCmdMsg"
#define XmcdCRemoteNoCmdMsg		"RemoteNoCmdMsg"
#define XmcdNappDefFileMsg		"appDefFileMsg"
#define XmcdCAppDefFileMsg		"AppDefFileMsg"
#define XmcdNkpModeDisableMsg		"kpModeDisableMsg"
#define XmcdCKpModeDisableMsg		"KpModeDisableMsg"
#define XmcdNdeleteAllHistoryMsg	"deleteAllHistoryMsg"
#define XmcdCDeleteAllHistoryMsg	"DeleteAllHistoryMsg"
#define XmcdNchangerScanningMsg		"changerScanningMsg"
#define XmcdCChangerScanningMsg		"ChangerScanningMsg"
#define XmcdNtheMsg			"theMsg"
#define XmcdCTheMsg			"TheMsg"
#define XmcdNnoneOfAboveMsg		"noneOfAboveMsg"
#define XmcdCNoneOfAboveMsg		"NoneOfAboveMsg"
#define XmcdNerrorMsg			"errorMsg"
#define XmcdCErrorMsg			"ErrorMsg"
#define XmcdNhandleRequiredMsg		"handleRequiredMsg"
#define XmcdCHandleRequiredMsg		"HandleRequiredMsg"
#define XmcdNhandleErrorMsg		"handleErrorMsg"
#define XmcdCHandleErrorMsg		"HandleErrorMsg"
#define XmcdNpasswdRequiredMsg		"passwdRequiredMsg"
#define XmcdCPasswdRequiredMsg		"PasswdRequiredMsg"
#define XmcdNpasswdMatchErrorMsg	"passwdMatchErrorMsg"
#define XmcdCPasswdMatchErrorMsg	"PasswdMatchErrorMsg"
#define XmcdNmailingHintMsg		"mailingHintMsg"
#define XmcdCMailingHintMsg		"MailingHintMsg"
#define XmcdNunknownHandleMsg		"unknownHandleMsg"
#define XmcdCUnknownHandleMsg		"UnknownHandleMsg"
#define XmcdNnoHintMsg			"noHintMsg"
#define XmcdCNoHintMsg			"NoHintMsg"
#define XmcdNnoMailHintMsg		"noMailHintMsg"
#define XmcdCNoMailHintMsg		"NoMailHintMsg"
#define XmcdNhintErrorMsg		"hintErrorMsg"
#define XmcdCHintErrorMsg		"HintErrorMsg"
#define XmcdNuserRegFailMsg		"userRegFailMsg"
#define XmcdCUserRegFailMsg		"UserRegFailMsg"
#define XmcdNnoWwwwarpMsg		"noWwwwarpMsg"
#define XmcdCNoWwwwarpMsg		"NoWwwwarpMsg"
#define XmcdNcannotInvokeMsg		"cannotInvokeMsg"
#define XmcdCCannotInvokeMsg		"CannotInvokeMsg"
#define XmcdNstopLoadMsg		"stopLoadMsg"
#define XmcdCStopLoadMsg		"StopLoadMsg"
#define XmcdNreloadMsg			"reloadMsg"
#define XmcdCReloadMsg			"ReloadMsg"
#define XmcdNneedRoleMsg		"needRoleMsg"
#define XmcdCNeedRoleMsg		"NeedRoleMsg"
#define XmcdNneedRoleNameMsg		"needRoleNameMsg"
#define XmcdCNeedRoleNameMsg		"NeedRoleNameMsg"
#define XmcdNdupCreditMsg		"dupCreditMsg"
#define XmcdCDupCreditMsg		"DupCreditMsg"
#define XmcdNdupTrackCreditMsg		"dupTrackCreditMsg"
#define XmcdCDupTrackCreditMsg		"DupTrackCreditMsg"
#define XmcdNdupDiscCreditMsg		"dupDiscCreditMsg"
#define XmcdCDupDiscCreditMsg		"DupDiscCreditMsg"
#define XmcdNnoFirstNameMsg		"noFirstNameMsg"
#define XmcdCNoFirstNameMsg		"NoFirstNameMsg"
#define XmcdNnoFirstLastNameMsg		"noFirstLastNameMsg"
#define XmcdCNoFirstLastNameMsg		"NoFirstLastNameMsg"
#define XmcdNalbumArtistMsg		"albumArtistMsg"
#define XmcdCAlbumArtistMsg		"AlbumArtistMsg"
#define XmcdNtrackArtistMsg		"trackArtistMsg"
#define XmcdCTrackArtistMsg		"TrackArtistMsg"
#define XmcdNcreditMsg			"creditMsg"
#define XmcdCCreditMsg			"CreditMsg"
#define XmcdNfullNameGuideMsg		"fullNameGuideMsg"
#define XmcdCFullNameGuideMsg		"FullNameGuideMsg"
#define XmcdNnoCategoryMsg		"noCategoryMsg"
#define XmcdCNoCategoryMsg		"NoCategoryMsg"
#define XmcdNnoNameMsg			"noNameMsg"
#define XmcdCNoNameMsg			"NoNameMsg"
#define XmcdNinvalidUrlMsg		"invalidUrlMsg"
#define XmcdCInvalidUrlMsg		"InvalidUrlMsg"
#define XmcdNsegmentPositionErrMsg	"segmentPositionErrMsg"
#define XmcdCSegmentPositionErrMsg	"SegmentPositionErrMsg"
#define XmcdNincompleteSegmentInfoMsg	"incompleteSegmentInfoMsg"
#define XmcdCIncompleteSegmentInfoMsg	"IncompleteSegmentInfoMsg"
#define XmcdNinvalidSegmentInfoMsg	"invalidSegmentInfoMsg"
#define XmcdCInvalidSegmentInfoMsg	"InvalidSegmentInfoMsg"
#define XmcdNdiscardChangeMsg		"discardChangeMsg"
#define XmcdCDiscardChangeMsg		"DiscardChangeMsg"
#define XmcdNnoAudioPathMsg		"noAudioPathMsg"
#define XmcdCNoAudioPathMsg		"NoAudioPathMsg"
#define XmcdNaudioPathExistsMsg		"audioPathExistsMsg"
#define XmcdCAudioPathExistsMsg		"AudioPathExistsMsg"
#define XmcdNplaybackModeMsg		"playbackModeMsg"
#define XmcdCPlaybackModeMsg		"PlaybackModeMsg"
#define XmcdNencodeParametersMsg	"encodeParametersMsg"
#define XmcdCEncodeParametersMsg	"EncodeParametersMsg"
#define XmcdNlameOptionsMsg		"lameOptionsMsg"
#define XmcdCLameOptionsMsg		"LameOptionsMsg"
#define XmcdNschedParametersMsg		"schedParametersMsg"
#define XmcdCSchedParametersMsg		"SchedParametersMsg"
#define XmcdNautoFuncsMsg		"autoFuncsMsg"
#define XmcdCAutoFuncsMsg		"AutoFuncsMsg"
#define XmcdNcdChangerMsg		"cdChangerMsg"
#define XmcdCCdChangerMsg		"CdChangerMsg"
#define XmcdNchannelRouteMsg		"channelRouteMsg"
#define XmcdCChannelRouteMsg		"ChannelRouteMsg"
#define XmcdNvolumeBalanceMsg		"volumeBalanceMsg"
#define XmcdCVolumeBalanceMsg		"VolumeBalanceMsg"
#define XmcdNcddbCdtextMsg		"cddbCdtextMsg"
#define XmcdCCddbCdtextMsg		"CddbCdtextMsg"
#define XmcdNcddaInitFailMsg		"cddaInitFailMsg"
#define XmcdCCddaInitFailMsg		"CddaInitFailMsg"
#define XmcdNfileNameRequiredMsg	"fileNameRequiredMsg"
#define XmcdCFileNameRequiredMsg	"FileNameRequiredMsg"
#define XmcdNnotRegularFileMsg		"notRegularFileMsg"
#define XmcdCNotRegularFileMsg		"NotRegularFileMsg"
#define XmcdNnoProgramMsg		"noProgramMsg"
#define XmcdCNoProgramMsg		"NoProgramMsg"
#define XmcdNoverwriteMsg		"overwriteMsg"
#define XmcdCOverwriteMsg		"OverwriteMsg"
#define XmcdNcddaOutputDirectoryMsg	"cddaOutputDirectoryMsg"
#define XmcdCCddaOutputDirectoryMsg	"CddaOutputDirectoryMsg"
#define XmcdNcddaExpandedPathMsg	"cddaExpandedPathMsg"
#define XmcdCCddaExpandedPathMsg	"CddaExpandedPathMsg"
#define XmcdNinvalidBrMinMeanMsg	"invalidBrMinMeanMsg"
#define XmcdCInvalidBrMinMeanMsg	"InvalidBrMinMeanMsg"
#define XmcdNinvalidBrMinMaxMsg		"invalidBrMinMaxMsg"
#define XmcdCInvalidBrMinMaxMsg		"InvalidBrMinMaxMsg"
#define XmcdNinvalidBrMaxMeanMsg	"invalidBrMaxMeanMsg"
#define XmcdCInvalidBrMaxMeanMsg	"InvalidBrMaxMeanMsg"
#define XmcdNinvalidBrMaxMinMsg		"invalidBrMaxMinMsg"
#define XmcdCInvalidBrMaxMinMsg		"InvalidBrMaxMinMsg"
#define XmcdNinvalidFreqLPMsg		"invalidFreqLPMsg"
#define XmcdCInvalidFreqLPMsg		"InvalidFreqLPMsg"
#define XmcdNinvalidFreqHPMsg		"invalidFreqHPMsg"
#define XmcdCInvalidFreqHPMsg		"InvalidFreqHPMsg"
#define XmcdNnoMessagesMsg		"noMessagesMsg"
#define XmcdCNoMessagesMsg		"NoMessagesMsg"

/* Short-cut key definitions */
#define XmcdNmainHotkeys		"mainHotkeys"
#define XmcdCMainHotkeys		"MainHotkeys"
#define XmcdNkeypadHotkeys		"keypadHotkeys"
#define XmcdCKeypadHotkeys		"KeypadHotkeys"


STATIC XtResource	resources[] = {
	/* X resources */
	{
		XmcdNversion, XmcdCVersion,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, version), XmRImmediate,
		(XtPointer) NULL,
	},
	{
		XmcdNmainWindowMode, XmcdCMainWindowMode,
		XmRInt, sizeof(int),
		XtOffsetOf(appdata_t, main_mode), XmRImmediate,
		(XtPointer) 0,
	},
	{
		XmcdNmodeChangeGravity, XmcdCModeChangeGravity,
		XmRInt, sizeof(int),
		XtOffsetOf(appdata_t, modechg_grav), XmRImmediate,
		(XtPointer) 0,
	},
	{
		XmcdNnormalMainWidth, XmcdCNormalMainWidth,
		XmRInt, sizeof(int),
		XtOffsetOf(appdata_t, normal_width), XmRImmediate,
		(XtPointer) 360,
	},
	{
		XmcdNnormalMainHeight, XmcdCNormalMainHeight,
		XmRInt, sizeof(int),
		XtOffsetOf(appdata_t, normal_height), XmRImmediate,
		(XtPointer) 135,
	},
	{
		XmcdNbasicMainWidth, XmcdCBasicMainWidth,
		XmRInt, sizeof(int),
		XtOffsetOf(appdata_t, basic_width), XmRImmediate,
		(XtPointer) 195,
	},
	{
		XmcdNbasicMainHeight, XmcdCBasicMainHeight,
		XmRInt, sizeof(int),
		XtOffsetOf(appdata_t, basic_height), XmRImmediate,
		(XtPointer) 60,
	},
	{
		XmcdNmainShowFocus, XmcdCMainShowFocus,
		XmRBoolean, sizeof(Boolean),
		XtOffsetOf(appdata_t, main_showfocus), XmRImmediate,
		(XtPointer) True,
	},
	{
		XmcdNdisplayBlinkOnInterval, XmcdCDisplayBlinkOnInterval,
		XmRInt, sizeof(int),
		XtOffsetOf(appdata_t, blinkon_interval), XmRImmediate,
		(XtPointer) 850,
	},
	{
		XmcdNdisplayBlinkOffInterval, XmcdCDisplayBlinkOffInterval,
		XmRInt, sizeof(int),
		XtOffsetOf(appdata_t, blinkoff_interval), XmRImmediate,
		(XtPointer) 150,
	},
	{
		XmcdNinstallColormap, XmcdCInstallColormap,
		XmRBoolean, sizeof(Boolean),
		XtOffsetOf(appdata_t, instcmap), XmRImmediate,
		(XtPointer) False,
	},
	{
		XmcdNremoteMode, XmcdCRemoteMode,
		XmRBoolean, sizeof(Boolean),
		XtOffsetOf(appdata_t, remotemode), XmRImmediate,
		(XtPointer) False,
	},
	{
		XmcdNremoteHost, XmcdCRemoteHost,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, remotehost), XmRImmediate,
		(XtPointer) NULL,
	},

	/* Common config parameters */
	{
		XmcdNdevice, XmcdCDevice,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, device), XmRImmediate,
		(XtPointer) NULL,
	},
	{
		XmcdNoutputPort, XmcdCOutputPort,
		XmRInt, sizeof(int),
		XtOffsetOf(appdata_t, outport), XmRImmediate,
		(XtPointer) 0,
	},
	{
		XmcdNdebugLevel, XmcdCDebugLevel,
		XmRInt, sizeof(int),
		XtOffsetOf(appdata_t, debug), XmRImmediate,
		(XtPointer) 0,
	},

	/* Device-specific config parameters */
	{
		XmcdNexitOnEject, XmcdCExitOnEject,
		XmRBoolean, sizeof(Boolean),
		XtOffsetOf(appdata_t, eject_exit), XmRImmediate,
		(XtPointer) False,
	},

	/* Various application message strings */
	{
		XmcdNmainWindowTitle, XmcdCMainWindowTitle,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, main_title), XmRImmediate,
		(XtPointer) MAIN_TITLE,
	},
	{
		XmcdNlocalMsg, XmcdCLocalMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_local), XmRImmediate,
		(XtPointer) STR_LOCAL,
	},
	{
		XmcdNcddbMsg, XmcdCCddbMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_cddb), XmRImmediate,
		(XtPointer) STR_CDDB,
	},
	{
		XmcdNcdTextMsg, XmcdCCdTextMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_cdtext), XmRImmediate,
		(XtPointer) STR_CDTEXT,
	},
	{
		XmcdNqueryMsg, XmcdCQueryMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_query), XmRImmediate,
		(XtPointer) STR_QUERY,
	},
	{
		XmcdNprogModeMsg, XmcdCProgModeMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_progmode), XmRImmediate,
		(XtPointer) STR_PROGMODE,
	},
	{
		XmcdNelapseMsg, XmcdCElapseMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_elapse), XmRImmediate,
		(XtPointer) STR_ELAPSE,
	},
	{
		XmcdNelapseSegmentMsg, XmcdCElapseSegmentMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_elapseseg), XmRImmediate,
		(XtPointer) STR_ELAPSE_SEG,
	},
	{
		XmcdNelapseDiscMsg, XmcdCElapseDiscMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_elapsedisc), XmRImmediate,
		(XtPointer) STR_ELAPSE_DISC,
	},
	{
		XmcdNremainTrackMsg, XmcdCRemainTrackMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_remaintrk), XmRImmediate,
		(XtPointer) STR_REMAIN_TRK,
	},
	{
		XmcdNremainSegmentMsg, XmcdCRemainSegmentMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_remainseg), XmRImmediate,
		(XtPointer) STR_REMAIN_SEG,
	},
	{
		XmcdNremainDiscMsg, XmcdCRemainDiscMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_remaindisc), XmRImmediate,
		(XtPointer) STR_REMAIN_DISC,
	},
	{
		XmcdNplayMsg, XmcdCPlayMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_play), XmRImmediate,
		(XtPointer) STR_PLAY,
	},
	{
		XmcdNpauseMsg, XmcdCPauseMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_pause), XmRImmediate,
		(XtPointer) STR_PAUSE,
	},
	{
		XmcdNreadyMsg, XmcdCReadyMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_ready), XmRImmediate,
		(XtPointer) STR_READY,
	},
	{
		XmcdNsampleMsg, XmcdCSampleMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_sample), XmRImmediate,
		(XtPointer) STR_SAMPLE,
	},
	{
		XmcdNbadOptsMsg, XmcdCBadOptsMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_badopts), XmRImmediate,
		(XtPointer) STR_BADOPTS,
	},
	{
		XmcdNnoDiscMsg, XmcdCNoDiscMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_nodisc), XmRImmediate,
		(XtPointer) STR_NODISC,
	},
	{
		XmcdNdevBusyMsg, XmcdCDevBusyMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_busy), XmRImmediate,
		(XtPointer) STR_BUSY,
	},
	{
		XmcdNunknownArtistMsg, XmcdCUnknownArtistMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_unknartist), XmRImmediate,
		(XtPointer) STR_UNKNARTIST,
	},
	{
		XmcdNunknownDiscMsg, XmcdCUnknownDiscMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_unkndisc), XmRImmediate,
		(XtPointer) STR_UNKNDISC,
	},
	{
		XmcdNunknownTrackMsg, XmcdCUnknownTrackMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_unkntrk), XmRImmediate,
		(XtPointer) STR_UNKNTRK,
	},
	{
		XmcdNdataMsg, XmcdCDataMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_data), XmRImmediate,
		(XtPointer) STR_DATA,
	},
	{
		XmcdNinfoMsg, XmcdCInfoMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_info), XmRImmediate,
		(XtPointer) STR_INFO,
	},
	{
		XmcdNwarningMsg, XmcdCWarningMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_warning), XmRImmediate,
		(XtPointer) STR_WARNING,
	},
	{
		XmcdNfatalMsg, XmcdCFatalMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_fatal), XmRImmediate,
		(XtPointer) STR_FATAL,
	},
	{
		XmcdNconfirmMsg, XmcdCConfirmMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_confirm), XmRImmediate,
		(XtPointer) STR_CONFIRM,
	},
	{
		XmcdNworkingMsg, XmcdCWorkingMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_working), XmRImmediate,
		(XtPointer) STR_WORKING,
	},
	{
		XmcdNaboutMsg, XmcdCAboutMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_about), XmRImmediate,
		(XtPointer) STR_ABOUT,
	},
	{
		XmcdNquitMsg, XmcdCQuitMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_quit), XmRImmediate,
		(XtPointer) STR_QUIT,
	},
	{
		XmcdNaskProceedMsg, XmcdCAskProceedMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_askproceed), XmRImmediate,
		(XtPointer) STR_ASKPROCEED,
	},
	{
		XmcdNclearProgramMsg, XmcdCClearProgramMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_clrprog), XmRImmediate,
		(XtPointer) STR_CLRPROG,
	},
	{
		XmcdNcancelSegmentMsg, XmcdCCancelSegmentMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_cancelseg), XmRImmediate,
		(XtPointer) STR_CANCELSEG,
	},
	{
		XmcdNrestartPlayMsg, XmcdCRestartPlayMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_restartplay), XmRImmediate,
		(XtPointer) STR_RESTARTPLAY,
	},
	{
		XmcdNnoMemMsg, XmcdCNoMemMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_nomemory), XmRImmediate,
		(XtPointer) STR_NOMEMORY,
	},
	{
		XmcdNnoMethodErrMsg, XmcdCNoMethodErrMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_nomethod), XmRImmediate,
		(XtPointer) STR_NOMETHOD,
	},
	{
		XmcdNnoVuErrMsg, XmcdCNoVuErrMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_novu), XmRImmediate,
		(XtPointer) STR_NOVU,
	},
	{
		XmcdNtmpdirErrMsg, XmcdCTmpdirErrMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_tmpdirerr), XmRImmediate,
		(XtPointer) STR_TMPDIRERR,
	},
	{
		XmcdNlibdirErrMsg, XmcdCLibdirErrMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_libdirerr), XmRImmediate,
		(XtPointer) STR_LIBDIRERR,
	},
	{
		XmcdNlongPathErrMsg, XmcdCLongPathErrMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_longpatherr), XmRImmediate,
		(XtPointer) STR_LONGPATHERR,
	},
	{
		XmcdNnoHelpMsg, XmcdCNoHelpMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_nohelp), XmRImmediate,
		(XtPointer) STR_NOHELP,
	},
	{
		XmcdNnoCfgMsg, XmcdCNoCfgMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_nocfg), XmRImmediate,
		(XtPointer) STR_NOCFG,
	},
	{
		XmcdNnoInfoMsg, XmcdCNoInfoMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_noinfo), XmRImmediate,
		(XtPointer) STR_NOINFO,
	},
	{
		XmcdNnotRomMsg, XmcdCNotRomMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_notrom), XmRImmediate,
		(XtPointer) STR_NOTROM,
	},
	{
		XmcdNnotScsi2Msg, XmcdCNotScsi2Msg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_notscsi2), XmRImmediate,
		(XtPointer) STR_NOTSCSI2,
	},
	{
		XmcdNsendConfirmMsg, XmcdCSendConfirmMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_submit), XmRImmediate,
		(XtPointer) STR_SUBMIT,
	},
	{
		XmcdNsubmitCorrectionMsg, XmcdCSubmitCorrectionMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_submitcorr), XmRImmediate,
		(XtPointer) STR_SUBMITCORR,
	},
	{
		XmcdNsubmitErrMsg, XmcdCSubmitErrMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_submiterr), XmRImmediate,
		(XtPointer) STR_SUBMITERR,
	},
	{
		XmcdNsubmitOkMsg, XmcdCSubmitOkMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_submitok), XmRImmediate,
		(XtPointer) STR_SUBMITOK,
	},
	{
		XmcdNmodeErrMsg, XmcdCModeErrMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_moderr), XmRImmediate,
		(XtPointer) STR_MODERR,
	},
	{
		XmcdNstatErrMsg, XmcdCStatErrMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_staterr), XmRImmediate,
		(XtPointer) STR_STATERR,
	},
	{
		XmcdNnodeErrMsg, XmcdCNodeErrMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_noderr), XmRImmediate,
		(XtPointer) STR_NODERR,
	},
	{
		XmcdNseqFmtErrMsg, XmcdCSeqFmtErrMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_seqfmterr), XmRImmediate,
		(XtPointer) STR_SEQFMTERR,
	},
	{
		XmcdNinvPgmTrkMsg, XmcdCInvPgmTrkMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_invpgmtrk), XmRImmediate,
		(XtPointer) STR_INVPGMTRK,
	},
	{
		XmcdNrecovErrMsg, XmcdCRecovErrMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_recoverr), XmRImmediate,
		(XtPointer) STR_RECOVERR,
	},
	{
		XmcdNmaxErrMsg, XmcdCMaxErrMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_maxerr), XmRImmediate,
		(XtPointer) STR_MAXERR,
	},
	{
		XmcdNsavErrForkMsg, XmcdCSavErrForkMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_saverr_fork), XmRImmediate,
		(XtPointer) STR_SAVERR_FORK,
	},
	{
		XmcdNsavErrSuidMsg, XmcdCSavErrSuidMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_saverr_suid), XmRImmediate,
		(XtPointer) STR_SAVERR_SUID,
	},
	{
		XmcdNsavErrOpenMsg, XmcdCSavErrOpenMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_saverr_open), XmRImmediate,
		(XtPointer) STR_SAVERR_OPEN,
	},
	{
		XmcdNsavErrCloseMsg, XmcdCSavErrCloseMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_saverr_close), XmRImmediate,
		(XtPointer) STR_SAVERR_CLOSE,
	},
	{
		XmcdNsavErrWriteMsg, XmcdCSavErrWriteMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_saverr_write), XmRImmediate,
		(XtPointer) STR_SAVERR_WRITE,
	},
	{
		XmcdNsavErrKilledMsg, XmcdCSavErrKilledMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_saverr_killed), XmRImmediate,
		(XtPointer) STR_SAVERR_KILLED,
	},
	{
		XmcdNchangeSaveMsg, XmcdCChangeSaveMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_chgsubmit), XmRImmediate,
		(XtPointer) STR_CHGSUBMIT,
	},
	{
		XmcdNdevlistUndefMsg, XmcdCDevlistUndefMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_devlist_undef), XmRImmediate,
		(XtPointer) STR_DEVLIST_UNDEF,
	},
	{
		XmcdNdevlistCountMsg, XmcdCDevlistCountMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_devlist_count), XmRImmediate,
		(XtPointer) STR_DEVLIST_COUNT,
	},
	{
		XmcdNchangerInitErrMsg, XmcdCChangerInitErrMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_medchg_noinit), XmRImmediate,
		(XtPointer) STR_MEDCHG_NOINIT,
	},
	{
		XmcdNproxyAuthFailMsg, XmcdCProxyAuthFailMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_authfail), XmRImmediate,
		(XtPointer) STR_AUTHFAIL,
	},
	{
		XmcdNnoClientMsg, XmcdCNoClientMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_noclient), XmRImmediate,
		(XtPointer) STR_NOCLIENT,
	},
	{
		XmcdNunsuppCmdMsg, XmcdCUnsuppCmdMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_unsuppcmd), XmRImmediate,
		(XtPointer) STR_UNSUPPCMD,
	},
	{
		XmcdNbadArgMsg, XmcdCBadArgMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_badarg), XmRImmediate,
		(XtPointer) STR_BADARG,
	},
	{
		XmcdNinvalidCmdMsg, XmcdCInvalidCmdMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_invcmd), XmRImmediate,
		(XtPointer) STR_INVCMD,
	},
	{
		XmcdNcommandFailMsg, XmcdCCommandFailMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_cmdfail), XmRImmediate,
		(XtPointer) STR_CMDFAIL,
	},
	{
		XmcdNremoteNotEnabledMsg, XmcdCRemoteNotEnabledMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_rmt_notenb), XmRImmediate,
		(XtPointer) STR_RMT_NOTENB,
	},
	{
		XmcdNremoteNoCmdMsg, XmcdCRemoteNoCmdMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_rmt_nocmd), XmRImmediate,
		(XtPointer) STR_RMT_NOCMD,
	},
	{
		XmcdNappDefFileMsg, XmcdCAppDefFileMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_appdef), XmRImmediate,
		(XtPointer) STR_APPDEF,
	},
	{
		XmcdNkpModeDisableMsg, XmcdCKpModeDisableMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_kpmodedsbl), XmRImmediate,
		(XtPointer) STR_KPMODEDSBL,
	},
	{
		XmcdNdeleteAllHistoryMsg, XmcdCDeleteAllHistoryMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_dlist_delall), XmRImmediate,
		(XtPointer) STR_DLIST_DELALL,
	},
	{
		XmcdNchangerScanningMsg, XmcdCChangerScanningMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_chgrscan), XmRImmediate,
		(XtPointer) STR_CHGRSCAN,
	},
	{
		XmcdNtheMsg, XmcdCTheMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_the), XmRImmediate,
		(XtPointer) STR_THE,
	},
	{
		XmcdNnoneOfAboveMsg, XmcdCNoneOfAboveMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_noneofabove), XmRImmediate,
		(XtPointer) STR_NONEOFABOVE,
	},
	{
		XmcdNerrorMsg, XmcdCErrorMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_error), XmRImmediate,
		(XtPointer) STR_ERROR,
	},
	{
		XmcdNhandleRequiredMsg, XmcdCHandleRequiredMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_handlereq), XmRImmediate,
		(XtPointer) STR_HANDLEREQ,
	},
	{
		XmcdNhandleErrorMsg, XmcdCHandleErrorMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_handleerr), XmRImmediate,
		(XtPointer) STR_HANDLEERR,
	},
	{
		XmcdNpasswdRequiredMsg, XmcdCPasswdRequiredMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_passwdreq), XmRImmediate,
		(XtPointer) STR_PASSWDREQ,
	},
	{
		XmcdNpasswdMatchErrorMsg, XmcdCPasswdMatchErrorMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_passwdmatcherr), XmRImmediate,
		(XtPointer) STR_PASSWDMATCHERR,
	},
	{
		XmcdNmailingHintMsg, XmcdCMailingHintMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_mailinghint), XmRImmediate,
		(XtPointer) STR_MAILINGHINT,
	},
	{
		XmcdNunknownHandleMsg, XmcdCUnknownHandleMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_unknhandle), XmRImmediate,
		(XtPointer) STR_UNKNHANDLE,
	},
	{
		XmcdNnoHintMsg, XmcdCNoHintMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_nohint), XmRImmediate,
		(XtPointer) STR_NOHINT,
	},
	{
		XmcdNnoMailHintMsg, XmcdCNoMailHintMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_nomailhint), XmRImmediate,
		(XtPointer) STR_NOMAILHINT,
	},
	{
		XmcdNhintErrorMsg, XmcdCHintErrorMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_hinterr), XmRImmediate,
		(XtPointer) STR_HINTERR,
	},
	{
		XmcdNuserRegFailMsg, XmcdCUserRegFailMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_userregfail), XmRImmediate,
		(XtPointer) STR_USERREGFAIL,
	},
	{
		XmcdNnoWwwwarpMsg, XmcdCNoWwwwarpMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_nowwwwarp), XmRImmediate,
		(XtPointer) STR_NOWWWWARP,
	},
	{
		XmcdNcannotInvokeMsg, XmcdCCannotInvokeMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_cannotinvoke), XmRImmediate,
		(XtPointer) STR_CANNOTINVOKE,
	},
	{
		XmcdNstopLoadMsg, XmcdCStopLoadMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_stopload), XmRImmediate,
		(XtPointer) STR_STOPLOAD,
	},
	{
		XmcdNreloadMsg, XmcdCReloadMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_reload), XmRImmediate,
		(XtPointer) STR_RELOAD,
	},
	{
		XmcdNneedRoleMsg, XmcdCNeedRoleMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_needrole), XmRImmediate,
		(XtPointer) STR_NEEDROLE,
	},
	{
		XmcdNneedRoleNameMsg, XmcdCNeedRoleNameMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_needrolename), XmRImmediate,
		(XtPointer) STR_NEEDROLENAME,
	},
	{
		XmcdNdupCreditMsg, XmcdCDupCreditMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_dupcredit), XmRImmediate,
		(XtPointer) STR_DUPCREDIT,
	},
	{
		XmcdNdupTrackCreditMsg, XmcdCDupTrackCreditMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_duptrkcredit), XmRImmediate,
		(XtPointer) STR_DUPTRKCREDIT,
	},
	{
		XmcdNdupDiscCreditMsg, XmcdCDupDiscCreditMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_dupdisccredit), XmRImmediate,
		(XtPointer) STR_DUPDISCCREDIT,
	},
	{
		XmcdNnoFirstNameMsg, XmcdCNoFirstNameMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_nofirst), XmRImmediate,
		(XtPointer) STR_NOFIRST,
	},
	{
		XmcdNnoFirstLastNameMsg, XmcdCNoFirstLastNameMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_nofirstlast), XmRImmediate,
		(XtPointer) STR_NOFIRSTLAST,
	},
	{
		XmcdNalbumArtistMsg, XmcdCAlbumArtistMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_albumartist), XmRImmediate,
		(XtPointer) STR_ALBUMARTIST,
	},
	{
		XmcdNtrackArtistMsg, XmcdCTrackArtistMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_trackartist), XmRImmediate,
		(XtPointer) STR_TRACKARTIST,
	},
	{
		XmcdNcreditMsg, XmcdCCreditMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_credit), XmRImmediate,
		(XtPointer) STR_CREDIT,
	},
	{
		XmcdNfullNameGuideMsg, XmcdCFullNameGuideMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_fnameguide), XmRImmediate,
		(XtPointer) STR_FNAMEGUIDE,
	},
	{
		XmcdNnoCategoryMsg, XmcdCNoCategoryMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_nocateg), XmRImmediate,
		(XtPointer) STR_NOCATEG,
	},
	{
		XmcdNnoNameMsg, XmcdCNoNameMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_noname), XmRImmediate,
		(XtPointer) STR_NONAME,
	},
	{
		XmcdNinvalidUrlMsg, XmcdCInvalidUrlMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_invalurl), XmRImmediate,
		(XtPointer) STR_INVALURL,
	},
	{
		XmcdNsegmentPositionErrMsg, XmcdCSegmentPositionErrMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_segposerr), XmRImmediate,
		(XtPointer) STR_SEGPOSERR,
	},
	{
		XmcdNincompleteSegmentInfoMsg, XmcdCIncompleteSegmentInfoMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_incseginfo), XmRImmediate,
		(XtPointer) STR_INCSEGINFO,
	},
	{
		XmcdNinvalidSegmentInfoMsg, XmcdCInvalidSegmentInfoMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_invseginfo), XmRImmediate,
		(XtPointer) STR_INVSEGINFO,
	},
	{
		XmcdNdiscardChangeMsg, XmcdCDiscardChangeMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_discardchg), XmRImmediate,
		(XtPointer) STR_DISCARDCHG,
	},
	{
		XmcdNnoAudioPathMsg, XmcdCNoAudioPathMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_noaudiopath), XmRImmediate,
		(XtPointer) STR_NOAUDIOPATH,
	},
	{
		XmcdNaudioPathExistsMsg, XmcdCAudioPathExistsMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_audiopathexists), XmRImmediate,
		(XtPointer) STR_AUDIOPATHEXISTS,
	},
	{
		XmcdNplaybackModeMsg, XmcdCPlaybackModeMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_playbackmode), XmRImmediate,
		(XtPointer) STR_PLAYBACKMODE,
	},
	{
		XmcdNencodeParametersMsg, XmcdCEncodeParametersMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_encodeparms), XmRImmediate,
		(XtPointer) STR_ENCODEPARMS,
	},
	{
		XmcdNlameOptionsMsg, XmcdCLameOptionsMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_lameopts), XmRImmediate,
		(XtPointer) STR_LAMEOPTS,
	},
	{
		XmcdNschedParametersMsg, XmcdCSchedParametersMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_schedparms), XmRImmediate,
		(XtPointer) STR_SCHEDPARMS,
	},
	{
		XmcdNautoFuncsMsg, XmcdCAutoFuncsMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_autofuncs), XmRImmediate,
		(XtPointer) STR_AUTOFUNCS,
	},
	{
		XmcdNcdChangerMsg, XmcdCCdChangerMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_cdchanger), XmRImmediate,
		(XtPointer) STR_CDCHANGER,
	},
	{
		XmcdNchannelRouteMsg, XmcdCChannelRouteMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_chroute), XmRImmediate,
		(XtPointer) STR_CHROUTE,
	},
	{
		XmcdNvolumeBalanceMsg, XmcdCVolumeBalanceMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_volbal), XmRImmediate,
		(XtPointer) STR_VOLBAL,
	},
	{
		XmcdNcddbCdtextMsg, XmcdCCddbCdtextMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_cddb_cdtext), XmRImmediate,
		(XtPointer) STR_CDDB_CDTEXT,
	},
	{
		XmcdNcddaInitFailMsg, XmcdCCddaInitFailMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_cddainit_fail), XmRImmediate,
		(XtPointer) STR_CDDAINIT_FAIL,
	},
	{
		XmcdNfileNameRequiredMsg, XmcdCFileNameRequiredMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_filenamereq), XmRImmediate,
		(XtPointer) STR_FILENAMEREQ,
	},
	{
		XmcdNnotRegularFileMsg, XmcdCNotRegularFileMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_notregfile), XmRImmediate,
		(XtPointer) STR_NOTREGFILE,
	},
	{
		XmcdNnoProgramMsg, XmcdCNoProgramMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_noprog), XmRImmediate,
		(XtPointer) STR_NOPROG,
	},
	{
		XmcdNoverwriteMsg, XmcdCOverwriteMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_overwrite), XmRImmediate,
		(XtPointer) STR_OVERWRITE,
	},
	{
		XmcdNcddaOutputDirectoryMsg, XmcdCCddaOutputDirectoryMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_outdir), XmRImmediate,
		(XtPointer) STR_OUTDIR,
	},
	{
		XmcdNcddaExpandedPathMsg, XmcdCCddaExpandedPathMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_pathexp), XmRImmediate,
		(XtPointer) STR_PATHEXP,
	},
	{
		XmcdNinvalidBrMinMeanMsg, XmcdCInvalidBrMinMeanMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_invbr_minmean), XmRImmediate,
		(XtPointer) STR_INVBR_MINMEAN,
	},
	{
		XmcdNinvalidBrMinMaxMsg, XmcdCInvalidBrMinMaxMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_invbr_minmax), XmRImmediate,
		(XtPointer) STR_INVBR_MINMAX,
	},
	{
		XmcdNinvalidBrMaxMeanMsg, XmcdCInvalidBrMaxMeanMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_invbr_maxmean), XmRImmediate,
		(XtPointer) STR_INVBR_MAXMEAN,
	},
	{
		XmcdNinvalidBrMaxMinMsg, XmcdCInvalidBrMaxMinMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_invbr_maxmin), XmRImmediate,
		(XtPointer) STR_INVBR_MAXMIN,
	},
	{
		XmcdNinvalidFreqLPMsg, XmcdCInvalidFreqLPMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_invfreq_lp), XmRImmediate,
		(XtPointer) STR_INVFREQ_LP,
	},
	{
		XmcdNinvalidFreqHPMsg, XmcdCInvalidFreqHPMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_invfreq_hp), XmRImmediate,
		(XtPointer) STR_INVFREQ_HP,
	},
	{
		XmcdNnoMessagesMsg, XmcdCNoMessagesMsg,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, str_nomsg), XmRImmediate,
		(XtPointer) STR_NOMSG,
	},

	/* Short-cut key definitions */
	{
		XmcdNmainHotkeys, XmcdCMainHotkeys,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, main_hotkeys), XmRImmediate,
		(XtPointer) NULL,
	},
	{
		XmcdNkeypadHotkeys, XmcdCKeypadHotkeys,
		XmRString, sizeof(String),
		XtOffsetOf(appdata_t, keypad_hotkeys), XmRImmediate,
		(XtPointer) NULL,
	},
};


STATIC XrmOptionDescRec	options[] = {
	{ "-dev",	"*device",	    XrmoptionSepArg,	NULL	},
	{ "-instcmap",	"*installColormap", XrmoptionNoArg,	"True"	},
	{ "-remote",	"*remoteMode",      XrmoptionNoArg,	"True"	},
	{ "-rmthost",	"*remoteHost",      XrmoptionSepArg,	NULL	},
	{ "-debug",	"*debugLevel",	    XrmoptionSepArg,	NULL	},
#if defined(SVR4) && (defined(sun) || defined(__sun__))
	/* Solaris 2 volume manager auto-startup support */
	{ "-c",		"*device",	    XrmoptionSepArg,	NULL	},
	{ "-X",		"*exitOnEject",	    XrmoptionNoArg,	"True"	},
	{ "-o",		"",		    XrmoptionNoArg,	"False"	},
#endif
};

#endif	/* __RESOURCE_H__ */

