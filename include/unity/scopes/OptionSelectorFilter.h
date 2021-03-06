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

#include <unity/scopes/FilterBase.h>
#include <unity/scopes/FilterOption.h>
#include <unity/scopes/Variant.h>
#include <unity/scopes/FilterGroup.h>
#include <string>
#include <list>
#include <set>

namespace unity
{

namespace scopes
{
class FilterState;

namespace internal
{
class OptionSelectorFilterImpl;
class FilterBaseImpl;
}

/**
\brief A selection filter that displays a list of choices and allows one or more of them to be selected.
*/

class OptionSelectorFilter : public FilterBase
{
public:
/// @cond
    UNITY_DEFINES_PTRS(OptionSelectorFilter);
/// @endcond

    /**
    \brief Creates an OpionSelectorFilter.
    \param id A unique identifier for the filter that can be used to later identify it among several filters.
    \param label A display label for the filter.
    \param multi_select If true, the filter permits more than option to be selected; otherwise, only a single
    option can be selected.

    \note The multi-selection cannot be combined with unity::scopes::FilterBase::DisplayHints::Primary flag set via
    unity::scopes::FilterBase::set_display_hints().
    */
    static OptionSelectorFilter::UPtr create(std::string const& id, std::string const& label, bool multi_select = false);

    /**
    \brief Get the label of this filter.
    \return The filter label.
    */
    std::string label() const;

    /**
    \brief Check if this filter supports multiple options to be selected.
    \return True if multi-selection is enabled.
    */
    bool multi_select() const;

    /**
    \brief Add a new option to this filter. The option is 'off' by default.

    \throws unity::InvalidArgumentException on invalid id or label
    \return The new option instance.
    */
    FilterOption::SCPtr add_option(std::string const& id, std::string const& label);

    /**
    \brief Get all options of this filter, in the order they were added.
    \return The list of options.
     */
    std::list<FilterOption::SCPtr> options() const;

    /**
    \brief Check if an option is active for this filter.
    \param filter_state The state of filters
    \return true if an option is active
    */
    bool has_active_option(FilterState const& filter_state) const;

    /**
    \brief Get the active options from a FilterState instance for this filter.

    The returned set may be empty if the user de-selected all options. However, if there is no state
    recorded for this filter in the filter_state instance, then all the options enabled by default are returned.

    \return The set of selected filter options (or options enabled by default if the filter is not present in the filter_state).
    */
    std::set<FilterOption::SCPtr> active_options(FilterState const& filter_state) const;

    /**
    \brief Marks given FilterOption of this filter instance as active (or not active) in a FilterState object.

    Records the given FilterOption as "selected" in the FilterState. This is meant to be used to modify a
    FilterState received with a search request before sending it back to the client (UI shell).
    */
    void update_state(FilterState& filter_state, FilterOption::SCPtr option, bool active) const;

    /**
    \brief Marks an option of a filter active/inactive in a FilterState object, without having an instance of OptionSelectorFilter.

    Updates an instance of FilterState, without the need for an OptionSelectorFilter instance. This is meant
    to be used when creating a canned Query that references another scope.
    */
    static void update_state(FilterState& filter_state, std::string const& filter_id, std::string const& option_id, bool value);

    /**
    \brief Add a new option to this filter and provide its default value.

    \throws unity::LogicException if multiple options with value of 'true' are provided for a single-selection OptionSelectorFilter.
    \throws unity::InvalidArgumentException on invalid id or label
    \return The new option instance.
    */
    FilterOption::SCPtr add_option(std::string const& id, std::string const& label, bool value);

    /**
    \brief Creates an OpionSelectorFilter inside a FilterGroup.
    \param id A unique identifier for the filter that can be used to later identify it among several filters.
    \param label A display label for the filter.
    \param group A filter group this filter should be added to.
    \param multi_select If true, the filter permits more than option to be selected; otherwise, only a single
    option can be selected.
    \throws unity::InvalidArgumentException on invalid null group.

    \note The multi-selection cannot be combined with unity::scopes::FilterBase::DisplayHints::Primary flag set via
    unity::scopes::FilterBase::set_display_hints().
    */
    static OptionSelectorFilter::UPtr create(std::string const& id, std::string const& label, FilterGroup::SCPtr const& group, bool multi_select = false);

private:
    OptionSelectorFilter(internal::OptionSelectorFilterImpl*);
    internal::OptionSelectorFilterImpl* fwd() const;
    friend class internal::OptionSelectorFilterImpl;
};

} // namespace scopes

} // namespace unity
