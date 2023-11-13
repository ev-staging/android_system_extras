/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "JITDebugReader.h"
#include "JITDebugReader_impl.h"

#include <android-base/file.h>
#include <android-base/strings.h>
#include <android-base/test_utils.h>
#include "get_test_data.h"

#include <gtest/gtest.h>

using namespace simpleperf;
using namespace simpleperf::JITDebugReader_impl;

TEST(TempSymFile, smoke) {
  TemporaryFile tmpfile;
  std::unique_ptr<TempSymFile> symfile = TempSymFile::Create(tmpfile.path, false);
  ASSERT_TRUE(symfile);
  // If we write entries starting from offset 0, libunwindstack will treat the whole file as an elf
  // file in its elf cache. So make sure we don't start from offset 0.
  uint64_t offset = symfile->GetOffset();
  ASSERT_NE(offset, 0u);

  // Write data and read it back.
  const std::string test_data = "test_data";
  ASSERT_TRUE(symfile->WriteEntry(test_data.c_str(), test_data.size()));
  ASSERT_TRUE(symfile->Flush());

  char buf[16];
  ASSERT_TRUE(android::base::ReadFullyAtOffset(tmpfile.fd, buf, test_data.size(), offset));
  ASSERT_EQ(strncmp(test_data.c_str(), buf, test_data.size()), 0);
}

TEST(JITDebugReader, ReadDexFileDebugInfo) {
  // 1. Create dex file in memory.
  std::string dex_file_data;
  ASSERT_TRUE(android::base::ReadFileToString(GetTestData("base.vdex"), &dex_file_data));
  const int dex_file_offset = 0x28;

  // 2. Create CodeEntry pointing to the dex file in memory.
  Process process;
  process.pid = getpid();
  process.initialized = true;
  std::vector<CodeEntry> code_entries(1);
  code_entries[0].addr = reinterpret_cast<uintptr_t>(&code_entries[0]);
  code_entries[0].symfile_addr = reinterpret_cast<uintptr_t>(&dex_file_data[dex_file_offset]);
  code_entries[0].symfile_size = dex_file_data.size() - dex_file_offset;
  code_entries[0].timestamp = 0;

  // 3. Test reading symbols from dex file in memory.
  JITDebugReader reader("", JITDebugReader::SymFileOption::kDropSymFiles,
                        JITDebugReader::SyncOption::kNoSync);
  std::vector<JITDebugInfo> debug_info;
  reader.ReadDexFileDebugInfo(process, code_entries, &debug_info);
  ASSERT_EQ(debug_info.size(), 1);
  const JITDebugInfo& info = debug_info[0];
  ASSERT_TRUE(info.dex_file_map);
  ASSERT_EQ(info.dex_file_map->start_addr,
            reinterpret_cast<uintptr_t>(&dex_file_data[dex_file_offset]));
  ASSERT_EQ(info.dex_file_map->len, dex_file_data.size() - dex_file_offset);
  ASSERT_TRUE(android::base::StartsWith(info.dex_file_map->name, kDexFileInMemoryPrefix));
  ASSERT_EQ(info.symbols.size(), 12435);
  // 4. Test if the symbols are sorted.
  uint64_t prev_addr = 0;
  for (const auto& symbol : info.symbols) {
    ASSERT_LE(prev_addr, symbol.addr);
    prev_addr = symbol.addr;
  }
}
