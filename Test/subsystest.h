//#ifndef TILOG_TILOGOVERRIDE_H
//#define TILOG_TILOGOVERRIDE_H

#ifdef TILOG_SUB_SYSTEM_DECLARE
namespace tilogspace
{
	// example: sub system for a nas server
	enum ETiLogSubSysIDEx : sub_sys_t
	{
		TILOG_SUB_MAIN = 1,
		TILOG_SUB_STOR = 2,
		TILOG_SUB_NETWORK = 3,
	};

#ifndef TILOG_CURRENT_SUBSYS_ID
#define TILOG_CURRENT_SUBSYS_ID tilogspace::TILOG_SUB_MAIN
#endif


	constexpr static TiLogSubsysCfg TILOG_STATIC_SUB_SYS_CFGS[] = {
		{ TILOG_SUB_SYSTEM_INTERNAL, "tilog", "a:/tilog/", PRINTER_TILOG_FILE, INFO, false },
		{ TILOG_SUB_MAIN, "main", "a:/main/", PRINTER_TILOG_FILE, VERBOSE, false },
		{ TILOG_SUB_STOR, "stor", "a:/stor/", PRINTER_TILOG_FILE, VERBOSE, false },
		{ TILOG_SUB_NETWORK, "network", "a:/network/", PRINTER_TILOG_FILE, VERBOSE, false },
	};
	constexpr static size_t TILOG_STATIC_SUB_SYS_SIZE = sizeof(TILOG_STATIC_SUB_SYS_CFGS) / sizeof(TILOG_STATIC_SUB_SYS_CFGS[0]);
}	 // namespace tilogspace
#endif





//#endif	  // TILOG_TILOGOVERRIDE_H
