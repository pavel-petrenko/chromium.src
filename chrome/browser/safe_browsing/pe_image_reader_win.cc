// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/pe_image_reader_win.h"

#include "base/logging.h"

namespace safe_browsing {

// A class template of traits pertaining to IMAGE_OPTIONAL_HEADER{32,64}.
template<class HEADER_TYPE>
struct OptionalHeaderTraits {
};

template<>
struct OptionalHeaderTraits<IMAGE_OPTIONAL_HEADER32> {
  static const PeImageReader::WordSize word_size = PeImageReader::WORD_SIZE_32;
};

template<>
struct OptionalHeaderTraits<IMAGE_OPTIONAL_HEADER64> {
  static const PeImageReader::WordSize word_size = PeImageReader::WORD_SIZE_64;
};

// A template for type-specific optional header implementations. This, in
// conjunction with the OptionalHeader interface, effectively erases the
// underlying structure type from the point of view of the PeImageReader.
template<class OPTIONAL_HEADER_TYPE>
class PeImageReader::OptionalHeaderImpl : public PeImageReader::OptionalHeader {
 public:
  typedef OptionalHeaderTraits<OPTIONAL_HEADER_TYPE> TraitsType;

  explicit OptionalHeaderImpl(const uint8_t* optional_header_start)
      : optional_header_(reinterpret_cast<const OPTIONAL_HEADER_TYPE*>(
                             optional_header_start)) {}

  virtual WordSize GetWordSize() OVERRIDE {
    return TraitsType::word_size;
  }

  virtual size_t GetDataDirectoryOffset() OVERRIDE {
    return offsetof(OPTIONAL_HEADER_TYPE, DataDirectory);
  }

  virtual DWORD GetDataDirectorySize() OVERRIDE {
    return optional_header_->NumberOfRvaAndSizes;
  }

  virtual const IMAGE_DATA_DIRECTORY* GetDataDirectoryEntries() OVERRIDE {
    return &optional_header_->DataDirectory[0];
  }

 private:
  const OPTIONAL_HEADER_TYPE* optional_header_;
  DISALLOW_COPY_AND_ASSIGN(OptionalHeaderImpl);
};

PeImageReader::PeImageReader()
    : image_data_(),
      image_size_(),
      validation_state_() {}

PeImageReader::~PeImageReader() {
  Clear();
}

bool PeImageReader::Initialize(const uint8_t* image_data, size_t image_size) {
  image_data_ = image_data;
  image_size_ = image_size;

  if (!ValidateDosHeader() ||
      !ValidatePeSignature() ||
      !ValidateCoffFileHeader() ||
      !ValidateOptionalHeader() ||
      !ValidateSectionHeaders()) {
    Clear();
    return false;
  }

  return true;
}

PeImageReader::WordSize PeImageReader::GetWordSize() {
  return optional_header_->GetWordSize();
}

const IMAGE_DOS_HEADER* PeImageReader::GetDosHeader() {
  DCHECK_NE((validation_state_ & VALID_DOS_HEADER), 0U);
  return reinterpret_cast<const IMAGE_DOS_HEADER*>(image_data_);
}

const IMAGE_FILE_HEADER* PeImageReader::GetCoffFileHeader() {
  DCHECK_NE((validation_state_ & VALID_COFF_FILE_HEADER), 0U);
  return reinterpret_cast<const IMAGE_FILE_HEADER*>(
      image_data_ + GetDosHeader()->e_lfanew + sizeof(DWORD));
}

const uint8_t* PeImageReader::GetOptionalHeaderData(
    size_t* optional_header_size) {
  *optional_header_size = GetOptionalHeaderSize();
  return GetOptionalHeaderStart();
}

size_t PeImageReader::GetNumberOfSections() {
  return GetCoffFileHeader()->NumberOfSections;
}

const IMAGE_SECTION_HEADER* PeImageReader::GetSectionHeaderAt(size_t index) {
  DCHECK_NE((validation_state_ & VALID_SECTION_HEADERS), 0U);
  DCHECK_LT(index, GetNumberOfSections());
  return reinterpret_cast<const IMAGE_SECTION_HEADER*>(
      GetOptionalHeaderStart() +
      GetOptionalHeaderSize() +
      (sizeof(IMAGE_SECTION_HEADER) * index));
}

const uint8_t* PeImageReader::GetExportSection(size_t* section_size) {
  size_t data_size = 0;
  const uint8_t* data = GetImageData(IMAGE_DIRECTORY_ENTRY_EXPORT, &data_size);

  // The export section data must be big enough for the export directory.
  if (!data || data_size < sizeof(IMAGE_EXPORT_DIRECTORY))
    return NULL;

  *section_size = data_size;
  return data;
}

size_t PeImageReader::GetNumberOfDebugEntries() {
  size_t data_size = 0;
  const uint8_t* data = GetImageData(IMAGE_DIRECTORY_ENTRY_DEBUG, &data_size);
  return data ? (data_size / sizeof(IMAGE_DEBUG_DIRECTORY)) : 0;
}

const IMAGE_DEBUG_DIRECTORY* PeImageReader::GetDebugEntry(
    size_t index,
    const uint8_t** raw_data,
    size_t* raw_data_size) {
  DCHECK_LT(index, GetNumberOfDebugEntries());

  // Get the debug directory.
  size_t debug_directory_size = 0;
  const IMAGE_DEBUG_DIRECTORY* entries =
      reinterpret_cast<const IMAGE_DEBUG_DIRECTORY*>(
          GetImageData(IMAGE_DIRECTORY_ENTRY_DEBUG, &debug_directory_size));
  if (!entries)
    return NULL;

  const IMAGE_DEBUG_DIRECTORY& entry = entries[index];
  const uint8_t* debug_data = NULL;
  if (GetStructureAt(entry.PointerToRawData, entry.SizeOfData, &debug_data)) {
    *raw_data = debug_data;
    *raw_data_size = entry.SizeOfData;
  }
  return &entry;
}

void PeImageReader::Clear() {
  image_data_ = NULL;
  image_size_ = 0;
  validation_state_ = 0;
  optional_header_.reset();
}

bool PeImageReader::ValidateDosHeader() {
  const IMAGE_DOS_HEADER* dos_header = NULL;
  if (!GetStructureAt(0, &dos_header) ||
      dos_header->e_magic != IMAGE_DOS_SIGNATURE ||
      dos_header->e_lfanew < 0) {
    return false;
  }

  validation_state_ |= VALID_DOS_HEADER;
  return true;
}

bool PeImageReader::ValidatePeSignature() {
  const DWORD* signature = NULL;
  if (!GetStructureAt(GetDosHeader()->e_lfanew, &signature) ||
      *signature != IMAGE_NT_SIGNATURE) {
    return false;
  }

  validation_state_ |= VALID_PE_SIGNATURE;
  return true;
}

bool PeImageReader::ValidateCoffFileHeader() {
  DCHECK_NE((validation_state_ & VALID_PE_SIGNATURE), 0U);
  const IMAGE_FILE_HEADER* file_header = NULL;
  if (!GetStructureAt(GetDosHeader()->e_lfanew +
                          offsetof(IMAGE_NT_HEADERS32, FileHeader),
                      &file_header)) {
    return false;
  }

  validation_state_ |= VALID_COFF_FILE_HEADER;
  return true;
}

bool PeImageReader::ValidateOptionalHeader() {
  const IMAGE_FILE_HEADER* file_header = GetCoffFileHeader();
  const size_t optional_header_offset =
      GetDosHeader()->e_lfanew + offsetof(IMAGE_NT_HEADERS32, OptionalHeader);
  const size_t optional_header_size = file_header->SizeOfOptionalHeader;
  const WORD* optional_header_magic = NULL;

  if (optional_header_size < sizeof(*optional_header_magic) ||
      !GetStructureAt(optional_header_offset, &optional_header_magic)) {
    return false;
  }

  scoped_ptr<OptionalHeader> optional_header;
  if (*optional_header_magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    optional_header.reset(new OptionalHeaderImpl<IMAGE_OPTIONAL_HEADER32>(
        image_data_ + optional_header_offset));
  } else if (*optional_header_magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    optional_header.reset(new OptionalHeaderImpl<IMAGE_OPTIONAL_HEADER64>(
        image_data_ + optional_header_offset));
  } else {
    return false;
  }

  // Does all of the claimed optional header fit in the image?
  if (optional_header_size > image_size_ - optional_header_offset)
    return false;

  // Is the claimed optional header big enough for everything but the dir?
  if (optional_header->GetDataDirectoryOffset() > optional_header_size)
    return false;

  // Is there enough room for all of the claimed directory entries?
  if (optional_header->GetDataDirectorySize() >
      ((optional_header_size - optional_header->GetDataDirectoryOffset()) /
       sizeof(IMAGE_DATA_DIRECTORY))) {
    return false;
  }

  optional_header_.swap(optional_header);
  validation_state_ |= VALID_OPTIONAL_HEADER;
  return true;
}

bool PeImageReader::ValidateSectionHeaders() {
  const uint8_t* first_section_header =
      GetOptionalHeaderStart() + GetOptionalHeaderSize();
  const size_t number_of_sections = GetNumberOfSections();

  // Do all section headers fit in the image?
  if (!GetStructureAt(first_section_header - image_data_,
                      number_of_sections * sizeof(IMAGE_SECTION_HEADER),
                      &first_section_header)) {
    return false;
  }

  validation_state_ |= VALID_SECTION_HEADERS;
  return true;
}

const uint8_t* PeImageReader::GetOptionalHeaderStart() {
  DCHECK_NE((validation_state_ & VALID_OPTIONAL_HEADER), 0U);
  return (image_data_ +
          GetDosHeader()->e_lfanew +
          offsetof(IMAGE_NT_HEADERS32, OptionalHeader));
}

size_t PeImageReader::GetOptionalHeaderSize() {
  return GetCoffFileHeader()->SizeOfOptionalHeader;
}

const IMAGE_DATA_DIRECTORY* PeImageReader::GetDataDirectoryEntryAt(
    size_t index) {
  DCHECK_NE((validation_state_ & VALID_OPTIONAL_HEADER), 0U);
  if (index >= optional_header_->GetDataDirectorySize())
    return NULL;
  return &optional_header_->GetDataDirectoryEntries()[index];
}

const IMAGE_SECTION_HEADER* PeImageReader::FindSectionFromRva(
    uint32_t relative_address) {
  const size_t number_of_sections = GetNumberOfSections();
  for (size_t i = 0; i < number_of_sections; ++i) {
    const IMAGE_SECTION_HEADER* section_header = GetSectionHeaderAt(i);
    // Is the raw data present in the image? If no, optimistically keep looking.
    const uint8_t* section_data = NULL;
    if (!GetStructureAt(section_header->PointerToRawData,
                        section_header->SizeOfRawData,
                        &section_data)) {
      continue;
    }
    // Does the RVA lie on or after this section's start when mapped? If no,
    // bail.
    if (section_header->VirtualAddress > relative_address)
      break;
    // Does the RVA lie within the section when mapped? If no, keep looking.
    size_t address_offset = relative_address - section_header->VirtualAddress;
    if (address_offset > section_header->Misc.VirtualSize)
      continue;
    // We have a winner.
    return section_header;
  }
  return NULL;
}

const uint8_t* PeImageReader::GetImageData(size_t index, size_t* data_length) {
  // Get the requested directory entry.
  const IMAGE_DATA_DIRECTORY* entry = GetDataDirectoryEntryAt(index);
  if (!entry)
    return NULL;

  // Find the section containing the data.
  const IMAGE_SECTION_HEADER* header =
      FindSectionFromRva(entry->VirtualAddress);
  if (!header)
    return NULL;

  // Does the data fit within the section when mapped?
  size_t data_offset = entry->VirtualAddress - header->VirtualAddress;
  if (entry->Size > (header->Misc.VirtualSize - data_offset))
    return NULL;

  // Is the data entirely present on disk (if not it's zeroed out when loaded)?
  if (data_offset >= header->SizeOfRawData ||
      header->SizeOfRawData - data_offset < entry->Size) {
    return NULL;
  }

  *data_length = entry->Size;
  return image_data_ + header->PointerToRawData + data_offset;
}

}  // namespace safe_browsing
