#include <mbootcore/transport/SerialEnumerator.hpp>

#ifdef _WIN32
#include "platform/Win32SerialEnumeration.hpp"
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#endif

namespace mbootcore {
namespace transport {

#ifdef _WIN32

std::vector<SerialPortEntry> SerialEnumerator::enumerate() {
    std::vector<SerialPortEntry> entries;
    auto ports = platform::enumerateSerialPorts();
    for (const auto& p : ports) {
        SerialPortEntry entry;
        entry.portName = p.portName;
        entry.description = p.description;
        entry.manufacturer = p.manufacturer;
        entry.serialNumber = p.serialNumber;
        entries.push_back(std::move(entry));
    }
    return entries;
}

std::vector<SerialPortEntry> SerialEnumerator::findByDescription(const std::string& desc) {
    std::vector<SerialPortEntry> entries;
    auto all = enumerate();
    for (const auto& e : all) {
        if (e.description.find(desc) != std::string::npos) {
            entries.push_back(e);
        }
    }
    return entries;
}

SerialPortEntry SerialEnumerator::findFirst() {
    auto ports = platform::enumerateSerialPorts();
    if (!ports.empty()) {
        const auto& p = ports.front();
        SerialPortEntry entry;
        entry.portName = p.portName;
        entry.description = p.description;
        entry.manufacturer = p.manufacturer;
        entry.serialNumber = p.serialNumber;
        return entry;
    }
    return {};
}

size_t SerialEnumerator::portCount() {
    return platform::enumerateSerialPorts().size();
}

#else // POSIX

namespace {

bool isSerialTty(const std::string& name) {
    if (name.find("ttyUSB") == 0) return true;
    if (name.find("ttyACM") == 0) return true;
    if (name.find("ttyS") == 0) return true;
    if (name.find("ttyAMA") == 0) return true;
    if (name.find("ttyO") == 0) return true;
    if (name.find("ttyMFD") == 0) return true;
    return false;
}

std::string readSysfsAttribute(const std::string& ttyName, const std::string& attr) {
    std::string path = "/sys/class/tty/" + ttyName + "/device/../" + attr;
    std::ifstream file(path);
    if (!file.is_open()) {
        path = "/sys/class/tty/" + ttyName + "/device/" + attr;
        file.open(path);
    }
    if (file.is_open()) {
        std::string value;
        std::getline(file, value);
        return value;
    }
    return {};
}

SerialPortEntry entryFromTty(const std::string& name) {
    SerialPortEntry entry;
    entry.portName = name;
    entry.description = readSysfsAttribute(name, "modalias");
    entry.manufacturer = readSysfsAttribute(name, "manufacturer");
    if (!entry.manufacturer.empty()) {
        entry.manufacturer.resize(entry.manufacturer.size() - 1);
    }
    entry.serialNumber = readSysfsAttribute(name, "serial");
    if (!entry.serialNumber.empty()) {
        entry.serialNumber.resize(entry.serialNumber.size() - 1);
    }
    entry.type = TransportType::Serial;
    return entry;
}

} // anonymous namespace

std::vector<SerialPortEntry> SerialEnumerator::enumerate() {
    std::vector<SerialPortEntry> entries;
    DIR* dir = opendir("/sys/class/tty");
    if (!dir) return entries;

    struct dirent* dent;
    while ((dent = readdir(dir)) != nullptr) {
        std::string name(dent->d_name);
        if (name == "." || name == "..") continue;
        if (!isSerialTty(name)) continue;

        struct stat st;
        std::string fullPath = "/sys/class/tty/" + name;
        if (stat(fullPath.c_str(), &st) != 0) continue;

        entries.push_back(entryFromTty(name));
    }
    closedir(dir);
    return entries;
}

std::vector<SerialPortEntry> SerialEnumerator::findByDescription(const std::string& desc) {
    std::vector<SerialPortEntry> entries;
    auto all = enumerate();
    for (const auto& e : all) {
        if (e.description.find(desc) != std::string::npos) {
            entries.push_back(e);
        }
    }
    return entries;
}

SerialPortEntry SerialEnumerator::findFirst() {
    auto all = enumerate();
    if (!all.empty()) return all.front();
    return {};
}

size_t SerialEnumerator::portCount() {
    return enumerate().size();
}

#endif

} // namespace transport
} // namespace mbootcore
