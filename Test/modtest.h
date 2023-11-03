//#ifndef TILOG_TILOGOVERRIDE_H
//#define TILOG_TILOGOVERRIDE_H

#ifdef TILOG_MODULE_DECLARE
namespace tilogspace
{
	enum ETiLogModule : uint8_t
	{
		TILOG_MODULE_START = 0,
		//...// user defined mod id begin
		TILOG_MODULE_0 = 0,
		TILOG_MODULE_1 = 1,
		TILOG_MODULE_2 = 2,
		//...// user defined mod id end
		TILOG_MODULES
	};

#define TILOG_REGISTER_MODULES TILOG_INTERNAL_REGISTER_MODULES_MACRO(tilogspace::TiLogMod0, tilogspace::TiLogMod1, tilogspace::TiLogMod2)


#define TILOG_GET_DEFAULT_MODULE_REF tilogspace::TiLog::getRInstance().GetMoudleRef<tilogspace::TILOG_MODULE_START>()
#define TILOG_GET_DEFAULT_MODULE_ENUM tilogspace::TILOG_MODULE_START

	struct TiLogModuleSpec
	{
		ETiLogModule mod;
		const char moduleName[32];
		const char data[256];
		printer_ids_t defaultEnabledPrinters;
		ELevel STATIC_LOG_LEVEL; // set the static log level,dynamic log level will always <= static log level
	};

	constexpr static TiLogModuleSpec TILOG_ACTIVE_MODULE_SPECS[] = {
		{ TILOG_MODULE_0, "mod0", "a:/mod0/", PRINTER_TILOG_FILE,VERBOSE },
		{ TILOG_MODULE_1, "mod1", "a:/mod1/", PRINTER_TILOG_FILE,VERBOSE },
		{ TILOG_MODULE_2, "mod2", "a:/mod2/", PRINTER_TILOG_FILE,VERBOSE },
	};
	constexpr static size_t TILOG_MODULE_SPECS_SIZE = sizeof(TILOG_ACTIVE_MODULE_SPECS) / sizeof(TILOG_ACTIVE_MODULE_SPECS[0]);
}	 // namespace tilogspace
#endif





#ifdef TILOG_MODULE_IMPLEMENT
class TiLogMod0 : public TiLogModBase
{
public:
	inline TiLogMod0() { mod = ETiLogModule::TILOG_MODULE_0; }
};

class TiLogMod1 : public TiLogModBase
{
public:
	inline TiLogMod1() { mod = ETiLogModule::TILOG_MODULE_1; }
};

class TiLogMod2 : public TiLogModBase
{
public:
	inline TiLogMod2() { mod = ETiLogModule::TILOG_MODULE_2; }
};
#endif


//#endif	  // TILOG_TILOGOVERRIDE_H
