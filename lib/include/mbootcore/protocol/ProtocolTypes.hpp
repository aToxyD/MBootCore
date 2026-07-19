#pragma once

// Platform
#include "CapabilityId.hpp"
#include "CapabilityDescriptor.hpp"
#include "CapabilitySet.hpp"
#include "ProtocolContext.hpp"
#include "ProtocolId.hpp"
#include "ProtocolResult.hpp"
#include "ProtocolVersion.hpp"
#include "SessionId.hpp"

#include "IProtocol.hpp"
#include "IProtocolDiscovery.hpp"
#include "IProtocolFactory.hpp"
#include "IProtocolSession.hpp"

// Semantic Vocabulary
#include "Command.hpp"
#include "CommandId.hpp"
#include "Event.hpp"
#include "MessageId.hpp"
#include "MessageMetadata.hpp"
#include "Payload.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "StatusCode.hpp"
#include "TransactionId.hpp"

// Serialization Layer
#include "PacketTypes.hpp"
