//#ifndef TILOG_TILOGOVERRIDE_H
//#define TILOG_TILOGOVERRIDE_H

#ifdef TILOG_MODULE_DECLARE
namespace tilogspace
{
	enum ETiLogModuleEx : mod_ids_t
	{
		TILOG_MODULE_0 = 0,
		TILOG_MODULE_1 = 1,
		TILOG_MODULE_2 = 2,
	};

#define TILOG_GET_DEFAULT_MODULE_REF tilogspace::TiLog::GetMoudleBaseRef(tilogspace::TILOG_MODULE_0)
#define TILOG_GET_DEFAULT_MODULE_ENUM tilogspace::TILOG_MODULE_0


	constexpr static TiLogModuleSpec TILOG_ACTIVE_MODULE_SPECS[] = {
		{ TILOG_MODULE_0, "mod0", "a:/mod0/", PRINTER_TILOG_FILE, VERBOSE, false },
		{ TILOG_MODULE_1, "mod1", "a:/mod1/", PRINTER_TILOG_FILE, VERBOSE, false },
		{ TILOG_MODULE_2, "mod2", "a:/mod2/", PRINTER_TILOG_FILE, VERBOSE, false },
	};
	constexpr static size_t TILOG_MODULE_SPECS_SIZE = sizeof(TILOG_ACTIVE_MODULE_SPECS) / sizeof(TILOG_ACTIVE_MODULE_SPECS[0]);
}	 // namespace tilogspace
#endif





//#endif	  // TILOG_TILOGOVERRIDE_H
