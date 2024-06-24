/**
 * Copyright (c) 2023 OceanBase
 * OceanBase is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#ifndef SRC_SHARE_VECTOR_INDEX_OB_IVFFLAT_INDEX_SEARCH_HELPER_H_
#define SRC_SHARE_VECTOR_INDEX_OB_IVFFLAT_INDEX_SEARCH_HELPER_H_

#include "share/vector_index/ob_ivf_index_search_helper.h"

namespace oceanbase
{
namespace share
{

class ObIvfflatIndexSearchHelper: public ObIvfIndexSearchHelper
{
public:
  ObIvfflatIndexSearchHelper()
    : ObIvfIndexSearchHelper("Ivfflat")
  {}
private:
  int get_vector_probes(const sql::ObSQLSessionInfo *session, uint64_t &probes) const override;
  int64_t get_vector_lists(const schema::ObTableSchema *container_table_schema) const override;
};
} // share
} // oceanbase
#endif