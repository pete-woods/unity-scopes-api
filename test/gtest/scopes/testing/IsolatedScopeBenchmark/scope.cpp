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
 * Authored by: James Henstridge <james.henstridge@canonical.com>
 *              Thomas Voß <thomas.voss@canonical.com>
 */

#include <unity/scopes/CannedQuery.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/Category.h>
#include <unity/scopes/Department.h>
#include <unity/scopes/PreviewReply.h>
#include <unity/scopes/Runtime.h>
#include <unity/scopes/ScopeBase.h>
#include <unity/scopes/SearchReply.h>

#include "scope.h"

#include <thread>

namespace testing
{

struct ActivationShowingDash : public unity::scopes::ActivationQueryBase
{
    std::mt19937& gen;
    std::normal_distribution<>& normal;

    ActivationShowingDash(unity::scopes::Result const& result,
                          unity::scopes::ActionMetadata const& metadata,
                          std::mt19937& gen,
                          std::normal_distribution<>& normal)
        : ActivationQueryBase(result, metadata),
          gen(gen),
          normal(normal)
    {
    }

    unity::scopes::ActivationResponse activate() override
    {
        std::this_thread::sleep_for(
                    std::chrono::milliseconds
                    {
                        static_cast<std::chrono::milliseconds::rep>(normal(gen))
                    });
        return unity::scopes::ActivationResponse{unity::scopes::ActivationResponse::ShowDash};
    }
};

struct LongRunningActivation : public unity::scopes::ActivationQueryBase
{
    std::mt19937& gen;
    std::normal_distribution<>& normal;

    LongRunningActivation(unity::scopes::Result const& result,
                          unity::scopes::ActionMetadata const& metadata,
                          std::string const& widget_id,
                          std::string const& action_id,
                          std::mt19937& gen,
                          std::normal_distribution<>& normal)
        : ActivationQueryBase(result, metadata, widget_id, action_id),
          gen(gen),
          normal(normal)
    {
    }

    unity::scopes::ActivationResponse activate() override
    {
        std::this_thread::sleep_for(
                    std::chrono::milliseconds
                    {
                        static_cast<std::chrono::milliseconds::rep>(normal(gen))
                    });

        return unity::scopes::ActivationResponse{unity::scopes::ActivationResponse::ShowDash};
    }
};

struct Query : public unity::scopes::SearchQueryBase
{
    std::mt19937& gen;
    std::normal_distribution<>& normal;

    Query(unity::scopes::CannedQuery const& query, unity::scopes::SearchMetadata const& metadata,
            std::mt19937& gen,
            std::normal_distribution<>& normal)
        : unity::scopes::SearchQueryBase(query, metadata),
          gen(gen),
          normal(normal)
    {
    }

    void cancelled() override
    {
    }

    void run(unity::scopes::SearchReplyProxy const&) override
    {
        std::this_thread::sleep_for(
                    std::chrono::milliseconds
                    {
                        static_cast<std::chrono::milliseconds::rep>(normal(gen))
                    });
    }
};

struct Preview : public unity::scopes::PreviewQueryBase
{
    std::mt19937& gen;
    std::normal_distribution<>& normal;

    Preview(unity::scopes::Result const& result,
            unity::scopes::ActionMetadata const& metadata,
            std::mt19937& gen,
            std::normal_distribution<>& normal)
        : unity::scopes::PreviewQueryBase(result, metadata),
          gen(gen),
          normal(normal)
    {
    }

    void cancelled() override
    {
    }

    void run(unity::scopes::PreviewReplyProxy const&) override
    {
        std::this_thread::sleep_for(
                    std::chrono::milliseconds
                    {
                        static_cast<std::chrono::milliseconds::rep>(normal(gen))
                    });
    }
};

} // namespace testing

testing::Scope::Scope(
        const std::chrono::milliseconds& mean,
        const std::chrono::milliseconds& variance)
    : gen(0),
      normal(mean.count(), variance.count())
{
}

void testing::Scope::start(std::string const&)
{
}

void testing::Scope::stop()
{
}

void testing::Scope::run()
{
}

unity::scopes::SearchQueryBase::UPtr testing::Scope::search(
        unity::scopes::CannedQuery const& query,
        unity::scopes::SearchMetadata const& metadata)
{
    return unity::scopes::SearchQueryBase::UPtr{new testing::Query(query, metadata, gen, normal)};
}

unity::scopes::ActivationQueryBase::UPtr testing::Scope::activate(
        unity::scopes::Result const& result,
        unity::scopes::ActionMetadata const& metadata)
{
    return unity::scopes::ActivationQueryBase::UPtr{new testing::ActivationShowingDash(result, metadata, gen, normal)};
}

unity::scopes::ActivationQueryBase::UPtr testing::Scope::perform_action(
        unity::scopes::Result const& result,
        unity::scopes::ActionMetadata const& metadata,
        std::string const& widget_id,
        std::string const& action_id)
{
    return unity::scopes::ActivationQueryBase::UPtr{new testing::LongRunningActivation(result, metadata, widget_id, action_id, gen, normal)};
}

unity::scopes::PreviewQueryBase::UPtr testing::Scope::preview(
        unity::scopes::Result const& result,
        unity::scopes::ActionMetadata const& metadata)
{
    return unity::scopes::PreviewQueryBase::UPtr{new testing::Preview(result, metadata, gen, normal)};
}
