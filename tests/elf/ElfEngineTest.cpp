#include <catch2/catch_test_macros.hpp>

#include "mbootcore/elf/ElfParser.hpp"
#include "mbootcore/elf/ElfValidator.hpp"
#include "mbootcore/elf/MemoryImageBuilder.hpp"
#include "mbootcore/elf/VirtualProgrammer.hpp"
#include "mbootcore/elf/ElfTypes.hpp"
#include "mbootcore/loader/ElfInspector.hpp"

#include <cstring>
#include <limits>
#include <vector>

using namespace mbootcore;
using namespace mbootcore::elf;

namespace {

struct ElfBuilder {
    ByteBuffer data;

    ElfBuilder(bool is64 = true, bool le = true) {
        if (is64) {
            build64(le);
        } else {
            build32(le);
        }
    }

    void build64(bool le) {
        data.resize(64 + 56 + 64 + 64, 0);
        uint8_t* p = data.data();

        p[0] = 0x7F; p[1] = 'E'; p[2] = 'L'; p[3] = 'F';
        p[4] = 2;
        p[5] = le ? 1 : 2;
        p[6] = 1;

        write16(p + 16, 2, le);
        write16(p + 18, 0xB7, le);
        write32(p + 20, 1, le);
        write64(p + 24, 0x8000, le);
        write64(p + 32, 64, le);
        write64(p + 40, 64 + 56, le);
        write32(p + 48, 0, le);
        write16(p + 52, 64, le);
        write16(p + 54, 56, le);
        write16(p + 56, 1, le);
        write16(p + 58, 64, le);
        write16(p + 60, 1, le);
        write16(p + 62, 0, le);

        uint8_t* ph = p + 64;
        write32(ph + 0, 1, le);
        write32(ph + 4, 7, le);
        write64(ph + 8, 64 + 56 + 64, le);
        write64(ph + 16, 0x8000, le);
        write64(ph + 24, 0x8000, le);
        write64(ph + 32, 64, le);
        write64(ph + 40, 64, le);
        write64(ph + 48, 4096, le);

        uint8_t* sh = p + 64 + 56;
        write32(sh + 0, 0, le);
        write32(sh + 4, 0, le);
        (void)sh;

        uint8_t* seg = p + 64 + 56 + 64;
        std::memset(seg, 0x90, 64);
    }

    void build32(bool le) {
        data.resize(52 + 32 + 64, 0);
        uint8_t* p = data.data();

        p[0] = 0x7F; p[1] = 'E'; p[2] = 'L'; p[3] = 'F';
        p[4] = 1;
        p[5] = le ? 1 : 2;
        p[6] = 1;

        write16(p + 16, 2, le);
        write16(p + 18, 40, le);
        write32(p + 20, 1, le);
        write32(p + 24, 0x8000, le);
        write32(p + 28, 52, le);
        write32(p + 32, 52 + 32, le);
        write32(p + 36, 0, le);
        write16(p + 40, 52, le);
        write16(p + 42, 32, le);
        write16(p + 44, 1, le);
        write16(p + 46, 40, le);
        write16(p + 48, 1, le);
        write16(p + 50, 0, le);

        uint8_t* ph = p + 52;
        write32(ph + 0, 1, le);
        write32(ph + 4, 52 + 32, le);
        write32(ph + 8, 0x8000, le);
        write32(ph + 12, 0x8000, le);
        write32(ph + 16, 64, le);
        write32(ph + 20, 64, le);
        write32(ph + 24, 7, le);
        write32(ph + 28, 1, le);

        uint8_t* seg = p + 52 + 32;
        std::memset(seg, 0x90, 64);
    }

    static void write16(uint8_t* p, uint16_t v, bool le) {
        if (le) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; }
        else { p[0] = (v >> 8) & 0xFF; p[1] = v & 0xFF; }
    }

    static void write32(uint8_t* p, uint32_t v, bool le) {
        if (le) { p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF;
                  p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF; }
        else { p[0] = (v >> 24) & 0xFF; p[1] = (v >> 16) & 0xFF;
               p[2] = (v >> 8) & 0xFF; p[3] = v & 0xFF; }
    }

    static void write64(uint8_t* p, uint64_t v, bool le) {
        write32(p, static_cast<uint32_t>(v & 0xFFFFFFFF), le);
        write32(p + 4, static_cast<uint32_t>((v >> 32) & 0xFFFFFFFF), le);
    }
};

// ELF64 program header field offsets (byte offsets within a single ph entry).
// Derived from ElfParser::readProgramHeader64().
constexpr size_t kElf64PhOffset = 8;    // p_offset  (uint64)
constexpr size_t kElf64PhFilesz = 32;   // p_filesz  (uint64)

// ELF32 program header field offsets (byte offsets within a single ph entry).
// Derived from ElfParser::readProgramHeader32().
constexpr size_t kElf32PhOffset = 4;    // p_offset  (uint32)
constexpr size_t kElf32PhFilesz = 16;   // p_filesz  (uint32)

// Program header table start offsets (after ELF header).
constexpr size_t kElf64PhStart = 64;    // sizeof(Elf64_Ehdr)
constexpr size_t kElf32PhStart = 52;    // sizeof(Elf32_Ehdr)

}

TEST_CASE("ElfEngineTest", "[elf]") {
    ElfParser m_parser;

SECTION("test_parse_valid_64bit") {
    ElfBuilder builder(true, true);
    auto result = m_parser.parse(builder.data);
    REQUIRE(result.isOk());

    auto& elf = result.value();
    REQUIRE(elf.isValid());
    REQUIRE(elf.header.elfClass == ElfClass::Elf64);
    REQUIRE(elf.header.machine == ElfMachine::AARCH64);
    REQUIRE(elf.header.type == ElfType::Exec);
    REQUIRE(elf.header.entry == uint64_t{0x8000});
    REQUIRE(elf.programHeaders.size() == size_t{1});
}

SECTION("test_parse_valid_32bit") {
    ElfBuilder builder(false, true);
    auto result = m_parser.parse(builder.data);
    REQUIRE(result.isOk());

    auto& elf = result.value();
    REQUIRE(elf.isValid());
    REQUIRE(elf.header.elfClass == ElfClass::Elf32);
    REQUIRE(elf.header.machine == ElfMachine::ARM);
    REQUIRE(elf.header.entry == uint64_t{0x8000});
    REQUIRE(elf.programHeaders.size() == size_t{1});
}

SECTION("test_parse_invalid_magic") {
    ByteBuffer bad = {0x00, 0x01, 0x02, 0x03};
    auto result = m_parser.parse(bad);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidElf);
}

SECTION("test_parse_empty") {
    ByteBuffer empty;
    auto result = m_parser.parse(empty);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidElf);
}

SECTION("test_parse_truncated") {
    ElfBuilder builder;
    ByteBuffer truncated(builder.data.begin(), builder.data.begin() + 30);
    auto result = m_parser.parse(truncated);
    REQUIRE(result.isError());
}

SECTION("test_header_fields") {
    ElfBuilder builder(true, true);
    auto result = m_parser.parse(builder.data);
    REQUIRE(result.isOk());

    auto& elf = result.value();
    REQUIRE(elf.header.ehsize == uint16_t{64});
    REQUIRE(elf.header.phentsize == uint16_t{56});
    REQUIRE(elf.header.phnum == uint16_t{1});
    REQUIRE(elf.header.shentsize == uint16_t{64});
    REQUIRE(elf.header.shnum == uint16_t{1});
    REQUIRE(elf.header.phoff > 0);
}

SECTION("test_class_identification") {
    ElfBuilder b64(true, true);
    REQUIRE(m_parser.is64Bit(b64.data));
    REQUIRE(!m_parser.is32Bit(b64.data));
    REQUIRE(m_parser.isElf(b64.data));
    REQUIRE(m_parser.identifyClass(b64.data) == ElfClass::Elf64);

    ElfBuilder b32(false, true);
    REQUIRE(m_parser.is32Bit(b32.data));
    REQUIRE(!m_parser.is64Bit(b32.data));
    REQUIRE(m_parser.isElf(b32.data));
    REQUIRE(m_parser.identifyClass(b32.data) == ElfClass::Elf32);
}

SECTION("test_entry_point") {
    ElfBuilder builder(true, true);
    auto entry = m_parser.parse(builder.data);
    REQUIRE(entry.isOk());
    REQUIRE(entry.value().header.entry == uint64_t{0x8000});
}

SECTION("test_parse_program_headers") {
    ElfBuilder builder;
    auto result = m_parser.parse(builder.data);
    REQUIRE(result.isOk());

    auto& elf = result.value();
    REQUIRE(elf.programHeaders.size() == size_t{1});

    auto& ph = elf.programHeaders[0];
    REQUIRE(ph.type == SegmentType::Load);
    REQUIRE(ph.isLoadable());
    REQUIRE(ph.isReadable());
    REQUIRE(ph.isWritable());
    REQUIRE(ph.isExecutable());
    REQUIRE(ph.vaddr == uint64_t{0x8000});
}

SECTION("test_segment_loadable_check") {
    ProgramHeader loadable;
    loadable.type = SegmentType::Load;
    REQUIRE(loadable.isLoadable());

    ProgramHeader notLoadable;
    notLoadable.type = SegmentType::Null;
    REQUIRE(!notLoadable.isLoadable());

    ProgramHeader dynamic;
    dynamic.type = SegmentType::Dynamic;
    REQUIRE(!dynamic.isLoadable());
}

SECTION("test_segment_overlap_detection") {
    ElfBuilder builder;
    auto result = m_parser.parse(builder.data);
    REQUIRE(result.isOk());

    ElfFile elf = std::move(result.value());
    ProgramHeader overlapping;
    overlapping.type = SegmentType::Load;
    overlapping.vaddr = 0x7F00;
    overlapping.memsz = 0x200;
    elf.programHeaders.push_back(overlapping);

    ElfValidator validator;
    REQUIRE(!validator.checkOverlappingSegments(elf.programHeaders));
}

SECTION("test_parse_section_headers") {
    ElfBuilder builder;
    auto result = m_parser.parse(builder.data);
    REQUIRE(result.isOk());
    REQUIRE(result.value().sectionHeaders.size() == size_t{1});
}

SECTION("test_section_string_table") {
    ElfBuilder builder;

    builder.data.resize(builder.data.size() + 40 + 32, 0);
    uint8_t* p = builder.data.data();

    ElfParser::readHeader(builder.data);

    size_t shOff = (p[4] == 2) ? (64 + 56) : (52 + 32);
    uint8_t* strtabSh = p + shOff + 64;
    (void)strtabSh;

    auto result = m_parser.parse(builder.data);
    REQUIRE(result.isOk());
}

SECTION("reject_elf64_phentsize_too_small") {
    constexpr uint16_t kTooSmall = 4;
    ElfBuilder builder(true, true);
    ElfBuilder::write16(builder.data.data() + 54, kTooSmall, true);
    auto result = m_parser.parse(builder.data);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidElf);
}

SECTION("reject_elf32_phentsize_too_small") {
    constexpr uint16_t kTooSmall = 4;
    ElfBuilder builder(false, true);
    ElfBuilder::write16(builder.data.data() + 42, kTooSmall, true);
    auto result = m_parser.parse(builder.data);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidElf);
}

SECTION("reject_elf64_shentsize_too_small") {
    constexpr uint16_t kTooSmall = 4;
    ElfBuilder builder(true, true);
    ElfBuilder::write16(builder.data.data() + 58, kTooSmall, true);
    auto result = m_parser.parse(builder.data);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidElf);
}

SECTION("reject_elf32_shentsize_too_small") {
    constexpr uint16_t kTooSmall = 4;
    ElfBuilder builder(false, true);
    ElfBuilder::write16(builder.data.data() + 46, kTooSmall, true);
    auto result = m_parser.parse(builder.data);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidElf);
}

SECTION("test_validate_valid_elf") {
    ElfBuilder builder;
    auto parseResult = m_parser.parse(builder.data);
    REQUIRE(parseResult.isOk());

    ElfValidator validator;
    auto v = validator.validate(parseResult.value(), builder.data);
    REQUIRE((v.valid || v.errors.empty()));
}

SECTION("test_validate_invalid_magic") {
    ByteBuffer bad = {0, 0, 0, 0};
    ElfValidator validator;
    REQUIRE(!validator.checkMagic(bad));
}

SECTION("test_validate_truncated") {
    ByteBuffer truncated(10, 0);
    truncated[0] = 0x7F; truncated[1] = 'E'; truncated[2] = 'L'; truncated[3] = 'F';
    truncated[4] = 2; truncated[5] = 1; truncated[6] = 1;

    auto result = m_parser.parse(truncated);
    REQUIRE(result.isError());
}

SECTION("test_validate_overlapping_segments") {
    std::vector<ProgramHeader> phs;

    ProgramHeader a;
    a.type = SegmentType::Load;
    a.vaddr = 0x8000;
    a.memsz = 0x1000;

    ProgramHeader b;
    b.type = SegmentType::Load;
    b.vaddr = 0x8080;
    b.memsz = 0x100;

    phs.push_back(a);
    phs.push_back(b);

    ElfValidator validator;
    REQUIRE(!validator.checkOverlappingSegments(phs));
}

SECTION("test_validate_alignment_warning") {
    std::vector<ProgramHeader> phs;

    ProgramHeader ph;
    ph.type = SegmentType::Load;
    ph.vaddr = 0x8001;
    ph.offset = 0;
    ph.align = 4096;

    phs.push_back(ph);

    ElfValidator validator;
    REQUIRE(!validator.checkAlignment(phs));
}

SECTION("test_memory_builder_single_segment") {
    ElfBuilder builder;
    auto parseResult = m_parser.parse(builder.data);
    REQUIRE(parseResult.isOk());

    MemoryImageBuilder imageBuilder;
    auto memResult = imageBuilder.build(parseResult.value());
    REQUIRE(memResult.isOk());

    auto& mem = memResult.value();
    REQUIRE(!mem.isEmpty());
    REQUIRE(mem.lowestAddr == uint64_t{0x8000});
    REQUIRE(mem.entryPoint == uint64_t{0x8000});
    REQUIRE(!mem.image.empty());
    REQUIRE(mem.segments.size() == size_t{1});
}

SECTION("test_memory_builder_no_segments") {
    ElfBuilder builder;
    auto parseResult = m_parser.parse(builder.data);
    REQUIRE(parseResult.isOk());

    for (auto& ph : parseResult.value().programHeaders) {
        ph.type = SegmentType::Null;
    }

    MemoryImageBuilder imageBuilder;
    auto memResult = imageBuilder.build(parseResult.value());
    REQUIRE(memResult.isError());
}

SECTION("test_memory_builder_entry_point") {
    ElfBuilder builder;
    auto parseResult = m_parser.parse(builder.data);
    REQUIRE(parseResult.isOk());

    MemoryImageBuilder imageBuilder;
    auto memResult = imageBuilder.build(parseResult.value());
    REQUIRE(memResult.isOk());
    REQUIRE(memResult.value().entryPoint == uint64_t{0x8000});
}

SECTION("test_virtual_execute_success") {
    ElfBuilder builder;
    auto parseResult = m_parser.parse(builder.data);
    REQUIRE(parseResult.isOk());

    MemoryImageBuilder imageBuilder;
    auto memResult = imageBuilder.build(parseResult.value());
    REQUIRE(memResult.isOk());

    VirtualProgrammer prog;
    auto r = prog.execute(memResult.value());
    REQUIRE(r.isOk());
    REQUIRE(prog.wasExecuted());
    REQUIRE(prog.state() == ProgrammerState::Completed);
}

SECTION("test_virtual_execute_fail_on_load") {
    VirtualProgrammer prog;
    prog.setFailOnLoad(true);

    MemoryImage emptyImage;
    auto r = prog.execute(emptyImage);
    REQUIRE(r.isError());
    REQUIRE(prog.state() == ProgrammerState::Failed);
}

SECTION("test_virtual_execute_fail_on_verify") {
    auto builder = ElfBuilder();
    auto parseResult = m_parser.parse(builder.data);
    REQUIRE(parseResult.isOk());

    MemoryImageBuilder imageBuilder;
    auto memResult = imageBuilder.build(parseResult.value());
    REQUIRE(memResult.isOk());

    VirtualProgrammer prog;
    prog.setFailOnVerify(true);
    auto r = prog.execute(memResult.value());
    REQUIRE(r.isError());
}

SECTION("test_virtual_execute_fail_on_transfer") {
    ElfBuilder builder;
    auto parseResult = m_parser.parse(builder.data);
    REQUIRE(parseResult.isOk());

    MemoryImageBuilder imageBuilder;
    auto memResult = imageBuilder.build(parseResult.value());
    REQUIRE(memResult.isOk());

    VirtualProgrammer prog;
    prog.setFailOnTransfer(true);
    auto r = prog.execute(memResult.value());
    REQUIRE(r.isError());
}

SECTION("test_virtual_execute_cancel") {
    VirtualProgrammer prog;
    prog.cancel();
    REQUIRE(prog.state() == ProgrammerState::Cancelled);
}

SECTION("test_virtual_execute_timeout") {
    ElfBuilder builder;
    auto parseResult = m_parser.parse(builder.data);
    REQUIRE(parseResult.isOk());

    MemoryImageBuilder imageBuilder;
    auto memResult = imageBuilder.build(parseResult.value());
    REQUIRE(memResult.isOk());

    VirtualProgrammer prog;
    prog.simulateTimeout(true);
    auto r = prog.execute(memResult.value());
    REQUIRE(r.isError());
    REQUIRE(r.error() == ErrorCode::TransportTimeout);
}

SECTION("test_virtual_state_transitions") {
    VirtualProgrammer prog;
    REQUIRE(prog.state() == ProgrammerState::Idle);

    ElfBuilder builder;
    auto parseResult = m_parser.parse(builder.data);
    REQUIRE(parseResult.isOk());

    MemoryImageBuilder imageBuilder;
    auto memResult = imageBuilder.build(parseResult.value());
    REQUIRE(memResult.isOk());

    REQUIRE(prog.loadImage(memResult.value()).isOk());
    REQUIRE(prog.state() == ProgrammerState::ImageLoaded);

    REQUIRE(prog.verifyImage().isOk());
    REQUIRE(prog.state() == ProgrammerState::ImageVerified);

    REQUIRE(prog.prepareExecution().isOk());
    REQUIRE(prog.state() == ProgrammerState::TransferReady);
}

SECTION("test_virtual_progress_callback") {
    int callbackCount = 0;
    VirtualProgrammer prog;

    prog.setProgressCallback([&](const ProgrammerProgress& p) {
        callbackCount++;
        (void)p;
    });

    ElfBuilder builder;
    auto parseResult = m_parser.parse(builder.data);
    REQUIRE(parseResult.isOk());

    MemoryImageBuilder imageBuilder;
    auto memResult = imageBuilder.build(parseResult.value());
    REQUIRE(memResult.isOk());

    (void)prog.execute(memResult.value());
    REQUIRE(callbackCount > 0);
}

SECTION("reject_overflow64_offset_plus_filesz") {
    ElfBuilder builder(true, true);
    // Craft overflow: p_offset + p_filesz wraps to 0 (passes naive check)
    ElfBuilder::write64(builder.data.data() + kElf64PhStart + kElf64PhOffset,
                        std::numeric_limits<uint64_t>::max(), true);
    ElfBuilder::write64(builder.data.data() + kElf64PhStart + kElf64PhFilesz,
                        1, true);
    auto parseResult = m_parser.parse(builder.data);
    REQUIRE(parseResult.isOk());
    REQUIRE_FALSE(parseResult.value().isValid());

    MemoryImageBuilder imageBuilder;
    auto memResult = imageBuilder.build(parseResult.value());
    REQUIRE(memResult.isError());
    REQUIRE(memResult.error() == ErrorCode::InvalidElf);
}

SECTION("reject_overflow32_offset_plus_filesz") {
    ElfBuilder builder(false, true);
    // Craft overflow: p_offset + p_filesz wraps to 0 (passes naive check)
    ElfBuilder::write32(builder.data.data() + kElf32PhStart + kElf32PhOffset,
                        std::numeric_limits<uint32_t>::max(), true);
    ElfBuilder::write32(builder.data.data() + kElf32PhStart + kElf32PhFilesz,
                        1, true);
    auto parseResult = m_parser.parse(builder.data);
    REQUIRE(parseResult.isOk());
    REQUIRE_FALSE(parseResult.value().isValid());

    MemoryImageBuilder imageBuilder;
    auto memResult = imageBuilder.build(parseResult.value());
    REQUIRE(memResult.isError());
    REQUIRE(memResult.error() == ErrorCode::InvalidElf);
}

SECTION("overflow_offset_plus_filesz_builder_defense") {
    // Purpose: Verify MemoryImageBuilder's independent overflow guard.
    //
    // We intentionally bypass validateProgramHeaders() by overriding
    // validation.valid after parse(). This simulates an ElfFile
    // constructed programmatically (outside ElfParser::parse) where
    // validation may not have been run. The builder must still not
    // invoke undefined behavior from the overflow.

    ElfBuilder builder(true, true);
    auto parseResult = m_parser.parse(builder.data);
    REQUIRE(parseResult.isOk());

    // Start from a well-formed ElfFile, then inject overflow values
    ElfFile elf = std::move(parseResult.value());
    elf.validation.valid = true;
    elf.programHeaders[0].offset = std::numeric_limits<uint64_t>::max();
    elf.programHeaders[0].filesz = 1;

    MemoryImageBuilder imageBuilder;
    auto memResult = imageBuilder.build(elf);
    // Builder must not crash. Segment data is skipped (offset out of
    // bounds), producing a zero-filled segment like BSS.
    REQUIRE(memResult.isOk());
    REQUIRE(memResult.value().segments.size() == 1);
    REQUIRE(memResult.value().segments[0].data.size() == 64);
}

SECTION("test_machine_to_string") {
    REQUIRE(std::strcmp(machineToString(ElfMachine::ARM), "ARM") == 0);
    REQUIRE(std::strcmp(machineToString(ElfMachine::AARCH64), "AArch64") == 0);
    REQUIRE(std::strcmp(machineToString(ElfMachine::x86_64), "x86-64") == 0);
    REQUIRE(std::strcmp(machineToString(ElfMachine::RISC_V), "RISC-V") == 0);
    REQUIRE(std::strcmp(machineToString(ElfMachine::None), "None") == 0);
}

SECTION("test_segment_type_to_string") {
    REQUIRE(std::strcmp(segmentTypeToString(SegmentType::Load), "PT_LOAD") == 0);
    REQUIRE(std::strcmp(segmentTypeToString(SegmentType::Null), "PT_NULL") == 0);
    REQUIRE(std::strcmp(segmentTypeToString(SegmentType::Dynamic), "PT_DYNAMIC") == 0);
}

SECTION("test_segment_flags") {
    SegmentFlag rwx = SegmentFlag::RWX;
    REQUIRE(hasFlag(rwx, SegmentFlag::R));
    REQUIRE(hasFlag(rwx, SegmentFlag::W));
    REQUIRE(hasFlag(rwx, SegmentFlag::X));

    SegmentFlag combined = SegmentFlag::R | SegmentFlag::W;
    REQUIRE(hasFlag(combined, SegmentFlag::R));
    REQUIRE(hasFlag(combined, SegmentFlag::W));
    REQUIRE(!hasFlag(combined, SegmentFlag::X));
}

SECTION("inspect_reject_truncated_elf4bytes") {
    ElfInspector inspector;
    ByteBuffer data{0x7F, 'E', 'L', 'F'};
    auto result = inspector.inspect(data);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidElf);
}

SECTION("inspect_reject_truncated_elf5bytes") {
    ElfInspector inspector;
    ByteBuffer data{0x7F, 'E', 'L', 'F', 2};
    auto result = inspector.inspect(data);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidElf);
}

SECTION("inspect_reject_truncated_elf6bytes") {
    ElfInspector inspector;
    ByteBuffer data{0x7F, 'E', 'L', 'F', 2, 1};
    auto result = inspector.inspect(data);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidElf);
}

SECTION("inspect_reject_no_magic") {
    ElfInspector inspector;
    ByteBuffer empty;
    auto result = inspector.inspect(empty);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidElf);
}

SECTION("inspect_reject_invalid_magic") {
    ElfInspector inspector;
    ByteBuffer data{0x00, 'E', 'L', 'F', 2, 1, 1};
    auto result = inspector.inspect(data);
    REQUIRE(result.isError());
    REQUIRE(result.error() == ErrorCode::InvalidElf);
}

SECTION("is32Bit_valid") {
    ElfInspector inspector;
    ElfBuilder builder(false);
    REQUIRE(inspector.is32Bit(builder.data));
    REQUIRE_FALSE(inspector.is64Bit(builder.data));
}

SECTION("is64Bit_valid") {
    ElfInspector inspector;
    ElfBuilder builder(true);
    REQUIRE(inspector.is64Bit(builder.data));
    REQUIRE_FALSE(inspector.is32Bit(builder.data));
}

SECTION("is32Bit_rejects_64bit") {
    ElfInspector inspector;
    ElfBuilder builder(true);
    REQUIRE_FALSE(inspector.is32Bit(builder.data));
}

SECTION("is64Bit_rejects_32bit") {
    ElfInspector inspector;
    ElfBuilder builder(false);
    REQUIRE_FALSE(inspector.is64Bit(builder.data));
}

SECTION("entryPoint_64bit") {
    ElfInspector inspector;
    ElfBuilder builder(true);
    REQUIRE(inspector.entryPoint(builder.data) == 0x8000);
}

SECTION("entryPoint_32bit") {
    ElfInspector inspector;
    ElfBuilder builder(false);
    REQUIRE(inspector.entryPoint(builder.data) == 0x8000);
}

SECTION("entryPoint_returns_zero_for_invalid") {
    ElfInspector inspector;
    ByteBuffer empty;
    REQUIRE(inspector.entryPoint(empty) == 0);

    ByteBuffer shortData{0x7F, 'E', 'L', 'F', 2, 1, 1};
    REQUIRE(inspector.entryPoint(shortData) == 0);

    ByteBuffer thirty2bitTooShort{0x7F, 'E', 'L', 'F', 1, 1, 1};
    REQUIRE(inspector.entryPoint(thirty2bitTooShort) == 0);
}

SECTION("architecture_known_machines") {
    ElfInspector inspector;
    ByteBuffer data(64, 0);
    data[0] = 0x7F; data[1] = 'E'; data[2] = 'L'; data[3] = 'F';
    data[4] = 2; data[5] = 1; data[6] = 1;

    data[18] = 0xB7; data[19] = 0x00;
    REQUIRE(inspector.architecture(data) == "AArch64");

    data[18] = 0x28; data[19] = 0x00;
    REQUIRE(inspector.architecture(data) == "ARM");

    data[18] = 0x3E; data[19] = 0x00;
    REQUIRE(inspector.architecture(data) == "x86-64");

    data[18] = 0x03; data[19] = 0x00;
    REQUIRE(inspector.architecture(data) == "x86");

    data[18] = 0xF3; data[19] = 0x00;
    REQUIRE(inspector.architecture(data) == "RISC-V");

    data[18] = 0x08; data[19] = 0x00;
    REQUIRE(inspector.architecture(data) == "MIPS");

    data[18] = 0x14; data[19] = 0x00;
    REQUIRE(inspector.architecture(data) == "PowerPC");
}

SECTION("architecture_unknown_and_no_magic") {
    ElfInspector inspector;
    ByteBuffer data(24, 0);
    data[0] = 0x7F; data[1] = 'E'; data[2] = 'L'; data[3] = 'F';
    data[4] = 2; data[5] = 1; data[6] = 1;
    data[18] = 0xFF; data[19] = 0xFF;
    REQUIRE(inspector.architecture(data) == "Unknown (0xffff)");

    ByteBuffer noMagic(24, 0);
    REQUIRE(inspector.architecture(noMagic) == "Unknown");
}

SECTION("inspect_valid_64bit") {
    ElfInspector inspector;
    ByteBuffer data(64, 0);
    data[0] = 0x7F; data[1] = 'E'; data[2] = 'L'; data[3] = 'F';
    data[4] = 2; data[5] = 1; data[6] = 1;

    ElfBuilder::write16(data.data() + 16, 2, true);
    ElfBuilder::write16(data.data() + 18, 0xB7, true);
    ElfBuilder::write64(data.data() + 24, 0x1000, true);
    ElfBuilder::write64(data.data() + 32, 64, true);
    ElfBuilder::write64(data.data() + 40, 120, true);
    ElfBuilder::write32(data.data() + 48, 0x1234, true);
    ElfBuilder::write16(data.data() + 52, 64, true);
    ElfBuilder::write16(data.data() + 54, 56, true);
    ElfBuilder::write16(data.data() + 56, 2, true);
    ElfBuilder::write16(data.data() + 58, 64, true);
    ElfBuilder::write16(data.data() + 60, 3, true);
    ElfBuilder::write16(data.data() + 62, 1, true);

    auto result = inspector.inspect(data);
    REQUIRE(result.isOk());
    auto info = result.value();
    REQUIRE(info.header.is64Bit);
    REQUIRE(info.header.isLittleEndian);
    REQUIRE(info.header.version == 1);
    REQUIRE(info.header.type == 2);
    REQUIRE(info.header.machine == 0xB7);
    REQUIRE(info.header.entry == 0x1000);
    REQUIRE(info.header.phoff == 64);
    REQUIRE(info.header.shoff == 120);
    REQUIRE(info.header.flags == 0x1234);
    REQUIRE(info.header.ehsize == 64);
    REQUIRE(info.header.phentsize == 56);
    REQUIRE(info.header.phnum == 2);
    REQUIRE(info.header.shentsize == 64);
    REQUIRE(info.header.shnum == 3);
    REQUIRE(info.header.shstrndx == 1);
    REQUIRE(info.architecture == "AArch64");
    REQUIRE(info.hasProgramHeaders);
    REQUIRE(info.hasSectionHeaders);
}

SECTION("inspect_valid_32bit") {
    ElfInspector inspector;
    ByteBuffer data(52, 0);
    data[0] = 0x7F; data[1] = 'E'; data[2] = 'L'; data[3] = 'F';
    data[4] = 1; data[5] = 1; data[6] = 1;

    ElfBuilder::write16(data.data() + 16, 3, true);
    ElfBuilder::write16(data.data() + 18, 0x28, true);
    ElfBuilder::write32(data.data() + 24, 0x2000, true);
    ElfBuilder::write32(data.data() + 28, 52, true);
    ElfBuilder::write32(data.data() + 32, 100, true);
    ElfBuilder::write32(data.data() + 36, 0xABCD, true);
    ElfBuilder::write16(data.data() + 40, 52, true);
    ElfBuilder::write16(data.data() + 42, 32, true);
    ElfBuilder::write16(data.data() + 44, 1, true);
    ElfBuilder::write16(data.data() + 46, 40, true);
    ElfBuilder::write16(data.data() + 48, 1, true);
    ElfBuilder::write16(data.data() + 50, 0, true);

    auto result = inspector.inspect(data);
    REQUIRE(result.isOk());
    auto info = result.value();
    REQUIRE_FALSE(info.header.is64Bit);
    REQUIRE(info.header.isLittleEndian);
    REQUIRE(info.header.version == 1);
    REQUIRE(info.header.type == 3);
    REQUIRE(info.header.machine == 0x28);
    REQUIRE(info.header.entry == 0x2000);
    REQUIRE(info.header.phoff == 52);
    REQUIRE(info.header.shoff == 100);
    REQUIRE(info.header.flags == 0xABCD);
    REQUIRE(info.header.ehsize == 52);
    REQUIRE(info.header.phentsize == 32);
    REQUIRE(info.header.phnum == 1);
    REQUIRE(info.header.shentsize == 40);
    REQUIRE(info.header.shnum == 1);
    REQUIRE(info.header.shstrndx == 0);
    REQUIRE(info.architecture == "ARM");
    REQUIRE(info.hasProgramHeaders);
    REQUIRE(info.hasSectionHeaders);
}

SECTION("inspect_hasHeaders_flags") {
    ElfInspector inspector;
    ByteBuffer data(64, 0);
    data[0] = 0x7F; data[1] = 'E'; data[2] = 'L'; data[3] = 'F';
    data[4] = 2; data[5] = 1; data[6] = 1;

    ElfBuilder::write16(data.data() + 54, 56, true);
    ElfBuilder::write16(data.data() + 56, 0, true);
    ElfBuilder::write16(data.data() + 58, 64, true);
    ElfBuilder::write16(data.data() + 60, 0, true);
    auto result1 = inspector.inspect(data);
    REQUIRE(result1.isOk());
    REQUIRE_FALSE(result1.value().hasProgramHeaders);
    REQUIRE_FALSE(result1.value().hasSectionHeaders);

    ElfBuilder::write16(data.data() + 54, 56, true);
    ElfBuilder::write16(data.data() + 56, 1, true);
    ElfBuilder::write16(data.data() + 58, 0, true);
    ElfBuilder::write16(data.data() + 60, 0, true);
    auto result2 = inspector.inspect(data);
    REQUIRE(result2.isOk());
    REQUIRE(result2.value().hasProgramHeaders);
    REQUIRE_FALSE(result2.value().hasSectionHeaders);

    ElfBuilder::write16(data.data() + 54, 0, true);
    ElfBuilder::write16(data.data() + 56, 0, true);
    ElfBuilder::write16(data.data() + 58, 64, true);
    ElfBuilder::write16(data.data() + 60, 1, true);
    auto result3 = inspector.inspect(data);
    REQUIRE(result3.isOk());
    REQUIRE_FALSE(result3.value().hasProgramHeaders);
    REQUIRE(result3.value().hasSectionHeaders);
}

SECTION("computeHash_empty") {
    ElfInspector inspector;
    ByteBuffer empty;
    auto hash = inspector.computeHash(empty);
    REQUIRE(hash.empty());
}

SECTION("computeHash_deterministic") {
    ElfInspector inspector;
    ByteBuffer data{0x7F, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 2, 0, 0xB7, 0, 1, 0, 0, 0};
    auto h1 = inspector.computeHash(data);
    auto h2 = inspector.computeHash(data);
    REQUIRE(h1 == h2);
    REQUIRE(h1.size() == 32);
}

SECTION("computeHash_differs_for_different_input") {
    ElfInspector inspector;
    ByteBuffer data1(32, 0xAA);
    ByteBuffer data2(32, 0xBB);
    auto h1 = inspector.computeHash(data1);
    auto h2 = inspector.computeHash(data2);
    REQUIRE(h1.size() == 32);
    REQUIRE(h2.size() == 32);
    REQUIRE(h1 != h2);
}

}
