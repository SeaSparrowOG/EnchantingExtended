#include "Serialization.h"

#include "Data/CreatedObjectManager.h"
#include "Data/EnchantArtManager.h"

namespace Serialization
{
	void SaveCallback(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc->OpenRecord(CreatedObjectManager, Version)) {
			_loggerError("Failed to open record for CreatedObjectManager");
			return;
		}

		if (!Data::CreatedObjectManager::GetSingleton()->Save(a_intfc)) {
			_loggerError("Failed to save CreatedObjectManager data");
		}
	}

	void LoadCallback(SKSE::SerializationInterface* a_intfc)
	{
		std::uint32_t type;
		std::uint32_t version;
		std::uint32_t length;
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (version != Version) {
				_loggerError("Loaded data is incompatible with plugin version!");
			}

			switch (type) {
			case CreatedObjectManager:
				if (!Data::CreatedObjectManager::GetSingleton()->Load(a_intfc)) {
					_loggerError("Failed to load CreatedObjectManager data");
				}
				break;

			default:
				_loggerError("Unrecognized record type ({})!", DecodeTypeCode(type));
				break;
			}
		}
	}

	void RevertCallback([[maybe_unused]] SKSE::SerializationInterface* a_intfc)
	{
		Data::CreatedObjectManager::GetSingleton()->Revert();
		Data::EnchantArtManager::GetSingleton()->Revert();
	}

	std::string Serialization::DecodeTypeCode(std::uint32_t a_typeCode)
	{
		constexpr std::size_t SIZE = sizeof(std::uint32_t);

		std::string sig;
		sig.resize(SIZE);
		char* iter = reinterpret_cast<char*>(&a_typeCode);
		for (std::size_t i = 0, j = SIZE - 2; i < SIZE - 1; ++i, --j) {
			sig[j] = iter[i];
		}
		return sig;
	}
}
