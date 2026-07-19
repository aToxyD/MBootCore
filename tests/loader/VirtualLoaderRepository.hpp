#pragma once

#include "mbootcore/loader/LoaderFramework.hpp"
#include "mbootcore/loader/LoaderMetadata.hpp"
#include "mbootcore/loader/LoaderData.hpp"
#include "mbootcore/logging/NullLogger.hpp"

#include <memory>
#include <string>
#include <random>
#include <array>
#include <cstdint>

namespace mbootcore {
namespace test {

class VirtualLoaderRepository {
public:
    static std::shared_ptr<ILogger> makeLogger() {
        return std::make_shared<NullLogger>();
    }

    static std::unique_ptr<LoaderFramework> create(size_t count = 120) {
        auto framework = std::make_unique<LoaderFramework>(makeLogger());
        populate(*framework, count);
        return framework;
    }

    static void populate(LoaderFramework& framework, size_t count = 120) {
        std::mt19937 rng{42};

        const std::vector<std::string> vendors = {
            "Qualcomm", "MediaTek", "UNISOC", "Samsung", "Broadcom"
        };

        const std::vector<std::string> chipsets = {
            "SM8250", "SM8450", "SM8550", "SM8650", "MT6785", "MT6885",
            "MT6895", "SC9863", "T610", "T820", "Exynos2200", "Exynos2400"
        };

        const std::vector<std::string> protocols = {
            "Sahara", "Firehose", "MTK-BROM", "SPD-BootROM", "Odin"
        };

        const std::vector<std::string> storageTypes = {
            "eMMC", "UFS", "NAND", "NOR", "SPI"
        };

        const std::vector<std::string> memoryTypes = {
            "DDR3", "DDR4", "DDR5", "LPDDR4", "LPDDR5", "SRAM"
        };

        const std::vector<uint32_t> msmIds = {
            0x000E, 0x0010, 0x0012, 0x0015, 0x0017, 0x0021, 0x0022, 0x0025
        };

        for (size_t i = 0; i < count; ++i) {
            auto vendor = vendors[rng() % vendors.size()];
            auto chipset = chipsets[rng() % chipsets.size()];
            auto protocol = protocols[rng() % protocols.size()];
            auto storage = storageTypes[rng() % storageTypes.size()];
            auto memory = memoryTypes[rng() % memoryTypes.size()];
            auto msmId = msmIds[rng() % msmIds.size()];

            LoaderMetadata meta;
            meta.vendor = vendor;
            meta.protocol = protocol;
            meta.chipset = chipset;
            meta.msmId = msmId;
            meta.oemId = rng() % 256;
            meta.modelId = rng() % 256;
            meta.version = std::to_string(rng() % 5 + 1) + "." +
                           std::to_string(rng() % 10);
            meta.build = "r" + std::to_string(rng() % 10000);
            meta.securityVersion = rng() % 3;
            meta.storageType = storage;
            meta.supportedMemory.push_back(memory);
            meta.loaderSize = (rng() % 1024 + 256) * 1024;
            meta.isSigned = (rng() % 3) > 0;
            meta.isEncrypted = (rng() % 5) == 0;
            meta.signatureState = meta.isSigned ? "signed" : "unsigned";
            meta.capabilities = { "upload", "reset" };

            std::vector<uint8_t> pkhash(32);
            for (auto& b : pkhash) b = static_cast<uint8_t>(rng() % 256);
            meta.pkhash = pkhash;

            std::vector<uint8_t> hash(32);
            for (auto& b : hash) b = static_cast<uint8_t>(rng() % 256);
            meta.hash = hash;
            meta.hashAlgorithm = "SHA256";

            std::string name = vendor + "_" + chipset + "_" + protocol +
                               "_" + std::to_string(i);

            ByteBuffer data(meta.loaderSize);
            for (auto& b : data) b = static_cast<uint8_t>(rng() % 256);
            data[0] = 0x7F; data[1] = 'E'; data[2] = 'L'; data[3] = 'F';

            (void)framework.addLoader(name, std::move(data), std::move(meta));
        }
    }
};

} // namespace test
} // namespace mbootcore
