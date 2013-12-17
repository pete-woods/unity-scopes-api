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

#ifndef UNITY_SCOPES_FILTERBASE_H
#define UNITY_SCOPES_FILTERBASE_H

#include <unity/SymbolExport.h>
#include <scopes/Variant.h>
#include <memory>

namespace unity
{

namespace api
{

namespace scopes
{

namespace internal
{
class FilterBaseImpl;
}

/**
\brief
*/
class UNITY_API FilterBase
{
public:
    std::string id() const;
    VariantMap serialize() const;
    virtual ~FilterBase();

protected:
    FilterBase(internal::FilterBaseImpl *pimpl);

    std::shared_ptr<internal::FilterBaseImpl> p;
};

} // namespace scopes

} // namespace api

} // namespace unity

#endif
