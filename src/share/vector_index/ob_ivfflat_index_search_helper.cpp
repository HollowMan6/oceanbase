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

#define USING_LOG_PREFIX COMMON

#include "share/vector_index/ob_ivfflat_index_search_helper.h"
#include "sql/session/ob_sql_session_info.h"

namespace oceanbase
{
namespace share
{
  int ObIvfflatIndexSearchHelper::get_vector_probes(const sql::ObSQLSessionInfo *session, uint64_t &probes) const {
    return session->get_vector_ivfflat_probes(probes);
  };
  int64_t ObIvfflatIndexSearchHelper::get_vector_lists(const schema::ObTableSchema *container_table_schema) const {
    return container_table_schema->get_vector_ivf_lists();
  };

} // share
} // oceanbase