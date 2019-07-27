/*
 *  Fast Positive Tuples (libfptu)
 *  Copyright 2016-2019 Leonid Yuriev, https://github.com/leo-yuriev/libfptu
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * ***************************************************************************
 *
 * Imported (with simplification) from 1Hippeus project.
 * Copyright (c) 2006-2013 Leonid Yuriev <leo@yuriev.ru>.
 */

#include <gtest/gtest.h>
#include <hippeus/buffer.h>

#include "../root.h"

using namespace hippeus;

TEST(Buffer, Base) {
  ASSERT_NE(nullptr, binder_handle2allot(HIPPEUS_ALLOT_LIBC));
  EXPECT_EQ(binder_handle2allot(HIPPEUS_ALLOT_LIBC),
            binder_handle2allot(HIPPEUS_ALLOT_DEFAULT));

  hip_buf_t *buf_1 = hippeus_borrow(hippeus_tag_default, 0, false);
  ASSERT_NE(nullptr, buf_1);
  ASSERT_TRUE(hippeus_buf_ensure(buf_1, true));
  EXPECT_TRUE(hippeus_buf_is_alterable(buf_1));

  hippeus_buf_addref(buf_1);
  EXPECT_FALSE(hippeus_buf_is_alterable(buf_1));

  hippeus_buf_release(buf_1);
  EXPECT_TRUE(hippeus_buf_is_alterable(buf_1));

  hippeus_buf_release(buf_1);
  ASSERT_FALSE(hippeus_buf_is_valid__expect_no(buf_1, false));
}

int main(int argc, char **argv) {
  testing ::InitGoogleTest(&argc, argv);
  hippeus_buf_flag__force_deep_checking = true;

  return RUN_ALL_TESTS();
}
