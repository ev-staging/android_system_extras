/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "read_symbol_map.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "dso.h"

using namespace simpleperf;

TEST(read_symbol_map, smoke) {
  std::string content(
      "\n"  // skip
      "   0x2000 0x20 two \n"
      "0x4000\n"            // skip
      "       0x40 four\n"  // skip
      "0x1000 0x10 one\n"
      "     \n"            // skip
      "0x5000 0x50five\n"  // skip
      " skip this line\n"  // skip
      "0x6000 0x60 six six\n"
      "0x3000 48   three   \n");

  auto symbols = ReadSymbolMapFromString(content);

  ASSERT_EQ(4u, symbols.size());

  ASSERT_EQ(0x1000, symbols[0].addr);
  ASSERT_EQ(0x10, symbols[0].len);
  ASSERT_STREQ("one", symbols[0].Name());

  ASSERT_EQ(0x2000, symbols[1].addr);
  ASSERT_EQ(0x20, symbols[1].len);
  ASSERT_STREQ("two", symbols[1].Name());

  ASSERT_EQ(0x3000, symbols[2].addr);
  ASSERT_EQ(0x30, symbols[2].len);
  ASSERT_STREQ("three", symbols[2].Name());

  // Specifically allow spaces in symbols, JIT runtimes such as V8 may generate
  // them, as "my_function (./path/to/file.js:42:0)" or "get property_name" for
  // example.
  ASSERT_EQ(0x6000, symbols[3].addr);
  ASSERT_EQ(0x60, symbols[3].len);
  ASSERT_STREQ("six six", symbols[3].Name());
}

TEST(read_symbol_map, v8_basic_perf_prof) {
  // Interesting sample of jitted function names generated by V8 running the
  // JetStream2 benchmark.
  std::string content(
      "0x300019b02e 0x77 Script:~ cli.js:1:1\n"
      "0x58f00097e0 0x4c8 JS:~initialize ./JetStreamDriver.js:373:21\n"
      "0x58f003c120 0x4c8 JS:~ (d8):246158:16\n"
      "0x58f065ad40 0x4c8 JS:~pp$1.parseEmptyStatement (d8):67313:37\n"
      "0x58f06e7aa0 0x638 RegExp:([a-z0-9$_]+)(=|@|>|%)([a-z0-9$_]+)\n"
      "0x5df00e05a0 0x324 JS:*runRichards (d8):1101:21\n"
      "0x5df017cc20 0xb18 JS:^stringToUTF8Array (d8):386:27\n"
      "0x7a4ddd2780 0x1c0 JS:wasm-to-js:iii:i-19-turbofan\n"
      "0x7a4ddedae0 0x600 JS:wasm-function[74]-74-liftoff\n");

  auto symbols = ReadSymbolMapFromString(content);

  ASSERT_EQ(9u, symbols.size());

  ASSERT_EQ(0x300019b02e, symbols[0].addr);
  ASSERT_EQ(0x77, symbols[0].len);
  ASSERT_STREQ("Script:~ cli.js:1:1", symbols[0].Name());

  ASSERT_EQ(0x58f00097e0, symbols[1].addr);
  ASSERT_EQ(0x4c8, symbols[1].len);
  ASSERT_STREQ("JS:~initialize ./JetStreamDriver.js:373:21", symbols[1].Name());

  ASSERT_EQ(0x58f003c120, symbols[2].addr);
  ASSERT_EQ(0x4c8, symbols[2].len);
  ASSERT_STREQ("JS:~ (d8):246158:16", symbols[2].Name());

  ASSERT_EQ(0x58f065ad40, symbols[3].addr);
  ASSERT_EQ(0x4c8, symbols[3].len);
  ASSERT_STREQ("JS:~pp$1.parseEmptyStatement (d8):67313:37", symbols[3].Name());

  ASSERT_EQ(0x58f06e7aa0, symbols[4].addr);
  ASSERT_EQ(0x638, symbols[4].len);
  ASSERT_STREQ("RegExp:([a-z0-9$_]+)(=|@|>|%)([a-z0-9$_]+)", symbols[4].Name());

  ASSERT_EQ(0x5df00e05a0, symbols[5].addr);
  ASSERT_EQ(0x324, symbols[5].len);
  ASSERT_STREQ("JS:*runRichards (d8):1101:21", symbols[5].Name());

  ASSERT_EQ(0x5df017cc20, symbols[6].addr);
  ASSERT_EQ(0xb18, symbols[6].len);
  ASSERT_STREQ("JS:^stringToUTF8Array (d8):386:27", symbols[6].Name());

  ASSERT_EQ(0x7a4ddd2780, symbols[7].addr);
  ASSERT_EQ(0x1c0, symbols[7].len);
  ASSERT_STREQ("JS:wasm-to-js:iii:i-19-turbofan", symbols[7].Name());

  ASSERT_EQ(0x7a4ddedae0, symbols[8].addr);
  ASSERT_EQ(0x600, symbols[8].len);
  ASSERT_STREQ("JS:wasm-function[74]-74-liftoff", symbols[8].Name());
}
