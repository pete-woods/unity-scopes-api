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

#include <scopes/internal/OptionSelectorFilterImpl.h>
#include <scopes/FilterState.h>
#include <unity/UnityExceptions.h>
#include <unordered_set>
#include <sstream>
#include <algorithm>

namespace unity
{

namespace api
{

namespace scopes
{

namespace internal
{

OptionSelectorFilterImpl::OptionSelectorFilterImpl(std::string const& id, std::string const& label, bool multi_select)
    : FilterBaseImpl(id),
    label_(label),
    multi_select_(multi_select)
{
}

OptionSelectorFilterImpl::OptionSelectorFilterImpl(VariantMap const& var)
    : FilterBaseImpl(var)
{
    deserialize(var);
}

std::string OptionSelectorFilterImpl::label() const
{
    return label_;
}

bool OptionSelectorFilterImpl::multi_select() const
{
    return multi_select_;
}

void OptionSelectorFilterImpl::serialize(VariantMap& var) const
{
    VariantArray ops = VariantArray();
    for (auto const& opt: options_)
    {
        VariantMap vm;
        vm["id"] = opt->id();
        vm["label"] = opt->label();
        ops.push_back(Variant(vm));
    }
    var["label"] = label_;
    var["options"] = ops;
    var["multi_select"] = multi_select_;
}

void OptionSelectorFilterImpl::throw_on_missing(VariantMap::const_iterator const& it, VariantMap::const_iterator const& endit, std::string const& name)
{
    if (it == endit)
    {
        throw LogicException("OptionSelectorFilter: missing " + name);
    }
}

void OptionSelectorFilterImpl::deserialize(VariantMap const& var)
{
    auto it = var.find("label");
    throw_on_missing(it, var.end(), "label");
    label_ = it->second.get_string();

    it = var.find("multi_select");
    throw_on_missing(it, var.end(), "multi_select");
    multi_select_ = it->second.get_bool();

    it = var.find("options");
    throw_on_missing(it, var.end(), "options");
    auto const opts = it->second.get_array();
    for (auto const& opt: opts)
    {
        auto const optvar = opt.get_dict();
        it = optvar.find("id");
        throw_on_missing(it, optvar.end(), "option id");
        auto opt_id = it->second.get_string();
        it = optvar.find("label");
        throw_on_missing(it, optvar.end(), "option label");
        add_option(opt_id, it->second.get_string());
    }
}

std::string OptionSelectorFilterImpl::filter_type() const
{
    return "option_selector";
}

FilterOption::SCPtr OptionSelectorFilterImpl::add_option(std::string const& id, std::string const& label)
{
    auto opt = std::shared_ptr<FilterOption>(new FilterOption(id, label));
    options_.push_back(opt);
    return opt;
}

std::list<FilterOption::SCPtr> OptionSelectorFilterImpl::options() const
{
    return options_;
}

std::set<FilterOption::SCPtr> OptionSelectorFilterImpl::active_options(FilterState const& filter_state) const
{
    std::set<FilterOption::SCPtr> opts;
    if (filter_state.has_filter(id()))
    {
        try
        {
            auto var = FilterBaseImpl::get(filter_state, id()).get_array(); // this can throw if of different type
            std::unordered_set<std::string> opts_ids;

            // create set for faster lookup
            for (auto const& idvar: var)
            {
                opts_ids.insert(idvar.get_string());
            }

            // insert selected options into opts
            for (auto const& opt: options_)
            {
                if (opts_ids.find(opt->id()) != opts_ids.end())
                {
                    opts.insert(opt);
                }
            }
        }
        catch (...)
        {
            // ignore all errors here - we could be getting an incorrect value for this filter
            // via a canned query from another scope, we shouldn't break this scope on it.
        }
    }
    return opts;
}

void OptionSelectorFilterImpl::update_state(FilterState& filter_state, FilterOption::SCPtr option, bool active) const
{
    auto const oid(option->id());
    auto it = std::find_if(options_.begin(), options_.end(), [oid](FilterOption::SCPtr const& opt) { return opt->id() == oid; });
    if (it == options_.end())
    {
        throw unity::InvalidArgumentException("OptionSelector::update_state(): unknown filter option: " + oid);
    }

    VariantMap& state = FilterBaseImpl::get(filter_state);
    // if this is single-selection filter, erase current state (only ensure only one option is active)
    if (active && !multi_select_)
    {
        auto it = state.find(id());
        if (it != state.end())
        {
            state.erase(it);
        }
    }
    update_state(filter_state, id(), option->id(), active);
}

void OptionSelectorFilterImpl::update_state(FilterState& filter_state, std::string const& filter_id, std::string const& option_id, bool value)
{
    VariantMap& state = FilterBaseImpl::get(filter_state);
    auto it = state.find(filter_id);
    // do we have this filter already?
    if (it == state.end())
    {
        if (value)
        {
            state[filter_id] = VariantArray({Variant(option_id)});
        }
        else
        {
            state[filter_id] = VariantArray(); // no option active
        }
    }
    else // modify existing entry for this filter
    {
        VariantArray var = (it->second).get_array(); // may throw if this filter was used for different filter type

        // do we have this option already?
        auto opt_it = std::find_if(var.begin(), var.end(), [option_id](Variant const& v1) { return v1.get_string() == option_id; });
        if (opt_it == var.end())
        {
            if (value)
            {
                var.push_back(Variant(option_id));
            } // else - option not selected, nothing to do
        }
        else // option already stored in the state
        {
            if (!value) // remove if it's now unselected
            {
                var.erase(opt_it);
            }
        }
        state[filter_id] = std::move(var);
    }
}

} // namespace internal

} // namespace scopes

} // namespace api

} // namespace unity
