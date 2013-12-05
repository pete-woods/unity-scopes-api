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

#ifndef UNITY_API_SCOPES_HYPERLINK_H
#define UNITY_API_SCOPES_HYPERLINK_H

#include <unity/SymbolExport.h>
#include <unity/util/DefinesPtrs.h>
#include <scopes/Query.h>
#include <memory>

namespace unity
{

namespace api
{

namespace scopes
{

namespace internal
{
class HyperlinkImpl;
class AnnotationImpl;
}

class UNITY_API Hyperlink final
{
public:
    /// @cond
    UNITY_DEFINES_PTRS(Hyperlink);
    /// @endcond

    std::string label() const;
    Query query() const;

    VariantMap serialize() const;
private:
    Hyperlink(std::string const& label, Query const& query);
    std::shared_ptr<internal::HyperlinkImpl> p;

    friend class internal::AnnotationImpl;
};

} // namespace scopes

} // namespace api

} // namespace unity

#endif
