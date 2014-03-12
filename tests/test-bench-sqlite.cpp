/*
  marky - A Markov chain generator.
  Copyright (C) 2011-2014  Nicholas Parker

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <gtest/gtest.h>
#include <marky/backend-sqlite.h>
#include <unistd.h> //unlink()
#include <test-bench-config.h> //TEST_DATA_PATH

using namespace marky;

#define SQLITE_DB_PATH "sqlite_test.db"

class SQLiteBench : public testing::Test {
protected:
    /* called before every test */
    virtual void SetUp() {
        unlink(SQLITE_DB_PATH);
    }

    /* called after every test */
    virtual void TearDown() {
        unlink(SQLITE_DB_PATH);
    }
};

TEST_F(SQLiteBench, get_prev) {
    /*
      TODO cpu/mem bench/profile these against a big text file:
      - sqlite standalone
      - sqlite+writecache
      - sqlite+rwcache
    */
    ASSERT_TRUE(false) << "TODO IMPLEMENT";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest( &argc, argv );
    return RUN_ALL_TESTS();
}
