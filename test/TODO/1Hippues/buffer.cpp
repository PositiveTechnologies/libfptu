/*
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
