//#ifndef TILOG_TILOGOVERRIDE_H
//#define TILOG_TILOGOVERRIDE_H

#ifdef TILOG_SUB_SYSTEM_DECLARE
namespace tilogspace
{
	enum ETiLogSubSysIDEx : sub_sys_t
	{
		TILOG_SUB_SYSTEM_0 = 0,
		TILOG_SUB_SYSTEM_1 = 1,
		TILOG_SUB_SYSTEM_2 = 2,
	};

#ifndef TILOG_CURRENT_SUBSYS_ID
#define TILOG_CURRENT_SUBSYS_ID tilogspace::TILOG_SUB_SYSTEM_0
#endif


	constexpr static TiLogSubsysCfg TILOG_STATIC_SUB_SYS_CFGS[] = {
		{ TILOG_SUB_SYSTEM_0, "mod0", "a:/mod0/", PRINTER_TILOG_FILE, VERBOSE, false },
		{ TILOG_SUB_SYSTEM_1, "mod1", "a:/mod1/", PRINTER_TILOG_FILE, VERBOSE, false },
		{ TILOG_SUB_SYSTEM_2, "mod2", "a:/mod2/", PRINTER_TILOG_FILE, VERBOSE, false },
	};
	constexpr static size_t TILOG_STATIC_SUB_SYS_SIZE = sizeof(TILOG_STATIC_SUB_SYS_CFGS) / sizeof(TILOG_STATIC_SUB_SYS_CFGS[0]);
}	 // namespace tilogspace
#endif





//#endif	  // TILOG_TILOGOVERRIDE_H
