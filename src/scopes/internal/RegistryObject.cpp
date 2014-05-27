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
 * Authored by: Marcus Tomlinson <marcus.tomlinson@canonical.com>
 */

#include <unity/scopes/internal/RegistryObject.h>

#include <unity/scopes/internal/MWRegistry.h>
#include <unity/scopes/internal/RuntimeImpl.h>
#include <unity/scopes/ScopeExceptions.h>
#include <unity/UnityExceptions.h>

#include <core/posix/child_process.h>
#include <core/posix/exec.h>

#include <cassert>

using namespace std;

namespace unity
{

namespace scopes
{

namespace internal
{

RegistryObject::RegistryObject(core::posix::ChildProcess::DeathObserver& death_observer, Executor::SPtr const& executor,
                               MiddlewareBase::SPtr middleware)
    : death_observer_(death_observer),
      death_observer_connection_
      {
          death_observer_.child_died().connect([this](const core::posix::ChildProcess& cp)
          {
              on_process_death(cp);
          })
      },
      state_receiver_(new StateReceiverObject()),
      state_receiver_connection_
      {
          state_receiver_->state_received().connect([this](std::string const& id,
                                                    StateReceiverObject::State const& s)
          {
              on_state_received(id, s);
          })
      },
      executor_(executor)
{
    if (middleware)
    {
        try
        {
            publisher_ = middleware->create_publisher(middleware->runtime()->registry_identity());
        }
        catch (std::exception const& e)
        {
            std::cerr << "RegistryObject(): failed to create registry publisher: " << e.what() << endl;
        }
    }
}

RegistryObject::~RegistryObject()
{
    // kill all scope processes
    for (auto& scope_process : scope_processes_)
    {
        try
        {
            // at this point the registry middleware is shutting down, hence we will not receive
            // "ScopeStopping" states from dying scopes. We manually set it here as to avoid
            // outputting bogus error messages.
            if (is_scope_running(scope_process.first))
            {
                scope_process.second.update_state(ScopeProcess::Stopping);
                scope_process.second.kill();
            }
        }
        catch(std::exception const& e)
        {
            cerr << "RegistryObject::~RegistryObject(): " << e.what() << endl;
        }
    }

    // clear scope maps
    scopes_.clear();
    scope_processes_.clear();
}

ScopeMetadata RegistryObject::get_metadata(std::string const& scope_id) const
{
    if (scope_id.empty())
    {
        // If the id is empty, it was sent as empty by the remote client.
        throw unity::InvalidArgumentException("RegistryObject::get_metadata(): Cannot search for scope with empty id");
    }

    // Look for the scope in both the local and the remote map.
    // Local scopes take precedence over remote ones of the same id.
    // (Ideally, this should never happen.)
    {
        lock_guard<decltype(mutex_)> lock(mutex_);
        auto const& scope_it = scopes_.find(scope_id);
        if (scope_it != scopes_.end())
        {
            return scope_it->second;
        }
    }
    // Unlock, so we don't call the remote registry while holding a lock.

    if (remote_registry_)
    {
        try
        {
            return remote_registry_->get_metadata(scope_id);
        }
        catch (std::exception const& e)
        {
            cerr << "cannot get metdata from remote registry: " << e.what() << endl;
            // TODO: log error
        }
    }

    throw NotFoundException("RegistryObject::get_metadata(): no such scope: ",  scope_id);
}

MetadataMap RegistryObject::list() const
{
    MetadataMap all_scopes;  // Local scopes
    {
        lock_guard<decltype(mutex_)> lock(mutex_);
        all_scopes = scopes_;  // Local scopes
    }
    // Unlock, so we don't call the remote registry while holding a lock.

    // If a remote scope has the same id as a local one,
    // this will not overwrite a local scope with a remote
    // one if they have the same id.
    if (remote_registry_)
    {
        try
        {
            MetadataMap remote_scopes = remote_registry_->list();
            all_scopes.insert(remote_scopes.begin(), remote_scopes.end());
        }
        catch (std::exception const& e)
        {
            cerr << "cannot get scopes list from remote registry: " << e.what() << endl;
            // TODO: log error
        }
    }

    return all_scopes;
}

ObjectProxy RegistryObject::locate(std::string const& identity)
{
    // If the id is empty, it was sent as empty by the remote client.
    if (identity.empty())
    {
        throw unity::InvalidArgumentException("RegistryObject::locate(): Cannot locate scope with empty id");
    }

    ObjectProxy proxy;
    ProcessMap::iterator proc_it;
    {
        lock_guard<decltype(mutex_)> lock(mutex_);

        auto scope_it = scopes_.find(identity);
        if (scope_it == scopes_.end())
        {
            throw NotFoundException("RegistryObject::locate(): Tried to locate unknown local scope", identity);
        }
        proxy = scope_it->second.proxy();

        proc_it = scope_processes_.find(identity);
        if (proc_it == scope_processes_.end())
        {
            throw NotFoundException("RegistryObject::locate(): Tried to exec unknown local scope", identity);
        }
    }

    // Exec after unlocking, so we can start processing another locate()
    proc_it->second.exec(death_observer_, executor_);

    return proxy;
}

bool RegistryObject::add_local_scope(std::string const& scope_id, ScopeMetadata const& metadata,
                                     ScopeExecData const& exec_data)
{
    if (scope_id.empty())
    {
        throw unity::InvalidArgumentException("RegistryObject::add_local_scope(): Cannot add scope with empty id");
    }
    if (scope_id.find('/') != std::string::npos)
    {
        throw unity::InvalidArgumentException("RegistryObject::add_local_scope(): Cannot create a scope with '/' in its id");
    }

    lock_guard<decltype(mutex_)> lock(mutex_);

    bool return_value = true;
    if (scopes_.find(scope_id) != scopes_.end())
    {
        scopes_.erase(scope_id);
        scope_processes_.erase(scope_id);
        return_value = false;
    }
    scopes_.insert(make_pair(scope_id, metadata));
    scope_processes_.insert(make_pair(scope_id, ScopeProcess(exec_data)));

    if (publisher_)
    {
        // Send a blank message to subscribers to inform them that the registry has been updated
        publisher_->send_message("");
    }
    return return_value;
}

bool RegistryObject::remove_local_scope(std::string const& scope_id)
{
    // If the id is empty, it was sent as empty by the remote client.
    if (scope_id.empty())
    {
        throw unity::InvalidArgumentException("RegistryObject::remove_local_scope(): Cannot remove scope "
                                              "with empty id");
    }

    lock_guard<decltype(mutex_)> lock(mutex_);

    scope_processes_.erase(scope_id);

    if (scopes_.erase(scope_id) == 1)
    {
        if (publisher_)
        {
            // Send a blank message to subscribers to inform them that the registry has been updated
            publisher_->send_message("");
        }
        return true;
    }

    return false;
}

void RegistryObject::set_remote_registry(MWRegistryProxy const& remote_registry)
{
    lock_guard<decltype(mutex_)> lock(mutex_);
    remote_registry_ = remote_registry;
}

bool RegistryObject::is_scope_running(std::string const& scope_id)
{
    lock_guard<decltype(mutex_)> lock(mutex_);

    auto it = scope_processes_.find(scope_id);
    if (it != scope_processes_.end())
    {
        return it->second.state() != ScopeProcess::ProcessState::Stopped;
    }

    throw NotFoundException("RegistryObject::is_scope_process_running(): no such scope: ",  scope_id);
}

StateReceiverObject::SPtr RegistryObject::state_receiver()
{
    return state_receiver_;
}

void RegistryObject::on_process_death(core::posix::Process const& process)
{
    // The death observer has signaled that a child has died.
    // Broadcast this message to each scope process until we have found the process in question.
    // (This is slightly more efficient than just connecting the signal to every scope process.)
    pid_t pid = process.pid();
    for (auto& scope_process : scope_processes_)
    {
        if (scope_process.second.on_process_death(pid))
        {
            break;
        }
    }
}

void RegistryObject::on_state_received(std::string const& scope_id, StateReceiverObject::State const& state)
{
    auto it = scope_processes_.find(scope_id);
    if (it != scope_processes_.end())
    {
        switch (state)
        {
            case StateReceiverObject::ScopeReady:
                it->second.update_state(ScopeProcess::ProcessState::Running);
                break;
            case StateReceiverObject::ScopeStopping:
                it->second.update_state(ScopeProcess::ProcessState::Stopping);
                break;
            default:
                std::cerr << "RegistryObject::on_state_received(): unknown state received from scope: " << scope_id;
        }
    }
    // simply ignore states from scopes the registry does not know about
}

RegistryObject::ScopeProcess::ScopeProcess(ScopeExecData exec_data)
    : exec_data_(exec_data)
{
}

RegistryObject::ScopeProcess::ScopeProcess(ScopeProcess const& other)
    : exec_data_(other.exec_data_)
{
}

RegistryObject::ScopeProcess::~ScopeProcess()
{
    try
    {
        kill();
    }
    catch(std::exception const& e)
    {
        cerr << "RegistryObject::ScopeProcess::~ScopeProcess(): " << e.what() << endl;
    }
}

RegistryObject::ScopeProcess::ProcessState RegistryObject::ScopeProcess::state() const
{
    std::lock_guard<std::mutex> lock(process_mutex_);
    return state_;
}

void RegistryObject::ScopeProcess::update_state(ProcessState state)
{
    std::lock_guard<std::mutex> lock(process_mutex_);
    update_state_unlocked(state);
}

bool RegistryObject::ScopeProcess::wait_for_state(ProcessState state) const
{
    std::unique_lock<std::mutex> lock(process_mutex_);
    return wait_for_state(lock, state);
}

void RegistryObject::ScopeProcess::exec(
        core::posix::ChildProcess::DeathObserver& death_observer,
        Executor::SPtr executor)
{
    std::unique_lock<std::mutex> lock(process_mutex_);

    // 1. check if the scope is running.
    //  1.1. if scope already running, return.
    if (state_ == ScopeProcess::Running)
    {
        return;
    }
    //  1.2. if scope running but is “stopping”, wait for it to stop / kill it.
    else if (state_ == ScopeProcess::Stopping)
    {
        if (!wait_for_state(lock, ScopeProcess::Stopped))
        {
            cerr << "RegistryObject::ScopeProcess::exec(): Force killing process. Scope: \""
                 << exec_data_.scope_id << "\" took too long to stop." << endl;
            try
            {
                kill(lock);
            }
            catch(std::exception const& e)
            {
                cerr << "RegistryObject::ScopeProcess::exec(): " << e.what() << endl;
            }
        }
    }

    // 2. exec the scope.
    update_state_unlocked(Starting);

    std::string program = exec_data_.scoperunner_path;
    std::vector<std::string> argv = {exec_data_.runtime_config, exec_data_.scope_config};

    std::map<std::string, std::string> env;
    core::posix::this_process::env::for_each([&env](const std::string& key, const std::string& value)
    {
        env.insert(std::make_pair(key, value));
    });

    {
        process_ = executor->exec(program, argv, env,
                                     core::posix::StandardStream::stdin | core::posix::StandardStream::stdout,
                                     exec_data_.confinement_profile);
        if (process_.pid() <= 0)
        {
            clear_handle_unlocked();
            throw unity::ResourceException("RegistryObject::ScopeProcess::exec(): Failed to exec scope via command: \""
                                           + exec_data_.scoperunner_path + " " + exec_data_.runtime_config + " "
                                           + exec_data_.scope_config + "\"");
        }
    }

    // 3. wait for scope to be "running".
    //  3.1. when ready, return.
    //  3.2. OR if timeout, kill process and throw.
    if (!wait_for_state(lock, ScopeProcess::Running))
    {
        try
        {
            kill(lock);
        }
        catch(std::exception const& e)
        {
            cerr << "RegistryObject::ScopeProcess::exec(): " << e.what() << endl;
        }
        throw unity::ResourceException("RegistryObject::ScopeProcess::exec(): exec aborted. Scope: \""
                                       + exec_data_.scope_id + "\" took too long to start.");
    }

    cout << "RegistryObject::ScopeProcess::exec(): Process for scope: \"" << exec_data_.scope_id << "\" started" << endl;

    // 4. add the scope process to the death observer
    death_observer.add(process_);
}

void RegistryObject::ScopeProcess::kill()
{
    std::unique_lock<std::mutex> lock(process_mutex_);
    kill(lock);
}

bool RegistryObject::ScopeProcess::on_process_death(pid_t pid)
{
    std::lock_guard<std::mutex> lock(process_mutex_);

    // check if this is the process reported to have died
    if (pid == process_.pid())
    {
        cout << "RegistryObject::ScopeProcess::on_process_death(): Process for scope: \"" << exec_data_.scope_id
             << "\" terminated" << endl;
        clear_handle_unlocked();
        return true;
    }

    return false;
}

void RegistryObject::ScopeProcess::clear_handle_unlocked()
{
    process_ = core::posix::ChildProcess::invalid();
    update_state_unlocked(Stopped);
}

void RegistryObject::ScopeProcess::update_state_unlocked(ProcessState state)
{
    if (state == state_)
    {
        return;
    }
    else if (state == Running && state_ != Starting)
    {
        cout << "RegistryObject::ScopeProcess: Process for scope: \"" << exec_data_.scope_id
             << "\" started manually" << endl;

        // Don't update state, treat this scope as not running if a locate() is requested
        return;
    }
    else if (state == Stopping && state_ != Running)
    {
        cout << "RegistryObject::ScopeProcess: Manually started process for scope: \""
             << exec_data_.scope_id << "\" terminated" << endl;

        // Don't update state, treat this scope as not running if a locate() is requested
        return;
    }
    else if (state == Stopped && state_ != Stopping)
    {
        cerr << "RegistryObject::ScopeProcess: Scope: \"" << exec_data_.scope_id
             << "\" closed unexpectedly. Either the process crashed or was killed forcefully." << endl;
    }
    state_ = state;
    state_change_cond_.notify_all();
}

bool RegistryObject::ScopeProcess::wait_for_state(std::unique_lock<std::mutex>& lock, ProcessState state) const
{
    return state_change_cond_.wait_for(lock,
                                       std::chrono::milliseconds(exec_data_.timeout_ms),
                                       [this, &state]{return state_ == state;});
}

void RegistryObject::ScopeProcess::kill(std::unique_lock<std::mutex>& lock)
{
    if (state_ == Stopped)
    {
        return;
    }

    try
    {
        // first try to close the scope process gracefully
        process_.send_signal_or_throw(core::posix::Signal::sig_term);

        if (!wait_for_state(lock, ScopeProcess::Stopped))
        {
            std::error_code ec;

            cerr << "RegistryObject::ScopeProcess::kill(): Scope: \"" << exec_data_.scope_id
                 << "\" is taking longer than expected to terminate gracefully. "
                 << "Killing the process instead." << endl;

            // scope is taking too long to close, send kill signal
            process_.send_signal(core::posix::Signal::sig_kill, ec);
        }

        // clear the process handle
        clear_handle_unlocked();
    }
    catch (std::exception const&)
    {
        cerr << "RegistryObject::ScopeProcess::kill(): Failed to kill scope: \""
             << exec_data_.scope_id << "\"" << endl;

        // clear the process handle
        // even on error, the previous handle will be unrecoverable at this point
        clear_handle_unlocked();
        throw;
    }
}

} // namespace internal

} // namespace scopes

} // namespace unity
