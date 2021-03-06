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

#include <unity/scopes/internal/zmq_middleware/RegistryI.h>

#include <scopes/internal/zmq_middleware/capnproto/Registry.capnp.h>
#include <scopes/internal/zmq_middleware/capnproto/Scope.capnp.h>
#include <unity/scopes/internal/RegistryException.h>
#include <unity/scopes/internal/RegistryObjectBase.h>
#include <unity/scopes/internal/zmq_middleware/ObjectAdapter.h>
#include <unity/scopes/internal/zmq_middleware/VariantConverter.h>
#include <unity/scopes/internal/zmq_middleware/ZmqScope.h>
#include <unity/scopes/ScopeExceptions.h>

#include <cassert>

using namespace std;

namespace unity
{

namespace scopes
{

namespace internal
{

namespace zmq_middleware
{

/*

interface Scope;

dictionary<string, VariantMap> MetadataMap;

exception NotFoundException
{
    string identity;
};

interface Registry
{
    ScopeMetadata get_metadata(string scope_id) throws NotFoundException;
    MetadataMap list();
    ObjectProxy locate(string identity) throws NotFoundException, RegistryException;
};

*/

using namespace std;
namespace ph = std::placeholders;

RegistryI::RegistryI(RegistryObjectBase::SPtr const& ro) :
    ServantBase(ro, { { "get_metadata", bind(&RegistryI::get_metadata_, this, ph::_1, ph::_2, ph::_3) },
                      { "list", bind(&RegistryI::list_, this, ph::_1, ph::_2, ph::_3) },
                      { "locate", bind(&RegistryI::locate_, this, ph::_1, ph::_2, ph::_3) },
                      { "is_scope_running", bind(&RegistryI::is_scope_running_, this, ph::_1, ph::_2, ph::_3) } })

{
}

RegistryI::~RegistryI()
{
}

void RegistryI::get_metadata_(Current const&,
                              capnp::AnyPointer::Reader& in_params,
                              capnproto::Response::Builder& r)
{
    auto req = in_params.getAs<capnproto::Registry::GetMetadataRequest>();
    string scope_id = req.getIdentity().cStr();
    auto delegate = dynamic_pointer_cast<RegistryObjectBase>(del());
    try
    {
        auto meta = delegate->get_metadata(scope_id);
        r.setStatus(capnproto::ResponseStatus::SUCCESS);
        auto get_metadata_response = r.initPayload().getAs<capnproto::Registry::GetMetadataResponse>().initResponse();
        auto dict = get_metadata_response.initReturnValue();
        to_value_dict(meta.serialize(), dict);
    }
    catch (NotFoundException const& e)
    {
        r.setStatus(capnproto::ResponseStatus::USER_EXCEPTION);
        auto get_metadata_response = r.initPayload().getAs<capnproto::Registry::GetMetadataResponse>().initResponse();
        get_metadata_response.initNotFoundException().setIdentity(e.name().c_str());
    }
}

void RegistryI::list_(Current const&,
                      capnp::AnyPointer::Reader&,
                      capnproto::Response::Builder& r)
{
    auto delegate = dynamic_pointer_cast<RegistryObjectBase>(del());
    auto metadata_map = delegate->list();
    r.setStatus(capnproto::ResponseStatus::SUCCESS);
    auto list_response = r.initPayload().getAs<capnproto::Registry::ListResponse>();
    auto dict = list_response.initReturnValue().initPairs(metadata_map.size());
    int i = 0;
    for (auto& pair : metadata_map)
    {
        dict[i].setName(pair.first.c_str());        // Scope ID
        auto md = dict[i].initValue().initDictVal();
        to_value_dict(pair.second.serialize(), md); // Scope metadata
        ++i;
    }
}

void RegistryI::locate_(Current const&,
                        capnp::AnyPointer::Reader& in_params,
                        capnproto::Response::Builder& r)
{
    auto req = in_params.getAs<capnproto::Registry::GetMetadataRequest>();
    string identity = req.getIdentity().cStr();
    auto delegate = dynamic_pointer_cast<RegistryObjectBase>(del());
    try
    {
        auto proxy = delegate->locate(identity);
        r.setStatus(capnproto::ResponseStatus::SUCCESS);
        auto locate_response = r.initPayload().getAs<capnproto::Registry::LocateResponse>().initResponse();
        auto return_proxy = locate_response.initReturnValue();
        return_proxy.setIdentity(proxy->identity());
        return_proxy.setCategory(proxy->target_category());
        return_proxy.setEndpoint(proxy->endpoint());
        return_proxy.setTimeout(proxy->timeout());
    }
    catch (NotFoundException const& e)
    {
        r.setStatus(capnproto::ResponseStatus::USER_EXCEPTION);
        auto locate_response = r.initPayload().getAs<capnproto::Registry::LocateResponse>().initResponse();
        locate_response.initNotFoundException().setIdentity(e.name().c_str());
    }
    catch (RegistryException const& e)
    {
        r.setStatus(capnproto::ResponseStatus::USER_EXCEPTION);
        auto locate_response = r.initPayload().getAs<capnproto::Registry::LocateResponse>().initResponse();
        locate_response.initRegistryException().setReason(e.reason().c_str());
    }
}

void RegistryI::is_scope_running_(Current const&,
                                  capnp::AnyPointer::Reader& in_params,
                                  capnproto::Response::Builder& r)
{
    auto req = in_params.getAs<capnproto::Registry::IsScopeRunningRequest>();
    string scope_id = req.getIdentity().cStr();
    auto delegate = dynamic_pointer_cast<RegistryObjectBase>(del());
    try
    {
        auto is_running = delegate->is_scope_running(scope_id);
        r.setStatus(capnproto::ResponseStatus::SUCCESS);
        auto is_scope_running_response = r.initPayload().getAs<capnproto::Registry::IsScopeRunningResponse>().initResponse();
        is_scope_running_response.setReturnValue(is_running);
    }
    catch (NotFoundException const& e)
    {
        r.setStatus(capnproto::ResponseStatus::USER_EXCEPTION);
        auto get_metadata_response = r.initPayload().getAs<capnproto::Registry::IsScopeRunningResponse>().initResponse();
        get_metadata_response.initNotFoundException().setIdentity(e.name().c_str());
    }
}

} // namespace zmq_middleware

} // namespace internal

} // namespace scopes

} // namespace unity
