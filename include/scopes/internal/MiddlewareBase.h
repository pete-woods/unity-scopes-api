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
 * Authored by: Michi Henning <michi.henning@canonical.com>
 */

#ifndef UNITY_SCOPES_INTERNAL_MIDDLEWAREBASE_H
#define UNITY_SCOPES_INTERNAL_MIDDLEWAREBASE_H

#include <scopes/internal/MWObjectProxyFwd.h>
#include <scopes/internal/MWQueryCtrlProxyFwd.h>
#include <scopes/internal/MWQueryProxyFwd.h>
#include <scopes/internal/MWRegistryProxyFwd.h>
#include <scopes/internal/MWScopeProxyFwd.h>
#include <scopes/internal/QueryObject.h>
#include <scopes/internal/QueryCtrlObject.h>
#include <scopes/internal/RegistryObject.h>
#include <scopes/internal/ReplyObject.h>
#include <scopes/internal/ScopeObject.h>

namespace unity
{

namespace scopes
{

class ScopeBase;

namespace internal
{

class RuntimeImpl;

// Abstract base class for our middleware transport. Any control functions we need can be added here
// as pure virtual functions. This is used by the remainder of the code to do things to the middleware
// without knowing which middleware it actually is.

class MiddlewareBase
{
public:
    NONCOPYABLE(MiddlewareBase)
    UNITY_DEFINES_PTRS(MiddlewareBase);

    MiddlewareBase(RuntimeImpl* runtime);
    virtual ~MiddlewareBase() noexcept;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void wait_for_shutdown() = 0;

    virtual MWProxy create_proxy(std::string const& identity, std::string const& endpoint) = 0;

    virtual MWRegistryProxy create_registry_proxy(std::string const& identity, std::string const& endpoint) = 0;
    virtual MWScopeProxy create_scope_proxy(std::string const& identity) = 0;
    virtual MWScopeProxy create_scope_proxy(std::string const& identity, std::string const& endpoint) = 0;

    virtual MWQueryCtrlProxy add_query_ctrl_object(QueryCtrlObject::SPtr const& ctrl) = 0;
    virtual MWQueryProxy add_query_object(QueryObject::SPtr const& query) = 0;
    virtual MWRegistryProxy add_registry_object(std::string const& identity, RegistryObject::SPtr const& registry) = 0;
    virtual MWReplyProxy add_reply_object(ReplyObject::SPtr const& reply) = 0;
    virtual MWScopeProxy add_scope_object(std::string const& identity, ScopeObject::SPtr const& scope) = 0;

    RuntimeImpl* runtime() const noexcept;

private:
    RuntimeImpl* const runtime_;
};

} // namespace internal

} // namespace scopes

} // namespace unity

#endif
