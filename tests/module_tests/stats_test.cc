/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

/*
 * Unit test for stats
 */

#include "evp_store_single_threaded_test.h"

static std::map<std::string, std::string> vals;

static void add_stats(const char *key, const uint16_t klen,
                      const char *val, const uint32_t vlen,
                      const void *cookie) {
    (void)cookie;
    std::string k(key, klen);
    std::string v(val, vlen);
    vals[k] = v;
}

class StatTest : public SingleThreadedEPStoreTest {
protected:

    void SetUp() {
        SingleThreadedEPStoreTest::SetUp();
        store->setVBucketState(vbid, vbucket_state_active, false);
    }

    void get_stat(const char *statname, const char *statkey = NULL) {
        vals.clear();
        ENGINE_HANDLE *handle = reinterpret_cast<ENGINE_HANDLE*>(engine.get());
        EXPECT_EQ(ENGINE_SUCCESS, engine->get_stats(handle, NULL, statkey,
                                                       statkey == NULL ? 0 :
                                                               strlen(statkey),
                                                               add_stats))
        << "Failed to get stats.";
    }
};

TEST_F(StatTest, vbucket_seqno_stats_test) {
    const std::string vbucket = "vb_" + std::to_string(vbid);

    // Map of known stats -> is value expected to be zero
     std::map<std::string, bool> stats = {
         {vbucket + ":high_seqno", true},
         {vbucket + ":abs_high_seqno", true},
         {vbucket + ":last_persisted_seqno", true},
         {vbucket + ":uuid", false},
         {vbucket + ":purge_seqno", true},
         {vbucket + ":last_persisted_snap_start", true},
         {vbucket + ":last_persisted_snap_end", true}
     };

     get_stat(nullptr, "vbucket-seqno");

     // Check to see if we can find all the from SeqnoVbStats
     for (const auto& stat : stats) {
          auto it = vals.find(stat.first);
          EXPECT_NE(vals.end(), it);
      };

     // Check that SeqnoVbStats does not contain any unknown stats
     // Check that all stat values are zero, except for the uuid
     for (const auto& kv : vals)  {
          auto it = stats.find(kv.first);
          EXPECT_NE(stats.end(), it) << "'" + kv.first + "' is an unknown stat";
          // Check values are all zero except for uuid
          if (it->second) {
              long int value = std::stol(kv.second);
              EXPECT_EQ(0, value) << "Stat '" + kv.first + "' has the value " <<
                      value << "and not zero as expected";
          }
      };
}
