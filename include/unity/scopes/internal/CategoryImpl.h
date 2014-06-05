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

#ifndef UNITY_SCOPES_INTERNAL_CATEGORYIMPL_H
#define UNITY_SCOPES_INTERNAL_CATEGORYIMPL_H

#include <unity/util/NonCopyable.h>
#include <unity/scopes/Variant.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/Category.h>
#include <string>
#include <memory>

namespace unity
{

namespace scopes
{

namespace internal
{

class CategoryImpl
{
public:
    NONCOPYABLE(CategoryImpl);

    CategoryImpl(VariantMap const& variant_map);
    CategoryImpl(std::string const& id, std::string const& title, std::string const &icon, CategoryRenderer const& renderer_template);
    CategoryImpl(std::string const& id, std::string const& title, std::string const &icon, CategoryRenderer const& renderer_template, Category::TapBehavior
            single_tap_behavior, Category::TapBehavior long_tap_behavior);
    std::string const& id() const;
    std::string const& title() const;
    std::string const& icon() const;
    Category::TapBehavior single_tap_behavior() const;
    Category::TapBehavior long_tap_behavior() const;
    CategoryRenderer const& renderer_template() const;
    VariantMap serialize() const;

private:
    void deserialize(VariantMap const& variant_map);

    std::string id_;
    std::string title_;
    std::string icon_;
    Category::TapBehavior single_tap_behavior_;
    Category::TapBehavior long_tap_behavior_;
    CategoryRenderer renderer_template_;
};

} // namespace internal

} // namespace scopes

} // namespace unity

#endif
