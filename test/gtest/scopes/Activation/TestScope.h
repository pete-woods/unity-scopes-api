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
 * Authored by: Pawel Stolowski <pawel.stolowski@canonical.com>
 */

#pragma once

#include <unity/scopes/ActivationListenerBase.h>
#include <unity/scopes/ActivationQueryBase.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/Category.h>
#include <unity/scopes/Runtime.h>
#include <unity/scopes/ScopeBase.h>
#include <unity/scopes/SearchReply.h>

namespace unity
{

namespace scopes
{

class TestQuery : public SearchQueryBase
{
public:
    TestQuery(CannedQuery const& query, SearchMetadata const& metadata)
        : SearchQueryBase(query, metadata)
    {
    }

    virtual void cancelled() override {}

    virtual void run(SearchReplyProxy const& reply) override
    {
        auto cat = reply->register_category("cat1", "Category 1", "");
        CategorisedResult res(cat);
        res.set_uri("uri");
        res.set_dnd_uri("dnd_uri");
        res.set_intercept_activation();
        reply->push(res);
    }
};

class TestActivation : public ActivationQueryBase
{
public:
    TestActivation(Result const& result, ActionMetadata const& metadata, std::string const& hint, std::string const& hint_val, std::string const &uri, Variant const& hints)
        : ActivationQueryBase(result, metadata),
          hint_key_(hint),
          hint_val_(hint_val),
          uri_(uri),
          recv_hints_(hints)
    {
    }

    TestActivation(Result const& result, ActionMetadata const& metadata, std::string const& widget_id, std::string const& action_id,
            std::string const& hint, std::string const& hint_val, std::string const &uri, Variant const& hints)
        : ActivationQueryBase(result, metadata, widget_id, action_id),
          hint_key_(hint),
          hint_val_(hint_val),
          uri_(uri),
          recv_hints_(hints)
    {
    }


    virtual ActivationResponse activate() override
    {
        ActivationResponse resp(ActivationResponse::Status::ShowDash);
        VariantMap var;
        var[hint_key_] = hint_val_;
        var["received_hints"] = recv_hints_; // send received hints back for testing
        var["activated_uri"] = uri_; //send activated uri back for testing
        resp.set_scope_data(Variant(var));
        return resp;
    }

private:
    std::string hint_key_;
    std::string hint_val_;
    std::string uri_;
    Variant recv_hints_;
};

class TestInCardActivation : public ActivationQueryBase
{
public:
    TestInCardActivation(Result const& result, ActionMetadata const& metadata, std::string const& action_id,
            std::string const& hint, std::string const& hint_val)
        : ActivationQueryBase(result, metadata, action_id),
          hint_key_(hint),
          hint_val_(hint_val)
    {
    }

    virtual ActivationResponse activate() override
    {
        ActivationResponse resp(result());
        VariantMap var;
        var[hint_key_] = hint_val_;
        var["received_hints"] = action_metadata().scope_data(); // send received hints back for testing
        resp.set_scope_data(Variant(var));
        return resp;
    }

private:
    std::string hint_key_;
    std::string hint_val_;
};

class TestScope : public ScopeBase
{
public:
    virtual void start(std::string const&) override {}

    virtual void stop() override {}

    virtual void run() override {}

    virtual SearchQueryBase::UPtr search(CannedQuery const& query, SearchMetadata const& metadata) override
    {
        return SearchQueryBase::UPtr(new TestQuery(query, metadata));
    }

    virtual PreviewQueryBase::UPtr preview(Result const&, ActionMetadata const &) override
    {
        return nullptr;
    }

    virtual ActivationQueryBase::UPtr activate(Result const& result, ActionMetadata const& hints) override
    {
        return ActivationQueryBase::UPtr(new TestActivation(result, hints, "foo", "bar", result.uri(), hints.scope_data()));
    }

    virtual ActivationQueryBase::UPtr perform_action(Result const& result, ActionMetadata const& hints, std::string const& widget_id, std::string const& action_id) override
    {
        return ActivationQueryBase::UPtr(new TestActivation(result, hints, widget_id, action_id, "activated action", widget_id + action_id, result.uri(), hints.scope_data()));
    }

    virtual ActivationQueryBase::UPtr activate_result_action(Result const& result, ActionMetadata const& hints, std::string const& action_id) override
    {
        return ActivationQueryBase::UPtr(new TestInCardActivation(result, hints, action_id, "activated result action", action_id));
    }
};

} // namespace scopes

} // namespace unity
