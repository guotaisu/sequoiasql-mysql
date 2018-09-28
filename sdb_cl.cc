/* Copyright (c) 2018, SequoiaDB and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include <my_base.h>
#include "sdb_cl.h"
#include "sdb_conn.h"
#include "sdb_err_code.h"

using namespace sdbclient;

Sdb_cl::Sdb_cl() : p_conn(NULL), m_thread_id(0) {}

Sdb_cl::~Sdb_cl() {
  close();
}

int Sdb_cl::init(Sdb_conn *connection, char *cs, char *cl) {
  int rc = SDB_ERR_OK;
  if (NULL == connection || NULL == cs || NULL == cl) {
    rc = SDB_ERR_INVALID_ARG;
    goto error;
  }

  p_conn = connection;
  m_thread_id = connection->thread_id();

  cs_name[SDB_CS_NAME_MAX_SIZE] = 0;
  strncpy(cs_name, cs, SDB_CS_NAME_MAX_SIZE);

  cl_name[SDB_CL_NAME_MAX_SIZE] = 0;
  strncpy(cl_name, cl, SDB_CL_NAME_MAX_SIZE);

  rc = re_init();

done:
  return rc;
error:
  goto done;
}

int Sdb_cl::re_init() {
  int rc = SDB_ERR_OK;
  int retry_times = 2;
  sdbCollectionSpace cs;

retry:
  rc = p_conn->get_sdb().getCollectionSpace(cs_name, cs);
  if (rc != SDB_ERR_OK) {
    goto error;
  }

  rc = cs.getCollection(cl_name, cl);
  if (rc != SDB_ERR_OK) {
    goto error;
  }

done:
  return rc;
error:
  if (IS_SDB_NET_ERR(rc)) {
    bool is_transaction = p_conn->is_transaction_on();
    if (!is_transaction && retry_times-- > 0 && 0 == p_conn->connect()) {
      goto retry;
    }
  }
  convert_sdb_code(rc);
  goto done;
}

int Sdb_cl::check_connect(int rc) {
  if (SDB_NETWORK == rc || SDB_NOT_CONNECTED == rc) {
    return p_conn->connect();
  }
  return SDB_ERR_OK;
}

bool Sdb_cl::is_transaction_on() {
  return p_conn->is_transaction_on();
}

char *Sdb_cl::get_cs_name() {
  return cs_name;
}

char *Sdb_cl::get_cl_name() {
  return cl_name;
}

int Sdb_cl::query(const bson::BSONObj &condition, const bson::BSONObj &selected,
                  const bson::BSONObj &orderBy, const bson::BSONObj &hint,
                  INT64 numToSkip, INT64 numToReturn, INT32 flags) {
  int rc = SDB_ERR_OK;
  int retry_times = 2;
retry:
  rc = cl.query(cursor, condition, selected, orderBy, hint, numToSkip,
                numToReturn, flags);
  if (SDB_ERR_OK != rc) {
    goto error;
  }

done:
  return rc;
error:
  if (IS_SDB_NET_ERR(rc)) {
    bool is_transaction = p_conn->is_transaction_on();
    if (0 == p_conn->connect() && !is_transaction && retry_times-- > 0) {
      goto retry;
    }
  }
  convert_sdb_code(rc);
  goto done;
}

int Sdb_cl::query_one(bson::BSONObj &obj, const bson::BSONObj &condition,
                      const bson::BSONObj &selected,
                      const bson::BSONObj &orderBy, const bson::BSONObj &hint,
                      INT64 numToSkip, INT32 flags) {
  int rc = SDB_ERR_OK;
  sdbclient::sdbCursor cursor_tmp;
  int retry_times = 2;
retry:
  rc = cl.query(cursor_tmp, condition, selected, orderBy, hint, numToSkip, 1,
                flags);
  if (rc != SDB_ERR_OK) {
    goto error;
  }

  rc = cursor_tmp.next(obj);
  if (rc != SDB_ERR_OK) {
    goto error;
  }

done:
  return rc;
error:
  if (IS_SDB_NET_ERR(rc)) {
    bool is_transaction = p_conn->is_transaction_on();
    if (0 == p_conn->connect() && !is_transaction && retry_times-- > 0) {
      goto retry;
    }
  }
  convert_sdb_code(rc);
  goto done;
}

int Sdb_cl::current(bson::BSONObj &obj) {
  int rc = SDB_ERR_OK;
  rc = cursor.current(obj);
  if (rc != SDB_ERR_OK) {
    if (SDB_DMS_EOC == rc) {
      rc = HA_ERR_END_OF_FILE;
    }
    goto error;
  }

done:
  return rc;
error:
  convert_sdb_code(rc);
  goto done;
}

int Sdb_cl::next(bson::BSONObj &obj) {
  int rc = SDB_ERR_OK;
  rc = cursor.next(obj);
  if (rc != SDB_ERR_OK) {
    if (SDB_DMS_EOC == rc) {
      rc = HA_ERR_END_OF_FILE;
    }
    goto error;
  }

done:
  return rc;
error:
  convert_sdb_code(rc);
  goto done;
}

int Sdb_cl::insert(bson::BSONObj &obj) {
  int rc = SDB_ERR_OK;
  int retry_times = 2;
retry:
  rc = cl.insert(obj);
  if (rc != SDB_ERR_OK) {
    goto error;
  }
done:
  return rc;
error:
  if (IS_SDB_NET_ERR(rc)) {
    bool is_transaction = p_conn->is_transaction_on();
    if (0 == p_conn->connect() && !is_transaction && retry_times-- > 0) {
      goto retry;
    }
  }
  convert_sdb_code(rc);
  goto done;
}

int Sdb_cl::upsert(const bson::BSONObj &rule, const bson::BSONObj &condition,
                   const bson::BSONObj &hint, const bson::BSONObj &setOnInsert,
                   INT32 flag) {
  int rc = SDB_ERR_OK;
  int retry_times = 2;
retry:
  rc = cl.upsert(rule, condition, hint, setOnInsert, flag);
  if (rc != SDB_ERR_OK) {
    goto error;
  }
done:
  return rc;
error:
  if (IS_SDB_NET_ERR(rc)) {
    bool is_transaction = p_conn->is_transaction_on();
    if (0 == p_conn->connect() && !is_transaction && retry_times-- > 0) {
      goto retry;
    }
  }
  convert_sdb_code(rc);
  goto done;
}

int Sdb_cl::update(const bson::BSONObj &rule, const bson::BSONObj &condition,
                   const bson::BSONObj &hint, INT32 flag) {
  int rc = SDB_ERR_OK;
  int retry_times = 2;
retry:
  rc = cl.update(rule, condition, hint, flag);
  if (rc != SDB_ERR_OK) {
    goto error;
  }
done:
  return rc;
error:
  if (IS_SDB_NET_ERR(rc)) {
    bool is_transaction = p_conn->is_transaction_on();
    if (0 == p_conn->connect() && !is_transaction && retry_times-- > 0) {
      goto retry;
    }
  }
  convert_sdb_code(rc);
  goto done;
}

int Sdb_cl::del(const bson::BSONObj &condition, const bson::BSONObj &hint) {
  int rc = SDB_ERR_OK;
  int retry_times = 2;
retry:
  rc = cl.del(condition, hint);
  if (rc != SDB_ERR_OK) {
    goto error;
  }
done:
  return rc;
error:
  if (IS_SDB_NET_ERR(rc)) {
    bool is_transaction = p_conn->is_transaction_on();
    if (0 == p_conn->connect() && !is_transaction && retry_times-- > 0) {
      goto retry;
    }
  }
  convert_sdb_code(rc);
  goto done;
}

int Sdb_cl::create_index(const bson::BSONObj &indexDef, const CHAR *pName,
                         BOOLEAN isUnique, BOOLEAN isEnforced) {
  int rc = SDB_ERR_OK;
  int retry_times = 2;
retry:
  rc = cl.createIndex(indexDef, pName, isUnique, isEnforced);
  if (SDB_IXM_REDEF == rc) {
    rc = SDB_ERR_OK;
  }
  if (rc != SDB_ERR_OK) {
    goto error;
  }
done:
  return rc;
error:
  if (IS_SDB_NET_ERR(rc)) {
    bool is_transaction = p_conn->is_transaction_on();
    if (0 == p_conn->connect() && !is_transaction && retry_times-- > 0) {
      goto retry;
    }
  }
  convert_sdb_code(rc);
  goto done;
}

int Sdb_cl::drop_index(const char *pName) {
  int rc = SDB_ERR_OK;
  int retry_times = 2;
retry:
  rc = cl.dropIndex(pName);
  if (SDB_IXM_NOTEXIST == rc) {
    rc = SDB_ERR_OK;
  }
  if (rc != SDB_ERR_OK) {
    goto error;
  }
done:
  return rc;
error:
  if (IS_SDB_NET_ERR(rc)) {
    bool is_transaction = p_conn->is_transaction_on();
    if (0 == p_conn->connect() && !is_transaction && retry_times-- > 0) {
      goto retry;
    }
  }
  convert_sdb_code(rc);
  goto done;
}

int Sdb_cl::truncate() {
  int rc = SDB_ERR_OK;
  int retry_times = 2;
retry:
  rc = cl.truncate();
  if (rc != SDB_ERR_OK) {
    goto error;
  }
done:
  return rc;
error:
  if (IS_SDB_NET_ERR(rc)) {
    bool is_transaction = p_conn->is_transaction_on();
    if (0 == p_conn->connect() && !is_transaction && retry_times-- > 0) {
      goto retry;
    }
  }
  convert_sdb_code(rc);
  goto done;
}

void Sdb_cl::close() {
  cursor.close();
}

my_thread_id Sdb_cl::thread_id() {
  return m_thread_id;
}

int Sdb_cl::drop() {
  int rc = SDB_ERR_OK;
  int retry_times = 2;
retry:
  rc = cl.drop();
  if (rc != SDB_ERR_OK) {
    if (SDB_DMS_NOTEXIST == rc) {
      rc = SDB_ERR_OK;
      goto done;
    }
    goto error;
  }
done:
  return rc;
error:
  if (IS_SDB_NET_ERR(rc)) {
    bool is_transaction = p_conn->is_transaction_on();
    if (0 == p_conn->connect() && !is_transaction && retry_times-- > 0) {
      goto retry;
    }
  }
  convert_sdb_code(rc);
  goto done;
}

int Sdb_cl::get_count(long long &count) {
  int rc = SDB_ERR_OK;
  int retry_times = 2;
retry:
  rc = cl.getCount(count);
  if (rc != SDB_ERR_OK) {
    goto error;
  }
done:
  return rc;
error:
  if (IS_SDB_NET_ERR(rc)) {
    bool is_transaction = p_conn->is_transaction_on();
    if (0 == p_conn->connect() && !is_transaction && retry_times-- > 0) {
      goto retry;
    }
  }
  convert_sdb_code(rc);
  goto done;
}
