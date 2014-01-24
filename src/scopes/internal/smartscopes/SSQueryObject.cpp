/*
 * Copyright (C) 2013 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Marcus Tomlinson <marcus.tomlinson@canonical.com>
 */

#include <unity/scopes/internal/smartscopes/SSQueryObject.h>

#include <unity/scopes/internal/MWQueryCtrl.h>
#include <unity/scopes/internal/MWReply.h>
#include <unity/scopes/internal/QueryCtrlObject.h>
#include <unity/scopes/internal/ReplyImpl.h>
#include <unity/scopes/SearchQuery.h>
#include <unity/scopes/PreviewQuery.h>
#include <unity/scopes/ActivationBase.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/SearchQuery.h>
#include <unity/Exception.h>

#include <iostream>
#include <cassert>

using namespace std;
using namespace unity::scopes::internal;

namespace unity
{

namespace scopes
{

namespace internal
{

namespace smartscopes
{

SSQueryObject::SSQueryObject() :
  QueryObjectBase()
{
}

SSQueryObject::~SSQueryObject() noexcept
{
}

void SSQueryObject::run(MWReplyProxy const& reply, InvokeInfo const& info) noexcept
{
  assert(self_);
  assert(queries_.find(info.id) != end(queries_));

  QueryBase::SPtr const& q_base = queries_[info.id].q_base;
  MWReplyProxy const& q_reply = queries_[info.id].q_reply;

  // Create the reply proxy to pass to query_base_ and keep a weak_ptr, which we will need
  // if cancel() is called later.
  auto reply_proxy = ReplyImpl::create(reply, self_);
  assert(reply_proxy);
  queries_[info.id].q_reply_proxy = reply_proxy;

  // The reply proxy now holds our reference count high, so
  // we can drop our own smart pointer and disconnect from the middleware.
  self_ = nullptr;
  disconnect();

  // Synchronous call into scope implementation.
  // On return, replies for the query may still be outstanding.
  try
  {
      auto search_query = dynamic_pointer_cast<SearchQuery>(q_base);
      assert(search_query);
      search_query->run(reply_proxy);
  }
  catch (std::exception const& e)
  {
      queries_[info.id].q_pushable = false;
      // TODO: log error
      q_reply->finished(ListenerBase::Error, e.what());     // Oneway, can't block
      cerr << "ScopeBase::run(): " << e.what() << endl;
  }
  catch (...)
  {
      queries_[info.id].q_pushable = false;
      // TODO: log error
      q_reply->finished(ListenerBase::Error, "unknown exception");     // Oneway, can't block
      cerr << "ScopeBase::run(): unknown exception" << endl;
  }
}

void SSQueryObject::cancel(InvokeInfo const& info)
{
  assert(queries_.find(info.id) != end(queries_));

  QueryBase::SPtr const& q_base = queries_[info.id].q_base;
  MWReplyProxy const& q_reply = queries_[info.id].q_reply;

  queries_[info.id].q_pushable = false;
  auto rp = queries_[info.id].q_reply_proxy.lock();
  if (rp)
  {
      // Send finished() to up-stream client to tell him the query is done.
      // We send via the MWReplyProxy here because that allows passing
      // a ListenerBase::Reason (whereas the public ReplyProxy does not).
      q_reply->finished(ListenerBase::Cancelled, "");     // Oneway, can't block
  }

  // Forward the cancellation to the query base (which in turn will forward it to any subqueries).
  // The query base also calls the cancelled() callback to inform the application code.
  q_base->cancel();
}

bool SSQueryObject::pushable(InvokeInfo const& info) const noexcept
{
  assert(queries_.find(info.id) != end(queries_));

  return queries_.at(info.id).q_pushable;
}

void SSQueryObject::set_self(QueryObjectBase::SPtr const& self) noexcept
{
  assert(self);
  assert(!self_);
  self_ = self;
}

void SSQueryObject::add_query(std::string const& scope_id, QueryBase::SPtr const& query_base, MWReplyProxy const& reply)
{
  queries_[scope_id] = SSQuery{query_base, reply, std::weak_ptr<ReplyBase>(), true};
}

} // namespace smartscopes

} // namespace internal

} // namespace scopes

} // namespace unity
