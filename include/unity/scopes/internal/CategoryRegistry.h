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

#ifndef UNITY_SCOPES_INTERNAL_CATEGORYREGISTRY_H
#define UNITY_SCOPES_INTERNAL_CATEGORYREGISTRY_H

#include <unity/scopes/Category.h>
#include <unity/scopes/Variant.h>
#include <string>
#include <memory>
#include <mutex>

namespace unity
{

namespace scopes
{

class CategoryRenderer;

namespace internal
{

/**
\brief Provides factory method for creating Category instances and keeping track of them.
This class is for internal use, most of its functionality is exposed via register_category and find_category method
of Reply class.
*/
class CategoryRegistry
{
public:
    CategoryRegistry() = default;

    /**
    \brief Deserializes category from a variant_map and registers it. Throws if category with same id exists.
    \return category instance
    */
    Category::SCPtr register_category(VariantMap const& variant_map);

    /**
    \brief Creates category from supplied parameters. Throws if category with same id exists.
    \return category instance
    */
    Category::SCPtr register_category(std::string const& id, std::string const& title, std::string const& icon, CategoryRenderer const& renderer_template,
            Category::TapBehavior single_tap_behavior = Category::TapBehavior::ShowPreview,
            Category::TapBehavior long_tap_behavior = Category::TapBehavior::ShowPreview);

    /**
    \brief Finds category instance with give id.
    \return category instance or nullptr if no such category has been registered
    */
    Category::SCPtr lookup_category(std::string const& id) const;

    /**
    \brief Register an existing category instance with this registry.
    Throws if category with sane id exists.
    */
    void register_category(Category::SCPtr category);

private:
    mutable std::mutex mutex_;
    std::map<std::string, Category::SCPtr> categories_;
};

} // namespace internal

} // namespace scopes

} // namespace unity

#endif
