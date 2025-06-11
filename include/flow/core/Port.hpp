// Copyright (c) 2024, Cisco Systems, Inc.
// All rights reserved.

#pragma once

#include "Core.hpp"
#include "IndexableName.hpp"
#include "NodeData.hpp"

#include <nlohmann/json_fwd.hpp>

#include <stdint.h>
#include <string_view>

FLOW_NAMESPACE_START

using json = nlohmann::json;

/**
 * @brief Defines a Port on a Node through which data can flow.
 *
 * @details A Port is the point of connection between nodes, through which the computed data flows.
 */
class Port
{
  public:
    /**
     * @brief Constructs a port with clarifying information and default data.
     * @param key The unique IndexableName of the Port.
     * @param caption A description or alternate name for the port.
     * @param type The typename of data that flows through the port.
     * @param data Default data, corresponding to the data type.
     * @param required Flag if this port requires non-null data (i.e. a reference type).
     * @param index The sortable index of the port in the Node.
     */
    Port(const IndexableName& key, const std::string& caption, std::string_view type, SharedNodeData data,
         bool required, std::size_t index);

    Port(const Port&)            = default;
    Port(Port&&)                 = default;
    Port& operator=(const Port&) = default;
    Port& operator=(Port&&)      = default;

    /**
     * @brief Checks if the port has a connection.
     * @returns true the port has a connection, false otherwise.
     */
    bool IsConnected() const noexcept { return _connected; }

    /**
     * @brief Mark the port as being connected.
     * @returns true if the connection was succesful, false if it is already connected.
     */
    bool Connect() noexcept;

    /**
     * @brief Mark the port as disconnected.
     * @returns true if the connection was severed, false if the port was not connected.
     */
    bool Disconnect() noexcept;

    /**
     * @brief Get the data currently being stored.
     * @returns The currently stored data pointer.
     */
    const SharedNodeData& GetData() const noexcept { return _data; }

    /**
     * @brief Get the unique hashable name of the Port.
     * @returns The IndeableName of the port that was provided on construction.
     */
    const IndexableName& GetKey() const noexcept { return _key; }

    /**
     * @brief Get the friendly name of the port.
     * @returns The string representation of the IndexableName key for the port.
     */
    std::string_view GetVarName() const noexcept { return std::string_view(_key); }

    /**
     * @brief Get the caption/alternate name of the Port.
     * @returns The friendly description/alternate name of the Port.
     */
    std::string_view GetCaption() const noexcept { return _caption; }

    /**
     * @brief Get the name of the data type currently stored.
     * @returns The current data typename, else returns the default typename.
     */
    std::string_view GetDataType() const noexcept { return _data ? _data->Type() : _type; }

    /**
     * @brief Check for if the port requires valid data.
     * @returns true if the port always requires valid (non-null) data, false otherwise.
     */
    bool IsRequired() const noexcept { return _required; }

    /**
     * @brief Get the index of the Port.
     * @returns The index of the port as it relates to the Node it belongs to.
     */
    std::size_t Index() const noexcept { return _index; }

    /**
     * @brief Store new data in the port.
     * @param data The new data to store.
     * @param output Flag if the changing data should trigger output down the connections.
     */
    void SetData(SharedNodeData data, bool output = false);

    /**
     * @brief Set a new caption for the port.
     * @param new_caption The new caption to set.
     */
    void SetCaption(std::string new_caption);

  private:
    std::shared_ptr<INodeData> _data;

    IndexableName _key   = IndexableName::None;
    std::string _caption = "";
    std::string _type    = "";
    bool _required       = false;
    bool _connected      = false;
    std::size_t _index   = 0;
};

using SharedPort = std::shared_ptr<Port>;

FLOW_NAMESPACE_END

template<>
struct std::less<FLOW_NAMESPACE::Port>
{
    std::uint64_t operator()(const FLOW_NAMESPACE::Port& lhs, const FLOW_NAMESPACE::Port& rhs) const
    {
        return lhs.Index() < rhs.Index();
    }
};

template<>
struct std::less<FLOW_NAMESPACE::SharedPort>
{
    std::uint64_t operator()(const FLOW_NAMESPACE::SharedPort& lhs, const FLOW_NAMESPACE::SharedPort& rhs) const
    {
        return lhs->Index() < rhs->Index();
    }
};
