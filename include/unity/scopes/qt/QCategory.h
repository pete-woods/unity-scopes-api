/*
 * Copyright (C) 2015 Canonical Ltd
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
 * Authored by: Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#pragma once

#ifndef _ENABLE_QT_EXPERIMENTAL_
#error You should define _ENABLE_QT_EXPERIMENTAL_ in order to use this experimental header file.
#endif

#include <unity/util/NonCopyable.h>
#include <unity/util/DefinesPtrs.h>

#include <unity/scopes/Category.h>

#include <QtCore/QString>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#include <QtCore/QVariantMap>
#pragma GCC diagnostic pop

class QCategorisedResult_test;

namespace unity
{

namespace scopes
{

namespace qt
{

namespace internal
{
class QSearchReplyImpl;
class QCategoryImpl;
class QCategorisedResultImpl;
}

/**
   \brief A set of related results returned by a scope
   and displayed within a single pane in the Unity dash.

   To create a QCategory, use QReplyProxy::register_category.
*/
class QCategory
{
public:
    /// @cond
    NONCOPYABLE(QCategory);
    UNITY_DEFINES_PTRS(QCategory);

    virtual ~QCategory();
    /// @endcond

    /**
     \brief Get identifier of this Category
     \return The category identifier.
    */
    QString id() const;

    /**
     \brief Get title of this Category
     \return The category title.
    */
    QString title() const;

    /**
     \brief Get icon of this Category
     \return The category icon.
    */
    QString icon() const;

    /**
     \brief Query to perform when this category is expanded
     \return The expansion query or nullptr.
    */
    CannedQuery::SCPtr query() const;

    /**
     \brief Get renderer template of this Category
     \return The category renderer template.
     */
    CategoryRenderer const& renderer_template() const;

    // @cond
    QVariantMap serialize() const;

private:
    /// @cond
    QCategory(unity::scopes::Category::SCPtr category);
    QCategory(internal::QCategoryImpl* impl);
    friend class unity::scopes::qt::internal::QSearchReplyImpl;

    std::unique_ptr<internal::QCategoryImpl> p;
    friend class internal::QCategoryImpl;
    friend class internal::QCategorisedResultImpl;
    /// @endcond
};

}  // namespace qt

}  // namespace scopes

}  // namespace unity
