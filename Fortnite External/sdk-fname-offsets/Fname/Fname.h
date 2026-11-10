#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

#include "../../Driver/communication.h"
#include "../Offsets/offsets.hpp"

class fname_p
{
public:
	static inline auto get_index(int32_t index) -> int32_t
	{
		if (!index)
			return 0;

		uint32_t result = 0x3A7D8347u * static_cast<uint32_t>(index) - 0x5593B7F2u;
		if (result == 0)
			result = 0xAA6C480Eu;

		return static_cast<int32_t>(result);
	}

	static auto get_fname_raw(int32_t raw_index) -> std::string
	{
		return get_fname_internal(raw_index);
	}

	static auto get_fname(int32_t index) -> std::string
	{
		int32_t decrypted_index = get_index(index);
		return get_fname_internal(decrypted_index);
	}

private:
	static auto get_fname_internal(int32_t decrypted_index) -> std::string
	{
		if (decrypted_index <= 0)
			return "";

		auto compute_key = [](uint32_t length) -> uint32_t
		{
			uint32_t T = 0x604FC9u + 8636u * length;
			return length + (T ^ (T >> 15));
		};

		auto decrypt_bytes = [&](const uint8_t* enc, uint8_t* out, uint32_t count, uint32_t length)
		{
			uint32_t v7 = compute_key(length);
			uint8_t v5 = static_cast<uint8_t>(static_cast<uint32_t>(-71) * v7);
			for (uint32_t i = 0; i < count; i++)
			{
				uint8_t eb = enc[i];
				uint8_t db = static_cast<uint8_t>(107u * static_cast<uint32_t>(static_cast<uint8_t>((v5 ^ eb) - v7)));
				out[i] = db;
				v5 = eb;
				uint32_t T = 0x604FC9u + 8636u * v7;
				v7 += T ^ (T >> 15);
			}
		};

		auto decode_entry = [&](uintptr_t entry) -> std::string
		{
			uint16_t header = read<uint16_t>(entry);
			if (header == 0)
				return "";

			uint32_t raw_length = (header >> 5) & 0x3FF;
			if (raw_length == 811)
				return "";

			uint32_t length = raw_length ^ 0x32B;
			bool is_wide = (header & 0x8000) != 0;

			if (length == 0 || length > 1024)
				return "";

			uint32_t byte_count = is_wide ? length * 2 : length;
			std::vector<uint8_t> buffer(byte_count);
			std::vector<uint8_t> dec(byte_count);
			read_buffer(entry + 2, buffer.data(), byte_count);
			decrypt_bytes(buffer.data(), dec.data(), byte_count, length);

			if (!is_wide)
			{
				std::string result(dec.begin(), dec.end());
				for (char& c : result)
				{
					if (c < 32 || c > 126)
						c = '_';
				}
				return result;
			}

			std::string result(length, '\0');
			for (uint32_t i = 0; i < length; i++)
			{
				uint16_t wc = dec[i * 2] | static_cast<uint16_t>(dec[i * 2 + 1] << 8);
				result[i] = (wc >= 32 && wc < 128) ? static_cast<char>(wc) : '_';
			}
			return result;
		};

		uint32_t chunk_index = static_cast<uint32_t>(decrypted_index) >> 16;
		uint16_t entry_index = static_cast<uint16_t>(decrypted_index);

		uintptr_t chunk_ptr = read<uintptr_t>(
			base_address + Offsets::GNames + (chunk_index + 2) * 8
		);
		if (!chunk_ptr)
			return "";

		uintptr_t entry = chunk_ptr + (2 * entry_index);
		uint16_t header = read<uint16_t>(entry);
		if (header == 0)
			return "";

		uint32_t raw_length = (header >> 5) & 0x3FF;

		if (raw_length == 811)
		{
			int32_t redirect_enc = read<int32_t>(entry + 6);
			int32_t r_decrypted = get_index(redirect_enc);
			if (r_decrypted <= 0)
				return "";

			uint32_t r_chunk_idx = static_cast<uint32_t>(r_decrypted) >> 16;
			uint16_t r_entry_idx = static_cast<uint16_t>(r_decrypted);

			uintptr_t r_chunk_ptr = read<uintptr_t>(
				base_address + Offsets::GNames + (r_chunk_idx + 2) * 8
			);
			if (!r_chunk_ptr)
				return "";

			uintptr_t r_entry = r_chunk_ptr + (2 * r_entry_idx);
			std::string base_name = decode_entry(r_entry);
			if (base_name.empty())
				return "";

			int32_t number_enc = read<int32_t>(entry + 2);
			if (number_enc == 0)
				return base_name;

			uint32_t prod = 0x3A7D8347u * static_cast<uint32_t>(number_enc);
			uint32_t v23 = prod + 0x42EBD788u;
			if (prod == 0xBD142877u)
				v23 = 0x42EBD788u;
			int32_t number = static_cast<int32_t>(v23);

			return base_name + "_" + std::to_string(number);
		}

		return decode_entry(entry);
	}

public:
	static auto decode_entry_from_buffer(const uint8_t* buf, uint32_t buf_size, uint32_t offset, uint32_t& out_entry_size) -> std::string
	{
		out_entry_size = 2;
		if (offset + 2 > buf_size)
			return "";

		uint16_t header = *(uint16_t*)(buf + offset);
		if (header == 0)
			return "";

		uint32_t raw_length = (header >> 5) & 0x3FF;
		if (raw_length == 811)
		{
			out_entry_size = 10;
			return "";
		}

		uint32_t length = raw_length ^ 0x32B;
		bool is_wide = (header & 0x8000) != 0;

		if (length == 0 || length > 1024)
			return "";

		uint32_t byte_count = is_wide ? length * 2 : length;
		out_entry_size = 2 + byte_count;

		if (offset + 2 + byte_count > buf_size)
			return "";

		auto compute_key = [](uint32_t len) -> uint32_t
		{
			uint32_t T = 0x604FC9u + 8636u * len;
			return len + (T ^ (T >> 15));
		};

		uint32_t v7 = compute_key(length);
		uint8_t v5 = static_cast<uint8_t>(static_cast<uint32_t>(-71) * v7);

		const uint8_t* enc = buf + offset + 2;
		std::string result;
		result.reserve(length);

		if (!is_wide)
		{
			for (uint32_t i = 0; i < byte_count; i++)
			{
				uint8_t eb = enc[i];
				uint8_t db = static_cast<uint8_t>(107u * static_cast<uint32_t>(static_cast<uint8_t>((v5 ^ eb) - v7)));
				char c = (db >= 32 && db <= 126) ? static_cast<char>(db) : '_';
				result += c;
				v5 = eb;
				uint32_t T = 0x604FC9u + 8636u * v7;
				v7 += T ^ (T >> 15);
			}
		}
		else
		{
			for (uint32_t i = 0; i < length; i++)
			{
				uint8_t eb0 = enc[i * 2];
				uint8_t db0 = static_cast<uint8_t>(107u * static_cast<uint32_t>(static_cast<uint8_t>((v5 ^ eb0) - v7)));
				v5 = eb0;
				uint32_t T = 0x604FC9u + 8636u * v7;
				v7 += T ^ (T >> 15);

				uint8_t eb1 = enc[i * 2 + 1];
				uint8_t db1 = static_cast<uint8_t>(107u * static_cast<uint32_t>(static_cast<uint8_t>((v5 ^ eb1) - v7)));
				v5 = eb1;
				T = 0x604FC9u + 8636u * v7;
				v7 += T ^ (T >> 15);

				uint16_t wc = db0 | (static_cast<uint16_t>(db1) << 8);
				result += (wc >= 32 && wc < 128) ? static_cast<char>(wc) : '_';
			}
		}

		return result;
	}

	static inline constexpr uint32_t CHUNK_OFFSET = 2;
};

namespace FNameResolver
{
    inline std::unordered_map<int32_t, std::string>   g_strCache;
    inline std::unordered_map<uintptr_t, std::string> g_classNameCache;
    inline std::mutex                                 g_strMutex;
    inline std::mutex                                 g_classMutex;

    inline std::string GetString(int32_t index)
    {
        if (!index) return "";
        {
            std::lock_guard<std::mutex> lk(g_strMutex);
            auto it = g_strCache.find(index);
            if (it != g_strCache.end()) return it->second;
        }

        std::string result = fname_p::get_fname(index);
        if (result.empty()) return result;

        {
            std::lock_guard<std::mutex> lk(g_strMutex);
            g_strCache[index] = result;
        }
        return result;
    }

    inline std::string GetObjectName(uintptr_t obj)
    {
        if (!obj) return "";
        int32_t idx = read<int32_t>(obj + Offsets::NAME_PRIVATE);
        return GetString(idx);
    }

    inline std::string GetClassName(uintptr_t obj)
    {
        if (!obj) return "";
        uintptr_t cls = read<uintptr_t>(obj + 0x8);
        if (!cls) return "";

        {
            std::lock_guard<std::mutex> lk(g_classMutex);
            auto it = g_classNameCache.find(cls);
            if (it != g_classNameCache.end()) return it->second;
        }

        std::string name = GetObjectName(cls);

        {
            std::lock_guard<std::mutex> lk(g_classMutex);
            g_classNameCache[cls] = name;
        }
        return name;
    }
}
